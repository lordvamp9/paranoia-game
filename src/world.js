// PARANOIA — world generation (all procedural, low-poly)
// Layout (meters):
//   spawn (0, 200) → winding path south→north (z 200 → 60)  [PHASE 1]
//   open forest z 60 → -85: fountain(-35,20), sign(10,-10), cabin(60,-30) [PHASE 2]
//   clearing z -85 → -140, white house at (0,-110)           [PHASE 3]

import * as THREE from '../lib/three.module.js';

export const WORLD = {
  colliders: [],   // {type:'circle',x,z,r} | {type:'box',minX,maxX,minZ,maxZ}
  groundZones: [], // {contains(x,z,y), height(x,z)} — checked by player.js
  eyes: [],
  pathX: (z) => 6 * Math.sin((200 - z) * 0.055),
  fountain: new THREE.Vector3(-35, 0, 20),
  sign: new THREE.Vector3(10, 0, -10),
  cabin: new THREE.Vector3(60, 0, -30),
  housePos: new THREE.Vector3(0, 0, -110),
  signCodeMesh: null,
  cabinDoor: null,
  dynamic: []
};

function addCircle(x, z, r) { WORLD.colliders.push({ type: 'circle', x, z, r }); }
function addBox(minX, maxX, minZ, maxZ) { WORLD.colliders.push({ type: 'box', minX, maxX, minZ, maxZ }); }

// ---------------------------------------------------------------- textures

function noiseTexture(base, variation, size = 128) {
  const c = document.createElement('canvas');
  c.width = c.height = size;
  const g = c.getContext('2d');
  g.fillStyle = base;
  g.fillRect(0, 0, size, size);
  const img = g.getImageData(0, 0, size, size);
  for (let i = 0; i < img.data.length; i += 4) {
    const n = (Math.random() - 0.5) * variation;
    img.data[i] += n; img.data[i + 1] += n; img.data[i + 2] += n;
  }
  g.putImageData(img, 0, 0);
  const t = new THREE.CanvasTexture(c);
  t.wrapS = t.wrapT = THREE.RepeatWrapping;
  return t;
}

function eyeTexture() {
  const c = document.createElement('canvas');
  c.width = 256; c.height = 128;
  const g = c.getContext('2d');
  g.clearRect(0, 0, 256, 128);
  // sclera
  g.fillStyle = 'rgba(190,190,200,0.85)';
  g.beginPath();
  g.ellipse(128, 64, 110, 46, 0, 0, Math.PI * 2);
  g.fill();
  // iris
  const grad = g.createRadialGradient(128, 64, 4, 128, 64, 34);
  grad.addColorStop(0, '#000');
  grad.addColorStop(0.55, '#1a1a22');
  grad.addColorStop(1, '#3a3a48');
  g.fillStyle = grad;
  g.beginPath(); g.arc(128, 64, 34, 0, Math.PI * 2); g.fill();
  g.fillStyle = '#000';
  g.beginPath(); g.arc(128, 64, 14, 0, Math.PI * 2); g.fill();
  // veins
  g.strokeStyle = 'rgba(120,60,60,0.5)';
  for (let i = 0; i < 7; i++) {
    g.beginPath();
    g.moveTo(20 + Math.random() * 60, 30 + Math.random() * 70);
    g.bezierCurveTo(60 + Math.random() * 40, 40 + Math.random() * 50,
      90, 60, 100 + Math.random() * 10, 60 + Math.random() * 10);
    g.stroke();
  }
  return new THREE.CanvasTexture(c);
}

// ---------------------------------------------------------------- builders

