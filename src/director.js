// PARANOIA — game director: phases, scripted events, puzzles,
// interactions, stress, checkpoints, endings.

import * as THREE from '../lib/three.module.js';
import { WORLD, openCabinDoor } from './world.js';
import { HOUSE, openBasementDoor, openBedroomDoor, openFalseWall } from './house.js';
import { ShadowEntity } from './entity.js';

const HX = 0, HZ = -110;

export class Director {
  constructor(scene, player, phone, audio, postfx) {
    this.scene = scene;
    this.player = player;
    this.phone = phone;
    this.audio = audio;
    this.postfx = postfx;
    this.entities = [];
    this.phase = 0;
    this.stress = 0;
    this.flags = {};
    this.tweens = [];
    this.checkpoint = { x: 0, z: 200, yaw: 0 };
    this.catchCooldown = 0;
    this.codeBuffer = null;
    this.gameTime = 0;
    this.ended = false;
    this._fired = new Set();
    this._buildHUD();
    this._buildKeyMeshes();
  }

  // ---------------------------------------------------------------- HUD

  _buildHUD() {
    const mk = (id, css) => {
      const d = document.createElement('div');
      d.id = id;
      d.style.cssText = `position:absolute;font-family:'Courier New',monospace;pointer-events:none;z-index:20;${css}`;
      document.body.appendChild(d);
      return d;
    };
    this.hudPrompt = mk('hud-prompt',
      'bottom:18%;left:50%;transform:translateX(-50%);color:#cfcfd6;font-size:15px;letter-spacing:3px;text-shadow:0 0 8px #000;');
    this.hudSub = mk('hud-sub',
      'bottom:10%;left:50%;transform:translateX(-50%);color:#9a9aa4;font-size:14px;letter-spacing:2px;text-shadow:0 0 8px #000;text-align:center;width:80%;');
    this.hudBig = mk('hud-big',
      'top:42%;left:50%;transform:translate(-50%,-50%);color:#d8d4cf;font-size:46px;letter-spacing:14px;text-shadow:0 0 22px rgba(200,40,40,0.6);opacity:0;transition:opacity 1.2s;text-align:center;');
    this.hudNote = mk('hud-note', `top:50%;left:50%;transform:translate(-50%,-50%);display:none;
      background:rgba(16,14,10,0.94);border:1px solid #4a4436;color:#cfc9b8;padding:36px 44px;
      font-size:17px;line-height:2;letter-spacing:1px;font-style:italic;text-align:center;`);
    this.hudFade = mk('hud-fade',
      'inset:0;background:#000;opacity:0;transition:opacity 3s;z-index:40;');
    this.hudCredits = mk('hud-credits',
      `inset:0;background:#000;opacity:0;transition:opacity 2s;z-index:45;display:none;
       color:#888;text-align:center;padding-top:24vh;font-size:14px;letter-spacing:4px;line-height:2.6;`);
    this.subT = 0;
  }

  sub(text, seconds = 4) {
    this.hudSub.textContent = text;
    this.subT = seconds;
  }

  big(text, seconds = 3) {
    this.hudBig.textContent = text;
    this.hudBig.style.opacity = '1';
    setTimeout(() => { this.hudBig.style.opacity = '0'; }, seconds * 1000);
  }

  // ------------------------------------------------------------- pickups

  _buildKeyMeshes() {
    // WATER KEY at the fountain rim — in shadow; you need the light to see it
    const keyGroup = new THREE.Group();
    const mat = new THREE.MeshStandardMaterial({ color: 0x6a523a, roughness: 0.55, metalness: 0.85 });
    const shaft = new THREE.Mesh(new THREE.CylinderGeometry(0.012, 0.012, 0.16, 6), mat);
    shaft.rotation.z = Math.PI / 2;
    const bow = new THREE.Mesh(new THREE.TorusGeometry(0.035, 0.01, 5, 8), mat);
    bow.position.x = -0.09;
    const tooth = new THREE.Mesh(new THREE.BoxGeometry(0.03, 0.035, 0.012), mat);
    tooth.position.set(0.07, -0.025, 0);
    keyGroup.add(shaft, bow, tooth);
    keyGroup.position.set(WORLD.fountain.x + 2.35, 0.92, WORLD.fountain.z + 0.6);
    keyGroup.rotation.y = 0.7;
    this.scene.add(keyGroup);
    this.waterKeyMesh = keyGroup;

    // KITCHEN KEY inside drawer (hidden until drawer opened)
    const kk = keyGroup.clone();
    kk.scale.setScalar(0.8);
    kk.position.set(HX + 4.85, 0.62, HZ + 1.47);
    kk.visible = false;
    this.scene.add(kk);
    this.kitchenKeyMesh = kk;
  }

