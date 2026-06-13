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

// ---- smooth humanoid primitives (spheres + tapered bones) ----
static Mesh eSphere, eCyl;

static void ESph(Vector3 c, float r, Texture2D tex, Color tint, float spec) {
  Matrix m = MatrixMultiply(MatrixScale(r, r, r), MatrixTranslate(c.x, c.y, c.z));
  SetSpec(0, spec);
  gMatEnt.maps[MATERIAL_MAP_ALBEDO].texture = tex;
  gMatEnt.maps[MATERIAL_MAP_ALBEDO].color = tint;
  DrawMesh(eSphere, gMatEnt, m);
}
static void ESphE(Vector3 c, Vector3 rad, Texture2D tex, Color tint, float spec) { // ellipsoid
  Matrix m = MatrixMultiply(MatrixScale(rad.x, rad.y, rad.z), MatrixTranslate(c.x, c.y, c.z));
  SetSpec(0, spec);
  gMatEnt.maps[MATERIAL_MAP_ALBEDO].texture = tex;
  gMatEnt.maps[MATERIAL_MAP_ALBEDO].color = tint;
  DrawMesh(eSphere, gMatEnt, m);
}
// tapered limb between a and b with end radii ra, rb (+ smooth joint balls)
static void EBone(Vector3 a, Vector3 b, float ra, float rb, Texture2D tex, Color tint, float spec) {
  Vector3 d = Vector3Subtract(b, a);
  float L = Vector3Length(d);
  if (L < 1e-4f) return;
  Quaternion q = QuaternionFromVector3ToVector3({ 0, 1, 0 }, Vector3Scale(d, 1.0f / L));
  // cylinder is uniform; approximate taper by average radius + joint balls of ra/rb
  float r = (ra + rb) * 0.5f;
  Matrix m = MatrixMultiply(MatrixMultiply(MatrixScale(r, L, r), QuaternionToMatrix(q)), MatrixTranslate(a.x, a.y, a.z));
  SetSpec(0, spec);
  gMatEnt.maps[MATERIAL_MAP_ALBEDO].texture = tex;
  gMatEnt.maps[MATERIAL_MAP_ALBEDO].color = tint;
  DrawMesh(eCyl, gMatEnt, m);
  ESph(a, ra, tex, tint, spec);
  ESph(b, rb, tex, tint, spec);
}
static Vector3 L2W(Matrix W, float x, float y, float z) { return Vector3Transform(Vector3{ x, y, z }, W); }

// sculpted gaunt head: skull + brow + cheeks + jaw + recessed black eyes + maw
static void EHead(Matrix W, Vector3 lc, float r, Texture2D skinTex, Color skin, bool maw, float twitch) {
  float ty = twitch * frand2() * 0.06f;
  Vector3 c = L2W(W, lc.x, lc.y + ty, lc.z);
  ESphE(c, { r * 0.92f, r * 1.12f, r * 0.98f }, skinTex, skin, 0.04f);        // cranium (tall)
  ESphE(L2W(W, lc.x, lc.y - r * 0.55f, lc.z + r * 0.28f), { r * 0.7f, r * 0.55f, r * 0.55f }, skinTex, skin, 0.04f); // jaw
  ESphE(L2W(W, lc.x, lc.y + r * 0.28f, lc.z + r * 0.74f), { r * 0.78f, r * 0.30f, r * 0.30f }, skinTex, skin, 0.03f); // brow ridge
  // recessed black eye sockets
  Color blk = { 2, 2, 4, 255 };
  ESphE(L2W(W, lc.x - r * 0.42f, lc.y + r * 0.08f, lc.z + r * 0.80f), { r * 0.30f, r * 0.34f, r * 0.22f }, gGfx.white, blk, 0);
  ESphE(L2W(W, lc.x + r * 0.42f, lc.y + r * 0.08f, lc.z + r * 0.80f), { r * 0.30f, r * 0.34f, r * 0.22f }, gGfx.white, blk, 0);
  // nose ridge
  ESphE(L2W(W, lc.x, lc.y - r * 0.10f, lc.z + r * 0.95f), { r * 0.16f, r * 0.32f, r * 0.20f }, skinTex, skin, 0.04f);
  // gaping mouth
  if (maw) ESphE(L2W(W, lc.x, lc.y - r * 0.62f, lc.z + r * 0.72f), { r * 0.34f, r * 0.42f, r * 0.20f }, gGfx.white, blk, 0);
  else ESphE(L2W(W, lc.x, lc.y - r * 0.58f, lc.z + r * 0.80f), { r * 0.30f, r * 0.12f, r * 0.12f }, gGfx.white, blk, 0);
}

