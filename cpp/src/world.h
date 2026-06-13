// PARANOIA v2 — world generation (terrain, forest, POIs, the well)
#pragma once
#include "game.h"

World gWorld;

float pathX(float z) { return 6.0f * sinf((200.0f - z) * 0.055f); }

static int AddItem(Mesh mesh, Matrix tr, Texture2D tex, Color tint, float spec = 0, bool shadow = true, float emis = 0, float fogMin = 0) {
  DrawItem d; d.mesh = mesh; d.transform = tr; d.tex = tex; d.tint = tint;
  d.specular = spec; d.castShadow = shadow; d.emis = emis; d.fogMin = fogMin;
  gWorld.items.push_back(d);
  return (int)gWorld.items.size() - 1;
}
static void AddCircleCol(float x, float z, float r) { gWorld.circles.push_back({ x, z, r }); }
static int AddBoxCol(float minX, float maxX, float minZ, float maxZ, float minY = -1e9f, float maxY = 1e9f) {
  gWorld.boxes.push_back({ minX, maxX, minZ, maxZ, minY, maxY, true });
  return (int)gWorld.boxes.size() - 1;
}

static Mesh BuildMesh(const std::vector<float>& v, const std::vector<float>& n, const std::vector<float>& uv, const std::vector<unsigned short>& idx) {
  Mesh m = { 0 };
  m.vertexCount = (int)v.size() / 3;
  m.triangleCount = (int)idx.size() / 3;
  m.vertices = (float*)RL_MALLOC(v.size() * sizeof(float));
  memcpy(m.vertices, v.data(), v.size() * sizeof(float));
  m.normals = (float*)RL_MALLOC(n.size() * sizeof(float));
  memcpy(m.normals, n.data(), n.size() * sizeof(float));
  m.texcoords = (float*)RL_MALLOC(uv.size() * sizeof(float));
  memcpy(m.texcoords, uv.data(), uv.size() * sizeof(float));
  m.indices = (unsigned short*)RL_MALLOC(idx.size() * sizeof(unsigned short));
  memcpy(m.indices, idx.data(), idx.size() * sizeof(unsigned short));
  UploadMesh(&m, false);
  return m;
}

// vertical textured quad (centered, facing +Z), for signs/murals/decals
static Mesh QuadMesh(float w, float h) {
  std::vector<float> v = { -w / 2, -h / 2, 0,  w / 2, -h / 2, 0,  w / 2, h / 2, 0,  -w / 2, h / 2, 0 };
  std::vector<float> n = { 0,0,1, 0,0,1, 0,0,1, 0,0,1 };
  std::vector<float> uv = { 0,1, 1,1, 1,0, 0,0 };
  std::vector<unsigned short> idx = { 0,1,2, 0,2,3 };
  return BuildMesh(v, n, uv, idx);
}

// ------------------------------------------------------------ textures
static Texture2D texGround, texDirt, texBark, texLeaf, texStone, texWood, texGrassT, texPale;
static Texture2D texEye, texMural, texSign, texSignCode, texWellSign, texPlaque, texNote, texOldPhone, texSkin;

static void DrawEllipseOnImage(Image* img, int cx, int cy, float rx, float ry, Color c, bool fill) {
  if (fill) {
    for (int y = (int)-ry; y <= (int)ry; y++)
      for (int x = (int)-rx; x <= (int)rx; x++)
        if ((x * x) / (rx * rx) + (y * y) / (ry * ry) <= 1.0f) ImageDrawPixel(img, cx + x, cy + y, c);
  } else {
    for (float a = 0; a < 6.3f; a += 0.02f)
      ImageDrawPixel(img, cx + (int)(cosf(a) * rx), cy + (int)(sinf(a) * ry), c);
  }
}

