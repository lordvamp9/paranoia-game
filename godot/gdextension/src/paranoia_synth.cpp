// PARANOIA — AudioSynth implementation (port of src/audio.js DSP graph).
#include "paranoia_synth.h"

#include <godot_cpp/classes/audio_stream_generator.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cmath>
#include <algorithm>

using namespace godot;

static const double TAU = 6.283185307179586;

// ---------------- Biquad (RBJ cookbook) ----------------
void Biquad::set(Type t, double freq, double q, double sr) {
    type = t;
    if (t == NONE) { b0 = 1; b1 = b2 = a1 = a2 = 0; return; }
    if (freq < 10.0) freq = 10.0;
    if (freq > sr * 0.45) freq = sr * 0.45;
    if (q < 0.0001) q = 0.0001;
    double w0 = TAU * freq / sr;
    double cw = std::cos(w0), sw = std::sin(w0);
    double alpha = sw / (2.0 * q);
    double a0;
    double nb0, nb1, nb2, na1, na2;
    switch (t) {
        case LOWPASS:
            nb0 = (1 - cw) / 2; nb1 = 1 - cw; nb2 = (1 - cw) / 2;
            a0 = 1 + alpha; na1 = -2 * cw; na2 = 1 - alpha; break;
        case HIGHPASS:
            nb0 = (1 + cw) / 2; nb1 = -(1 + cw); nb2 = (1 + cw) / 2;
            a0 = 1 + alpha; na1 = -2 * cw; na2 = 1 - alpha; break;
        case BANDPASS: // constant 0 dB peak gain
            nb0 = alpha; nb1 = 0; nb2 = -alpha;
            a0 = 1 + alpha; na1 = -2 * cw; na2 = 1 - alpha; break;
        default:
            b0 = 1; b1 = b2 = a1 = a2 = 0; return;
    }
    b0 = nb0 / a0; b1 = nb1 / a0; b2 = nb2 / a0;
    a1 = na1 / a0; a2 = na2 / a0;
}

double Biquad::process(double x) {
    if (type == NONE) return x;
    double y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1; x1 = x; y2 = y1; y1 = y;
    return y;
}

// ---------------- Envelope ----------------
float Voice::eval_env() const {
    if (env.empty()) return gainMul;
    if (age <= env.front().t0) return env.front().v0 * gainMul;
    for (const EnvSeg &s : env) {
        if (age >= s.t0 && age <= s.t1) {
            double span = s.t1 - s.t0;
            double f = span > 1e-9 ? (age - s.t0) / span : 1.0;
            float v;
            if (s.exp) {
                float v0 = std::max(s.v0, 1e-5f);
                float v1 = std::max(s.v1, 1e-5f);
                v = (float)(v0 * std::pow((double)v1 / v0, f));
            } else {
                v = (float)(s.v0 + (s.v1 - s.v0) * f);
            }
            return v * gainMul;
        }
    }
    return env.back().v1 * gainMul;
}

// ---------------- AudioSynth ----------------
AudioSynth::AudioSynth() {}
AudioSynth::~AudioSynth() {}

