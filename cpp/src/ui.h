// PARANOIA v3 — menus, settings, pause, intro cinematic. Drawn into the
// scene RT (so the dither/grain applies), input scaled to internal res.
#pragma once
#include "game.h"

int gState = GS_MENU;
float gStateT = 0;
bool gReqQuit = false, gReqRestart = false;

static int gMenuSel = 0;          // highlighted menu item
static int gRebindAction = -1;    // action awaiting a key, -1 none
static int gDragSlider = -1;      // slider being dragged, -1 none
static bool gFromPause = false;   // settings opened from pause vs main menu

static Vector2 UIMouse() {
  return { GetMouseX() * (float)INTERNAL_W / GetScreenWidth(),
           GetMouseY() * (float)INTERNAL_H / GetScreenHeight() };
}
static bool Hit(Rectangle r, Vector2 m) { return CheckCollisionPointRec(m, r); }

static void TextC(const char* t, float y, float size, float sp, Color c) {
  Vector2 m = MeasureTextEx(gGfx.font, t, size, sp);
  DrawTextEx(gGfx.font, t, { (INTERNAL_W - m.x) / 2, y }, size, sp, c);
}

// a centered clickable button; returns true on click
static bool Button(const char* label, float y, Vector2 mouse, bool click, float w = 220) {
  Rectangle r = { (INTERNAL_W - w) / 2, y, w, 26 };
  bool hov = Hit(r, mouse);
  DrawRectangleLinesEx(r, 1, Fade(hov ? Color{ 210, 210, 218, 255 } : Color{ 90, 90, 100, 255 }, 0.9f));
  if (hov) DrawRectangleRec(r, Fade(WHITE, 0.04f));
  Vector2 m = MeasureTextEx(gGfx.font, label, 14, 3);
  DrawTextEx(gGfx.font, label, { (INTERNAL_W - m.x) / 2, y + 6 }, 14, 3, hov ? Color{ 235, 235, 240, 255 } : Color{ 170, 170, 178, 255 });
  return hov && click;
}

// a labelled slider 0..1; updates *v on drag; returns the row rect
static void Slider(const char* label, float* v, float y, int idx, Vector2 mouse, bool down) {
  float bx = INTERNAL_W / 2 - 60, bw = 160;
  DrawTextEx(gGfx.font, label, { bx - 130.0f, y - 3 }, 11, 2, Color{ 175, 175, 182, 255 });
  Rectangle bar = { bx, y, bw, 6 };
  DrawRectangleRec(bar, Color{ 40, 40, 46, 255 });
  DrawRectangleRec({ bx, y, bw * (*v), 6 }, Color{ 150, 160, 180, 255 });
  float hx = bx + bw * (*v);
  DrawRectangle((int)hx - 2, (int)y - 3, 4, 12, Color{ 220, 220, 228, 255 });
  Rectangle hot = { bx - 4, y - 8, bw + 8, 20 };
  if (down && Hit(hot, mouse)) gDragSlider = idx;
  if (gDragSlider == idx) *v = Clamp((mouse.x - bx) / bw, 0.0f, 1.0f);
  char pct[8]; snprintf(pct, 8, "%d", (int)(*v * 100));
  DrawTextEx(gGfx.font, pct, { bx + bw + 8.0f, y - 3 }, 11, 2, Color{ 130, 130, 138, 255 });
}

void IntroStart() { gState = GS_INTRO; gStateT = 0; MusicSetMode(MUS_MELANCHOLY); AudioSetRain(0.7f); }

