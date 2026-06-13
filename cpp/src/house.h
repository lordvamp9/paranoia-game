// PARANOIA v2 — the white house (exterior, interior, basement, hidden room)
// Center (0,-110). Front door faces +Z. Same floor plan as v1.
#pragma once
#include "game.h"

static const float HX = 0, HZ = -110;
static int gMirrorItem = -1, gMirrorFigItem = -1;
static float gMirrorFigOpacity = 0;

void HouseBuild() {
  Color extC = { 205, 205, 215, 255 };
  Color wallC = { 168, 164, 156, 255 };
  Color woodC = { 120, 100, 80, 255 };
  Texture2D texWall = MakeNoiseTexture(128, Color{ 120, 118, 112, 255 }, 12);
  Texture2D texExt = MakeNoiseTexture(128, Color{ 150, 150, 158, 255 }, 8);
  Texture2D texFloor = MakeNoiseTexture(128, Color{ 86, 80, 72, 255 }, 16);

  // exterior: unreal white — emissive, never fully fogged (visible from afar)
  auto ext = [&](float w, float h, float d, float x, float y, float z) {
    AddItem(GenMeshCube(w, h, d), MatrixTranslate(HX + x, y, HZ + z), texExt, extC, 0.05f, true, 0.30f, 0.42f);
  };
  // front wall: a wide doorway, x[-1.0,1.0], so you actually fit through
  ext(4.0f, 5.6f, 0.25f, -3.95f, 2.8f, 5); ext(4.0f, 5.6f, 0.25f, 3.95f, 2.8f, 5);
  ext(2.2f, 3.0f, 0.25f, 0, 4.1f, 5); // lintel above the door
  ext(12, 5.6f, 0.25f, 0, 2.8f, -5);
  ext(0.25f, 5.6f, 10, -6, 2.8f, 0); ext(0.25f, 5.6f, 10, 6, 2.8f, 0);
  AddBoxCol(HX - 6.1f, HX - 1.05f, HZ + 4.85f, HZ + 5.15f); AddBoxCol(HX + 1.05f, HX + 6.1f, HZ + 4.85f, HZ + 5.15f);
  AddBoxCol(HX - 6.1f, HX + 6.1f, HZ - 5.2f, HZ - 4.8f);
  AddBoxCol(HX - 6.2f, HX - 5.85f, HZ - 5, HZ + 5); AddBoxCol(HX + 5.85f, HX + 6.2f, HZ - 5, HZ + 5);
  // roof + chimney (asymmetric on purpose)
  Mesh roof = GenMeshCone(8.6f, 2.6f, 4);
  AddItem(roof, MTRS3({ HX + 0.2f, 5.6f, HZ }, { 0, PI / 4 + 0.05f, 0 }, { 1.18f, 1, 0.95f }), texExt, Color{ 188, 188, 198, 255 }, 0.04f, true, 0.26f, 0.42f);
  AddItem(GenMeshCube(0.7f, 1.8f, 0.7f), MTRS3({ HX + 3.1f, 7.0f, HZ - 1.2f }, { 0, 0.06f, 0 }, { 1, 1, 1 }), texExt, Color{ 175, 175, 185, 255 }, 0, true, 0.22f, 0.42f);
  // windows — black voids, misaligned
  float wins[8][4] = { { -3.9f, 1.6f, 5.14f, 0 }, { 2.8f, 1.75f, 5.14f, 0 }, { 4.6f, 1.5f, 5.14f, 0 }, { -5.0f, 1.65f, 5.14f, 0 },
                       { -2.6f, 4.3f, 5.14f, 0.03f }, { 3.3f, 4.45f, 5.14f, -0.04f }, { 6.14f, 1.6f, -1, PI / 2 }, { -6.14f, 1.7f, 1.5f, -PI / 2 } };
  for (auto& w : wins)
    AddItem(QuadMesh(0.9f, 1.2f), MTRS3({ HX + w[0], w[1], HZ + w[2] }, { 0, w[3], w[3] == 0.03f || w[3] == -0.04f ? w[3] : 0 }, { 1, 1, 1 }), gGfx.white, Color{ 2, 2, 5, 255 }, 0, false, 0, 0.42f);
  // front door — dark red, swung wide open (hinge on its left edge at x=-1)
  gWorld.frontDoor = AddItem(GenMeshCube(2.0f, 2.6f, 0.08f),
    MatrixMultiply(MatrixMultiply(MatrixTranslate(1.0f, 0, 0), MatrixRotateY(-1.45f)), MatrixTranslate(HX - 1.0f, 1.3f, HZ + 5)),
    texWood, Color{ 96, 30, 24, 255 }, 0.10f);
  // yard fence, broken
  for (int i = 0; i < 14; i++) {
    if (frand() < 0.25f) continue;
    float a = i / 14.0f * 2 * PI;
    AddItem(GenMeshCube(0.1f, 0.9f, 0.1f), MTRS3({ HX + cosf(a) * 10.5f, 0.45f, HZ + sinf(a) * 9 }, { 0, 0, frand2() * 0.3f }, { 1, 1, 1 }), texWood, Color{ 150, 150, 148, 255 });
  }

  // ---------- interior ----------
  auto mkInt = [&](float w, float h, float d, float x, float y, float z, Color c, bool collide, float minY = -1e9f, float maxY = 1e9f) {
    AddItem(GenMeshCube(w, h, d), MatrixTranslate(HX + x, y, HZ + z), texWall, c);
    if (collide) AddBoxCol(HX + x - w / 2, HX + x + w / 2, HZ + z - d / 2, HZ + z + d / 2, minY, maxY);
  };
  // floors
  AddItem(GenMeshCube(12, 0.1f, 10), MatrixTranslate(HX, 0.02f, HZ), texFloor, Color{ 200, 195, 188, 255 }, 0.03f, false);
  // upstairs slab with stairwell hole (x[-6,-4.2] z[-4.8,-0.2])
  mkInt(12, 0.18f, 4.0f, 0, 3.0f, 3.0f, Color{ 165, 160, 152, 255 }, false);
  mkInt(12, 0.18f, 1.2f, 0, 3.0f, 0.4f, Color{ 165, 160, 152, 255 }, false);
  mkInt(10.2f, 0.18f, 4.6f, 0.9f, 3.0f, -2.5f, Color{ 165, 160, 152, 255 }, false);
  mkInt(12, 0.18f, 0.2f, 0, 3.0f, -4.9f, Color{ 165, 160, 152, 255 }, false);
  // wall living/kitchen x=2 (z 0..5), opening z[2,3.2]
  mkInt(0.15f, 3, 2.0f, 2, 1.5f, 1.0f, wallC, true);
  mkInt(0.15f, 3, 1.8f, 2, 1.5f, 4.1f, wallC, true);
  // hall wall z=0, openings x[-4,-2.8] and x[3,4.2]
  mkInt(2.0f, 3, 0.15f, -5.0f, 1.5f, 0, wallC, true);
  mkInt(5.8f, 3, 0.15f, 0.1f, 1.5f, 0, wallC, true);
  mkInt(1.8f, 3, 0.15f, 5.1f, 1.5f, 0, wallC, true);

  // living room
  mkInt(2.2f, 0.75f, 0.9f, -3.5f, 0.38f, 3.9f, Color{ 120, 120, 126, 255 }, true);
  mkInt(2.2f, 0.5f, 0.25f, -3.5f, 0.95f, 4.35f, Color{ 108, 108, 116, 255 }, false);
  mkInt(1.1f, 0.45f, 0.7f, -3.4f, 0.23f, 2.2f, woodC, true);
  AddItem(GenMeshCylinder(0.04f, 1.5f, 5), MTRS3({ HX - 5.3f, 0.4f, HZ + 1.0f }, { 0, 0, 0.5f }, { 1, 1, 1 }), texWood, woodC); // broken lamp
  for (int i = 0; i < 3; i++) { // empty frames
    float fx = (i == 0) ? -2.0f : (i == 1) ? -4.5f : 0.5f, fy = (i == 1) ? 1.65f : 1.8f + i * 0.05f;
    AddItem(GenMeshCube(0.6f, 0.8f, 0.04f), MTRS3({ HX + fx, fy, HZ + 4.9f }, { 0, 0, frand2() * 0.1f }, { 1, 1, 1 }), texWood, woodC);
    AddItem(QuadMesh(0.46f, 0.66f), MTRS3({ HX + fx, fy, HZ + 4.86f }, { 0, PI, 0 }, { 1, 1, 1 }), gGfx.white, Color{ 16, 14, 12, 255 }, 0, false);
  }

  // kitchen
  mkInt(0.9f, 1.9f, 0.8f, 5.4f, 0.95f, 4.4f, Color{ 145, 145, 142, 255 }, true);  // fridge
  mkInt(0.9f, 0.95f, 0.8f, 5.4f, 0.48f, 3.3f, Color{ 122, 120, 112, 255 }, true); // stove
  mkInt(0.9f, 0.9f, 2.4f, 5.4f, 0.45f, 1.5f, woodC, true);                        // counters
  for (int i = 0; i < 3; i++) { // drawers; #1 (middle) hides the kitchen key
    int idx = AddItem(GenMeshCube(0.06f, 0.22f, 0.6f), MatrixTranslate(HX + 4.93f, 0.62f, HZ + 0.75f + i * 0.72f), texWood, Color{ 105, 88, 66, 255 });
    if (i == 1) gWorld.drawer2 = idx;
  }
  mkInt(0.9f, 0.72f, 0.9f, 3.0f, 0.36f, 3.8f, woodC, true); // small table
  AddItem(GenMeshCylinder(0.18f, 0.02f, 8), MatrixTranslate(HX + 3.0f, 0.73f, HZ + 3.8f), gGfx.white, Color{ 70, 68, 40, 255 }, 0, false); // rotten plate

  // hall + main stairs (x[-5.6,-4.4], z -0.2 -> -4.6 rise 0->3)
  for (int i = 0; i < 12; i++)
    AddItem(GenMeshCube(1.2f, 0.25f, 0.38f), MatrixTranslate(HX - 5.0f, 0.125f + i * 0.25f, HZ - 0.4f - i * 0.367f), texWood, Color{ 128, 116, 100, 255 });
  AddBoxCol(HX - 4.45f, HX - 4.3f, HZ - 4.6f, HZ - 0.2f); // railing
  AddItem(GenMeshCube(0.08f, 0.12f, 4.4f), MTRS3({ HX - 4.38f, 1.9f, HZ - 2.4f }, { -0.6f, 0, 0 }, { 1, 1, 1 }), texWood, woodC);

  // basement stairs (x[4.4,5.6], z -0.3 -> -4.6 drop 0->-2.5)
  for (int i = 0; i < 11; i++)
    AddItem(GenMeshCube(1.2f, 0.25f, 0.4f), MatrixTranslate(HX + 5.0f, -0.23f - i * 0.227f, HZ - 0.55f - i * 0.39f), texWood, Color{ 116, 106, 92, 255 });
  mkInt(0.15f, 3, 4.6f, 4.4f, 1.5f, -2.5f, wallC, true); // stairwell wall
  // basement door (locked, cold metal)
  gWorld.basementDoor = AddItem(GenMeshCube(1.1f, 2.1f, 0.09f), MatrixTranslate(HX + 5.0f, 1.05f, HZ - 0.3f), gGfx.white, Color{ 64, 82, 94, 255 }, 0.25f);
  gWorld.basementDoorCol = AddBoxCol(HX + 4.4f, HX + 5.6f, HZ - 0.42f, HZ - 0.18f);

  // upstairs: bedroom wall x=-4.2 (z -5..1), opening z[-4.7,-3.7]
  AddItem(GenMeshCube(0.15f, 2.9f, 4.9f), MatrixTranslate(HX - 4.2f, 4.55f, HZ - 1.25f), texWall, wallC);
  AddBoxCol(HX - 4.3f, HX - 4.1f, HZ - 3.7f, HZ + 1, 2.5f);
  AddItem(GenMeshCube(0.15f, 0.8f, 1.0f), MatrixTranslate(HX - 4.2f, 5.6f, HZ - 4.2f), texWall, wallC);
  gWorld.bedroomDoor = AddItem(GenMeshCube(0.09f, 2.1f, 1.0f), MatrixTranslate(HX - 4.2f, 4.1f, HZ - 4.2f), texWood, Color{ 122, 92, 64, 255 });
  gWorld.bedroomDoorCol = AddBoxCol(HX - 4.32f, HX - 4.08f, HZ - 4.72f, HZ - 3.68f, 2.5f);
  AddItem(GenMeshCube(12, 2.9f, 0.15f), MatrixTranslate(HX, 4.55f, HZ + 1), texWall, wallC); // upstairs south wall
  AddBoxCol(HX - 6, HX + 6, HZ + 0.9f, HZ + 1.1f, 2.5f);

  // bedroom furniture
  AddItem(GenMeshCube(1.6f, 0.5f, 2.2f), MatrixTranslate(HX - 1.5f, 3.35f, HZ - 3.6f), texWall, Color{ 142, 142, 138, 255 }); // bed
  AddBoxCol(HX - 2.3f, HX - 0.7f, HZ - 4.7f, HZ - 2.5f, 2.5f);
  AddItem(GenMeshCube(0.7f, 2.2f, 1.6f), MatrixTranslate(HX + 5.5f, 4.2f, HZ - 2.0f), texWood, Color{ 88, 70, 50, 255 }); // wardrobe
  AddBoxCol(HX + 5.1f, HX + 6, HZ - 2.8f, HZ - 1.2f, 2.5f);
  gMirrorItem = AddItem(QuadMesh(0.9f, 1.7f), MTRS3({ HX + 5.12f, 4.2f, HZ - 2.0f }, { 0, -PI / 2, 0 }, { 1, 1, 1 }), gGfx.white, Color{ 40, 50, 60, 255 }, 0.95f, false);
  // the figure "inside" the mirror
  gMirrorFigItem = AddItem(GenMeshCube(0.34f, 1.4f, 0.05f), MTRS3({ HX + 5.06f, 4.15f, HZ - 2.0f }, { 0, -PI / 2, 0 }, { 1, 1, 1 }), gGfx.white, Color{ 0, 0, 0, 0 }, 0, false);
  AddItem(GenMeshCube(1.2f, 0.78f, 0.6f), MatrixTranslate(HX + 2.5f, 3.5f, HZ - 4.6f), texWood, woodC); // desk
  AddItem(GenMeshCube(1.4f, 2.2f, 0.1f), MatrixTranslate(HX + 0.5f, 4.3f, HZ + 0.92f), texWall, Color{ 62, 62, 70, 255 }); // curtain
  AddItem(GenMeshCube(2.2f, 0.02f, 1.6f), MatrixTranslate(HX + 1.2f, 3.12f, HZ - 2.0f), texWall, Color{ 70, 76, 96, 255 }, 0, false); // rug

  // ---------- basement (y -2.5, x -7.5..6, entities never enter) ----------
  Color bWall = { 125, 120, 110, 255 };
  AddItem(GenMeshCube(13.5f, 0.1f, 10), MatrixTranslate(HX - 0.75f, -2.55f, HZ), texFloor, Color{ 130, 126, 118, 255 }, 0, false);
  AddItem(GenMeshCube(13.5f, 0.1f, 10), MatrixTranslate(HX - 0.75f, -0.25f, HZ), texFloor, Color{ 110, 106, 100, 255 }, 0, false);
  auto mkB = [&](float w, float h, float d, float x, float y, float z, bool col = true) {
    AddItem(GenMeshCube(w, h, d), MatrixTranslate(HX + x, y, HZ + z), texWall, bWall);
    if (col) AddBoxCol(HX + x - w / 2, HX + x + w / 2, HZ + z - d / 2, HZ + z + d / 2, -1e9f, -0.5f);
  };
  mkB(13.5f, 2.4f, 0.2f, -0.75f, -1.4f, 5);
  mkB(13.5f, 2.4f, 0.2f, -0.75f, -1.4f, -5);
  mkB(0.2f, 2.4f, 10, -7.5f, -1.4f, 0);
  mkB(0.2f, 2.4f, 10, 6, -1.4f, 0);
  mkB(0.2f, 2.4f, 8.2f, 4.4f, -1.4f, 0.9f); // seals corridor except entry
  for (int i = 0; i < 3; i++) { // rusty pipes
    float px2[3] = { -2, 1, 3.5f };
    AddItem(GenMeshCylinder(0.08f, 9, 6), MTRS3({ HX + px2[i] - 4.5f, -0.5f, HZ - 4.6f }, { 0, 0, -PI / 2 }, { 1, 1, 1 }), texWood, Color{ 110, 88, 70, 255 }, 0.15f);
  }
  mkB(1.0f, 0.8f, 1.0f, 2.5f, -2.1f, 3.5f); mkB(0.8f, 0.6f, 0.8f, 3.6f, -2.2f, 3.2f); // boxes
  AddItem(GenMeshCube(1.8f, 1.6f, 1.0f), MatrixTranslate(HX - 2.5f, -1.7f, HZ + 4.2f), texWall, Color{ 96, 104, 94, 255 }, 0.2f); // dead machine
  AddBoxCol(HX - 3.4f, HX - 1.6f, HZ + 3.7f, HZ + 4.7f, -1e9f, -0.5f);
  for (int i = 0; i < 5; i++) // hanging cables
    AddItem(GenMeshCylinder(0.015f, 0.7f + frand() * 0.5f, 4), MTRS3({ HX - 3 + frand() * 6, -1.05f, HZ - 3 + frand() * 6 }, { frand2() * 0.3f, 0, 0 }, { 1, 1, 1 }), gGfx.white, Color{ 30, 30, 30, 255 });
  // MURAL
  AddItem(QuadMesh(6.5f, 2.0f), MatrixTranslate(HX - 1, -1.45f, HZ - 4.88f), texMural, WHITE, 0, false);

  // false wall (west, hides the hidden room) — opening z[-1,0.2]
  mkB(0.2f, 2.4f, 3.8f, -4, -1.4f, -3.1f);
  mkB(0.2f, 2.4f, 4.6f, -4, -1.4f, 2.5f);
  gWorld.falseWall = AddItem(GenMeshCube(0.22f, 2.4f, 1.3f), MatrixTranslate(HX - 4, -1.4f, HZ - 0.4f), texWall, bWall);
  gWorld.falseWallCol = AddBoxCol(HX - 4.12f, HX - 3.88f, HZ - 1.05f, HZ + 0.25f, -1e9f, -0.5f);

  // hidden room: pedestal + the original smartphone + note
  mkB(3.5f, 2.4f, 0.2f, -5.75f, -1.4f, -2.5f);
  mkB(3.5f, 2.4f, 0.2f, -5.75f, -1.4f, 1.5f);
  AddItem(GenMeshCube(0.5f, 1.0f, 0.5f), MatrixTranslate(HX - 6, -2.0f, HZ - 0.5f), texStone, bWall);
  AddItem(GenMeshCube(0.08f, 0.02f, 0.16f), MTRS({ HX - 6, -1.48f, HZ - 0.5f }, 0.4f, 1), gGfx.white, Color{ 18, 18, 20, 255 }, 0.5f);
  Mesh scr = QuadMesh(0.07f, 0.14f);
  AddItem(scr, MTRS3({ HX - 6, -1.465f, HZ - 0.5f }, { -PI / 2, 0.4f, 0 }, { 1, 1, 1 }), texOldPhone, WHITE, 0, false, 0.35f);
  AddItem(QuadMesh(0.22f, 0.3f), MTRS3({ HX - 5.7f, -1.486f, HZ - 0.25f }, { -PI / 2, 0.3f, 0 }, { 1, 1, 1 }), texNote, WHITE, 0, false);
  // battery pickup glints
  gWorld.baseBatteryItem = AddItem(GenMeshCube(0.07f, 0.12f, 0.07f), MatrixTranslate(gWorld.basementBattery.x, gWorld.basementBattery.y, gWorld.basementBattery.z), gGfx.white, Color{ 30, 160, 70, 255 }, 0.5f, true, 0.10f);
  gWorld.wellBatteryItem = AddItem(GenMeshCube(0.07f, 0.12f, 0.07f), MatrixTranslate(gWorld.wellBattery.x, 0.10f, gWorld.wellBattery.z), gGfx.white, Color{ 30, 160, 70, 255 }, 0.5f, true, 0.10f);
  // water key at the fountain rim (visible only with light — it is tiny and dark)
  gWorld.waterKeyItem = AddItem(GenMeshCube(0.16f, 0.02f, 0.05f), MTRS({ gWorld.waterKeyP.x, gWorld.waterKeyP.y, gWorld.waterKeyP.z }, 0.7f, 1), gGfx.white, Color{ 124, 96, 64, 255 }, 0.6f);

  // ---------- ground zones ----------
  gWorld.zones.push_back({ 2,
    [](float x, float z) { return x > HX - 6 && x < HX + 6 && z > HZ - 5 && z < HZ + 1; },
    [](float, float) { return 3.1f; } });
  gWorld.zones.push_back({ 3,
    [](float x, float z) { return x > HX - 5.6f && x < HX - 4.4f && z > HZ - 4.75f && z < HZ - 0.1f; },
    [](float, float z) { float t = Clamp((-(z - HZ) - 0.2f) / 4.4f, 0.0f, 1.0f); return t * 3.1f; } });
  gWorld.zones.push_back({ 3,
    [](float x, float z) { return x > HX + 4.4f && x < HX + 5.6f && z > HZ - 4.75f && z < HZ - 0.25f; },
    [](float, float z) { float t = Clamp((-(z - HZ) - 0.3f) / 4.3f, 0.0f, 1.0f); return t * -2.5f; } });
  gWorld.zones.push_back({ 2,
    [](float x, float z) { return x > HX - 7.5f && x < HX + 6 && z > HZ - 5 && z < HZ + 5; },
    [](float, float) { return -2.5f; } });
}

