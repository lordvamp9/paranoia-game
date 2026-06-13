// PARANOIA v2 — director: phases, scripted dread, puzzles, endings.
#pragma once
#include "game.h"

Director gDir;

static int gE0 = -1;            // the phase-1 watcher (scripted)
static float gE0moveT = -1; static Vector3 gE0from, gE0to;
static float gWatcherT = 40;    // next watcher apparition
static float gBreathT = 0, gHeartT = 0, gWhisperT = 9, gDripT = 4, gThunderT = 8;
static bool fired[16] = { false };
#define FIRE(n) (!fired[n] && (fired[n] = true))

static void Sub(const char* t, float sec) { gDir.sub = t; gDir.subT = sec; }
static void Big(const char* t, float sec) { gDir.big = t; gDir.bigT = sec; }

void WorldResetDynamics();

void DirectorReset() {
  gE0 = -1; gE0moveT = -1; gWatcherT = 40;
  gBreathT = gHeartT = 0; gWhisperT = 9; gDripT = 4; gThunderT = 8;
  for (int i = 0; i < 16; i++) fired[i] = false;
  gEntities.clear();
  gDir = Director{};
  gPhone = Phone{};
  gInv = Inventory{};
  WorldResetDynamics();
  MusicSetMode(MUS_OFF);
}

void DirectorStart() {
  gDir.phase = 1;
  gDir.flashBlack = 2.2f;
  gPhone.objective = "FOLLOW THE PATH";
  gPhone.message = "TRUST ME"; gPhone.messageT = 0; // shown at 3.5s
  AudioSetWind(1.0f);
  // scripted phase-1 shadow
  SpawnShadow({ pathX(120) - 14, 0, 120 }, {}, 1.0f, false);
  gE0 = (int)gEntities.size() - 1;
  gEntities[gE0].frozen = true;
  gEntities[gE0].visible = false;
  SpawnSamara();
  gDir.checkpoint = { 0, 0, 200 }; gDir.checkpointYaw = 0;
}

static void StartPhase2() {
  gDir.phase = 2;
  gPhone.objective = "REACH THE WHITE HOUSE";
  Sub("The trees open. Something white waits in the distance.", 5);
  gDir.checkpoint = { pathX(58), 0, 58 }; gDir.checkpointYaw = 0;
  Entity& e0 = gEntities[gE0];
  gE0from = e0.pos; gE0to = { e0.pos.x - 20, 0, e0.pos.z }; gE0moveT = 0;
  SpawnShadow({ -8, 0, 30 }, { { -6, 0, 40 }, { 2, 0, 5 }, { -4, 0, -35 }, { -20, 0, -10 } }, 1.0f, true);
  SpawnShadow({ 38, 0, -45 }, { { 35, 0, -15 }, { 20, 0, -45 }, { 48, 0, -52 }, { 55, 0, -20 } }, 1.05f, true);
}

static void StartPhase3() {
  gDir.phase = 3;
  gPhone.objective = "ENTER THE HOUSE";
  gDir.checkpoint = { 0, 0, -88 }; gDir.checkpointYaw = 0;
  SpawnShadow({ -18, 0, -96 }, { { -16, 0, -95 }, { 14, 0, -94 }, { 20, 0, -122 }, { -19, 0, -125 } }, 1.1f, true);
}

static void EnterHouse() {
  gDir.fInHouse = true;
  gPhone.objective = "FIND A WAY DOWN";
  Sub("Too quiet.", 4);
  gDir.checkpoint = { 0, 0, -106.5f }; gDir.checkpointYaw = PI; // just inside, facing in
  AudioSetWind(0.25f);
  // interior stalker patrols the BACK of the house, never camps the doorway
  SpawnShadow({ 3.5f, 0, -113.5f },
    { { 4.2f, 0, -113.5f }, { -3.5f, 0, -113.0f }, { -4.5f, 0, -108.5f }, { 3.0f, 0, -108.0f } },
    0.8f, true);
  gDir.graceT = 4.0f; // a few seconds to get your bearings before it hunts
}

