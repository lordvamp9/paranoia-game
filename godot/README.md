# PARANOIA — Godot 4.x port

Migration target project. See [`../docs/MIGRATION_ARCHITECTURE.md`](../docs/MIGRATION_ARCHITECTURE.md)
for the full design. Source of truth being ported: `../cpp/src/` (C++/raylib).

## Status
- **Session 1 (done):** architecture + project scaffold (this folder), autoload
  contracts, GDExtension skeleton, baseline reference `.exe`.
- **Session 2 (next):** Phase 1 — install Godot 4.3 LTS + SCons, build
  `libparanoia.dll`, get `Main.tscn` running with the state FSM.

## Layout
```
project.godot          Godot config: InputMap, autoloads, render settings
paranoia.gdextension   Loads the native lib per platform
scripts/               GDScript autoloads (game_state, director, config) + gameplay
scenes/                .tscn scenes (Main, World, Player, Phone, entities, ui)
shaders/               world.gdshader + post.gdshader (ported from cpp/src/shaders.h)
gdextension/           C++ GDExtension (audio synth, world gen) — godot-cpp
assets/                textures, fonts (no audio files — audio is synthesized)
```

## Build the GDExtension (Session 2)
```bash
cd gdextension
git clone -b 4.3 https://github.com/godotengine/godot-cpp
pip install scons
scons platform=windows target=template_debug   # -> bin/libparanoia.*.dll
```
Then open this folder in Godot 4.3 and press Play.

## Export a Windows .exe
Godot editor → Project → Export → Windows Desktop → `dist/`. Compared each
session against `../dist/PARANOIA-baseline-raylib-v3.1.0-win64.exe`.