void OpenBasementDoor() {
  gWorld.items[gWorld.basementDoor].transform = MTRS3({ HX + 4.05f, 1.05f, HZ - 0.3f }, { 0, 1.2f, 0 }, { 1, 1, 1 });
  gWorld.boxes[gWorld.basementDoorCol].enabled = false;
}
void OpenBedroomDoor() {
  gWorld.items[gWorld.bedroomDoor].transform = MTRS3({ HX - 4.2f, 4.1f, HZ - 3.3f }, { 0, 1.2f, 0 }, { 1, 1, 1 });
  gWorld.boxes[gWorld.bedroomDoorCol].enabled = false;
}
void OpenFalseWall() {
  gWorld.items[gWorld.falseWall].transform = MatrixTranslate(HX - 4, -3.5f, HZ - 0.4f); // sinks into the floor
  gWorld.boxes[gWorld.falseWallCol].enabled = false;
}
void OpenDrawer() {
  gWorld.items[gWorld.drawer2].transform = MatrixTranslate(HX + 4.93f + 0.28f, 0.62f, HZ + 0.75f + 0.72f);
}
void SetMirrorFigure(float opacity) {
  gMirrorFigOpacity = opacity;
  gWorld.items[gMirrorFigItem].tint = Color{ 5, 5, 8, (unsigned char)(opacity * 255) };
}

