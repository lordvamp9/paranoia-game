// PARANOIA v2 — the hand, the phone, the inventory.
// Right hand never lets go of the phone. The left hand reaches, takes,
// inspects. Items are detailed and lit; the dither does the flesh.
#pragma once
#include "game.h"

Phone gPhone;
Inventory gInv;

static Mesh mshPalm, mshFinger, mshPhoneBody, mshScreenQ, mshKeyShaft, mshKeyBow, mshKeyTooth, mshNoteQ, mshBattery;
static Material gMatHand;
static bool gHandInit = false;
static Texture2D texSkinH;

static Mesh QuadMeshFlipV(float w, float h) {
  std::vector<float> v = { -w / 2, -h / 2, 0,  w / 2, -h / 2, 0,  w / 2, h / 2, 0,  -w / 2, h / 2, 0 };
  std::vector<float> n = { 0,0,1, 0,0,1, 0,0,1, 0,0,1 };
  std::vector<float> uv = { 0,0, 1,0, 1,1, 0,1 };
  std::vector<unsigned short> idx = { 0,1,2, 0,2,3 };
  return BuildMesh(v, n, uv, idx);
}

static void HandInit() {
  mshPalm = GenMeshCube(1, 1, 1);
  mshFinger = GenMeshCube(1, 1, 1);
  mshPhoneBody = GenMeshCube(0.165f, 0.34f, 0.018f);
  mshScreenQ = QuadMeshFlipV(0.150f, 0.325f);
  mshKeyShaft = GenMeshCylinder(0.011f, 0.155f, 6);
  mshKeyBow = GenMeshTorus(0.011f, 0.035f, 6, 9);
  mshKeyTooth = GenMeshCube(0.030f, 0.034f, 0.011f);
  mshNoteQ = QuadMesh(0.22f, 0.30f);
  mshBattery = GenMeshCube(0.07f, 0.12f, 0.07f);
  gMatHand = LoadMaterialDefault();
  texSkinH = MakeNoiseTexture(128, Color{ 172, 132, 104, 255 }, 16);
  gHandInit = true;
}

void InvGive(int id) {
  if (id == IT_BATTERY) gInv.batteries++;
  gInv.has[id] = true;
  gInv.pickupAnim = 1.0f;
  gInv.pickupItem = id;
  SfxPickup();
}

void InvSelect(int slot) {
  int map[5] = { IT_PHONE, IT_WATERKEY, IT_KITCHENKEY, IT_NOTE, IT_BATTERY };
  if (slot < 0 || slot > 4) return;
  int id = map[slot];
  if (id == IT_PHONE) { gInv.inspecting = -1; return; }
  if (!gInv.has[id]) return;
  if (id == IT_BATTERY) {
    if (gInv.batteries > 0 && gPhone.battery < 99) {
      gInv.batteries--;
      if (gInv.batteries <= 0) gInv.has[IT_BATTERY] = false;
      gPhone.battery = fminf(100.0f, gPhone.battery + 50.0f);
      gPhone.lightOn = true;
      gPhone.messageT = 3; gPhone.message = "BATTERY SWAPPED";
      SfxPickup();
    }
    return;
  }
  gInv.inspecting = (gInv.inspecting == id) ? -1 : id;
}

void InvUpdate(float dt) {
  float target = gInv.inspecting >= 0 ? 1.0f : 0.0f;
  gInv.inspectAnim += (target - gInv.inspectAnim) * fminf(1, dt * 6);
  gInv.pickupAnim = fmaxf(0, gInv.pickupAnim - dt * 1.1f);
}

void PhoneUpdate(float dt, float time, bool indoor, bool entitiesNear, float stress) {
  gPhone.messageT = fmaxf(0, gPhone.messageT - dt);
  // drain
  float rate = 0.025f;
  if (gPhone.lightOn) rate += 0.062f;
  rate *= 1.0f + (stress / 100.0f);
  if (entitiesNear) rate *= 2.0f;
  gPhone.battery = fmaxf(0, gPhone.battery - rate * dt);
  if (gPhone.battery <= 0) gPhone.lightOn = false;
  // flicker
  gPhone.flickerT -= dt;
  if (gPhone.flickerT <= 0) {
    float chance = gPhone.battery < 20 ? 0.5f : gPhone.battery < 50 ? 0.12f : 0.03f;
    if (stress > 60) chance += 0.15f;
    gPhone.flickerOff = frand() < chance;
    gPhone.flickerT = gPhone.flickerOff ? 0.05f + frand() * 0.12f : 0.4f + frand() * 1.6f;
  }
}