// full standing humanoid; sw = limb swing, lean forward optional
static void EHumanoid(Matrix W, Entity& e, float time, Texture2D tex, Color skin, bool longArms) {
  float sp = Clamp(e.speed / 4.0f, 0.0f, 1.0f);
  float sw = sinf(e.walkPhase) * 0.34f * sp;
  float bob = fabsf(cosf(e.walkPhase)) * 0.04f * sp;
  float yo = bob;
  float aL = longArms ? 0.40f : 0.32f; // arm segment length
  // torso
  Vector3 hip = L2W(W, 0, 0.92f + yo, 0), chest = L2W(W, 0, 1.42f + yo, 0), neck = L2W(W, 0, 1.60f + yo, 0);
  EBone(hip, chest, 0.15f, 0.13f, tex, skin, 0.05f);          // torso
  EBone(chest, neck, 0.07f, 0.055f, tex, skin, 0.05f);        // neck
  ESphE(L2W(W, 0, 1.06f + yo, 0), { 0.16f, 0.18f, 0.13f }, tex, skin, 0.05f); // pelvis mass
  // head
  EHead(W, { 0, 1.78f + yo, 0 }, 0.135f, tex, skin, true, e.twitchT > 0 ? 1.0f : 0.0f);
  // arms (long, hanging, swinging)
  Vector3 shL = L2W(W, -0.18f, 1.50f + yo, 0), shR = L2W(W, 0.18f, 1.50f + yo, 0);
  Vector3 elL = L2W(W, -0.24f, 1.50f - aL + yo, sw * 0.5f), elR = L2W(W, 0.24f, 1.50f - aL + yo, -sw * 0.5f);
  Vector3 haL = L2W(W, -0.26f, 1.50f - aL * 2 + yo, sw), haR = L2W(W, 0.26f, 1.50f - aL * 2 + yo, -sw);
  EBone(shL, elL, 0.055f, 0.045f, tex, skin, 0.05f); EBone(elL, haL, 0.045f, 0.05f, tex, skin, 0.05f);
  EBone(shR, elR, 0.055f, 0.045f, tex, skin, 0.05f); EBone(elR, haR, 0.045f, 0.05f, tex, skin, 0.05f);
  // legs
  Vector3 hpL = L2W(W, -0.10f, 0.90f, 0), hpR = L2W(W, 0.10f, 0.90f, 0);
  Vector3 knL = L2W(W, -0.11f, 0.46f, sw * 0.6f), knR = L2W(W, 0.11f, 0.46f, -sw * 0.6f);
  Vector3 ftL = L2W(W, -0.11f, 0.04f, -sw * 0.3f), ftR = L2W(W, 0.11f, 0.04f, sw * 0.3f);
  EBone(hpL, knL, 0.08f, 0.06f, tex, skin, 0.05f); EBone(knL, ftL, 0.06f, 0.05f, tex, skin, 0.05f);
  EBone(hpR, knR, 0.08f, 0.06f, tex, skin, 0.05f); EBone(knR, ftR, 0.06f, 0.05f, tex, skin, 0.05f);
}

