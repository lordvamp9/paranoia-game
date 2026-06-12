// PARANOIA — the smartphone: your camera, your light, your only companion.
// Hand + phone rendered in front of the camera; the screen is a live
// CanvasTexture (battery, objective, REC, glitch corruption).

import * as THREE from '../lib/three.module.js';

const CORRUPT = { a: '4', e: '3', i: '1', o: '0', s: '5', t: '7', E: '3', A: '4', S: '5', T: '7', O: '0' };
const ZALGO = ['̴', '̶', '̖', '͓', '͙'];

export class Phone {
  constructor(camera) {
    this.camera = camera;
    this.battery = 100;
    this.lightOn = true;
    this.objective = '';
    this.message = '';
    this.messageT = 0;
    this.stress = 0;
    this.sway = new THREE.Vector2();
    this.bob = 0;
    this._uiClock = 0;
    this._flickerT = 0;
    this._flickerOff = false;

    this.group = new THREE.Group();
    camera.add(this.group);
    this.group.position.set(0.23, -0.26, -0.55);
    this.group.rotation.set(-0.12, -0.06, 0.03);

    this._buildPhone();
    this._buildHand();
    this._buildLight();
  }

  _buildPhone() {
    const body = new THREE.Mesh(
      new THREE.BoxGeometry(0.165, 0.34, 0.018),
      new THREE.MeshStandardMaterial({ color: 0x16161a, roughness: 0.35, metalness: 0.7 })
    );
    // screen canvas
    const c = document.createElement('canvas');
    c.width = 256; c.height = 512;
    this.screenCanvas = c;
    this.screenCtx = c.getContext('2d');
    this.screenTex = new THREE.CanvasTexture(c);
    const screen = new THREE.Mesh(
      new THREE.PlaneGeometry(0.15, 0.325),
      new THREE.MeshBasicMaterial({ map: this.screenTex, toneMapped: false })
    );
    screen.position.z = 0.0101;
    // camera bump on the back (it faces the world: it IS recording)
    const camBump = new THREE.Mesh(
      new THREE.CylinderGeometry(0.012, 0.012, 0.006, 10),
      new THREE.MeshStandardMaterial({ color: 0x06070a, roughness: 0.1, metalness: 0.8 })
    );
    camBump.rotation.x = Math.PI / 2;
    camBump.position.set(0.05, 0.13, -0.012);
    this.group.add(body, screen, camBump);
    this.screenMesh = screen;
  }

  _buildHand() {
    const skin = new THREE.MeshStandardMaterial({ color: 0x8a6a52, roughness: 0.75 });
    const hand = new THREE.Group();
    // palm wrapping the lower phone
    const palm = new THREE.Mesh(new THREE.BoxGeometry(0.07, 0.14, 0.055), skin);
    palm.position.set(0.085, -0.1, -0.012);
    palm.rotation.z = 0.18;
    hand.add(palm);
    // thumb across the front
    const thumb = new THREE.Mesh(new THREE.CapsuleGeometry(0.014, 0.07, 3, 6), skin);
    thumb.position.set(0.045, -0.11, 0.02);
    thumb.rotation.z = 1.15;
    hand.add(thumb);
    // fingers curling behind
    for (let i = 0; i < 4; i++) {
      const f = new THREE.Mesh(new THREE.CapsuleGeometry(0.0125, 0.06, 3, 6), skin);
      f.position.set(0.07 - i * 0.001, -0.045 - i * 0.038, -0.028);
      f.rotation.z = Math.PI / 2 - 0.25;
      f.rotation.y = 0.15;
      hand.add(f);
    }
    // wrist/forearm fading off-screen
    const wrist = new THREE.Mesh(new THREE.CapsuleGeometry(0.034, 0.16, 3, 8), skin);
    wrist.position.set(0.13, -0.26, -0.04);
    wrist.rotation.z = 0.45;
    wrist.rotation.x = -0.3;
    hand.add(wrist);
    this.group.add(hand);
  }

