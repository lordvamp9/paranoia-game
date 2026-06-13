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
  // charging: 5s, fills battery, cancelled if you walk away
  if (gPhone.charging) {
    if (gPlayer.moving) { gPhone.charging = false; gPhone.message = "UNPLUGGED"; gPhone.messageT = 2; }
    else {
      gPhone.chargeT += dt;
      gPhone.battery = fminf(100.0f, gPhone.battery + (100.0f / 5.0f) * dt);
      gPhone.lightOn = true;
      static float pip = 0; pip -= dt;
      if (pip <= 0) { pip = 0.5f; SfxChargeTick(); }
      if (gPhone.chargeT >= 5.0f || gPhone.battery >= 100.0f) {
        gPhone.charging = false; gPhone.battery = 100; gPhone.message = "FULLY CHARGED"; gPhone.messageT = 3; SfxBeep(false);
      }
    }
  }
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
  const float PW = PHONE_SCR_W, PH = PHONE_SCR_H;   // 384 x 768 — crisp, legible
  BeginTextureMode(gGfx.phoneScr);
  if (gPhone.battery <= 0) { ClearBackground(BLACK); EndTextureMode(); return; }
  ClearBackground(Color{ 4, 7, 10, 255 });
  for (int i = 0; i < 60; i++)
    DrawRectangle(GetRandomValue(0, (int)PW), GetRandomValue(0, (int)PH), 3, 3, Fade(WHITE, 0.03f));
  // top bar: battery
  int bat = (int)ceilf(gPhone.battery);
  Color bc = bat < 20 ? (sinf(time * 8) > 0 ? Color{ 255, 70, 60, 255 } : Color{ 130, 34, 28, 255 }) : Color{ 150, 230, 150, 255 };
  DrawTextEx(gGfx.font, TextFormat("%d%%", bat), { 16, 16 }, 32, 1, bc);
  DrawRectangleLines(110, 20, 52, 26, bc);
  DrawRectangle(114, 24, (int)(44 * bat / 100.0f), 18, bc);
  DrawRectangle(162, 27, 5, 12, bc);
  // charging bolt
  if (gPhone.charging) DrawTextEx(gGfx.font, "CHG", { 200, 16 }, 26, 1, Color{ 150, 230, 150, (unsigned char)(150 + 100 * sinf(time * 10)) });
  // clock
  DrawTextEx(gGfx.font, s > 0.7f ? "??:??" : TextFormat("03:%02d", 33 + ((int)(time / 60)) % 27), { PW - 90, 16 }, 28, 1, Color{ 110, 110, 125, 255 });
  // objective
  char buf[96];
  if (!gPhone.objective.empty()) {
    CorruptText(gPhone.objective.c_str(), s, buf, 96);
    Vector2 m = MeasureTextEx(gGfx.font, buf, 24, 1);
    DrawTextEx(gGfx.font, buf, { (PW - m.x) / 2, 78 }, 24, 1, Fade(Color{ 222, 222, 232, 255 }, 0.9f - s * 0.25f));
  }
  // transient message — big and clear
  if (gPhone.messageT > 0 && sinf(time * 6) > -0.4f) {
    CorruptText(gPhone.message.c_str(), s, buf, 96);
    Vector2 m = MeasureTextEx(gGfx.font, buf, 36, 1);
    DrawTextEx(gGfx.font, buf, { (PW - m.x) / 2, PH / 2 - 24 }, 36, 1, Color{ 238, 232, 226, 245 });
  }
  // charging progress bar
  if (gPhone.charging) {
    float cw = PW - 80;
    DrawRectangleLines(40, PH / 2 + 60, (int)cw, 22, Color{ 90, 110, 90, 255 });
    DrawRectangle(43, PH / 2 + 63, (int)(cw * gPhone.chargeT / 5.0f), 16, Color{ 110, 220, 120, 230 });
    Vector2 m = MeasureTextEx(gGfx.font, "CHARGING", 22, 1);
    DrawTextEx(gGfx.font, "CHARGING", { (PW - m.x) / 2, PH / 2 + 30 }, 22, 1, Color{ 150, 230, 150, 255 });
  }
  // stamina strip
  DrawRectangle(16, PH - 96, (int)(96 * gPlayer.stamina / 100.0f), 6, gPlayer.exhausted ? Color{ 200, 60, 50, 200 } : Color{ 120, 130, 150, 160 });
  if (s > 0.45f && frand() < 0.4f) {
    Vector2 m = MeasureTextEx(gGfx.font, "WATCHING", 22, 1);
    DrawTextEx(gGfx.font, "WATCHING", { (PW - m.x) / 2, 150 }, 22, 1, Fade(Color{ 210, 64, 58, 255 }, (s - 0.45f) * 1.2f));
  }
  // REC — always
  DrawCircle(28, PH - 44, 10, sinf(time * 3.5f) > 0 ? Color{ 255, 70, 60, 255 } : Fade(Color{ 255, 70, 60, 255 }, 0.25f));
  DrawTextEx(gGfx.font, "REC", { 46, PH - 56 }, 26, 1, Color{ 190, 190, 190, 255 });
  int rt = (int)time;
  DrawTextEx(gGfx.font, TextFormat("%02d:%02d:%02d", rt / 3600, (rt / 60) % 60, rt % 60), { 130, PH - 52 }, 20, 1, Color{ 90, 90, 90, 255 });
  if (bat < 20 && sinf(time * 5) > 0) {
    Vector2 m = MeasureTextEx(gGfx.font, "LOW BATTERY", 28, 1);
    DrawTextEx(gGfx.font, "LOW BATTERY", { (PW - m.x) / 2, PH - 128 }, 28, 1, Color{ 255, 70, 60, 255 });
  }
  if (s > 0.4f) {
    int n = (int)((s - 0.4f) * 12);
    for (int i = 0; i < n; i++) {
      int y = GetRandomValue(0, (int)PH);
      DrawRectangle(GetRandomValue(-24, 24), y, (int)PW, GetRandomValue(3, 12), Fade(frand() < 0.5f ? WHITE : Color{ 255, 40, 40, 255 }, 0.07f));
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
void HudDraw(float time, float stress) {
  if (gState != GS_PLAY) { UIDrawOverlay(time); return; }

  // rain streaks (screen-space, outdoors) — sells the storm with the dither
  if (gDir.rainVisual > 0.02f) {
    int n = (int)(gDir.rainVisual * 90);
    for (int i = 0; i < n; i++) {
      float rx = fmodf(i * 53.7f + time * 220.0f * (0.6f + (i % 5) * 0.12f), (float)INTERNAL_W);
      float ry = fmodf(i * 97.3f + time * 540.0f * (0.7f + (i % 7) * 0.1f), (float)INTERNAL_H);
      float len = 7 + (i % 5) * 3;
      DrawLineEx({ rx, ry }, { rx - 1.5f, ry + len }, 1.0f, Fade(Color{ 170, 180, 200, 255 }, 0.18f * gDir.rainVisual));
    }
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
  // lore note reading panel — centered, legible
  if (!gDir.readingNote.empty()) {
    int pw = 300, ph2 = 150;
    int px = (INTERNAL_W - pw) / 2, py = (INTERNAL_H - ph2) / 2;
    DrawRectangle(0, 0, INTERNAL_W, INTERNAL_H, Fade(BLACK, 0.55f));
    DrawRectangle(px, py, pw, ph2, Color{ 18, 16, 13, 245 });
    DrawRectangleLines(px, py, pw, ph2, Color{ 74, 68, 54, 255 });
    DrawTextEx(gGfx.font, gDir.readingNote.c_str(), { (float)px + 16, (float)py + 16 }, 11, 1, Color{ 206, 200, 184, 255 });
    DrawTextEx(gGfx.font, "[E] CLOSE", { (float)px + pw - 70, (float)py + ph2 - 16 }, 9, 1, Color{ 120, 116, 104, 255 });
    return; // suppress the rest of the HUD while reading
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