static void BuildTextures() {
  texGround = MakeNoiseTexture(256, Color{ 52, 58, 44, 255 }, 26);
  texDirt = MakeNoiseTexture(128, Color{ 92, 74, 52, 255 }, 24);
  texBark = MakeNoiseTexture(128, Color{ 58, 44, 32, 255 }, 22);
  texLeaf = MakeNoiseTexture(128, Color{ 26, 44, 30, 255 }, 18);
  texStone = MakeNoiseTexture(128, Color{ 96, 100, 92, 255 }, 20);
  texWood = MakeNoiseTexture(128, Color{ 72, 58, 40, 255 }, 22);
  texGrassT = MakeNoiseTexture(64, Color{ 34, 52, 34, 255 }, 22);
  texPale = MakeNoiseTexture(128, Color{ 168, 168, 178, 255 }, 14);
  texSkin = MakeNoiseTexture(128, Color{ 158, 122, 96, 255 }, 13);

  { // sky eye
    Image img = GenImageColor(128, 64, BLANK);
    DrawEllipseOnImage(&img, 64, 32, 54, 22, Color{ 185, 185, 195, 215 }, true);
    DrawEllipseOnImage(&img, 64, 32, 17, 17, Color{ 26, 26, 34, 255 }, true);
    DrawEllipseOnImage(&img, 64, 32, 7, 7, Color{ 2, 2, 4, 255 }, true);
    for (int i = 0; i < 7; i++)
      ImageDrawLine(&img, 14 + GetRandomValue(0, 30), 14 + GetRandomValue(0, 36), 48, 32, Color{ 120, 56, 56, 130 });
    texEye = LoadTextureFromImage(img); UnloadImage(img);
  }
  { // crossroad sign: weathered text + hidden code
    Image img = GenImageColor(256, 80, Color{ 70, 57, 42, 255 });
    for (int i = 0; i < 240; i++)
      ImageDrawRectangle(&img, GetRandomValue(0, 252), GetRandomValue(0, 72), 2, 8, Color{ 0, 0, 0, (unsigned char)GetRandomValue(15, 60) });
    ImageDrawText(&img, "CASA BLANCA >", 32, 16, 20, Color{ 200, 190, 170, 60 });
    ImageDrawText(&img, "el agua recuerda", 48, 48, 10, Color{ 200, 190, 170, 50 });
    texSign = LoadTextureFromImage(img); UnloadImage(img);

    Image code = GenImageColor(256, 80, BLANK);
    ImageDrawText(&code, "2 - 7 - 4 - 1", 48, 26, 30, Color{ 228, 222, 200, 245 });
    texSignCode = LoadTextureFromImage(code); UnloadImage(code);
  }
  { // well sign — the Ringu reference
    Image img = GenImageColor(256, 80, Color{ 56, 48, 40, 255 });
    for (int i = 0; i < 260; i++)
      ImageDrawRectangle(&img, GetRandomValue(0, 252), GetRandomValue(0, 72), 2, 9, Color{ 0, 0, 0, (unsigned char)GetRandomValue(20, 70) });
    ImageDrawText(&img, "< EL POZO", 50, 14, 20, Color{ 190, 178, 160, 95 });
    ImageDrawText(&img, "siete dias", 76, 48, 10, Color{ 150, 60, 56, 110 });
    texWellSign = LoadTextureFromImage(img); UnloadImage(img);
  }
  { // fountain plaque
    Image img = GenImageColor(192, 48, Color{ 58, 63, 54, 255 });
    ImageDrawText(&img, "AQVA OCVLI", 24, 8, 16, Color{ 180, 180, 170, 95 });
    ImageDrawText(&img, "SEMPER VIDENT", 18, 28, 14, Color{ 180, 180, 170, 80 });
    texPlaque = LoadTextureFromImage(img); UnloadImage(img);
  }
  { // mural — rows of eyes + warning
    Image img = GenImageColor(512, 256, Color{ 30, 28, 26, 255 });
    for (int row = 0; row < 3; row++)
      for (int col = 0; col < 7; col++) {
        int x = 40 + col * 68 + (row % 2) * 30, y = 30 + row * 56;
        DrawEllipseOnImage(&img, x, y, 24, 11, Color{ 172, 152, 132, 210 }, false);
        DrawEllipseOnImage(&img, x, y, 6, 6, Color{ 152, 32, 26, 190 }, true);
      }
    ImageDrawText(&img, "TRUST ME", 186, 196, 24, Color{ 202, 188, 162, 230 });
    ImageDrawText(&img, "DON'T LOOK BACK - YOU ARE BEING OBSERVED", 56, 230, 10, Color{ 202, 188, 162, 210 });
    texMural = LoadTextureFromImage(img); UnloadImage(img);
  }
  { // handwritten note
    Image img = GenImageColor(128, 160, Color{ 207, 201, 184, 255 });
    const char* lines[] = { "El smartphone", "es una puerta.", "", "Confia en el.", "O desconfia.", "", "Decide antes de", "que ellos decidan", "por ti." };
    for (int i = 0; i < 9; i++) ImageDrawText(&img, lines[i], 10, 14 + i * 15, 10, Color{ 42, 37, 32, 235 });
    texNote = LoadTextureFromImage(img); UnloadImage(img);
  }
  { // the original smartphone, cracked
    Image img = GenImageColor(64, 128, Color{ 5, 6, 8, 255 });
    for (int i = 0; i < 9; i++)
      ImageDrawLine(&img, 32, 64, GetRandomValue(0, 63), GetRandomValue(0, 127), Color{ 180, 180, 200, 120 });
    ImageDrawText(&img, "YOU ARE", 10, 44, 10, Color{ 140, 255, 160, 210 });
    ImageDrawText(&img, "THE", 22, 58, 10, Color{ 140, 255, 160, 210 });
    ImageDrawText(&img, "RECORDING", 4, 72, 10, Color{ 140, 255, 160, 210 });
    texOldPhone = LoadTextureFromImage(img); UnloadImage(img);
  }
}

