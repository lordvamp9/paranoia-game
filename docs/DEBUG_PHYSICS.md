# PARANOIA — Debugging, física, colisiones y optimización

> **Sesión 7 — Technical design.** Auditoría real del código de colisión/spawn/
> física (`player.h`, `entity.h`, `world.h`, `director.h`) con fixes concretos, más
> el mapeo a Godot 4.x (capas de física, NavMesh, pools). Restricción: los fixes
> **no rompen gameplay** y **solo mejoran** rendimiento. Tres se implementan esta
> sesión en el build raylib; el resto queda especificado para Godot.

---

## 0. Modelo físico real del juego (importante)

El juego **no usa un motor de físicas**: usa **colisión analítica propia** (rápida,
determinista). Esto cambia el diagnóstico respecto al prompt (que asume Godot
`PhysicsBody3D`). Lo real:

| Concepto | Implementación real | Archivo |
|---|---|---|
| Colisión jugador | círculo radio `P_RADIUS=0.30` vs `circles` (árboles) + `boxes` (paredes/AABB) | `player.h:36 Collide` |
| Suelo / multi-piso | `GroundZone` con `priority` + `height(x,z)`, umbral 0.6 | `player.h:23 ResolveGround` |
| Colisión entidades | **solo** deslizan contra `circles`; ignoran `boxes` y entre sí | `entity.h:298` |
| "Pool" de entidades | `std::vector<Entity> gEntities`, flag `removed`/`frozen` | `entity.h:9` |
| Spawns | funciones `Spawn*` que hacen `push_back` sin validar | `entity.h:56-97` |

> En la migración a Godot, esta colisión analítica se conserva en una **GDExtension
> C++** (`WorldGen`, ver `MIGRATION_ARCHITECTURE.md §4`) para preservar el *feel*,
> **no** se sustituye por el PhysicsServer. Las capas/NavMesh de Godot se usan para
> lo que el método analítico no cubre (IA pathfinding, triggers).

---

## 1. Bugs identificados (5) — descripción + causa + fix

### BUG-1 · Pool sin limpieza (fuga lenta) — **CRÍTICO** ✅ corregido
- **Síntoma:** en una sesión larga, `gEntities` crece sin límite. Los Watchers
  hacen `e.removed = true` ([entity.h:124](../cpp/src/entity.h)) pero **nunca se
  borran del vector**; cada 45–85 s aparece uno nuevo. Tras 20 min hay decenas de
  cadáveres iterados cada frame (stress loop, draw loop, separación).
- **Causa:** `removed` solo se chequea al iterar; no hay compactación del vector.
- **Fix (implementado):** compactar (`erase`/`remove_if`) las entidades `removed`
  cuando el pool supera un umbral. Seguro respecto a `gE0` (el índice del Shadow
  guionizado es 0; los Watchers borrados siempre tienen índice mayor).

### BUG-2 · Entidades atraviesan paredes — **ALTO** 🔶 documentado
- **Síntoma:** el acechador interior de la casa puede cortar a través de un muro
  hacia el jugador.
- **Causa:** el movimiento de entidad solo desliza contra `circles` (árboles),
  **ignora `boxes`** ([entity.h:298-305](../cpp/src/entity.h)).
- **Fix propuesto (NO esta sesión, por riesgo):** resolver también contra `boxes`
  como en `Collide`. **Riesgo:** sin pathfinding, una entidad puede quedar atrapada
  empujándose contra una esquina. **Solución correcta en Godot:** `NavMesh` +
  `NavigationAgent3D` para que la IA rodee la geometría en lugar de empujarla.

### BUG-3 · Entidades se solapan (se funden) — **MEDIO** ✅ corregido
- **Síntoma:** dos o más perseguidores convergen al mismo punto y se dibujan como
  un solo cuerpo; rompe la ilusión de "jauría".
- **Causa:** no hay separación entidad-entidad; todos buscan el mismo `target`.
- **Fix (implementado):** separación suave — cada entidad (no Watcher/Samara) se
  aparta de las cercanas dentro de 0.8 m. Gentil (no rompe el flanqueo).

