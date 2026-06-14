# PARANOIA — Loop de gameplay y pacing narrativo

> **Sesión 6 — Rhythm, pacing y loop.** El ciclo **Acción → Alivio → Recompensa →
> Desafío** y su curva de tensión, anclado a los sistemas reales (fases del
> `director.h`, checkpoints de `NARRATIVE_DESIGN.md`, batería de `BATTERY_SYSTEM.md`,
> entidades de `ENTITY_DESIGN.md`). Filosofía: **nunca tensión cero, nunca tensión
> máxima insostenible**, pendiente ascendente por loop.

---

## 0. El loop maestro (4 fases)

```
   ┌──────────────── EL CICLO (8–12 min) ─────────────────┐
   │                                                       │
   │  ① ACCIÓN (3–5 min)        ② ALIVIO/CARGA (1–2 min)  │
   │  objetivo nuevo → navegar  llegas a un punto de carga │
   │  → acecho → decisión       esperas (vulnerable) →     │
   │  (luz/oscuridad/prisa)     pequeña revelación         │
   │  tensión ▁▂▃▄▅▆▇           tensión ▇▅▃▂ (valle, no 0) │
   │         │                          │                  │
   │         ▼                          ▼                  │
   │  ④ DESAFÍO (prep)         ③ RECOMPENSA NARRATIVA      │
   │  nuevo objetivo, algo      "ahora sé que…" + nueva    │
   │  peor → anticipación       área/mecánica desbloqueada │
   │  tensión ▃▄▅ (sube)        tensión ▂▃ (recupera)      │
   │         └──────────► vuelve a ① ◄──────────┘          │
   └───────────────────────────────────────────────────────┘
```

Mapa a los sistemas del build:
- **① Acción** = un tramo de `Phase1/2/3Script` con acecho de entidades + decisión de
  linterna (drenaje de batería).
- **② Alivio** = una **estación de carga** (cabaña baja estrés −5/s; checkpoint se fija).
- **③ Recompensa** = una **nota de lore**, un **objeto** (llave/pila), o un **giro**
  (espejo, otro teléfono) + `gPhone.objective` nuevo.
- **④ Desafío** = el siguiente objetivo, una entidad nueva o más agresiva.

---

## 1. Los 8 loops principales

Cada loop: **Acción → Alivio → Recompensa → Desafío**. Mapea a CP1–CP10 de
`NARRATIVE_DESIGN.md`. ✅ existe en el build · 🔶 ampliación.

### Loop 1 — Despertar *(Acto I · ~8 min)*
- **Acción:** levantarte en la carretera, aprender a moverte/iluminar, seguir el
  sendero mientras un Shadow se asoma (no letal). ✅ `Phase1Script`
- **Alivio:** el coche del accidente / poste — primera carga ligera. 🔶
- **Recompensa:** 1ª nota ("iba a buscarla") + mensaje `TRUST ME`. ✅
- **Desafío:** "REACH THE WHITE HOUSE" — el bosque se abre. ✅
- **Narrativa:** *"Despierto del accidente. Una voz me dice que confíe."*

### Loop 2 — Primer encuentro *(Acto I · ~9 min)*
- **Acción:** cruzar el bosque liminal; susurros "que no son palabras"; el Shadow
  se planta en el camino y desaparece. ✅
- **Alivio:** la fuente seca (panel solar lento) — vulnerable mientras cargas. 🔶
- **Recompensa:** 2ª nota ("siempre volvemos") + aparecen los **Watchers**. ✅
- **Desafío:** llegar a la cabaña con el código. ✅
- **Narrativa:** *"Veo algo… imposible. Y no es la primera vez."*

### Loop 3 — El código y el refugio *(Acto II · ~10 min)*
- **Acción:** encontrar el código 2741 (revelado por la luz en el cartel),
  evitar a los Shadows que ahora patrullan. ✅
- **Alivio:** **la cabaña** — carga completa real (5 s), estrés baja, refugio. ✅
- **Recompensa:** 3ª nota ("la casa se queda con lo que toma"). ✅
- **Desafío:** el pozo — "take what's down there". ✅
- **Narrativa:** *"Este lugar me conoce. Yo dejé estas notas."*

### Loop 4 — El pozo *(Acto II · ~11 min — ROMPE EL LOOP)*
- **Acción:** acercarte al agua; **Samara emerge** (primer peligro letal real),
  reptar a tirones. ✅
- **Alivio:** recoger la **pila externa** (+50%) — pero despertarla fue el precio. ✅
- **Recompensa:** entender que "ella" está en el agua. ✅
- **Desafío:** entrar en la casa blanca. ✅
- **Narrativa:** *"El agua se la llevó. La oigo subir."*

### Loop 5 — La casa *(Acto II→III · ~10 min)*
- **Acción:** entrar; "demasiado silencio"; un acechador interior patrulla el fondo. ✅
- **Alivio:** generador del porche (ruidoso, atrae) / cuadro de cocina. 🔶
- **Recompensa:** objeto de *ella* en el cajón + llave de cocina. ✅(llave) 🔶(objeto)
- **Desafío:** el dormitorio de arriba — "THE MIRROR REMEMBERS". ✅
- **Narrativa:** *"Conozco esta casa. ¿Por qué conozco esta casa?"*

