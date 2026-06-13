// PARANOIA v2 — entities.
// Shadow stalkers: pure black, proportions wrong, they WALK now — heavy,
// deliberate, arms too long. Samara: pale, wet, crawls out of the well,
// moves in lurches. Watchers: tall pale things that only ever stand still.
// Balance: every pursuer is slower than your sprint. Stamina is the duel.
#pragma once
#include "game.h"

std::vector<Entity> gEntities;

static Mesh mshPart; // shared unit cube for body parts
static Material gMatEnt;
static bool gEntInit = false;
static Texture2D texGhostFace, texGhostFaceGrin, texHairStrand;

static void BuildEntityTextures() {
  { // hollow spectral face: pale, sunken black eyes, faint open mouth
    Image im = GenImageColor(64, 80, BLANK);
    DrawEllipseOnImage(&im, 32, 40, 24, 34, Color{ 196, 198, 192, 235 }, true);  // face
    DrawEllipseOnImage(&im, 32, 40, 24, 34, Color{ 150, 150, 150, 60 }, false);
    DrawEllipseOnImage(&im, 22, 34, 7, 9, Color{ 4, 4, 6, 255 }, true);          // L eye socket
    DrawEllipseOnImage(&im, 42, 34, 7, 9, Color{ 4, 4, 6, 255 }, true);          // R eye socket
    // dark trails under eyes
    ImageDrawRectangle(&im, 20, 42, 4, 16, Color{ 30, 25, 28, 150 });
    ImageDrawRectangle(&im, 40, 42, 4, 16, Color{ 30, 25, 28, 150 });
    DrawEllipseOnImage(&im, 32, 62, 5, 8, Color{ 10, 6, 8, 220 }, true);         // gaping mouth
    texGhostFace = LoadTextureFromImage(im); UnloadImage(im);
  }
  { // the grin: under-bed crawler — wide teeth, black eyes
    Image im = GenImageColor(64, 64, BLANK);
    DrawEllipseOnImage(&im, 32, 30, 28, 28, Color{ 205, 205, 198, 240 }, true);
    DrawEllipseOnImage(&im, 21, 24, 6, 7, Color{ 2, 2, 4, 255 }, true);
    DrawEllipseOnImage(&im, 43, 24, 6, 7, Color{ 2, 2, 4, 255 }, true);
    DrawEllipseOnImage(&im, 32, 44, 18, 9, Color{ 6, 4, 6, 255 }, true);          // mouth
    for (int i = -3; i <= 3; i++) ImageDrawRectangle(&im, 32 + i * 5 - 1, 38, 3, 14, Color{ 220, 218, 208, 255 }); // teeth
    texGhostFaceGrin = LoadTextureFromImage(im); UnloadImage(im);
  }
  { // a single hair strand strip (vertical), dark, for hair curtains
    Image im = GenImageColor(8, 64, BLANK);
    for (int y = 0; y < 64; y++) { int a = 200 - y; ImageDrawRectangle(&im, 1 + (y % 3), y, 4, 1, Color{ 6, 5, 8, (unsigned char)Clamp(a, 40, 255) }); }
    texHairStrand = LoadTextureFromImage(im); UnloadImage(im);
  }
}

bool EntityLit(const Entity& e) {
  if (!gPhone.lightOn || gPhone.battery <= 0 || gPhone.flickerOff) return false;
  Vector3 to = Vector3Subtract(e.pos, gPlayer.pos);
  to.y = 0;
  float d = Vector3Length(to);
  if (d > 19 || d < 0.01f) return false;
  to = Vector3Scale(to, 1.0f / d);
  Vector3 f = PlayerForward();
  return Vector3DotProduct(f, to) > 0.82f;
}

void SpawnShadow(Vector3 home, std::vector<Vector3> patrol, float aggression, bool lethal) {
  Entity e;
  e.kind = EKind::Shadow;
  e.home = home; e.pos = home;
  e.patrol = patrol;
  e.aggression = aggression;
  e.lethal = lethal;
  e.audioVoice = AudioEntityVoice();
  gEntities.push_back(e);
}

