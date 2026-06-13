// PARANOIA v2 — native build. a game by vamp9.
// Unity build: every module is included into this translation unit.
#include "game.h"
#include "shaders.h"
#include "gfx.h"
#include "world.h"
#include "house.h"
#include "player.h"
#include "audio.h"
#include "phone.h"
#include "entity.h"
#include "director.h"

int main(int argc, char** argv) {
  // ---- args (debug/verification tooling)
  bool shotMode = false, windowed = false;
  const char* shotFile = "shot.png";
  float sx = 0, sz = 200, syaw = 0, spitch = 0, sground = 0;
  int shotFrames = 45;
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--windowed")) windowed = true;
    if (!strcmp(argv[i], "--shot") && i + 5 < argc) {
      shotMode = true;
      shotFile = argv[i + 1];
      sx = (float)atof(argv[i + 2]); sz = (float)atof(argv[i + 3]);
      syaw = (float)atof(argv[i + 4]); spitch = (float)atof(argv[i + 5]);
      if (i + 6 < argc) sground = (float)atof(argv[i + 6]);
    }
    if (!strcmp(argv[i], "--frames") && i + 1 < argc) shotFrames = atoi(argv[i + 1]);
  }

  SetTraceLogLevel(LOG_WARNING);
  SetConfigFlags(FLAG_VSYNC_HINT);
  InitWindow(1280, 720, "PARANOIA");
  if (!windowed && !shotMode) ToggleBorderlessWindowed();
  SetExitKey(KEY_NULL);
  SetRandomSeed(0x9A4F0911u);

  GfxInit();
  WorldBuild();
  if (!shotMode) AudioInit();

  gPlayer.cam.position = { 0, EYE_H, 200 };
  gPlayer.cam.target = { 0, EYE_H, 199 };
  gPlayer.cam.up = { 0, 1, 0 };
  gPlayer.cam.fovy = 71;
  gPlayer.cam.projection = CAMERA_PERSPECTIVE;
  PlayerTeleport(0, 200, 0);

  bool probeMode = false;
  for (int i = 1; i < argc; i++) if (!strcmp(argv[i], "--probe")) probeMode = true;
  if (probeMode) {
    gMenuActive = false;
    DirectorStart();
    gDir.catchCooldown = 1e9f;
    int pass = 0, fail = 0;
    auto CHECK = [&](const char* name, bool ok) {
      printf("[%s] %s\n", ok ? "PASS" : "FAIL", name);
      if (ok) pass++; else fail++;
    };
    // walk up the main stairs
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
    // water key pickup
    gDir.phase = 2;
    gPlayer.groundY = 0;
    PlayerTeleport(gWorld.waterKeyP.x + 0.4f, gWorld.waterKeyP.z, 0);
    gPhone.lightOn = true;
    DirectorInteract();
    CHECK("water key pickup", gDir.fWaterKey && gInv.has[IT_WATERKEY]);
    // cabin code
    gDir.codeBuffer = "";
    DirectorKey(KEY_TWO); DirectorKey(KEY_SEVEN); DirectorKey(KEY_FOUR); DirectorKey(KEY_ONE);
    CHECK("cabin code 2741", gDir.fCabinOpen);
    // kitchen drawer two-step
    gDir.fInHouse = true;
    PlayerTeleport(gWorld.kitchenDrawer.x - 0.6f, gWorld.kitchenDrawer.z, 0);
    DirectorInteract(); DirectorInteract();
    CHECK("kitchen key", gDir.fKitchenKey && gInv.has[IT_KITCHENKEY]);
    // bedroom door + basement door
    gPlayer.groundY = 3.1f;
    PlayerTeleport(gWorld.bedroomDoorP.x + 0.8f, gWorld.bedroomDoorP.z, 0);
    DirectorInteract();
    CHECK("bedroom unlock", gDir.fBedroomOpen);
    gPlayer.groundY = 0;
    PlayerTeleport(gWorld.basementDoorP.x, gWorld.basementDoorP.z + 1.0f, 0);
    DirectorInteract();
    CHECK("basement unlock (water key)", gDir.fBasementOpen);
    // mirror puzzle
    gDir.fMirrorSolved = false;
    gPlayer.groundY = 3.1f;
    PlayerTeleport(gWorld.mirrorSpot.x, gWorld.mirrorSpot.z, -PI / 2);
    for (int f = 0; f < 200; f++) DirectorUpdate(1.0f / 60.0f, f / 60.0f);
    CHECK("mirror puzzle solve", gDir.fMirrorSolved);
    // false wall + truth + note
    gDir.fBasementOnce = true;
    gPlayer.groundY = -2.5f;
    PlayerTeleport(-3.2f, -110.4f, PI / 2);
    DirectorInteract();
    CHECK("false wall opens", gDir.fHiddenOpen);
    PlayerTeleport(gWorld.oldPhoneP.x + 0.4f, gWorld.oldPhoneP.z, PI / 2);
    DirectorInteract();
    CHECK("truth (recording)", gDir.fTruth && gPhone.battery >= 99);
    DirectorInteract();
    CHECK("note pending", gDir.fNotePending);
    DirectorInteract();
    CHECK("note read", gDir.fNoteRead);
    // battery item use
    gInv.has[IT_BATTERY] = true; gInv.batteries = 1; gPhone.battery = 30;
    InvSelect(4);
    CHECK("battery swap +50", gPhone.battery > 79);
    // samara trigger
    gPlayer.groundY = 0;
    PlayerTeleport(gWorld.well.x, gWorld.well.z + 4.0f, 0);
    for (int f = 0; f < 60; f++) DirectorUpdate(1.0f / 60.0f, f / 60.0f);
    bool samaraUp = false;
    for (auto& e : gEntities) if (e.kind == EKind::Samara && e.state != EState::Dormant) samaraUp = true;
    CHECK("samara emerges", samaraUp);
    // ending refuse
    DirectorKey(KEY_Q);
    CHECK("ending refuse", gDir.ended && gDir.ending == 2);
    printf("== %d pass, %d fail ==\n", pass, fail);
    CloseWindow();
    return fail > 0 ? 1 : 0;
  }

  if (shotMode) {
    gMenuActive = false;
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

  float time = 0;
  bool started = false;
  while (!WindowShouldClose()) {
    float dt = fminf(0.05f, GetFrameTime());
    time += dt;

    if (gMenuActive) {
      if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
        gMenuActive = false;
        started = true;
        DisableCursor();
        gPlayer.enabled = true;
        DirectorStart();
      }
      // idle menu camera: stand at spawn, slow breathing
      gPlayer.swayT += dt;
    } else if (started) {
      // input routed to director first (codes, endings)
      int k = GetKeyPressed();
      while (k) { DirectorKey(k); k = GetKeyPressed(); }
      if (IsKeyPressed(KEY_F) && gPhone.battery > 0) {
        gPhone.lightOn = !gPhone.lightOn;
        SfxBeep(!gPhone.lightOn);
      }
      if (IsKeyPressed(KEY_E)) DirectorInteract();
      for (int s = 0; s < 5; s++)
        if (IsKeyPressed(KEY_ONE + s)) InvSelect(s);
      if (IsKeyPressed(KEY_F11)) ToggleBorderlessWindowed();
      if (IsKeyPressed(KEY_ESCAPE)) {
        if (gDir.credits) break;
        if (IsCursorHidden()) EnableCursor(); else DisableCursor();
      }
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !IsCursorHidden()) DisableCursor();
    }

    PlayerUpdate(dt, gDir.stress);
    if (started) DirectorUpdate(dt, time);
    InvUpdate(dt);
    bool indoor = (gPlayer.groundY < -1.0f) ||
      (fabsf(gPlayer.pos.x) < 6.5f && fabsf(gPlayer.pos.z + 110) < 5.5f) ||
      (fabsf(gPlayer.pos.x - 60) < 3.0f && fabsf(gPlayer.pos.z + 30) < 2.4f);
    PhoneUpdate(dt, time, indoor, gDir.entitiesNear, gDir.stress);
    WorldTick(dt, time);
    AudioSetListener(gPlayer.pos, gPlayer.yaw);
    PhoneDrawScreen(time, gDir.stress);
    GfxRenderFrame(time, dt);
  }

  if (!shotMode) AudioClose();
  CloseWindow();
  return 0;
}