// ------------------------------------------------------------ sky eyes
struct SkyEye { Vector3 pos; float w, blinkT, baseOp, rotZ, closing = 0; };
static std::vector<SkyEye> gEyes;

// dust motes drifting in the flashlight beam + falling leaves
struct Mote { Vector3 pos, vel; };
static std::vector<Mote> gMotes;
struct Leaf { Vector3 pos; float spin, fall; bool active = false; };
static std::vector<Leaf> gLeaves;

// ------------------------------------------------------------ builders
static Mesh mshCube, mshTrunk, mshCanopy, mshGrass, mshRock, mshPost, mshBranch, mshStone;

static void BuildForest() {
  mshTrunk = GenMeshCylinder(0.26f, 5.6f, 6);
  mshCanopy = GenMeshCone(1.9f, 4.2f, 7);
  mshRock = GenMeshSphere(0.5f, 5, 6);
  mshPost = GenMeshCube(0.12f, 1.1f, 0.12f);
  mshBranch = GenMeshCylinder(0.05f, 1.6f, 5);
  mshStone = GenMeshSphere(0.15f, 4, 5);
  { // crossed grass quads
    std::vector<float> v, n, uv; std::vector<unsigned short> idx;
    for (int q = 0; q < 2; q++) {
      float a = q * PI / 2 + 0.4f;
      float dx = cosf(a) * 0.22f, dz = sinf(a) * 0.22f;
      int b = q * 4;
      float quad[12] = { -dx, 0, -dz, dx, 0, dz, dx, 0.45f, dz, -dx, 0.45f, -dz };
      v.insert(v.end(), quad, quad + 12);
      for (int i = 0; i < 4; i++) { n.insert(n.end(), { 0, 1, 0 }); }
      uv.insert(uv.end(), { 0, 1, 1, 1, 1, 0, 0, 0 });
      idx.insert(idx.end(), { (unsigned short)b, (unsigned short)(b + 1), (unsigned short)(b + 2), (unsigned short)b, (unsigned short)(b + 2), (unsigned short)(b + 3) });
    }
    mshGrass = BuildMesh(v, n, uv, idx);
  }

  auto reject = [](float x, float z) {
    if (z > 58) { float d = fabsf(x - pathX(z)); return d < 4.5f || d > 42.0f; }
    if (Vector2Distance({ x, z }, { gWorld.fountain.x, gWorld.fountain.z }) < 7) return true;
    if (Vector2Distance({ x, z }, { gWorld.sign.x, gWorld.sign.z }) < 5) return true;
    if (Vector2Distance({ x, z }, { gWorld.cabin.x, gWorld.cabin.z }) < 9) return true;
    if (Vector2Distance({ x, z }, { gWorld.housePos.x, gWorld.housePos.z }) < 22) return true;
    if (Vector2Distance({ x, z }, { gWorld.well.x, gWorld.well.z }) < 6) return true;
    if (z < -88 && fabsf(x) < 12) return true;
    return false;
  };

  InstancedSet trunks{ mshTrunk, texBark, Color{ 200, 200, 200, 255 }, 0.04f };
  InstancedSet canopyA{ mshCanopy, texLeaf, Color{ 190, 200, 190, 255 }, 0.02f };
  InstancedSet canopyB{ mshCanopy, texLeaf, Color{ 160, 175, 160, 255 }, 0.02f };
  std::vector<Vector4> trees; // x z scale yaw
  for (int i = 0; i < 420; i++) { // phase 1 flanks, dense
    float z = 55 + frand() * 152;
    float side = frand() < 0.5f ? -1.0f : 1.0f;
    float x = pathX(z) + side * (5 + frand() * 34);
    if (!reject(x, z)) trees.push_back({ x, z, 0.75f + frand() * 0.8f, frand() * 6.28f });
  }
  for (int i = 0; i < 560; i++) { // forest, progressive density
    float z = -135 + frand() * 192;
    float x = -125 + frand() * 250;
    if (z > 58) continue;
    float density = z > 20 ? 0.6f : z > -40 ? 0.85f : z > -85 ? 1.0f : 0.4f;
    if (frand() > density) continue;
    if (!reject(x, z)) trees.push_back({ x, z, 0.75f + frand() * 0.8f, frand() * 6.28f });
  }
  for (auto& t : trees) {
    Matrix base = MTRS({ t.x, 0, t.y }, t.w, t.z);
    trunks.mats.push_back(base);
    canopyA.mats.push_back(MatrixMultiply(MatrixMultiply(MatrixScale(t.z, t.z, t.z), MatrixTranslate(0, 3.4f * t.z, 0)), MatrixMultiply(MatrixRotateY(t.w), MatrixTranslate(t.x, 0, t.y))));
    canopyB.mats.push_back(MatrixMultiply(MatrixMultiply(MatrixScale(t.z * 0.62f, t.z * 0.8f, t.z * 0.62f), MatrixTranslate(0, 5.6f * t.z, 0)), MatrixMultiply(MatrixRotateY(t.w + 0.5f), MatrixTranslate(t.x, 0, t.y))));
    AddCircleCol(t.x, t.y, 0.4f * t.z + 0.15f);
  }
  gWorld.instanced.push_back(trunks);
  gWorld.instanced.push_back(canopyA);
  gWorld.instanced.push_back(canopyB);

  InstancedSet grass{ mshGrass, texGrassT, Color{ 200, 215, 195, 255 }, 0.0f };
  grass.sway = true;
  for (int i = 0; i < 4200; i++) {
    float z = -140 + frand() * 350;
    float x = -130 + frand() * 260;
    if (z > 58 && fabsf(x - pathX(z)) < 2.2f) continue;       // keep path clear
    if (fabsf(x) < 7.5f && fabsf(z + 110) < 6.5f) continue;   // house
    if (Vector2Distance({ x, z }, { gWorld.cabin.x, gWorld.cabin.z }) < 3.5f) continue;
    float s = 0.6f + frand() * 1.1f;
    grass.mats.push_back(MTRS({ x, 0, z }, frand() * 6.28f, s));
  }
  gWorld.instanced.push_back(grass);

  InstancedSet bushes{ mshCanopy, texLeaf, Color{ 120, 135, 120, 255 }, 0.0f };
  for (int i = 0; i < 260; i++) {
    float z = -135 + frand() * 340;
    float x = -125 + frand() * 250;
    if (reject(x, z)) continue;
    if (z > 58 && fabsf(x - pathX(z)) < 3.4f) continue;
    bushes.mats.push_back(MTRS3({ x, -0.1f, z }, { frand2() * 0.15f, frand() * 6.28f, frand2() * 0.15f }, { 0.30f + frand() * 0.25f, 0.16f + frand() * 0.12f, 0.30f + frand() * 0.25f }));
  }
  gWorld.instanced.push_back(bushes);

  InstancedSet branches{ mshBranch, texBark, Color{ 150, 140, 130, 255 }, 0.0f };
  for (int i = 0; i < 130; i++) {
    float z = -130 + frand() * 330;
    float x = -120 + frand() * 240;
    if (z > 58 && fabsf(x - pathX(z)) < 2.5f) continue;
    branches.mats.push_back(MTRS3({ x, 0.04f, z }, { PI / 2 + frand2() * 0.2f, frand() * 6.28f, 0 }, { 1, 1, 1 }));
  }
  gWorld.instanced.push_back(branches);

  InstancedSet rocks{ mshRock, texStone, Color{ 170, 170, 165, 255 }, 0.05f };
  for (int i = 0; i < 80; i++) {
    float z = -130 + frand() * 330;
    float x = -120 + frand() * 240;
    if (z > 58 && fabsf(x - pathX(z)) < 4) continue;
    float s = 0.4f + frand() * 1.3f;
    rocks.mats.push_back(MTRS3({ x, 0.05f, z }, { 0, frand() * 6.28f, 0 }, { s, s * 0.65f, s }));
    if (s > 0.8f) AddCircleCol(x, z, s * 0.5f);
  }
  gWorld.instanced.push_back(rocks);

  InstancedSet posts{ mshPost, texWood, Color{ 165, 150, 135, 255 }, 0.0f };
  for (float z = 195; z >= 90; z -= 3.2f) {
    if (frand() < 0.25f) continue;
    float side = fmodf(z, 2.0f) > 1.0f ? 1.0f : -1.0f;
    posts.mats.push_back(MTRS3({ pathX(z) + side * 3.4f, 0.42f, z }, { frand2() * 0.4f, 0, frand2() * 0.4f }, { 1, 0.7f + frand() * 0.5f, 1 }));
  }
  gWorld.instanced.push_back(posts);

  InstancedSet stones{ mshStone, texPale, Color{ 230, 230, 222, 255 }, 0.08f };
  for (float z = 202; z >= 58; z -= 2.6f)
    for (int s = -1; s <= 1; s += 2)
      stones.mats.push_back(MTRS3({ pathX(z) + s * (1.9f + frand() * 0.3f), 0.08f, z + frand() }, { 0, frand() * 6.28f, 0 }, { 1, 0.8f, 1 }));
  gWorld.instanced.push_back(stones);
}