void SpawnSamara() {
  Entity e;
  e.kind = EKind::Samara;
  e.home = gWorld.well; e.home.y = -2.0f;
  e.pos = e.home;
  e.state = EState::Dormant;
  e.visible = false;
  e.lethal = true;
  e.audioVoice = AudioEntityVoice();
  gEntities.push_back(e);
}

void SpawnWatcher(Vector3 at) {
  Entity e;
  e.kind = EKind::Watcher;
  e.home = at; e.pos = at;
  e.state = EState::Idle;
  e.lethal = false;
  gEntities.push_back(e);
}

void SpawnCrawler(Vector3 home) {
  Entity e;
  e.kind = EKind::Crawler;
  e.home = home; e.pos = home;
  e.state = EState::Idle;
  e.aggression = 1.25f;
  e.lethal = true;
  e.audioVoice = AudioEntityVoice();
  gEntities.push_back(e);
}

static void AlertOthers(Entity& self) {
  for (auto& e : gEntities) {
    if (&e == &self || e.frozen || e.removed || e.kind != EKind::Shadow) continue;
    if (Vector3Distance(e.pos, self.pos) < 45 && (e.state == EState::Idle || e.state == EState::Alerted)) {
      e.lastSeen = gPlayer.pos;
      e.state = EState::Pursuing; e.stateT = 0;
    }
  }
}

