// PARANOIA — procedural audio engine (WebAudio, no external assets)
// All sounds are synthesized: wind, footsteps, breathing, whispers,
// entity sounds, heartbeat, stings. Stress couples into pitch/distortion.

export class AudioManager {
  constructor() {
    this.ctx = null;
    this.started = false;
    this.stress = 0;
    this.entityPanners = new Map();
    this._stepFlip = false;
    this._lastBreath = 0;
    this._lastWhisper = 0;
    this._lastHeart = 0;
    this._lastDrip = 0;
  }

  start() {
    if (this.started) return;
    const ctx = new AudioContext();
    this.ctx = ctx;
    this.master = ctx.createGain();
    this.master.gain.value = 0.9;

    // Distortion stage (driven by stress)
    this.shaper = ctx.createWaveShaper();
    this._setDrive(0);
    this.shaper.connect(this.master);
    this.master.connect(ctx.destination);

    // Noise buffer shared by everything
    const len = ctx.sampleRate * 2;
    const buf = ctx.createBuffer(1, len, ctx.sampleRate);
    const d = buf.getChannelData(0);
    for (let i = 0; i < len; i++) d[i] = Math.random() * 2 - 1;
    this.noiseBuf = buf;

    this._startWind();
    this.started = true;
  }

  _setDrive(amount) {
    // amount 0..1 -> waveshaper curve from linear to soft clip
    const n = 256, curve = new Float32Array(n);
    const k = 1 + amount * 18;
    for (let i = 0; i < n; i++) {
      const x = (i / (n - 1)) * 2 - 1;
      curve[i] = Math.tanh(k * x) / Math.tanh(k);
    }
    this.shaper.curve = curve;
  }

  _noiseSrc(rate = 1) {
    const s = this.ctx.createBufferSource();
    s.buffer = this.noiseBuf;
    s.loop = true;
    s.playbackRate.value = rate;
    return s;
  }

  _startWind() {
    const ctx = this.ctx;
    const src = this._noiseSrc(0.5);
    const bp = ctx.createBiquadFilter();
    bp.type = 'bandpass'; bp.frequency.value = 280; bp.Q.value = 0.7;
    const g = ctx.createGain();
    g.gain.value = 0.0;
    // slow LFO for gusts
    const lfo = ctx.createOscillator();
    lfo.frequency.value = 0.07;
    const lfoG = ctx.createGain(); lfoG.gain.value = 90;
    lfo.connect(lfoG); lfoG.connect(bp.frequency);
    const lfo2 = ctx.createOscillator(); lfo2.frequency.value = 0.11;
    const lfo2G = ctx.createGain(); lfo2G.gain.value = 0.035;
    lfo2.connect(lfo2G); lfo2G.connect(g.gain);
    src.connect(bp); bp.connect(g); g.connect(this.shaper);
    src.start(); lfo.start(); lfo2.start();
    this.windGain = g;
    g.gain.setTargetAtTime(0.09, ctx.currentTime, 2);
  }

  setWind(level) { // 0..1
    if (!this.started) return;
    this.windGain.gain.setTargetAtTime(0.02 + level * 0.1, this.ctx.currentTime, 1.5);
  }

  setStress(s) {
    this.stress = s;
    if (!this.started) return;
    this._setDrive(Math.max(0, (s - 55) / 45) * 0.8);
  }

  // ---- one-shots -------------------------------------------------------

  footstep(speedFactor = 1, hard = false) {
    if (!this.started) return;
    const ctx = this.ctx;
    this._stepFlip = !this._stepFlip;
    const src = this._noiseSrc(hard ? 1.5 : 0.85 + (this._stepFlip ? 0.07 : 0));
    const f = ctx.createBiquadFilter();
    f.type = 'lowpass';
    f.frequency.value = hard ? 1800 : 700;
    const g = ctx.createGain();
    const t = ctx.currentTime;
    g.gain.setValueAtTime(0.0, t);
    g.gain.linearRampToValueAtTime(0.14 * speedFactor, t + 0.012);
    g.gain.exponentialRampToValueAtTime(0.001, t + (hard ? 0.13 : 0.19));
    src.connect(f); f.connect(g); g.connect(this.shaper);
    src.start(t); src.stop(t + 0.25);
  }