void DirectorCatch(Entity& e) {
  if (gDir.catchCooldown > 0 || gDir.graceT > 0 || gDir.ended) return;
  gDir.catchCooldown = 5;
  gDir.graceT = 4.5f; // breathing room after respawn so it's not a death loop
  gDir.flashBlack = 1.7f;
  SfxScare();
  gDir.stress = 85;
  gPhone.battery = fmaxf(5, gPhone.battery - 8);
  PlayerTeleport(gDir.checkpoint.x, gDir.checkpoint.z, gDir.checkpointYaw);
  for (auto& en : gEntities) {
    if (en.frozen || en.removed || !en.lethal) continue;
    if (en.kind == EKind::Samara) { if (en.state != EState::Dormant) { en.state = EState::Retreating; en.stateT = 0; } continue; }
    en.pos = en.home; en.state = EState::Retreating; en.stateT = 0; // walk away, don't re-camp
  }
  Big(e.kind == EKind::Samara ? "SHE FOUND YOU" : "IT TOUCHED YOU", 2.4f);
}

// ---------------------------------------------------------------- phases
static void Phase1Script(float time) {
  float z = gPlayer.pos.z;
  Entity& e0 = gEntities[gE0];
  if (z < 172 && FIRE(0)) {
    SfxBranch(-0.8f);
    e0.visible = true;
    e0.pos = { pathX(z - 18) - 7, 0, z - 18 };
    gE0from = e0.pos; gE0to = { pathX(z - 19) + 8, 0, z - 19 }; gE0moveT = 0;
    gDir.stress += 12;
  }
  if (z < 150 && FIRE(1)) {
    Sub("Footsteps. Behind you.", 3.5f);
    gDir.whispers = true;
    gDir.stress += 8;
  }
  if (z < 122 && FIRE(2)) {
    gPhone.flickerOff = true; gPhone.flickerT = 0.4f;
    SfxBeep(true);
    gDir.stress += 6; gDir.glitch += 0.5f;
  }
  if (z < 92 && FIRE(3)) {
    e0.visible = true;
    e0.pos = { pathX(74), 0, 74 };
    SfxSting(1.1f);
    gDir.stress += 18;
    Sub("It is standing on the path.", 4);
  }
  if (fired[3] && !fired[4]) {
    Entity& e = gEntities[gE0];
    e.facing = atan2f(gPlayer.pos.x - e.pos.x, gPlayer.pos.z - e.pos.z);
    float d = Vector2Distance({ e.pos.x, e.pos.z }, { gPlayer.pos.x, gPlayer.pos.z });
    if (d < 3.2f || z < 71) {
      fired[4] = true;
      gE0from = e.pos; gE0to = { e.pos.x - 16, 0, e.pos.z - 4 }; gE0moveT = 0;
      SfxBranch(0.9f);
      gDir.glitch += 0.8f;
      gDir.stress += 10;
    }
  }
  if (z < 62) StartPhase2();
}

static void Phase2Script() {
  Vector3 p = gPlayer.pos;
  if (p.z < 30 && FIRE(5)) { SfxWhisper(); Sub("Whispers. They are not words.", 4); }
  if (p.z < -20 && FIRE(6)) {
    for (auto& e : gEntities) {
      if (e.frozen || e.removed || e.kind != EKind::Shadow || !e.lethal) continue;
      e.lastSeen = { p.x + frand2() * 10, 0, p.z + frand2() * 10 };
      e.state = EState::Alerted; e.stateT = 0;
    }
    SfxSting(0.7f);
    Sub("Two breathing patterns. Neither is yours.", 4.5f);
  }
  if (p.z < -85) StartPhase3();
}

