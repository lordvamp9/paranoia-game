# PARANOIA — Batería del celular: sistema de supervivencia y psicosis

> **Sesión 4 — Diseño de recursos y psicología del jugador.** La batería es la
> **columna vertebral** del gameplay. Anclado a los números reales del build
> (`phone.h` `PhoneUpdate`, `director.h` modelo de estrés, `shaders.h` post-FX).
> Coherente con Sesión 2 (la batería es **la correa**: cargar = obedecer; quedarse
> sin carga = la mente se rompe y **el bucle te reclama**, no un game-over injusto).

---

## 0. Punto de partida: lo que el build ya hace

| Mecánica | Valor real actual (`phone.h`/`director.h`) |
|---|---|
| Drenaje base | `0.025 %/s` = **1.5 %/min** |
| + Linterna | `+0.062 %/s` = **+3.72 %/min** (total ~5.2 %/min con luz) |
| × Estrés | `× (1 + estrés/100)` → hasta **×2** a estrés 100 |
| × Entidad cerca | `× 2` cuando `entitiesNear` |
| Carga (cabaña) | `20 %/s` → **lleno en 5 s** (única estación) |
| Batería externa (item) | **+50 %** instantáneo (`InvSelect(4)`) |
| Penalización al ser atrapado | **−8 %** (mínimo 5 %) |
| A 0 % | la linterna se apaga — **NO mueres** (el diseño de esta sesión lo cambia) |
| Psicosis (glitch) | la conduce el **estrés**, no la batería (esta sesión la liga a la batería) |

Dos cambios de diseño de esta sesión, ambos pedidos por el prompt:
1. **La batería conduce la psicosis** (no solo el estrés).
2. **0 % = muerte por psicosis** → secuencia → **respawn en checkpoint** (bucle
   justo, no softlock).

---

## 1. Sistema de batería detallado

### A. Consumo (tabla base + modificadores)

Diseño objetivo: **presión constante pero superable sin cargar** un acto completo.
Una carga del 100 % debe durar ~**18–20 min con linterna** (un acto entero), menos
bajo persecución. Valores propuestos (afinan los reales, mantienen el feel):

| Acción / estado | Drenaje | Notas de diseño |
|---|---|---|
| **Base** (existir, REC siempre on) | **1.5 %/min** | El teléfono graba siempre: nunca drena 0. |
| **Linterna encendida** | **+3.7 %/min** | El gran consumidor: usarla es **decisión estratégica**. |
| **Registrar entorno** (apuntar a un objeto/nota) | **+1.0 %/min** mientras enfocas | Leer lore tiene coste → tensión "¿leo o ahorro?". |
| **Mensaje/flashback en pantalla** | **+0.5 %** por mensaje (pico) | Cada revelación cuesta un sorbo de batería. |
| **Conectividad** (la voz "te guía") | **+0.3 %/min** pasivo en Acto II+ | La correa consume sola: el guía no es gratis. |
| **Modificador estrés** | **× (1 + estrés/100)** | Tener miedo gasta: psicología ↔ recurso. |
| **Modificador entidad en persecución** | **× 2** | Huir es caro: incentiva resolver, no esconderse. |
| **Modo bajo consumo** (nuevo, §4) | **× 0.5** | Apaga conectividad/registro; objetivos+linterna siguen. |

> **Peso de la linterna:** apagada, una carga rinde >60 min (imposible morir por
> tiempo); encendida bajo estrés/persecución, <12 min. **La luz es el dial de
> dificultad que el propio jugador gira.**

### B. Puntos de carga (10)

Distribuidos para que **no haya dead zones**: nunca estás a más de ~un acto de una
fuente. ✅ existe · 🔶 propuesto.

