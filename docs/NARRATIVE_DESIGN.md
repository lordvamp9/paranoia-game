# PARANOIA — Diseño de narrativa, progresión y cinemáticas

> **Sesión 2 — Game design (horror psicológico, ambiental tipo Silent Hill 2).**
> Diseño coherente con el canon **ya implementado** en `cpp/src/director.h` y
> `phone.h` (no se reinventa la historia: se sistematiza y se expande). IP propia,
> sin material con copyright de Silent Hill / The Ring — solo el *lenguaje* del
> género (ambigüedad, culpa, niebla, sin respuestas limpias).

---

## 0. Canon establecido (punto de partida)

Lo que el juego actual ya afirma (notas de lore + mensajes + el giro del sótano):

- Despiertas en una carretera del bosque, sangre en la cara, sin memoria. Hubo un
  choque. *Conducías para encontrar **a ella*** — no recuerdas su cara.
- Encuentras notas **con tu propia letra** que no recuerdas haber escrito:
  *"Sigue el camino. Llega a la casa. No confíes en la luz."*
- *"Has estado aquí antes. **Siempre volvemos.** La casa blanca se queda con lo que
  toma. Ella sigue dentro. **Tú también.**"*
- El teléfono te habla: **"TRUST ME."** Te guía paso a paso.
- En la habitación oculta del sótano hay **otro teléfono**. Al examinarlo:
  **"YOU ARE THE RECORDING."**
- Dos finales: **CONFIAR** en el teléfono / **RECHAZARLO**.
- El teléfono **siempre está grabando** (REC + timestamp en pantalla).

Sobre eso construimos la lectura completa, manteniendo la ambigüedad de SH2.

---

## 1. Sistema de narrativa ambiental

### 1.1 ¿Quién eres?

** daniel "vamp" loyola — no, mejor sin nombre claro.** Eres **el conductor**.
La única identidad firme es funcional: el que graba, el que vuelve. Pistas
ambientales (licencia en el coche destrozado, el nombre medio borrado en las
notas) sugieren un nombre que **nunca se confirma del todo** — porque tú mismo ya
no estás seguro de ser esa persona.

Trasfondo (se infiere, nunca se narra en bloque):

- Ibas conduciendo de noche, bajo lluvia, **a buscar a alguien** — *ella*. Una
  discusión, una llamada, una promesa rota: el detalle exacto se deja abierto.
- **Tú provocaste el choque.** El volante "se resbaló" justo cuando su nombre
  estaba en la punta de tu lengua. La culpa es el motor emocional (registro SH2:
  el horror es el castigo que te impones).
- No eres del todo "tú". Eres una **grabación** — un eco del conductor que ya
  recorrió esto. El campo te reproduce. Por eso *siempre volvemos*.

### 1.2 ¿Qué pasó en el accidente? (revelación gradual)

| Capa | Lo que el jugador cree | Lo que se revela |
|---|---|---|
| Superficie | "Tuve un accidente y estoy perdido." | Hay un coche envuelto en un árbol. |
| Media | "Vine a buscar a alguien." | Ella iba **contigo** en el coche. |
| Profunda | "Sobreviví." | **Ella no.** Se ahogó / murió en el agua (el pozo, la fuente, el sótano inundado). |
| Última | "Estoy intentando salir." | Llevas **incontables vueltas** intentando devolver lo que el choque se llevó. El teléfono te mantiene en el bucle "para ayudarte". |

### 1.3 ¿Por qué estás atrapado?

El campo no es un lugar: es un **proceso de duelo congelado**. La casa blanca "se
queda con lo que toma" — se quedó con *ella*, y contigo como guardián/grabación
que vuelve a buscarla y nunca la encuentra (o nunca la deja ir). La batería del
teléfono es la **correa**: mientras tenga carga, la voz te guía y el bucle
continúa. El misterio crece porque cada vuelta deja **una nota más en tu letra**
que la anterior — estás acumulando evidencia de que ya lo viviste.

### 1.4 ¿Quiénes/qué son las entidades? (deliberadamente ambiguas)