  _buildLight() {
    // the phone torch — the only real light in the world
    this.spot = new THREE.SpotLight(0xbfd2e8, 0, 30, 0.5, 0.5, 0.9);
    this.spot.castShadow = true;
    this.spot.shadow.mapSize.set(1024, 1024);
    this.spot.shadow.camera.near = 0.3;
    this.spot.shadow.camera.far = 26;
    this.spot.shadow.bias = -0.002;
    this.camera.add(this.spot);
    this.spot.position.set(0.2, -0.2, -0.3);
    this.spotTarget = new THREE.Object3D();
    this.camera.add(this.spotTarget);
    this.spotTarget.position.set(0, -0.05, -10);
    this.spot.target = this.spotTarget;
    // small fill so the hand/phone are readable
    this.fill = new THREE.PointLight(0x8a96b8, 0.18, 1.2);
    this.fill.position.set(0.15, -0.2, -0.4);
    this.camera.add(this.fill);
  }

  toggleLight() {
    this.lightOn = !this.lightOn;
    return this.lightOn;
  }

  setObjective(text) { this.objective = text; }

  showMessage(text, seconds = 4) {
    this.message = text;
    this.messageT = seconds;
  }

  drainTick(dt, entitiesNear, stress) {
    let rate = 0.025; // idle: phone alive ≈ slow drain
    if (this.lightOn) rate += 0.062;            // light: ~20 min of battery
    rate *= 1 + (stress / 100) * 1.0;           // stress doubles drain
    if (entitiesNear) rate *= 2.0;              // pressure: entities feed on it
    this.battery = Math.max(0, this.battery - rate * dt);
    if (this.battery <= 0) this.lightOn = false;
  }

  recharge(amount = 100) {
    this.battery = Math.min(100, this.battery + amount);
    if (amount >= 50) this.lightOn = true; // the revelation re-lights the torch
  }

  update(dt, time, moving, sprinting, stress, entitiesNear, indoor = false) {
    this.stress = stress;
    this.messageT = Math.max(0, this.messageT - dt);
    this.drainTick(dt, entitiesNear, stress);

    // light intensity from battery + flicker
    this._flickerT -= dt;
    if (this._flickerT <= 0) {
      const flickerChance = this.battery < 20 ? 0.5 : this.battery < 50 ? 0.12 : 0.03;
      const stressBump = stress > 60 ? 0.15 : 0;
      this._flickerOff = Math.random() < flickerChance + stressBump;
      this._flickerT = this._flickerOff ? 0.05 + Math.random() * 0.12 : 0.4 + Math.random() * 1.6;
    }
    const batFactor = this.battery <= 0 ? 0 : 0.45 + 0.55 * Math.min(1, this.battery / 50);
    const on = this.lightOn && !this._flickerOff && this.battery > 0;
    const baseI = indoor ? 22 : 70; // close walls blow out otherwise
    this.spot.intensity += ((on ? baseI * batFactor : 0) - this.spot.intensity) * Math.min(1, dt * 22);
    this.fill.intensity = on ? 0.18 : 0.04;

    // hand sway / bob
    this.bob += dt * (sprinting ? 11 : 7) * (moving ? 1 : 0.25);
    const bobAmt = moving ? (sprinting ? 0.009 : 0.005) : 0.0018;
    const tremor = (stress / 100) * 0.004;
    this.group.position.x = 0.23 + Math.sin(this.bob * 0.5) * bobAmt + (Math.random() - 0.5) * tremor;
    this.group.position.y = -0.26 + Math.abs(Math.sin(this.bob)) * bobAmt + (Math.random() - 0.5) * tremor;
    this.group.rotation.z = 0.03 + Math.sin(this.bob * 0.5) * 0.01 + (Math.random() - 0.5) * tremor;

    // screen redraw ~12 Hz
    this._uiClock -= dt;
    if (this._uiClock <= 0) {
      this._uiClock = 1 / 12;
      this._drawScreen(time);
    }
  }

  _corrupt(text) {
    const lvl = this.stress / 100;
    if (lvl < 0.3) return text;
    let out = '';
    for (const ch of text) {
      let c = ch;
      if (Math.random() < (lvl - 0.3) * 0.9 && CORRUPT[ch]) c = CORRUPT[ch];
      if (lvl > 0.7 && Math.random() < (lvl - 0.7) * 0.8) c += ZALGO[(Math.random() * ZALGO.length) | 0];
      out += c;
    }
    return out;
  }