// ---------------------------------------------------------------- intro
static bool gIntroCrashed = false;
static void DrawIntro(float t) {
  DrawRectangle(0, 0, INTERNAL_W, INTERNAL_H, BLACK);
  // the crash: white flash + impact at t=7.0 (once)
  if (t >= 6.9f && !gIntroCrashed) { gIntroCrashed = true; gDir.flashWhite = 1.5f; SfxScare(); SfxThunder(); }
  if (t < 6.9f) gIntroCrashed = false;
  gDir.flashWhite = fmaxf(0, gDir.flashWhite - GetFrameTime() * 0.6f);
  float cx = INTERNAL_W / 2.0f, cy = INTERNAL_H / 2.0f;
  // approaching headlights that swerve, then a white crash, then aftermath
  if (t < 7.0f) {
    float k = t / 7.0f;
    float sep = 6 + k * k * 120;
    float yy = cy - 20 + k * 40;
    float br = 0.2f + k * 0.8f;
    float swerve = (t > 5.0f) ? (t - 5.0f) * (t - 5.0f) * 60 : 0;
    DrawCircle((int)(cx - sep / 2 - swerve), (int)yy, 6 + k * 14, Fade(Color{ 255, 250, 235, 255 }, br));
    DrawCircle((int)(cx + sep / 2 - swerve), (int)yy, 6 + k * 14, Fade(Color{ 255, 250, 235, 255 }, br));
    DrawRectangle(0, 0, INTERNAL_W, INTERNAL_H, Fade(BLACK, 0.25f));
  }
  const char* line = nullptr;
  float fade = 0;
  auto panel = [&](float a, float b, const char* s) {
    if (t >= a && t < b) { line = s; float u = t - a, len = b - a; fade = fminf(fminf(u, b - t), 1.0f); }
  };
  panel(0.5f, 4.0f, "You don't remember the road.");
  panel(4.0f, 6.9f, "Headlights. A curve you took too fast.");
  // crash at 7.0
  panel(7.4f, 11.0f, "When you woke, the car was silent.");
  panel(11.0f, 14.5f, "The forest was not.");
  panel(14.5f, 18.0f, "You don't know why you were out here.");
  panel(18.0f, 21.5f, "Your phone still has signal. It still does.");
  panel(22.0f, 25.5f, "It says: TRUST ME.");
  if (line) TextC(line, cy + 40, 14, 2, Fade(Color{ 198, 196, 200, 255 }, Clamp(fade, 0.0f, 1.0f)));
  TextC("[ENTER] skip", INTERNAL_H - 20.0f, 9, 2, Color{ 60, 60, 66, 255 });
}

// ---------------------------------------------------------------- settings
static void DrawSettings(float t, Vector2 mouse, bool click, bool down) {
  DrawRectangle(0, 0, INTERNAL_W, INTERNAL_H, Fade(BLACK, 0.82f));
  TextC("SETTINGS", 26, 22, 6, Color{ 205, 205, 212, 255 });

  TextC("- AUDIO -", 60, 11, 3, Color{ 110, 110, 120, 255 });
  Slider("MASTER", &gCfg.volMaster, 80, 0, mouse, down);
  Slider("MUSIC", &gCfg.volMusic, 95, 1, mouse, down);
  Slider("AMBIENT", &gCfg.volAmbient, 110, 2, mouse, down);
  Slider("VOICES", &gCfg.volVoices, 125, 3, mouse, down);
  Slider("MOUSE SENS", &gCfg.mouseSens, 145, 4, mouse, down); // 0..1 maps to 0..2x below
  AudioSetVolumes(gCfg.volMaster, gCfg.volMusic, gCfg.volAmbient, gCfg.volVoices);

  TextC("- CONTROLS -  (click, then press a key)", 172, 10, 2, Color{ 110, 110, 120, 255 });
  float y = 188;
  for (int a = 0; a < ACT_COUNT; a++) {
    float rx = INTERNAL_W / 2 - 130;
    DrawTextEx(gGfx.font, ActionName(a), { rx, y }, 10, 2, Color{ 165, 165, 172, 255 });
    Rectangle kb = { rx + 150, y - 2, 80, 14 };
    bool hov = Hit(kb, mouse);
    bool waiting = (gRebindAction == a);
    DrawRectangleLinesEx(kb, 1, Fade(waiting ? Color{ 230, 200, 90, 255 } : hov ? Color{ 200, 200, 210, 255 } : Color{ 80, 80, 90, 255 }, 0.9f));
    const char* kl = waiting ? "..." : KeyLabel(gCfg.keys[a]);
    DrawTextEx(gGfx.font, kl, { rx + 158, y }, 10, 2, waiting ? Color{ 230, 200, 90, 255 } : Color{ 200, 200, 208, 255 });
    if (hov && click) gRebindAction = a;
    y += 17;
  }

  // capture rebind
  if (gRebindAction >= 0) {
    int k = GetKeyPressed();
    if (k != 0 && k != KEY_ESCAPE) { gCfg.keys[gRebindAction] = k; gRebindAction = -1; }
  }

  if (Button("BACK", INTERNAL_H - 34.0f, mouse, click && gRebindAction < 0, 160)) {
    CfgSave();
    gState = gFromPause ? GS_PAUSE : GS_MENU;
    gStateT = 0;
  }
}

