// PARANOIA — custom post-processing (no jsm deps)
// Renders the scene to a target, then draws a fullscreen quad with
// grain / vignette / chromatic aberration / glitch driven by stress.

import * as THREE from '../lib/three.module.js';

const VERT = /* glsl */`
varying vec2 vUv;
void main() {
  vUv = uv;
  gl_Position = vec4(position.xy, 0.0, 1.0);
}`;

const FRAG = /* glsl */`
precision highp float;
varying vec2 vUv;
uniform sampler2D tDiffuse;
uniform float uTime;
uniform float uStress;     // 0..1
uniform float uGlitch;     // 0..1 momentary spikes
uniform float uFlashWhite; // 0..1
uniform float uFlashBlack; // 0..1
uniform float uBattery;    // 0..1 (dims & flickers when low)

float rand(vec2 co) {
  return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
  vec2 uv = vUv;
  float s = uStress;
  float g = uGlitch;

  // glitch: horizontal block displacement
  if (g > 0.01) {
    float block = floor(uv.y * 24.0);
    float shift = (rand(vec2(block, floor(uTime * 18.0))) - 0.5) * 0.12 * g;
    if (rand(vec2(block, floor(uTime * 7.0))) > 0.72) uv.x += shift;
  }

  // chromatic aberration grows with stress
  float ca = 0.0015 + s * 0.006 + g * 0.01;
  vec2 dir = uv - 0.5;
  vec3 col;
  col.r = texture2D(tDiffuse, uv + dir * ca).r;
  col.g = texture2D(tDiffuse, uv).g;
  col.b = texture2D(tDiffuse, uv - dir * ca).b;

  // film grain
  float grain = (rand(uv * vec2(1920.0, 1080.0) + fract(uTime) * 100.0) - 0.5);
  col += grain * (0.045 + s * 0.16 + g * 0.1);

  // desaturate with stress
  float lum = dot(col, vec3(0.299, 0.587, 0.114));
  col = mix(col, vec3(lum), s * 0.35);

  // scanlines at high stress
  col -= sin(uv.y * 900.0 + uTime * 12.0) * 0.5 * (s * s * 0.08 + g * 0.06);

  // vignette
  float vig = smoothstep(1.15 - s * 0.35, 0.25, length(dir) * 1.4);
  col *= mix(0.35, 1.0, vig);

  // low battery dimming + flicker
  float bat = clamp(uBattery, 0.0, 1.0);
  if (bat < 0.2) {
    float fl = step(0.5, rand(vec2(floor(uTime * 9.0), 1.0))) * (0.2 - bat) * 2.5;
    col *= 1.0 - fl * 0.6;
  }

  // flashes
  col = mix(col, vec3(1.0), clamp(uFlashWhite, 0.0, 1.0));
  col = mix(col, vec3(0.0), clamp(uFlashBlack, 0.0, 1.0));

  gl_FragColor = vec4(col, 1.0);
}`;

export class PostFX {
  constructor(renderer) {
    this.renderer = renderer;
    const size = renderer.getSize(new THREE.Vector2());
    this.target = new THREE.WebGLRenderTarget(size.x, size.y, {
      samples: 2
    });
    this.scene = new THREE.Scene();
    this.camera = new THREE.OrthographicCamera(-1, 1, 1, -1, 0, 1);
    this.material = new THREE.ShaderMaterial({
      vertexShader: VERT,
      fragmentShader: FRAG,
      uniforms: {
        tDiffuse: { value: this.target.texture },
        uTime: { value: 0 },
        uStress: { value: 0 },
        uGlitch: { value: 0 },
        uFlashWhite: { value: 0 },
        uFlashBlack: { value: 0 },
        uBattery: { value: 1 }
      },
      depthTest: false,
      depthWrite: false
    });
    const quad = new THREE.Mesh(new THREE.PlaneGeometry(2, 2), this.material);
    quad.frustumCulled = false;
    this.scene.add(quad);
    this.glitch = 0;
    this.flashWhite = 0;
    this.flashBlack = 0;
  }

  resize(w, h) {
    this.target.setSize(w, h);
  }

  spikeGlitch(amount = 1) {
    this.glitch = Math.min(1, this.glitch + amount);
  }

  render(scene, camera, time, dt, stress, battery) {
    this.glitch = Math.max(0, this.glitch - dt * 1.8);
    this.flashWhite = Math.max(0, this.flashWhite - dt * 0.7);
    this.flashBlack = Math.max(0, this.flashBlack - dt * 1.2);
    const u = this.material.uniforms;
    u.uTime.value = time;
    u.uStress.value = stress / 100;
    u.uGlitch.value = this.glitch + (stress > 80 ? (stress - 80) / 100 : 0);
    u.uFlashWhite.value = this.flashWhite;
    u.uFlashBlack.value = this.flashBlack;
    u.uBattery.value = battery / 100;

    this.renderer.setRenderTarget(this.target);
    this.renderer.render(scene, camera);
    this.renderer.setRenderTarget(null);
    this.renderer.render(this.scene, this.camera);
  }
}