void AudioSynth::_bind_methods() {
    ClassDB::bind_method(D_METHOD("start"), &AudioSynth::start);
    ClassDB::bind_method(D_METHOD("stop"), &AudioSynth::stop);
    ClassDB::bind_method(D_METHOD("set_stress", "s"), &AudioSynth::set_stress);
    ClassDB::bind_method(D_METHOD("set_wind", "level"), &AudioSynth::set_wind);

    ClassDB::bind_method(D_METHOD("play_wind", "intensity"), &AudioSynth::play_wind);
    ClassDB::bind_method(D_METHOD("play_footstep", "surface"), &AudioSynth::play_footstep);
    ClassDB::bind_method(D_METHOD("play_branch_crack"), &AudioSynth::play_branch_crack);
    ClassDB::bind_method(D_METHOD("play_whisper"), &AudioSynth::play_whisper);
    ClassDB::bind_method(D_METHOD("play_drip"), &AudioSynth::play_drip);
    ClassDB::bind_method(D_METHOD("play_sting"), &AudioSynth::play_sting);
    ClassDB::bind_method(D_METHOD("play_scare"), &AudioSynth::play_scare);
    ClassDB::bind_method(D_METHOD("play_pickup"), &AudioSynth::play_pickup);
    ClassDB::bind_method(D_METHOD("play_unlock"), &AudioSynth::play_unlock);
    ClassDB::bind_method(D_METHOD("play_ui_beep"), &AudioSynth::play_ui_beep);

    ClassDB::bind_method(D_METHOD("play_entity_breath", "entity_id", "distance"),
                         &AudioSynth::play_entity_breath);
    ClassDB::bind_method(D_METHOD("stop_entity_breath", "entity_id"),
                         &AudioSynth::stop_entity_breath);

    ClassDB::bind_method(D_METHOD("render_test", "seconds"), &AudioSynth::render_test);
}

void AudioSynth::_ready() {
    setup_stream();
    set_process(true);
    start();
}

void AudioSynth::setup_stream() {
    if (_player) return;
    _player = memnew(AudioStreamPlayer);
    Ref<AudioStreamGenerator> gen;
    gen.instantiate();
    gen->set_mix_rate((float)_sr);
    gen->set_buffer_length(0.1f);
    _player->set_stream(gen);
    add_child(_player);
    _player->play();
    Ref<AudioStreamPlayback> pb = _player->get_stream_playback();
    _playback = pb;
    _windFilt.set(Biquad::BANDPASS, 280.0, 0.7, _sr);
}

void AudioSynth::_process(double /*delta*/) {
    if (!_started) return;
    fill_available();
}

void AudioSynth::fill_available() {
    if (_playback.is_null()) return;
    int n = _playback->get_frames_available();
    if (n <= 0) return;
    PackedVector2Array buf;
    buf.resize(n);
    Vector2 *w = buf.ptrw();
    for (int i = 0; i < n; i++) {
        float l, r;
        render_frame(l, r);
        w[i] = Vector2(l, r);
    }
    _playback->push_buffer(buf);
}

void AudioSynth::drive_amount_from_stress() {
    // audio.js: amount = max(0,(s-55)/45)*0.8 ; k = 1 + amount*18
    double amount = std::max(0.0, ((double)_stress - 55.0) / 45.0) * 0.8;
    _driveK = 1.0 + amount * 18.0;
}

