// PARANOIA v3 — settings: audio buses, rebindable controls, mouse sensitivity.
// Persisted next to the exe as paranoia.cfg.
#pragma once
#include "game.h"

enum Action {
  ACT_FWD = 0, ACT_BACK, ACT_LEFT, ACT_RIGHT, ACT_SPRINT,
  ACT_LIGHT, ACT_INTERACT, ACT_COUNT
};

struct Settings {
  int keys[ACT_COUNT];
  float volMaster = 0.9f, volMusic = 0.7f, volAmbient = 0.8f, volVoices = 1.0f;
  float mouseSens = 1.0f;
  int fullscreen = 1;
};
extern Settings gCfg;

static const char* ActionName(int a) {
  switch (a) {
    case ACT_FWD: return "MOVE FORWARD";
    case ACT_BACK: return "MOVE BACK";
    case ACT_LEFT: return "MOVE LEFT";
    case ACT_RIGHT: return "MOVE RIGHT";
    case ACT_SPRINT: return "SPRINT";
    case ACT_LIGHT: return "FLASHLIGHT";
    case ACT_INTERACT: return "INTERACT";
  }
  return "?";
}

// human-readable key label
static const char* KeyLabel(int k) {
  static char b[16];
  if (k >= KEY_A && k <= KEY_Z) { b[0] = (char)('A' + (k - KEY_A)); b[1] = 0; return b; }
  if (k >= KEY_ZERO && k <= KEY_NINE) { b[0] = (char)('0' + (k - KEY_ZERO)); b[1] = 0; return b; }
  switch (k) {
    case KEY_SPACE: return "SPACE";
    case KEY_LEFT_SHIFT: return "L-SHIFT";
    case KEY_RIGHT_SHIFT: return "R-SHIFT";
    case KEY_LEFT_CONTROL: return "L-CTRL";
    case KEY_E: return "E"; case KEY_F: return "F";
    case KEY_UP: return "UP"; case KEY_DOWN: return "DOWN";
    case KEY_LEFT: return "LEFT"; case KEY_RIGHT: return "RIGHT";
    case KEY_TAB: return "TAB";
  }
  snprintf(b, 16, "K%d", k);
  return b;
}

Settings gCfg;

static void CfgDefaults() {
  gCfg.keys[ACT_FWD] = KEY_W;
  gCfg.keys[ACT_BACK] = KEY_S;
  gCfg.keys[ACT_LEFT] = KEY_A;
  gCfg.keys[ACT_RIGHT] = KEY_D;
  gCfg.keys[ACT_SPRINT] = KEY_LEFT_SHIFT;
  gCfg.keys[ACT_LIGHT] = KEY_F;
  gCfg.keys[ACT_INTERACT] = KEY_E;
  gCfg.volMaster = 0.9f; gCfg.volMusic = 0.7f; gCfg.volAmbient = 0.8f; gCfg.volVoices = 1.0f;
  gCfg.mouseSens = 0.5f; gCfg.fullscreen = 0; // 0 = borderless windowed, 1 = exclusive fullscreen
}

// apply the chosen display mode (0 borderless windowed, 1 exclusive fullscreen)
static void ApplyDisplayMode(int mode) {
  if (IsWindowFullscreen()) ToggleFullscreen();                          // normalise to plain windowed
  if (IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE)) ToggleBorderlessWindowed();
  if (mode == 1) ToggleFullscreen();
  else ToggleBorderlessWindowed();
}

static const char* CfgPath() {
  static char path[1024];
  // GetApplicationDirectory ends with a path separator
  snprintf(path, 1024, "%sparanoia.cfg", GetApplicationDirectory());
  return path;
}

void CfgSave() {
  FILE* f = fopen(CfgPath(), "w");
  if (!f) return;
  for (int i = 0; i < ACT_COUNT; i++) fprintf(f, "key%d %d\n", i, gCfg.keys[i]);
  fprintf(f, "master %.3f\nmusic %.3f\nambient %.3f\nvoices %.3f\nsens %.3f\nfull %d\n",
    gCfg.volMaster, gCfg.volMusic, gCfg.volAmbient, gCfg.volVoices, gCfg.mouseSens, gCfg.fullscreen);
  fclose(f);
}

void CfgLoad() {
  CfgDefaults();
  FILE* f = fopen(CfgPath(), "r");
  if (!f) return;
  char k[32]; float v;
  while (fscanf(f, "%31s %f", k, &v) == 2) {
    if (!strncmp(k, "key", 3)) { int i = atoi(k + 3); if (i >= 0 && i < ACT_COUNT) gCfg.keys[i] = (int)v; }
    else if (!strcmp(k, "master")) gCfg.volMaster = v;
    else if (!strcmp(k, "music")) gCfg.volMusic = v;
    else if (!strcmp(k, "ambient")) gCfg.volAmbient = v;
    else if (!strcmp(k, "voices")) gCfg.volVoices = v;
    else if (!strcmp(k, "sens")) gCfg.mouseSens = v;
    else if (!strcmp(k, "full")) gCfg.fullscreen = (int)v;
  }
  fclose(f);
}

static inline bool ActDown(int a) { return IsKeyDown(gCfg.keys[a]); }
static inline bool ActPressed(int a) { return IsKeyPressed(gCfg.keys[a]); }