static void BuildTerrain() {
  // ground 700x700, gentle noise away from playable strip
  const int G = 70; const float SZ = 700;
  std::vector<float> v, n, uv; std::vector<unsigned short> idx;
  auto h = [](float x, float z) {
    float edge = fmaxf(0, (fabsf(x) - 110) / 80.0f) + fmaxf(0, (fabsf(z - 30) - 190) / 80.0f);
    float nn = sinf(x * 0.05f) * cosf(z * 0.06f) * 0.4f;
    return -0.02f + nn * fminf(1.0f, edge * 0.5f) + edge * edge * 6.0f;
  };
  for (int j = 0; j <= G; j++)
    for (int i = 0; i <= G; i++) {
      float x = -SZ / 2 + SZ * i / G, z = -SZ / 2 + SZ * j / G + 30;
      v.insert(v.end(), { x, h(x, z), z });
      float hx = h(x + 2, z) - h(x - 2, z), hz = h(x, z + 2) - h(x, z - 2);
      Vector3 nv = Vector3Normalize({ -hx, 4, -hz });
      n.insert(n.end(), { nv.x, nv.y, nv.z });
      uv.insert(uv.end(), { (float)i / G * 60, (float)j / G * 60 });
    }
  for (int j = 0; j < G; j++)
    for (int i = 0; i < G; i++) {
      int a = j * (G + 1) + i, b = a + 1, c = a + G + 1, d = c + 1;
      idx.insert(idx.end(), { (unsigned short)a, (unsigned short)c, (unsigned short)b, (unsigned short)b, (unsigned short)c, (unsigned short)d });
    }
  AddItem(BuildMesh(v, n, uv, idx), MatrixIdentity(), texGround, Color{ 185, 195, 175, 255 }, 0.0f, false);

  // dirt path strip
  std::vector<float> pv, pn, puv; std::vector<unsigned short> pidx;
  std::vector<Vector3> pts;
  for (float z = 205; z >= 55; z -= 2.5f) pts.push_back({ pathX(z), 0, z });
  for (float z = 55; z >= -88; z -= 4) pts.push_back({ pathX(55) * (z + 88) / 143.0f, 0, z });
  for (size_t i = 0; i < pts.size(); i++) {
    Vector3 p = pts[i];
    Vector3 dir = (i + 1 < pts.size()) ? Vector3Subtract(pts[i + 1], p) : Vector3Subtract(p, pts[i - 1]);
    Vector3 side = Vector3Scale(Vector3Normalize({ -dir.z, 0, dir.x }), 1.7f);
    pv.insert(pv.end(), { p.x - side.x, 0.04f, p.z - side.z, p.x + side.x, 0.04f, p.z + side.z });
    pn.insert(pn.end(), { 0, 1, 0, 0, 1, 0 });
    puv.insert(puv.end(), { 0, (float)i * 0.5f, 1, (float)i * 0.5f });
    if (i > 0) {
      unsigned short a = (unsigned short)((i - 1) * 2);
      pidx.insert(pidx.end(), { a, (unsigned short)(a + 1), (unsigned short)(a + 2), (unsigned short)(a + 1), (unsigned short)(a + 3), (unsigned short)(a + 2) });
    }
  }
  AddItem(BuildMesh(pv, pn, puv, pidx), MatrixIdentity(), texDirt, Color{ 200, 190, 175, 255 }, 0.0f, false);

  // world bounds
  AddBoxCol(-160, -140, -200, 230); AddBoxCol(140, 160, -200, 230);
  AddBoxCol(-160, 160, -200, -155); AddBoxCol(-160, 160, 212, 230);
  gWorld.zones.push_back({ 0, [](float, float) { return true; }, [](float, float) { return 0.0f; } });
}