// Synthesize a single stereo frame, advancing all state by 1/sr.
void AudioSynth::render_frame(float &out_l, float &out_r) {
    const double dt = 1.0 / _sr;
    double preL = 0.0, preR = 0.0;   // routed through stress drive
    double byL = 0.0, byR = 0.0;     // bypass drive (raw)

    // ---- persistent wind ----
    if (_started) {
        // LFO-modulated band-pass center + gusty gain (audio.js _startWind)
        double center = 280.0 + 90.0 * std::sin(TAU * 0.07 * _time);
        _windFilt.set(Biquad::BANDPASS, center, 0.7, _sr);
        // smooth toward target (setTargetAtTime ~1.5s)
        double tau = 1.5;
        _windLevel += (float)((_windTarget - _windLevel) * (dt / tau));
        double gust = 0.035 * std::sin(TAU * 0.11 * _time);
        double g = std::max(0.0, (double)_windLevel + gust);
        double w = _windFilt.process(noise()) * g;
        preL += w; preR += w;
    }

    // ---- generic voices ----
    for (Voice &v : _voices) {
        if (v.dead) continue;
        // source
        double s;
        if (v.source == Voice::NOISE) {
            s = noise();
        } else {
            double f = v.freq * std::pow(2.0, v.detune / 1200.0);
            v.phase += TAU * f * dt;
            if (v.phase > TAU) v.phase -= TAU;
            double ph = v.phase / TAU; // 0..1
            switch (v.source) {
                case Voice::SINE:     s = std::sin(v.phase); break;
                case Voice::SAW:      s = 2.0 * ph - 1.0; break;
                case Voice::SQUARE:   s = ph < 0.5 ? 1.0 : -1.0; break;
                case Voice::TRIANGLE: s = 4.0 * std::fabs(ph - 0.5) - 1.0; break;
                default:              s = 0.0; break;
            }
        }
        // bandpass center automation (whisper formants)
        if (!v.freqAuto.empty() && v.filt.type != Biquad::NONE) {
            double c = v.freqAuto.back().second;
            for (size_t k = 0; k + 1 < v.freqAuto.size(); k++) {
                if (v.age >= v.freqAuto[k].first && v.age < v.freqAuto[k + 1].first) {
                    double t0 = v.freqAuto[k].first, t1 = v.freqAuto[k + 1].first;
                    double f = (t1 - t0) > 1e-9 ? (v.age - t0) / (t1 - t0) : 0.0;
                    c = v.freqAuto[k].second + (v.freqAuto[k + 1].second - v.freqAuto[k].second) * f;
                    break;
                }
            }
            v.filt.set(Biquad::BANDPASS, c, 6.0, _sr);
        }
        s = v.filt.process(s);
        s *= v.eval_env();

        double l, r;
        if (v.hasPan) {
            double a = (v.pan + 1.0) * 0.5 * (TAU * 0.25); // 0..pi/2
            l = s * std::cos(a); r = s * std::sin(a);
        } else {
            l = s; r = s;
        }
        if (v.bypassDrive) { byL += l; byR += r; }
        else { preL += l; preR += r; }

        v.age += dt;
        if (!v.persistent && v.age > v.duration) v.dead = true;
    }

    // ---- entity breath voices ----
    for (auto &kv : _entities) {
        EntityVoice &e = kv.second;
        if (e.dead) continue;
        e.phaseO += TAU * e.freqO * dt;   if (e.phaseO > TAU) e.phaseO -= TAU;
        e.phaseSub += TAU * 47.0 * dt;    if (e.phaseSub > TAU) e.phaseSub -= TAU;
        double osc = std::sin(e.phaseO);
        double phs = e.phaseSub / TAU;
        double sub = 4.0 * std::fabs(phs - 0.5) - 1.0; // triangle 47Hz
        // breath envelope (linear segments)
        if (!e.env.empty()) {
            float g = e.env.back().v1;
            if (e.age <= e.env.front().t0) g = e.env.front().v0;
            else for (const EnvSeg &sg : e.env) {
                if (e.age >= sg.t0 && e.age <= sg.t1) {
                    double span = sg.t1 - sg.t0;
                    double f = span > 1e-9 ? (e.age - sg.t0) / span : 1.0;
                    g = (float)(sg.v0 + (sg.v1 - sg.v0) * f);
                    break;
                }
            }
            e.gain = g;
        }
        double s = (osc + sub) * 0.5 * e.gain * e.atten;
        preL += s; preR += s;
        e.age += dt;
    }

    // ---- stress drive (tanh waveshaper), then bypass, then master ----
    double thK = std::tanh(_driveK);
    double dl = std::tanh(_driveK * preL) / thK;
    double dr = std::tanh(_driveK * preR) / thK;
    double l = (dl + byL) * _master;
    double r = (dr + byR) * _master;
    out_l = (float)std::max(-1.0, std::min(1.0, l));
    out_r = (float)std::max(-1.0, std::min(1.0, r));

    _time += dt;

    // reap dead voices occasionally (cheap, bounded)
    if (!_voices.empty()) {
        _voices.erase(std::remove_if(_voices.begin(), _voices.end(),
                      [](const Voice &v) { return v.dead; }), _voices.end());
    }
}