  // ------------------------------------------------------------- phases

  start() {
    this.phase = 1;
    this.postfx.flashBlack = 2.2; // fade in from black
    this.phone.setObjective('Follow the path');
    setTimeout(() => { this.phone.showMessage('TRUST ME', 5); this.audio.uiBeep(); }, 3500);
    this.audio.setWind(1);
    // phase-1 watcher: non-lethal, scripted
    const e0 = new ShadowEntity(this.scene, new THREE.Vector3(WORLD.pathX(120) - 14, 0, 120), [], { lethal: false });
    e0.frozen = true;
    e0.mesh.visible = false;
    this.e0 = e0;
    this.entities.push(e0);
    this.audio.attachEntity(e0.id);
    this.checkpoint = { x: 0, z: 200, yaw: 0 };
  }

  _startPhase2() {
    this.phase = 2;
    this.phone.setObjective('Reach the white house');
    this.sub('The trees open. Something white waits in the distance.', 5);
    this.checkpoint = { x: WORLD.pathX(58), z: 58, yaw: 0 };
    // retire the watcher
    this._tween(this.e0.pos, { x: this.e0.pos.x - 20, z: this.e0.pos.z }, 3, () => {
      this.e0.remove();
    });
    // forest entities
    const e1 = new ShadowEntity(this.scene, new THREE.Vector3(-8, 0, 30), [
      new THREE.Vector3(-6, 0, 40), new THREE.Vector3(2, 0, 5),
      new THREE.Vector3(-4, 0, -35), new THREE.Vector3(-20, 0, -10)
    ], { aggression: 1 });
    const e2 = new ShadowEntity(this.scene, new THREE.Vector3(38, 0, -45), [
      new THREE.Vector3(35, 0, -15), new THREE.Vector3(20, 0, -45),
      new THREE.Vector3(48, 0, -52), new THREE.Vector3(55, 0, -20)
    ], { aggression: 1.05 });
    this.entities.push(e1, e2);
    this.audio.attachEntity(e1.id);
    this.audio.attachEntity(e2.id);
  }

  _startPhase3() {
    this.phase = 3;
    this.phone.setObjective('Enter the house');
    this.checkpoint = { x: 0, z: -88, yaw: 0 };
    const e3 = new ShadowEntity(this.scene, new THREE.Vector3(-18, 0, -96), [
      new THREE.Vector3(-16, 0, -95), new THREE.Vector3(14, 0, -94),
      new THREE.Vector3(20, 0, -122), new THREE.Vector3(-19, 0, -125)
    ], { aggression: 1.15 });
    this.entities.push(e3);
    this.audio.attachEntity(e3.id);
  }

  _enterHouse() {
    this.flags.inHouseOnce = true;
    this.phone.setObjective('Find a way down');
    this.sub('Too quiet.', 4);
    this.checkpoint = { x: HX, z: HZ + 6.5, yaw: 0 };
    this.audio.setWind(0.25);
    // interior stalker — slow, patrols the ground floor
    const e4 = new ShadowEntity(this.scene, new THREE.Vector3(HX + 4, 0, HZ + 2.5), [
      new THREE.Vector3(HX - 2, 0, HZ + 3), new THREE.Vector3(HX + 4, 0, HZ + 3),
      new THREE.Vector3(HX + 4.5, 0, HZ - 2), new THREE.Vector3(HX - 2, 0, HZ - 2.5)
    ], { aggression: 0.9 });
    e4.patrolSpeed = 1.0;
    e4.pursuitSpeed = 2.9;
    this.entities.push(e4);
    this.audio.attachEntity(e4.id);
  }

  // ------------------------------------------------------------- tweens

  _tween(vec, to, dur, onDone) {
    this.tweens.push({ vec, from: { x: vec.x, z: vec.z }, to, t: 0, dur, onDone });
  }