### BUG-4 · Spawn sin validación de posición — **MEDIO** ✅ corregido
- **Síntoma:** un Watcher puede aparecer **dentro de un árbol** (su punto se calcula
  por ángulo+distancia sin comprobar colisión, [entity.h:79-86](../cpp/src/entity.h)).
- **Causa:** los `Spawn*` hacen `push_back` directo sin *shape query*.
- **Fix (implementado):** `ValidateSpawn(p)` empuja el punto fuera de cualquier
  `circle` solapado antes de colocar la entidad. Logging en build `DEV_CONSOLE`.

### BUG-5 · ResolveGround y el umbral 0.6 — **BAJO** 🔶 documentado
- **Síntoma potencial:** en transiciones entre pisos apilados (sótano −2.5 / suelo 0
  / planta 3.1), si una `GroundZone` está mal autorizada, el jugador puede "caer" o
  hacer snap a una altura equivocada.
- **Causa:** `ResolveGround` solo acepta zonas cuya altura está a <0.6 del suelo
  actual ([player.h:31](../cpp/src/player.h)); es lo que hace funcionar las
  escaleras, pero es sensible al diseño de zonas.
- **Mitigación:** el `--probe` ya cubre escaleras/sótano/dormitorio (6 checks de
  suelo). Mantener esos tests verdes es la red de seguridad. No se cambia el
  algoritmo (riesgo de romper el multi-piso).

---

## 2. Especificación técnica

### A. SpawnManager (pseudocódigo, objetivo Godot)
```cpp
class SpawnManager {
  Dictionary<EntityKind, ObjectPool<Entity>> pool;   // pre-instanciado, sin alloc en runtime
  Array<Entity*> active;
  Queue<SpawnRequest> retry;

  Entity* Spawn(kind, pos, settings) {
    pos = ValidatePosition(pos);                      // 1) validar
    if (pos == INVALID) { retry.push({kind,pos,settings}); log_fail(kind,pos); return null; }
    Entity* e = pool[kind].acquire();                 // 2) sacar del pool (reuso)
    if (!e) { log("pool exhausted", kind); return null; }
    e->reset(); e->pos = pos; e->apply(settings);     // 3) init limpio
    active.push(e);                                    // 4) registrar
    return e;
  }
  void Despawn(Entity* e) {                            // reuso, no free
    e->active = false; active.erase(e); pool[e->kind].release(e);
  }
  Vector3 ValidatePosition(pos) {                      // shape query
    if (overlaps_static(pos, radius)) pos = push_out(pos);   // fuera de colliders
    if (!on_navmesh(pos)) return INVALID;             // alcanzable por IA
    if (overlaps_other_spawn(pos, frame)) return INVALID;    // sin duplicado este frame
    return pos;
  }
};
```
> En el build raylib actual, `ValidateSpawn` (entity.h) implementa el `push_out`; la
> compactación implementa el reuso del pool de forma simplificada. El pool completo
> con `acquire/release` y NavMesh es trabajo de Godot.

### B. Capas de física (objetivo Godot) — tabla visual
```
 bit  Layer            Colisiona con            Tipo Godot
 ───  ───────────────  ───────────────────────  ───────────────────────────
  0   World            (es estático)            StaticBody3D + Concave/AABB
  1   Player           World, Entities          CharacterBody3D + CapsuleShape3D
  2   Entities         World, Player, Entities  CharacterBody3D (kinematic)
  3   Triggers         (sin colisión física)    Area3D (monitorable)
  4   Projectiles      —  (el juego no tiene)   —
```
| Cuerpo | Layer | Mask (con qué colisiona) |
|---|---|---|
| Terreno/estructuras | 0 | — |
| Jugador | 1 | 0, 2 |
| Entidad | 2 | 0, 1, 2 |
| Punto de carga / objetivo | 3 | 1 (solo detecta al jugador) |

> Mapa al modelo analítico actual: `circles`+`boxes` = Layer 0; `P_RADIUS` = cápsula
> del Player (Layer 1); `gEntities` = Layer 2; `CurrentInteractables` (radios de
> `[E]`) = Layer 3 (triggers, sin física).