// humanoid on all fours (crawler / samara crawl)
static void ECrawler(Matrix W, Entity& e, float time, Texture2D tex, Color skin, bool grin) {
  float ph = e.walkPhase;
  float lg = sinf(ph) * 0.5f, lg2 = sinf(ph + PI) * 0.5f;
  // spine roughly horizontal, hips low, head up toward player
  Vector3 hip = L2W(W, 0, 0.55f, -0.55f), chest = L2W(W, 0, 0.62f, 0.10f), neck = L2W(W, 0, 0.66f, 0.45f);
  EBone(hip, chest, 0.16f, 0.13f, tex, skin, 0.06f);
  EBone(chest, neck, 0.08f, 0.06f, tex, skin, 0.06f);
  ESphE(L2W(W, 0, 0.55f, -0.5f), { 0.17f, 0.16f, 0.18f }, tex, skin, 0.06f);
  EHead(W, { 0, 0.70f, 0.62f }, 0.135f, tex, skin, true, e.twitchT > 0 ? 1.0f : 0.0f);
  // arms reaching forward to the ground
  Vector3 shL = L2W(W, -0.20f, 0.62f, 0.15f), shR = L2W(W, 0.20f, 0.62f, 0.15f);
  Vector3 haL = L2W(W, -0.24f, 0.02f, 0.45f + lg * 0.3f), haR = L2W(W, 0.24f, 0.02f, 0.45f + lg2 * 0.3f);
  Vector3 elL = Vector3Lerp(shL, haL, 0.5f); elL.y += 0.12f;
  Vector3 elR = Vector3Lerp(shR, haR, 0.5f); elR.y += 0.12f;
  EBone(shL, elL, 0.05f, 0.04f, tex, skin, 0.05f); EBone(elL, haL, 0.04f, 0.045f, tex, skin, 0.05f);
  EBone(shR, elR, 0.05f, 0.04f, tex, skin, 0.05f); EBone(elR, haR, 0.04f, 0.045f, tex, skin, 0.05f);
  // legs trailing
  Vector3 hpL = L2W(W, -0.12f, 0.5f, -0.6f), hpR = L2W(W, 0.12f, 0.5f, -0.6f);
  Vector3 ftL = L2W(W, -0.14f, 0.04f, -1.05f + lg2 * 0.3f), ftR = L2W(W, 0.14f, 0.04f, -1.05f + lg * 0.3f);
  Vector3 knL = Vector3Lerp(hpL, ftL, 0.5f); knL.y += 0.14f;
  Vector3 knR = Vector3Lerp(hpR, ftR, 0.5f); knR.y += 0.14f;
  EBone(hpL, knL, 0.07f, 0.055f, tex, skin, 0.05f); EBone(knL, ftL, 0.055f, 0.05f, tex, skin, 0.05f);
  EBone(hpR, knR, 0.07f, 0.055f, tex, skin, 0.05f); EBone(knR, ftR, 0.055f, 0.05f, tex, skin, 0.05f);
}

void EntityDraw(Entity& e, float time) {
  if (!gEntInit) {
    mshPart = GenMeshCube(1, 1, 1); gMatEnt = LoadMaterialDefault(); BuildEntityTextures();
    eSphere = GenMeshSphere(1.0f, 10, 12); eCyl = GenMeshCylinder(1.0f, 1.0f, 9);
    gEntInit = true;
  }
  gMatEnt.shader = gGfx.world;

  float tw = (e.twitchT > 0) ? 1.0f : 0.0f;
  float jx = tw * frand2() * 0.05f, jz = tw * frand2() * 0.05f;
  Matrix W = MatrixMultiply(MatrixRotateY(e.facing), MatrixTranslate(e.pos.x + jx, e.pos.y, e.pos.z + jz));

  if (e.kind == EKind::Shadow) {
    // a gaunt ash-grey humanoid, translucent, long-armed; pale gaunt face
    Color skin = { 78, 80, 92, 232 };
    BeginBlendMode(BLEND_ALPHA);
    EHumanoid(W, e, time, texSkin, skin, true);
    EndBlendMode();
    DrawHair(L2W(W, 0, 1.92f, 0.04f), 0.36f, 0.5f, 0.7f);
    DrawCylinderEx({ e.pos.x, 0.015f, e.pos.z }, { e.pos.x, 0.02f, e.pos.z }, 0.6f, 0.6f, 12, Fade(BLACK, 0.45f));
  } else if (e.kind == EKind::Crawler) {
    Color flesh = { 196, 192, 182, 244 };
    BeginBlendMode(BLEND_ALPHA);
    ECrawler(W, e, time, texSkin, flesh, true);
    EndBlendMode();
    DrawHair(L2W(W, 0, 0.80f, 0.55f), 0.42f, 0.34f, 0.9f);
  } else if (e.kind == EKind::Samara) {
    Color gown = { 214, 218, 220, 246 };
    bool crawl = (e.state == EState::Pursuing || e.state == EState::Retreating);
    BeginBlendMode(BLEND_ALPHA);
    if (crawl) {
      ECrawler(W, e, time, texPale, gown, false);
      EndBlendMode();
      DrawHair(L2W(W, 0, 0.78f, 0.62f), 0.5f, 0.72f, 0.97f); // long wet hair forward over the face
    } else {
      float k = (e.state == EState::Emerging) ? Clamp(e.stateT / 3.6f, 0.0f, 1.0f) : 1.0f;
      EHumanoid(W, e, time, texPale, gown, false);
      EndBlendMode();
      DrawHair(L2W(W, 0, 1.80f, 0.05f), 0.52f, 0.95f, 0.97f * k); // curtain hides the face
    }
  } else { // Watcher: pale woman, motionless, hair over the face
    Color pale = { 188, 190, 186, 236 };
    e.speed = 0;
    BeginBlendMode(BLEND_ALPHA);
    EHumanoid(W, e, time, texPale, pale, false);
    EndBlendMode();
    DrawHair(L2W(W, 0, 1.80f, 0.05f), 0.52f, 0.85f, 0.96f);
  }
}