| Entidad | Lectura "real" sugerida | Lectura psicológica | En el build |
|---|---|---|---|
| **Shadow** (acechadores pálidos) | Otras grabaciones / conductores anteriores que ya fracasaron. | Tus versiones pasadas; la vergüenza que te persigue por el camino. | `EKind::Shadow`, patrullan y cazan |
| **Samara** (emerge del pozo) | **Ella.** Lo que se ahogó, deformado por cómo la recuerdas. | Tu culpa hecha cuerpo: sube cuando te acercas al agua. | `EKind::Samara`, sale del pozo |
| **Watcher** (figuras lejanas) | Testigos del bucle; el campo mirándote. | La sensación de ser observado/juzgado. Nunca te tocan. | `EKind::Watcher`, apariciones distantes |
| **Crawler** (bajo la cama) | Lo que hiciste / no hiciste esa noche, escondido en la casa. | El recuerdo que evitas mirar de frente. | `EKind::Crawler`, bajo la cama |

> **Regla SH2:** ninguna de estas lecturas se confirma en pantalla. Las notas y
> el segundo teléfono insinúan; el jugador completa.

### 1.5 Los 7 beats narrativos

| # | Beat | Vehículo principal | Dónde (geografía del build) |
|---|---|---|---|
| **B1** | *Despertar culpable.* Choque, sangre, una voz que dice confía. | **Cinemática Intro** + 1ª nota | Carretera, spawn (z≈200) |
| **B2** | *No es la primera vez.* Encuentras notas con tu letra. | Nota #2 (fuente) + mensajes | Bosque liminal, fuente |
| **B3** | *El agua se la llevó.* El pozo gime; algo sube. | Ambiental (pozo, Samara) + nota #3 | Pozo (-72,-52), cabaña |
| **B4** | *La casa se queda con lo que toma.* Entras; demasiado silencio. | Mensaje + ambiental (casa) | Casa blanca (0,-110) |
| **B5** | *Ese no es tu reflejo.* El espejo recuerda lo que tú borras. | Puzzle del espejo + cinemática **Giro** | Dormitorio (planta alta) |
| **B6** | *Tú eres la grabación.* El otro teléfono. | Hallazgo del 2º teléfono + **flash** | Habitación oculta del sótano |
| **B7** | *Confiar o romper el bucle.* La decisión. | Cinemática **Preludio** + **Finales** | Sótano, ante la nota final |

---

## 2. Estructura de checkpoints y objetivos

### 2.1 La batería como recurso NARRATIVO (reconciliación con el build)

El prompt pide que la progresión sea "cargar el celular" y plantea actos por % de
batería (0-30 / 30-70 / 70-100). El build actual hace lo contrario: la batería
**drena** y solo se carga en la cabaña. Reconciliamos con un modelo de **doble
lectura** que además *es* el horror del juego:

- **Batería de supervivencia (la que ves):** drena con el tiempo, la linterna, el
  estrés y la cercanía de entidades (ya implementado en `phone.h`). Es tensión
  momento a momento. Puede caer a 0 → oscuridad → vulnerable.
- **"Carga" narrativa (oculta, 0→100%):** un medidor de progreso que **sube ~10%
  en cada punto de carga/checkpoint**. Es la columna que marca los actos. El
  jugador nunca ve este número directo; lo *siente* porque cada carga va seguida
  de una revelación y de que la voz te deja ir más hondo.
- **La ambigüedad:** en estrés alto la pantalla **corrompe el porcentaje**
  (`CorruptText` ya lo hace: 23% → "23%"/"2E%"/"??"). ¿El teléfono te miente sobre
  su propia batería? Esa duda es el recurso narrativo: cargar = **someterte** a la
  voz a cambio de seguir; vaciarte = el mundo se vuelve más "real" y hostil pero
  más honesto (los Watcher aparecen más, las notas se leen sin corromper).

> Cargar no es solo sobrevivir: **es obedecer.** Cada estación de carga es un
> punto donde aceptas la guía del teléfono y el bucle te premia con el siguiente
> fragmento. Por eso cada checkpoint = un punto de carga (cumple el requisito).

### 2.2 Diez checkpoints clave

Cada uno: **(C)** punto de carga · **(R)** revelación · **(D)** desbloqueo ·
**(T)** tensión. ✅ = ya existe en el build · 🔶 = propuesto para implementar.

