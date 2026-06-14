// PARANOIA v2 — graphics pipeline: low-res scene RT -> dither/quantize post.
// Flashlight shadow map rendered each frame from the camera-attached light.
#pragma once
#include "game.h"
#include "shaders.h"

Gfx gGfx;

// uniform locations
static struct {
  int viewPos, lightPos, lightDir, lightI, lightCut, fogD, ambientC, lightVP, shadowOn, specK, uTime, uSway, texNoise, texShadow;
} ULoc[2]; // 0 world, 1 worldInst
static struct { int uRes, uTime, uStress, uGlitch, uFlashW, uFlashB, uBattery, uDanger; } UPost;

static RenderTexture2D LoadDepthRT(int width, int height) {
  RenderTexture2D target = { 0 };
  target.id = rlLoadFramebuffer();
  target.texture.width = width;
  target.texture.height = height;
  if (target.id > 0) {
    rlEnableFramebuffer(target.id);
    target.depth.id = rlLoadTextureDepth(width, height, false);
    target.depth.width = width;
    target.depth.height = height;
    target.depth.format = 19;
    target.depth.mipmaps = 1;
    rlFramebufferAttach(target.id, target.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);
    gGfx.shadowOK = rlFramebufferComplete(target.id) ? 1 : 0;
    rlDisableFramebuffer();
  }
  return target;
}

Texture2D MakeNoiseTexture(int size, Color base, int variation) {
  Image img = GenImageColor(size, size, base);
  Color* px = (Color*)img.data;
  for (int i = 0; i < size * size; i++) {
    int n = GetRandomValue(-variation, variation);
    px[i].r = (unsigned char)Clamp(px[i].r + n, 0, 255);
    px[i].g = (unsigned char)Clamp(px[i].g + n, 0, 255);
    px[i].b = (unsigned char)Clamp(px[i].b + n, 0, 255);
  }
  Texture2D t = LoadTextureFromImage(img);
  UnloadImage(img);
  SetTextureWrap(t, TEXTURE_WRAP_REPEAT);
  SetTextureFilter(t, TEXTURE_FILTER_BILINEAR);
  return t;
}

static void CacheWorldULocs(int i, Shader sh) {
  ULoc[i].viewPos = GetShaderLocation(sh, "viewPos");
  ULoc[i].lightPos = GetShaderLocation(sh, "lightPos");
  ULoc[i].lightDir = GetShaderLocation(sh, "lightDir");
  ULoc[i].lightI = GetShaderLocation(sh, "lightI");
  ULoc[i].lightCut = GetShaderLocation(sh, "lightCut");
  ULoc[i].fogD = GetShaderLocation(sh, "fogD");
  ULoc[i].ambientC = GetShaderLocation(sh, "ambientC");
  ULoc[i].lightVP = GetShaderLocation(sh, "lightVP");
  ULoc[i].shadowOn = GetShaderLocation(sh, "shadowOn");
  ULoc[i].specK = GetShaderLocation(sh, "specK");
  ULoc[i].uTime = GetShaderLocation(sh, "uTime");
  ULoc[i].uSway = GetShaderLocation(sh, "uSway");
  ULoc[i].texNoise = GetShaderLocation(sh, "texNoise");
  ULoc[i].texShadow = GetShaderLocation(sh, "texShadow");
}

