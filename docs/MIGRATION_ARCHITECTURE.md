# PARANOIA — Arquitectura de migración a C++ / Godot 4.x

> **Sesión 1 — Diseño de arquitectura.** Documento de referencia para la
> migración del juego a un stack C++ (GDExtension) + Godot Engine 4.x + GDScript.
> Autor del juego: **vamp9**. Versión actual: **v3.1.0**.

---

## 0. Estado real del proyecto (importante)

El prompt asume que el juego "está actualmente en Electron + Three.js". Eso fue
cierto en **v1.0.0**, pero el repositorio ya contiene **dos** codebases:

| Codebase | Ubicación | Estado | Stack |
|---|---|---|---|
| Original (v1.0.0) | `src/*.js`, `lib/three.module.js`, `main.js`, `index.html` | Legado, congelado | Electron 33 + Three.js 0.170 |
| **Producción (v2.0.0 → v3.1.0)** | `cpp/src/*.{h,cpp}` | **Lo que se distribuye hoy** | **C++17 + raylib 6.0 (OpenGL 3.3)** |

> **Conclusión:** la migración real es **C++/raylib → C++/Godot**, no
> Electron/Three.js → Godot. El código fuente de verdad (4.165 LOC C++) ya es
> nativo y de alto rendimiento. El mapeo de módulos de este documento usa el
> codebase **C++/raylib** como origen, porque es la fuente de la verdad. La
> versión JS se conserva solo como referencia de diseño histórico.

Esto es una **buena noticia para la migración**: gran parte de la lógica
(audio sintetizado, director/FSM, generación de mundo, colisión) es C++17 puro
sin dependencias de raylib y se puede **reusar casi verbatim dentro de una
GDExtension**, sustituyendo únicamente la capa de raylib por la API de Godot.

### Inventario del codebase C++/raylib actual

```
cpp/src/  (unity build: main.cpp incluye todos los headers)
├── game.h      250  Estado global compartido, constantes, structs, enums, decls
├── main.cpp    248  Entry point + bucle principal + FSM de estados + modos shot/probe
├── config.h     99  Settings (rebind de teclas, buses de audio, sens) → paranoia.cfg
├── world.h     674  Generación procedural del mundo + colisión + multi-piso + draw
├── house.h     224  Geometría de la casa (puertas dinámicas, cajón, espejo, muro falso)
├── player.h    168  Movimiento 1ª persona, stamina, head-bob, ResolveGround, Collide
├── entity.h    513  FSM de espectros (Shadow/Samara/Watcher/Crawler), draw humanoide
├── director.h  509  ORQUESTADOR: fases, flags de progreso, puzzles, finales, checkpoint
├── audio.h     415  Motor de audio: síntesis 48 kHz en callback, 48 voces, reverb
├── music.h      64  Modos musicales / swell dinámico
├── phone.h     352  Teléfono: batería, linterna, pantalla RT, mano 3D, HUD, inventario
├── gfx.h       259  Pipeline de render dual-res + shadow map + post FX
├── shaders.h   206  GLSL 330: world, depth, post (dither/quantize/glitch)
└── ui.h        184  Menús, intro cinemática, pausa, settings, créditos
```

**Singletons globales** (estado del juego): `gWorld`, `gPlayer`, `gEntities`,
`gInv`, `gPhone`, `gDir`, `gGfx`, `gCfg`. En Godot pasan a ser **autoloads**
(GDScript) o nodos de servicio.

---

## 1. Diagrama de arquitectura objetivo

