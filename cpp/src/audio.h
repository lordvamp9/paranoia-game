// PARANOIA v2 — procedural cinematic audio engine.
// Everything is synthesized at 48kHz in the audio callback: layered wind,
// gravel footsteps, breath, whispers with moving formants, entity growls,
// Samara's pitch-falling groans, Schroeder reverb per environment.
// No bleeps. No retro. Only things that breathe.
#pragma once
#include "game.h"

#define SR 48000.0f
#define NVOICES 48

static unsigned int gRng = 0x9E3779B9u;
static inline float arand() { gRng ^= gRng << 13; gRng ^= gRng >> 17; gRng ^= gRng << 5; return (gRng & 0xFFFFFF) / 8388608.0f - 1.0f; }

enum VType { V_OFF = 0, V_NOISE, V_TONE, V_GROAN, V_BREATH, V_WHISPER, V_GROWL, V_THUMP, V_PIANO };
enum ABus { BUS_SFX = 0, BUS_AMBIENT, BUS_VOICE, BUS_MUSIC };

// per-bus gains, driven by the settings menu (atomic-ish floats)
static volatile float gVolMaster = 0.9f, gVolMusic = 0.7f, gVolAmbient = 0.8f, gVolVoices = 1.0f;
static volatile float gRainLvl = 0.0f, gRainTarget = 0.0f;
static volatile float gMusicGain = 0.0f, gMusicTarget = 0.0f; // overall music swell

struct Voice {
  volatile int type = V_OFF;
  int bus = BUS_SFX;
  // piano partials
  float pf[6]; float pa[6]; float pp[6];
  // common
  float t = 0, dur = 0.2f, att = 0.005f, gain = 0.2f, pan = 0, rev = 0.2f;
  // filter (state-variable)
  float f0 = 800, f1 = 800, q = 1.0f;
  float lp = 0, bp = 0;
  int band = 1;          // 0 lowpass, 1 bandpass, 2 highpass
  // tone/groan
  float freq0 = 100, freq1 = 100, phase = 0;
  int wave = 0;          // 0 sine, 1 saw, 2 tri
  // loop voices (growl): externally driven target gain/pan
  volatile float tGain = 0, tPan = 0, tCut = 2000;
  float sGain = 0, sPan = 0, sCut = 2000;
  float wPhase = 0, wDrift = 0;
  float lpDist = 0;      // distance lowpass state
};
static Voice gVoices[NVOICES];

// listener
static volatile float gLisX = 0, gLisZ = 200, gLisYaw = 0;
static volatile float gStressA = 0, gWindLvl = 1.0f;
static volatile int gEnv = 0;

// wind + room state
static float wBrownL = 0, wBrownR = 0, wLfo1 = 0, wLfo2 = 0.7f, wLp1 = 0, wBp1 = 0, wLp2 = 0, wBp2 = 0;
static float wGust = 0.5f, wGustT = 0;
static float rumbleLp = 0;

// Schroeder reverb
#define NCOMB 4
static float combBufL[NCOMB][2048], combBufR[NCOMB][2048];
static int combIdx[NCOMB] = { 0 };
static const int combLen[NCOMB] = { 1426, 1781, 1973, 2098 % 2048 };
static float apBufL[480], apBufR[440];
static int apIdxL = 0, apIdxR = 0;
static volatile float gRevFb = 0.78f, gRevWet = 0.12f;

static int FindVoice() {
  for (int i = 8; i < NVOICES; i++) if (gVoices[i].type == V_OFF) return i;
  return -1;
}

static inline float BusGain(int bus) {
  switch (bus) {
    case BUS_MUSIC: return gVolMusic;
    case BUS_AMBIENT: return gVolAmbient;
    case BUS_VOICE: return gVolVoices;
    default: return 1.0f; // sfx ride master only
  }
}

// spatialize: returns gain & pan & lp cutoff for a world position
static void Spatial(Vector3 p, float& gain, float& pan, float& cutoff) {
  float dx = p.x - gLisX, dz = p.z - gLisZ;
  float d = sqrtf(dx * dx + dz * dz);
  gain = 1.0f / (1.0f + d * 0.30f + d * d * 0.012f);
  float az = atan2f(dx, -dz) - (-gLisYaw); // listener yaw
  pan = Clamp(sinf(az), -0.9f, 0.9f);
  cutoff = Clamp(9000.0f / (1.0f + d * 0.25f), 320.0f, 9000.0f);
}

