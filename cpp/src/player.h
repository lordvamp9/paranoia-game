// PARANOIA v2 — player controller: acceleration, gravity, stamina,
// head-bob and camera sway. You have weight now. So do your pursuers.
#pragma once
#include "game.h"

Player gPlayer;

static const float EYE_H = 1.62f;
static const float P_RADIUS = 0.30f;
static const float WALK_SPD = 3.2f;
static const float SPRINT_SPD = 5.8f;

Vector3 PlayerForward() { return { -sinf(gPlayer.yaw), 0, -cosf(gPlayer.yaw) }; }
Vector3 PlayerForward3D() {
  float cp = cosf(gPlayer.pitch);
  return Vector3Normalize({ -sinf(gPlayer.yaw) * cp, sinf(gPlayer.pitch), -cosf(gPlayer.yaw) * cp });
}
Vector3 PlayerEye() {
  float bobY = sinf(gPlayer.bobPhase * 2) * gPlayer.bobAmp - gPlayer.landDip;
  return { gPlayer.pos.x, gPlayer.pos.y + EYE_H + bobY, gPlayer.pos.z };
}

float ResolveGround(float x, float z, float current) {
  float best = 0; bool found = false; int bestPrio = -9999;
  float closest = 0, closestD = 1e9f;
  for (auto& zo : gWorld.zones) {
    if (!zo.contains(x, z)) continue;
    float h = zo.height(x, z);
    float d = fabsf(h - current);
    if (d < closestD) { closestD = d; closest = h; }
    if (d < 0.6f && zo.priority > bestPrio) { bestPrio = zo.priority; best = h; found = true; }
  }
  return found ? best : closest;
}

static void Collide(float& nx, float& nz, float y) {
  for (auto& c : gWorld.circles) {
    if (y > 2.0f || y < -1.0f) continue;
    float dx = nx - c.x, dz = nz - c.z;
    float d2 = dx * dx + dz * dz, r = c.r + P_RADIUS;
    if (d2 < r * r && d2 > 1e-7f) {
      float d = sqrtf(d2);
      nx = c.x + dx / d * r; nz = c.z + dz / d * r;
    }
  }
  for (auto& b : gWorld.boxes) {
    if (!b.enabled) continue;
    if (y < b.minY || y > b.maxY) continue;
    if (b.minY < -1e8f && b.maxY > 1e8f && (y > 2.0f || y < -1.0f)) continue; // plain ground boxes
    float cx = Clamp(nx, b.minX, b.maxX), cz = Clamp(nz, b.minZ, b.maxZ);
    float dx = nx - cx, dz = nz - cz;
    float d2 = dx * dx + dz * dz;
    if (d2 < P_RADIUS * P_RADIUS) {
      if (d2 > 1e-7f) {
        float d = sqrtf(d2);
        nx = cx + dx / d * P_RADIUS; nz = cz + dz / d * P_RADIUS;
      } else {
        float pl = nx - b.minX + P_RADIUS, pr = b.maxX - nx + P_RADIUS;
        float pb = nz - b.minZ + P_RADIUS, pf = b.maxZ - nz + P_RADIUS;
        float m = fminf(fminf(pl, pr), fminf(pb, pf));
        if (m == pl) nx = b.minX - P_RADIUS;
        else if (m == pr) nx = b.maxX + P_RADIUS;
        else if (m == pb) nz = b.minZ - P_RADIUS;
        else nz = b.maxZ + P_RADIUS;
      }
    }
  }
}

void PlayerTeleport(float x, float z, float yaw) {
  gPlayer.pos = { x, ResolveGround(x, z, gPlayer.groundY), z };
  gPlayer.groundY = gPlayer.pos.y;
  gPlayer.vel = { 0, 0, 0 };
  gPlayer.yaw = yaw;
}