export function buildWorld(scene) {
  scene.background = new THREE.Color(0x000004);
  scene.fog = new THREE.FogExp2(0x010104, 0.030);

  const ambient = new THREE.AmbientLight(0x10121e, 0.55);
  scene.add(ambient);
  // faint cold rim from "nothing" so silhouettes read at close range
  const hemi = new THREE.HemisphereLight(0x0c0e1a, 0x000000, 0.35);
  scene.add(hemi);

  buildGround(scene);
  buildPath(scene);
  buildTrees(scene);
  buildFences(scene);
  buildRocks(scene);
  buildSkyEyes(scene);
  buildFountain(scene);
  buildSign(scene);
  buildCabin(scene);

  // world bounds
  addBox(-160, -140, -200, 230); addBox(140, 160, -200, 230);
  addBox(-160, 160, -200, -155); addBox(-160, 160, 212, 230);
  // default ground zone (outdoors, y=0)
  WORLD.groundZones.push({ contains: () => true, height: () => 0, priority: 0 });
}

function buildGround(scene) {
  const geo = new THREE.PlaneGeometry(700, 700, 70, 70);
  geo.rotateX(-Math.PI / 2);
  const pos = geo.attributes.position;
  for (let i = 0; i < pos.count; i++) {
    const x = pos.getX(i), z = pos.getZ(i);
    // gentle bumps away from the playable strip, sharp hills at far edges
    const edge = Math.max(0, (Math.abs(x) - 110) / 80) + Math.max(0, (Math.abs(z - 30) - 190) / 80);
    const n = Math.sin(x * 0.05) * Math.cos(z * 0.06) * 0.4;
    pos.setY(i, -0.02 + n * Math.min(1, edge * 0.5) + edge * edge * 6);
  }
  geo.computeVertexNormals();
  const tex = noiseTexture('#3d4434', 34, 256);
  tex.repeat.set(60, 60);
  const mat = new THREE.MeshLambertMaterial({ color: 0xffffff, map: tex, flatShading: true });
  const ground = new THREE.Mesh(geo, mat);
  ground.receiveShadow = true;
  scene.add(ground);
}

function buildPath(scene) {
  // dirt strip along the winding path + white marker stones
  const pts = [];
  for (let z = 205; z >= 55; z -= 2.5) pts.push(new THREE.Vector3(WORLD.pathX(z), 0, z));
  // continue faintly through forest to the house
  for (let z = 55; z >= -88; z -= 4) pts.push(new THREE.Vector3(WORLD.pathX(55) * (z + 88) / 143, 0, z));

  const shapeGeo = [];
  const idx = [];
  for (let i = 0; i < pts.length; i++) {
    const p = pts[i];
    const dir = (i < pts.length - 1) ? pts[i + 1].clone().sub(p) : p.clone().sub(pts[i - 1]);
    const side = new THREE.Vector3(-dir.z, 0, dir.x).normalize().multiplyScalar(1.6);
    shapeGeo.push(p.x - side.x, 0.03, p.z - side.z, p.x + side.x, 0.03, p.z + side.z);
    if (i > 0) {
      const a = (i - 1) * 2;
      idx.push(a, a + 1, a + 2, a + 1, a + 3, a + 2);
    }
  }
  const geo = new THREE.BufferGeometry();
  geo.setAttribute('position', new THREE.Float32BufferAttribute(shapeGeo, 3));
  geo.setIndex(idx);
  geo.computeVertexNormals();
  const tex = noiseTexture('#5c4a34', 30, 128);
  tex.repeat.set(4, 40);
  const mat = new THREE.MeshLambertMaterial({ color: 0xffffff, map: tex });
  const path = new THREE.Mesh(geo, mat);
  path.receiveShadow = true;
  scene.add(path);

  // white stones flanking phase-1 path
  const stoneGeo = new THREE.IcosahedronGeometry(0.14, 0);
  const stoneMat = new THREE.MeshLambertMaterial({ color: 0xb9b9b2, flatShading: true });
  const stones = new THREE.InstancedMesh(stoneGeo, stoneMat, 120);
  const m = new THREE.Matrix4();
  let si = 0;
  for (let z = 202; z >= 58 && si < 120; z -= 2.6) {
    for (const s of [-1, 1]) {
      if (si >= 120) break;
      m.makeRotationY(Math.random() * Math.PI);
      m.setPosition(WORLD.pathX(z) + s * (1.9 + Math.random() * 0.3), 0.1, z + Math.random());
      stones.setMatrixAt(si++, m);
    }
  }
  stones.castShadow = true;
  scene.add(stones);
}