// ---------------------------------------------------------------- callback
static void AudioCB(void* buffer, unsigned int frames) {
  float* out = (float*)buffer;
  float stress = gStressA;
  float windAmt = gWindLvl;
  int env = gEnv;
  float envWet[4] = { 0.10f, 0.22f, 0.42f, 0.16f };
  float wet = envWet[env & 3];

  for (unsigned int i = 0; i < frames; i++) {
    float L = 0, R = 0, revL = 0, revR = 0;

    gRainLvl += (gRainTarget - gRainLvl) * 0.00006f;
    gMusicGain += (gMusicTarget - gMusicGain) * 0.00004f;

    // ---- rain: dense high noise + spatial spatter, an AMBIENT bed
    if (gRainLvl > 0.001f) {
      float rn = arand();
      static float rlpA = 0, rlpB = 0;
      rlpA += 0.5f * (rn - rlpA);
      float hiss = (rn - rlpA);
      rlpB += 0.02f * (arand() - rlpB);
      float body = rlpB * 2.0f;
      float ra = gRainLvl * gVolAmbient * 0.5f;
      L += (hiss * 0.5f + body * 0.4f) * ra;
      R += (hiss * 0.5f - body * 0.4f + arand() * 0.1f) * ra;
      revL += body * ra * 0.4f; revR += body * ra * 0.4f;
    }

    // ---- wind: twin decorrelated brown noise through swept bandpass
    wGustT -= 1.0f / SR;
    if (wGustT <= 0) { wGustT = 2.0f + (arand() * 0.5f + 0.5f) * 5.0f; }
    float gustTarget = 0.35f + (arand() * 0.5f + 0.5f) * 0.0f + (sinf(wLfo1 * 0.13f) * 0.5f + 0.5f) * 0.65f;
    wGust += (gustTarget - wGust) * 0.00002f;
    wLfo1 += 0.45f / SR; wLfo2 += 0.31f / SR;
    float cutoff = 240.0f + sinf(wLfo1 * 6.28f) * 120.0f + wGust * 160.0f;
    float f = 2.0f * sinf(PI * cutoff / SR);
    wBrownL += arand() * 0.02f; wBrownL *= 0.998f;
    wBrownR += arand() * 0.02f; wBrownR *= 0.998f;
    wLp1 += f * wBp1; float hp1 = wBrownL * 2.2f - wLp1 - 0.6f * wBp1; wBp1 += f * hp1;
    wLp2 += f * 0.93f * wBp2; float hp2 = wBrownR * 2.2f - wLp2 - 0.6f * wBp2; wBp2 += f * 0.93f * hp2;
    float wAmp = windAmt * (0.16f + wGust * 0.22f) * (env == 0 ? 1.0f : env == 3 ? 0.4f : 0.18f) * gVolAmbient;
    L += wBp1 * wAmp; R += wBp2 * wAmp;
    revL += wBp1 * wAmp * 0.3f; revR += wBp2 * wAmp * 0.3f;

    // ---- low room rumble (always there, worse below)
    rumbleLp += (arand() * 0.5f - rumbleLp) * 0.002f;
    float rumAmp = (env == 2 ? 0.14f : 0.05f) * gVolAmbient;
    L += rumbleLp * rumAmp; R += rumbleLp * rumAmp * 0.96f;

    // ---- stress: ear ring + blood pressure
    if (stress > 62) {
      float ring = sinf(i * 0.7f + wLfo1 * 90000.0f) * ((stress - 62) / 38.0f) * 0.012f;
      L += ring; R += ring * 0.92f;
    }

    // ---- voices
    for (int v = 0; v < NVOICES; v++) {
      Voice& V = gVoices[v];
      if (V.type == V_OFF) continue;
      float s = 0;
      if (V.type == V_GROWL) {
        // continuous entity presence: sub growl, amplitude turbulence
        V.sGain += (V.tGain - V.sGain) * 0.0004f;
        V.sPan += (V.tPan - V.sPan) * 0.0004f;
        V.sCut += (V.tCut - V.sCut) * 0.0004f;
        if (V.sGain < 0.0004f) { continue; }
        V.wDrift += (arand() * 0.4f - V.wDrift) * 0.0008f;
        float fq = 52.0f + V.wDrift * 14.0f;
        V.wPhase += fq / SR;
        float raw = sinf(V.wPhase * 6.28f) + 0.5f * sinf(V.wPhase * 12.6f + 0.7f);
        float turb = 0.55f + 0.45f * sinf(V.wPhase * 1.9f + V.wDrift * 8.0f);
        s = raw * turb * V.sGain * 0.5f;
        float a = Clamp(2.0f * sinf(PI * V.sCut / SR), 0.0f, 1.0f);
        V.lpDist += a * (s - V.lpDist);
        s = V.lpDist * BusGain(V.bus);
        float pg = V.sPan;
        L += s * (1 - fmaxf(0, pg)) ; R += s * (1 + fminf(0, pg));
        revL += s * 0.35f; revR += s * 0.35f;
        continue;
      }
      V.t += 1.0f / SR;
      if (V.t >= V.dur) { V.type = V_OFF; continue; }
      float envA = (V.t < V.att) ? V.t / V.att : powf(1.0f - (V.t - V.att) / (V.dur - V.att), 1.6f);
      float k = V.t / V.dur;
      switch (V.type) {
        case V_NOISE: {
          float n = arand();
          float fc = V.f0 + (V.f1 - V.f0) * k;
          float a = 2.0f * sinf(PI * Clamp(fc, 30.0f, 14000.0f) / SR);
          V.lp += a * V.bp;
          float hp = n - V.lp - V.bp / V.q;
          V.bp += a * hp;
          s = (V.band == 0 ? V.lp : V.band == 1 ? V.bp : hp) * envA * V.gain;
          break;
        }
        case V_TONE: case V_THUMP: {
          float fq = V.freq0 + (V.freq1 - V.freq0) * k;
          V.phase += fq / SR;
          float ph = fmodf(V.phase, 1.0f);
          float raw = V.wave == 0 ? sinf(ph * 6.28f) : V.wave == 1 ? (ph * 2 - 1) : (fabsf(ph * 4 - 2) - 1);
          s = raw * envA * V.gain;
          if (V.type == V_THUMP) { // soften: bodies, not synths
            V.lp += 0.04f * (s - V.lp); s = V.lp * 1.6f;
          }
          break;
        }
        case V_GROAN: {
          float fq = V.freq0 + (V.freq1 - V.freq0) * k;
          float trem = 1.0f + 0.35f * sinf(V.t * 31.0f) + 0.2f * arand();
          V.phase += fq * trem / SR;
          float ph = fmodf(V.phase, 1.0f);
          float raw = (ph * 2 - 1) * 0.7f + arand() * 0.45f; // voice + breath
          float fc = V.f0 + (V.f1 - V.f0) * k;
          float a = 2.0f * sinf(PI * fc / SR);
          V.lp += a * V.bp;
          float hp = raw - V.lp - V.bp / 2.4f;
          V.bp += a * hp;
          s = V.bp * envA * V.gain;
          break;
        }
        case V_BREATH: {
          float n = arand();
          float shape = sinf(k * PI); // inhale->exhale arc
          float a = 2.0f * sinf(PI * (500.0f + shape * 300.0f) / SR);
          V.lp += a * V.bp;
          float hp = n - V.lp - V.bp / 1.1f;
          V.bp += a * hp;
          s = V.bp * shape * envA * V.gain;
          break;
        }
        case V_WHISPER: {
          float n = arand();
          V.wDrift += (arand() * 0.5f - V.wDrift) * 0.0012f;
          float fc = V.f0 + V.wDrift * 700.0f + sinf(k * 23.0f) * 220.0f;
          float a = 2.0f * sinf(PI * Clamp(fc, 300.0f, 4000.0f) / SR);
          V.lp += a * V.bp;
          float hp = n - V.lp - V.bp / 5.0f;
          V.bp += a * hp;
          s = V.bp * envA * V.gain * (0.7f + 0.3f * sinf(k * 67.0f)); // syllables
          break;
        }
        case V_PIANO: {
          // struck-string: 6 inharmonic partials with independent decay +
          // hammer-thump transient. grave when fundamental is low.
          float body = 0;
          for (int p = 0; p < 6; p++) {
            V.pp[p] += V.pf[p] / SR;
            float decay = expf(-V.t * (1.2f + p * 0.9f) / V.dur * 3.0f);
            body += sinf(V.pp[p] * 6.28318f) * V.pa[p] * decay;
          }
          float hammer = (V.t < 0.012f) ? arand() * (1.0f - V.t / 0.012f) * 0.5f : 0.0f;
          float amp = (V.t < V.att) ? V.t / V.att : 1.0f;
          s = (body * 0.5f + hammer) * amp * V.gain * gMusicGain;
          break;
        }
      }
      s *= BusGain(V.bus);
      float pg = V.pan;
      L += s * (1 - fmaxf(0, pg));
      R += s * (1 + fminf(0, pg));
      revL += s * V.rev; revR += s * V.rev;
    }

    // ---- Schroeder reverb
    float rsumL = 0, rsumR = 0;
    for (int c = 0; c < NCOMB; c++) {
      int len = combLen[c];
      int idx = combIdx[c] % len;
      float yl = combBufL[c][idx], yr = combBufR[c][(idx * 7 + 13) % len];
      combBufL[c][idx] = revL + yl * gRevFb;
      combBufR[c][(idx * 7 + 13) % len] = revR + yr * gRevFb;
      rsumL += yl; rsumR += yr;
      combIdx[c]++;
    }
    float apl = apBufL[apIdxL]; apBufL[apIdxL] = rsumL * 0.25f + apl * 0.5f;
    float outRl = apl - 0.5f * apBufL[apIdxL]; apIdxL = (apIdxL + 1) % 480;
    float apr = apBufR[apIdxR]; apBufR[apIdxR] = rsumR * 0.25f + apr * 0.5f;
    float outRr = apr - 0.5f * apBufR[apIdxR]; apIdxR = (apIdxR + 1) % 440;
    L += outRl * wet; R += outRr * wet;

    // ---- master soft clip
    out[i * 2] = tanhf(L * 1.25f) * 0.92f * gVolMaster;
    out[i * 2 + 1] = tanhf(R * 1.25f) * 0.92f * gVolMaster;
  }
}