Voice AudioSynth::make_voice() const { return Voice(); }
void AudioSynth::add_voice(const Voice &v) { _voices.push_back(v); }

// ---------------- public API ----------------
void AudioSynth::start() {
    _started = true;
    _windTarget = 0.09f;
    drive_amount_from_stress();
}

void AudioSynth::stop() { _started = false; }

void AudioSynth::set_stress(float s) {
    _stress = s;
    drive_amount_from_stress();
}

void AudioSynth::set_wind(float level) {
    _windTarget = 0.02f + level * 0.1f; // audio.js setWind
}

void AudioSynth::play_wind(float intensity) {
    _started = true;
    set_wind(intensity);
}

void AudioSynth::play_footstep(const String &surface) {
    String s = surface.to_lower();
    bool hard = (s == "wood" || s == "concrete" || s == "stone" || s == "hard" || s == "tile");
    Voice v;
    v.source = Voice::NOISE;
    v.filt.set(Biquad::LOWPASS, hard ? 1800.0 : 700.0, 1.0, _sr);
    double dur = hard ? 0.13 : 0.19;
    v.env = {
        {0.0, 0.012, 0.0f, 0.14f, false},
        {0.012, dur, 0.14f, 0.001f, true},
    };
    v.duration = 0.25;
    add_voice(v);
}

void AudioSynth::play_branch_crack() {
    Voice v;
    v.source = Voice::NOISE;
    v.filt.set(Biquad::HIGHPASS, 900.0, 1.0, _sr);
    v.env = {{0.0, 0.09, 0.5f, 0.001f, true}};
    v.hasPan = true; v.pan = -0.7f;
    v.duration = 0.12;
    add_voice(v);
}

void AudioSynth::play_whisper() {
    double dur = 0.9 + frand() * 1.4;
    Voice v;
    v.source = Voice::NOISE;
    v.filt.set(Biquad::BANDPASS, 900.0 + frand() * 600.0, 6.0, _sr);
    for (int i = 0; i < 8; i++)
        v.freqAuto.push_back({dur * i / 8.0, 700.0 + frand() * 1500.0});
    float peak = 0.05f + _stress * 0.0006f;
    v.env = {
        {0.0, dur * 0.3, 0.0f, peak, false},
        {dur * 0.3, dur, peak, 0.0f, false},
    };
    v.hasPan = true; v.pan = frand() * 2.0f - 1.0f;
    v.duration = dur + 0.1;
    add_voice(v);
}

void AudioSynth::play_drip() {
    Voice v;
    v.source = Voice::SINE;
    v.freq = 1900.0; // swept down via env-free pitch glide approximated by env on a sine? use osc glide
    // pitch glide 1900->500 over 0.09s: emulate with two short voices is overkill;
    // approximate timbre with sine + fast amplitude decay.
    v.env = {{0.0, 0.25, 0.07f, 0.001f, true}};
    v.duration = 0.3;
    add_voice(v);
    // descending chirp: a second quick sine sweep stand-in
    Voice c;
    c.source = Voice::SINE; c.freq = 900.0;
    c.env = {{0.0, 0.09, 0.05f, 0.001f, true}};
    c.duration = 0.1;
    add_voice(c);
}

void AudioSynth::play_sting() {
    for (int i = 0; i < 4; i++) {
        Voice v;
        v.source = Voice::SAW;
        v.freq = 55.0 * (i + 1) * (1.0 + frand() * 0.06);
        v.detune = (frand() - 0.5) * 90.0;
        v.env = {
            {0.0, 0.4, 0.001f, 0.12f, true},
            {0.4, 2.2, 0.12f, 0.001f, true},
        };
        v.duration = 2.4;
        add_voice(v);
    }
}