| # | Punto | Tipo | Velocidad | Riesgo | Justificación narrativa | Build |
|---|---|---|---|---|---|---|
| 1 | **Coche del accidente** (spawn) | Cargador de coche | 20 %/30 s (mientras el motor tose) | Bajo; expuesto en la carretera | Donde empezó todo; arranca con ~30 %, no 100 % | 🔶 |
| 2 | Poste de luz caído (camino) | Cable pelado | 10 %/30 s, parpadea | Te inmoviliza en el sendero del Shadow | Infraestructura muerta del campo | 🔶 |
| 3 | **Fuente seca** (panel) | Panel solar | **lenta**: 5 %/30 s, solo sin tormenta | Largo = vulnerable; la lluvia lo corta | Irónico: energía limpia en un lugar muerto | 🔶 |
| 4 | **La cabaña** | Cargador de pared | **20 %/s, lleno en 5 s** | Quieto 5 s; si te mueves, se desconecta | Refugio real; baja estrés (−5/s) | ✅ |
| 5 | **Pila del pozo** | Batería externa | **+50 %** instantáneo | Recogerla despierta a Samara | Alguien la dejó antes (otra vuelta) | ✅ |
| 6 | **Generador del porche** | Generador portátil | 20 %/30 s; **hace ruido → atrae** | Atrae al acechador interior | Última gente que intentó refugiarse | 🔶 |
| 7 | Cuadro eléctrico (cocina) | Cargador de pared | 15 %/30 s | Cerca del Crawler | La casa aún tiene un hilo de corriente | 🔶 |
| 8 | **Pila del sótano** | Batería externa | **+50 %** instantáneo | Oscuridad total, aislamiento | Reserva escondida | ✅ |
| 9 | Toma del muro falso | Cargador de pared | 10 %/30 s | Te expone de espaldas a la sala | Detrás de lo que escondiste | 🔶 |
| 10 | **El otro teléfono** | (clímax) | **fuerza 100 %** | Ninguno: calma ominosa | "La voz te llena" al revelar la verdad | ✅ |

**Riesgo de carga (regla general):**
- **Vulnerabilidad:** durante la carga **fija** (cabaña/generador) no puedes moverte
  sin desconectar; las entidades **sí** pueden alcanzarte (te atrapan → respawn).
- **Sonido:** generador (6) y cargador de pared hacen ruido → suben el radio de
  detección de la Bride/acechadores. Panel solar y baterías externas son silenciosos.
- **Duración fija:** las estaciones de pared/generador tienen tiempo mínimo; las
  baterías externas son instantáneas pero **finitas** (2 en todo el juego).

### C. Tipos de cargadores (resumen)

| Tipo | Velocidad | Silencioso | Repetible | Dónde |
|---|---|---|---|---|
| Cargador de coche | 20 %/30 s | sí | sí (hasta que el motor muere) | accidente |
| Cargador de pared | 10–15 %/30 s | **no** | sí | estructuras |
| Generador portátil | 20 %/30 s | **no (ruidoso)** | sí | porche |
| Batería externa | +50 % instantáneo | sí | **no (consumible)** | pozo, sótano |
| Panel solar | 5 %/30 s | sí | sí, **solo sin lluvia** | claros |
| Cabaña (especial) | full/5 s | sí | sí | refugio |

---

## 2. Sistema de psicosis (conducido por batería)

### A. Progresión de síntomas por rango

| Batería | Estado | Visual | Audio | UI del teléfono | Entidades |
|---|---|---|---|---|---|
| **75–100 %** | Normal | limpio (solo grano/dither base) | ambiente normal | nítida, verde | normales |
| **50–75 %** | Paranoia leve | viñeta leve, aberración cromática sutil | ambiente más presente, goteo | el HUD **parpadea** a veces; reloj ok | igual |
| **25–50 %** | Psicosis severa | **glitch de bandas**, desaturación, CA fuerte | susurros/voces, latido | mensajes **se corrompen** (`CorruptText`), reloj `??:??` | más erráticas en visión periférica (Watchers) |
| **0–25 %** | Punto de ruptura | pantalla llena de artefactos, **flicker** de apagado | sonido distorsionado, **tu respiración amplificada** | "LOW BATTERY" rojo parpadeante; % en rojo | convergen |
| **0 %** | Colapso | **negro total con distorsión** | grito + corte | pantalla negra | te alcanzan |

> Implementación: la batería baja **alimenta el mismo `uGlitch`/`uStress`** del post
> (esta sesión liga `lowBat = (25−bat)/25` al glitch). Así el descenso de batería
> *se siente* como pérdida de cordura, no como una barra que baja.

### B. Muerte por psicosis (a 0 %)