// ---------------------------------------------------------------- API
static AudioStream gStream;

void AudioInit() {
  InitAudioDevice();
  SetAudioStreamBufferSizeDefault(1024);
  gStream = LoadAudioStream(48000, 32, 2);
  SetAudioStreamCallback(gStream, AudioCB);
  PlayAudioStream(gStream);
}
void AudioClose() { StopAudioStream(gStream); CloseAudioDevice(); }
void AudioSetListener(Vector3 pos, float yaw) { gLisX = pos.x; gLisZ = pos.z; gLisYaw = yaw; }
void AudioSetStress(float s) { gStressA = s; }
void AudioSetWind(float w) { gWindLvl = w; }
void AudioSetEnv(int env) { gEnv = env; gRevFb = env == 2 ? 0.86f : env == 1 ? 0.80f : 0.74f; }

static void Fire(int type, float dur, float att, float gain, float pan, float rev,
                 float f0, float f1, float q, int band, float fr0, float fr1, int wave, int bus = BUS_SFX) {
  int i = FindVoice();
  if (i < 0) return;
  Voice& V = gVoices[i];
  V.t = 0; V.dur = dur; V.att = att; V.gain = gain; V.pan = pan; V.rev = rev;
  V.f0 = f0; V.f1 = f1; V.q = q; V.band = band;
  V.freq0 = fr0; V.freq1 = fr1; V.wave = wave; V.phase = 0;
  V.lp = V.bp = 0; V.wDrift = 0; V.bus = bus;
  V.type = type; // last: arms the voice
}