```
┌──────────────────────────────────────────────────────────────────────────┐
│                          GODOT 4.x  (runtime + editor)                     │
│                                                                            │
│  ┌─────────────────────────── CAPA GDScript (glue / gameplay) ──────────┐  │
│  │                                                                       │  │
│  │   Main.tscn (raíz)                                                     │  │
│  │     ├── GameState (autoload)   FSM: MENU→INTRO→PLAY→PAUSE→…  ◄── main.cpp│
│  │     ├── Director (autoload)    fases, flags, puzzles, finales ◄── director.h│
│  │     ├── Config  (autoload)     InputMap + buses + saves      ◄── config.h │
│  │     ├── World    (Node3D)      instancia geometría/colisión  ◄── world/house│
│  │     ├── Player   (CharacterBody3D)  cámara, mov, stamina     ◄── player.h │
│  │     ├── Phone    (Node3D+SubVP)  linterna, batería, pantalla ◄── phone.h │
│  │     ├── Entities (Node3D)      pool de espectros (FSM .gd)   ◄── entity.h │
│  │     └── UI (CanvasLayer)       menús, HUD, subtítulos        ◄── ui.h    │
│  │                                                                       │  │
│  └───────────────────────────────────┬───────────────────────────────────┘  │
│                                       │  llamadas vía API GDExtension         │
│  ┌────────────────────────────── CAPA C++ (GDExtension nativa) ──────────┐  │
│  │                                                                       │  │
│  │   libparanoia.dll  (godot-cpp, C++17)                                  │  │
│  │     ├── ParanoiaSynth   AudioStreamPlayback custom  ◄──── audio.h/music.h│
│  │     │     síntesis 48 kHz, 48 voces, reverb Schroeder (REUSO VERBATIM) │  │
│  │     ├── WorldGen        generación procedural + colisión   ◄── world/house│
│  │     │     ResolveGround multi-piso, Collide círculos/cajas (REUSO)     │  │
│  │     ├── EntityBrain     (opcional) hot-path FSM de IA       ◄── entity.h │
│  │     └── PostFX          shaders portados a Godot .gdshader  ◄── shaders.h│
│  │                                                                       │  │
│  └───────────────────────────────────────────────────────────────────────┘  │
│                                                                            │
│  ┌──────────────────── Render objetivo (firma visual) ────────────────────┐ │
│  │  SubViewport 640×360 (point filter)  ── world.gdshader (dither/fog) ──┐ │ │
│  │        ↓ upscale nearest a pantalla                                   │ │ │
│  │  ColorRect post.gdshader (quantize / glitch / flash / vignette)       │ │ │
│  │        ↓                                                              │ │ │
│  │  Pase de actores full-res (espectros, mano, teléfono) con oclusión    │ │ │
│  │  por depth contra el SubViewport  →  CanvasLayer HUD 2D nítido        │ │ │
│  └──────────────────────────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────────────────────────┘
                                   │
                          paranoia.cfg  (settings — formato preservado)
                          paranoia.sav  (checkpoint — nuevo, ver §6)
```

---

## 2. Estructura de carpetas propuesta

```
Paranoia/
├── cpp/                      # ← codebase raylib ACTUAL (se conserva como baseline)
│   └── src/*.h, main.cpp     #   sirve de "antes" para comparar cada build de Godot
├── src/, lib/                # ← legado Electron/Three.js (referencia histórica)
│
├── godot/                    # ← NUEVO proyecto Godot 4.x  (raíz del juego migrado)
│   ├── project.godot         #   config del proyecto: InputMap, autoloads, render
│   ├── icon.svg
│   ├── paranoia.gdextension  #   manifiesto que carga libparanoia.dll por plataforma
│   │
│   ├── scenes/               #   escenas .tscn
│   │   ├── Main.tscn         #     raíz: orquesta estados y subescenas
│   │   ├── World.tscn        #     mundo (poblado por WorldGen en runtime)
│   │   ├── Player.tscn       #     CharacterBody3D + Camera3D + Phone
│   │   ├── Phone.tscn        #     mano + teléfono + SubViewport de pantalla
│   │   ├── entities/         #     una escena por espectro (Shadow, Samara, …)
│   │   └── ui/               #     Menu, Intro, Pause, Settings, Credits, HUD
│   │
│   ├── scripts/              #   GDScript (gameplay / glue)
│   │   ├── game_state.gd     #     autoload — FSM de estados (← main.cpp)
│   │   ├── director.gd       #     autoload — orquestador (← director.h)
│   │   ├── config.gd         #     autoload — settings + InputMap (← config.h)
│   │   ├── player.gd         #     movimiento, stamina, bob (← player.h)
│   │   ├── phone.gd          #     batería, linterna, inventario (← phone.h)
│   │   ├── world.gd          #     puente a WorldGen, gestión de puertas (← world/house)
│   │   └── entities/         #     FSM por tipo de entidad (← entity.h)
│   │
│   ├── shaders/              #   .gdshader portados de shaders.h
│   │   ├── world.gdshader    #     spot + shadow + fog + dither
│   │   └── post.gdshader     #     quantize / glitch / flash / vignette
│   │
│   ├── gdextension/          #   módulos críticos en C++ (godot-cpp)
│   │   ├── SConstruct        #     build SCons (oficial godot-cpp)
│   │   ├── CMakeLists.txt    #     build alternativo CMake
│   │   ├── godot-cpp/        #     submódulo git (bindings de la API de Godot)
│   │   ├── src/
│   │   │   ├── register_types.{h,cpp}   # registro de clases en Godot
│   │   │   ├── paranoia_synth.{h,cpp}   # ParanoiaSynth (← audio.h/music.h)
│   │   │   ├── world_gen.{h,cpp}        # WorldGen colisión/multipiso (← world/house)
│   │   │   └── core/                    # headers reusados del juego (port limpio)
│   │   └── bin/              #     libparanoia.{dll,so} compiladas
│   │
│   ├── assets/               #   recursos importados
│   │   ├── textures/, fonts/, materials/
│   │   └── (audio: NO hay .wav — todo es síntesis en ParanoiaSynth)
│   │
│   └── addons/               #   plugins de editor (si aplica)
│
├── docs/
│   └── MIGRATION_ARCHITECTURE.md   # este documento
│
└── dist/                     # releases .exe por sesión (baseline + builds Godot)
    └── PARANOIA-baseline-raylib-v3.1.0-win64.exe   # referencia "antes"
```