function buildTrees(scene) {
  const trunkGeo = new THREE.CylinderGeometry(0.18, 0.32, 5.4, 5);
  trunkGeo.translate(0, 2.7, 0);
  const trunkMat = new THREE.MeshLambertMaterial({ color: 0x2b2018, flatShading: true });
  const canopyGeo = new THREE.ConeGeometry(1.7, 4.4, 6);
  canopyGeo.translate(0, 6.4, 0);
  const canopyMat = new THREE.MeshLambertMaterial({ color: 0x16241a, flatShading: true });

  const positions = [];
  const reject = (x, z) => {
    if (z > 58) { // phase 1: keep a corridor around the path
      const d = Math.abs(x - WORLD.pathX(z));
      if (d < 4.5) return true;
      if (d > 42) return true;
      return false;
    }
    if (Math.hypot(x - WORLD.fountain.x, z - WORLD.fountain.z) < 7) return true;
    if (Math.hypot(x - WORLD.sign.x, z - WORLD.sign.z) < 5) return true;
    if (Math.hypot(x - WORLD.cabin.x, z - WORLD.cabin.z) < 9) return true;
    if (Math.hypot(x - WORLD.housePos.x, z - WORLD.housePos.z) < 22) return true;
    if (z < -88 && Math.abs(x) < 12) return true; // house approach
    return false;
  };

  // phase 1 flanks — dense
  for (let i = 0; i < 280; i++) {
    const z = 55 + Math.random() * 152;
    const side = Math.random() < 0.5 ? -1 : 1;
    const x = WORLD.pathX(z) + side * (5 + Math.random() * 34);
    if (!reject(x, z)) positions.push([x, z]);
  }
  // forest — progressive density toward the house
  for (let i = 0; i < 380; i++) {
    const z = -135 + Math.random() * 192;
    const x = -125 + Math.random() * 250;
    if (z > 58) continue;
    const density = z > 20 ? 0.55 : z > -40 ? 0.8 : z > -85 ? 1.0 : 0.35;
    if (Math.random() > density) continue;
    if (!reject(x, z)) positions.push([x, z]);
  }

  const trunks = new THREE.InstancedMesh(trunkGeo, trunkMat, positions.length);
  const canopies = new THREE.InstancedMesh(canopyGeo, canopyMat, positions.length);
  const m = new THREE.Matrix4();
  const q = new THREE.Quaternion();
  const e = new THREE.Euler();
  positions.forEach(([x, z], i) => {
    const s = 0.75 + Math.random() * 0.8;
    e.set((Math.random() - 0.5) * 0.07, Math.random() * Math.PI, (Math.random() - 0.5) * 0.07);
    q.setFromEuler(e);
    m.compose(new THREE.Vector3(x, 0, z), q, new THREE.Vector3(s, s, s));
    trunks.setMatrixAt(i, m);
    canopies.setMatrixAt(i, m);
    addCircle(x, z, 0.4 * s + 0.15);
  });
  trunks.castShadow = true;
  canopies.castShadow = true;
  scene.add(trunks, canopies);
}

function buildFences(scene) {
  // broken rusty fence posts along parts of the phase-1 path
  const postGeo = new THREE.BoxGeometry(0.12, 1.1, 0.12);
  postGeo.translate(0, 0.55, 0);
  const postMat = new THREE.MeshLambertMaterial({ color: 0x3a3027, flatShading: true });
  const posts = new THREE.InstancedMesh(postGeo, postMat, 70);
  const m = new THREE.Matrix4();
  const q = new THREE.Quaternion();
  const e = new THREE.Euler();
  let i = 0;
  for (let z = 195; z >= 90 && i < 70; z -= 3.2) {
    if (Math.random() < 0.25) continue; // gaps: broken fence
    const side = z % 2 > 1 ? 1 : -1;
    e.set((Math.random() - 0.5) * 0.5, 0, (Math.random() - 0.5) * 0.5);
    q.setFromEuler(e);
    m.compose(
      new THREE.Vector3(WORLD.pathX(z) + side * 3.4, 0, z),
      q, new THREE.Vector3(1, 0.7 + Math.random() * 0.5, 1)
    );
    posts.setMatrixAt(i++, m);
  }
  posts.count = i;
  posts.castShadow = true;
  scene.add(posts);
}