void SfxFootstep(bool indoor, bool sprint) {
  float g = sprint ? 0.34f : 0.22f;
  if (indoor) {
    Fire(V_NOISE, 0.09f, 0.002f, g * 0.9f, frand2() * 0.06f, 0.30f, 130, 70, 0.9f, 0, 0, 0, 0);   // heel knock
    Fire(V_NOISE, 0.16f, 0.015f, g * 0.30f, frand2() * 0.08f, 0.34f, 900, 350, 1.6f, 1, 0, 0, 0); // wood scuff
    if (frand() < 0.4f) Fire(V_NOISE, 0.5f, 0.05f, g * 0.10f, frand2() * 0.2f, 0.5f, 240, 180, 6.0f, 1, 0, 0, 0); // floor creak
  } else {
    Fire(V_NOISE, 0.07f, 0.002f, g, frand2() * 0.06f, 0.10f, 110, 60, 0.8f, 0, 0, 0, 0);          // earth thud
    Fire(V_NOISE, 0.13f, 0.008f, g * 0.5f, frand2() * 0.08f, 0.12f, 1300, 500, 1.2f, 1, 0, 0, 0); // dirt scuff
    for (int k = 0; k < 5; k++)                                                                    // gravel grains
      Fire(V_NOISE, 0.018f + frand() * 0.02f, 0.001f, g * (0.10f + frand() * 0.12f), frand2() * 0.15f, 0.08f,
           2400 + frand() * 3200, 1800, 2.2f, 2, 0, 0, 0);
  }
}
void SfxBranch(float pan) {
  Fire(V_NOISE, 0.05f, 0.001f, 0.5f, pan, 0.25f, 2600, 900, 1.8f, 2, 0, 0, 0);
  Fire(V_NOISE, 0.12f, 0.004f, 0.3f, pan, 0.3f, 700, 250, 1.4f, 1, 0, 0, 0);
}
void SfxWhisper() {
  Fire(V_WHISPER, 1.2f + frand() * 1.4f, 0.4f, 0.05f + gStressA * 0.0006f, frand2() * 0.85f, 0.7f, 900 + frand() * 700, 0, 5, 1, 0, 0, 0, BUS_VOICE);
}
void SfxDrip() {
  Fire(V_TONE, 0.20f, 0.002f, 0.07f, frand2() * 0.3f, 0.85f, 0, 0, 1, 1, 1700 + frand() * 500, 420, 0, BUS_AMBIENT);
}
void SfxBeep(bool low) { // UI: muffled phone vibration, not a bleep
  Fire(V_THUMP, 0.10f, 0.004f, 0.18f, 0.25f, 0.05f, 0, 0, 1, 0, low ? 95.0f : 150.0f, low ? 70.0f : 130.0f, 0);
}
void SfxPickup() {
  Fire(V_NOISE, 0.07f, 0.003f, 0.22f, 0.18f, 0.2f, 3000, 1400, 2.0f, 1, 0, 0, 0); // metallic brush
  Fire(V_TONE, 0.16f, 0.004f, 0.05f, 0.18f, 0.4f, 0, 0, 1, 0, 2900, 2400, 0);     // faint ring
}
void SfxUnlock() {
  Fire(V_NOISE, 0.06f, 0.002f, 0.4f, 0, 0.35f, 2100, 800, 2.4f, 1, 0, 0, 0);
  Fire(V_THUMP, 0.35f, 0.01f, 0.5f, 0, 0.45f, 0, 0, 1, 0, 120, 60, 0);
}
void SfxSting(float k) {
  Fire(V_GROAN, 2.4f, 0.5f, 0.16f * k, -0.2f, 0.7f, 300, 150, 1, 1, 62, 47, 1, BUS_VOICE);
  Fire(V_GROAN, 2.6f, 0.6f, 0.13f * k, 0.25f, 0.7f, 420, 200, 1, 1, 93, 71, 1, BUS_VOICE);
  Fire(V_THUMP, 1.6f, 0.01f, 0.5f * k, 0, 0.5f, 0, 0, 1, 0, 52, 30, 0, BUS_VOICE);
}
void SfxScare() {
  Fire(V_NOISE, 0.7f, 0.004f, 0.85f, 0, 0.4f, 400, 6000, 0.8f, 2, 0, 0, 0, BUS_VOICE); // air slam
  SfxSting(1.6f);
}
// distinct cry/scream voices per entity kind
void SfxEntityCry(Vector3 at, int kind) {
  float g, p, c; Spatial(at, g, p, c);
  if (kind == 1) { // Samara: drowned child wail, pitch falling
    Fire(V_GROAN, 2.0f + frand() * 0.9f, 0.3f, 0.6f * g, p, 0.6f, 520, 200, 1, 1, 240 + frand() * 60, 70, 1, BUS_VOICE);
    for (int i = 0; i < 5; i++) // wet bone cracks
      Fire(V_NOISE, 0.03f, 0.001f, 0.32f * g, p + frand2() * 0.1f, 0.5f, 1900 + frand() * 900, 900, 3.0f, 1, 0, 0, 0, BUS_VOICE);
  } else if (kind == 2) { // Watcher: high keening whine
    Fire(V_GROAN, 1.6f, 0.5f, 0.4f * g, p, 0.75f, 1400, 800, 2, 1, 600 + frand() * 200, 300, 0, BUS_VOICE);
  } else if (kind == 3) { // Crawler: rising distorted shriek + chittering
    Fire(V_GROAN, 1.1f + frand() * 0.5f, 0.06f, 0.55f * g, p, 0.6f, 800, 2600, 1.8f, 1, 300, 900 + frand() * 300, 1, BUS_VOICE);
    for (int i = 0; i < 6; i++)
      Fire(V_NOISE, 0.02f, 0.001f, 0.25f * g, p + frand2() * 0.2f, 0.4f, 2600 + frand() * 1500, 1400, 4.0f, 1, 0, 0, 0, BUS_VOICE);
  } else { // Shadow: low broken sob/scream
    Fire(V_GROAN, 1.4f + frand() * 0.7f, 0.25f, 0.5f * g, p, 0.65f, 700, 300, 1.4f, 1, 150 + frand() * 80, 55, 1, BUS_VOICE);
  }
}
void SfxSamaraGroan(Vector3 at) { SfxEntityCry(at, 1); }
void SfxHeartThump(float k) {
  Fire(V_THUMP, 0.13f, 0.005f, 0.30f * k, 0, 0.02f, 0, 0, 1, 0, 54, 38, 0, BUS_VOICE);
  Fire(V_THUMP, 0.11f, 0.005f, 0.20f * k, 0, 0.02f, 0, 0, 1, 0, 47, 33, 0, BUS_VOICE);
}
void SfxBreathPlayer(float intensity) {
  Fire(V_BREATH, 1.1f, 0.3f, 0.012f + intensity * 0.05f, 0, 0.02f, 0, 0, 1, 1, 0, 0, 0, BUS_VOICE);
}
void SfxThunder() {
  Fire(V_NOISE, 2.6f, 0.02f, 0.6f, frand2() * 0.4f, 0.6f, 90, 40, 0.7f, 0, 0, 0, 0, BUS_AMBIENT);
  Fire(V_NOISE, 1.2f, 0.005f, 0.4f, frand2() * 0.5f, 0.5f, 1800, 200, 0.9f, 0, 0, 0, 0, BUS_AMBIENT);
}