static void Phase3Script() {
  Vector3 p = gPlayer.pos;
  bool inHouse = fabsf(p.x) < 6 && fabsf(p.z + 110) < 5;
  if (inHouse && !gDir.fInHouse) EnterHouse();
  if (gDir.fInHouse && gPlayer.groundY < -2 && !gDir.fBasementOnce) {
    gDir.fBasementOnce = true;
    gPhone.objective = "SOMETHING IS HERE";
    Sub("They did not follow you down.", 5);
    gDir.checkpoint = { 5, 0, -114.2f }; gDir.checkpointYaw = PI / 2;
  }
}

// ---------------------------------------------------------------- interact
// SH2-style lore: each note is a piece of who you are and why you're here
struct LoreNote { Vector3 pos; const char* text; };
static const LoreNote gLore[4] = {
  { { pathX(150) + 1.2f, 0.1f, 150 },
    "My head is bleeding. The car is back\nthere, wrapped around a tree.\n\nI was driving to find her. I think.\nWhy can't I remember her face?" },
  { { gWorld.fountain.x - 2.4f, 0.9f, gWorld.fountain.z - 1.4f },
    "I keep finding notes in my own hand.\nI don't remember writing them.\n\n'Follow the path. Reach the house.\nDon't trust the light.'  -- me?" },
  { { gWorld.cabin.x - 1.6f, 0.86f, gWorld.cabin.z + 0.9f },
    "If you're reading this, you've been\nhere before. We always come back.\n\nThe white house keeps what it takes.\nShe is still inside. So are you." },
  { { -3.4f, 0.5f, -107.8f },
    "Her name was on the tip of my tongue\nwhen the wheel slipped.\n\nThe crash took more than my memory.\nI came here to give the rest back." },
};

struct Interactable { Vector3 pos; float r; std::string prompt; bool needLight; int id; };
static std::vector<Interactable> CurrentInteractables() {
  std::vector<Interactable> L;
  Director& D = gDir;
  if (D.phase >= 2 && !D.fWaterKey)
    L.push_back({ gWorld.waterKeyP, 1.6f, gPhone.lightOn ? "[E] TAKE THE IRON KEY" : "", true, 1 });
  if (D.phase >= 2 && !D.fCabinOpen)
    L.push_back({ { gWorld.cabin.x - 2.6f, 1.2f, gWorld.cabin.z + 0.85f }, 1.7f, "[E] KEYPAD", false, 2 });
  if (D.fCabinOpen && gPhone.battery < 99 && !gPhone.charging)
    L.push_back({ { gWorld.cabin.x + 1.2f, 0.9f, gWorld.cabin.z - 0.8f }, 1.6f, "[E] CHARGE PHONE", false, 3 });
  if (!D.fWellBattery && D.phase >= 2)
    L.push_back({ { gWorld.wellBattery.x, 0.1f, gWorld.wellBattery.z }, 1.3f, "[E] TAKE BATTERY", false, 4 });
  if (D.fInHouse && !D.fKitchenKey)
    L.push_back({ gWorld.kitchenDrawer, 1.4f, D.fDrawerOpen ? "[E] TAKE KEY" : "[E] OPEN DRAWER", false, 5 });
  if (D.fInHouse && !D.fBedroomOpen)
    L.push_back({ gWorld.bedroomDoorP, 1.6f, D.fKitchenKey ? "[E] UNLOCK BEDROOM" : "LOCKED - A SMALL KEYHOLE", false, 6 });
  if (D.fInHouse && !D.fBasementOpen)
    L.push_back({ gWorld.basementDoorP, 1.6f, (D.fWaterKey || D.fMirrorSolved) ? "[E] UNLOCK BASEMENT" : "LOCKED - THE KEYHOLE IS WATER-WORN", false, 7 });
  if (D.fBasementOnce && !D.fBaseBattery)
    L.push_back({ gWorld.basementBattery, 1.3f, "[E] TAKE BATTERY", false, 8 });
  if (D.fBasementOnce && !D.fHiddenOpen && gPlayer.groundY < -1) {
    bool facing = gPhone.lightOn && PlayerForward().x < -0.8f;
    L.push_back({ gWorld.falseWallP, 2.2f, facing ? "[E] THE WALL SOUNDS HOLLOW" : "", false, 9 });
  }
  if (D.fHiddenOpen && !D.fTruth)
    L.push_back({ gWorld.oldPhoneP, 1.5f, "[E] EXAMINE THE OTHER PHONE", false, 10 });
  if (D.fTruth && !D.fNoteRead)
    L.push_back({ gWorld.noteP, 1.5f, "[E] READ THE NOTE", false, 11 });
  // lore notes last — never override a real interaction
  for (int i = 0; i < 4; i++)
    if (!D.loreRead[i]) L.push_back({ gLore[i].pos, 1.5f, "[E] READ", false, 30 + i });
  return L;
}