  _runTweens(dt) {
    for (let i = this.tweens.length - 1; i >= 0; i--) {
      const tw = this.tweens[i];
      tw.t += dt;
      const f = Math.min(1, tw.t / tw.dur);
      tw.vec.x = tw.from.x + (tw.to.x - tw.from.x) * f;
      tw.vec.z = tw.from.z + (tw.to.z - tw.from.z) * f;
      if (f >= 1) {
        this.tweens.splice(i, 1);
        if (tw.onDone) tw.onDone();
      }
    }
  }

  // -------------------------------------------------------- script: ph.1

  _fire(key, fn) {
    if (this._fired.has(key)) return;
    this._fired.add(key);
    fn();
  }

  _phase1Script() {
    const z = this.player.pos.z;
    if (z < 172) this._fire('branch', () => {
      this.audio.branchCrack(-0.8);
      // shadow dashes across the path ahead
      const e = this.e0;
      e.mesh.visible = true;
      e.pos.set(WORLD.pathX(z - 18) - 7, 0, z - 18);
      e.visPos.copy(e.pos);
      this._tween(e.pos, { x: WORLD.pathX(z - 18) + 8, z: z - 19 }, 2.4, () => {
        e.mesh.visible = false;
      });
      this.stress += 12;
    });
    if (z < 150) this._fire('steps', () => {
      this.sub('Footsteps. Behind you.', 3.5);
      let n = 0;
      const iv = setInterval(() => {
        this.audio.footstep(0.7, false);
        if (++n > 5) clearInterval(iv);
      }, 480);
      this.flags.whispers = true;
      this.stress += 8;
    });
    if (z < 122) this._fire('flicker', () => {
      this.phone._flickerOff = true;
      this.phone._flickerT = 0.4;
      this.audio.uiBeep(true);
      this.stress += 6;
      this.postfx.spikeGlitch(0.5);
    });
    if (z < 92) this._fire('blocker', () => {
      // the entity stands on the path. it does not attack. it watches.
      const e = this.e0;
      e.mesh.visible = true;
      e.pos.set(WORLD.pathX(74), 0, 74);
      e.visPos.copy(e.pos);
      this.audio.sting(1.1);
      this.stress += 18;
      this.sub('It is standing on the path.', 4);
    });
    if (this._fired.has('blocker') && !this._fired.has('blockerGone')) {
      const e = this.e0;
      e.faceTowards(this.player.pos);
      const d = e.pos.distanceTo(this.player.pos);
      if (d < 3.2 || z < 71) {
        this._fire('blockerGone', () => {
          this._tween(e.pos, { x: e.pos.x - 16, z: e.pos.z - 4 }, 1.6, () => {
            e.mesh.visible = false;
          });
          this.audio.branchCrack(0.9);
          this.postfx.spikeGlitch(0.8);
          this.stress += 10;
        });
      }
    }
    if (z < 62) this._startPhase2();
  }

  _phase2Script() {
    const p = this.player.pos;
    if (p.z < 30) this._fire('p2whisper', () => {
      this.audio.whisper();
      this.sub('Whispers. They are not words.', 4);
    });
    if (p.z < -20) this._fire('p2double', () => {
      // double presence: both entities know roughly where you are
      for (const e of this.entities) {
        if (e.frozen || e.lethal === false) continue;
        e.lastSeenPlayer.set(p.x + (Math.random() - 0.5) * 10, 0, p.z + (Math.random() - 0.5) * 10);
        e.setState('alerted');
      }
      this.audio.sting(0.7);
      this.sub('Two breathing patterns. Neither is yours.', 4.5);
    });
    if (p.z < -85) this._startPhase3();
  }

  _phase3Script() {
    const p = this.player.pos;
    const inHouse = Math.abs(p.x - HX) < 6 && Math.abs(p.z - HZ) < 5;
    if (inHouse && !this.flags.inHouseOnce) this._enterHouse();
    if (this.flags.inHouseOnce && p.y < -2 && !this.flags.basementOnce) {
      this.flags.basementOnce = true;
      this.phone.setObjective('Something is here');
      this.sub('They did not follow you down.', 5);
      this.checkpoint = { x: HX + 5, z: HZ - 4.2, yaw: Math.PI / 2 };
    }
  }