  branchCrack(pan = -0.7) {
    if (!this.started) return;
    const ctx = this.ctx, t = ctx.currentTime;
    const src = this._noiseSrc(2.2);
    const f = ctx.createBiquadFilter();
    f.type = 'highpass'; f.frequency.value = 900;
    const g = ctx.createGain();
    g.gain.setValueAtTime(0.5, t);
    g.gain.exponentialRampToValueAtTime(0.001, t + 0.09);
    const p = ctx.createStereoPanner(); p.pan.value = pan;
    src.connect(f); f.connect(g); g.connect(p); p.connect(this.shaper);
    src.start(t); src.stop(t + 0.12);
  }

  whisper() {
    if (!this.started) return;
    const ctx = this.ctx, t = ctx.currentTime;
    const dur = 0.9 + Math.random() * 1.4;
    const src = this._noiseSrc(0.6 + Math.random() * 0.3);
    const f = ctx.createBiquadFilter();
    f.type = 'bandpass'; f.Q.value = 6;
    f.frequency.setValueAtTime(900 + Math.random() * 600, t);
    // formant wobble: makes it sound like unintelligible speech
    for (let i = 0; i < 8; i++) {
      f.frequency.linearRampToValueAtTime(700 + Math.random() * 1500, t + (dur * i) / 8);
    }
    const g = ctx.createGain();
    g.gain.setValueAtTime(0, t);
    g.gain.linearRampToValueAtTime(0.05 + this.stress * 0.0006, t + dur * 0.3);
    g.gain.linearRampToValueAtTime(0, t + dur);
    const p = ctx.createStereoPanner();
    p.pan.value = Math.random() * 2 - 1;
    src.connect(f); f.connect(g); g.connect(p); p.connect(this.shaper);
    src.start(t); src.stop(t + dur + 0.1);
  }

  drip() {
    if (!this.started) return;
    const ctx = this.ctx, t = ctx.currentTime;
    const o = ctx.createOscillator();
    o.type = 'sine';
    o.frequency.setValueAtTime(1900, t);
    o.frequency.exponentialRampToValueAtTime(500, t + 0.09);
    const g = ctx.createGain();
    g.gain.setValueAtTime(0.07, t);
    g.gain.exponentialRampToValueAtTime(0.001, t + 0.25);
    o.connect(g); g.connect(this.shaper);
    o.start(t); o.stop(t + 0.3);
  }

  uiBeep(low = false) {
    if (!this.started) return;
    const ctx = this.ctx, t = ctx.currentTime;
    const o = ctx.createOscillator();
    o.type = 'square';
    o.frequency.value = low ? 320 : 880;
    const g = ctx.createGain();
    g.gain.setValueAtTime(0.045, t);
    g.gain.exponentialRampToValueAtTime(0.001, t + 0.12);
    o.connect(g); g.connect(this.shaper);
    o.start(t); o.stop(t + 0.14);
  }

  pickup() {
    if (!this.started) return;
    const ctx = this.ctx, t = ctx.currentTime;
    [620, 930].forEach((fr, i) => {
      const o = ctx.createOscillator();
      o.type = 'triangle'; o.frequency.value = fr;
      const g = ctx.createGain();
      g.gain.setValueAtTime(0.0, t + i * 0.07);
      g.gain.linearRampToValueAtTime(0.06, t + i * 0.07 + 0.02);
      g.gain.exponentialRampToValueAtTime(0.001, t + i * 0.07 + 0.2);
      o.connect(g); g.connect(this.shaper);
      o.start(t + i * 0.07); o.stop(t + i * 0.07 + 0.25);
    });
  }

