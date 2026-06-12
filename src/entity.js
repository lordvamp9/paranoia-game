// PARANOIA — Shadow Entities
// Pure black humanoids, proportions slightly wrong, jerky movement.
// State machine: idle(patrol) → alerted → pursuing → hunting → retreating.
// They avoid direct light at close range, coordinate with each other,
// and never enter the basement.

import * as THREE from '../lib/three.module.js';
import { WORLD } from './world.js';

const BLACK = new THREE.MeshBasicMaterial({ color: 0x000000 });

let nextId = 1;

export class ShadowEntity {
  constructor(scene, home, patrol, opts = {}) {
    this.id = nextId++;
    this.scene = scene;
    this.home = home.clone();
    this.patrol = patrol; // array of Vector3 waypoints
    this.patrolIdx = 0;
    this.state = 'idle';
    this.stateT = 0;
    this.pos = home.clone();
    this.lethal = opts.lethal !== false;
    this.aggression = opts.aggression ?? 1;
    this.lastSeenPlayer = new THREE.Vector3();
    this.lastSeenTime = -999;
    this.lostSightT = 0;
    this.frozen = false;
    this.huntRole = 'chaser';
    this.recoilT = 0;

    this.patrolSpeed = 1.6;
    this.alertSpeed = 2.2;
    this.pursuitSpeed = 3.4 * this.aggression;
    this.huntSpeed = 4.6 * this.aggression;

    this._buildMesh();
    // jerky rendering: visual position updates at ~9 Hz
    this.visPos = this.pos.clone();
    this.jerkT = 0;
  }

  _buildMesh() {
    const g = new THREE.Group();
    // proportions deliberately wrong: too tall, arms too long, head small
    const head = new THREE.Mesh(new THREE.BoxGeometry(0.21, 0.27, 0.22), BLACK);
    head.position.y = 1.97;
    head.rotation.z = 0.07;
    const torso = new THREE.Mesh(new THREE.BoxGeometry(0.46, 0.85, 0.24), BLACK);
    torso.position.y = 1.4;
    const armL = new THREE.Mesh(new THREE.BoxGeometry(0.09, 1.05, 0.09), BLACK);
    armL.position.set(-0.31, 1.25, 0);
    const armR = new THREE.Mesh(new THREE.BoxGeometry(0.09, 1.1, 0.09), BLACK);
    armR.position.set(0.31, 1.2, 0);
    const legL = new THREE.Mesh(new THREE.BoxGeometry(0.13, 1.0, 0.13), BLACK);
    legL.position.set(-0.12, 0.5, 0);
    const legR = new THREE.Mesh(new THREE.BoxGeometry(0.13, 1.0, 0.13), BLACK);
    legR.position.set(0.12, 0.5, 0);
    g.add(head, torso, armL, armR, legL, legR);
    this.limbs = { armL, armR, legL, legR, head };
    g.scale.setScalar(1.05); // ≈2.1 m tall
    this.mesh = g;
    this.scene.add(g);
    // shadow-stain under it (the floor remembers)
    const stain = new THREE.Mesh(
      new THREE.CircleGeometry(0.7, 10),
      new THREE.MeshBasicMaterial({ color: 0x000000, transparent: true, opacity: 0.55, depthWrite: false })
    );
    stain.rotation.x = -Math.PI / 2;
    stain.position.y = 0.02;
    g.add(stain);
  }

  setState(s) {
    if (this.state === s) return;
    this.state = s;
    this.stateT = 0;
  }

  // --- senses -----------------------------------------------------------

  litByPlayer(player, phone) {
    if (!phone.lightOn || phone.battery <= 0) return false;
    const toMe = this.pos.clone().sub(player.pos);
    const dist = toMe.length();
    if (dist > 19) return false;
    toMe.normalize();
    const fwd = player.forwardDir();
    return fwd.dot(toMe) > 0.82; // inside the flashlight cone
  }

  inBasement(p) {
    return p.y < -1;
  }

  // --- main update ------------------------------------------------------