### C. NavMesh (objetivo Godot)
- Generar `NavigationRegion3D` que cubra: sendero, bosque liminal, interior de la
  casa (3 pisos), perímetro del pozo/cabaña.
- Validar conectividad: la IA debe alcanzar todo punto jugable (test §4).
- Reconstruir al abrir puertas dinámicas (cabaña, sótano, dormitorio, muro falso):
  togglear el área de navegación de la puerta.
- **Mientras tanto (raylib):** la IA usa "ir hacia el jugador + deslizar contra
  árboles"; el NavMesh resuelve BUG-2 al migrar.

---

## 3. Checklist de validación (24 items)

**Spawn**
- [x] No hay spawns dentro de `circles` (ValidateSpawn) — *implementado*
- [ ] No hay spawns dentro de `boxes` (estructuras) — *Godot: shape query*
- [x] No hay duplicados perfectos en el mismo punto (separación) — *implementado*
- [x] Spawn failures se registran (`DEV_CONSOLE`) — *implementado*
- [x] El pool se compacta/reinicia (no crece sin límite) — *implementado + test*

**Física / colisiones**
- [x] El jugador no cae a través del suelo (6 checks de `--probe`) — *cubierto*
- [x] Puertas/objetos interactivos funcionan (probe: cabaña, sótano, dormitorio) — *cubierto*
- [ ] Entidades no atraviesan paredes — *BUG-2, requiere NavMesh (Godot)*
- [x] Entidades no se apilan/funden (separación) — *implementado + test*
- [x] Terreno navegable (probe: caminar a la casa, escaleras) — *cubierto*
- [ ] Esquinas agudas no atrapan al jugador — *doble `Collide`; revisar en Godot*

**Acceso a estructuras**
- [x] Todos los checkpoints accesibles (probe recorre puzzles) — *cubierto*
- [x] La cabaña (carga) es alcanzable (probe: código 2741) — *cubierto*
- [ ] Las 10 estaciones de carga propuestas son alcanzables — *al implementarlas*
- [x] No hay áreas muertas en el recorrido principal (probe full path) — *cubierto*
- [ ] NavMesh cubre todo el mapa jugable — *Godot*

**Performance**
- [x] 60 FPS estable (VSYNC, dual-res 640×360; sin caídas en spawn) — *cumplido*
- [x] Memoria estable (compactación elimina la fuga lenta) — *implementado*
- [x] Spawn time < 100 ms (push_back + validación O(circles)) — *trivial*
- [x] Física tick < 16 ms (colisión analítica O(n), n pequeño) — *cumplido*

**Regresión**
- [x] `--probe` verde (25 checks) — *gate de cada build*
- [x] Sin warnings nuevos de compilación que importen — *revisado*
- [x] El fix no cambia el feel de movimiento (probe de stairs/door) — *cubierto*
- [ ] Profiling de 70–95 min de sesión (memoria plana) — *al alargar el juego*

---

## 4. Testing automatizado

El juego **ya trae un harness headless**: `paranoia.exe --probe` (modo en
`main.cpp`) corre lógica sin ventana y devuelve exit≠0 si algo falla. Esta sesión lo
amplía a **25 checks**, incluidos los del prompt:

```
// "Spawn N entidades, verificar 0 overlaps" (prompt §4C) — implementado:
SpawnShadow(p0); SpawnShadow(p1 ≈ p0);     // casi encimadas
run EntityUpdate x40 frames;
assert dist(e0,e1)^2 > 0.3;                 // la separación las despega   ✅

// "Pool se reinicia correctamente" — implementado:
spawn 20 watchers; mark all removed; DirectorUpdate();
assert gEntities.size() < 20;               // compactación limpia          ✅

// ya existentes: stairs/floors (6), puzzles/keys/codes, samara emerge,
// endings, psychosis death+respawn, pacing floor.
```
Pseudocódigo del test de pathfinding (objetivo Godot):
```
for cada punto-objetivo P en [cabaña, pozo, casa, sótano, dormitorio]:
    agente = NavigationAgent3D(); agente.target = P
    assert agente.is_target_reachable()    // 0 áreas inalcanzables
```

---

## 5. Performance targets