  // --------------------------------------------------------- interaction

  _interactables() {
    const f = this.flags;
    const list = [];
    if (this.phase >= 2 && !f.waterKey) {
      list.push({
        pos: this.waterKeyMesh.position, r: 1.6,
        prompt: this.phone.lightOn ? '[E] TAKE THE IRON KEY' : null,
        needLight: true,
        act: () => {
          f.waterKey = true;
          this.waterKeyMesh.visible = false;
          this.audio.pickup();
          this.phone.showMessage('WATER KEY', 3);
          this.stress += 15;
          this.postfx.spikeGlitch(0.7);
          // taking it wakes something
          for (const e of this.entities) {
            if (e.frozen || !e.lethal) continue;
            if (e.state === 'idle') {
              e.lastSeenPlayer.copy(this.player.pos);
              e.setState('alerted');
            }
          }
        }
      });
    }
    if (this.phase >= 2 && !f.cabinOpen) {
      list.push({
        pos: new THREE.Vector3(WORLD.cabin.x - 2.6, 1.2, WORLD.cabin.z + 0.85), r: 1.7,
        prompt: '[E] KEYPAD',
        act: () => { this._openCodeEntry(); }
      });
    }
    if (f.cabinOpen && !f.fakeCharge) {
      list.push({
        pos: new THREE.Vector3(WORLD.cabin.x + 1.2, 0.9, WORLD.cabin.z - 0.8), r: 1.4,
        prompt: '[E] CHARGE PHONE',
        act: () => {
          f.fakeCharge = true;
          this.audio.uiBeep();
          this.phone.showMessage('CHARGING...', 3);
          setTimeout(() => {
            this.phone.showMessage('NO POWER DETECTED', 4);
            this.audio.uiBeep(true);
            this.postfx.spikeGlitch(0.5);
          }, 3200);
        }
      });
    }
    if (this.flags.inHouseOnce && !f.kitchenKey) {
      list.push({
        pos: HOUSE.p.kitchenDrawer, r: 1.4,
        prompt: '[E] OPEN DRAWER',
        act: () => {
          if (!f.drawerOpen) {
            f.drawerOpen = true;
            HOUSE.drawers[1].position.x += 0.28;
            this.kitchenKeyMesh.visible = true;
            this.kitchenKeyMesh.position.x += 0.2;
            this.audio.branchCrack(0.2);
          } else {
            f.kitchenKey = true;
            this.kitchenKeyMesh.visible = false;
            this.audio.pickup();
            this.phone.showMessage('KITCHEN KEY', 3);
            this.phone.setObjective('The bedroom upstairs');
          }
        }
      });
    }
    if (this.flags.inHouseOnce && !f.bedroomOpen) {
      list.push({
        pos: HOUSE.p.bedroomDoor, r: 1.6,
        prompt: f.kitchenKey ? '[E] UNLOCK BEDROOM' : 'LOCKED — A SMALL KEYHOLE',
        act: () => {
          if (!f.kitchenKey) { this.audio.uiBeep(true); return; }
          f.bedroomOpen = true;
          openBedroomDoor();
          this.audio.unlock();
          this.phone.setObjective('The mirror remembers');
        }
      });
    }
    if (this.flags.inHouseOnce && !f.basementOpen) {
      list.push({
        pos: HOUSE.p.basementDoor, r: 1.6,
        prompt: (f.waterKey || f.mirrorSolved) ? '[E] UNLOCK BASEMENT' : 'LOCKED — THE KEYHOLE IS WATER-WORN',
        act: () => {
          if (!(f.waterKey || f.mirrorSolved)) { this.audio.uiBeep(true); return; }
          f.basementOpen = true;
          openBasementDoor();
          this.audio.unlock();
          this.phone.setObjective('Go down');
        }
      });
    }
    if (f.basementOnce && !f.hiddenOpen && this.player.groundY < -1) {
      list.push({
        pos: HOUSE.p.falseWall, r: 2.2,
        prompt: this._facingFalseWall() ? '[E] THE WALL SOUNDS HOLLOW' : null,
        act: () => {
          if (!this._facingFalseWall()) return;
          f.hiddenOpen = true;
          openFalseWall();
          this.audio.unlock();
          this.audio.sting(0.8);
          this.sub('The wall sinks into the floor.', 4);
        }
      });
    }
    if (f.hiddenOpen && !f.truth) {
      list.push({
        pos: HOUSE.p.oldPhone, r: 1.5,
        prompt: '[E] EXAMINE THE OTHER PHONE',
        act: () => {
          f.truth = true;
          this.postfx.flashWhite = 2.5;
          this.phone.recharge(100);
          this.stress = 0;
          this.audio.sting(0.6);
          this.phone.showMessage('YOU ARE THE RECORDING', 6);
          this.phone.setObjective('Read the note');
        }
      });
    }
    if (f.truth && !f.noteRead) {
      list.push({
        pos: HOUSE.p.note, r: 1.5,
        prompt: '[E] READ THE NOTE',
        act: () => { this._showNote(); }
      });
    }
    return list;
  }

