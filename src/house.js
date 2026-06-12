// PARANOIA — the white house (exterior, interior, basement, hidden room)
// House center: WORLD.housePos (0,-110). Local coords x[-6,6], z[-5,5],
// front door faces +z (the direction the player arrives from).

import * as THREE from '../lib/three.module.js';
import { WORLD } from './world.js';

export const HOUSE = {
  bedroomDoor: null,
  basementDoor: null,
  falseWall: null,
  mirror: null,
  mirrorFigure: null,
  drawerKeyTaken: false,
  // world-space points of interest
  p: {} // filled in build
};

const HX = 0, HZ = -110; // house center world coords
const W = (x, z) => [HX + x, HZ + z];

function addBoxL(minX, maxX, minZ, maxZ) {
  WORLD.colliders.push({ type: 'box', minX: HX + minX, maxX: HX + maxX, minZ: HZ + minZ, maxZ: HZ + maxZ });
}

function muralTexture() {
  const c = document.createElement('canvas');
  c.width = 1024; c.height = 512;
  const g = c.getContext('2d');
  g.fillStyle = '#1c1a18'; g.fillRect(0, 0, 1024, 512);
  // rows of crude painted eyes
  for (let row = 0; row < 4; row++) {
    for (let col = 0; col < 9; col++) {
      const x = 60 + col * 105 + (row % 2) * 50, y = 50 + row * 95;
      g.strokeStyle = 'rgba(170,150,130,0.8)';
      g.lineWidth = 3;
      g.beginPath();
      g.ellipse(x, y, 38, 17, (Math.random() - 0.5) * 0.3, 0, Math.PI * 2);
      g.stroke();
      g.fillStyle = 'rgba(150,30,25,0.7)';
      g.beginPath(); g.arc(x, y, 8 + Math.random() * 4, 0, Math.PI * 2); g.fill();
    }
  }
  g.fillStyle = 'rgba(200,185,160,0.9)';
  g.font = 'bold 44px monospace';
  g.textAlign = 'center';
  g.fillText('TRUST ME', 512, 430);
  g.font = 'bold 28px monospace';
  g.fillText("DON'T LOOK BACK — YOU ARE BEING OBSERVED", 512, 478);
  return new THREE.CanvasTexture(c);
}

function noteTexture() {
  const c = document.createElement('canvas');
  c.width = 256; c.height = 320;
  const g = c.getContext('2d');
  g.fillStyle = '#cfc9b8'; g.fillRect(0, 0, 256, 320);
  g.fillStyle = '#2a2520';
  g.font = 'italic 17px serif';
  const lines = ['El smartphone es', 'una puerta.', '', 'Confía en él.', 'O desconfía.', '', 'Pero decide antes', 'de que ellos', 'decidan por ti.'];
  lines.forEach((l, i) => g.fillText(l, 26, 50 + i * 28));
  return new THREE.CanvasTexture(c);
}