static void BuildFountain() {
  Vector3 F = gWorld.fountain;
  Mesh basin = GenMeshCylinder(2.4f, 0.85f, 10);
  AddItem(basin, MatrixTranslate(F.x, 0, F.z), texStone, Color{ 150, 158, 144, 255 }, 0.08f);
  Mesh rim = GenMeshTorus(0.16f, 2.3f, 6, 10);
  AddItem(rim, MatrixMultiply(MatrixRotateX(PI / 2), MatrixTranslate(F.x, 0.88f, F.z)), texStone, Color{ 150, 158, 144, 255 }, 0.08f);
  Mesh pillar = GenMeshCylinder(0.26f, 1.6f, 7);
  AddItem(pillar, MatrixTranslate(F.x, 0.85f, F.z), texStone, Color{ 140, 148, 136, 255 }, 0.06f);
  Mesh bowl = GenMeshCylinder(0.8f, 0.35f, 9);
  AddItem(bowl, MatrixTranslate(F.x, 2.1f, F.z), texStone, Color{ 140, 148, 136, 255 }, 0.06f);
  Mesh water = GenMeshCylinder(2.1f, 0.04f, 12);
  AddItem(water, MatrixTranslate(F.x, 0.70f, F.z), gGfx.white, Color{ 14, 20, 26, 255 }, 0.9f, false);
  AddItem(QuadMesh(1.6f, 0.4f), MatrixTranslate(F.x, 0.55f, F.z + 2.52f), texPlaque, WHITE, 0.0f, false);
  AddCircleCol(F.x, F.z, 2.7f);
}