| Métrica | Target | Estado real |
|---|---|---|
| FPS | 60 estable | ✅ (VSYNC, render interno 640×360) |
| Spawn time | < 100 ms | ✅ (~µs: push_back + O(#circles)) |
| Physics tick | < 16 ms | ✅ (colisión analítica O(n), n<10 entidades) |
| Memoria | plana (sin leaks) | ✅ tras BUG-1 (antes: crecía ~1 entidad/min) |
| Draw calls | acotado | ✅ instancing de vegetación (`InstancedSet`) |

---

## 6. Herramientas de debugging

- **Probe headless** (`--probe`): 25 checks de lógica/física, exit code para CI.
- **Shot/portrait** (`--shot`, `--portrait`, `--uishot`): captura determinista para
  inspección visual de spawns/render.
- **DEV_CONSOLE** (`make debug` → `paranoia-dev.exe`): consola con logs.
  - `ValidateSpawn`: *"spawn nudged out of tree at (x,z)"*.
  - Compactación: *"pool compacted: N -> M"*.
- **Propuesto (Godot):** dibujo de colliders en wireframe, overlay de NavMesh,
  markers de spawn, `print` de `PhysicsBody3D sin CollisionShape3D`.

---

## 7. Optimización específica para Godot

| Área | Recomendación |
|---|---|
| Memoria | **Object pool** de entidades pre-instanciadas (sin `instantiate/free` en runtime → evita stutter). Descargar assets entre actos. |
| Física | IA como `CharacterBody3D` **kinematic** (no RigidBody). `CapsuleShape3D` (más barato que convex). Spatial hashing para queries de proximidad si N crece. |
| Render | **LOD** en espectros (lejos = menos esferas/huesos). Frustum culling (auto). Shadow map de la linterna a 1024² (ya), considerar 512² en GPU débil. |
| GDExtension | Mantener la síntesis de audio y `WorldGen` en C++ (hot-path). Glue/eventos en GDScript. |

---

## 8. Troubleshooting guide

- **"La entidad cae a través del suelo"** → ¿la `GroundZone` cubre (x,z)? ¿su altura
  está a <0.6 del suelo previo? Correr `--probe` (checks de stairs/floor). En Godot:
  ¿el `StaticBody3D` del suelo tiene `CollisionShape3D` y está en Layer 0?
- **"Spawn falla en silencio"** → usar `paranoia-dev.exe` y leer el log de
  `ValidateSpawn` / pool. ¿La posición cae fuera del NavMesh? ¿el pool está agotado?
- **"FPS cae durante spawn"** → ¿se está haciendo `alloc`/`instantiate` en runtime?
  Migrar a pool. Perfilar `EntityUpdate` (O(n²) por la separación: acotar N).
- **"El jugador queda atrapado en geometría"** → ¿esquina aguda? El doble `Collide`
  ayuda; en Godot reducir el radio de la cápsula o `move_and_slide` con `max_slides`.

---

## 9. Mapa de implementación

| Fix | Archivo | Estado |
|---|---|---|
| BUG-1 compactación del pool | `director.h` (fin de `DirectorUpdate`) | ✅ esta sesión |
| BUG-3 separación entidad-entidad | `entity.h` `EntityUpdate` | ✅ esta sesión |
| BUG-4 validación de spawn + log | `entity.h` `ValidateSpawn`/`SpawnWatcher` | ✅ esta sesión |
| Tests automatizados (overlap, pool) | `main.cpp` `--probe` (→25 checks) | ✅ esta sesión |
| BUG-2 entidad vs paredes | `entity.h` / Godot NavMesh | 🔶 Godot |
| BUG-5 robustez de ResolveGround | `player.h` / autoría de zonas | 🔶 (cubierto por probe) |
| Capas de física, NavMesh, pool Godot | proyecto Godot | 🔶 Fase 2 |

> **Esta sesión** corrige 3 bugs reales (fuga del pool, solapamiento, spawn sin
> validar) y añade los 2 tests automatizados que pide el prompt. `--probe` pasa de
> 23 a **25 checks**, todos verdes. Sin degradar rendimiento (la compactación lo
> mejora).

---

*Fin del documento técnico — Sesión 7.*