std::string DirectorPrompt() {
  if (!gDir.readingNote.empty()) return "[E] CLOSE";
  if (gDir.fNotePending) return "[E] CLOSE";
  if (gDir.fNoteRead && !gDir.ended) return "[E] TRUST THE PHONE      [Q] REFUSE";
  Vector3 p = gPlayer.pos;
  for (auto& it : CurrentInteractables()) {
    float d = Vector2Distance({ it.pos.x, it.pos.z }, { p.x, p.z });
    float dy = fabsf(it.pos.y - (p.y + 1.0f));
    if (d < it.r && dy < 2.4f && !it.prompt.empty()) return it.prompt;
  }
  return "";
}

static void Ending(int which);

void DirectorInteract() {
  Director& D = gDir;
  if (D.ended) return;
  if (!D.readingNote.empty()) { D.readingNote.clear(); return; }
  if (D.fNotePending) { D.fNotePending = false; D.fNoteRead = true; gPhone.objective = "DECIDE"; return; }
  if (D.fNoteRead) { Ending(1); return; }
  Vector3 p = gPlayer.pos;
  for (auto& it : CurrentInteractables()) {
    float d = Vector2Distance({ it.pos.x, it.pos.z }, { p.x, p.z });
    float dy = fabsf(it.pos.y - (p.y + 1.0f));
    if (d >= it.r || dy >= 2.4f) continue;
    if (it.needLight && !gPhone.lightOn) continue;
    if (it.id >= 30 && it.id < 34) {
      int n = it.id - 30;
      D.loreRead[n] = true;
      D.readingNote = gLore[n].text;
      SfxPickup();
      return;
    }
    switch (it.id) {
      case 1:
        D.fWaterKey = true;
        gWorld.items[gWorld.waterKeyItem].tint.a = 0;
        InvGive(IT_WATERKEY);
        gPhone.message = "WATER KEY"; gPhone.messageT = 3;
        D.stress += 15; D.glitch += 0.7f;
        for (auto& e : gEntities)
          if (!e.frozen && !e.removed && e.lethal && e.kind == EKind::Shadow && e.state == EState::Idle) { e.lastSeen = p; e.state = EState::Alerted; e.stateT = 0; }
        return;
      case 2: D.codeBuffer = ""; return;
      case 3:
        gPhone.charging = true; gPhone.chargeT = 0;
        SfxBeep(false);
        gPhone.message = "CHARGING"; gPhone.messageT = 2;
        Sub("Hold still while it charges.", 4);
        return;
      case 4: D.fWellBattery = true; gWorld.items[gWorld.wellBatteryItem].tint.a = 0; InvGive(IT_BATTERY); return;
      case 5:
        if (!D.fDrawerOpen) { D.fDrawerOpen = true; OpenDrawer(); SfxBranch(0.2f); }
        else { D.fKitchenKey = true; InvGive(IT_KITCHENKEY); gPhone.message = "KITCHEN KEY"; gPhone.messageT = 3; gPhone.objective = "THE BEDROOM UPSTAIRS"; }
        return;
      case 6:
        if (!D.fKitchenKey) { SfxBeep(true); return; }
        D.fBedroomOpen = true; OpenBedroomDoor(); SfxUnlock();
        gPhone.objective = "THE MIRROR REMEMBERS";
        SpawnCrawler({ HX - 1.5f, 0, HZ - 3.6f }); // it was under the bed
        gEntities.back().pos.y = 3.1f; gEntities.back().home.y = 3.1f;
        SfxEntityCry({ HX - 1.5f, 3.1f, HZ - 3.6f }, 3);
        return;
      case 7:
        if (!(D.fWaterKey || D.fMirrorSolved)) { SfxBeep(true); return; }
        D.fBasementOpen = true; OpenBasementDoor(); SfxUnlock();
        gPhone.objective = "GO DOWN";
        return;
      case 8: D.fBaseBattery = true; gWorld.items[gWorld.baseBatteryItem].tint.a = 0; InvGive(IT_BATTERY); return;
      case 9:
        if (!(gPhone.lightOn && PlayerForward().x < -0.8f)) return;
        D.fHiddenOpen = true; OpenFalseWall(); SfxUnlock(); SfxSting(0.8f);
        Sub("The wall sinks into the floor.", 4);
        return;
      case 10:
        D.fTruth = true;
        D.flashWhite = 2.5f;
        gPhone.battery = 100; gPhone.lightOn = true;
        D.stress = 0;
        SfxSting(0.6f);
        gPhone.message = "YOU ARE THE RECORDING"; gPhone.messageT = 6;
        gPhone.objective = "READ THE NOTE";
        return;
      case 11:
        D.fNotePending = true;
        InvGive(IT_NOTE);
        gInv.inspecting = IT_NOTE;
        gPhone.objective = "";
        return;
    }
  }
}