  _facingFalseWall() {
    // need the light on, close, aimed at the west wall
    if (!this.phone.lightOn) return false;
    const fwd = this.player.forwardDir();
    return fwd.x < -0.8;
  }

  _showNote() {
    this.flags.notePending = true;
    this.hudNote.innerHTML =
      'El smartphone es una puerta.<br><br>Confía en él. O desconfía.<br><br>' +
      'Pero decide antes de que<br>ellos decidan por ti.<br><br>' +
      '<span style="font-size:12px;color:#7a7468">[E] CLOSE</span>';
    this.hudNote.style.display = 'block';
  }

  _closeNote() {
    this.flags.notePending = false;
    this.flags.noteRead = true;
    this.hudNote.style.display = 'none';
    this.phone.setObjective('Decide');
    this.sub('[E] TRUST THE PHONE      [Q] REFUSE', 9999);
  }

  // ---------------------------------------------------------- code entry

  _openCodeEntry() {
    this.codeBuffer = '';
    this.hudPrompt.textContent = 'ENTER CODE: ____';
  }

  handleKey(code, key) {
    if (this.ended) return true;
    if (this.flags.notePending && (code === 'KeyE' || code === 'Escape')) {
      this._closeNote();
      return true;
    }
    if (this.flags.noteRead && !this.flags.notePending) {
      if (code === 'KeyE') { this._ending('trust'); return true; }
      if (code === 'KeyQ') { this._ending('refuse'); return true; }
    }
    if (this.codeBuffer !== null) {
      if (code === 'Escape') { this.codeBuffer = null; return true; }
      const d = key >= '0' && key <= '9' ? key : null;
      if (d) {
        this.codeBuffer += d;
        this.audio.uiBeep();
        if (this.codeBuffer.length >= 4) {
          if (this.codeBuffer === '2741') {
            this.flags.cabinOpen = true;
            openCabinDoor();
            this.audio.unlock();
            this.sub('The cabin accepts you.', 4);
          } else {
            this.audio.uiBeep(true);
            this.sub('Nothing.', 2);
          }
          this.codeBuffer = null;
        }
      }
      return true;
    }
    return false;
  }

  tryInteract() {
    if (this.ended || this.flags.notePending) return;
    const list = this._interactables();
    const p = this.player.pos;
    for (const it of list) {
      const d = Math.hypot(it.pos.x - p.x, it.pos.z - p.z);
      const dy = Math.abs((it.pos.y ?? 0) - (p.y + 1.0));
      if (d < it.r && dy < 2.4) {
        if (it.needLight && !this.phone.lightOn) continue;
        it.act();
        return;
      }
    }
  }

  // ------------------------------------------------------------- endings

  _ending(which) {
    if (this.ended) return;
    this.ended = true;
    this.hudSub.textContent = '';
    this.hudPrompt.textContent = '';
    this.player.enabled = false;
    if (which === 'trust') {
      this.phone.showMessage('TRUST ME', 30);
      this.audio.setWind(0);
      const fade = document.getElementById('hud-fade');
      fade.style.background = '#fff';
      fade.style.transition = 'opacity 6s';
      fade.style.opacity = '1';
      setTimeout(() => this._credits('TRUST ME',
        'You stepped through the door it offered.<br>The recording continues. It always was you.'), 7000);
    } else {
      this.phone.battery = 0;
      this.phone.lightOn = false;
      this.audio.scare();
      this.audio.setWind(0);
      const fade = document.getElementById('hud-fade');
      fade.style.background = '#000';
      fade.style.transition = 'opacity 2.5s';
      fade.style.opacity = '1';
      setTimeout(() => {
        this.hudBig.style.zIndex = '46';
        this.big('THEY SEE YOU NOW', 5);
        this.audio.sting(1.8);
      }, 3000);
      setTimeout(() => this._credits('REJECT TRUTH',
        'The screen died. The dark is full of breathing.<br>They no longer need the camera to watch you.'), 9000);
    }
  }