static void BuildSigns() {
  Vector3 S = gWorld.sign;
  AddItem(GenMeshCube(0.18f, 2.6f, 0.18f), MatrixTranslate(S.x, 1.3f, S.z), texWood, Color{ 150, 135, 118, 255 });
  AddCircleCol(S.x, S.z, 0.3f);
  AddItem(GenMeshCube(2.0f, 0.62f, 0.07f), MTRS3({ S.x, 1.9f, S.z + 0.10f }, { 0, 0, 0.04f }, { 1, 1, 1 }), texSign, WHITE);
  gWorld.signCodeItem = AddItem(QuadMesh(2.0f, 0.62f), MTRS3({ S.x, 1.9f, S.z + 0.155f }, { 0, 0, 0.04f }, { 1, 1, 1 }), texSignCode, Color{ 255, 255, 255, 0 }, 0, false);

  // well sign at the crossroad, pointing west — siete dias
  Vector3 W = { S.x - 3.0f, 0, S.z - 2.0f };
  AddItem(GenMeshCube(0.16f, 2.2f, 0.16f), MatrixTranslate(W.x, 1.1f, W.z), texWood, Color{ 140, 126, 110, 255 });
  AddCircleCol(W.x, W.z, 0.28f);
  AddItem(GenMeshCube(1.7f, 0.5f, 0.06f), MTRS3({ W.x, 1.7f, W.z + 0.09f }, { 0, -0.35f, -0.06f }, { 1, 1, 1 }), texWellSign, WHITE);
}

static void BuildWell() {
  Vector3 W = gWorld.well;
  // stone ring of 12 blocks
  for (int i = 0; i < 12; i++) {
    float a = i / 12.0f * 2 * PI;
    AddItem(GenMeshCube(0.85f, 0.9f, 0.45f),
      MTRS3({ W.x + cosf(a) * 1.45f, 0.45f, W.z + sinf(a) * 1.45f }, { 0, -a + PI / 2, 0 }, { 1, 1, 1 }),
      texStone, Color{ 130, 134, 126, 255 }, 0.05f);
  }
  // black void inside — it has no bottom
  AddItem(GenMeshCylinder(1.3f, 0.05f, 12), MatrixTranslate(W.x, 0.82f, W.z), gGfx.white, Color{ 0, 0, 0, 255 }, 0, false);
  // wooden frame + crossbar + rope
  AddItem(GenMeshCube(0.14f, 2.4f, 0.14f), MatrixTranslate(W.x - 1.5f, 1.2f, W.z), texWood, Color{ 130, 116, 100, 255 });
  AddItem(GenMeshCube(0.14f, 2.4f, 0.14f), MatrixTranslate(W.x + 1.5f, 1.2f, W.z), texWood, Color{ 130, 116, 100, 255 });
  AddItem(GenMeshCube(3.3f, 0.12f, 0.12f), MatrixTranslate(W.x, 2.35f, W.z), texWood, Color{ 130, 116, 100, 255 });
  AddItem(GenMeshCylinder(0.025f, 1.55f, 5), MatrixTranslate(W.x, 0.85f, W.z), texWood, Color{ 90, 82, 70, 255 }, 0, false);
  AddCircleCol(W.x, W.z, 1.95f);
}

