// PARANOIA — player controller: pointer-lock look, WASD, collision,
// multi-floor ground resolution (stairs, upstairs, basement).

import * as THREE from '../lib/three.module.js';
import { WORLD } from './world.js';

const EYE = 1.62;
const RADIUS = 0.32;

export class Player {
  constructor(camera, dom) {
    this.camera = camera;
    this.pos = new THREE.Vector3(0, 0, 200);  // feet position
    this.groundY = 0;
    this.yaw = 0;   // facing -z (north, down the path)
    this.pitch = 0;
    this.keys = {};
    this.sprinting = false;
    this.moving = false;
    this.speed = 3.1;
    this.sprintSpeed = 5.4;
    this.stepT = 0;
    this.locked = false;
    this.enabled = false;
    this.onStep = null;

    dom.addEventListener('click', () => {
      if (this.enabled && !this.locked) dom.requestPointerLock();
    });
    document.addEventListener('pointerlockchange', () => {
      this.locked = document.pointerLockElement === dom;
    });
    document.addEventListener('mousemove', (e) => {
      if (!this.locked || !this.enabled) return;
      this.yaw -= e.movementX * 0.0021;
      this.pitch -= e.movementY * 0.0021;
      this.pitch = Math.max(-1.45, Math.min(1.45, this.pitch));
    });
    window.addEventListener('keydown', (e) => { this.keys[e.code] = true; });
    window.addEventListener('keyup', (e) => { this.keys[e.code] = false; });
  }

  teleport(x, z, yaw = this.yaw) {
    this.pos.set(x, this._resolveGround(x, z), z);
    this.groundY = this.pos.y;
    this.yaw = yaw;
  }

  _resolveGround(x, z) {
    // pick the zone height closest to current ground (handles overlapped floors)
    let best = 0, bestD = Infinity;
    for (const zo of WORLD.groundZones) {
      if (!zo.contains(x, z)) continue;
      const h = zo.height(x, z);
      const d = Math.abs(h - this.groundY) - (zo.priority || 0) * 0.01;
      if (d < bestD) { bestD = d; best = h; }
    }
    return best;
  }

  _collide(nx, nz) {
    const y = this.groundY;
    for (const c of WORLD.colliders) {
      if (c.type === 'circle') {
        if (y > 2.0 || y < -1.0) continue; // trees etc. only at ground level
        const dx = nx - c.x, dz = nz - c.z;
        const d2 = dx * dx + dz * dz;
        const r = c.r + RADIUS;
        if (d2 < r * r && d2 > 1e-7) {
          const d = Math.sqrt(d2);
          nx = c.x + (dx / d) * r;
          nz = c.z + (dz / d) * r;
        }
      } else { // box / box2
        if (c.minY !== undefined && y < c.minY) continue;
        if (c.maxY !== undefined && y > c.maxY) continue;
        if (c.type === 'box' && (y > 2.0 || y < -1.0) && c.minY === undefined && c.maxY === undefined) continue;
        const cx = Math.max(c.minX, Math.min(nx, c.maxX));
        const cz = Math.max(c.minZ, Math.min(nz, c.maxZ));
        const dx = nx - cx, dz = nz - cz;
        const d2 = dx * dx + dz * dz;
        if (d2 < RADIUS * RADIUS) {
          if (d2 > 1e-7) {
            const d = Math.sqrt(d2);
            nx = cx + (dx / d) * RADIUS;
            nz = cz + (dz / d) * RADIUS;
          } else {
            // inside the box: push out along smallest axis
            const pushL = nx - c.minX + RADIUS, pushR = c.maxX - nx + RADIUS;
            const pushB = nz - c.minZ + RADIUS, pushF = c.maxZ - nz + RADIUS;
            const m = Math.min(pushL, pushR, pushB, pushF);
            if (m === pushL) nx = c.minX - RADIUS;
            else if (m === pushR) nx = c.maxX + RADIUS;
            else if (m === pushB) nz = c.minZ - RADIUS;
            else nz = c.maxZ + RADIUS;
          }
        }
      }
    }
    return [nx, nz];
  }

  update(dt, stress) {
    if (!this.enabled) { this._applyCamera(); return; }
    const k = this.keys;
    let fwd = 0, str = 0;
    if (k['KeyW'] || k['ArrowUp']) fwd += 1;
    if (k['KeyS'] || k['ArrowDown']) fwd -= 1;
    if (k['KeyA'] || k['ArrowLeft']) str -= 1;
    if (k['KeyD'] || k['ArrowRight']) str += 1;
    this.sprinting = !!(k['ShiftLeft'] || k['ShiftRight']) && fwd > 0;
    this.moving = fwd !== 0 || str !== 0;

    if (this.moving) {
      const len = Math.hypot(fwd, str);
      fwd /= len; str /= len;
      let sp = this.sprinting ? this.sprintSpeed : this.speed;
      if (stress > 80) sp *= 0.82; // panic slows you
      const sin = Math.sin(this.yaw), cos = Math.cos(this.yaw);
      const dx = (sin * -fwd + cos * str) * sp * dt;
      const dz = (cos * -fwd - sin * str) * sp * dt;
      let nx = this.pos.x + dx, nz = this.pos.z + dz;
      [nx, nz] = this._collide(nx, nz);
      [nx, nz] = this._collide(nx, nz); // second pass for corners
      this.pos.x = nx; this.pos.z = nz;

      this.stepT -= dt * (this.sprinting ? 1.7 : 1);
      if (this.stepT <= 0) {
        this.stepT = 0.52;
        if (this.onStep) this.onStep(this.sprinting);
      }
    }

    // ground height (stairs/floors)
    const target = this._resolveGround(this.pos.x, this.pos.z);
    this.groundY += (target - this.groundY) * Math.min(1, dt * 14);
    this.pos.y = this.groundY;

    this._applyCamera();
  }

  _applyCamera() {
    this.camera.position.set(this.pos.x, this.pos.y + EYE, this.pos.z);
    this.camera.rotation.order = 'YXZ';
    this.camera.rotation.y = this.yaw;
    this.camera.rotation.x = this.pitch;
  }

  forwardDir() {
    return new THREE.Vector3(-Math.sin(this.yaw), 0, -Math.cos(this.yaw));
  }
}