### Justificación

- **`cpp/` se conserva intacto.** Es el baseline de rendimiento y comportamiento.
  Cada sesión genera un `.exe` de Godot que se compara contra él ("ver la
  diferencia"). No se borra hasta que la paridad esté demostrada.
- **`godot/` es autocontenido.** Un proyecto Godot tiene que abrir desde una
  carpeta con `project.godot` en la raíz; aislarlo evita que el editor escanee
  `node_modules/` ni el árbol de raylib.
- **`gdextension/` dentro de `godot/`** porque la `.dll` resultante se referencia
  con rutas relativas al proyecto desde `paranoia.gdextension`.
- **Separación tajante C++ vs GDScript** (ver §4): lo determinista y de hot-path
  (audio, generación, colisión) en C++; lo que cambia a menudo y es de orquestación
  (estados, eventos, puzzles, UI) en GDScript, donde el ciclo de iteración es
  instantáneo (sin recompilar).
- **`shaders/`** separado: los GLSL 330 actuales se traducen al lenguaje de
  shaders de Godot (similar a GLSL ES 3.0) — es traducción de sintaxis, no
  reescritura algorítmica.

---

## 3. Mapeo de módulos: raylib actual → Godot

| Componente actual | Origen | Destino Godot | Capa | Notas de migración |
|---|---|---|---|---|
| Bucle + FSM de estados | `main.cpp` | `game_state.gd` (autoload) + `Main.tscn` | GDScript | `GS_MENU…GS_CREDITS` → enum + `_process` por estado. Hooks de entrada de estado → señales. |
| Orquestador / progreso | `director.h` | `director.gd` (autoload) | GDScript | Fases, ~30 flags booleanos, puzzles (código 2741, espejo, muro falso), 2 finales, checkpoint. Lógica de eventos: ideal para GDScript + señales. |
| Settings + rebind | `config.h` | `config.gd` (autoload) | GDScript | Teclas → `InputMap` de Godot. Buses → `AudioServer`. **Formato `paranoia.cfg` se preserva** (§6). |
| Mundo procedural + colisión | `world.h`, `house.h` | `WorldGen` (C++) + `world.gd` | **C++** + glue | `ResolveGround` multi-piso y `Collide` (círculos/cajas) se reusan verbatim; `world.gd` instancia mallas/`StaticBody3D` y abre puertas. |
| Render 3D | `gfx.h` | `SubViewport` + `world.gdshader` + pase de actores | Godot + shader | **El punto más delicado** (§5). Dual-res, shadow map de linterna, oclusión manual por depth. |
| Shaders | `shaders.h` (GLSL 330) | `*.gdshader` | Shader | Traducción de sintaxis. `world`, `depth` (→ `DepthTexture`/shadow de Godot), `post`. |
| Audio paranormal | `audio.h`, `music.h` | `ParanoiaSynth` (`AudioStreamPlayback`) | **C++** | Síntesis 48 kHz, 48 voces, reverb Schroeder, buses. **Reuso casi total**; se sustituye la salida raylib por `push_buffer`/`AudioStreamGenerator`. Voces posicionales → `AudioStreamPlayer3D` o paneo propio (ya implementado). |
| Jugador (1ª persona) | `player.h` | `Player.tscn` (`CharacterBody3D`) + `player.gd` | GDScript | Mov, stamina, head-bob, sprint. `ResolveGround`/`Collide` delegados a `WorldGen` para paridad exacta de física. |
| Entidades / espectros | `entity.h` | `scenes/entities/*.tscn` + `*.gd` (FSM) | GDScript (+ C++ opcional) | FSM `Idle/Alerted/Pursuing/Hunting/Retreating/Dormant/Emerging`. Modelo humanoide → `MeshInstance3D` jerárquico o malla importada. Pooling (§7). |
| Teléfono / inventario / HUD | `phone.h` | `Phone.tscn` (`SubViewport`) + `phone.gd` + HUD `Control` | GDScript | Pantalla del teléfono = `SubViewport` con UI 2D → textura sobre la malla. Batería, linterna (`SpotLight3D`), inventario 1–5. |
| Input / controles | `config.h` + `main.cpp` | `InputMap` (project.godot) + `_input` | GDScript | Acciones: `move_*`, `sprint`, `light`, `interact`, `slot_1..5`, `pause`. Rebind en runtime con `InputMap.action_*`. |
| Física / colisiones | `player.h`, `world.h` | `WorldGen` (C++) **o** `PhysicsBody3D` | **C++ (recomendado)** | El juego **no** usa motor de físicas: usa colisión analítica propia. Mantenerla en C++ garantiza paridad; usar PhysicsServer de Godot cambiaría el feel. |
| Menús / intro / pausa | `ui.h` | `scenes/ui/*.tscn` (`Control`) | GDScript | UI nativa de Godot (themes, `Label`, `Button`). Intro cinemática → `AnimationPlayer`. |

---

## 4. Decisiones arquitectónicas clave (C++ vs GDScript)

### 4.1 Qué va en C++ (GDExtension) y por qué

| En **C++** | Razón | Trade-off |
|---|---|---|
| **Motor de audio** (`ParanoiaSynth`) | Síntesis muestra a muestra a 48 kHz en el hilo de audio: no puede correr en GDScript (latencia/GC). El código ya es C++ puro reusable. | Recompilar al tocarlo; pero cambia poco una vez portado. |
| **Generación procedural + colisión** (`WorldGen`) | `ResolveGround`/`Collide` se llaman por frame por jugador y entidades. Paridad exacta del *feel* de movimiento. Determinismo (misma semilla → mismo mundo). | Atado al ciclo de build C++; mitigado porque es lógica estable. |
| **FSM de IA en hot-path** (`EntityBrain`, *opcional*) | Solo si con N entidades el FSM en GDScript no llega a 60 FPS. | Empezar en GDScript; subir a C++ solo si el profiler lo pide (evitar optimización prematura). |

### 4.2 Qué va en GDScript y por qué

| En **GDScript** | Razón | Trade-off |
|---|---|---|
| **Director / orquestación** (`director.gd`) | Es donde más se itera (puzzles, ritmo, sustos, finales). Iteración instantánea sin recompilar. Señales de Godot encajan con su modelo de eventos. | ~Más lento que C++, pero no es hot-path (corre lógica, no aritmética por muestra). |
| **FSM de estados del juego** (`game_state.gd`) | Pega de alto nivel entre escenas. | — |
| **UI / menús / HUD** | El editor de Godot + `Control` + themes es muy superior a dibujar UI a mano. | Rehacer la UI (no portar pixel a pixel). |
| **Gameplay de jugador y entidades** | Tuning constante (velocidades, agresión, animación). | — |
| **Eventos de teléfono / inventario** | Lógica de estado, no de señal de audio. | — |

> **Principio rector:** *"C++ para lo determinista y caliente; GDScript para lo
> que cambia."* La frontera es la API de la GDExtension: GDScript llama a
> `WorldGen.resolve_ground(x,z,cur)`, `ParanoiaSynth.trigger(...)`, etc.

### 4.3 Sistema de assets

- **Audio: no hay archivos.** Todo se sintetiza. No se importan `.wav`/`.ogg`;
  `ParanoiaSynth` alimenta un `AudioStreamGenerator`. Esto es atípico y simplifica
  el pipeline de assets enormemente.
- **Geometría: procedural en runtime.** El mundo se construye con `ArrayMesh`/
  `CSGShape3D`/`MultiMeshInstance3D` (para vegetación instanciada — el actual usa
  `InstancedSet`). No se importan escenas `.glb` para el terreno.
- **Espectros:** decisión abierta — (a) malla humanoide importada (`.glb`) con
  `Skeleton3D`, o (b) reconstruir el humanoide procedural actual con
  `MeshInstance3D` jerárquicos. Recomendado (a) para mejor animación, manteniendo
  el shading "nítido sobre mundo dithered".
- **Texturas:** las que genera el juego (ruido, decals como el código 2741) se
  generan en C++/GDScript a `ImageTexture`; las nuevas se importan a `assets/textures/`.

### 4.4 Gestión de memoria

- **Pool de entidades:** el actual usa `std::vector<Entity> gEntities` con
  `removed`/`frozen`. En Godot → **object pool** de nodos espectro pre-instanciados
  (evita `instantiate()`/`free()` en runtime, que causa stutter). `director.gd`
  marca activos/inactivos en vez de crear/destruir.
- **Proyectiles:** el juego no tiene proyectiles (no hay armas), así que el pool
  aplica a entidades, decals y partículas (lluvia/niebla).
- **GDExtension:** objetos C++ con vida ligada a nodos Godot (RAII vía
  `Ref<>`/`RefCounted`). Los buffers de audio y mundo se asignan una vez al cargar.

---

## 5. Riesgo técnico nº1: reproducir la "firma visual"

El estilo (mundo dithered de baja resolución + jugador/manos/espectros nítidos
encima, con la cosa nunca "perteneciendo" a ese lugar) es **la identidad del
juego** y la parte más difícil de migrar. Cómo lograrlo en Godot 4:

1. **Capa de mundo:** renderizar a un `SubViewport` de **640×360** con filtro
   `Nearest`. Aplicar `world.gdshader` (spotlight de linterna + shadow + fog +
   dither tipo Bayer). Escalar a pantalla con nearest → look pixelado.
2. **Post:** un `ColorRect` full-screen con `post.gdshader` (quantize de color,
   glitch por estrés, flash blanco/negro, viñeta por batería). Equivalente directo
   del `FS_POST` actual.
3. **Capa de actores nítidos:** segundo pase a resolución nativa (espectros, mano,
   teléfono) con **oclusión por depth** contra el depth del SubViewport del mundo.
   En Godot esto se resuelve con un `Compositor`/`CompositorEffect` (Godot 4.3+) o
   leyendo `DEPTH_TEXTURE`. **Es el reto de integración**; hay que prototiparlo
   en la Fase 2 antes de comprometer el resto del render.
4. **HUD 2D:** `CanvasLayer` nítido a resolución nativa.

> **Plan B** si el doble-viewport con depth compartido resulta inviable en Godot:
> renderizar todo a 640×360 y aplicar el "nítido" solo al HUD/teléfono, aceptando
> una pérdida menor de fidelidad. Se decide tras el prototipo de Fase 2.

---

## 6. Compatibilidad de saves / checkpoints (requisito duro)

Estado actual:

- **`paranoia.cfg`** (settings): texto plano `clave valor` por línea (ver
  `config.h`). Es lo único que se persiste hoy. **Se preserva el formato exacto**:
  `config.gd` lo lee/escribe igual, de modo que un `.cfg` existente sigue siendo
  válido tras la migración.
- **Checkpoint:** hoy **no se guarda a disco**. `gDir.checkpoint` (posición
  `Vector3` + yaw) vive solo en memoria y se usa para reaparecer tras ser
  atrapado. No hay "partida guardada" persistente.

Plan de migración:

1. **Settings:** `config.gd` mantiene lectura/escritura de `paranoia.cfg` con el
   mismo esquema (`key0..6`, `master/music/ambient/voices`, `sens`, `full`). 100%
   retrocompatible.
2. **Checkpoint persistente (mejora):** serializar el estado del `Director` (fase
   + flags + checkpoint + inventario) a `paranoia.sav` (formato versionado:
   `JSON` o `ConfigFile` de Godot). Esto **añade** guardado entre sesiones sin
   romper nada, cumpliendo "mantener compatibilidad con saves/checkpoints" y
   mejorándolo. La cabecera lleva `version` para migraciones futuras.

---

## 7. Roadmap de migración por fases

### Fase 1 — Setup + estructura base  *(esta sesión: andamiaje; impl. en sesión 2)*
- [x] Documento de arquitectura (este archivo).
- [x] Árbol de carpetas `godot/` + manifiestos (`project.godot`, `.gdextension`,
      `SConstruct`/`CMakeLists.txt`) + stubs de autoloads GDScript.
- [x] Baseline `.exe` raylib empaquetado como referencia.
- [ ] **(Sesión 2)** Instalar Godot 4.x LTS + SCons + clonar `godot-cpp`;
      compilar `libparanoia.dll` vacía que cargue en Godot ("Hello GDExtension").
- [ ] **(Sesión 2)** `Main.tscn` arranca, FSM de estados navega Menú→Intro→Play.

### Fase 2 — Core gameplay
- [ ] `Player.tscn`: cámara 1ª persona + movimiento + stamina + head-bob.
- [ ] `WorldGen` (C++): portar `world.h`/`house.h` (colisión, multipiso, geometría).
- [ ] Render dual-res: prototipo del SubViewport + `world.gdshader` + post + pase
      de actores con oclusión por depth (**hito de riesgo, §5**).
- [ ] Spawn/pool de entidades; FSM de una entidad (Shadow) en `entity.gd`.

### Fase 3 — Contenido y paridad
- [ ] Las 4 entidades (Shadow, Samara, Watcher, Crawler) con sus voces.
- [ ] `ParanoiaSynth` (C++): portar `audio.h`/`music.h` completo, buses, reverb.
- [ ] Teléfono: linterna (`SpotLight3D`), batería, pantalla (`SubViewport`),
      inventario, carga.
- [ ] Director completo: puzzles (2741, espejo, muro falso, llaves), 2 finales,
      checkpoint persistente (`paranoia.sav`).
- [ ] UI: menús, intro cinemática, pausa, settings (rebind), créditos.
- [ ] Export a `.exe` Windows x64 → comparar contra baseline; objetivo **60 FPS**.

### Criterio de "paridad" (cierre de migración)
La build de Godot debe pasar el equivalente del modo `--probe` actual (24 checks
de lógica: llaves, códigos, puzzles, finales) y mantener la firma visual y de
audio a 60 FPS en GPU moderna.

---

## 8. Herramientas y dependencias

| Herramienta | Versión | Estado en el equipo | Uso |
|---|---|---|---|
| Godot Engine | 4.x (LTS, 4.3+ por `CompositorEffect`) | ❌ por instalar (sesión 2) | Editor + runtime + export |
| godot-cpp | rama acorde a la versión de Godot | ❌ por clonar | Bindings C++ de la API |
| C++ compiler | C++17 (MinGW-w64 ya presente en `tools/w64devkit`) | ✅ | Compilar GDExtension |
| SCons | ≥ 4.x | ❌ por instalar (`pip install scons`) | Build oficial de godot-cpp |
| CMake | ≥ 3.20 (alternativa a SCons) | ⚠️ verificar | Build alternativo |
| Python | 3.12 / 3.14 | ✅ | SCons |
| Export templates Godot | misma versión que el editor | ❌ por descargar | Generar `.exe` |

Librerías externas adicionales: **ninguna**. El juego no usa físicas externas,
ni formatos de audio, ni red. Todo es Godot + la GDExtension propia.

> Restricciones respetadas: **sin Unreal Engine** (Godot es ligero); paridad de
> saves (`paranoia.cfg` intacto, `paranoia.sav` aditivo); objetivo **60 FPS**.

---

## 9. Resumen de decisiones (trade-offs en una línea)

1. **Migrar desde raylib, no desde Three.js** — el código real ya es C++ nativo;
   ahorra reescribir la lógica desde cero. *Trade-off:* el prompt asumía Three.js.
2. **Audio y mundo/colisión en C++ (reuso casi verbatim)** — máximo rendimiento y
   paridad. *Trade-off:* ciclo de build C++ al tocarlos.
3. **Director, UI y gameplay en GDScript** — iteración rápida donde más se itera.
   *Trade-off:* algo más lento, irrelevante fuera del hot-path.
4. **Colisión analítica propia, no PhysicsServer de Godot** — preserva el *feel*.
   *Trade-off:* no aprovecha el motor de físicas (que no se necesita).
5. **Render dual-res con SubViewport + CompositorEffect** — replica la firma
   visual. *Trade-off:* es el mayor riesgo técnico; lleva Plan B.
6. **Saves: `.cfg` preservado + `.sav` aditivo versionado** — compatibilidad y
   mejora. *Trade-off:* ninguno relevante.
7. **`cpp/` raylib se conserva como baseline** — permite "ver la diferencia" cada
   sesión. *Trade-off:* repo más grande hasta cerrar la paridad.

---

*Fin del documento de arquitectura — Sesión 1. Próxima sesión: Fase 1 de
implementación (bootstrap de Godot + GDExtension "Hello World" que carga).*