Secuencia (~4 s), implementada esta sesión:
1. La batería toca 0 % → `psychosis = true`, jugador **inmovilizado**.
2. **Glitch máximo**, estrés a 100, `SfxScare` + latido, las entidades letales
   pasan a **Hunting** y convergen hacia ti.
3. Pantalla a negro creciente + **"THE MIND BROKE"** (la mente se rompió).
4. **El bucle te reclama:** respawn en el último checkpoint con **30 %** de batería,
   estrés alto, breve gracia anti-death-loop, y **"YOU ARE BEING OBSERVED"**.

> Por qué respawn y no game-over duro: coherencia con el canon ("siempre volvemos")
> **y** la restricción "la muerte debe sentirse como fracaso del jugador, no
> injusticia". Pierdes progreso de batería y posición, pero el campo no te suelta.
> (Variante Pesadilla, §3: la muerte a 0 % **sí** es game-over definitivo.)

---

## 3. Incentivos, riesgo y dificultad

### A. Dilemas estratégicos (diagrama)

```
  ┌─ DILEMA 1: "30% y un checkpoint lejos" ──────────────────────────┐
  │  Llegar directo (rápido, arriesgado) ──► si fallas: psicosis      │
  │  Desviarte a cargar (seguro, lento) ───► gastas batería en llegar │
  └──────────────────────────────────────────────────────────────────┘
  ┌─ DILEMA 2: "Luz vs oscuridad" ───────────────────────────────────┐
  │  Linterna ON  → ves, pero −3.7%/min y atraes/ahuyentas Shadows    │
  │  Linterna OFF → ahorras y eres invisible, pero te pierdes/Crawler │
  └──────────────────────────────────────────────────────────────────┘
  ┌─ DILEMA 3: "Cargar completo vs salir con 50%" ───────────────────┐
  │  Quedarte (full) → 5s+ vulnerable, ruido atrae                    │
  │  Salir con 50%  → móvil ya, pero vuelves a la presión antes       │
  └──────────────────────────────────────────────────────────────────┘
  ┌─ DILEMA 4: "Gastar la batería externa ahora o guardarla" ────────┐
  │  Usar +50% ya  → alivio inmediato, pero no la tendrás en el clímax│
  │  Guardarla     → aguantas con poco, seguro para el Acto III       │
  └──────────────────────────────────────────────────────────────────┘
```

### B. Tensión narrativa unida al recurso

- `BATERÍA 15% — ÚLTIMA CARGA HACE 6:20` en pantalla = urgencia diegética.
- Los avisos usan la **voz tipográfica** de Sesión 2: `LOW BATTERY` (orden, mayús)
  vs `dont let it die` (tu cabeza, minús).
- Gameplay y narrativa son **el mismo medidor**: la correa que te guía es la que te
  mata si la sueltas.

### C. Balance de dificultades

| Modo | Drenaje | Puntos de carga | Muerte a 0 % | Batería externa |
|---|---|---|---|---|
| **Easy** | ×0.7 | +2 extra | respawn (gracia larga) | 3 |
| **Normal** | ×1.0 | 10 | respawn checkpoint | 2 |
| **Difícil** | ×1.4 | −2 (8) | respawn (gracia corta) | 1 |
| **Pesadilla** | ×1.7 | 6, **nunca cargan al 100 %** (tope 80 %) | **game-over definitivo** | 0 |

---

## 4. Interfaz del celular (UX)

### A. Indicador de batería — 4 tiers de color (implementado esta sesión)

```
 ┌─────────────────────────────┐   esquina superior izq. del teléfono
 │  73%  [██████▒▒▒]      03:48 │   ← 75-50% = AMARILLO
 │                             │
 │        REACH THE WHITE HOUSE│   ← objetivo (corrompido con psicosis)
 │                             │
 │           she is still      │   ← mensaje voz-interna (azul frío)
 │            inside.          │
 │                             │
 │  ● REC  00:06:20            │
 │            LOW BATTERY      │   ← solo <20%, rojo parpadeante
 └─────────────────────────────┘
```

