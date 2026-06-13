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
  } else if (!playerBelow && e.state != EState::Retreating) {
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
      e.pos.x = nx; e.pos.z = nz; e.pos.y = 0;
      e.facing = atan2f(dir.x, dir.z);
      e.speed = speed;
      e.walkPhase += speed * dt * 1.45f; // stride drives the legs
    }
  }
  if (frand() < dt * 1.2f) e.twitchT = 0.06f + frand() * 0.08f;
  e.twitchT = fmaxf(0, e.twitchT - dt);

  if (e.lethal && dist < 0.9f && !playerBelow && !playerAbove &&
      (e.state == EState::Pursuing || e.state == EState::Hunting)) {
    DirectorCatch(e);
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

void EntityDraw(Entity& e, float time) {
  if (!gEntInit) { mshPart = GenMeshCube(1, 1, 1); gMatEnt = LoadMaterialDefault(); gEntInit = true; }
  gMatEnt.shader = gGfx.world;

  float tw = (e.twitchT > 0) ? 1.0f : 0.0f;
  float jx = tw * frand2() * 0.06f, jz = tw * frand2() * 0.06f;
  Matrix W = MatrixMultiply(MatrixRotateY(e.facing), MatrixTranslate(e.pos.x + jx, e.pos.y, e.pos.z + jz));

  if (e.kind == EKind::Shadow) {
    Color black = { 0, 0, 0, 255 };
    float swing = sinf(e.walkPhase) * Clamp(e.speed / 4.0f, 0.0f, 1.0f);
    float bob = fabsf(cosf(e.walkPhase)) * 0.06f * Clamp(e.speed / 2.0f, 0.0f, 1.0f);
    float headTilt = tw * frand2() * 0.5f + sinf(time * 0.3f + e.walkPhase * 0.1f) * 0.08f;
    // legs plant the ground: hips at 1.0, feet swing
    DrawPart({ 0.13f, 1.0f, 0.13f }, { -0.12f, 0.5f + bob, 0 }, { swing * 0.55f, 0, 0 }, W, gGfx.white, black, 0);
    DrawPart({ 0.13f, 1.0f, 0.13f }, { 0.12f, 0.5f + bob, 0 }, { -swing * 0.55f, 0, 0 }, W, gGfx.white, black, 0);
    DrawPart({ 0.46f, 0.85f, 0.24f }, { 0, 1.4f + bob, 0 }, { 0.04f, 0, 0.03f }, W, gGfx.white, black, 0);
    DrawPart({ 0.09f, 1.1f, 0.09f }, { -0.31f, 1.22f + bob, 0 }, { -swing * 0.5f, 0, 0.08f }, W, gGfx.white, black, 0);
    DrawPart({ 0.09f, 1.15f, 0.09f }, { 0.31f, 1.18f + bob, 0 }, { swing * 0.5f, 0, -0.08f }, W, gGfx.white, black, 0);
    DrawPart({ 0.21f, 0.27f, 0.22f }, { 0, 1.97f + bob, 0 }, { 0, headTilt, 0.07f }, W, gGfx.white, black, 0);
    // its stain on the ground
    DrawCylinderEx({ e.pos.x, 0.015f, e.pos.z }, { e.pos.x, 0.02f, e.pos.z }, 0.65f, 0.65f, 10, Fade(BLACK, 0.55f));
  } else if (e.kind == EKind::Samara) {
    Color gown = { 225, 228, 230, 255 };
    Color skin = { 200, 205, 198, 255 };
    Color hair = { 8, 8, 10, 255 };
    bool crawl = (e.state == EState::Pursuing || e.state == EState::Retreating);
    float ph = e.walkPhase;
    float lurch = sinf(ph) * 0.6f;
    if (crawl) {
      // on all fours, spine parallel to ground, head up at you. wrong.
      Matrix C = MatrixMultiply(MatrixRotateX(1.25f), W);
      DrawPart({ 0.42f, 1.15f, 0.30f }, { 0, 0.75f, -0.15f }, { 0, 0, lurch * 0.10f }, C, texPale, gown, 0.07f);
      DrawPart({ 0.085f, 0.85f, 0.085f }, { -0.26f, 0.45f, 0.32f }, { lurch * 0.9f - 0.7f, 0, 0.2f }, W, texPale, skin, 0.05f);
      DrawPart({ 0.085f, 0.85f, 0.085f }, { 0.26f, 0.45f, 0.32f }, { -lurch * 0.9f - 0.7f, 0, -0.2f }, W, texPale, skin, 0.05f);
      DrawPart({ 0.10f, 0.8f, 0.10f }, { -0.16f, 0.4f, -0.78f }, { lurch * 0.7f + 0.5f, 0, 0 }, W, texPale, gown, 0.04f);
      DrawPart({ 0.10f, 0.8f, 0.10f }, { 0.16f, 0.4f, -0.78f }, { -lurch * 0.7f + 0.5f, 0, 0 }, W, texPale, gown, 0.04f);
      // head snapped up through the hair
      DrawPart({ 0.20f, 0.25f, 0.21f }, { 0, 0.78f, 0.55f }, { -0.9f, tw * frand2() * 0.8f, lurch * 0.2f }, W, texPale, skin, 0.06f);
      DrawPart({ 0.30f, 0.5f, 0.26f }, { 0, 0.72f, 0.50f }, { -0.6f, 0, 0 }, W, gGfx.white, hair, 0);
    } else {
      // rising from the well: vertical, dripping, hair curtain over the face
      float k = (e.state == EState::Emerging) ? Clamp(e.stateT / 3.6f, 0.0f, 1.0f) : 1.0f;
      DrawPart({ 0.46f, 1.45f, 0.32f }, { 0, 0.85f, 0 }, { 0.06f, 0, sinf(time * 7) * 0.02f * k }, W, texPale, gown, 0.07f);
      DrawPart({ 0.08f, 0.95f, 0.08f }, { -0.28f, 1.05f, 0 }, { 0, 0, 0.16f }, W, texPale, skin, 0.05f);
      DrawPart({ 0.08f, 0.95f, 0.08f }, { 0.28f, 1.02f, 0 }, { 0, 0, -0.20f }, W, texPale, skin, 0.05f);
      DrawPart({ 0.21f, 0.26f, 0.22f }, { 0, 1.78f, 0 }, { 0.22f, 0, tw * frand2() * 0.6f }, W, texPale, skin, 0.06f);
      DrawPart({ 0.34f, 0.72f, 0.30f }, { 0, 1.62f, 0.05f }, { 0.1f, 0, 0 }, W, gGfx.white, hair, 0);
    }
  } else { // Watcher
    Color pale = { 188, 190, 186, 255 };
    DrawPart({ 0.30f, 1.7f, 0.20f }, { 0, 0.85f, 0 }, { 0, 0, 0.02f }, W, texPale, pale, 0.03f);
    DrawPart({ 0.16f, 0.24f, 0.18f }, { 0, 1.86f, 0 }, { 0.1f, 0, 0.12f }, W, texPale, pale, 0.03f);
    DrawPart({ 0.06f, 1.15f, 0.06f }, { -0.20f, 0.95f, 0 }, { 0, 0, 0.06f }, W, texPale, pale, 0.03f);
    DrawPart({ 0.06f, 1.15f, 0.06f }, { 0.20f, 0.95f, 0 }, { 0, 0, -0.06f }, W, texPale, pale, 0.03f);
  }
}