void AudioSynth::play_scare() {
    // raw noise hit bypassing drive + amplified sting
    Voice v;
    v.source = Voice::NOISE;
    v.env = {{0.0, 0.8, 0.5f, 0.001f, true}};
    v.bypassDrive = true;
    v.duration = 0.9;
    add_voice(v);
    // sting at 1.6 intensity
    for (int i = 0; i < 4; i++) {
        Voice s;
        s.source = Voice::SAW;
        s.freq = 55.0 * (i + 1) * (1.0 + frand() * 0.06);
        s.detune = (frand() - 0.5) * 90.0;
        s.env = {
            {0.0, 0.4, 0.001f, 0.12f * 1.6f, true},
            {0.4, 2.2, 0.12f * 1.6f, 0.001f, true},
        };
        s.duration = 2.4;
        add_voice(s);
    }
}

void AudioSynth::play_pickup() {
    double freqs[2] = {620.0, 930.0};
    for (int i = 0; i < 2; i++) {
        Voice v;
        v.source = Voice::TRIANGLE;
        v.freq = freqs[i];
        double off = i * 0.07;
        v.env = {
            {off, off + 0.02, 0.0f, 0.06f, false},
            {off + 0.02, off + 0.2, 0.06f, 0.001f, true},
        };
        v.duration = off + 0.25;
        add_voice(v);
    }
}

void AudioSynth::play_unlock() {
    Voice v; // bandpassed noise click
    v.source = Voice::NOISE;
    v.filt.set(Biquad::BANDPASS, 2400.0, 9.0, _sr);
    v.env = {
        {0.0, 0.03, 0.001f, 0.25f, true},
        {0.03, 0.3, 0.25f, 0.001f, true},
    };
    v.duration = 0.35;
    add_voice(v);
    Voice o; // low sine thunk
    o.source = Voice::SINE; o.freq = 140.0;
    o.env = {
        {0.05, 0.05, 0.0f, 0.15f, false},
        {0.05, 0.45, 0.15f, 0.001f, true},
    };
    o.duration = 0.5;
    add_voice(o);
}

void AudioSynth::play_ui_beep() {
    Voice v;
    v.source = Voice::SQUARE;
    v.freq = 880.0;
    v.env = {{0.0, 0.12, 0.045f, 0.001f, true}};
    v.duration = 0.14;
    add_voice(v);
}

void AudioSynth::play_entity_breath(int entity_id, float distance) {
    EntityVoice &e = _entities[entity_id];
    if (e.freqO < 80.0) e.freqO = 85.0 + frand() * 30.0; // init once
    e.dead = false;
    // distance attenuation (exponential rolloff, refDistance 2)
    double d = std::max(1.0, (double)distance);
    e.atten = (float)std::min(1.0, std::pow(d / 2.0, -1.4));
    // retrigger an irregular breath
    bool aggressive = distance < 6.0f;
    float peak = aggressive ? 0.5f : 0.25f;
    double up = 0.25 + frand() * 0.3;
    double down = 0.9 + frand() * 0.6;
    e.age = 0.0;
    e.env = {
        {0.0, up, e.gain, peak, false},
        {up, up + down, peak, 0.02f, false},
    };
    // articulation click when close
    if (aggressive) {
        Voice c;
        c.source = Voice::NOISE;
        c.filt.set(Biquad::HIGHPASS, 2500.0, 1.0, _sr);
        c.env = {{0.0, 0.04, 0.4f * e.atten, 0.001f, true}};
        c.duration = 0.05;
        add_voice(c);
    }
}

void AudioSynth::stop_entity_breath(int entity_id) {
    auto it = _entities.find(entity_id);
    if (it != _entities.end()) _entities.erase(it);
}

float AudioSynth::render_test(float seconds) {
    bool was = _started;
    _started = true;
    int n = (int)(seconds * _sr);
    float peak = 0.0f;
    for (int i = 0; i < n; i++) {
        float l, r;
        render_frame(l, r);
        peak = std::max(peak, std::max(std::fabs(l), std::fabs(r)));
    }
    _started = was;
    return peak;
}