void EntityUpdate(Entity& e, float dt, float time) {
  if (e.frozen || e.removed) return;
  e.stateT += dt;
  e.recoilT = fmaxf(0, e.recoilT - dt);
  Vector3 pp = gPlayer.pos;
  float dist = Vector2Distance({ e.pos.x, e.pos.z }, { pp.x, pp.z });
  bool lit = EntityLit(e);
  bool playerBelow = gPlayer.groundY < -1.0f;
  bool playerAbove = gPlayer.groundY > 2.0f;

  // ---------------- WATCHER: it only watches. light kills the image.
  if (e.kind == EKind::Watcher) {
    e.facing = atan2f(pp.x - e.pos.x, pp.z - e.pos.z);
    if ((lit && dist < 16) || dist < 3.5f) {
      e.removed = true;
      gDir.glitch += 0.7f;
      gDir.stress = fminf(100, gDir.stress + 9);
      SfxBranch(frand2());
    }
    return;
  }

  // ---------------- SAMARA: the well remembers. siete dias.
  if (e.kind == EKind::Samara) {
    switch (e.state) {
      case EState::Dormant:
        e.visible = false;
        if (!gDir.fSamaraDone && Vector2Distance({ pp.x, pp.z }, { gWorld.well.x, gWorld.well.z }) < 5.2f && !playerBelow) {
          e.state = EState::Emerging; e.stateT = 0;
          e.visible = true;
          e.pos = { gWorld.well.x, -1.9f, gWorld.well.z };
          SfxSamaraGroan(e.pos);
          SfxSting(1.3f);
          gDir.stress = fminf(100, gDir.stress + 25);
        }
        break;
      case EState::Emerging: {
        // 3.6s climb: rise inside the ring, tip over the rim
        float t = e.stateT / 3.6f;
        if (t < 0.62f) {
          e.pos.y = -1.9f + (t / 0.62f) * 2.45f; // head over the rim
        } else {
          float k = (t - 0.62f) / 0.38f;
          Vector3 out = Vector3Normalize({ pp.x - gWorld.well.x, 0, pp.z - gWorld.well.z });
          e.pos.x = gWorld.well.x + out.x * k * 2.4f;
          e.pos.z = gWorld.well.z + out.z * k * 2.4f;
          e.pos.y = 0.55f * (1.0f - k);
        }
        e.facing = atan2f(pp.x - e.pos.x, pp.z - e.pos.z);
        if (frand() < dt * 3) e.twitchT = 0.1f;
        if (t >= 1) { e.state = EState::Pursuing; e.stateT = 0; e.pos.y = 0; }
        break;
      }
      case EState::Pursuing: {
        // lurching crawl: bursts and dead stops — escapable, unforgettable
        float burst = fmodf(e.stateT, 1.6f);
        e.speed = (burst < 1.0f) ? 4.9f : 0.4f;
        if (burst < 0.06f && e.stateT > 0.5f) SfxSamaraGroan(e.pos);
        Vector3 dir = { pp.x - e.pos.x, 0, pp.z - e.pos.z };
        float d = Vector3Length(dir);
        if (d > 0.05f) {
          dir = Vector3Scale(dir, 1.0f / d);
          e.pos.x += dir.x * e.speed * dt;
          e.pos.z += dir.z * e.speed * dt;
          e.facing = atan2f(dir.x, dir.z);
        }
        e.walkPhase += e.speed * dt * 2.2f;
        // she gives up far from her well, or if you go where she cannot
        float fromWell = Vector2Distance({ e.pos.x, e.pos.z }, { gWorld.well.x, gWorld.well.z });
        if (fromWell > 55 || playerBelow || dist > 30) { e.state = EState::Retreating; e.stateT = 0; }
        if (e.lethal && dist < 0.95f && !playerBelow) DirectorCatch(e);
        break;
      }
      case EState::Retreating: {
        Vector3 dir = { gWorld.well.x - e.pos.x, 0, gWorld.well.z - e.pos.z };
        float d = Vector3Length(dir);
        if (d > 0.6f) {
          dir = Vector3Scale(dir, 1.0f / d);
          e.pos.x += dir.x * 3.2f * dt;
          e.pos.z += dir.z * 3.2f * dt;
          e.facing = atan2f(dir.x, dir.z);
          e.walkPhase += 3.2f * dt * 2.2f;
        } else {
          e.pos.y -= dt * 1.2f; // sinks back down
          if (e.pos.y < -1.9f) { e.state = EState::Dormant; e.visible = false; }
        }
        break;
      }
      default: break;
    }
    if (e.audioVoice >= 0) AudioEntityUpdate(e.audioVoice, e.pos, e.state, e.kind);
    return;
  }

  // ---------------- SHADOW: the original watchers
  if (lit && dist < 7 && e.recoilT <= 0) {
    e.recoilT = 2.2f;
    e.state = EState::Retreating; e.stateT = 0;
    gDir.stress = fminf(100, gDir.stress + 6);
    gDir.glitch += 0.4f;
  } else if (!playerBelow && e.state != EState::Retreating && gDir.graceT <= 0) {
    if (lit && dist >= 7) {
      e.lastSeen = pp; e.lastSeenTime = time;
      if (e.state == EState::Idle || e.state == EState::Alerted) {
        if (e.stateT > 2 && e.aggression > 0.8f) { e.state = EState::Pursuing; e.stateT = 0; AlertOthers(e); }
        else if (e.state == EState::Idle) { e.state = EState::Alerted; e.stateT = 0; }
      }
    }
    if (dist < 5 && !lit && !playerAbove) {
      e.lastSeen = pp; e.lastSeenTime = time;
      if (e.state != EState::Pursuing && e.state != EState::Hunting) { e.state = EState::Pursuing; e.stateT = 0; AlertOthers(e); }
    }
    if (gPlayer.sprinting && gPlayer.moving && dist < 15 && e.state == EState::Idle) {
      e.lastSeen = pp;
      e.state = EState::Alerted; e.stateT = 0;
    }
  }
  if ((playerBelow || playerAbove) && (e.state == EState::Pursuing || e.state == EState::Hunting)) {
    // they do not go below. above, they wait at the foot of the stairs.
    if (playerBelow) { e.state = EState::Retreating; e.stateT = 0; }
  }

  float speed = 1.4f;
  Vector3 target = e.pos;
  bool hasTarget = false;

  switch (e.state) {
    case EState::Idle:
      if (!e.patrol.empty()) {
        target = e.patrol[e.patrolIdx]; hasTarget = true;
        if (Vector2Distance({ e.pos.x, e.pos.z }, { target.x, target.z }) < 1.2f) {
          e.patrolIdx = (e.patrolIdx + 1) % (int)e.patrol.size();
          if (frand() < 0.3f) e.pauseT = 1.5f + frand() * 2.5f; // stops to listen
        }
      }
      if (e.pauseT > 0) { e.pauseT -= dt; hasTarget = false; }
      break;
    case EState::Alerted:
      speed = 2.1f;
      target = e.lastSeen; hasTarget = true;
      if (e.stateT > 12 || (Vector2Distance({ e.pos.x, e.pos.z }, { target.x, target.z }) < 1.5f && e.stateT > 4)) { e.state = EState::Idle; e.stateT = 0; }
      if (dist < 11 && time - e.lastSeenTime < 1) { e.state = EState::Pursuing; e.stateT = 0; AlertOthers(e); }
      break;
    case EState::Pursuing: {
      speed = 4.4f * e.aggression;
      Vector3 lead = gPlayer.moving ? Vector3Scale(PlayerForward(), 1.4f) : Vector3{ 0, 0, 0 };
      target = Vector3Add(pp, lead); hasTarget = true;
      e.lastSeen = pp;
      int pursuers = 0;
      for (auto& o : gEntities) if (!o.frozen && !o.removed && o.kind == EKind::Shadow && (o.state == EState::Pursuing || o.state == EState::Hunting)) pursuers++;
      if (pursuers >= 2) {
        int role = 0;
        for (auto& o : gEntities) {
          if (o.frozen || o.removed || o.kind != EKind::Shadow) continue;
          if (o.state == EState::Pursuing || o.state == EState::Hunting) { o.state = EState::Hunting; o.huntRole = role++; }
        }
      }
      if (lit && dist < 10 && e.stateT > 6 && frand() < dt * 0.25f) { e.state = EState::Alerted; e.stateT = 0; } // blinded into doubt
      if (dist > 24) e.lostT += dt; else e.lostT = 0;
      if (e.lostT > 6) { e.lostT = 0; e.state = EState::Idle; e.stateT = 0; } // fair: they give up
      break;
    }
    case EState::Hunting: {
      speed = 4.9f * e.aggression;
      if (e.huntRole == 0) target = pp;
      else target = Vector3Add(pp, Vector3Scale(PlayerForward(), 7.0f)); // cut the escape
      hasTarget = true;
      int pursuers = 0;
      for (auto& o : gEntities) if (!o.frozen && !o.removed && o.kind == EKind::Shadow && (o.state == EState::Pursuing || o.state == EState::Hunting)) pursuers++;
      if (pursuers < 2) { e.state = EState::Pursuing; e.stateT = 0; }
      if (dist > 26) { e.state = EState::Alerted; e.stateT = 0; }
      break;
    }
    case EState::Retreating:
      speed = 3.0f;
      target = e.home; hasTarget = true;
      if (Vector2Distance({ e.pos.x, e.pos.z }, { e.home.x, e.home.z }) < 2 && e.stateT > 5) { e.state = EState::Idle; e.stateT = 0; }
      break;
    default: break;
  }

  e.speed = 0;
  if (hasTarget) {
    Vector3 dir = { target.x - e.pos.x, 0, target.z - e.pos.z };
    float d = Vector3Length(dir);
    if (d > 0.05f) {
      dir = Vector3Scale(dir, 1.0f / d);
      float nx = e.pos.x + dir.x * speed * dt;
      float nz = e.pos.z + dir.z * speed * dt;
      for (auto& c : gWorld.circles) { // slide around trees
        float dx = nx - c.x, dz = nz - c.z;
        float d2 = dx * dx + dz * dz, r = c.r + 0.3f;
        if (d2 < r * r && d2 > 1e-6f) {
          float dd = sqrtf(d2);
          nx = c.x + dx / dd * r; nz = c.z + dz / dd * r;
        }
      }
      e.pos.x = nx; e.pos.z = nz;
      // crawler follows floors (stairs/upstairs); shadows stay on the ground plane
      e.pos.y = (e.kind == EKind::Crawler) ? ResolveGround(nx, nz, e.pos.y) : 0;
      e.facing = atan2f(dir.x, dir.z);
      e.speed = speed;
      e.walkPhase += speed * dt * 1.45f; // stride drives the legs
    }
  }
  if (frand() < dt * 1.2f) e.twitchT = 0.06f + frand() * 0.08f;
  e.twitchT = fmaxf(0, e.twitchT - dt);

  if (e.lethal && dist < 0.95f && fabsf(e.pos.y - gPlayer.groundY) < 1.5f && !playerBelow &&
      (e.kind == EKind::Crawler || !playerAbove) &&
      (e.state == EState::Pursuing || e.state == EState::Hunting)) {
    DirectorCatch(e);
  }
  // unique cries: agitated when hunting, rare distant sob when idle
  e.cryT -= dt;
  if (e.cryT <= 0) {
    bool agitated = (e.state == EState::Pursuing || e.state == EState::Hunting || e.state == EState::Alerted);
    if (agitated && dist < 28) { SfxEntityCry(e.pos, e.kind == EKind::Crawler ? 3 : 0); e.cryT = 2.6f + frand() * 2.5f; }
    else e.cryT = 9 + frand() * 10;
  }
  if (e.audioVoice >= 0) AudioEntityUpdate(e.audioVoice, e.pos, e.state, e.kind);
}