  unlock() {
    if (!this.started) return;
    const ctx = this.ctx, t = ctx.currentTime;
    const src = this._noiseSrc(1.1);
    const f = ctx.createBiquadFilter();
    f.type = 'bandpass'; f.frequency.value = 2400; f.Q.value = 9;
    const g = ctx.createGain();
    g.gain.setValueAtTime(0.001, t);
    g.gain.exponentialRampToValueAtTime(0.25, t + 0.03);
    g.gain.exponentialRampToValueAtTime(0.001, t + 0.3);
    src.connect(f); f.connect(g); g.connect(this.shaper);
    src.start(t); src.stop(t + 0.35);
    const o = ctx.createOscillator();
    o.type = 'sine'; o.frequency.value = 140;
    const g2 = ctx.createGain();
    g2.gain.setValueAtTime(0.15, t + 0.05);
    g2.gain.exponentialRampToValueAtTime(0.001, t + 0.45);
    o.connect(g2); g2.connect(this.shaper);
    o.start(t + 0.05); o.stop(t + 0.5);
  }

  sting(intensity = 1) {
    if (!this.started) return;
    const ctx = this.ctx, t = ctx.currentTime;
    for (let i = 0; i < 4; i++) {
      const o = ctx.createOscillator();
      o.type = 'sawtooth';
      o.frequency.value = 55 * (i + 1) * (1 + Math.random() * 0.06);
      o.detune.value = (Math.random() - 0.5) * 90;
      const g = ctx.createGain();
      g.gain.setValueAtTime(0.001, t);
      g.gain.exponentialRampToValueAtTime(0.12 * intensity, t + 0.4);
      g.gain.exponentialRampToValueAtTime(0.001, t + 2.2);
      o.connect(g); g.connect(this.shaper);
      o.start(t); o.stop(t + 2.4);
    }
  }

  scare() {
    if (!this.started) return;
    const ctx = this.ctx, t = ctx.currentTime;
    const src = this._noiseSrc(0.7);
    const g = ctx.createGain();
    g.gain.setValueAtTime(0.5, t);
    g.gain.exponentialRampToValueAtTime(0.001, t + 0.8);
    src.connect(g); g.connect(this.master); // bypass shaper: raw hit
    src.start(t); src.stop(t + 0.9);
    this.sting(1.6);
  }

  // ---- positional entity audio ----------------------------------------

  attachEntity(id) {
    if (!this.started) return;
    const ctx = this.ctx;
    const panner = ctx.createPanner();
    panner.panningModel = 'HRTF';
    panner.distanceModel = 'exponential';
    panner.refDistance = 2;
    panner.rolloffFactor = 1.4;
    panner.connect(this.shaper);

    // low irregular breath: 90Hz osc, amplitude-modulated by random envelope
    const o = ctx.createOscillator();
    o.type = 'sine'; o.frequency.value = 85 + Math.random() * 30;
    const sub = ctx.createOscillator();
    sub.type = 'triangle'; sub.frequency.value = 47;
    const g = ctx.createGain(); g.gain.value = 0;
    o.connect(g); sub.connect(g); g.connect(panner);
    o.start(); sub.start();

    const st = { panner, gain: g, nextBreath: 0, clickT: 0 };
    this.entityPanners.set(id, st);
    return st;
  }

  updateEntity(id, x, y, z, state, time) {
    const st = this.entityPanners.get(id);
    if (!st) return;
    const ctx = this.ctx;
    st.panner.positionX.setTargetAtTime(x, ctx.currentTime, 0.1);
    st.panner.positionY.setTargetAtTime(y, ctx.currentTime, 0.1);
    st.panner.positionZ.setTargetAtTime(z, ctx.currentTime, 0.1);
    // irregular non-human breathing
    if (time > st.nextBreath) {
      const aggressive = state === 'pursuing' || state === 'hunting';
      const t = ctx.currentTime;
      const peak = aggressive ? 0.5 : 0.25;
      st.gain.gain.cancelScheduledValues(t);
      st.gain.gain.setValueAtTime(st.gain.gain.value, t);
      st.gain.gain.linearRampToValueAtTime(peak, t + 0.25 + Math.random() * 0.3);
      st.gain.gain.linearRampToValueAtTime(0.02, t + 0.9 + Math.random() * 0.6);
      st.nextBreath = time + (aggressive ? 0.9 : 2.0) + Math.random() * (aggressive ? 0.5 : 2.5);
    }
    // articulation clicks while moving
    if ((state === 'pursuing' || state === 'hunting' || state === 'alerted') && time > st.clickT) {
      const t = ctx.currentTime;
      const src = this._noiseSrc(3.5);
      const f = ctx.createBiquadFilter();
      f.type = 'highpass'; f.frequency.value = 2500;
      const cg = ctx.createGain();
      cg.gain.setValueAtTime(0.4, t);
      cg.gain.exponentialRampToValueAtTime(0.001, t + 0.04);
      src.connect(f); f.connect(cg); cg.connect(st.panner);
      src.start(t); src.stop(t + 0.05);
      st.clickT = time + 0.15 + Math.random() * 0.5;
    }
  }

