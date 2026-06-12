// PARANOIA — entry point
// a game by vamp9. you are being observed.

import * as THREE from '../lib/three.module.js';
import { buildWorld, updateEyes, WORLD } from './world.js';
import { buildHouse } from './house.js';
import { Player } from './player.js';
import { Phone } from './phone.js';
import { AudioManager } from './audio.js';
import { PostFX } from './postfx.js';
import { Director } from './director.js';

const canvas = document.getElementById('game');
const renderer = new THREE.WebGLRenderer({ canvas, antialias: false, powerPreference: 'high-performance' });
renderer.setSize(window.innerWidth, window.innerHeight);
renderer.setPixelRatio(Math.min(window.devicePixelRatio, 1.5));
renderer.shadowMap.enabled = true;
renderer.shadowMap.type = THREE.PCFShadowMap;
renderer.toneMapping = THREE.ACESFilmicToneMapping;
renderer.toneMappingExposure = 1.05;

const scene = new THREE.Scene();
const camera = new THREE.PerspectiveCamera(72, window.innerWidth / window.innerHeight, 0.05, 600);
scene.add(camera);

buildWorld(scene);
buildHouse(scene);

const player = new Player(camera, canvas);
const phone = new Phone(camera);
const audio = new AudioManager();
const postfx = new PostFX(renderer);
const director = new Director(scene, player, phone, audio, postfx);

player.onStep = (sprinting) => {
  const indoors = player.groundY < -1 ||
    (Math.abs(player.pos.x) < 6 && Math.abs(player.pos.z + 110) < 5);
  audio.footstep(sprinting ? 1.25 : 0.9, indoors);
};

window.addEventListener('keydown', (e) => {
  if (director.handleKey(e.code, e.key)) return;
  if (!player.enabled) return;
  if (e.code === 'KeyF') {
    phone.toggleLight();
    audio.uiBeep(phone.lightOn ? false : true);
  }
  if (e.code === 'KeyE') director.tryInteract();
});

window.addEventListener('resize', () => {
  renderer.setSize(window.innerWidth, window.innerHeight);
  camera.aspect = window.innerWidth / window.innerHeight;
  camera.updateProjectionMatrix();
  postfx.resize(window.innerWidth, window.innerHeight);
});

// ---- menu --------------------------------------------------------------
document.getElementById('btn-start').addEventListener('click', () => {
  document.getElementById('menu').classList.add('hidden');
  audio.start();
  player.enabled = true;
  player.teleport(0, 200, 0);
  canvas.requestPointerLock();
  director.start();
});

// debug handle (used by automated tests; harmless in release)
window.__G = { player, phone, director, audio, scene, camera, WORLD };

// ---- main loop ----------------------------------------------------------
const clock = new THREE.Clock();
let time = 0;

function loop() {
  requestAnimationFrame(loop);
  const dt = Math.min(0.05, clock.getDelta());
  time += dt;

  player.update(dt, director.stress);
  director.update(dt, time);
  const indoor = player.groundY < -1 ||
    (Math.abs(player.pos.x) < 6.5 && Math.abs(player.pos.z + 110) < 5.5) ||
    (Math.abs(player.pos.x - 60) < 3 && Math.abs(player.pos.z + 30) < 2.4);
  phone.update(dt, time, player.moving, player.sprinting, director.stress, director.entitiesNear, indoor);
  updateEyes(time, dt, director.stress);
  audio.updateListener(camera);

  postfx.render(scene, camera, time, dt, director.stress, phone.battery);
}
loop();