// ---------------------------------------------------------------- screen
static const char* CorruptText(const char* in, float lvl, char* buf, int bufLen) {
  int j = 0;
  for (int i = 0; in[i] && j < bufLen - 2; i++) {
    char c = in[i];
    if (lvl > 0.3f && frand() < (lvl - 0.3f) * 0.9f) {
      switch (c) {
        case 'a': case 'A': c = '4'; break;
        case 'e': case 'E': c = '3'; break;
        case 'i': case 'I': c = '1'; break;
        case 'o': case 'O': c = '0'; break;
        case 's': case 'S': c = '5'; break;
        case 't': case 'T': c = '7'; break;
      }
    }
    buf[j++] = c;
    if (lvl > 0.7f && frand() < (lvl - 0.7f) * 0.5f && j < bufLen - 2) buf[j++] = (frand() < 0.5f) ? '#' : '_';
  }
  buf[j] = 0;
  return buf;
}

void PhoneDrawScreen(float time, float stress) {
  float s = stress / 100.0f;
  BeginTextureMode(gGfx.phoneScr);
  if (gPhone.battery <= 0) { ClearBackground(BLACK); EndTextureMode(); return; }
  ClearBackground(Color{ 4, 7, 10, 255 });
  // sensor noise
  for (int i = 0; i < 40; i++)
    DrawRectangle(GetRandomValue(0, PHONE_SCR_W), GetRandomValue(0, PHONE_SCR_H), 2, 2, Fade(WHITE, 0.03f));
  // battery
  int bat = (int)ceilf(gPhone.battery);
  Color bc = bat < 20 ? (sinf(time * 8) > 0 ? Color{ 255, 59, 48, 255 } : Color{ 122, 29, 24, 255 }) : Color{ 154, 223, 154, 255 };
  DrawTextEx(gGfx.font, TextFormat("%d%%", bat), { 8, 8 }, 16, 1, bc);
  DrawRectangleLines(48, 9, 26, 13, bc);
  DrawRectangle(50, 11, (int)(22 * bat / 100.0f), 9, bc);
  DrawRectangle(74, 12, 3, 6, bc);
  // clock — wrong at high stress
  DrawTextEx(gGfx.font, s > 0.7f ? "??:??" : TextFormat("03:%02d", 33 + ((int)(time / 60)) % 27), { PHONE_SCR_W - 44.0f, 8 }, 14, 1, Color{ 102, 102, 119, 255 });
  // objective
  char buf[96];
  if (!gPhone.objective.empty()) {
    CorruptText(gPhone.objective.c_str(), s, buf, 96);
    Vector2 m = MeasureTextEx(gGfx.font, buf, 11, 1);
    DrawTextEx(gGfx.font, buf, { (PHONE_SCR_W - m.x) / 2, 38 }, 11, 1, Fade(Color{ 220, 220, 230, 255 }, 0.85f - s * 0.3f));
  }
  // message
  if (gPhone.messageT > 0 && sinf(time * 6) > -0.4f) {
    CorruptText(gPhone.message.c_str(), s, buf, 96);
    Vector2 m = MeasureTextEx(gGfx.font, buf, 17, 1);
    DrawTextEx(gGfx.font, buf, { (PHONE_SCR_W - m.x) / 2, PHONE_SCR_H / 2.0f - 10 }, 17, 1, Color{ 235, 230, 225, 242 });
  }
  // stamina strip (the phone watches your pulse too)
  DrawRectangle(8, PHONE_SCR_H - 50, (int)(50 * gPlayer.stamina / 100.0f), 3, gPlayer.exhausted ? Color{ 200, 60, 50, 200 } : Color{ 120, 130, 150, 160 });
  if (s > 0.45f && frand() < 0.4f)
    DrawTextEx(gGfx.font, "* WATCHING *", { 56, 70 }, 10, 1, Fade(Color{ 200, 60, 55, 255 }, (s - 0.45f) * 1.2f));
  // REC — always
  DrawCircle(14, PHONE_SCR_H - 22, 5, sinf(time * 3.5f) > 0 ? Color{ 255, 59, 48, 255 } : Fade(Color{ 255, 59, 48, 255 }, 0.25f));
  DrawTextEx(gGfx.font, "REC", { 24, PHONE_SCR_H - 28.0f }, 12, 1, Color{ 187, 187, 187, 255 });
  int rt = (int)time;
  DrawTextEx(gGfx.font, TextFormat("%02d:%02d:%02d", rt / 3600, (rt / 60) % 60, rt % 60), { 58, PHONE_SCR_H - 27.0f }, 10, 1, Color{ 85, 85, 85, 255 });
  if (bat < 20 && sinf(time * 5) > 0) {
    DrawTextEx(gGfx.font, "LOW BATTERY", { 46, PHONE_SCR_H - 64.0f }, 14, 1, Color{ 255, 59, 48, 255 });
  }
  // glitch bands
  if (s > 0.4f) {
    int n = (int)((s - 0.4f) * 12);
    for (int i = 0; i < n; i++) {
      int y = GetRandomValue(0, PHONE_SCR_H);
      DrawRectangle(GetRandomValue(-12, 12), y, PHONE_SCR_W, GetRandomValue(2, 7), Fade(frand() < 0.5f ? WHITE : Color{ 255, 40, 40, 255 }, 0.07f));
    }
  }
  EndTextureMode();
}

