// PARANOIA v2 — native C++/raylib port. a game by vamp9.
// Shared state, constants and declarations for the unity build.
#pragma once
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <vector>
#include <string>
#include <functional>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <algorithm>

// ---------------------------------------------------------------- constants
#define INTERNAL_W 640
#define INTERNAL_H 360
#define PHONE_SCR_W 384
#define PHONE_SCR_H 768

static inline float frand() { return (float)GetRandomValue(0, 10000) / 10000.0f; }
static inline float frand2() { return frand() * 2.0f - 1.0f; }

// ---------------------------------------------------------------- world data
struct CircleCol { float x, z, r; };
struct BoxCol { float minX, maxX, minZ, maxZ; float minY = -1e9f, maxY = 1e9f; bool enabled = true; };
struct GroundZone { int priority; std::function<bool(float, float)> contains; std::function<float(float, float)> height; };

struct DrawItem { // one mesh + material-ish params drawn with the world shader
  Mesh mesh; Matrix transform; Texture2D tex; Color tint;
  float specular = 0; bool castShadow = true;
  float emis = 0, fogMin = 0;
};
struct InstancedSet { Mesh mesh; Texture2D tex; Color tint; float specular = 0; std::vector<Matrix> mats; bool sway = false; };

struct World {
  std::vector<CircleCol> circles;
  std::vector<BoxCol> boxes;
  std::vector<GroundZone> zones;
  std::vector<DrawItem> items;
  std::vector<InstancedSet> instanced;
  // dynamic door meshes (index into items)
  int cabinDoor = -1, basementDoor = -1, bedroomDoor = -1, falseWall = -1, frontDoor = -1;
  int cabinDoorCol = -1, basementDoorCol = -1, bedroomDoorCol = -1, falseWallCol = -1;
  int signCodeItem = -1;        // hidden 2741 decal
  int drawer2 = -1;             // kitchen drawer mesh index
  int waterKeyItem = -1, wellBatteryItem = -1, baseBatteryItem = -1;
  float signCodeAlpha = 0;
  Vector3 fountain{ -35, 0, 20 }, sign{ 10, 0, -10 }, cabin{ 60, 0, -30 }, housePos{ 0, 0, -110 };
  Vector3 well{ -72, 0, -52 };
  Vector3 mirrorSpot{ 1.2f, 3.1f, -112.0f }, mirrorPos{ 5.12f, 4.2f, -112.0f };
  Vector3 kitchenDrawer{ 4.93f, 0.62f, -108.53f }, basementDoorP{ 5.0f, 0, -110.3f }, bedroomDoorP{ -4.2f, 3.1f, -114.2f };
  Vector3 falseWallP{ -4.0f, -1.4f, -110.4f }, oldPhoneP{ -6.0f, -1.48f, -110.5f }, noteP{ -5.7f, -1.49f, -110.25f };
  Vector3 wellBattery{ -69.5f, 0, -54.0f }, basementBattery{ 3.6f, -1.9f, -106.8f };
  Vector3 waterKeyP{ -32.65f, 0.92f, 20.6f };
};
extern World gWorld;
float pathX(float z);

// ---------------------------------------------------------------- player
struct Player {
  Vector3 pos{ 0, 0, 200 };       // feet
  Vector3 vel{ 0, 0, 0 };
  float yaw = 0, pitch = 0;       // yaw 0 => looking -Z
  float groundY = 0;
  float stamina = 100;
  bool sprinting = false, moving = false, exhausted = false, grounded = true;
  float bobPhase = 0, bobAmp = 0, landDip = 0, swayT = 0;
  bool enabled = false;
  float stepCycle = 0;
  Camera3D cam{};
};
extern Player gPlayer;
void PlayerUpdate(float dt, float stress);
void PlayerTeleport(float x, float z, float yaw);
float ResolveGround(float x, float z, float currentGround);
Vector3 PlayerForward();     // flat (XZ) facing
Vector3 PlayerForward3D();   // pitch-aware view direction
Vector3 PlayerEye();