// ---------------------------------------------------------------- drawing
static void DrawPart(Vector3 size, Vector3 local, Vector3 rot, Matrix world, Texture2D tex, Color tint, float spec) {
  Matrix m = MatrixMultiply(MatrixMultiply(MatrixScale(size.x, size.y, size.z), MatrixRotateXYZ(rot)), MatrixTranslate(local.x, local.y, local.z));
  m = MatrixMultiply(m, world);
  SetSpec(0, spec);
  gMatEnt.maps[MATERIAL_MAP_ALBEDO].texture = tex;
  gMatEnt.maps[MATERIAL_MAP_ALBEDO].color = tint;
  DrawMesh(mshPart, gMatEnt, m);
}

// billboard a face/grin texture at a world point, facing the camera
static void DrawFace(Texture2D tex, Vector3 at, float size, float alpha) {
  Rectangle src = { 0, 0, (float)tex.width, (float)tex.height };
  DrawBillboardPro(gPlayer.cam, tex, src, at, { 0, 1, 0 }, { size, size * tex.height / tex.width },
    { size / 2, size * tex.height / tex.width / 2 }, 0, Fade(WHITE, alpha));
}
// hair curtain: dark strands hanging from a head point, facing camera
static void DrawHair(Vector3 head, float w, float len, float alpha) {
  Rectangle src = { 0, 0, (float)texHairStrand.width, (float)texHairStrand.height };
  for (int i = -2; i <= 2; i++) {
    Vector3 p = head; p.x += i * w * 0.18f;
    DrawBillboardPro(gPlayer.cam, texHairStrand, src, { p.x, p.y - len / 2, p.z }, { 0, 1, 0 },
      { w * 0.5f, len }, { w * 0.25f, len / 2 }, 0, Fade(WHITE, alpha));
  }
}