// ---------------------------------------------------------------- 3D hand
static Matrix BasisMatrix(Vector3 r, Vector3 u, Vector3 b, Vector3 p) {
  Matrix m = MatrixIdentity();
  m.m0 = r.x; m.m1 = r.y; m.m2 = r.z;
  m.m4 = u.x; m.m5 = u.y; m.m6 = u.z;
  m.m8 = b.x; m.m9 = b.y; m.m10 = b.z;
  m.m12 = p.x; m.m13 = p.y; m.m14 = p.z;
  return m;
}

static void HPart(Mesh mesh, Vector3 size, Vector3 local, Vector3 rot, Matrix rig, Texture2D tex, Color tint, float spec) {
  Matrix m = MatrixMultiply(MatrixMultiply(MatrixScale(size.x, size.y, size.z), MatrixRotateXYZ(rot)), MatrixTranslate(local.x, local.y, local.z));
  m = MatrixMultiply(m, rig);
  SetSpec(0, spec);
  gMatHand.maps[MATERIAL_MAP_ALBEDO].texture = tex;
  gMatHand.maps[MATERIAL_MAP_ALBEDO].color = tint;
  DrawMesh(mesh, gMatHand, m);
}

void HandDraw(float time, float stress) {
  if (!gHandInit) HandInit();
  gMatHand.shader = gGfx.world;
  Color skin = { 235, 225, 218, 255 };

  Vector3 eye = PlayerEye();
  Vector3 f = PlayerForward3D();
  Vector3 up0 = { 0, 1, 0 };
  Vector3 right = Vector3Normalize(Vector3CrossProduct(f, up0));
  Vector3 up = Vector3CrossProduct(right, f);
  Vector3 back = Vector3Scale(f, -1);

  // sway / bob / tremor (port of v1 feel, plus inertia from velocity)
  float spd = sqrtf(gPlayer.vel.x * gPlayer.vel.x + gPlayer.vel.z * gPlayer.vel.z);
  float bobA = (spd > 0.3f) ? (gPlayer.sprinting ? 0.010f : 0.006f) : 0.002f;
  float bx = sinf(gPlayer.bobPhase) * bobA;
  float by = fabsf(sinf(gPlayer.bobPhase * 2)) * bobA;
  float tr = (stress / 100.0f) * 0.004f;
  float ox = 0.225f + bx + frand2() * tr;
  float oy = -0.255f + by + frand2() * tr;
  float oz = -0.52f;

  Vector3 base = Vector3Add(eye, Vector3Add(Vector3Scale(right, ox), Vector3Add(Vector3Scale(up, oy), Vector3Scale(f, -oz))));
  Matrix rig = BasisMatrix(right, up, back, base);
  rig = MatrixMultiply(MatrixRotateXYZ({ -0.12f, -0.06f, 0.03f + sinf(gPlayer.bobPhase) * 0.01f }), rig);
  // NOTE: rotation must precompose in local space:
  rig = MatrixMultiply(MatrixMultiply(MatrixRotateXYZ({ -0.12f, -0.06f, 0.03f }), BasisMatrix(right, up, back, { 0, 0, 0 })), MatrixTranslate(base.x, base.y, base.z));

  // phone body + live screen
  HPart(mshPhoneBody, { 1, 1, 1 }, { 0, 0, 0 }, { 0, 0, 0 }, rig, gGfx.white, Color{ 24, 24, 28, 255 }, 0.55f);
  gMatHand.maps[MATERIAL_MAP_ALBEDO].texture = gGfx.phoneScr.texture;
  gMatHand.maps[MATERIAL_MAP_ALBEDO].color = WHITE;
  SetSpec(0, 0.0f);
  {
    Matrix m = MatrixMultiply(MatrixTranslate(0, 0, 0.0105f), rig);
    int le = GetShaderLocation(gGfx.world, "emisK");
    float em = 1.4f; SetShaderValue(gGfx.world, le, &em, SHADER_UNIFORM_FLOAT);
    DrawMesh(mshScreenQ, gMatHand, m);
    em = 0; SetShaderValue(gGfx.world, le, &em, SHADER_UNIFORM_FLOAT);
  }
  // right hand gripping
  HPart(mshPalm, { 0.07f, 0.13f, 0.055f }, { 0.085f, -0.10f, -0.012f }, { 0, 0, 0.18f }, rig, texSkinH, skin, 0.12f);
  HPart(mshFinger, { 0.026f, 0.085f, 0.026f }, { 0.045f, -0.105f, 0.022f }, { 0, 0, 1.15f }, rig, texSkinH, skin, 0.10f); // thumb
  for (int i = 0; i < 4; i++)
    HPart(mshFinger, { 0.024f, 0.062f, 0.024f }, { 0.068f, -0.045f - i * 0.038f, -0.030f }, { 0, 0.15f, PI / 2 - 0.25f }, rig, texSkinH, skin, 0.10f);
  HPart(mshPalm, { 0.062f, 0.17f, 0.062f }, { 0.13f, -0.25f, -0.04f }, { -0.3f, 0, 0.45f }, rig, texSkinH, skin, 0.10f); // wrist

  // ---- left hand: inspection / pickup
  float ia = gInv.inspectAnim, pa = gInv.pickupAnim;
  if (ia > 0.02f || pa > 0.02f) {
    float reach = pa > 0.02f ? sinf(pa * PI) : 0; // out and back
    float lx = -0.20f - 0.06f * ia, ly = -0.34f + 0.13f * ia + reach * 0.06f, lz = -0.46f - reach * 0.22f;
    Vector3 lbase = Vector3Add(eye, Vector3Add(Vector3Scale(right, lx), Vector3Add(Vector3Scale(up, ly), Vector3Scale(f, -lz))));
    Matrix lrig = MatrixMultiply(MatrixMultiply(MatrixRotateXYZ({ -0.3f + ia * 0.25f, 0.3f, -0.2f }), BasisMatrix(right, up, back, { 0, 0, 0 })), MatrixTranslate(lbase.x, lbase.y, lbase.z));
    HPart(mshPalm, { 0.07f, 0.13f, 0.05f }, { 0, 0, 0 }, { 0.3f, 0, -0.18f }, lrig, texSkinH, skin, 0.12f);
    HPart(mshFinger, { 0.026f, 0.08f, 0.026f }, { -0.045f, 0.05f, 0.02f }, { 0.5f, 0, -1.0f }, lrig, texSkinH, skin, 0.10f);
    for (int i = 0; i < 4; i++)
      HPart(mshFinger, { 0.022f, 0.06f, 0.022f }, { 0.03f - i * 0.020f, 0.075f, 0.012f }, { 1.45f, 0, 0.08f * i }, lrig, texSkinH, skin, 0.10f);
    int show = (pa > 0.02f) ? gInv.pickupItem : gInv.inspecting;
    float wob = sinf(time * 1.3f) * 0.06f;
    if (show == IT_WATERKEY || show == IT_KITCHENKEY) {
      Color iron = show == IT_WATERKEY ? Color{ 124, 96, 64, 255 } : Color{ 140, 134, 126, 255 };
      Matrix krig = MatrixMultiply(MatrixRotateXYZ({ 0.3f + wob, time * 0.0f, 0.2f }), MatrixMultiply(MatrixTranslate(0, 0.10f, 0.03f), lrig));
      HPart(mshKeyShaft, { 1, 1, 1 }, { 0, 0, 0 }, { 0, 0, PI / 2 }, krig, gGfx.white, iron, 0.7f);
      HPart(mshKeyBow, { 1, 1, 1 }, { -0.085f, 0, 0 }, { 0, 0, 0 }, krig, gGfx.white, iron, 0.7f);
      HPart(mshKeyTooth, { 1, 1, 1 }, { 0.065f, -0.026f, 0 }, { 0, 0, 0 }, krig, gGfx.white, iron, 0.7f);
    } else if (show == IT_NOTE) {
      Matrix nrig = MatrixMultiply(MatrixRotateXYZ({ -0.5f + wob, 0.15f, 0 }), MatrixMultiply(MatrixTranslate(0, 0.14f, 0.05f), lrig));
      gMatHand.maps[MATERIAL_MAP_ALBEDO].texture = texNote;
      gMatHand.maps[MATERIAL_MAP_ALBEDO].color = WHITE;
      SetSpec(0, 0.02f);
      DrawMesh(mshNoteQ, gMatHand, nrig);
    } else if (show == IT_BATTERY) {
      HPart(mshBattery, { 1, 1, 1 }, { 0, 0.11f, 0.03f }, { 0.4f + wob, 0.8f, 0 }, lrig, gGfx.white, Color{ 30, 160, 70, 255 }, 0.5f);
    }
  }
}