// ---------------------------------------------------------------- pause
static void DrawPause(float t, Vector2 mouse, bool click) {
  DrawRectangle(0, 0, INTERNAL_W, INTERNAL_H, Fade(BLACK, 0.62f));
  TextC("PAUSED", 70, 26, 8, Color{ 200, 200, 208, 255 });
  if (Button("RESUME", 130, mouse, click)) { gState = GS_PLAY; DisableCursor(); }
  if (Button("SETTINGS", 162, mouse, click)) { gFromPause = true; gState = GS_SETTINGS; gStateT = 0; }
  if (Button("EXIT TO MENU", 194, mouse, click)) { gState = GS_MENU; gStateT = 0; gReqRestart = true; }
}

// ---------------------------------------------------------------- main menu
static void DrawMenu(float t, Vector2 mouse, bool click) {
  DrawRectangle(0, 0, INTERNAL_W, INTERNAL_H, Fade(BLACK, 0.5f));
  float fl = (sinf(t * 1.1f) > 0.96f) ? 0.4f : 1.0f;
  TextC("P A R A N O I A", 60, 42, 10, Fade(Color{ 188, 188, 192, 255 }, fl));
  TextC("YOU ARE BEING OBSERVED", 110, 11, 5, Color{ 95, 80, 80, 255 });
  if (Button("BEGIN", 150, mouse, click)) IntroStart();
  if (Button("SETTINGS", 184, mouse, click)) { gFromPause = false; gState = GS_SETTINGS; gStateT = 0; }
  if (Button("QUIT", 218, mouse, click)) gReqQuit = true;
  TextC("HEADPHONES STRONGLY RECOMMENDED", 270, 9, 2, Color{ 70, 60, 60, 255 });
  TextC("vamp9 - MMXXVI", INTERNAL_H - 18.0f, 9, 2, Color{ 52, 52, 52, 255 });
}

// ---------------------------------------------------------------- credits
static void DrawCredits(float t) {
  DrawRectangle(0, 0, INTERNAL_W, INTERNAL_H, BLACK);
  TextC("P A R A N O I A", 64, 30, 8, Color{ 200, 196, 191, 255 });
  TextC(gDir.ending == 1 ? "ENDING - TRUST ME" : "ENDING - REJECT TRUTH", 112, 12, 3, Color{ 106, 64, 64, 255 });
  const char* blurb = gDir.ending == 1
    ? "You stepped through the door it offered.\nThe recording continues. It always was you."
    : "The screen died. The dark is full of breathing.\nThey no longer need the camera to watch you.";
  Vector2 m = MeasureTextEx(gGfx.font, blurb, 11, 2);
  DrawTextEx(gGfx.font, blurb, { (INTERNAL_W - m.x) / 2, 158 }, 11, 2, Color{ 119, 119, 119, 255 });
  TextC("a game by vamp9", 228, 13, 3, Color{ 170, 170, 170, 255 });
  TextC("design - code - sound: procedural", 250, 9, 2, Color{ 68, 68, 68, 255 });
  TextC("[ENTER] menu      [ESC] quit", 300, 11, 2, Color{ 85, 85, 85, 255 });
}

// ---------------------------------------------------------------- dispatch
void UIDrawOverlay(float time) {
  Vector2 mouse = UIMouse();
  bool click = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
  bool down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
  if (!down) gDragSlider = -1;
  switch (gState) {
    case GS_MENU: DrawMenu(gStateT, mouse, click); break;
    case GS_INTRO: DrawIntro(gStateT); break;
    case GS_SETTINGS: DrawSettings(gStateT, mouse, click, down); break;
    case GS_PAUSE: DrawPause(gStateT, mouse, click); break;
    case GS_CREDITS: DrawCredits(gStateT); break;
    default: break;
  }
}