// restore all dynamic world objects to their initial (closed/present) state
void WorldResetDynamics() {
  gWorld.items[gWorld.cabinDoor].transform = MatrixTranslate(gWorld.cabin.x - 2.5f, 1.0f, gWorld.cabin.z);
  gWorld.boxes[gWorld.cabinDoorCol].enabled = true;
  gWorld.items[gWorld.basementDoor].transform = MatrixTranslate(HX + 5.0f, 1.05f, HZ - 0.3f);
  gWorld.boxes[gWorld.basementDoorCol].enabled = true;
  gWorld.items[gWorld.bedroomDoor].transform = MatrixTranslate(HX - 4.2f, 4.1f, HZ - 4.2f);
  gWorld.boxes[gWorld.bedroomDoorCol].enabled = true;
  gWorld.items[gWorld.falseWall].transform = MatrixTranslate(HX - 4, -1.4f, HZ - 0.4f);
  gWorld.boxes[gWorld.falseWallCol].enabled = true;
  gWorld.items[gWorld.drawer2].transform = MatrixTranslate(HX + 4.93f, 0.62f, HZ + 0.75f + 0.72f);
  gWorld.items[gWorld.waterKeyItem].tint.a = 255;
  gWorld.items[gWorld.wellBatteryItem].tint.a = 255;
  gWorld.items[gWorld.baseBatteryItem].tint.a = 255;
  SetMirrorFigure(0);
  gWorld.signCodeAlpha = 0;
  gWorld.items[gWorld.signCodeItem].tint.a = 0;
}
