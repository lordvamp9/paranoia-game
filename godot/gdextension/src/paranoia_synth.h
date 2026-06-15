// PARANOIA — AudioSynth GDExtension (C++ port of src/audio.js)
// 100% procedural: no audio files. A small software synthesizer (white noise,
// oscillators, RBJ biquad filters, piecewise envelopes, stress-driven tanh
// drive) mixes all active voices and pushes stereo frames into an
// AudioStreamGenerator playback. Mirrors the WebAudio graph from audio.js.
#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/audio_stream_player.hpp>
#include <godot_cpp/classes/audio_stream_generator_playback.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/string.hpp>

#include <vector>
#include <unordered_map>
#include <cstdint>

namespace godot {

// One RBJ biquad (lowpass / highpass / bandpass), recomputable per-sample.
struct Biquad {
    enum Type { NONE, LOWPASS, HIGHPASS, BANDPASS };
    Type type = NONE;
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; // a0-normalized
    double x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    void set(Type t, double freq, double q, double sr);
    double process(double x);
};

// A single scheduled gain segment (absolute seconds from voice start).
struct EnvSeg {
    double t0, t1;
    float v0, v1;
    bool exp; // exponential ramp toward v1 (else linear)
};

struct Voice {
    enum Source { NOISE, SINE, SAW, SQUARE, TRIANGLE };
    Source source = NOISE;
    double freq = 0.0;      // oscillator base frequency (Hz)
    double detune = 0.0;    // cents
    double phase = 0.0;

    Biquad filt;
    // Optional bandpass center-frequency automation (whisper formant wobble).
    std::vector<std::pair<double, double>> freqAuto; // {time, centerHz}

    std::vector<EnvSeg> env;

    bool hasPan = false;
    float pan = 0.0f;       // -1..1 (equal-power)

    bool bypassDrive = false; // route straight to master (raw hit / breathing)
    float gainMul = 1.0f;

    double age = 0.0;
    double duration = 0.0;  // voice freed when age > duration (0 = persistent)
    bool persistent = false;
    bool dead = false;

    float eval_env() const;
};

// Per-entity positional breath voice (sine + sub triangle), retriggered.
struct EntityVoice {
    double phaseO = 0.0, phaseSub = 0.0;
    double freqO = 95.0;
    float gain = 0.0f;          // current breath amplitude
    std::vector<EnvSeg> env;    // active breath envelope
    double age = 0.0;
    float atten = 1.0f;         // distance attenuation 0..1
    bool dead = false;
};

class AudioSynth : public Node {
    GDCLASS(AudioSynth, Node)

public:
    AudioSynth();
    ~AudioSynth();

    void _ready() override;
    void _process(double delta) override;

    // ---- public API (mirrors audio.js) ----
    void start();
    void stop();
    void set_stress(float s);
    void set_wind(float level);

    void play_wind(float intensity);
    void play_footstep(const String &surface);
    void play_branch_crack();
    void play_whisper();
    void play_drip();
    void play_sting();
    void play_scare();
    void play_pickup();
    void play_unlock();
    void play_ui_beep();

    void play_entity_breath(int entity_id, float distance);
    void stop_entity_breath(int entity_id);

    // Headless DSP verification: renders `seconds` of audio into a scratch
    // buffer (not played) and returns peak absolute amplitude. Proves the
    // synth produces signal without needing an audio device.
    float render_test(float seconds);

protected:
    static void _bind_methods();

private:
    void setup_stream();
    void fill_available();
    // Renders one stereo frame, advancing all voice state by 1/sr seconds.
    void render_frame(float &out_l, float &out_r);
    void drive_amount_from_stress();

    Voice make_voice() const;
    void add_voice(const Voice &v);

    AudioStreamPlayer *_player = nullptr;
    Ref<AudioStreamGeneratorPlayback> _playback;

    double _sr = 44100.0;
    double _time = 0.0;
    bool _started = false;

    float _master = 0.9f;
    float _stress = 0.0f;
    double _driveK = 1.0;       // tanh drive sharpness (1 = ~clean)

    // Persistent wind voice state.
    Biquad _windFilt;
    double _windPhaseNoise = 0.0;
    float _windTarget = 0.09f;  // smoothed level target
    float _windLevel = 0.0f;

    std::vector<Voice> _voices;
    std::unordered_map<int, EntityVoice> _entities;

    uint32_t _rng = 0x9e3779b9u;
    inline float noise() {
        _rng ^= _rng << 13; _rng ^= _rng >> 17; _rng ^= _rng << 5;
        return (float)((int32_t)_rng) / 2147483648.0f; // -1..1
    }
    inline float frand() { return (noise() + 1.0f) * 0.5f; } // 0..1
};

} // namespace godot