void EntityDraw(Entity& e, float time) {
  if (!gEntInit) { mshPart = GenMeshCube(1, 1, 1); gMatEnt = LoadMaterialDefault(); BuildEntityTextures(); gEntInit = true; }
  gMatEnt.shader = gGfx.world;

  float tw = (e.twitchT > 0) ? 1.0f : 0.0f;
  float jx = tw * frand2() * 0.06f, jz = tw * frand2() * 0.06f;
  Matrix W = MatrixMultiply(MatrixRotateY(e.facing), MatrixTranslate(e.pos.x + jx, e.pos.y, e.pos.z + jz));
  bool agit = (e.state == EState::Pursuing || e.state == EState::Hunting);

  if (e.kind == EKind::Shadow) {
    // spectral: near-black translucent body, ragged, with a pale face floating in it
    Color body = { 6, 6, 10, 225 };
    float swing = sinf(e.walkPhase) * Clamp(e.speed / 4.0f, 0.0f, 1.0f);
    float bob = fabsf(cosf(e.walkPhase)) * 0.06f * Clamp(e.speed / 2.0f, 0.0f, 1.0f);
    float headTilt = tw * frand2() * 0.5f + sinf(time * 0.3f) * 0.08f;
    float waver = sinf(time * 2.0f + e.pos.x) * 0.03f;
    BeginBlendMode(BLEND_ALPHA);
    DrawPart({ 0.13f, 1.0f, 0.13f }, { -0.12f, 0.5f + bob, 0 }, { swing * 0.55f, 0, 0 }, W, gGfx.white, body, 0);
    DrawPart({ 0.13f, 1.0f, 0.13f }, { 0.12f, 0.5f + bob, 0 }, { -swing * 0.55f, 0, 0 }, W, gGfx.white, body, 0);
    DrawPart({ 0.46f, 0.9f, 0.24f }, { 0, 1.4f + bob, 0 }, { 0.04f, 0, 0.03f + waver }, W, gGfx.white, body, 0);
    DrawPart({ 0.09f, 1.15f, 0.09f }, { -0.33f, 1.22f + bob, 0 }, { -swing * 0.6f, 0, 0.12f }, W, gGfx.white, body, 0); // long arms
    DrawPart({ 0.09f, 1.2f, 0.09f }, { 0.33f, 1.18f + bob, 0 }, { swing * 0.6f, 0, -0.12f }, W, gGfx.white, body, 0);
    DrawPart({ 0.20f, 0.26f, 0.21f }, { 0, 1.97f + bob, 0 }, { 0, headTilt, 0.07f }, W, gGfx.white, body, 0);
    EndBlendMode();
    // the face — only really visible when your light finds it
    Vector3 head = Vector3Transform(Vector3{ 0, 1.99f + bob, 0.10f }, W);
    DrawFace(texGhostFace, head, 0.30f, agit ? 0.92f : 0.7f);
    DrawHair(head, 0.34f, 0.5f, 0.85f);
    DrawCylinderEx({ e.pos.x, 0.015f, e.pos.z }, { e.pos.x, 0.02f, e.pos.z }, 0.65f, 0.65f, 10, Fade(BLACK, 0.5f));
  } else if (e.kind == EKind::Crawler) {
    // pale, on all fours, grinning — the under-bed thing. lunging gait.
    Color flesh = { 198, 196, 188, 240 };
    float lunge = sinf(e.walkPhase * 1.4f);
    Matrix C = MatrixMultiply(MatrixRotateX(1.35f), W);
    BeginBlendMode(BLEND_ALPHA);
    DrawPart({ 0.44f, 1.0f, 0.34f }, { 0, 0.55f, -0.1f }, { 0, 0, lunge * 0.1f }, C, texSkin, flesh, 0.08f);
    DrawPart({ 0.09f, 0.8f, 0.09f }, { -0.26f, 0.4f, 0.34f }, { lunge * 1.0f - 0.7f, 0, 0.2f }, W, texSkin, flesh, 0.06f);
    DrawPart({ 0.09f, 0.8f, 0.09f }, { 0.26f, 0.4f, 0.34f }, { -lunge * 1.0f - 0.7f, 0, -0.2f }, W, texSkin, flesh, 0.06f);
    DrawPart({ 0.10f, 0.78f, 0.10f }, { -0.17f, 0.38f, -0.7f }, { -lunge * 0.8f + 0.5f, 0, 0 }, W, texSkin, flesh, 0.05f);
    DrawPart({ 0.10f, 0.78f, 0.10f }, { 0.17f, 0.38f, -0.7f }, { lunge * 0.8f + 0.5f, 0, 0 }, W, texSkin, flesh, 0.05f);
    DrawPart({ 0.20f, 0.22f, 0.20f }, { 0, 0.62f, 0.5f }, { -1.0f, tw * frand2() * 0.7f, 0 }, W, texSkin, flesh, 0.07f);
    EndBlendMode();
    Vector3 head = Vector3Transform(Vector3{ 0, 0.62f, 0.62f }, W);
    DrawFace(texGhostFaceGrin, head, 0.32f, 0.95f);
    DrawHair(head, 0.40f, 0.34f, 0.9f);
  } else if (e.kind == EKind::Samara) {
    Color gown = { 222, 226, 228, 245 };
    Color skin = { 190, 196, 188, 245 };
    bool crawl = (e.state == EState::Pursuing || e.state == EState::Retreating);
    float lurch = sinf(e.walkPhase) * 0.6f;
    BeginBlendMode(BLEND_ALPHA);
    if (crawl) {
      Matrix C = MatrixMultiply(MatrixRotateX(1.25f), W);
      DrawPart({ 0.42f, 1.15f, 0.30f }, { 0, 0.75f, -0.15f }, { 0, 0, lurch * 0.10f }, C, texPale, gown, 0.07f);
      DrawPart({ 0.085f, 0.85f, 0.085f }, { -0.26f, 0.45f, 0.32f }, { lurch * 0.9f - 0.7f, 0, 0.2f }, W, texPale, skin, 0.05f);
      DrawPart({ 0.085f, 0.85f, 0.085f }, { 0.26f, 0.45f, 0.32f }, { -lurch * 0.9f - 0.7f, 0, -0.2f }, W, texPale, skin, 0.05f);
      DrawPart({ 0.10f, 0.8f, 0.10f }, { -0.16f, 0.4f, -0.78f }, { lurch * 0.7f + 0.5f, 0, 0 }, W, texPale, gown, 0.04f);
      DrawPart({ 0.10f, 0.8f, 0.10f }, { 0.16f, 0.4f, -0.78f }, { -lurch * 0.7f + 0.5f, 0, 0 }, W, texPale, gown, 0.04f);
      DrawPart({ 0.20f, 0.25f, 0.21f }, { 0, 0.78f, 0.55f }, { -0.9f, tw * frand2() * 0.8f, lurch * 0.2f }, W, texPale, skin, 0.06f);
      EndBlendMode();
      Vector3 head = Vector3Transform(Vector3{ 0, 0.80f, 0.66f }, W);
      DrawFace(texGhostFace, head, 0.26f, 0.85f);
      DrawHair(head, 0.46f, 0.55f, 0.95f); // long wet hair dragging forward
    } else {
      float k = (e.state == EState::Emerging) ? Clamp(e.stateT / 3.6f, 0.0f, 1.0f) : 1.0f;
      DrawPart({ 0.46f, 1.45f, 0.32f }, { 0, 0.85f, 0 }, { 0.06f, 0, sinf(time * 7) * 0.02f * k }, W, texPale, gown, 0.07f);
      DrawPart({ 0.08f, 0.95f, 0.08f }, { -0.28f, 1.05f, 0 }, { 0, 0, 0.16f }, W, texPale, skin, 0.05f);
      DrawPart({ 0.08f, 0.95f, 0.08f }, { 0.28f, 1.02f, 0 }, { 0, 0, -0.20f }, W, texPale, skin, 0.05f);
      DrawPart({ 0.21f, 0.26f, 0.22f }, { 0, 1.78f, 0 }, { 0.22f, 0, tw * frand2() * 0.6f }, W, texPale, skin, 0.06f);
      EndBlendMode();
      Vector3 head = Vector3Transform(Vector3{ 0, 1.80f, 0.04f }, W);
      DrawHair(head, 0.5f, 0.95f, 0.97f);   // hair curtain hides the face — dread
      DrawFace(texGhostFace, head, 0.24f, 0.35f * k); // barely seen behind hair
    }
  } else { // Watcher: pale gowned woman, hair over the face, motionless
    Color pale = { 196, 198, 192, 235 };
    BeginBlendMode(BLEND_ALPHA);
    DrawPart({ 0.34f, 1.55f, 0.22f }, { 0, 0.82f, 0 }, { 0, 0, 0.02f }, W, texPale, pale, 0.03f);
    DrawPart({ 0.16f, 0.24f, 0.18f }, { 0, 1.74f, 0 }, { 0.05f, 0, 0.06f }, W, texPale, pale, 0.03f);
    DrawPart({ 0.06f, 1.1f, 0.06f }, { -0.22f, 0.92f, 0 }, { 0, 0, 0.06f }, W, texPale, pale, 0.03f);
    DrawPart({ 0.06f, 1.1f, 0.06f }, { 0.22f, 0.92f, 0 }, { 0, 0, -0.06f }, W, texPale, pale, 0.03f);
    EndBlendMode();
    Vector3 head = Vector3Transform(Vector3{ 0, 1.76f, 0.06f }, W);
    DrawHair(head, 0.5f, 0.8f, 0.95f);
    DrawFace(texGhostFace, head, 0.22f, 0.4f);
  }
}