static void BuildCabin() {
  float cx = gWorld.cabin.x, cz = gWorld.cabin.z;
  Color wall = { 148, 134, 116, 255 };
  auto mk = [&](float w, float h, float d, float x, float y, float z) {
    AddItem(GenMeshCube(w, h, d), MatrixTranslate(cx + x, y, cz + z), texWood, wall);
    AddBoxCol(cx + x - w / 2, cx + x + w / 2, cz + z - d / 2, cz + z + d / 2);
  };
  mk(5, 2.6f, 0.2f, 0, 1.3f, -2); mk(5, 2.6f, 0.2f, 0, 1.3f, 2);
  mk(0.2f, 2.6f, 4, 2.5f, 1.3f, 0);
  mk(0.2f, 2.6f, 1.5f, -2.5f, 1.3f, -1.25f); mk(0.2f, 2.6f, 1.5f, -2.5f, 1.3f, 1.25f);
  AddItem(GenMeshCube(0.2f, 0.6f, 1.0f), MatrixTranslate(cx - 2.5f, 2.3f, cz), texWood, wall);
  Mesh roof = GenMeshCone(4.2f, 1.6f, 4);
  AddItem(roof, MTRS({ cx, 2.6f, cz }, PI / 4, 1), texWood, Color{ 120, 108, 94, 255 });
  AddItem(GenMeshCube(5, 0.08f, 4), MatrixTranslate(cx, 0.04f, cz), texWood, Color{ 110, 100, 88, 255 }, 0, false);
  // table + dead charger
  AddItem(GenMeshCube(1.2f, 0.08f, 0.7f), MatrixTranslate(cx + 1.2f, 0.8f, cz - 0.8f), texWood, wall);
  AddItem(GenMeshCube(0.9f, 0.78f, 0.5f), MatrixTranslate(cx + 1.2f, 0.4f, cz - 0.8f), texWood, Color{ 95, 86, 74, 255 });
  AddItem(GenMeshCube(0.25f, 0.1f, 0.18f), MatrixTranslate(cx + 1.2f, 0.9f, cz - 0.8f), gGfx.white, Color{ 22, 30, 22, 255 }, 0.3f, true, 0.06f);
  AddBoxCol(cx + 0.6f, cx + 1.8f, cz - 1.15f, cz - 0.45f);
  // keypad
  AddItem(GenMeshCube(0.04f, 0.3f, 0.2f), MatrixTranslate(cx - 2.55f, 1.4f, cz + 0.85f), gGfx.white, Color{ 30, 36, 44, 255 }, 0.4f, false, 0.05f);
  // locked door
  gWorld.cabinDoor = AddItem(GenMeshCube(0.08f, 2.0f, 1.0f), MatrixTranslate(cx - 2.5f, 1.0f, cz), texWood, Color{ 132, 104, 76, 255 });
  gWorld.cabinDoorCol = AddBoxCol(cx - 2.6f, cx - 2.4f, cz - 0.5f, cz + 0.5f);
}

void OpenCabinDoor() {
  gWorld.items[gWorld.cabinDoor].transform = MTRS3({ gWorld.cabin.x - 2.5f, 1.0f, gWorld.cabin.z - 0.95f }, { 0, 0.9f, 0 }, { 1, 1, 1 });
  gWorld.boxes[gWorld.cabinDoorCol].enabled = false;
}

static void BuildSkyEyes() {
  for (int i = 0; i < 14; i++) {
    SkyEye e;
    e.pos = { -260 + frand() * 520, 110 + frand() * 90, -260 + frand() * 520 };
    e.w = 26 + frand() * 40;
    e.blinkT = frand() * 14;
    e.baseOp = 0.06f + frand() * 0.10f;
    e.rotZ = frand() * 6.28f;
    gEyes.push_back(e);
  }
  for (int i = 0; i < 90; i++) gMotes.push_back({ { 0, 0, 0 }, { frand2() * 0.06f, frand2() * 0.03f, frand2() * 0.06f } });
  gLeaves.resize(14);
}

void WorldTick(float dt, float time) {
  // motes drift near the player, recycle outside a 7m bubble
  Vector3 eye = PlayerEye();
  for (auto& m : gMotes) {
    m.pos = Vector3Add(m.pos, Vector3Scale(m.vel, dt * 8));
    if (Vector3Distance(m.pos, eye) > 7.0f) {
      Vector3 f = PlayerForward();
      m.pos = { eye.x + f.x * (1.5f + frand() * 4.5f) + frand2() * 2.2f,
                eye.y + frand2() * 1.6f,
                eye.z + f.z * (1.5f + frand() * 4.5f) + frand2() * 2.2f };
    }
  }
  for (auto& l : gLeaves) {
    if (!l.active) {
      if (frand() < dt * 0.10f && gPlayer.groundY > -0.5f) {
        Vector3 f = PlayerForward();
        l.pos = { gPlayer.pos.x + f.x * 5 + frand2() * 7, 5.5f + frand() * 2, gPlayer.pos.z + f.z * 5 + frand2() * 7 };
        l.spin = frand() * 6.28f; l.fall = 0.5f + frand() * 0.4f; l.active = true;
      }
    } else {
      l.pos.y -= l.fall * dt;
      l.pos.x += sinf(time * 2 + l.spin) * dt * 0.5f;
      l.spin += dt * 3;
      if (l.pos.y < 0.05f) l.active = false;
    }
  }
  for (auto& e : gEyes) {
    e.blinkT -= dt;
    if (e.blinkT < 0) { e.blinkT = 6 + frand() * 16; e.closing = 0.28f; }
    if (e.closing > 0) e.closing -= dt;
  }
}