| % | Color | Comportamiento |
|---|---|---|
| 100–75 | **verde** | estable |
| 75–50 | **amarillo** | estable |
| 50–25 | **naranja** | parpadeo ocasional del HUD |
| 25–0 | **rojo** | parpadeo + "LOW BATTERY" |
| 0 | pantalla negra | muerte por psicosis |

### B. Acciones relacionadas (propuestas)

- **Modo bajo consumo** (tecla dedicada): drenaje ×0.5; desactiva conectividad y
  registro; **mantiene objetivos y linterna**. Trade-off: no lees lore ni recibes
  flashbacks mientras esté activo.
- **Uso por aplicación:** una pantalla de ajustes del teléfono muestra qué consume
  ("LINTERNA 71% · GRABACIÓN 18% · SEÑAL 11%") — diegético, refuerza las decisiones.
- **Modo Supervivencia:** todo apagado salvo objetivo + linterna (extremo del bajo
  consumo, para el Acto III).

---

## 5. Progresión de batería por acto + curva de tensión

```
Batería
100% ┤●╲ ACTO I (100→70)            ACTO II (70→30)         ACTO III (30→0)
     │  ╲╲  baja presión      presión creciente        crisis total
 70% ┤   ╲▼cabaña            ╲                                │
     │    ╲___╱╲             ╲╲  pila pozo                    │
 50% ┤        ╲╲___╱╲(porche) ╲╲___╱╲                         │
     │          ╲      ╲       ╲     ╲╲ pila sótano           │
 30% ┤           ╲      ╲       ╲     ╲╲___╱╲                 │
     │            ╲      ╲       ╲          ╲╲                ▼ otro teléfono
  0% ┼─────────────╲──────╲───────╲──────────╲╲______________●(=100%)→ clímax
     └─────────────────────────────────────────────────────────────────►
Intensidad                                                  tiempo →
narrativa  ▁▁▂▂▃▃▃▄▄▅▅▅▆▆▆▆▇▇▇▇▇████  (sube monótona; pico en el otro teléfono)
```

Lectura: la batería **oscila en sierra** (cae, cargas, cae) con valles cada vez más
bajos y cargas cada vez más lejanas; la **intensidad narrativa sube monótona**. Los
dos se cruzan en el clímax: justo cuando la batería tocaría fondo en el Acto III,
**el otro teléfono te llena al 100 %** — alivio mecánico que coincide con el horror
de descubrir que eres la grabación. *Ese cruce es el diseño.*

---

## 6. Restricciones (checklist)

- ✅ **Columna vertebral:** todo (luz, lectura, miedo, huida) pasa por la batería.
- ✅ **Nunca un chore:** no hay "rellenar barras" porque sí; cada carga es un
  punto narrativo (Sesión 2) y cada gasto una decisión (luz/lectura/huida).
- ✅ **Sin dead zones:** 10 puntos distribuidos uno por sub-zona; baterías externas
  como red de seguridad portátil.
- ✅ **Muerte justa:** drenaje predecible + avisos claros (`LOW BATTERY`, color) +
  respawn con 30 % → morir es por decisiones, no por sorpresa del sistema.

---

## 7. Mapa de implementación

| Diseño | Archivo | Estado |
|---|---|---|
| Psicosis conducida por batería (glitch ramping <25 %) | `gfx.h` GfxRenderFrame | ✅ esta sesión |
| Muerte por psicosis a 0 % + respawn en checkpoint | `director.h` + `game.h` (Director fields) | ✅ esta sesión |
| 4 tiers de color de batería (verde/amarillo/naranja/rojo) | `phone.h` PhoneDrawScreen | ✅ esta sesión |
| 10 puntos de carga (coche/poste/solar/generador/cuadros/muro) | `director.h` `CurrentInteractables` + `world.h` | 🔶 sesiones siguientes |
| Modo bajo consumo / uso por app | `phone.h` + input | 🔶 |
| Modos de dificultad | `config.h` + drenaje en `phone.h` | 🔶 |

> **Esta sesión** implementa el **corazón de la mecánica**: la batería ahora puede
> matarte (psicosis a 0 %), conduce la psicosis visual, y se lee en 4 colores. Sin
> tocar la lógica de puzzles → los 20 checks del `--probe` siguen verdes.

---

*Fin del diseño de batería — Sesión 4.*