function buildRocks(scene) {
  const geo = new THREE.IcosahedronGeometry(0.5, 0);
  const mat = new THREE.MeshLambertMaterial({ color: 0x33332f, flatShading: true });
  const rocks = new THREE.InstancedMesh(geo, mat, 60);
  const m = new THREE.Matrix4();
  for (let i = 0; i < 60; i++) {
    const x = -120 + Math.random() * 240;
    const z = -130 + Math.random() * 330;
    if (Math.abs(x - WORLD.pathX(Math.max(58, z))) < 4 && z > 58) { i--; continue; }
    const s = 0.4 + Math.random() * 1.3;
    m.makeRotationY(Math.random() * Math.PI);
    m.scale(new THREE.Vector3(s, s * 0.7, s));
    m.setPosition(x, 0.1, z);
    rocks.setMatrixAt(i, m);
    if (s > 0.8) addCircle(x, z, s * 0.5);
  }
  rocks.castShadow = true;
  scene.add(rocks);
}

function buildSkyEyes(scene) {
  const tex = eyeTexture();
  for (let i = 0; i < 14; i++) {
    const mat = new THREE.MeshBasicMaterial({
      map: tex, transparent: true,
      opacity: 0.05 + Math.random() * 0.1,
      fog: false, depthWrite: false
    });
    const w = 26 + Math.random() * 40;
    const eye = new THREE.Mesh(new THREE.PlaneGeometry(w, w / 2), mat);
    eye.position.set(-260 + Math.random() * 520, 110 + Math.random() * 90, -260 + Math.random() * 520);
    eye.rotation.x = Math.PI / 2 + (Math.random() - 0.5) * 0.5; // looking down
    eye.rotation.z = Math.random() * Math.PI * 2;
    scene.add(eye);
    WORLD.eyes.push({ mesh: eye, blinkT: Math.random() * 14, baseOp: mat.opacity, baseSy: eye.scale.y });
  }
}

export function updateEyes(time, dt, stress) {
  for (const e of WORLD.eyes) {
    e.blinkT -= dt;
    if (e.blinkT < 0) {
      e.blinkT = 6 + Math.random() * 16;
      e.closing = 0.28;
    }
    if (e.closing > 0) {
      e.closing -= dt;
      const f = Math.abs(Math.sin((e.closing / 0.28) * Math.PI));
      e.mesh.scale.y = e.baseSy * Math.max(0.05, f);
    } else {
      e.mesh.scale.y = e.baseSy;
    }
    e.mesh.material.opacity = e.baseOp + (stress / 100) * 0.16;
  }
}