void WorldBuild() {
  BuildTextures();
  BuildTerrain();
  BuildForest();
  BuildFountain();
  BuildSigns();
  BuildWell();
  BuildCabin();
  BuildSkyEyes();
  mshCube = GenMeshCube(1, 1, 1);
  HouseBuild();
}

// ------------------------------------------------------------ draw
static Material gMatItem, gMatInst;
static bool gMatInit = false;

void WorldDraw(bool shadowPass, float time) {
  if (!gMatInit) { gMatItem = LoadMaterialDefault(); gMatInst = LoadMaterialDefault(); gMatInit = true; }
  Shader shItem = shadowPass ? gGfx.depth : gGfx.world;
  Shader shInst = shadowPass ? gGfx.depthInst : gGfx.worldInst;
  gMatItem.shader = shItem;
  gMatInst.shader = shInst;

  for (auto& d : gWorld.items) {
    if (shadowPass && !d.castShadow) continue;
    if (!shadowPass) {
      SetShaderValue(gGfx.world, ULoc[0].specK, &d.specular, SHADER_UNIFORM_FLOAT);
      int le = GetShaderLocation(gGfx.world, "emisK");
      int lf = GetShaderLocation(gGfx.world, "fogMin");
      SetShaderValue(gGfx.world, le, &d.emis, SHADER_UNIFORM_FLOAT);
      SetShaderValue(gGfx.world, lf, &d.fogMin, SHADER_UNIFORM_FLOAT);
    }
    gMatItem.maps[MATERIAL_MAP_ALBEDO].texture = d.tex;
    gMatItem.maps[MATERIAL_MAP_ALBEDO].color = d.tint;
    DrawMesh(d.mesh, gMatItem, d.transform);
  }
  float zero = 0, one = 0;
  if (!shadowPass) {
    int le = GetShaderLocation(gGfx.world, "emisK");
    int lf = GetShaderLocation(gGfx.world, "fogMin");
    SetShaderValue(gGfx.world, le, &zero, SHADER_UNIFORM_FLOAT);
    SetShaderValue(gGfx.world, lf, &one, SHADER_UNIFORM_FLOAT);
    int lei = GetShaderLocation(gGfx.worldInst, "emisK");
    int lfi = GetShaderLocation(gGfx.worldInst, "fogMin");
    SetShaderValue(gGfx.worldInst, lei, &zero, SHADER_UNIFORM_FLOAT);
    SetShaderValue(gGfx.worldInst, lfi, &zero, SHADER_UNIFORM_FLOAT);
  }
  for (auto& s : gWorld.instanced) {
    if (!shadowPass) {
      SetShaderValue(gGfx.worldInst, ULoc[1].specK, &s.specular, SHADER_UNIFORM_FLOAT);
      SetSway(1, s.sway ? 1 : 0);
    } else {
      int lsw = GetShaderLocation(gGfx.depthInst, "uSway");
      int sw = s.sway ? 1 : 0;
      SetShaderValue(gGfx.depthInst, lsw, &sw, SHADER_UNIFORM_INT);
      int lt = GetShaderLocation(gGfx.depthInst, "uTime");
      SetShaderValue(gGfx.depthInst, lt, &time, SHADER_UNIFORM_FLOAT);
    }
    gMatInst.maps[MATERIAL_MAP_ALBEDO].texture = s.tex;
    gMatInst.maps[MATERIAL_MAP_ALBEDO].color = s.tint;
    DrawMeshInstanced(s.mesh, gMatInst, s.mats.data(), (int)s.mats.size());
  }

  if (!shadowPass) {
    // sky eyes — unlit, blinking, watching
    for (auto& e : gEyes) {
      float sy = 1.0f;
      if (e.closing > 0) sy = fmaxf(0.05f, fabsf(sinf((e.closing / 0.28f) * PI)));
      float op = e.baseOp + (gDir.stress / 100.0f) * 0.15f;
      Rectangle src = { 0, 0, (float)texEye.width, (float)texEye.height };
      DrawBillboardPro(gPlayer.cam, texEye, src, e.pos, { 0, 1, 0 }, { e.w, e.w / 2 * sy }, { e.w / 2, e.w / 4 * sy }, e.rotZ * RAD2DEG * 0.06f, Fade(WHITE, op));
    }
    // dust motes (lit only by the beam — correct: they use the world shader's light? they
    // are drawn unlit tiny; keep them subtle gray so dithering shows them in the beam)
    for (auto& m : gMotes) DrawCubeV(m.pos, { 0.012f, 0.012f, 0.012f }, Fade(Color{ 200, 205, 215, 255 }, 0.16f));
    for (auto& l : gLeaves) if (l.active) DrawCubeV(l.pos, { 0.09f, 0.012f, 0.07f }, Fade(Color{ 90, 80, 55, 255 }, 0.8f));
  }
}