| # | Checkpoint | Carga narr. | (R) Revelación | (D) Desbloqueo / mecánica | (T) Tensión |
|---|---|---|---|---|---|
| **CP1** | 🔶 El coche destrozado (spawn) | 10% | B1: sangre, "iba a buscarla" (nota #1) | Tutorial de luz/movimiento; objetivo inicial | Susurro lejano, un Shadow al borde del camino |
| **CP2** | 🔶 El mojón del camino (z≈120) | 20% | El Shadow se planta en el sendero y desaparece | Aprendes que la luz **atrae** tanto como protege | 1er encuentro guionizado (ya existe en `Phase1Script`) |
| **CP3** | ✅🔶 La fuente seca (-35,20) | 30% | B2: nota #2, "siempre volvemos" | Se abre el bosque liminal; aparecen Watchers | Goteo, susurros que "no son palabras" |
| **CP4** | ✅ **La cabaña** (60,-30) — *única carga del build hoy* | 40% | El código 2741 se revela bajo la luz en el cartel | **Carga real de batería (5 s)** + refugio (baja estrés) | Si te quedas quieto cargando, algo se acerca |
| **CP5** | 🔶 El brocal del pozo (-72,-52) | 50% | B3: nota #3, el agua, "ella sigue dentro" | Recoges una pila (well battery, ya existe) | **Samara emerge** — primer peligro letal real |
| **CP6** | 🔶 El porche de la casa (z≈-100) | 60% | B4: "demasiado silencio" | Entras a la casa; cambia el ambiente (lluvia muffled) | El acechador interior patrulla el fondo |
| **CP7** | 🔶 La cocina (cajón) | 70% | Foto/objeto de *ella* en un cajón | Llave de cocina → dormitorio | El Crawler se anuncia (llanto bajo la cama) |
| **CP8** | 🔶 El dormitorio / espejo | 80% | B5: "ese no es tu reflejo" → **Cinemática Giro** | Puzzle del espejo abre el sótano | Susto del espejo (+22 estrés, glitch) |
| **CP9** | 🔶 El sótano (bajada) | 90% | "No te siguieron hasta aquí abajo" | Muro falso suena hueco → habitación oculta | Aislamiento, sin lluvia, solo tu respiración |
| **CP10** | 🔶 El otro teléfono (hab. oculta) | 100% | B6: **"YOU ARE THE RECORDING"** → flash blanco | Lees la nota final → **decisión de finales** | Batería forzada a 100% (la voz "te llena") |

> **Justificación de mecánica por checkpoint (restricción cumplida):** CP1 luz/mov,
> CP2 luz-como-cebo, CP3 zona abierta+Watchers, CP4 carga+refugio, CP5 pila+peligro
> letal, CP6 transición interior, CP7 llaves/cajones, CP8 puzzle espejo, CP9
> exploración con linterna direccional (muro hueco), CP10 decisión final.

### 2.3 Estaciones de carga (geografía de la "correa")

El build tiene **1** estación (cabaña). El diseño propone **3 reales + 7 simbólicas**:

- **Reales (recargan batería de supervivencia):** Cabaña (CP4), un **generador en
  el porche** (CP6, 🔶) y **el otro teléfono** (CP10, fuerza 100% — ya en el build).
  Distribuidas para que el Acto II no sea una sola fuente.
- **Simbólicas (suben solo la "carga narrativa"):** el resto de checkpoints marcan
  progreso sin recargar, manteniendo la presión de supervivencia. Así el jugador
  **siempre** siente escasez aunque progrese: cargar de verdad es un alivio escaso
  y deliberado.

---

## 3. Progresión de la historia (3 actos)

```
ACTO I — "El camino"            ACTO II — "El campo"            ACTO III — "La casa"
carga narr. 0→30%               carga narr. 30→70%              carga narr. 70→100%
CP1 · CP2 · CP3                 CP4 · CP5 · CP6 · CP7           CP8 · CP9 · CP10 → finales
─────────────────────          ─────────────────────          ─────────────────────
Despertar del choque.          Patrones: notas con tu          Clímax: el espejo y el
Exploración del sendero.       letra, el pozo, la voz          otro teléfono. "Eres la
Primer Shadow (no letal):      que sabe demasiado. El          grabación." Última
lo paranormal "existe".        jugador entiende que ESTO       confrontación interna.
Tono: desorientación.          NO ES NORMAL: es un bucle.      Decisión: confiar/romper.
                               Samara = primer peligro real.   2-3 finales.
```

- **Acto I (0-30%):** ritmo lento, mucha niebla y silencio, sustos de bajo coste
  (ramas, una figura que no caza). Enseña reglas sin matar. Beats B1-B2.
- **Acto II (30-70%):** sube el ritmo. Aparecen Watchers y el primer enemigo
  **letal** (Samara desde el pozo). Las notas conectan los puntos: empiezas a
  reconocer tu propia letra. El refugio (cabaña) hace que cargar **se sienta**.
  Beats B3-B4-B5(inicio).
- **Acto III (70-100%):** interiores, claustrofobia, puzzle del espejo, el Crawler,
  y el hallazgo del segundo teléfono. Clímax psicológico y decisión. Beats B5-B6-B7.

---

## 4. Sistema de cinemáticas (estilo SH2: oscuro, desaturado, claridad puntual)

Principios visuales compartidos: paleta casi monocroma azul-verdosa, grano y
dither (reutiliza el `post.gdshader`), cortes a negro, audio que precede a la
imagen, **nunca** un primer plano que explique. Máx. 3 min.

### Cinemática 1 — INTRO: "El choque" · ~2:00
- **Qué pasa:** Negro. Lluvia y un limpiaparabrisas. Faros temblando. Una voz
  femenina dice algo que **no se entiende** (filtrada). Un volantazo, vidrio,
  silencio. Plano subjetivo: tu mano busca el teléfono en el barro; la pantalla se
  enciende: **"TRUST ME."** Te levantas en el sendero. (El build ya hace un
  `flashBlack` + "TRUST ME" al empezar; esto lo expande.)
- **Recursos:** modelo del coche destrozado + árbol; parabrisas con shader de
  lluvia; SFX (lluvia, metal, vidrio, voz femenina procesada — sintetizable en
  `ParanoiaSynth`); 1 textura de pantalla de teléfono. Sin modelos de personaje.
- **Gameplay:** arranca en el spawn con batería al ~30% (no llena: refuerza
  escasez). Fija el objetivo CP1.

### Cinemática 2 — GIRO (mitad): "El reflejo" · ~1:30
- **Qué pasa:** En el dormitorio, al resolver el espejo. El reflejo **no te imita**:
  se queda quieto mientras tú te mueves, luego se gira hacia el agua. Cortes de
  1-2 frames: una mujer en el coche, agua subiendo, tu mano soltando la suya.
  Vuelve a negro. (Engancha con `fMirrorScare`/`fMirrorSolved` ya existentes.)
- **Recursos:** modelo de "reflejo" (reusa el humanoide); 3-4 stills de flashback
  (planos fijos desaturados); SFX agua + sting (ya hay `SfxSting`).
- **Gameplay:** desbloquea el sótano; sube el estrés base; a partir de aquí las
  notas se leen **sin** corromperse (la verdad empuja).

### Cinemática 3 — PRELUDIO FINAL: "La habitación que escondiste" · ~1:15
- **Qué pasa:** El muro falso se hunde. Avanzas hacia el otro teléfono sobre una
  mesa. En la pared, decenas de notas con tu letra — **todas iguales**. El teléfono
  reproduce tu propia voz grabada diciendo las instrucciones que creías nuevas.
  Flash blanco: **"YOU ARE THE RECORDING."** (Ya existe el flash + mensaje; se
  envuelve en una secuencia.)
- **Recursos:** props de notas (decals, ya hay sistema de decals); modelo del 2º
  teléfono; audio: tu voz en bucle (sintetizada/procesada).
- **Gameplay:** fuerza batería a 100%, estrés a 0 (calma ominosa). Abre la decisión.

### Cinemática 4 — FINALES (2-3 variantes) · ~1:00–1:30 c/u
Ramifican según la decisión y la "carga narrativa" / secretos (ver §6).

- **A · CONFIAR ("Otra vuelta más"):** aceptas el teléfono. Flash blanco creciente,
  *"TRUST ME"*, y vuelves a despertar en la carretera — el REC nunca paró. Bucle.
  *Recursos:* fundido blanco, loop de audio de intro. *(Build: `Ending(1)`.)*
- **B · RECHAZAR ("Te ven ahora"):** rompes el teléfono. La luz muere. **"THEY SEE
  YOU NOW"** — todas las entidades convergen en la oscuridad. ¿Liberación o
  condena? Ambiguo. *Recursos:* negro creciente, sting, voces convergentes.
  *(Build: `Ending(2)`.)*
- **C · 🔶 "Devolverla" (final secreto):** solo si leíste las 4 notas de lore **y**
  recogiste el objeto de *ella* (CP7). Bajas al agua del sótano por voluntad propia
  y la sueltas: el bucle se cierra de verdad. Negro tranquilo, una sola gota.
  *Recursos:* plano del agua, un susurro femenino final claro (único momento de
  "claridad" SH2). *(Propuesto: gate por `loreRead[4]` + flag nuevo.)*

---

## 5. Sistema de mensajes del celular (árbol de 28 mensajes)

Interfaz: la pantalla realista actual (`PhoneDrawScreen`): barra de batería,
reloj que se vuelve `??:??` con estrés, objetivo centrado arriba, mensaje grande
transitorio, REC + timestamp abajo. Tipos:
**[OBJ]** objetivo · **[WRN]** warning · **[FLB]** flashback/cinemático ·
**[HINT]** pista ambiental.

| # | Tipo | Disparador | Texto en pantalla |
|---|---|---|---|
| M01 | OBJ | Inicio (CP1) | `FOLLOW THE PATH` |
| M02 | FLB | t+3.5 s | `TRUST ME` |
| M03 | HINT | 1ª nota leída | `you wrote this. you don't remember.` |
| M04 | WRN | Shadow a la vista (Phase1 fired[0]) | `don't run. it likes when you run.` |
| M05 | WRN | Linterna ON cerca de entidad | `the light is a door. for both of you.` |
| M06 | OBJ | Entra Phase 2 | `REACH THE WHITE HOUSE — north` |
| M07 | FLB | Cerca de la fuente | `she was in the car. wasn't she.` |
| M08 | HINT | Cartel bajo la luz | `2 — 7 — 4 — 1` |
| M09 | OBJ | Código correcto | `CHARGE. then keep going.` |
| M10 | WRN | Cargando + entidad cerca | `stay still. 23%. something is close.` |
| M11 | HINT | Sale de la cabaña | `the cabin remembers your hands.` |
| M12 | OBJ | Rumbo al pozo | `THE WELL — take what's down there` |
| M13 | WRN | Samara emerge | `don't look at the water.` |
| M14 | FLB | Nota #3 leída | `the white house keeps what it takes.` |
| M15 | OBJ | Llega al porche | `GET INSIDE` |
| M16 | WRN | Dentro, acechador | `it's in here with you. so are you.` |
| M17 | OBJ | Dentro | `FIND A WAY DOWN` |
| M18 | HINT | Cajón cocina | `her face. almost. then it's gone.` |
| M19 | OBJ | Llave de cocina | `THE BEDROOM UPSTAIRS` |
| M20 | WRN | Crawler (bajo la cama) | `something under the bed is breathing.` |
| M21 | OBJ | Dormitorio | `THE MIRROR REMEMBERS` |
| M22 | FLB | Susto del espejo | `that is not your reflection.` |
| M23 | OBJ | Espejo resuelto | `GO DOWN` |
| M24 | HINT | Sótano | `they did not follow you down here.` |
| M25 | HINT | Muro hueco (luz de frente) | `the wall sounds hollow.` |
| M26 | FLB | Otro teléfono | `YOU ARE THE RECORDING` |
| M27 | OBJ | Verdad | `READ THE NOTE` |
| M28 | OBJ | Decisión | `[E] TRUST THE PHONE      [Q] REFUSE` |

Warnings de sistema (transversales, ya parcialmente en el build):
`LOW BATTERY` (<20%), `WATCHING` (estrés>45%, parpadea), `SHE FOUND YOU` /
`IT TOUCHED YOU` (al ser atrapado), `UNPLUGGED` (te mueves cargando).

> **Voz consistente:** los OBJ son imperativos en MAYÚSCULAS (órdenes de la voz);
> los FLB/HINT van en minúsculas (tu propia cabeza, dudando). Ese contraste
> tipográfico cuenta quién habla sin decirlo.

---

## 6. Diagrama de progresión vs. batería

Doble curva: **carga narrativa** (sube en escalón por checkpoint) vs **batería de
supervivencia** (sierra: drena y sube en las 3 estaciones reales de carga).

```
100% ┤                                                              ╭● CP10 (forzada 100%)
     │  carga narrativa (oculta) ──────────────────╮        ╭───────╯
 70% ┤                              ╭───────────────╯ CP8   │  ← umbral ACTO III
     │              ╭───────────────╯ CP4..CP7             │
 30% ┤  ╭───────────╯ CP1..CP3                              │  ← umbral ACTO II
     │ ╱                                                    │
  0% ┼●─────────────────────────────────────────────────────────────────────►
     │ \    batería supervivencia (lo que ves)                    progreso →
     │  \╲      ╱╲(recarga cabaña CP4)   ╱╲(porche CP6)
     │   ╲╲____╱  ╲___________________╱  ╲____________╱╲___ (cae si abusas de luz)
     │    drena    drena               drena            ↑ peligro: 0% = oscuridad
     └─ ACTO I ──┼──────── ACTO II ────────┼──────── ACTO III ───────┘
        CP1 CP2 CP3   CP4  CP5  CP6  CP7      CP8   CP9   CP10
        ▲recarga real: CP4 (cabaña) · CP6 (porche) · CP10 (otro teléfono, =100%)
```

Lectura: el **progreso** sube monótono (carga narrativa); la **tensión** oscila
(batería real). Los valles de batería justo antes de cada estación de carga son
los picos de miedo. Las dos curvas convergen en CP10, donde la voz "te llena" al
100% justo cuando descubres que eres su grabación: el alivio mecánico coincide
con el horror narrativo. *Ese* es el recurso narrativo.

---

## 7. Sistema de ramificación

```
                         ┌── leíste 4 notas de lore + objeto de ella (CP7)? ──┐
                         │ sí                                              no │
                         ▼                                                    ▼
   ── CP10: el otro teléfono ──►  DECISIÓN  ──[E] CONFIAR──►  FINAL A "Otra vuelta"
        "YOU ARE THE RECORDING"      │                         (bucle, default)
                                     ├──[Q] RECHAZAR──────────► FINAL B "Te ven ahora"
                                     │                          (ruptura ambigua)
                                     └──(solo si gate ✔) ──────► FINAL C "Devolverla"
                                                                 (cierre real, secreto)
```

- **Tronco lineal** hasta CP10 (coherente con el build: una sola ruta de puzzles).
  La rejugabilidad SH2 está en **qué descubres**, no en bifurcar el mapa.
- **Ramas en el clímax:** A (confiar) y B (rechazar) ya existen
  (`Ending(1)`/`Ending(2)`). **C es propuesto**, gated por completar el lore — el
  típico "final extra" de SH2 que premia la atención, no la habilidad.
- **Caminos alternativos menores (opcionales, 🔶):** el sótano puede abrirse con la
  **llave de agua** (pozo) *o* resolviendo el **espejo** — ya está así en el build
  (`fWaterKey || fMirrorSolved`). Esto da dos órdenes de exploración sin romper la
  linealidad. Quien va primero al pozo vive antes a Samara; quien va al espejo vive
  antes el giro. Misma historia, distinto orden de revelación.

---

## 8. Coherencia con SH2 (checklist de restricciones)

- ✅ **Ambigüedad:** ninguna entidad ni final se "explica"; todo admite lectura
  real y psicológica. La verdad se insinúa con props y voz, nunca con un monólogo.
- ✅ **Horror psicológico, no gore:** el motor es la culpa y el bucle; las amenazas
  castigan, no salpican.
- ✅ **Cada checkpoint justifica mecánica o narrativa** (tabla §2.2, fila por fila).
- ✅ **Batería = recurso narrativo:** doble lectura (supervivencia vs correa),
  corrupción del % bajo estrés, cargar = obedecer. No es solo un número.
- ✅ **Cinemáticas ≤ 3 min**, desaturadas, con claridad puntual (el final C).

---

## 9. Mapa de implementación (qué toca el código)

| Diseño | Archivo del build | Estado |
|---|---|---|
| Árbol de 28 mensajes (M01-M28) | `director.h` (objetivos/mensajes) + `phone.h` (render) | parcial → enriquecer |
| Tipografía MAYÚS vs minús (voz vs cabeza) | `phone.h` `PhoneDrawScreen` | 🔶 nuevo |
| Carga narrativa oculta (0→100%, por checkpoint) | `Director` struct: `float narrCharge` | 🔶 nuevo |
| 3 estaciones de carga reales | `director.h` `CurrentInteractables` | 1/3 (cabaña ✅) |
| Final C secreto (gate por lore) | `director.h` `Ending`/`DirectorKey` | 🔶 nuevo |
| 4 cinemáticas como secuencias | nuevo `cinematics.h` o estados | 🔶 nuevo |

> **Esta sesión** cablea en el `.exe` la capa de **mensajes/voz** (segura, sin
> tocar lógica de puzzles → los 20 checks del `--probe` siguen verdes). El resto
> (estaciones extra, carga narrativa, final C, cinemáticas) queda especificado
> para sesiones siguientes.

---

*Fin del diseño narrativo — Sesión 2.*