### Loop 6 — El espejo *(Acto III · ~9 min — ROMPE EL LOOP)*
- **Acción:** dormitorio; el **Crawler** sale de debajo de la cama; resolver el
  espejo (que no te refleja). ✅
- **Alivio:** breve, tras resolver — "something unlocked below". ✅
- **Recompensa:** **cinemática Giro** (el reflejo, flashbacks). 🔶(secuencia)
- **Desafío:** bajar al sótano. ✅
- **Narrativa:** *"Ese no es mi reflejo. Es lo que evito mirar."*

### Loop 7 — El sótano *(Acto III · ~9 min)*
- **Acción:** bajar (las Shadows no te siguen); aislamiento; encontrar el muro
  hueco con la luz de frente. ✅
- **Alivio:** pila del sótano (+50%) — calma ominosa, sin lluvia. ✅
- **Recompensa:** se abre la habitación oculta (muro falso). ✅
- **Desafío:** el otro teléfono. ✅
- **Narrativa:** *"No me siguieron aquí abajo. Estoy solo con lo que escondí."*

### Loop 8 — La verdad *(Acto III · ~7 min — CLÍMAX)*
- **Acción:** la habitación de las notas; el otro teléfono reproduce tu voz. ✅
- **Alivio:** la voz "te llena" al 100% — calma total antes de la decisión. ✅
- **Recompensa:** **"YOU ARE THE RECORDING"** — la verdad. ✅
- **Desafío/Decisión:** leer la nota → **confiar / rechazar / (devolverla)**. ✅🔶
- **Narrativa:** *"Soy la grabación. ¿Rompo el bucle o doy otra vuelta?"*

---

## 2. Curva de tensión

```
Tensión
 MÁX ┤                                              ╱╲      ╱╲    ╱╲ (clímax)
     │                                   ╱╲    ╱╲  ╱  ╲    ╱  ╲  ╱  ╲
     │                        ╱╲   ╱╲   ╱  ╲  ╱  ╲╱    ╲  ╱    ╲╱    ╲
     │             ╱╲   ╱╲   ╱  ╲ ╱  ╲ ╱    ╲╱      (Samara) (Crawler) (decisión)
     │   ╱╲   ╱╲  ╱  ╲ ╱  ╲ ╱    V    V
PISO ┤▁▁╱  ╲▁╱  ╲╱    V    (la tensión NUNCA toca 0: el "paranoia floor" activo)
   0 ┼─────────────────────────────────────────────────────────────────────►
     │ L1   L2    L3    L4     L5     L6     L7      L8           tiempo →
     └ valle = punto de carga (alivio)   pico = acecho/persecución
        pendiente general ASCENDENTE: cada loop pica más alto que el anterior
```

Dos reglas implementadas como **gobernador** (ver §7):
1. **Techo:** tras un susto/persecución intensa hay enfriamiento (catch cooldown +
   grace ya existen) → la tensión no se queda clavada arriba.
2. **Suelo:** si llevas demasiado tiempo en calma total, el director **mete un beat
   de inquietud** (susurro, rama, +estrés leve) → la tensión nunca cae a 0.

---

## 3. Tabla de recompensas narrativas por loop

| Loop | Tipo de recompensa | Contenido | Desbloqueo |
|---|---|---|---|
| 1 | Información + objeto | 1ª nota, `TRUST ME`, primera carga | el bosque |
| 2 | Información + amenaza | 2ª nota, los Watchers | la cabaña |
| 3 | Refugio + información | carga plena, 3ª nota, código | el pozo |
| 4 | Objeto + visión | pila +50%, Samara | la casa |
| 5 | Objeto narrativo | foto de *ella*, llave | el dormitorio |
| 6 | Visión (cinemática) | el espejo / flashback | el sótano |
| 7 | Objeto + apertura | pila, muro falso | habitación oculta |
| 8 | **Verdad** | "eres la grabación" | **los finales** |

> **Restricción cumplida:** cada recompensa se siente **merecida** (cuesta navegar,
> evadir, resolver) y la narrativa **avanza visiblemente** cada loop (una nota, un
> giro, una verdad). Ningún loop deja "algo a medias": termina con objetivo nuevo.

---

## 4. Variación para evitar monotonía

### Bucles temáticos (cambia la dinámica dominante)
| Loops | Tema | Dinámica |
|---|---|---|
| 1–2 | **Exploración** | conocer el mapa, amenazas no letales, mucha carga |
| 3–4 | **Persecución** | entidades letales (Samara), carga más lejana |
| 5–6 | **Investigación** | interiores, puzzles (espejo), Crawler |
| 7–8 | **Clímax** | aislamiento, verdad, decisión, pocas cargas |

### Cambio de dinámicas a lo largo del juego
- **Temprano:** Shadows que retroceden con la luz, Watchers inofensivos, cargas
  frecuentes.