  updateListener(cam) {
    if (!this.started) return;
    const l = this.ctx.listener, t = this.ctx.currentTime;
    const p = cam.position;
    const fwd = { x: 0, y: 0, z: -1 };
    const m = cam.matrixWorld.elements;
    const fx = -m[8], fy = -m[9], fz = -m[10];
    const ux = m[4], uy = m[5], uz = m[6];
    if (l.positionX) {
      l.positionX.setTargetAtTime(p.x, t, 0.05);
      l.positionY.setTargetAtTime(p.y, t, 0.05);
      l.positionZ.setTargetAtTime(p.z, t, 0.05);
      l.forwardX.setTargetAtTime(fx, t, 0.05);
      l.forwardY.setTargetAtTime(fy, t, 0.05);
      l.forwardZ.setTargetAtTime(fz, t, 0.05);
      l.upX.setTargetAtTime(ux, t, 0.05);
      l.upY.setTargetAtTime(uy, t, 0.05);
      l.upZ.setTargetAtTime(uz, t, 0.05);
    }
  }

  // ---- continuous ambience tick ----------------------------------------

  tick(time, ctx2) {
    if (!this.started) return;
    const s = this.stress;
    // player breathing — faster with stress
    const breathGap = 4.2 - (s / 100) * 2.6;
    if (time - this._lastBreath > breathGap) {
      this._lastBreath = time;
      const ctx = this.ctx, t = ctx.currentTime;
      const src = this._noiseSrc(0.45);
      const f = ctx.createBiquadFilter();
      f.type = 'bandpass'; f.frequency.value = 600; f.Q.value = 1.2;
      const g = ctx.createGain();
      const vol = 0.012 + (s / 100) * 0.05;
      g.gain.setValueAtTime(0, t);
      g.gain.linearRampToValueAtTime(vol, t + 0.5);
      g.gain.linearRampToValueAtTime(0, t + 1.3);
      src.connect(f); f.connect(g); g.connect(this.master);
      src.start(t); src.stop(t + 1.4);
    }
    // heartbeat when stressed
    if (s > 40) {
      const gap = 1.1 - (s / 100) * 0.55;
      if (time - this._lastHeart > gap) {
        this._lastHeart = time;
        const ctx = this.ctx;
        [0, 0.16].forEach((off, i) => {
          const t = ctx.currentTime + off;
          const o = ctx.createOscillator();
          o.type = 'sine'; o.frequency.value = i === 0 ? 55 : 48;
          const g = ctx.createGain();
          g.gain.setValueAtTime(((s - 40) / 60) * (i === 0 ? 0.22 : 0.14), t);
          g.gain.exponentialRampToValueAtTime(0.001, t + 0.14);
          o.connect(g); g.connect(this.master);
          o.start(t); o.stop(t + 0.16);
        });
      }
    }
    // ambient whispers driven by director context
    if (ctx2 && ctx2.whispers && time - this._lastWhisper > 7 + Math.random() * 14) {
      this._lastWhisper = time;
      this.whisper();
    }
    // drips near fountain / in basement
    if (ctx2 && ctx2.drips && time - this._lastDrip > 4 + Math.random() * 3) {
      this._lastDrip = time;
      this.drip();
    }
  }
}