void PlayerUpdate(float dt, float stress) {
  Player& P = gPlayer;
  if (P.enabled) {
    Vector2 md = GetMouseDelta();
    float sens = 0.0021f * gCfg.mouseSens * 2.0f; // slider 0.5 = default speed
    P.yaw -= md.x * sens;
    P.pitch = Clamp(P.pitch - md.y * sens, -1.45f, 1.45f);

    float fwd = 0, str = 0;
    if (ActDown(ACT_FWD) || IsKeyDown(KEY_UP)) fwd += 1;
    if (ActDown(ACT_BACK) || IsKeyDown(KEY_DOWN)) fwd -= 1;
    if (ActDown(ACT_LEFT) || IsKeyDown(KEY_LEFT)) str -= 1;
    if (ActDown(ACT_RIGHT) || IsKeyDown(KEY_RIGHT)) str += 1;
    bool wantSprint = ActDown(ACT_SPRINT) && fwd > 0;

    // stamina: you cannot run forever; they count on that
    if (wantSprint && !P.exhausted && P.stamina > 0) {
      P.sprinting = true;
      P.stamina = fmaxf(0, P.stamina - 16.0f * dt);
      if (P.stamina <= 0) P.exhausted = true;
    } else {
      P.sprinting = false;
      P.stamina = fminf(100, P.stamina + (P.moving ? 9.0f : 14.0f) * dt);
      if (P.exhausted && P.stamina > 30) P.exhausted = false;
    }

    P.moving = (fwd != 0 || str != 0);
    float targetSpd = P.sprinting ? SPRINT_SPD : WALK_SPD;
    if (stress > 85) targetSpd *= 0.86f;

    Vector3 wish = { 0, 0, 0 };
    if (P.moving) {
      float len = sqrtf(fwd * fwd + str * str);
      fwd /= len; str /= len;
      float s = sinf(P.yaw), c = cosf(P.yaw);
      wish = { (-s * fwd + c * str) * targetSpd, 0, (-c * fwd - s * str) * targetSpd };
    }
    // acceleration / friction: weight
    float accel = P.moving ? 14.0f : 11.0f;
    P.vel.x += (wish.x - P.vel.x) * fminf(1.0f, accel * dt);
    P.vel.z += (wish.z - P.vel.z) * fminf(1.0f, accel * dt);

    float nx = P.pos.x + P.vel.x * dt;
    float nz = P.pos.z + P.vel.z * dt;
    Collide(nx, nz, P.groundY);
    Collide(nx, nz, P.groundY);
    P.pos.x = nx; P.pos.z = nz;

    // gravity + ground snap (stairs ride this)
    float target = ResolveGround(P.pos.x, P.pos.z, P.groundY);
    if (P.pos.y > target + 0.02f) {
      P.vel.y -= 22.0f * dt;
      P.pos.y += P.vel.y * dt;
      if (P.pos.y <= target) {
        if (P.vel.y < -5.5f) P.landDip = fminf(0.16f, -P.vel.y * 0.02f); // landing thud
        P.pos.y = target; P.vel.y = 0; P.grounded = true;
      } else P.grounded = false;
    } else {
      P.pos.y += (target - P.pos.y) * fminf(1.0f, dt * 14);
      P.vel.y = 0; P.grounded = true;
    }
    P.groundY = P.pos.y;
    P.landDip = fmaxf(0, P.landDip - dt * 0.5f);

    // head bob driven by actual horizontal speed
    float spd = sqrtf(P.vel.x * P.vel.x + P.vel.z * P.vel.z);
    float targetAmp = P.grounded ? Clamp(spd / SPRINT_SPD, 0, 1) * (P.sprinting ? 0.055f : 0.035f) : 0;
    P.bobAmp += (targetAmp - P.bobAmp) * fminf(1, dt * 8);
    float prev = sinf(P.bobPhase * 2);
    P.bobPhase += dt * (3.4f + spd * 1.25f) * (spd > 0.3f ? 1.0f : 0.0f);
    float cur = sinf(P.bobPhase * 2);
    if (prev > -0.7f && cur <= -0.7f && spd > 0.8f) { // footfall at bob bottom
      bool indoor = P.groundY < -1.0f || (fabsf(P.pos.x) < 6.5f && fabsf(P.pos.z + 110) < 5.5f) ||
        (fabsf(P.pos.x - 60) < 3.0f && fabsf(P.pos.z + 30) < 2.4f);
      SfxFootstep(indoor, P.sprinting);
    }
    P.swayT += dt;
  }

  // camera
  float roll = -P.vel.x * cosf(P.yaw) * 0.004f - sinf(P.bobPhase) * P.bobAmp * 0.35f;
  float idle = sinf(P.swayT * 0.9f) * 0.0025f + sinf(P.swayT * 1.7f) * 0.0015f; // breathing
  float tremor = (stress / 100.0f) * 0.003f;
  Vector3 eye = PlayerEye();
  eye.y += idle;
  Vector3 f3 = PlayerForward3D();
  P.cam.position = eye;
  P.cam.target = Vector3Add(eye, f3);
  P.cam.up = Vector3Normalize({ sinf(roll + frand2() * tremor), 1, 0 });
  P.cam.fovy = 71.0f + (P.sprinting ? 2.5f : 0);
  P.cam.projection = CAMERA_PERSPECTIVE;
}