- **Medio:** Samara letal, casa con acechador, baterías escasas.
- **Final:** Crawler agresivo, sótano sin refugio, batería al límite.

---

## 5. Eventos que rompen el loop (5 momentos especiales)

1. **Samara emerge del pozo** (Loop 4) — cambia las reglas: un peligro letal que se
   mueve a tirones y "te siente por el agua". El refugio deja de bastar. ✅
2. **El Crawler bajo la cama** (Loop 6) — la amenaza viene de **dentro** del refugio
   (la casa) y **te sigue por pisos** (las Shadows no). ✅
3. **El espejo / cinemática Giro** (Loop 6) — flashback obligatorio; el ritmo se
   detiene para un golpe narrativo. 🔶
4. **Cambio ambiental** — la lluvia/niebla del exterior se corta al entrar (silencio
   opresivo); el sótano elimina la lluvia y amplifica tu respiración. ✅
5. **La verdad / el otro teléfono** (Loop 8) — fuerza batería a 100%, estrés a 0:
   una **calma ominosa** que rompe el ciclo justo antes del final. ✅

*(Propuesto: un 6º — encuentro con otro "superviviente" que resulta ser otra
grabación/Shadow, NPC efímero que refuerza el bucle.)* 🔶

---

## 6. Tres finales (condiciones de disparo)

```
                          ┌── ¿leíste las 4 notas + objeto de ella? ──┐
                          │ sí                                      no │
                          ▼                                            ▼
   LOOP 8 · otro teléfono → DECISIÓN ──[E] CONFIAR──────────► FINAL C: BUCLE
        "YOU ARE THE             │      (Ending 1 ✅)          das otra vuelta;
         RECORDING"              │                             despiertas en la carretera
                                 ├──[Q] RECHAZAR──────────────► FINAL B: SUBMERSIÓN
                                 │      (Ending 2 ✅)           "THEY SEE YOU NOW";
                                 │                              el campo te absorbe
                                 └──(gate ✔) bajar al agua ───► FINAL A: ESCAPE/RELEASE
                                        (propuesto 🔶)          la sueltas, el bucle se
                                                                cierra; sales de verdad
```

| Final | Nombre | Disparador | Tono |
|---|---|---|---|
| **A** | Escape / Release | completar lore + elegir soltarla | catarsis ambigua (único momento de "claridad") |
| **B** | Submersión | `[Q]` Rechazar el teléfono | terror — te unes al mundo paranormal |
| **C** | Bucle | `[E]` Confiar (default) | desesperanza — todo se repite |

> Coherente con `NARRATIVE_DESIGN §7`. A y la ramificación menor del sótano
> (llave de agua **o** espejo) dan rejugabilidad sin bifurcar el mapa.

### Decisiones implícitas que tiñen la experiencia (no el final)
- **¿Cargas completo o aceleras?** → más seguro vs más tensión sostenida.
- **¿Investigas o vas directo?** → completar el lore habilita el Final A.
- **¿Evades o confrontas (luz)?** → gestionar batería vs gestionar miedo.

---

## 7. Duración y ritmo total

| Acto | Loops | Duración objetivo | Build actual |
|---|---|---|---|
| I | L1–L2 | 20–30 min | ~8 min (condensado) |
| II | L3–L5 | 30–40 min | ~12 min |
| III | L6–L8 | 20–25 min | ~10 min |
| **Total** | 8 loops | **70–95 min** | **~30 min** |

- **Nuevo objetivo cada ~8–10 min** (8 objetivos principales + lateral: 4 notas de
  lore opcionales).
- El build actual es una versión **condensada** (~30 min). Llegar a 70–95 min
  requiere alargar cada tramo (más estructuras, más estaciones de carga, los 3
  arquetipos nuevos) — trabajo de contenido, especificado, no de sistemas.

---

## 8. Mapa de implementación

| Diseño | Archivo | Estado |
|---|---|---|
| **Gobernador de pacing — suelo de tensión** (beat de inquietud en calma larga) | `director.h` DirectorUpdate | ✅ esta sesión |
| Techo de tensión (cooldown + grace tras susto) | `director.h` `DirectorCatch` | ✅ ya existía |
| Alivio en carga (cabaña −estrés) | `director.h` | ✅ ya existía |
| Estaciones de carga extra (loops 1,2,5) | `director.h`/`world.h` | 🔶 |
| Final A (escape/release) gated por lore | `director.h` | 🔶 |
| Cinemáticas (Giro, etc.) como secuencias | nuevo módulo | 🔶 |
| Alargar actos a 70–95 min | contenido/mundo | 🔶 |

> **Esta sesión** implementa el **gobernador de pacing**: el "paranoia floor" pasa de
> pasivo (una frase) a **activo** — si llevas demasiado tiempo en calma total al aire
> libre, el director introduce un susurro + micro-tensión para que la curva nunca
> toque 0. Junto al techo (ya existente) cierra el balance tensión/alivio. Sin tocar
> la lógica de puzzles → `--probe` sigue verde.

---

*Fin del diseño de loop y pacing — Sesión 6.*