  _credits(title, blurb) {
    const c = this.hudCredits;
    c.innerHTML = `
      <div style="font-size:30px;letter-spacing:16px;color:#c8c4bf;margin-bottom:8px">PARANOIA</div>
      <div style="font-size:13px;color:#6a4040;margin-bottom:40px">ENDING — ${title}</div>
      <div style="font-size:13px;color:#777;margin-bottom:50px">${blurb}</div>
      <div>a game by <span style="color:#aaa">vamp9</span></div>
      <div style="font-size:11px;color:#444">design · code · sound — procedural, like everything else you saw</div>
      <div style="margin-top:50px;font-size:12px;color:#555">[R] RESTART&nbsp;&nbsp;&nbsp;&nbsp;[ESC] QUIT</div>`;
    c.style.display = 'block';
    requestAnimationFrame(() => { c.style.opacity = '1'; });
    const onKey = (e) => {
      if (e.code === 'KeyR') location.reload();
    };
    window.addEventListener('keydown', onKey);
  }

  // ------------------------------------------------------------- update

  update(dt, time) {
    if (this.ended) return;
    this.gameTime += dt;
    this.catchCooldown = Math.max(0, this.catchCooldown - dt);
    this._runTweens(dt);

    if (this.phase === 1) this._phase1Script();
    else if (this.phase === 2) this._phase2Script();
    if (this.phase >= 3) this._phase3Script();

    // ---- entity updates
    const onCatch = (e) => {
      if (this.catchCooldown > 0) return;
      this.catchCooldown = 3;
      this.postfx.flashBlack = 1.6;
      this.audio.scare();
      this.stress = 85;
      this.phone.battery = Math.max(5, this.phone.battery - 8);
      const cp = this.checkpoint;
      this.player.teleport(cp.x, cp.z, cp.yaw);
      for (const en of this.entities) {
        if (en.frozen || !en.lethal) continue;
        en.pos.copy(en.home);
        en.visPos.copy(en.home);
        en.setState('idle');
      }
      this.big('IT TOUCHED YOU', 2.2);
    };
    const onLitClose = () => {
      this.stress += 6;
      this.postfx.spikeGlitch(0.4);
    };
    for (const e of this.entities) {
      if (e.removed) continue;
      e.update(dt, time, this.player, this.phone, this.entities, onCatch, onLitClose);
      if (!e.frozen && e.state !== 'idle') e.faceTowards(this.player.pos);
      this.audio.updateEntity(e.id, e.pos.x, 1.6, e.pos.z, e.state, time);
    }

    // ---- stress model
    let nearest = Infinity, pursuit = false;
    for (const e of this.entities) {
      if (e.frozen || e.removed) continue;
      const d = e.pos.distanceTo(this.player.pos);
      nearest = Math.min(nearest, d);
      if ((e.state === 'pursuing' || e.state === 'hunting') && d < 30) pursuit = true;
    }
    if (pursuit) this.stress += 9 * dt;
    else if (nearest < 9) this.stress += 4.5 * dt;
    else if (nearest < 16) this.stress += 1.5 * dt;
    else {
      // calm decay; the light is comfort
      this.stress -= (2.0 + (this.phone.lightOn ? 0.8 : 0)) * dt;
      this.stress += 0.25 * dt; // baseline paranoia never sleeps
    }
    if (this.phone.battery < 20) this.stress += 1.2 * dt;
    const inCabin = this.flags.cabinOpen &&
      Math.abs(this.player.pos.x - WORLD.cabin.x) < 2.4 &&
      Math.abs(this.player.pos.z - WORLD.cabin.z) < 1.9;
    if (inCabin) this.stress -= 5 * dt;
    if (this.player.groundY < -1) this.stress += 0.8 * dt; // the basement presses on you
    this.stress = Math.max(0, Math.min(100, this.stress));

    // ---- puzzles (passive)
    this._signCode();
    this._mirror(dt, time);

    // ---- HUD prompt
    if (this.codeBuffer !== null) {
      this.hudPrompt.textContent = 'ENTER CODE: ' + this.codeBuffer.padEnd(4, '_').split('').join(' ');
    } else if (!this.flags.noteRead) {
      let prompt = '';
      const p = this.player.pos;
      for (const it of this._interactables()) {
        const d = Math.hypot(it.pos.x - p.x, it.pos.z - p.z);
        const dy = Math.abs((it.pos.y ?? 0) - (p.y + 1.0));
        if (d < it.r && dy < 2.4 && it.prompt) { prompt = it.prompt; break; }
      }
      this.hudPrompt.textContent = prompt;
    }
    this.subT -= dt;
    if (this.subT <= 0 && !this.flags.noteRead) this.hudSub.textContent = '';

    // ---- audio context
    const nearFountain = this.player.pos.distanceTo(WORLD.fountain) < 12;
    this.audio.tick(time, {
      whispers: this.flags.whispers && this.player.groundY > -1,
      drips: nearFountain || this.player.groundY < -1
    });
    this.audio.setStress(this.stress);

    // entities near → battery pressure flag
    this.entitiesNear = pursuit && nearest < 14;
  }