// scene RT with BOTH color and depth as sampleable textures
static RenderTexture2D LoadSceneRT(int w, int h) {
  RenderTexture2D t = { 0 };
  t.id = rlLoadFramebuffer();
  rlEnableFramebuffer(t.id);
  t.texture.id = rlLoadTexture(NULL, w, h, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
  t.texture.width = w; t.texture.height = h; t.texture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8; t.texture.mipmaps = 1;
  t.depth.id = rlLoadTextureDepth(w, h, false); // sampleable depth texture
  t.depth.width = w; t.depth.height = h; t.depth.format = 19; t.depth.mipmaps = 1;
  rlFramebufferAttach(t.id, t.texture.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
  rlFramebufferAttach(t.id, t.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);
  rlFramebufferComplete(t.id);
  rlDisableFramebuffer();
  return t;
}

void GfxInit() {
  gGfx.scene = LoadSceneRT(INTERNAL_W, INTERNAL_H);
  SetTextureFilter(gGfx.scene.texture, TEXTURE_FILTER_POINT);
  gGfx.phoneScr = LoadRenderTexture(PHONE_SCR_W, PHONE_SCR_H);
  SetTextureFilter(gGfx.phoneScr.texture, TEXTURE_FILTER_POINT);
  gGfx.shadowMap = LoadDepthRT(1024, 1024);

  std::string vs = std::string("#version 330\n") + VS_BODY;
  std::string vsi = std::string("#version 330\n#define INSTANCED\n") + VS_BODY;
  gGfx.world = LoadShaderFromMemory(vs.c_str(), FS_WORLD);
  gGfx.worldInst = LoadShaderFromMemory(vsi.c_str(), FS_WORLD);
  gGfx.worldInst.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocationAttrib(gGfx.worldInst, "instanceTransform");
  gGfx.depth = LoadShaderFromMemory(vs.c_str(), FS_DEPTH);
  gGfx.depthInst = LoadShaderFromMemory(vsi.c_str(), FS_DEPTH);
  gGfx.depthInst.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocationAttrib(gGfx.depthInst, "instanceTransform");
  gGfx.post = LoadShaderFromMemory(nullptr, FS_POST);
  CacheWorldULocs(0, gGfx.world);
  CacheWorldULocs(1, gGfx.worldInst);
  UPost.uRes = GetShaderLocation(gGfx.post, "uRes");
  UPost.uTime = GetShaderLocation(gGfx.post, "uTime");
  UPost.uStress = GetShaderLocation(gGfx.post, "uStress");
  UPost.uGlitch = GetShaderLocation(gGfx.post, "uGlitch");
  UPost.uFlashW = GetShaderLocation(gGfx.post, "uFlashW");
  UPost.uFlashB = GetShaderLocation(gGfx.post, "uFlashB");
  UPost.uBattery = GetShaderLocation(gGfx.post, "uBattery");
  UPost.uDanger = GetShaderLocation(gGfx.post, "uDanger");

  gGfx.noiseTex = MakeNoiseTexture(256, Color{ 128, 128, 128, 255 }, 70);
  Image w = GenImageColor(4, 4, WHITE);
  gGfx.white = LoadTextureFromImage(w);
  UnloadImage(w);
  gGfx.font = GetFontDefault();
}

// set per-frame uniforms on both world shaders
static void SetWorldUniforms(float time, float lightIntensity, Vector3 lightPos, Vector3 lightDir) {
  Vector3 eye = PlayerEye();
  float cut[2] = { cosf(0.34f), cosf(0.52f) };
  float fogD = (gPlayer.groundY < -1.0f) ? 0.10f : 0.052f;
  Vector3 amb = (gPlayer.groundY < -1.0f) ? Vector3{ 0.020f, 0.018f, 0.022f } : Vector3{ 0.026f, 0.030f, 0.044f };
  Shader shs[2] = { gGfx.world, gGfx.worldInst };
  for (int i = 0; i < 2; i++) {
    SetShaderValue(shs[i], ULoc[i].viewPos, &eye, SHADER_UNIFORM_VEC3);
    SetShaderValue(shs[i], ULoc[i].lightPos, &lightPos, SHADER_UNIFORM_VEC3);
    SetShaderValue(shs[i], ULoc[i].lightDir, &lightDir, SHADER_UNIFORM_VEC3);
    SetShaderValue(shs[i], ULoc[i].lightI, &lightIntensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shs[i], ULoc[i].lightCut, cut, SHADER_UNIFORM_VEC2);
    SetShaderValue(shs[i], ULoc[i].fogD, &fogD, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shs[i], ULoc[i].ambientC, &amb, SHADER_UNIFORM_VEC3);
    SetShaderValueMatrix(shs[i], ULoc[i].lightVP, gGfx.lightVP);
    SetShaderValue(shs[i], ULoc[i].shadowOn, &gGfx.shadowOK, SHADER_UNIFORM_INT);
    SetShaderValue(shs[i], ULoc[i].uTime, &time, SHADER_UNIFORM_FLOAT);
    // bind noise + shadow samplers to fixed slots
    rlEnableShader(shs[i].id);
    int slotN = 11, slotS = 10;
    rlActiveTextureSlot(slotN); rlEnableTexture(gGfx.noiseTex.id);
    rlSetUniform(ULoc[i].texNoise, &slotN, SHADER_UNIFORM_INT, 1);
    rlActiveTextureSlot(slotS); rlEnableTexture(gGfx.shadowMap.depth.id);
    rlSetUniform(ULoc[i].texShadow, &slotS, SHADER_UNIFORM_INT, 1);
    rlActiveTextureSlot(0);
  }
}

void SetSpec(int instanced, float spec) {
  SetShaderValue(instanced ? gGfx.worldInst : gGfx.world, ULoc[instanced].specK, &spec, SHADER_UNIFORM_FLOAT);
}
void SetSway(int instanced, int sway) {
  SetShaderValue(instanced ? gGfx.worldInst : gGfx.world, ULoc[instanced].uSway, &sway, SHADER_UNIFORM_INT);
}

void GfxRenderFrame(float time, float dt) {
  gDir.glitch = fmaxf(0, gDir.glitch - dt * 1.8f);
  gDir.flashWhite = fmaxf(0, gDir.flashWhite - dt * 0.7f);
  gDir.flashBlack = fmaxf(0, gDir.flashBlack - dt * 1.2f);

  Vector3 eye = PlayerEye();
  Vector3 fwd = PlayerForward3D();
  bool lightActive = gPhone.lightOn && !gPhone.flickerOff && gPhone.battery > 0;
  float batF = gPhone.battery <= 0 ? 0 : (0.45f + 0.55f * fminf(1.0f, gPhone.battery / 50.0f));
  bool indoor = (gPlayer.groundY < -1.0f) ||
    (fabsf(gPlayer.pos.x) < 6.5f && fabsf(gPlayer.pos.z + 110) < 5.5f) ||
    (fabsf(gPlayer.pos.x - 60) < 3.0f && fabsf(gPlayer.pos.z + 30) < 2.4f);
  float lightI = lightActive ? (indoor ? 3.1f : 7.2f) * batF : 0.0f;

  // ---- shadow pass (flashlight POV)
  if (gGfx.shadowOK && lightActive) {
    gGfx.lightCam.position = eye;
    gGfx.lightCam.target = Vector3Add(eye, fwd);
    gGfx.lightCam.up = { 0, 1, 0 };
    gGfx.lightCam.fovy = 62.0f;
    gGfx.lightCam.projection = CAMERA_PERSPECTIVE;
    BeginTextureMode(gGfx.shadowMap);
    rlSetClipPlanes(0.2, 40.0);
    ClearBackground(WHITE);
    BeginMode3D(gGfx.lightCam);
    Matrix lv = rlGetMatrixModelview();
    Matrix lp = rlGetMatrixProjection();
    WorldDraw(true, time);
    EndMode3D();
    EndTextureMode();
    gGfx.lightVP = MatrixMultiply(lv, lp);
  }

  // ---- WORLD pass: low-res (this layer gets the pixelated dither look)
  BeginTextureMode(gGfx.scene);
  rlSetClipPlanes(0.05, 320.0);
  ClearBackground(Color{ 1, 1, 3, 255 });
  BeginMode3D(gPlayer.cam);
  SetWorldUniforms(time, lightI, eye, fwd);
  int occ0 = 0;
  SetShaderValue(gGfx.world, GetShaderLocation(gGfx.world, "occlude"), &occ0, SHADER_UNIFORM_INT);
  SetShaderValue(gGfx.worldInst, GetShaderLocation(gGfx.worldInst, "occlude"), &occ0, SHADER_UNIFORM_INT);
  WorldDraw(false, time);
  EndMode3D();
  EndTextureMode();

  int sw = GetScreenWidth(), sh = GetScreenHeight();

  BeginDrawing();
  ClearBackground(BLACK);

  // ---- POST: draw the dithered world full-screen
  float res[2] = { (float)INTERNAL_W, (float)INTERNAL_H };
  float st = gDir.stress / 100.0f;
  // low battery itself drives the psychosis glitch, not just stress (BATTERY_SYSTEM §2A)
  float lowBat = gPhone.battery < 25.0f ? (25.0f - gPhone.battery) / 25.0f : 0.0f;
  float gl = gDir.glitch + (gDir.stress > 80 ? (gDir.stress - 80) / 100.0f : 0) + lowBat * 0.6f;
  float bat = gPhone.battery / 100.0f;
  SetShaderValue(gGfx.post, UPost.uRes, res, SHADER_UNIFORM_VEC2);
  SetShaderValue(gGfx.post, UPost.uTime, &time, SHADER_UNIFORM_FLOAT);
  SetShaderValue(gGfx.post, UPost.uStress, &st, SHADER_UNIFORM_FLOAT);
  SetShaderValue(gGfx.post, UPost.uGlitch, &gl, SHADER_UNIFORM_FLOAT);
  SetShaderValue(gGfx.post, UPost.uFlashW, &gDir.flashWhite, SHADER_UNIFORM_FLOAT);
  SetShaderValue(gGfx.post, UPost.uFlashB, &gDir.flashBlack, SHADER_UNIFORM_FLOAT);
  SetShaderValue(gGfx.post, UPost.uBattery, &bat, SHADER_UNIFORM_FLOAT);
  SetShaderValue(gGfx.post, UPost.uDanger, &gDir.danger, SHADER_UNIFORM_FLOAT);
  BeginShaderMode(gGfx.post);
  Rectangle srcS = { 0, 0, (float)INTERNAL_W, -(float)INTERNAL_H };
  Rectangle dstS = { 0, 0, (float)sw, (float)sh };
  DrawTexturePro(gGfx.scene.texture, srcS, dstS, Vector2{ 0, 0 }, 0, WHITE);
  EndShaderMode();

  // ---- ACTOR pass: full-res, crisp. You, the phone, items, the spectres.
  // Occlusion against the low-res world depth keeps them behind walls.
  rlDrawRenderBatchActive();
  glClearDepthHack();
  rlEnableDepthTest();
  rlSetClipPlanes(0.05, 320.0);   // MUST match the world pass for depth compare
  BeginMode3D(gPlayer.cam);
  SetWorldUniforms(time, lightI, eye, fwd);
  float ares[2] = { (float)sw, (float)sh };
  int occ1 = 1, depthSlot = 9;
  for (int i = 0; i < 2; i++) {
    Shader sh2 = i ? gGfx.worldInst : gGfx.world;
    rlEnableShader(sh2.id);
    SetShaderValue(sh2, GetShaderLocation(sh2, "occlude"), &occ1, SHADER_UNIFORM_INT);
    SetShaderValue(sh2, GetShaderLocation(sh2, "uResActor"), ares, SHADER_UNIFORM_VEC2);
    rlActiveTextureSlot(depthSlot); rlEnableTexture(gGfx.scene.depth.id);
    int ds = depthSlot;
    SetShaderValue(sh2, GetShaderLocation(sh2, "texWorldDepth"), &ds, SHADER_UNIFORM_INT);
    rlActiveTextureSlot(0);
  }
  for (auto& e : gEntities) if (!e.removed && e.visible) EntityDraw(e, time);
  if (gState == GS_PLAY || gState == GS_PAUSE) {
    rlDrawRenderBatchActive();
    glClearDepthHack();              // hand never clips the world or actors
    int occ0b = 0;
    SetShaderValue(gGfx.world, GetShaderLocation(gGfx.world, "occlude"), &occ0b, SHADER_UNIFORM_INT);
    HandDraw(time, gDir.stress);
  }
  EndMode3D();

  // ---- UI pass: full-res 2D, crisp text. Scaled from internal coords.
  float uiScale = (float)sw / INTERNAL_W;
  Camera2D ui = { 0 }; ui.zoom = uiScale;
  BeginMode2D(ui);
  HudDraw(time, gDir.stress);
  EndMode2D();

  EndDrawing();
}
