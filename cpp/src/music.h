// PARANOIA v3 — music director. Drives the piano synth in audio.h.
// Melancholic for the intro/lore, grave/intense for tension, low ostinato
// for active chases. All notes synthesized; no files.
#pragma once
#include "game.h"

enum MusicMode { MUS_OFF, MUS_MENU, MUS_MELANCHOLY, MUS_TENSION, MUS_CHASE };

static int gMusMode = MUS_OFF;
static float gMusT = 0;     // beat clock
static int gMusStep = 0;
static float gMusStepLen = 0.7f;

static inline float Note(int semisFromA4) { return 440.0f * powf(2.0f, semisFromA4 / 12.0f); }
// semitone offsets relative to A4 (=0). negatives = lower octaves.

void MusicSetMode(int m) {
  if (gMusMode == m) return;
  gMusMode = m;
  gMusStep = 0; gMusT = 0;
  switch (m) {
    case MUS_OFF: AudioSetMusicTarget(0.0f); break;
    case MUS_MENU: AudioSetMusicTarget(0.55f); gMusStepLen = 2.2f; break;
    case MUS_MELANCHOLY: AudioSetMusicTarget(0.85f); gMusStepLen = 0.95f; break;
    case MUS_TENSION: AudioSetMusicTarget(0.75f); gMusStepLen = 0.85f; break;
    case MUS_CHASE: AudioSetMusicTarget(1.0f); gMusStepLen = 0.34f; break;
  }
}

void MusicTick(float dt) {
  if (gMusMode == MUS_OFF) return;
  gMusT += dt;
  if (gMusT < gMusStepLen) return;
  gMusT -= gMusStepLen;
  int s = gMusStep++;

  if (gMusMode == MUS_MENU) {
    // single grave tolling note, long
    if (s % 2 == 0) PianoNote(Note(-24 - (s % 4 == 0 ? 0 : 5)), 4.0f, 0.32f, frand2() * 0.2f);
  } else if (gMusMode == MUS_MELANCHOLY) {
    // A-minor lament: rolling left-hand + sparse right-hand melody
    static const int bass[4] = { -24, -19, -17, -21 };   // A2 E3 F#... low roots
    static const int mel[8] = { 0, 3, 7, 3, 2, 0, -2, -5 }; // a c e c b a g e (minor-ish)
    int bar = (s / 4) % 4;
    PianoNote(Note(bass[bar]), 2.6f, 0.30f, -0.25f);                 // root
    PianoNote(Note(bass[bar] + 7), 2.2f, 0.16f, -0.1f);             // fifth
    if (s % 2 == 0) {
      int m = mel[(s / 2) % 8];
      PianoNote(Note(m - 12), 1.6f, 0.26f, 0.28f);                   // melody, soft
    }
  } else if (gMusMode == MUS_TENSION) {
    // grave dissonant clusters, swelling — root + tritone
    int root = -29 + (s % 3);
    PianoNote(Note(root), 1.4f, 0.34f, -0.2f);
    PianoNote(Note(root + 6), 1.4f, 0.24f, 0.2f);     // tritone — wrongness
    if (s % 4 == 3) PianoNote(Note(root - 12), 2.2f, 0.4f, 0);  // deep punctuation
  } else if (gMusMode == MUS_CHASE) {
    // driving low ostinato, hammered
    int root = -29;
    PianoNote(Note(root + (s % 2 ? 1 : 0)), 0.4f, 0.40f, 0);
    if (s % 2 == 0) PianoNote(Note(root + 6), 0.35f, 0.22f, frand2() * 0.3f);
    if (s % 8 == 0) PianoNote(Note(root - 12), 1.6f, 0.5f, 0);
  }
}