// ---------------------------------------------------------------- music
void PianoNote(float freq, float dur, float gain, float pan) {
  int i = FindVoice();
  if (i < 0) return;
  Voice& V = gVoices[i];
  V.t = 0; V.dur = dur; V.att = 0.004f; V.gain = gain; V.pan = pan; V.rev = 0.45f;
  V.bus = BUS_MUSIC;
  for (int p = 0; p < 6; p++) {
    float inh = 1.0f + p * p * 0.0008f;       // string inharmonicity
    V.pf[p] = freq * (p + 1) * inh;
    V.pa[p] = (p == 0 ? 1.0f : 0.62f / (p + 1)) * (0.85f + frand() * 0.3f);
    V.pp[p] = 0;
  }
  V.type = V_PIANO;
}
void AudioSetMusicTarget(float g) { gMusicTarget = g; }
void AudioSetRain(float r) { gRainTarget = r; }
void AudioSetVolumes(float master, float music, float ambient, float voices) {
  gVolMaster = master; gVolMusic = music; gVolAmbient = ambient; gVolVoices = voices;
}

int AudioEntityVoice() {
  for (int i = 0; i < 8; i++) if (gVoices[i].type == V_OFF) { gVoices[i].type = V_GROWL; gVoices[i].tGain = 0; return i; }
  return -1;
}
void AudioEntityUpdate(int voice, Vector3 pos, EState st, EKind kind) {
  if (voice < 0) return;
  float g, p, c; Spatial(pos, g, p, c);
  bool aggressive = st == EState::Pursuing || st == EState::Hunting || st == EState::Emerging;
  float base = (kind == EKind::Samara) ? 0.65f : 0.45f;
  gVoices[voice].tGain = g * base * (aggressive ? 1.5f : 0.7f);
  gVoices[voice].tPan = p;
  gVoices[voice].tCut = c;
}
void AudioEntityStop(int voice) { if (voice >= 0) gVoices[voice].tGain = 0; }