  _drawScreen(time) {
    const g = this.screenCtx, w = 256, h = 512;
    const s = this.stress / 100;
    if (this.battery <= 0) {
      g.fillStyle = '#000';
      g.fillRect(0, 0, w, h);
      this.screenTex.needsUpdate = true;
      return;
    }
    // camera-feed look: very dark viewport with subtle noise
    g.fillStyle = '#04070a';
    g.fillRect(0, 0, w, h);
    g.fillStyle = 'rgba(255,255,255,0.022)';
    for (let i = 0; i < 40; i++) g.fillRect(Math.random() * w, Math.random() * h, 2, 2);

    // top bar — battery
    const bat = Math.ceil(this.battery);
    g.font = '16px monospace';
    g.fillStyle = bat < 20 ? (Math.sin(time * 8) > 0 ? '#ff3b30' : '#7a1d18') : '#9adf9a';
    g.fillText(`${bat}%`, 10, 24);
    g.strokeStyle = g.fillStyle;
    g.strokeRect(52, 10, 30, 15);
    g.fillRect(54, 12, 26 * (bat / 100), 11);
    g.fillRect(82, 14, 3, 7);
    // clock — wrong on purpose at high stress
    g.fillStyle = '#667';
    const hh = s > 0.7 ? '??' : '03';
    const mm = s > 0.7 ? '??' : String(33 + ((time / 60) | 0) % 27).padStart(2, '0');
    g.fillText(`${hh}:${mm}`, 196, 24);

    // objective
    if (this.objective) {
      g.font = '13px monospace';
      g.fillStyle = `rgba(220,220,230,${0.85 - s * 0.3})`;
      g.textAlign = 'center';
      const txt = this._corrupt(this.objective.toUpperCase());
      g.fillText(txt, w / 2, 58, w - 16);
      g.textAlign = 'left';
    }

    // transient message (e.g. "TRUST ME")
    if (this.messageT > 0 && Math.sin(time * 6) > -0.4) {
      g.font = 'bold 22px monospace';
      g.fillStyle = 'rgba(235,230,225,0.95)';
      g.textAlign = 'center';
      g.fillText(this._corrupt(this.message), w / 2, h / 2, w - 12);
      g.textAlign = 'left';
    }

    // stress word
    if (s > 0.45 && Math.random() < 0.4) {
      g.font = '11px monospace';
      g.fillStyle = `rgba(200,60,55,${(s - 0.45) * 1.2})`;
      g.fillText('· WATCHING ·', 84, 96);
    }

    // REC indicator — always on. always.
    g.font = '14px monospace';
    g.fillStyle = Math.sin(time * 3.5) > 0 ? '#ff3b30' : 'rgba(255,59,48,0.25)';
    g.beginPath(); g.arc(18, h - 26, 6, 0, Math.PI * 2); g.fill();
    g.fillStyle = '#bbb';
    g.fillText('REC', 32, h - 21);
    g.fillStyle = '#555';
    g.font = '11px monospace';
    const rt = (time | 0);
    g.fillText(`${String((rt / 3600) | 0).padStart(2, '0')}:${String(((rt / 60) | 0) % 60).padStart(2, '0')}:${String(rt % 60).padStart(2, '0')}`, 72, h - 21);
    // low battery warning
    if (bat < 20 && Math.sin(time * 5) > 0) {
      g.font = 'bold 15px monospace';
      g.fillStyle = '#ff3b30';
      g.textAlign = 'center';
      g.fillText('LOW BATTERY', w / 2, h - 60);
      g.textAlign = 'left';
    }

    // glitch blocks under stress
    if (s > 0.4) {
      const n = ((s - 0.4) * 14) | 0;
      for (let i = 0; i < n; i++) {
        const y = Math.random() * h;
        const data = g.getImageData(0, y, w, 4 + Math.random() * 8);
        g.putImageData(data, (Math.random() - 0.5) * 30 * s, y);
      }
      if (Math.random() < s * 0.15) {
        g.fillStyle = `rgba(${Math.random() < 0.5 ? '255,255,255' : '255,40,40'},0.08)`;
        g.fillRect(0, Math.random() * h, w, 10 + Math.random() * 40);
      }
    }
    this.screenTex.needsUpdate = true;
  }
}