  update(dt, time, player, phone, all, onCatch, onLitClose) {
    if (this.frozen) { this._render(dt, time); return; }
    this.stateT += dt;
    this.recoilT = Math.max(0, this.recoilT - dt);

    const playerInBasement = player.groundY < -1;
    const dist = this.pos.distanceTo(player.pos);
    const lit = this.litByPlayer(player, phone);

    // --- sense triggers
    if (lit && dist < 6 && this.recoilT <= 0) {
      // direct light at close range: recoil hard
      this.recoilT = 2.2;
      this.setState('retreating');
      if (onLitClose) onLitClose(this, dist);
    } else if (!playerInBasement && this.state !== 'retreating') {
      if (lit && dist >= 6) {
        // it saw your light. it knows.
        this.lastSeenPlayer.copy(player.pos);
        this.lastSeenTime = time;
        if (this.state === 'idle' || this.state === 'alerted') {
          this.setState(this.stateT > 2 && this.aggression > 0.8 ? 'pursuing' : 'alerted');
          if (this.state === 'pursuing') this._alertOthers(all, player);
        }
      }
      if (dist < 5 && !lit) {
        // proximity in the dark: immediate pursuit
        this.lastSeenPlayer.copy(player.pos);
        this.lastSeenTime = time;
        this.setState('pursuing');
        this._alertOthers(all, player);
      }
      if (player.sprinting && player.moving && dist < 15 && this.state === 'idle') {
        this.lastSeenPlayer.copy(player.pos);
        this.setState('alerted');
      }
    }
    if (playerInBasement && (this.state === 'pursuing' || this.state === 'hunting')) {
      this.setState('retreating'); // they will not follow you down there
    }

    // --- state behaviour
    let speed = this.patrolSpeed;
    let target = null;

    switch (this.state) {
      case 'idle': {
        if (this.patrol.length) {
          target = this.patrol[this.patrolIdx];
          if (this.pos.distanceTo(target) < 1.2) {
            this.patrolIdx = (this.patrolIdx + 1) % this.patrol.length;
            if (Math.random() < 0.3) this.pauseT = 1.5 + Math.random() * 2.5; // stop to "listen"
          }
        }
        if (this.pauseT > 0) { this.pauseT -= dt; target = null; }
        break;
      }
      case 'alerted': {
        speed = this.alertSpeed;
        target = this.lastSeenPlayer;
        if (this.stateT > 12 || (target && this.pos.distanceTo(target) < 1.5 && this.stateT > 4)) {
          this.setState('idle');
        }
        // escalate if player visible while alerted
        if (dist < 11 && time - this.lastSeenTime < 1) {
          this.setState('pursuing');
          this._alertOthers(all, player);
        }
        break;
      }
      case 'pursuing': {
        speed = this.pursuitSpeed;
        // predict player movement
        const lead = player.moving ? player.forwardDir().multiplyScalar(1.6) : new THREE.Vector3();
        target = player.pos.clone().add(lead);
        this.lastSeenPlayer.copy(player.pos);
        // count pursuers → coordinated hunt
        const pursuers = all.filter(e => (e.state === 'pursuing' || e.state === 'hunting') && !e.frozen);
        if (pursuers.length >= 2) {
          pursuers.forEach((e, i) => {
            e.setState('hunting');
            e.huntRole = i === 0 ? 'chaser' : 'flanker';
          });
        }
        // frustration: blocked too long / blinded persistently
        if (lit && dist < 10 && this.stateT > 6 && Math.random() < dt * 0.25) {
          this.setState('alerted');
        }
        if (dist > 26) this.lostSightT += dt; else this.lostSightT = 0;
        if (this.lostSightT > 20) { this.lostSightT = 0; this.setState('idle'); }
        break;
      }
      case 'hunting': {
        speed = this.huntSpeed;
        if (this.huntRole === 'chaser') {
          target = player.pos.clone();
        } else {
          // flanker: cut off the player's forward escape
          const cut = player.forwardDir().multiplyScalar(7);
          target = player.pos.clone().add(cut);
        }
        const pursuers = all.filter(e => (e.state === 'pursuing' || e.state === 'hunting') && !e.frozen);
        if (pursuers.length < 2) this.setState('pursuing');
        if (dist > 30) this.setState('alerted');
        break;
      }
      case 'retreating': {
        speed = this.alertSpeed * 1.4;
        target = this.home;
        if (this.pos.distanceTo(this.home) < 2 && this.stateT > 3) {
          if (this.stateT > 6) this.setState('idle');
          else target = null;
        }
        break;
      }
    }

    // --- move (never into the basement / house upper floor; ground only)
    if (target) {
      const dir = target.clone().sub(this.pos);
      dir.y = 0;
      const d = dir.length();
      if (d > 0.05) {
        dir.normalize();
        let nx = this.pos.x + dir.x * speed * dt;
        let nz = this.pos.z + dir.z * speed * dt;
        // entities slide around trees crudely
        for (const c of WORLD.colliders) {
          if (c.type !== 'circle') continue;
          const dx = nx - c.x, dz = nz - c.z;
          const d2 = dx * dx + dz * dz, r = c.r + 0.3;
          if (d2 < r * r && d2 > 1e-6) {
            const dd = Math.sqrt(d2);
            nx = c.x + (dx / dd) * r;
            nz = c.z + (dz / dd) * r;
          }
        }
        this.pos.x = nx; this.pos.z = nz;
        this.pos.y = 0; // they glide on the ground plane
      }
    }

    // --- catch the player
    if (this.lethal && dist < 0.95 && !playerInBasement &&
      (this.state === 'pursuing' || this.state === 'hunting')) {
      onCatch(this);
    }

    this._render(dt, time, speed, target != null);
  }

  _alertOthers(all, player) {
    for (const e of all) {
      if (e === this || e.frozen) continue;
      if (e.pos.distanceTo(this.pos) < 45 && (e.state === 'idle' || e.state === 'alerted')) {
        e.lastSeenPlayer.copy(player.pos);
        e.setState('pursuing');
      }
    }
  }

  _render(dt, time, speed = 0, moving = false) {
    // jerky discrete updates — like missing frames
    this.jerkT -= dt;
    if (this.jerkT <= 0) {
      this.jerkT = 0.07 + Math.random() * 0.07;
      this.visPos.copy(this.pos);
      if (this.state !== 'idle') {
        this.visPos.x += (Math.random() - 0.5) * 0.1;
        this.visPos.z += (Math.random() - 0.5) * 0.1;
      }
      // discrete limb poses, never interpolated
      const a = Math.random() * Math.PI - Math.PI / 2;
      if (moving) {
        this.limbs.armL.rotation.x = a * 0.7;
        this.limbs.armR.rotation.x = -a * 0.8;
        this.limbs.legL.rotation.x = -a * 0.5;
        this.limbs.legR.rotation.x = a * 0.5;
        this.limbs.head.rotation.y = (Math.random() - 0.5) * 0.5;
      } else {
        this.limbs.head.rotation.y = Math.sin(time * 0.3 + this.id) * 0.8;
      }
    }
    this.mesh.position.copy(this.visPos);
  }

  faceTowards(p) {
    this.mesh.lookAt(p.x, this.mesh.position.y, p.z);
  }

  remove() {
    this.scene.remove(this.mesh);
    this.frozen = true;
    this.removed = true;
  }
}