// depth-only clear so hand/phone never clip the world (opengl32 export)
extern "C" void __stdcall glClear(unsigned int mask);
static inline void glClearDepthHack() { glClear(0x00000100); }

// compose scale -> yaw -> translate
static inline Matrix MTRS(Vector3 pos, float yaw, float scale) {
  return MatrixMultiply(MatrixMultiply(MatrixScale(scale, scale, scale), MatrixRotateY(yaw)), MatrixTranslate(pos.x, pos.y, pos.z));
}
static inline Matrix MTRS3(Vector3 pos, Vector3 rot, Vector3 scale) {
  return MatrixMultiply(MatrixMultiply(MatrixScale(scale.x, scale.y, scale.z), MatrixRotateXYZ(rot)), MatrixTranslate(pos.x, pos.y, pos.z));
}
void SetSpec(int instanced, float spec);
void SetSway(int instanced, int sway);

// ---------------------------------------------------------------- entities
enum class EState { Idle, Alerted, Pursuing, Hunting, Retreating, Dormant, Emerging };
enum class EKind { Shadow, Samara, Watcher, Crawler };
struct Entity {
  EKind kind = EKind::Shadow;
  EState state = EState::Idle;
  Vector3 pos, home;
  std::vector<Vector3> patrol;
  int patrolIdx = 0;
  float stateT = 0, pauseT = 0, lostT = 0, recoilT = 0, speed = 0;
  float walkPhase = 0, twitchT = 0, emergeT = 0, cryT = 0;
  float aggression = 1.0f;
  float decay = 0.5f;          // 0 recently-dead/opaque .. 1 ancient/ethereal (modulates alpha)
  float heightScale = 1.0f;    // real size: Samara ~0.82, Watcher ~1.14 (see ENTITY_DESIGN §3)
  bool lethal = true, frozen = false, removed = false, visible = true;
  int huntRole = 0; // 0 chaser, 1 flanker
  Vector3 lastSeen{};
  float lastSeenTime = -999;
  float facing = 0;
  int audioVoice = -1;
};
extern std::vector<Entity> gEntities;
void EntityUpdate(Entity& e, float dt, float time);
void EntityDraw(Entity& e, float time);
void SpawnShadow(Vector3 home, std::vector<Vector3> patrol, float aggression, bool lethal);
void SpawnSamara();
void SpawnCrawler(Vector3 home);
bool EntityLit(const Entity& e);

// ---------------------------------------------------------------- inventory
enum ItemId { IT_PHONE = 0, IT_WATERKEY, IT_KITCHENKEY, IT_NOTE, IT_BATTERY, IT_COUNT };
struct Inventory {
  bool has[IT_COUNT] = { true, false, false, false, false };
  int batteries = 0;            // consumable count
  int inspecting = -1;          // item raised in left hand (-1 none)
  float inspectAnim = 0;        // 0..1 raise
  float pickupAnim = 0;         // reach-out animation
  int pickupItem = -1;
};
extern Inventory gInv;
void InvGive(int id);
void InvSelect(int slot);
void InvUpdate(float dt);

// ---------------------------------------------------------------- phone
struct Phone {
  float battery = 100;
  bool lightOn = true;
  std::string objective = "";
  std::string message = ""; float messageT = 0;
  float flickerT = 0; bool flickerOff = false;
  float uiClock = 0;
  bool charging = false; float chargeT = 0; // 5s charge animation
};
extern Phone gPhone;
void PhoneUpdate(float dt, float time, bool indoor, bool entitiesNear, float stress);
void PhoneDrawScreen(float time, float stress); // renders to phone screen RT
void HandDraw(float time, float stress);        // 3D hand+phone+item, after world
void HudDraw(float time, float stress);         // 2D inventory bar etc (internal res)