  _signCode() {
    const mesh = WORLD.signCodeMesh;
    if (!mesh) return;
    let target = 0;
    if (this.phone.lightOn && this.phase >= 2) {
      const p = this.player.pos;
      const d = Math.hypot(WORLD.sign.x - p.x, WORLD.sign.z - p.z);
      if (d > 1.2 && d < 5.5) {
        const to = new THREE.Vector3(WORLD.sign.x - p.x, 0, WORLD.sign.z - p.z).normalize();
        const a = this.player.forwardDir().dot(to);
        // must stand south of the sign and aim very precisely
        if (a > 0.975 && p.z > WORLD.sign.z + 1.0) {
          target = Math.min(1, (a - 0.975) / 0.02);
        }
      }
    }
    mesh.material.opacity += (target - mesh.material.opacity) * 0.08;
    if (mesh.material.opacity > 0.6 && !this.flags.codeSeen) {
      this.flags.codeSeen = true;
      this.audio.uiBeep();
      this.sub('2 · 7 · 4 · 1', 6);
    }
  }

  _mirror(dt, time) {
    if (!this.flags.bedroomOpen || this.flags.mirrorSolved) return;
    if (this.player.groundY < 2.5) return;
    const p = this.player.pos;
    const spot = HOUSE.p.mirrorSpot;
    const dSpot = Math.hypot(spot.x - p.x, spot.z - p.z);
    const fig = HOUSE.mirrorFigure;
    // the figure fades in as you approach the mirror
    const dMirror = Math.hypot(HOUSE.p.mirrorPos.x - p.x, HOUSE.p.mirrorPos.z - p.z);
    const vis = dMirror < 4 ? Math.min(0.85, (4 - dMirror) / 3) : 0;
    fig.mat.opacity += (vis - fig.mat.opacity) * Math.min(1, dt * 3);
    if (vis > 0.4 && !this.flags.mirrorScare) {
      this.flags.mirrorScare = true;
      this.audio.sting(1.2);
      this.stress += 22;
      this.postfx.spikeGlitch(1);
      this.sub('That is not your reflection.', 4);
    }
    // solve: stand on the rug, face the mirror (east) precisely, hold 2s
    if (dSpot < 0.9) {
      const fwd = this.player.forwardDir();
      if (fwd.x > 0.94) {
        this.mirrorHold = (this.mirrorHold || 0) + dt;
        if (this.mirrorHold > 0.4) {
          this.hudPrompt.textContent = 'HOLD STILL — THE REFLECTION IS ALIGNING';
        }
        if (this.mirrorHold > 2.2) {
          this.flags.mirrorSolved = true;
          this.audio.unlock();
          this.audio.sting(0.7);
          this.sub('Something unlocked below.', 5);
          this.phone.setObjective('Go down');
          fig.mat.opacity = 0;
        }
      } else this.mirrorHold = 0;
    } else this.mirrorHold = 0;
  }
}