void DirectorKey(int key) {
  Director& D = gDir;
  if (D.ended) return;
  if (D.codeBuffer[0] != '\xFF') {
    if (key == KEY_ESCAPE) { D.codeBuffer = "\xFF"; return; }
    if (key >= KEY_ZERO && key <= KEY_NINE) {
      D.codeBuffer += (char)('0' + (key - KEY_ZERO));
      SfxBeep(false);
      if (D.codeBuffer.size() >= 4) {
        if (D.codeBuffer == "2741") {
          D.fCabinOpen = true; OpenCabinDoor(); SfxUnlock();
          Sub("The cabin accepts you.", 4);
        } else { SfxBeep(true); Sub("Nothing.", 2); }
        D.codeBuffer = "\xFF";
      }
    }
    return;
  }
  if (D.fNoteRead && !D.fNotePending && key == KEY_Q) Ending(2);
}

// ---------------------------------------------------------------- endings
static void Ending(int which) {
  if (gDir.ended) return;
  gDir.ended = true;
  gDir.ending = which;
  gDir.endingT = 0;
  gPlayer.enabled = false;
  AudioSetWind(0);
  if (which == 1) { gPhone.message = "TRUST ME"; gPhone.messageT = 30; }
  else { gPhone.battery = 0; gPhone.lightOn = false; SfxScare(); }
}