function buildFountain(scene) {
  const g = new THREE.Group();
  const stone = new THREE.MeshLambertMaterial({ color: 0x4a4f44, flatShading: true });
  const basin = new THREE.Mesh(new THREE.CylinderGeometry(2.3, 2.5, 0.85, 10), stone);
  basin.position.y = 0.42;
  const rim = new THREE.Mesh(new THREE.TorusGeometry(2.3, 0.16, 5, 10), stone);
  rim.rotation.x = Math.PI / 2; rim.position.y = 0.86;
  const pillar = new THREE.Mesh(new THREE.CylinderGeometry(0.22, 0.3, 1.6, 7), stone);
  pillar.position.y = 1.4;
  const bowl = new THREE.Mesh(new THREE.CylinderGeometry(0.85, 0.5, 0.35, 9), stone);
  bowl.position.y = 2.25;
  const water = new THREE.Mesh(
    new THREE.CircleGeometry(2.1, 12),
    new THREE.MeshStandardMaterial({ color: 0x06090c, roughness: 0.15, metalness: 0.6 })
  );
  water.rotation.x = -Math.PI / 2; water.position.y = 0.72;
  g.add(basin, rim, pillar, bowl, water);
  g.position.copy(WORLD.fountain);
  g.traverse(o => { o.castShadow = true; o.receiveShadow = true; });
  scene.add(g);
  addCircle(WORLD.fountain.x, WORLD.fountain.z, 2.7);

  // illegible latin inscription plate
  const c = document.createElement('canvas');
  c.width = 256; c.height = 64;
  const ctx = c.getContext('2d');
  ctx.fillStyle = '#3a3f36'; ctx.fillRect(0, 0, 256, 64);
  ctx.fillStyle = 'rgba(180,180,170,0.35)';
  ctx.font = 'italic 18px serif';
  ctx.fillText('AQVA · OCVLI · SEMPER · VIDENT', 12, 38);
  const plate = new THREE.Mesh(
    new THREE.PlaneGeometry(1.6, 0.4),
    new THREE.MeshLambertMaterial({ map: new THREE.CanvasTexture(c) })
  );
  plate.position.set(WORLD.fountain.x, 0.55, WORLD.fountain.z + 2.52);
  scene.add(plate);
}

function buildSign(scene) {
  const wood = new THREE.MeshLambertMaterial({ color: 0x3d3226, flatShading: true });
  const post = new THREE.Mesh(new THREE.BoxGeometry(0.18, 2.6, 0.18), wood);
  post.position.set(WORLD.sign.x, 1.3, WORLD.sign.z);
  post.castShadow = true;
  scene.add(post);
  addCircle(WORLD.sign.x, WORLD.sign.z, 0.3);

  // weathered board with faded text
  const c = document.createElement('canvas');
  c.width = 512; c.height = 160;
  const g = c.getContext('2d');
  g.fillStyle = '#46392a'; g.fillRect(0, 0, 512, 160);
  for (let i = 0; i < 300; i++) {
    g.fillStyle = `rgba(0,0,0,${Math.random() * 0.18})`;
    g.fillRect(Math.random() * 512, Math.random() * 160, 3, 14);
  }
  g.fillStyle = 'rgba(200,190,170,0.20)';
  g.font = '34px serif';
  g.fillText('CASA  BLANCA  →', 60, 60);
  g.font = '22px serif';
  g.fillText('el agua recuerda', 90, 110);
  const board = new THREE.Mesh(
    new THREE.BoxGeometry(2.0, 0.62, 0.07),
    [wood, wood, wood, wood,
      new THREE.MeshLambertMaterial({ map: new THREE.CanvasTexture(c) }), wood]
  );
  board.position.set(WORLD.sign.x, 1.9, WORLD.sign.z + 0.13);
  board.rotation.z = 0.04;
  board.castShadow = true;
  scene.add(board);

  // hidden code "2-7-4-1" — only visible when lit at a precise angle
  const cc = document.createElement('canvas');
  cc.width = 512; cc.height = 160;
  const gc = cc.getContext('2d');
  gc.clearRect(0, 0, 512, 160);
  gc.fillStyle = 'rgba(225,220,200,0.95)';
  gc.font = 'bold 56px monospace';
  gc.fillText('2 · 7 · 4 · 1', 110, 100);
  const codeMat = new THREE.MeshBasicMaterial({
    map: new THREE.CanvasTexture(cc), transparent: true, opacity: 0
  });
  const codeMesh = new THREE.Mesh(new THREE.PlaneGeometry(2.0, 0.62), codeMat);
  codeMesh.position.set(WORLD.sign.x, 1.9, WORLD.sign.z + 0.175);
  codeMesh.rotation.z = 0.04;
  scene.add(codeMesh);
  WORLD.signCodeMesh = codeMesh;
}