export function buildHouse(scene) {
  const group = new THREE.Group();

  // ---------- EXTERIOR SHELL — unreal white, ignores fog (visible from afar)
  const extMat = new THREE.MeshLambertMaterial({
    color: 0x9a9aa0, emissive: 0x202024, flatShading: true, fog: false
  });
  const darkMat = new THREE.MeshBasicMaterial({ color: 0x010103, fog: false });

  const mkExt = (w, h, d, x, y, z) => {
    const m = new THREE.Mesh(new THREE.BoxGeometry(w, h, d), extMat);
    m.position.set(HX + x, y, HZ + z);
    m.castShadow = true; m.receiveShadow = true;
    group.add(m);
    return m;
  };
  // south (front) wall with door opening x[-0.6,0.6], h 2.2
  mkExt(5.4, 5.6, 0.25, -3.3, 2.8, 5);
  mkExt(5.4, 5.6, 0.25, 3.3, 2.8, 5);
  mkExt(1.2, 3.4, 0.25, 0, 2.2 + 1.7, 5);
  // back / sides full
  mkExt(12, 5.6, 0.25, 0, 2.8, -5);
  mkExt(0.25, 5.6, 10, -6, 2.8, 0);
  mkExt(0.25, 5.6, 10, 6, 2.8, 0);
  addBoxL(-6.1, -0.65, 4.8, 5.2); addBoxL(0.65, 6.1, 4.8, 5.2);
  addBoxL(-6.1, 6.1, -5.2, -4.8);
  addBoxL(-6.2, -5.85, -5, 5); addBoxL(5.85, 6.2, -5, 5);

  // roof — asymmetric on purpose
  const roof = new THREE.Mesh(new THREE.ConeGeometry(8.6, 2.6, 4), extMat);
  roof.rotation.y = Math.PI / 4 + 0.05;
  roof.scale.set(1.18, 1, 0.95);
  roof.position.set(HX + 0.2, 6.7, HZ);
  roof.castShadow = true;
  group.add(roof);
  // chimney (no smoke)
  const chim = mkExt(0.7, 1.8, 0.7, 3.1, 7.0, -1.2);
  chim.rotation.y = 0.06;

  // windows — black voids, deliberately misaligned
  const winGeo = new THREE.PlaneGeometry(0.9, 1.2);
  const wins = [
    [-3.4, 1.6, 5.13, 0], [2.8, 1.75, 5.13, 0], [4.6, 1.5, 5.13, 0], [-1.7, 1.65, 5.13, 0],
    [-2.6, 4.3, 5.13, 0.03], [3.3, 4.45, 5.13, -0.04],
    [6.13, 1.6, -1, Math.PI / 2], [-6.13, 1.7, 1.5, -Math.PI / 2]
  ];
  for (const [x, y, z, rot] of wins) {
    const m = new THREE.Mesh(winGeo, darkMat);
    if (Math.abs(z) > 5) { m.position.set(HX + x, y, HZ + z); m.rotation.y = rot; }
    else { m.position.set(HX + x, y, HZ + z); m.rotation.y = rot; }
    group.add(m);
  }

  // front door — dark red, ajar
  const door = new THREE.Mesh(new THREE.BoxGeometry(1.2, 2.2, 0.08),
    new THREE.MeshLambertMaterial({ color: 0x4a1410, flatShading: true }));
  door.position.set(HX - 0.6, 1.1, HZ + 5);
  door.geometry.translate(0.6, 0, 0); // hinge on left edge
  door.rotation.y = -1.1; // ajar — an invitation
  group.add(door);

  // broken fence + overgrown yard hints
  const fenceMat = new THREE.MeshLambertMaterial({ color: 0x5a5a58, flatShading: true });
  for (let i = 0; i < 14; i++) {
    const fp = new THREE.Mesh(new THREE.BoxGeometry(0.1, 0.9, 0.1), fenceMat);
    const ang = (i / 14) * Math.PI * 2;
    fp.position.set(HX + Math.cos(ang) * 10.5, 0.45, HZ + Math.sin(ang) * 9);
    fp.rotation.z = (Math.random() - 0.5) * 0.6;
    if (Math.random() < 0.75) group.add(fp);
  }

  // ---------- INTERIOR -------------------------------------------------
  const wallMat = new THREE.MeshLambertMaterial({ color: 0x5e5c58, flatShading: true });
  const floorMat = new THREE.MeshLambertMaterial({ color: 0x4a4540 });
  const woodMat = new THREE.MeshLambertMaterial({ color: 0x3d3226, flatShading: true });
  const mkInt = (w, h, d, x, y, z, mat = wallMat, collide = true) => {
    const m = new THREE.Mesh(new THREE.BoxGeometry(w, h, d), mat);
    m.position.set(HX + x, y, HZ + z);
    m.receiveShadow = true; m.castShadow = true;
    group.add(m);
    if (collide) addBoxL(x - w / 2, x + w / 2, z - d / 2, z + d / 2);
    return m;
  };

  // ground floor slab + ceiling (= upstairs floor, with stairwell hole)
  const gfloor = new THREE.Mesh(new THREE.BoxGeometry(12, 0.1, 10), floorMat);
  gfloor.position.set(HX, 0.02, HZ);
  gfloor.receiveShadow = true;
  group.add(gfloor);
  // upstairs slab: hole at x[-6,-4.2], z[-4.8,-0.2]
  mkInt(12, 0.18, 4.0, 0, 3.0, 3.0, floorMat, false);     // z [1..5] (attic void edge)
  mkInt(12, 0.18, 1.2, 0, 3.0, 0.4, floorMat, false);     // z [-0.2..1]
  mkInt(10.2, 0.18, 4.6, 0.9, 3.0, -2.5, floorMat, false); // x[-4.2..6] z[-4.8..-0.2]
  mkInt(12, 0.18, 0.2, 0, 3.0, -4.9, floorMat, false);

  // interior wall: living/kitchen at x=2 (z 0..5), opening z[2,3.2]
  mkInt(0.15, 3, 2.0, 2, 1.5, 1.0);
  mkInt(0.15, 3, 1.8, 2, 1.5, 4.1);
  // interior wall: hall at z=0, openings x[-4,-2.8] and x[3,4.2]
  mkInt(2.0, 3, 0.15, -5.0, 1.5, 0);
  mkInt(5.8, 3, 0.15, 0.1, 1.5, 0);
  mkInt(1.8, 3, 0.15, 5.1, 1.5, 0);

  // ---------- LIVING ROOM (x -6..2, z 0..5)
  mkInt(2.2, 0.75, 0.9, -3.5, 0.38, 3.9, new THREE.MeshLambertMaterial({ color: 0x4e4e52, flatShading: true })); // sofa
  mkInt(2.2, 0.5, 0.25, -3.5, 0.95, 4.35, new THREE.MeshLambertMaterial({ color: 0x44444a, flatShading: true }), false); // sofa back
  mkInt(1.1, 0.45, 0.7, -3.4, 0.23, 2.2, woodMat); // coffee table
  // broken floor lamp
  const lamp = new THREE.Mesh(new THREE.CylinderGeometry(0.03, 0.05, 1.5, 5), woodMat);
  lamp.position.set(HX - 5.3, 0.7, HZ + 1.0);
  lamp.rotation.z = 0.5;
  group.add(lamp);
  // empty picture frames
  for (const [fx, fy, fz] of [[-2, 1.8, 4.9], [-4.5, 1.65, 4.9], [0.5, 1.9, 4.9]]) {
    const fr = new THREE.Mesh(new THREE.BoxGeometry(0.6, 0.8, 0.04), woodMat);
    fr.position.set(HX + fx, fy, HZ + fz);
    fr.rotation.z = (Math.random() - 0.5) * 0.12;
    group.add(fr);
    const inner = new THREE.Mesh(new THREE.PlaneGeometry(0.46, 0.66),
      new THREE.MeshLambertMaterial({ color: 0x14110e }));
    inner.position.set(HX + fx, fy, HZ + fz - 0.025);
    inner.rotation.y = Math.PI;
    inner.rotation.z = fr.rotation.z;
    group.add(inner);
  }

  // ---------- KITCHEN (x 2..6, z 0..5)
  mkInt(0.9, 1.9, 0.8, 5.4, 0.95, 4.4, new THREE.MeshLambertMaterial({ color: 0x6e6e6a, flatShading: true })); // fridge
  mkInt(0.9, 0.95, 0.8, 5.4, 0.48, 3.3, new THREE.MeshLambertMaterial({ color: 0x55544e, flatShading: true })); // stove
  // counters with drawers along east wall (x≈5.5, z 0.4..2.6)
  mkInt(0.9, 0.9, 2.4, 5.4, 0.45, 1.5, woodMat);
  const drawerMat = new THREE.MeshLambertMaterial({ color: 0x4a3e2e, flatShading: true });
  HOUSE.drawers = [];
  for (let i = 0; i < 3; i++) {
    const d = new THREE.Mesh(new THREE.BoxGeometry(0.06, 0.22, 0.6), drawerMat);
    d.position.set(HX + 4.93, 0.62, HZ + 0.75 + i * 0.72);
    group.add(d);
    HOUSE.drawers.push(d);
  }
  // rotten food decals on a small table
  mkInt(0.9, 0.72, 0.9, 3.0, 0.36, 3.8, woodMat);
  const plate = new THREE.Mesh(new THREE.CircleGeometry(0.18, 8),
    new THREE.MeshLambertMaterial({ color: 0x3c3a22 }));
  plate.rotation.x = -Math.PI / 2;
  plate.position.set(HX + 3.0, 0.74, HZ + 3.8);
  group.add(plate);

  // ---------- HALL + STAIRS (z -5..0)
  // main stairs UP along west wall: x[-5.6,-4.4], z -0.2 → -4.6 rises 0→3
  const stairMat = new THREE.MeshLambertMaterial({ color: 0x4e463c, flatShading: true });
  for (let i = 0; i < 12; i++) {
    const st = new THREE.Mesh(new THREE.BoxGeometry(1.2, 0.25, 0.38), stairMat);
    st.position.set(HX - 5.0, 0.125 + i * 0.25, HZ - 0.4 - i * 0.367);
    st.castShadow = true; st.receiveShadow = true;
    group.add(st);
  }
  // railing collider east of stairs
  addBoxL(-4.45, -4.3, -4.6, -0.2);
  const rail = new THREE.Mesh(new THREE.BoxGeometry(0.08, 1.0, 4.4), woodMat);
  rail.position.set(HX - 4.38, 1.9, HZ - 2.4);
  rail.rotation.x = -0.6;
  group.add(rail);

  // basement stairs DOWN along east wall: x[4.4,5.6], z -0.3 → -4.6 drops 0→-2.5
  for (let i = 0; i < 11; i++) {
    const st = new THREE.Mesh(new THREE.BoxGeometry(1.2, 0.25, 0.4), stairMat);
    st.position.set(HX + 5.0, -0.23 - i * 0.227, HZ - 0.55 - i * 0.39);
    group.add(st);
  }
  // stairwell walls
  mkInt(0.15, 3, 4.6, 4.4, 1.5, -2.5);
  // basement door at corridor entrance (locked)
  const bDoor = new THREE.Mesh(new THREE.BoxGeometry(1.1, 2.1, 0.09),
    new THREE.MeshLambertMaterial({ color: 0x33414a, flatShading: true })); // cold metal blue
  bDoor.position.set(HX + 5.0, 1.05, HZ - 0.3);
  group.add(bDoor);
  HOUSE.basementDoor = bDoor;
  const bCol = { type: 'box', minX: HX + 4.4, maxX: HX + 5.6, minZ: HZ - 0.42, maxZ: HZ - 0.18 };
  WORLD.colliders.push(bCol);
  HOUSE.basementDoorCollider = bCol;

  // ---------- UPSTAIRS: landing + bedroom door + bedroom
  // wall sealing bedroom from landing at x=-4.2 (z -5..1), opening z[-4.7,-3.7]
  const upWall1 = new THREE.Mesh(new THREE.BoxGeometry(0.15, 2.9, 4.9), wallMat);
  upWall1.position.set(HX - 4.2, 4.55, HZ + -1.25);
  group.add(upWall1);
  WORLD.colliders.push({ type: 'box2', minX: HX - 4.3, maxX: HX - 4.1, minZ: HZ - 3.7, maxZ: HZ + 1, minY: 2.5 });
  const lintel = new THREE.Mesh(new THREE.BoxGeometry(0.15, 0.8, 1.0), wallMat);
  lintel.position.set(HX - 4.2, 5.6, HZ - 4.2);
  group.add(lintel);
  // bedroom door (locked — KITCHEN KEY)
  const bedDoor = new THREE.Mesh(new THREE.BoxGeometry(0.09, 2.1, 1.0),
    new THREE.MeshLambertMaterial({ color: 0x55402c, flatShading: true }));
  bedDoor.position.set(HX - 4.2, 4.1, HZ - 4.2);
  group.add(bedDoor);
  HOUSE.bedroomDoor = bedDoor;
  const bedCol = { type: 'box2', minX: HX - 4.32, maxX: HX - 4.08, minZ: HZ - 4.72, maxZ: HZ - 3.68, minY: 2.5 };
  WORLD.colliders.push(bedCol);
  HOUSE.bedroomDoorCollider = bedCol;
  // upstairs south wall (z=1)
  const upWall2 = new THREE.Mesh(new THREE.BoxGeometry(12, 2.9, 0.15), wallMat);
  upWall2.position.set(HX, 4.55, HZ + 1);
  group.add(upWall2);
  WORLD.colliders.push({ type: 'box2', minX: HX - 6, maxX: HX + 6, minZ: HZ + 0.9, maxZ: HZ + 1.1, minY: 2.5 });

  // bedroom furniture (x -4.2..6, z -5..1, floor y=3.1)
  const bedB = new THREE.Mesh(new THREE.BoxGeometry(1.6, 0.5, 2.2),
    new THREE.MeshLambertMaterial({ color: 0x6a6a66, flatShading: true }));
  bedB.position.set(HX - 1.5, 3.35, HZ - 3.6);
  group.add(bedB);
  WORLD.colliders.push({ type: 'box2', minX: HX - 2.3, maxX: HX - 0.7, minZ: HZ - 4.7, maxZ: HZ - 2.5, minY: 2.5 });
  // wardrobe + mirror on east wall, facing west
  const ward = new THREE.Mesh(new THREE.BoxGeometry(0.7, 2.2, 1.6),
    new THREE.MeshLambertMaterial({ color: 0x33291d, flatShading: true }));
  ward.position.set(HX + 5.5, 4.2, HZ - 2.0);
  group.add(ward);
  WORLD.colliders.push({ type: 'box2', minX: HX + 5.1, maxX: HX + 6, minZ: HZ - 2.8, maxZ: HZ - 1.2, minY: 2.5 });
  const mirror = new THREE.Mesh(new THREE.PlaneGeometry(0.9, 1.7),
    new THREE.MeshStandardMaterial({ color: 0x202830, roughness: 0.08, metalness: 0.9 }));
  mirror.position.set(HX + 5.12, 4.2, HZ - 2.0);
  mirror.rotation.y = -Math.PI / 2;
  group.add(mirror);
  HOUSE.mirror = mirror;
  // the figure that appears "in" the mirror (hidden until triggered)
  const figMat = new THREE.MeshBasicMaterial({ color: 0x000000, transparent: true, opacity: 0 });
  const fig = new THREE.Group();
  const fHead = new THREE.Mesh(new THREE.BoxGeometry(0.22, 0.3, 0.2), figMat);
  fHead.position.y = 0.62;
  const fBody = new THREE.Mesh(new THREE.BoxGeometry(0.34, 0.8, 0.18), figMat);
  fBody.position.y = 0.05;
  fig.add(fHead, fBody);
  fig.position.set(HX + 5.04, 4.1, HZ - 2.0);
  fig.rotation.y = -Math.PI / 2;
  fig.scale.set(0.9, 0.9, 0.05);
  group.add(fig);
  HOUSE.mirrorFigure = { group: fig, mat: figMat };
  // desk + window with heavy curtains + geometric rug
  const desk = new THREE.Mesh(new THREE.BoxGeometry(1.2, 0.78, 0.6), woodMat);
  desk.position.set(HX + 2.5, 3.5, HZ - 4.6);
  group.add(desk);
  const curt = new THREE.Mesh(new THREE.BoxGeometry(1.4, 2.2, 0.1),
    new THREE.MeshLambertMaterial({ color: 0x2a2a30, flatShading: true }));
  curt.position.set(HX + 0.5, 4.3, HZ + 0.92);
  group.add(curt);
  const rugC = document.createElement('canvas');
  rugC.width = rugC.height = 128;
  const rg = rugC.getContext('2d');
  rg.fillStyle = '#262830'; rg.fillRect(0, 0, 128, 128);
  rg.strokeStyle = '#4a4e62';
  for (let i = 0; i < 6; i++) { rg.strokeRect(10 + i * 9, 10 + i * 9, 108 - i * 18, 108 - i * 18); }
  const rug = new THREE.Mesh(new THREE.PlaneGeometry(2.2, 1.6),
    new THREE.MeshLambertMaterial({ map: new THREE.CanvasTexture(rugC) }));
  rug.rotation.x = -Math.PI / 2;
  rug.position.set(HX + 1.2, 3.12, HZ - 2.0);
  group.add(rug);
  HOUSE.p.mirrorSpot = new THREE.Vector3(HX + 1.2, 3.1, HZ - 2.0); // stand on the rug

  // ---------- BASEMENT (y -2.5, x -7.5..6, z -5..5; entities never enter)
  const bFloorMat = new THREE.MeshLambertMaterial({ color: 0x37352f });
  const bfloor = new THREE.Mesh(new THREE.BoxGeometry(13.5, 0.1, 10), bFloorMat);
  bfloor.position.set(HX - 0.75, -2.55, HZ);
  bfloor.receiveShadow = true;
  group.add(bfloor);
  const bCeil = new THREE.Mesh(new THREE.BoxGeometry(13.5, 0.1, 10), bFloorMat);
  bCeil.position.set(HX - 0.75, -0.25, HZ);
  group.add(bCeil);
  const bWallMat = new THREE.MeshLambertMaterial({ color: 0x55524a, flatShading: true });
  const mkB = (w, h, d, x, y, z, collide = true) => {
    const m = new THREE.Mesh(new THREE.BoxGeometry(w, h, d), bWallMat);
    m.position.set(HX + x, y, HZ + z);
    m.receiveShadow = true;
    group.add(m);
    if (collide) WORLD.colliders.push({ type: 'box2', minX: HX + x - w / 2, maxX: HX + x + w / 2, minZ: HZ + z - d / 2, maxZ: HZ + z + d / 2, maxY: 0 });
    return m;
  };
  mkB(13.5, 2.4, 0.2, -0.75, -1.4, 5);
  mkB(13.5, 2.4, 0.2, -0.75, -1.4, -5);
  mkB(0.2, 2.4, 10, -7.5, -1.4, 0);
  mkB(0.2, 2.4, 10, 6, -1.4, 0);
  // wall sealing basement from stair corridor except entry at z[-4.6..-3.4]
  mkB(0.2, 2.4, 8.2, 4.4, -1.4, 0.9);
  // pipes
  const pipeMat = new THREE.MeshLambertMaterial({ color: 0x4a3c30, flatShading: true });
  for (const [px, pz] of [[-2, -4.6], [1, -4.6], [3.5, -4.6]]) {
    const pipe = new THREE.Mesh(new THREE.CylinderGeometry(0.08, 0.08, 9, 6), pipeMat);
    pipe.rotation.z = Math.PI / 2;
    pipe.position.set(HX + px, -0.5, HZ + pz);
    group.add(pipe);
  }
  // old boxes + dead machine
  mkB(1.0, 0.8, 1.0, 2.5, -2.1, 3.5);
  mkB(0.8, 0.6, 0.8, 3.6, -2.2, 3.2);
  const machine = new THREE.Mesh(new THREE.BoxGeometry(1.8, 1.6, 1.0),
    new THREE.MeshLambertMaterial({ color: 0x3a4038, flatShading: true }));
  machine.position.set(HX - 2.5, -1.7, HZ + 4.2);
  group.add(machine);
  WORLD.colliders.push({ type: 'box2', minX: HX - 3.4, maxX: HX - 1.6, minZ: HZ + 3.7, maxZ: HZ + 4.7, maxY: 0 });
  // hanging cables
  for (let i = 0; i < 5; i++) {
    const cab = new THREE.Mesh(new THREE.CylinderGeometry(0.015, 0.015, 0.7 + Math.random() * 0.5, 4),
      new THREE.MeshLambertMaterial({ color: 0x1a1a1a }));
    cab.position.set(HX - 3 + Math.random() * 6, -0.65, HZ - 3 + Math.random() * 6);
    cab.rotation.x = (Math.random() - 0.5) * 0.3;
    group.add(cab);
  }
  // MURAL on north basement wall
  const mural = new THREE.Mesh(new THREE.PlaneGeometry(6.5, 2.0),
    new THREE.MeshLambertMaterial({ map: muralTexture() }));
  mural.position.set(HX - 1, -1.45, HZ - 4.88);
  group.add(mural);
  HOUSE.p.mural = mural.position.clone();

  // FALSE WALL west side: opening z[-1,0.2] hidden behind sliding slab
  // perimeter wall at x=-4 covers everything except that gap
  mkB(0.2, 2.4, 3.8, -4, -1.4, -3.1);
  mkB(0.2, 2.4, 4.6, -4, -1.4, 2.5);
  const fWall = new THREE.Mesh(new THREE.BoxGeometry(0.22, 2.4, 1.3),
    new THREE.MeshLambertMaterial({ color: 0x55524a, flatShading: true }));
  fWall.position.set(HX - 4, -1.4, HZ - 0.4);
  group.add(fWall);
  HOUSE.falseWall = fWall;
  const fCol = { type: 'box2', minX: HX - 4.12, maxX: HX - 3.88, minZ: HZ - 1.05, maxZ: HZ + 0.25, maxY: 0 };
  WORLD.colliders.push(fCol);
  HOUSE.falseWallCollider = fCol;
  HOUSE.p.falseWall = fWall.position.clone();

  // HIDDEN ROOM (x -7.5..-4, z -2.5..1.5)
  mkB(3.5, 2.4, 0.2, -5.75, -1.4, -2.5);
  mkB(3.5, 2.4, 0.2, -5.75, -1.4, 1.5);
  const ped = new THREE.Mesh(new THREE.BoxGeometry(0.5, 1.0, 0.5), bWallMat);
  ped.position.set(HX - 6, -2.0, HZ - 0.5);
  group.add(ped);
  // THE ORIGINAL SMARTPHONE — cracked, faintly alive
  const oldPhone = new THREE.Mesh(new THREE.BoxGeometry(0.08, 0.02, 0.16),
    new THREE.MeshStandardMaterial({ color: 0x0a0a0a, roughness: 0.4, metalness: 0.5 }));
  oldPhone.position.set(HX - 6, -1.48, HZ - 0.5);
  oldPhone.rotation.y = 0.4;
  group.add(oldPhone);
  const opC = document.createElement('canvas');
  opC.width = 64; opC.height = 128;
  const og = opC.getContext('2d');
  og.fillStyle = '#050608'; og.fillRect(0, 0, 64, 128);
  og.strokeStyle = 'rgba(180,180,200,0.5)';
  for (let i = 0; i < 8; i++) { // cracks
    og.beginPath(); og.moveTo(32, 64);
    og.lineTo(Math.random() * 64, Math.random() * 128); og.stroke();
  }
  og.fillStyle = 'rgba(140,255,160,0.8)';
  og.font = '10px monospace';
  og.fillText('YOU ARE', 8, 56);
  og.fillText('THE', 22, 68);
  og.fillText('RECORDING', 4, 80);
  const opScr = new THREE.Mesh(new THREE.PlaneGeometry(0.07, 0.14),
    new THREE.MeshBasicMaterial({ map: new THREE.CanvasTexture(opC) }));
  opScr.rotation.x = -Math.PI / 2;
  opScr.rotation.z = -0.4;
  opScr.position.set(HX - 6, -1.465, HZ - 0.5);
  group.add(opScr);
  HOUSE.oldPhoneScreen = opScr;
  // handwritten note
  const note = new THREE.Mesh(new THREE.PlaneGeometry(0.22, 0.3),
    new THREE.MeshLambertMaterial({ map: noteTexture() }));
  note.rotation.x = -Math.PI / 2;
  note.rotation.z = 0.3;
  note.position.set(HX - 5.7, -1.49, HZ - 0.25);
  group.add(note);
  HOUSE.p.hiddenRoom = new THREE.Vector3(HX - 6, -1.5, HZ - 0.5);

  scene.add(group);

  // ---------- GROUND ZONES (floor heights) ------------------------------
  const Z = WORLD.groundZones;
  // upstairs floor
  Z.push({
    priority: 2,
    contains: (x, z) => x > HX - 6 && x < HX + 6 && z > HZ - 5 && z < HZ + 1 &&
      !(x < HX - 4.2 && z > HZ - 4.8 && z < HZ - 0.2 && false),
    height: () => 3.1
  });
  // main stairs ramp
  Z.push({
    priority: 3,
    contains: (x, z) => x > HX - 5.6 && x < HX - 4.4 && z > HZ - 4.75 && z < HZ - 0.1,
    height: (x, z) => {
      const t = Math.min(1, Math.max(0, (-(z - HZ) - 0.2) / 4.4));
      return t * 3.1;
    }
  });
  // basement stairs ramp
  Z.push({
    priority: 3,
    contains: (x, z) => x > HX + 4.4 && x < HX + 5.6 && z > HZ - 4.75 && z < HZ - 0.25,
    height: (x, z) => {
      const t = Math.min(1, Math.max(0, (-(z - HZ) - 0.3) / 4.3));
      return t * -2.5;
    }
  });
  // basement floor
  Z.push({
    priority: 2,
    contains: (x, z) => x > HX - 7.5 && x < HX + 6 && z > HZ - 5 && z < HZ + 5,
    height: () => -2.5
  });

  // interaction points
  HOUSE.p.frontDoor = new THREE.Vector3(HX, 0, HZ + 5);
  HOUSE.p.kitchenDrawer = new THREE.Vector3(HX + 4.93, 0.62, HZ + 1.47); // 2nd drawer
  HOUSE.p.basementDoor = new THREE.Vector3(HX + 5.0, 0, HZ - 0.3);
  HOUSE.p.bedroomDoor = new THREE.Vector3(HX - 4.2, 3.1, HZ - 4.2);
  HOUSE.p.mirrorPos = mirror.position.clone();
  HOUSE.p.oldPhone = oldPhone.position.clone();
  HOUSE.p.note = note.position.clone();
}

export function openBasementDoor() {
  const d = HOUSE.basementDoor;
  d.position.x -= 0.95;
  d.rotation.y = 1.2;
  const c = HOUSE.basementDoorCollider;
  c.minX = c.maxX = -9999; c.minZ = c.maxZ = -9999;
}

export function openBedroomDoor() {
  const d = HOUSE.bedroomDoor;
  d.position.z += 0.9;
  d.rotation.y = 1.2;
  const c = HOUSE.bedroomDoorCollider;
  c.minX = c.maxX = -9999; c.minZ = c.maxZ = -9999;
}

export function openFalseWall() {
  const w = HOUSE.falseWall;
  w.position.y -= 2.1; // sinks into the floor
  const c = HOUSE.falseWallCollider;
  c.minX = c.maxX = -9999; c.minZ = c.maxZ = -9999;
}