// ---------------------------------------------------------------- audio
void AudioInit();
void AudioClose();
void AudioSetListener(Vector3 pos, float yaw);
void AudioSetStress(float s);
void AudioSetEnv(int env); // 0 forest, 1 house, 2 basement, 3 cabin
void AudioSetWind(float w);
void SfxFootstep(bool indoor, bool sprint);
void SfxBranch(float pan);
void SfxWhisper();
void SfxDrip();
void SfxBeep(bool low);
void SfxPickup();
void SfxUnlock();
void SfxSting(float intensity);
void SfxScare();
void SfxSamaraGroan(Vector3 at);
void SfxHeartThump(float strength);
int  AudioEntityVoice();
void AudioEntityUpdate(int voice, Vector3 pos, EState st, EKind kind);
void AudioEntityStop(int voice);

// ---------------------------------------------------------------- director
struct Director {
  int phase = 0;
  float stress = 0;
  float gameTime = 0;
  float catchCooldown = 0;
  float graceT = 0;            // brief safety after entering/being caught
  bool entitiesNear = false;
  bool whispers = false;
  int ending = 0;             // 0 none, 1 trust, 2 refuse
  float endingT = 0;
  bool ended = false, credits = false;
  Vector3 checkpoint{ 0, 0, 200 }; float checkpointYaw = 0;
  std::string codeBuffer = "\xFF"; // \xFF = inactive
  // flags
  bool fWaterKey = false, fCabinOpen = false, fFakeCharge = false, fCodeSeen = false;
  bool fInHouse = false, fDrawerOpen = false, fKitchenKey = false, fBedroomOpen = false;
  bool fBasementOpen = false, fBasementOnce = false, fMirrorScare = false, fMirrorSolved = false;
  bool fHiddenOpen = false, fTruth = false, fNoteRead = false, fNotePending = false;
  bool fWellBattery = false, fBaseBattery = false, fSamaraDone = false;
  float mirrorHold = 0;
  std::string readingNote;     // non-empty = a lore note panel is open
  bool loreRead[6] = { false };
  std::string sub; float subT = 0;
  std::string big; float bigT = 0;
  float flashWhite = 0, flashBlack = 0, glitch = 0;
  float rainVisual = 0;
  float danger = 0;            // 0 safe .. 1 entity here — drives the red proximity vignette (UI_UX §5)
  bool psychosis = false; float psychosisT = 0; // battery-0 death sequence (BATTERY_SYSTEM §2B)
};
extern Director gDir;
void DirectorStart();
void DirectorUpdate(float dt, float time);
void DirectorKey(int key);
void DirectorInteract();
std::string DirectorPrompt();
void DirectorCatch(Entity& e);

// ---------------------------------------------------------------- gfx
struct Gfx {
  RenderTexture2D scene;        // internal-res scene target
  RenderTexture2D phoneScr;     // phone screen UI
  RenderTexture2D shadowMap;    // flashlight shadow depth
  Shader world, worldInst, post, depth, depthInst;
  Texture2D noiseTex, white;
  Camera3D lightCam{};
  Matrix lightVP;
  int shadowOK = 0;
  Font font;
};
extern Gfx gGfx;
void GfxInit();
void GfxRenderFrame(float time, float dt);
Texture2D MakeNoiseTexture(int size, Color base, int variation);
Texture2D MakeTextTexture(int w, int h, Color bg, std::function<void()> draw);

// ---------------------------------------------------------------- states
enum GState { GS_MENU, GS_INTRO, GS_PLAY, GS_PAUSE, GS_SETTINGS, GS_CREDITS };
extern int gState;
extern float gStateT;        // time in current state
extern bool gReqQuit, gReqRestart;
void UIDrawOverlay(float time);   // menus/intro/pause/credits (in scene RT)
void UIHandleInput();             // mouse+key for non-play states
void IntroStart();

void WorldBuild();
void WorldDraw(bool shadowPass, float time);
void HouseBuild();
void OpenCabinDoor();
void OpenBasementDoor();
void OpenBedroomDoor();
void OpenFalseWall();
void OpenDrawer();
void SetMirrorFigure(float opacity);
void WorldTick(float dt, float time);
void SpawnWatcher(Vector3 at);
void SfxBreathPlayer(float intensity);
void SfxChargeTick();