function buildCabin(scene) {
  const cx = WORLD.cabin.x, cz = WORLD.cabin.z;
  const wallMat = new THREE.MeshLambertMaterial({ color: 0x4a4238, flatShading: true });
  const g = new THREE.Group();

  // cabin 5(x) x 4(z), door on west wall facing the sign
  const mk = (w, h, d, x, y, z) => {
    const m = new THREE.Mesh(new THREE.BoxGeometry(w, h, d), wallMat);
    m.position.set(x, y, z);
    m.castShadow = true; m.receiveShadow = true;
    g.add(m);
    addBox(cx + x - w / 2, cx + x + w / 2, cz + z - d / 2, cz + z + d / 2);
  };
  mk(5, 2.6, 0.2, 0, 1.3, -2);   // north
  mk(5, 2.6, 0.2, 0, 1.3, 2);    // south
  mk(0.2, 2.6, 4, 2.5, 1.3, 0);  // east
  // west wall with door opening (door 1.0 wide centered)
  mk(0.2, 2.6, 1.5, -2.5, 1.3, -1.25);
  mk(0.2, 2.6, 1.5, -2.5, 1.3, 1.25);
  mk(0.2, 0.6, 1.0, -2.5, 2.3, 0); // lintel
  // roof
  const roof = new THREE.Mesh(new THREE.ConeGeometry(4.2, 1.6, 4), wallMat);
  roof.rotation.y = Math.PI / 4;
  roof.position.y = 3.4;
  roof.castShadow = true;
  g.add(roof);
  // floor
  const floor = new THREE.Mesh(new THREE.BoxGeometry(5, 0.08, 4),
    new THREE.MeshLambertMaterial({ color: 0x35302a }));
  floor.position.y = 0.04;
  floor.receiveShadow = true;
  g.add(floor);
  // table + fake charger
  const table = new THREE.Mesh(new THREE.BoxGeometry(1.2, 0.08, 0.7), wallMat);
  table.position.set(1.2, 0.8, -0.8);
  const leg = new THREE.Mesh(new THREE.BoxGeometry(0.9, 0.78, 0.5), new THREE.MeshLambertMaterial({ color: 0x2e2922 }));
  leg.position.set(1.2, 0.4, -0.8);
  const charger = new THREE.Mesh(new THREE.BoxGeometry(0.25, 0.1, 0.18),
    new THREE.MeshLambertMaterial({ color: 0x111111, emissive: 0x062006 }));
  charger.position.set(1.2, 0.9, -0.8);
  g.add(table, leg, charger);
  addBox(cx + 0.6, cx + 1.8, cz - 1.15, cz - 0.45);

  // keypad by the door
  const pad = new THREE.Mesh(new THREE.BoxGeometry(0.04, 0.3, 0.2),
    new THREE.MeshLambertMaterial({ color: 0x222226, emissive: 0x101820 }));
  pad.position.set(-2.55, 1.4, 0.85);
  g.add(pad);

  // door (closed; opens with code 2741)
  const door = new THREE.Mesh(new THREE.BoxGeometry(0.08, 2.0, 1.0),
    new THREE.MeshLambertMaterial({ color: 0x55402c, flatShading: true }));
  door.position.set(cx - 2.5, 1.0, cz);
  door.castShadow = true;
  scene.add(door);
  WORLD.cabinDoor = door;
  const doorCol = { type: 'box', minX: cx - 2.6, maxX: cx - 2.4, minZ: cz - 0.5, maxZ: cz + 0.5 };
  WORLD.colliders.push(doorCol);
  WORLD.cabinDoorCollider = doorCol;

  g.position.set(cx, 0, cz);
  scene.add(g);
}

export function openCabinDoor() {
  if (!WORLD.cabinDoor) return;
  WORLD.cabinDoor.position.z -= 0.95; // slide open
  WORLD.cabinDoor.rotation.y = 0.9;
  const c = WORLD.cabinDoorCollider;
  c.minX = c.maxX = -9999; // disable
  c.minZ = c.maxZ = -9999;
}