// ---------------------------------------------------------------- update
void DirectorUpdate(float dt, float time) {
  Director& D = gDir;
  D.subT = fmaxf(0, D.subT - dt);
  D.bigT = fmaxf(0, D.bigT - dt);

  if (D.ended) {
    D.endingT += dt;
    if (D.ending == 1) {
      D.flashWhite = fminf(1.2f, D.endingT * 0.25f);
      if (D.endingT > 6.5f) D.credits = true;
    } else {
      D.flashBlack = fminf(1.0f, D.endingT * 0.5f);
      if (D.endingT > 3.0f && D.endingT - dt <= 3.0f) { Big("THEY SEE YOU NOW", 5); SfxSting(1.8f); }
      if (D.endingT > 8.5f) D.credits = true;
    }
    return;
  }

  D.gameTime += dt;
  D.catchCooldown = fmaxf(0, D.catchCooldown - dt);
  D.graceT = fmaxf(0, D.graceT - dt);

  if (D.phase == 1) Phase1Script(time);
  else if (D.phase == 2) Phase2Script();
  if (D.phase >= 3) Phase3Script();

  // scripted e0 tween
  if (gE0moveT >= 0 && gE0 >= 0) {
    gE0moveT += dt / 2.2f;
    Entity& e0 = gEntities[gE0];
    float k = fminf(1.0f, gE0moveT);
    e0.pos = Vector3Lerp(gE0from, gE0to, k);
    e0.walkPhase += dt * 9;
    e0.speed = 4;
    if (k >= 1) { gE0moveT = -1; if (fired[4] || !fired[3]) e0.visible = false; }
  }

  // watchers: distant pale figures, phase 2+
  if (D.phase >= 2 && gPlayer.groundY > -1) {
    gWatcherT -= dt;
    if (gWatcherT <= 0) {
      gWatcherT = 45 + frand() * 40;
      Vector3 f = PlayerForward();
      float ang = atan2f(f.x, -f.z) + frand2() * 0.6f;
      Vector3 at = { gPlayer.pos.x + sinf(ang) * 15, 0, gPlayer.pos.z - cosf(ang) * 15 };
      SpawnWatcher(at);
      SfxWhisper();
    }
  }

  // entities
  for (auto& e : gEntities) EntityUpdate(e, dt, time);

  // stress model
  float nearest = 1e9f; bool pursuit = false; bool samaraOut = false;
  for (auto& e : gEntities) {
    if (e.frozen || e.removed || !e.visible) continue;
    float d = Vector2Distance({ e.pos.x, e.pos.z }, { gPlayer.pos.x, gPlayer.pos.z });
    nearest = fminf(nearest, d);
    if ((e.state == EState::Pursuing || e.state == EState::Hunting || e.state == EState::Emerging) && d < 30) pursuit = true;
    if (e.kind == EKind::Samara && e.state != EState::Dormant) samaraOut = true;
  }
  if (pursuit) D.stress += (samaraOut ? 12.0f : 9.0f) * dt;
  else if (nearest < 9) D.stress += 4.5f * dt;
  else if (nearest < 16) D.stress += 1.5f * dt;
  else {
    D.stress -= (2.0f + (gPhone.lightOn ? 0.8f : 0)) * dt;
    D.stress += 0.25f * dt; // paranoia floor
  }
  if (gPhone.battery < 20) D.stress += 1.2f * dt;
  bool inCabin = D.fCabinOpen && fabsf(gPlayer.pos.x - gWorld.cabin.x) < 2.4f && fabsf(gPlayer.pos.z - gWorld.cabin.z) < 1.9f;
  if (inCabin) D.stress -= 5 * dt;
  if (gPlayer.groundY < -1) D.stress += 0.8f * dt;
  D.stress = Clamp(D.stress, 0, 100);
  D.entitiesNear = pursuit && nearest < 14;

  // sign code reveal (precise light angle)
  {
    float target = 0;
    if (gPhone.lightOn && D.phase >= 2) {
      Vector3 p = gPlayer.pos;
      float d = Vector2Distance({ gWorld.sign.x, gWorld.sign.z }, { p.x, p.z });
      if (d > 1.2f && d < 5.5f) {
        Vector3 to = Vector3Normalize({ gWorld.sign.x - p.x, 0, gWorld.sign.z - p.z });
        float a = Vector3DotProduct(PlayerForward(), to);
        if (a > 0.975f && p.z > gWorld.sign.z + 1.0f) target = fminf(1.0f, (a - 0.975f) / 0.02f);
      }
    }
    gWorld.signCodeAlpha += (target - gWorld.signCodeAlpha) * fminf(1, dt * 5);
    gWorld.items[gWorld.signCodeItem].tint.a = (unsigned char)(gWorld.signCodeAlpha * 255);
    if (gWorld.signCodeAlpha > 0.6f && !D.fCodeSeen) {
      D.fCodeSeen = true;
      SfxBeep(false);
      Sub("2 - 7 - 4 - 1", 6);
    }
  }

  // mirror puzzle
  if (D.fBedroomOpen && !D.fMirrorSolved && gPlayer.groundY > 2.5f) {
    Vector3 p = gPlayer.pos;
    float dMirror = Vector2Distance({ gWorld.mirrorPos.x, gWorld.mirrorPos.z }, { p.x, p.z });
    float vis = dMirror < 4 ? fminf(0.85f, (4 - dMirror) / 3) : 0;
    SetMirrorFigure(vis);
    if (vis > 0.4f && !D.fMirrorScare) {
      D.fMirrorScare = true;
      SfxSting(1.2f);
      D.stress += 22; D.glitch += 1.0f;
      Sub("That is not your reflection.", 4);
    }
    float dSpot = Vector2Distance({ gWorld.mirrorSpot.x, gWorld.mirrorSpot.z }, { p.x, p.z });
    if (dSpot < 0.9f && PlayerForward().x > 0.94f) {
      D.mirrorHold += dt;
      if (D.mirrorHold > 2.2f) {
        D.fMirrorSolved = true;
        SfxUnlock(); SfxSting(0.7f);
        Sub("Something unlocked below.", 5);
        gPhone.objective = "GO DOWN";
        SetMirrorFigure(0);
      }
    } else D.mirrorHold = 0;
  }

  // ambient one-shots
  if (D.whispers && gPlayer.groundY > -1) {
    gWhisperT -= dt;
    if (gWhisperT <= 0) { gWhisperT = 7 + frand() * 14; SfxWhisper(); }
  }
  bool nearFountain = Vector2Distance({ gWorld.fountain.x, gWorld.fountain.z }, { gPlayer.pos.x, gPlayer.pos.z }) < 12;
  if (nearFountain || gPlayer.groundY < -1) {
    gDripT -= dt;
    if (gDripT <= 0) { gDripT = 4 + frand() * 3; SfxDrip(); }
  }
  gBreathT -= dt;
  if (gBreathT <= 0) { gBreathT = 4.2f - (D.stress / 100.0f) * 2.6f; SfxBreathPlayer(D.stress / 100.0f); }
  if (D.stress > 40) {
    gHeartT -= dt;
    if (gHeartT <= 0) { gHeartT = 1.1f - (D.stress / 100.0f) * 0.55f; SfxHeartThump((D.stress - 40) / 60.0f); }
  }
  // trust-me intro message
  if (D.gameTime > 3.5f && FIRE(8)) { gPhone.message = "TRUST ME"; gPhone.messageT = 5; SfxBeep(false); }

  // environment for audio
  int env = 0;
  if (gPlayer.groundY < -1) env = 2;
  else if (D.fInHouse && fabsf(gPlayer.pos.x) < 6.2f && fabsf(gPlayer.pos.z + 110) < 5.2f) env = 1;
  else if (inCabin) env = 3;
  AudioSetEnv(env);
  AudioSetStress(D.stress);

  // ---- music director: chase > tension > silence
  bool indoorHouse = (env == 1 || env == 2);
  if (pursuit) MusicSetMode(MUS_CHASE);
  else if (samaraOut || D.stress > 55 || (nearest < 12 && !inCabin)) MusicSetMode(MUS_TENSION);
  else MusicSetMode(MUS_OFF);
  MusicTick(dt);

  // ---- rain & thunder: heavy outdoors, muffled inside, gone in the basement
  float rainTarget = (env == 2) ? 0.05f : (env == 1) ? 0.32f : (env == 3) ? 0.4f : 1.0f;
  AudioSetRain(rainTarget);
  D.rainVisual += ((env == 0 ? 1.0f : 0.0f) - D.rainVisual) * fminf(1, dt * 2);
  if (env != 2) {
    gThunderT -= dt;
    if (gThunderT <= 0) {
      gThunderT = 14 + frand() * 26;
      SfxThunder();
      D.flashWhite = fmaxf(D.flashWhite, 0.18f); // distant lightning
      D.stress += 2;
    }
  }
}