// ---------------------------------------------------------------- 2D HUD
bool gMenuActive = true;

void HudDraw(float time, float stress) {
  if (gMenuActive) {
    DrawRectangle(0, 0, INTERNAL_W, INTERNAL_H, Fade(BLACK, 0.55f));
    const char* title = "P A R A N O I A";
    Vector2 m = MeasureTextEx(gGfx.font, title, 42, 10);
    float fl = (sinf(time * 1.1f) > 0.96f) ? 0.35f : 1.0f;
    DrawTextEx(gGfx.font, title, { (INTERNAL_W - m.x) / 2, 84 }, 42, 10, Fade(Color{ 184, 184, 184, 255 }, fl));
    const char* sub2 = "YOU ARE BEING OBSERVED";
    m = MeasureTextEx(gGfx.font, sub2, 11, 5);
    DrawTextEx(gGfx.font, sub2, { (INTERNAL_W - m.x) / 2, 136 }, 11, 5, Color{ 95, 80, 80, 255 });
    const char* go = "PRESS  ENTER";
    m = MeasureTextEx(gGfx.font, go, 16, 4);
    if (sinf(time * 2.4f) > -0.3f)
      DrawTextEx(gGfx.font, go, { (INTERNAL_W - m.x) / 2, 210 }, 16, 4, Color{ 200, 200, 208, 255 });
    const char* keys = "WASD MOVE   MOUSE LOOK   SHIFT RUN   F LIGHT   E INTERACT   1-5 ITEMS";
    m = MeasureTextEx(gGfx.font, keys, 9, 2);
    DrawTextEx(gGfx.font, keys, { (INTERNAL_W - m.x) / 2, 268 }, 9, 2, Color{ 70, 70, 78, 255 });
    const char* hp = "HEADPHONES STRONGLY RECOMMENDED";
    m = MeasureTextEx(gGfx.font, hp, 9, 2);
    DrawTextEx(gGfx.font, hp, { (INTERNAL_W - m.x) / 2, 286 }, 9, 2, Color{ 70, 60, 60, 255 });
    const char* br = "vamp9 - MMXXVI";
    m = MeasureTextEx(gGfx.font, br, 9, 2);
    DrawTextEx(gGfx.font, br, { (INTERNAL_W - m.x) / 2, INTERNAL_H - 22.0f }, 9, 2, Color{ 52, 52, 52, 255 });
    return;
  }
  if (gDir.credits) {
    DrawRectangle(0, 0, INTERNAL_W, INTERNAL_H, BLACK);
    const char* title = "P A R A N O I A";
    Vector2 m = MeasureTextEx(gGfx.font, title, 30, 8);
    DrawTextEx(gGfx.font, title, { (INTERNAL_W - m.x) / 2, 70 }, 30, 8, Color{ 200, 196, 191, 255 });
    const char* end = gDir.ending == 1 ? "ENDING - TRUST ME" : "ENDING - REJECT TRUTH";
    m = MeasureTextEx(gGfx.font, end, 12, 3);
    DrawTextEx(gGfx.font, end, { (INTERNAL_W - m.x) / 2, 116 }, 12, 3, Color{ 106, 64, 64, 255 });
    const char* blurb = gDir.ending == 1
      ? "You stepped through the door it offered.\nThe recording continues. It always was you."
      : "The screen died. The dark is full of breathing.\nThey no longer need the camera to watch you.";
    m = MeasureTextEx(gGfx.font, blurb, 11, 2);
    DrawTextEx(gGfx.font, blurb, { (INTERNAL_W - m.x) / 2, 160 }, 11, 2, Color{ 119, 119, 119, 255 });
    const char* by = "a game by vamp9";
    m = MeasureTextEx(gGfx.font, by, 13, 3);
    DrawTextEx(gGfx.font, by, { (INTERNAL_W - m.x) / 2, 230 }, 13, 3, Color{ 170, 170, 170, 255 });
    const char* pc = "design - code - sound: procedural, like everything else you saw";
    m = MeasureTextEx(gGfx.font, pc, 9, 2);
    DrawTextEx(gGfx.font, pc, { (INTERNAL_W - m.x) / 2, 252 }, 9, 2, Color{ 68, 68, 68, 255 });
    const char* q = "[ESC] QUIT";
    m = MeasureTextEx(gGfx.font, q, 11, 2);
    DrawTextEx(gGfx.font, q, { (INTERNAL_W - m.x) / 2, 300 }, 11, 2, Color{ 85, 85, 85, 255 });
    return;
  }
  // inventory bar
  const char* names[5] = { "PHONE", "KEY", "KEY-2", "NOTE", "BATT" };
  int ids[5] = { IT_PHONE, IT_WATERKEY, IT_KITCHENKEY, IT_NOTE, IT_BATTERY };
  int x = 10, y = INTERNAL_H - 30;
  for (int i = 0; i < 5; i++) {
    bool has = gInv.has[ids[i]];
    bool sel = (ids[i] == IT_PHONE && gInv.inspecting < 0) || gInv.inspecting == ids[i];
    Color border = sel ? Color{ 200, 200, 210, 200 } : Color{ 90, 90, 100, has ? (unsigned char)150 : (unsigned char)50 };
    DrawRectangleLines(x, y, 38, 22, border);
    DrawTextEx(gGfx.font, TextFormat("%d", i + 1), { (float)x + 2, (float)y + 1 }, 8, 1, Fade(border, 0.8f));
    if (has) {
      const char* label = (ids[i] == IT_BATTERY) ? TextFormat("x%d", gInv.batteries) : names[i];
      DrawTextEx(gGfx.font, label, { (float)x + 7, (float)y + 9 }, 8, 1, Fade(WHITE, has ? 0.75f : 0.2f));
    }
    x += 42;
  }
  // prompt
  if (!gDir.ended) {
    std::string pr = (gDir.codeBuffer[0] != '\xFF')
      ? ("ENTER CODE: " + gDir.codeBuffer + std::string(4 - fminf(4, gDir.codeBuffer.size()), '_'))
      : DirectorPrompt();
    if (!pr.empty()) {
      Vector2 m = MeasureTextEx(gGfx.font, pr.c_str(), 12, 2);
      DrawTextEx(gGfx.font, pr.c_str(), { (INTERNAL_W - m.x) / 2, INTERNAL_H * 0.70f }, 12, 2, Color{ 207, 207, 214, 235 });
    }
  }
  // subtitle
  if (gDir.subT > 0 && !gDir.sub.empty()) {
    Vector2 m = MeasureTextEx(gGfx.font, gDir.sub.c_str(), 11, 2);
    DrawTextEx(gGfx.font, gDir.sub.c_str(), { (INTERNAL_W - m.x) / 2, INTERNAL_H * 0.84f }, 11, 2, Color{ 154, 154, 164, 220 });
  }
  // big text
  if (gDir.bigT > 0 && !gDir.big.empty()) {
    Vector2 m = MeasureTextEx(gGfx.font, gDir.big.c_str(), 30, 6);
    float a = fminf(1.0f, gDir.bigT);
    DrawTextEx(gGfx.font, gDir.big.c_str(), { (INTERNAL_W - m.x) / 2, INTERNAL_H * 0.40f }, 30, 6, Fade(Color{ 216, 212, 207, 255 }, a));
  }
}
