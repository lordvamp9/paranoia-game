// PARANOIA v3 — native build. a game by vamp9.
// Unity build: every module is included into this translation unit.
#include "game.h"
#include "shaders.h"
#include "gfx.h"
#include "config.h"
#include "world.h"
#include "house.h"
#include "player.h"
#include "audio.h"
#include "music.h"
#include "phone.h"
#include "entity.h"
#include "director.h"
#include "ui.h"

static void BeginPlay() {
  DirectorReset();
  PlayerTeleport(0, 200, 0);
  gPlayer.pitch = 0;
  gPlayer.enabled = true;
  DirectorStart();
  gState = GS_PLAY;
  gStateT = 0;
  DisableCursor();
}

int main(int argc, char** argv) {
  bool shotMode = false, windowed = false, probeMode = false;
  const char* shotFile = "shot.png";
  const char* uiShot = nullptr; int uiState = GS_MENU;
  float sx = 0, sz = 200, syaw = 0, spitch = 0, sground = 0;
  int shotFrames = 45;
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--windowed")) windowed = true;
    if (!strcmp(argv[i], "--probe")) probeMode = true;
    if (!strcmp(argv[i], "--frames") && i + 1 < argc) shotFrames = atoi(argv[i + 1]);
    if (!strcmp(argv[i], "--uishot") && i + 2 < argc) { uiShot = argv[i + 1]; uiState = atoi(argv[i + 2]); }
    if (!strcmp(argv[i], "--portrait") && i + 2 < argc) { uiShot = argv[i + 1]; uiState = 100 + atoi(argv[i + 2]); }
    if (!strcmp(argv[i], "--shot") && i + 5 < argc) {
      shotMode = true;
      shotFile = argv[i + 1];
      sx = (float)atof(argv[i + 2]); sz = (float)atof(argv[i + 3]);
      syaw = (float)atof(argv[i + 4]); spitch = (float)atof(argv[i + 5]);
      if (i + 6 < argc) sground = (float)atof(argv[i + 6]);
    }
  }

  SetTraceLogLevel(LOG_WARNING);
  SetConfigFlags(FLAG_VSYNC_HINT);
  InitWindow(1280, 720, "PARANOIA");
  SetExitKey(KEY_NULL);
  SetRandomSeed(0x9A4F0911u);

  GfxInit();
  CfgLoad();
  if (!windowed && !shotMode) ApplyDisplayMode(gCfg.fullscreen); // borderless / fullscreen from settings
  WorldBuild();
  if (!shotMode) { AudioInit(); AudioSetVolumes(gCfg.volMaster, gCfg.volMusic, gCfg.volAmbient, gCfg.volVoices); }

  gPlayer.cam.position = { 0, EYE_H, 200 };
  gPlayer.cam.target = { 0, EYE_H, 199 };
  gPlayer.cam.up = { 0, 1, 0 };
  gPlayer.cam.fovy = 71;
  gPlayer.cam.projection = CAMERA_PERSPECTIVE;
  PlayerTeleport(0, 200, 0);

  // -------- probe: headless logic checks --------
  if (probeMode) {
    gState = GS_PLAY;
    DirectorStart();
    gDir.catchCooldown = 1e9f;
    int pass = 0, fail = 0;
    auto CHECK = [&](const char* name, bool ok) { printf("[%s] %s\n", ok ? "PASS" : "FAIL", name); if (ok) pass++; else fail++; };
    gPlayer.groundY = 0;
    for (float z = -110.3f; z >= -114.7f; z -= 0.2f) gPlayer.groundY = ResolveGround(-5, z, gPlayer.groundY);
    CHECK("stairs up -> 3.1", fabsf(gPlayer.groundY - 3.1f) < 0.15f);
    gPlayer.groundY = ResolveGround(-3.5f, -114.2f, gPlayer.groundY);
    CHECK("bedroom floor 3.1", fabsf(gPlayer.groundY - 3.1f) < 0.05f);
    for (float z = -114.7f; z <= -110.3f; z += 0.2f) gPlayer.groundY = ResolveGround(-5, z, gPlayer.groundY);
    CHECK("stairs back down", gPlayer.groundY < 0.4f);
    gPlayer.groundY = 0;
    for (float z = -110.4f; z >= -114.7f; z -= 0.2f) gPlayer.groundY = ResolveGround(5, z, gPlayer.groundY);
    CHECK("basement stairs -> -2.5", fabsf(gPlayer.groundY + 2.5f) < 0.15f);
    CHECK("basement room", fabsf(ResolveGround(0, -112, gPlayer.groundY) + 2.5f) < 0.05f);
    { float wx = 0, wz = -101; for (int s = 0; s < 240; s++) { wz -= 0.05f; Collide(wx, wz, 0); }
      CHECK("walk through front door", wz < -107.0f && fabsf(wx) < 1.5f); }
    gDir.phase = 2; gPlayer.groundY = 0;
    PlayerTeleport(gWorld.waterKeyP.x + 0.4f, gWorld.waterKeyP.z, 0); gPhone.lightOn = true;
    DirectorInteract(); CHECK("water key pickup", gDir.fWaterKey && gInv.has[IT_WATERKEY]);
    gDir.codeBuffer = ""; DirectorKey(KEY_TWO); DirectorKey(KEY_SEVEN); DirectorKey(KEY_FOUR); DirectorKey(KEY_ONE);
    CHECK("cabin code 2741", gDir.fCabinOpen);
    gDir.fInHouse = true;
    PlayerTeleport(gWorld.kitchenDrawer.x - 0.6f, gWorld.kitchenDrawer.z, 0);
    DirectorInteract(); DirectorInteract(); CHECK("kitchen key", gDir.fKitchenKey && gInv.has[IT_KITCHENKEY]);
    gPlayer.groundY = 3.1f; PlayerTeleport(gWorld.bedroomDoorP.x + 0.8f, gWorld.bedroomDoorP.z, 0);
    DirectorInteract(); CHECK("bedroom unlock", gDir.fBedroomOpen);
    gPlayer.groundY = 0; PlayerTeleport(gWorld.basementDoorP.x, gWorld.basementDoorP.z + 1.0f, 0);
    DirectorInteract(); CHECK("basement unlock (water key)", gDir.fBasementOpen);
    gDir.fMirrorSolved = false; gPlayer.groundY = 3.1f;
    PlayerTeleport(gWorld.mirrorSpot.x, gWorld.mirrorSpot.z, -PI / 2);
    for (int f = 0; f < 200; f++) DirectorUpdate(1.0f / 60.0f, f / 60.0f);
    CHECK("mirror puzzle solve", gDir.fMirrorSolved);
    gDir.fBasementOnce = true; gPlayer.groundY = -2.5f;
    PlayerTeleport(-3.2f, -110.4f, PI / 2); DirectorInteract(); CHECK("false wall opens", gDir.fHiddenOpen);
    PlayerTeleport(gWorld.oldPhoneP.x + 0.4f, gWorld.oldPhoneP.z, PI / 2);
    DirectorInteract(); CHECK("truth (recording)", gDir.fTruth && gPhone.battery >= 99);
    DirectorInteract(); CHECK("note pending", gDir.fNotePending);
    DirectorInteract(); CHECK("note read", gDir.fNoteRead);
    gInv.has[IT_BATTERY] = true; gInv.batteries = 1; gPhone.battery = 30; InvSelect(4);
    CHECK("battery swap +50", gPhone.battery > 79);
    gPlayer.groundY = 0; PlayerTeleport(gWorld.well.x, gWorld.well.z + 4.0f, 0);
    for (int f = 0; f < 60; f++) DirectorUpdate(1.0f / 60.0f, f / 60.0f);
    bool samaraUp = false; for (auto& e : gEntities) if (e.kind == EKind::Samara && e.state != EState::Dormant) samaraUp = true;
    CHECK("samara emerges", samaraUp);
    DirectorKey(KEY_Q); CHECK("ending refuse", gDir.ended && gDir.ending == 2);
    // psychosis: 0% battery -> collapse -> loop reclaims you (respawn @30%)
    gDir.ended = false; gDir.psychosis = false; gPlayer.enabled = true;
    gDir.checkpoint = { 0, 0, 200 }; gDir.checkpointYaw = 0; gPhone.battery = 0;
    DirectorUpdate(1.0f / 60.0f, 0);
    bool pStarted = gDir.psychosis && !gPlayer.enabled;
    int guard = 0; while (gDir.psychosis && guard++ < 600) DirectorUpdate(1.0f / 60.0f, guard / 60.0f);
    CHECK("psychosis death at 0% battery", pStarted);
    CHECK("psychosis respawns @30% + re-enable", !gDir.psychosis && gPlayer.enabled && gPhone.battery > 25 && gPhone.battery <= 35);
    // pacing governor: a long calm earns a dread beat (tension never hits 0)
    DirectorReset();
    gDir.phase = 2; gPlayer.enabled = true; gPlayer.groundY = 0; gDir.stress = 0;
    PlayerTeleport(0, 100, 0); gCalmT = 0;
    float calmPeak = 0;
    for (int f = 0; f < 1600; f++) { DirectorUpdate(1.0f / 60.0f, f / 60.0f); calmPeak = fmaxf(calmPeak, gDir.stress); }
    CHECK("pacing floor nudges tension in long calm", calmPeak > 4.5f);
    // BUG-3: entity separation un-stacks overlapping spawns
    DirectorReset();
    SpawnShadow({ 0, 0, 0 }, {}, 1.0f, true);
    SpawnShadow({ 0.05f, 0, 0.05f }, {}, 1.0f, true); // almost on top
    for (int f = 0; f < 40; f++) for (auto& e : gEntities) EntityUpdate(e, 1.0f / 60.0f, f / 60.0f);
    { float dx = gEntities[0].pos.x - gEntities[1].pos.x, dz = gEntities[0].pos.z - gEntities[1].pos.z;
      CHECK("entity separation un-stacks overlap", dx * dx + dz * dz > 0.3f); }
    // BUG-1: object pool compacts spent entities instead of growing forever
    DirectorReset();
    for (int i = 0; i < 20; i++) SpawnWatcher({ frand2() * 12, 0, frand2() * 12 });
    { size_t before = gEntities.size();
      for (auto& e : gEntities) e.removed = true;
      gDir.phase = 2; DirectorUpdate(1.0f / 60.0f, 0);
      CHECK("object pool compacts spent entities", gEntities.size() < before); }
    // reset path
    DirectorReset(); CHECK("reset clears entities", gEntities.empty() && !gDir.fWaterKey);
    printf("== %d pass, %d fail ==\n", pass, fail);
    CloseWindow();
    return fail > 0 ? 1 : 0;
  }

  // -------- shot: auto-screenshot --------
  if (shotMode) {
    gState = GS_PLAY;
    DirectorStart();
    gDir.catchCooldown = 1e9f;
    gPlayer.groundY = sground;
    PlayerTeleport(sx, sz, syaw);
    gPlayer.pitch = spitch;
    gDir.flashBlack = 0;
    for (int f = 0; f < shotFrames; f++) {
      float dt = 1.0f / 60.0f, time = f * dt;
      PlayerUpdate(dt, gDir.stress);
      DirectorUpdate(dt, time);
      InvUpdate(dt);
      PhoneUpdate(dt, time, false, gDir.entitiesNear, gDir.stress);
      gPhone.flickerOff = false;
      WorldTick(dt, time);
      PhoneDrawScreen(time, gDir.stress);
      gDir.flashBlack = 0; gDir.flashWhite = 0;
      GfxRenderFrame(time, dt);
    }
    TakeScreenshot(shotFile);
    CloseWindow();
    return 0;
  }

  // -------- entity portrait --------
  if (uiShot && uiState >= 100) {
    int kind = uiState - 100;
    gState = GS_PLAY;
    gPhone.lightOn = true; gPhone.battery = 100;
    PlayerTeleport(0, 0, 0); gPlayer.pitch = 0; gPlayer.groundY = 0;
    Entity e;
    e.pos = { 0, 0, -3.4f }; e.home = e.pos; e.facing = 0; e.visible = true; e.frozen = true;
    e.state = (kind == 1) ? EState::Pursuing : EState::Idle; // samara crawling
    if (kind == 0) { e.kind = EKind::Shadow; e.decay = 0.5f; e.heightScale = 1.05f; }
    else if (kind == 1) { e.kind = EKind::Samara; e.walkPhase = 1.2f; e.decay = 0.6f; e.heightScale = 0.82f; }
    else if (kind == 2) { e.kind = EKind::Watcher; e.decay = 0.85f; e.heightScale = 1.14f; }
    else { e.kind = EKind::Crawler; e.decay = 0.25f; e.heightScale = 1.0f; }
    gEntities.push_back(e);
    for (int f = 0; f < 4; f++) { PlayerUpdate(1.0f / 60.0f, 0); gDir.flashBlack = 0; PhoneDrawScreen(0.5f, 20); GfxRenderFrame(1.2f, 1.0f / 60.0f); }
    TakeScreenshot(uiShot);
    CloseWindow();
    return 0;
  }

  // -------- ui screenshot --------
  if (uiShot) {
    gState = uiState; gStateT = (uiState == GS_INTRO) ? 5.4f : 2.0f;
    if (uiState == GS_PAUSE || uiState == GS_SETTINGS) { DirectorStart(); gState = uiState; gDir.flashBlack = 0; }
    for (int f = 0; f < 4; f++) { gDir.flashBlack = 0; PhoneDrawScreen(0.5f, 0); GfxRenderFrame(0.5f, 1.0f / 60.0f); }
    TakeScreenshot(uiShot);
    CloseWindow();
    return 0;
  }

  // -------- normal --------
  EnableCursor();
  MusicSetMode(MUS_MENU);
  int prevState = -1;
  float time = 0;
  while (!WindowShouldClose() && !gReqQuit) {
    float dt = fminf(0.05f, GetFrameTime());
    time += dt;
    gStateT += dt;

    // state-entry hooks
    if (gState != prevState) {
      if (gState == GS_MENU) { EnableCursor(); MusicSetMode(MUS_MENU); }
      else if (gState == GS_PLAY) { DisableCursor(); }
      else if (gState == GS_PAUSE || gState == GS_SETTINGS) { EnableCursor(); }
      else if (gState == GS_CREDITS) { EnableCursor(); }
      prevState = gState;
    }

    // restart requested (exit to menu)
    if (gReqRestart) { gReqRestart = false; DirectorReset(); PlayerTeleport(0, 200, 0); gPlayer.enabled = false; gState = GS_MENU; prevState = -1; }

    // ---- per-state input & updates
    if (gState == GS_PLAY) {
      int k = GetKeyPressed();
      while (k) { DirectorKey(k); k = GetKeyPressed(); }
      if (ActPressed(ACT_LIGHT) && gPhone.battery > 0) { gPhone.lightOn = !gPhone.lightOn; SfxBeep(!gPhone.lightOn); }
      if (ActPressed(ACT_INTERACT)) DirectorInteract();
      for (int s = 0; s < 5; s++) if (IsKeyPressed(KEY_ONE + s)) InvSelect(s);
      if (IsKeyPressed(KEY_F11)) { gCfg.fullscreen = !gCfg.fullscreen; ApplyDisplayMode(gCfg.fullscreen); CfgSave(); }
      if (IsKeyPressed(KEY_ESCAPE)) { gState = GS_PAUSE; gStateT = 0; }

      PlayerUpdate(dt, gDir.stress);
      DirectorUpdate(dt, time);
      InvUpdate(dt);
      bool indoor = (gPlayer.groundY < -1.0f) ||
        (fabsf(gPlayer.pos.x) < 6.5f && fabsf(gPlayer.pos.z + 110) < 5.5f) ||
        (fabsf(gPlayer.pos.x - 60) < 3.0f && fabsf(gPlayer.pos.z + 30) < 2.4f);
      PhoneUpdate(dt, time, indoor, gDir.entitiesNear, gDir.stress);
      WorldTick(dt, time);
      AudioSetListener(gPlayer.pos, gPlayer.yaw);
      if (gDir.credits) { gState = GS_CREDITS; gStateT = 0; }
    } else if (gState == GS_INTRO) {
      MusicTick(dt);
      PlayerUpdate(dt, 0); // keep camera valid at spawn
      if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE) || gStateT > 27.0f)
        BeginPlay();
    } else if (gState == GS_MENU) {
      MusicTick(dt);
      PlayerUpdate(dt, 0);
    } else if (gState == GS_PAUSE) {
      if (IsKeyPressed(KEY_ESCAPE)) { gState = GS_PLAY; }
      // world frozen; keep camera
    } else if (gState == GS_SETTINGS) {
      if (IsKeyPressed(KEY_ESCAPE) && gRebindAction < 0) { CfgSave(); gState = gFromPause ? GS_PAUSE : GS_MENU; }
    } else if (gState == GS_CREDITS) {
      if (IsKeyPressed(KEY_ENTER)) { gReqRestart = true; }
      if (IsKeyPressed(KEY_ESCAPE)) break;
    }

    PhoneDrawScreen(time, gDir.stress);
    GfxRenderFrame(time, dt);
  }

  AudioClose();
  CfgSave();
  CloseWindow();
  return 0;
}
