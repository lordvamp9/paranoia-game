# PARANOIA — Diseño de entidades paranormales e IA

> **Sesión 3 — Diseño de criaturas + IA.** Espectros **humanoides, fotorealistas
> dentro del lenguaje visual del juego** (nítidos sobre el mundo dithered),
> **espectrales**. Anclado al sistema real `cpp/src/entity.h` (4 entidades vivas:
> Shadow, Samara, Watcher, Crawler) + 3 arquetipos nuevos propuestos.
> **Nota de IP:** Samara es un **homenaje** al arquetipo de *The Ring* (pelo negro
> mojado sobre la cara, pozo, reptar a tirones), ejecutado con arte original — **no
> se copian assets ni se usa material con copyright.**

---

## 0. Cómo se construye un espectro en este motor (base técnica)

No hay "monigotes": cada entidad es un **humanoide suave** hecho de esferas y
huesos cónicos (`EBone`/`ESph`/`ESphE` en `entity.h`), con un **cráneo esculpido**
(`EHead`: cráneo alto, mandíbula, ceja, cuencas oculares negras recesadas, nariz,
boca abierta) y **cortinas de pelo** en billboard (`DrawHair`). Se dibujan a
**resolución nativa nítida** sobre el mundo pixelado, en `BLEND_ALPHA` (de ahí la
translucidez espectral). La carne la "envejece" el dither del post. Sobre esa base
parametrizamos: **color de piel, alpha (translucidez), escala/proporción, longitud
de brazos, postura (de pie / a cuatro patas), pelo y rostro.**

Tres parámetros nuevos que esta sesión añade al `struct Entity` para dar variedad
(ver §3 y §9): **`decay`** (0 recién muerta → 1 ancestral/etérea, modula el alpha),
**`heightScale`** (tamaño real), y reuso del flag **`longArms`** para proporción.

---

## 1. Arquetipos (7)

Leyenda de estado en build: ✅ implementada · 🔶 propuesta.

### A · SAMARA — "La del pozo" (The Well Entity) ✅ *(obligatoria, homenaje)*
- **Designación:** Samara / *The Well Entity*. Narrativamente: **ella**, lo que el
  agua se llevó, deformado por cómo la recuerdas.
- **Visual (4 frases):** Una niña-mujer pálida, empapada, de ~1.4 m. Un vestido/
  camisón blanco que el agua volvió gris y translúcido, pegado al cuerpo. El pelo
  negro larguísimo cae **mojado por delante de la cara**, que nunca ves entera —
  solo destellos de una cuenca negra. La piel tiene el brillo céreo de lo que
  estuvo mucho tiempo sumergido.
- **Proporción:** más pequeña que un humano (0.82× del humanoide base); brazos
  normales pero **muñecas y dedos largos**; columna que se quiebra al reptar.
- **Deterioro:** tier 2 (deteriorada/mojada) — translúcida en los bordes, goteando.
- **Animaciones:** (1) *emerger* del pozo (3.6 s: sube dentro del brocal, se vuelca
  sobre el borde — ya implementado en `Emerging`); (2) *reptar a tirones*
  (ráfagas de 1 s a ~4.9 m/s + paradas muertas de 0.4 s — `Pursuing`); (3) *hundirse*
  de vuelta (`Retreating`); (4) *espasmo* de cabeza (`twitchT`).
- **Sonidos:** agua corriendo/goteo, respiración ahogada-raspante, **gemido de
  tono descendente** (`SfxSamaraGroan`, ya existe), una risa infantil corrompida
  (propuesta). 
- **IA (Dormant → Emerging → Pursuing → Retreating):** ver §2 diagrama. **No tiene
  Idle ni patrulla**: existe en el pozo y solo reacciona a tu cercanía al agua.
- **Detección:** **humedad/agua** — te "siente" cuando entras en el radio del pozo
  (5.2 m) si no estás bajo techo/sótano. No usa vista ni oído normales.
- **Mecánica / ¿invulnerable?:** **Invulnerable e inmatable.** Se **evade**: sus
  tirones la hacen escapable a sprint; pierde interés si te alejas >55 m del pozo,
  bajas al sótano, o rompes línea. **Mata** de un toque (`DirectorCatch`) → no
  game-over, sino respawn al checkpoint con susto y pérdida de batería (coherente
  con el bucle narrativo: no mueres, **vuelves**).
- **Firma:** el agua. Donde hay agua, ella puede estar. Te enseña a temer la sed.

### B · SHADOW — "Los que volvieron" (The Returned) ✅
- **Visual:** Humanoides altos y enjutos de color ceniza-gris, **translúcidos**,
  de **brazos demasiado largos** que cuelgan hasta las rodillas. Rostro pálido y
  demacrado con cuencas negras; una cortina de pelo ralo. Caminan **pesado y
  deliberado**, no corren — la amenaza es que no paran.
- **Proporción:** 1.05× (≈1.9 m), `longArms = true`. Hombros caídos.
- **Deterioro:** tier 2 (deteriorada). Sombra proyectada difusa a sus pies.
- **Animaciones:** caminar pesado (swing de extremidades por `walkPhase`),
  *retroceder/encogerse* cuando los iluminas de cerca (`recoilT`), *flanqueo*
  coordinado (rol chaser/flanker en `Hunting`).
- **Sonidos:** pasos lentos sordos, respiración grave, **sollozo distante** cuando
  están idle / jadeo agitado al cazar (`SfxEntityCry`).
- **IA completa (Idle→Alerted→Pursuing→Hunting→Retreating):** el arquetipo más
  "inteligente": patrullan rutas, investigan tu última posición vista, y **se
  coordinan** (cuando hay ≥2 cazando, uno persigue y otro **corta tu escape** 7 m
  por delante).
- **Detección:** **vista por luz** — tu linterna los alerta a >7 m (la luz es un
  cebo) pero de cerca (<7 m) **los hace retroceder** (la luz también es un arma
  débil); **oído** — correr con sprint a <15 m los pone en Alerta; **tacto/sexto
  sentido** — a <5 m sin luz te detectan aunque no te vean.
- **Inteligencia adaptativa:** comunican (`AlertOthers` propaga la persecución a
  45 m); reaccionan a la luz (dudan si los ciegas mucho rato); **debilidad
  explotable:** la luz de cerca + tu sprint (son más lentos que tu sprint: el
  duelo real es tu **stamina**).
- **Firma:** la jauría paciente. Nunca uno solo cuando importa.

### C · WATCHER — "El Vigilante" ✅
- **Visual:** Una figura femenina pálida, **muy alta y quieta**, de pie a lo lejos
  en la niebla, con el pelo cubriéndole por completo la cara. No tiene postura de
  ataque: solo **te mira**. Su sola presencia sube el estrés.
- **Proporción:** 1.14× (≈2.0 m), cuello y torso **alargados** (lo antinatural es
  la altura y la quietud absoluta).
- **Deterioro:** tier 3 (muy antigua/etérea) — la **más translúcida**, casi
  niebla con forma.
- **Animaciones:** **ninguna de locomoción** — solo gira la cara hacia ti
  (`facing` te sigue) y un micro-espasmo. Su animación es la **desaparición**: si
  la iluminas de cerca (<16 m) o te acercas (<3.5 m), **se borra** con un glitch.
- **Sonidos:** un susurro al aparecer/desaparecer (`SfxWhisper`), un zumbido sub-
  grave de "aura" mientras está presente (propuesto), rama partiéndose al irse.
- **IA (solo Idle/Observa → Desaparece):** **no persigue, no es letal.** Aparece a
  intervalos a tu espalda (Phase 2+), te observa, y se desvanece si la confrontas.
- **Detección:** te localiza siempre (es el campo mirándote), pero **no actúa**.
- **Inteligencia:** psicológica, no táctica — explota la paranoia. **Debilidad:**
  mirarla de frente con luz la disuelve (premia enfrentar el miedo).
- **Firma:** la sensación de ser observado hecha cuerpo. Nunca te toca; siempre
  está un poco más cerca que la última vez.

### D · CRAWLER — "El de debajo de la cama" ✅
- **Visual:** Carne pálida y casi opaca (lo **más reciente**, lo más "real"), un
  cuerpo humano roto que se mueve **a cuatro patas** con la columna torcida y la
  cabeza levantada hacia ti enseñando una **sonrisa de dientes anchos** (textura
  `texGhostFaceGrin`). Pelo cayendo hacia delante desde la nuca baja.
- **Proporción:** humana (1.0×) pero **plegada**: hombros altos, caderas bajas,
  miembros que reptan. Cabeza desproporcionadamente erguida.
- **Deterioro:** tier 1 (recién fallecida) — opaca, detalle de piel visible.
- **Animaciones:** *reptar* (brazos y piernas alternados, `ECrawler`), *acecho*
  bajo la cama (idle, llanto), *embestida* rápida (la más agresiva, `aggression
  1.25`), sigue suelos/escaleras (`ResolveGround`).
- **Sonidos:** **respiración bajo la cama**, arrastre de uñas/carne, llanto-grito
  agudo (`SfxEntityCry` tipo 3), el aviso "something under the bed is breathing".
- **IA (Idle bajo la cama → Hunt):** dormida hasta que abres el dormitorio;
  entonces **caza** por la casa, subiendo y bajando pisos (las Shadow no bajan; el
  Crawler sí te sigue por la geometría).
- **Detección:** **oído + cercanía** dentro de la casa; ignora la luz (no retrocede
  como las Shadow — es más valiente, más reciente, más rabiosa).
- **Inteligencia:** persecución inteligente por niveles (te corta en escaleras).
  **Debilidad:** espacios abiertos donde tu sprint rinde; es letal en pasillos.
- **Firma:** el miedo infantil concreto — lo que hay debajo de la cama.

### E · THE HANGED — "El Ahorcado" 🔶
- **Visual:** Un hombre **colgando** de una soga invisible entre los árboles, el
  **cuello roto y estirado**, la cabeza caída sobre el hombro, los pies a 30 cm del
  suelo balanceándose. Piel azulada de asfixia, lengua oscura, ojos saltones bajo
  el pelo. Cuando te acercas, **deja de colgar** y *cae* a perseguirte arrastrando
  la soga.
- **Proporción:** cuello **un 40% más largo** (lo roto), brazos inertes, postura
  vencida.
- **Deterioro:** tier 2. Marcas de soga, livideces.
- **Animaciones:** *péndulo* (idle, balanceo lento), *caída* (transición a chase),
  *arrastre asfixiante* (chase con tropiezos), *cuelga de nuevo* al perderte.
- **Sonidos:** crujido de cuerda y madera, **estertor de ahogo**, respiración que
  silba por una tráquea rota.
- **IA (Idle-colgando → Alert → Chase → vuelve a colgar):** estático y casi
  invisible hasta que pasas bajo él; entonces cae. Territorial de su árbol.
- **Detección:** **proximidad bajo su árbol** (como una trampa); luego oído.
- **Inteligencia:** baja-media; es una **emboscada** ambiental. **Debilidad:** si lo
  ves colgando y rodeas su árbol, no se activa. Premia mirar hacia arriba.
- **Propósito narrativo:** uno de "los que volvieron" que **se rindió** — un final
  posible para ti hecho carne. Refuerza B (el bucle, los anteriores).

### F · THE BRIDE — "La Novia" 🔶
- **Visual:** Una mujer con un **vestido de novia** empapado y enlodado, el velo
  hecho jirones, buscando algo entre la niebla con las manos extendidas. Bellísima
  y **equivocada**: la cara, bajo el velo, está mal — demasiado lisa, sin rasgos.
  Murmura un nombre que **casi** es el de *ella*.
- **Proporción:** humana, grácil, pero **dedos larguísimos** que tantean el aire.
- **Deterioro:** tier 2-3, etérea de cintura para abajo (el vestido se deshace en
  niebla).
- **Animaciones:** *deambular buscando* (idle lento, manos al frente), *acudir*
  cuando haces ruido (se **acerca** a los sonidos, no a la vista), *abrazo* letal.
- **Sonidos:** arrastre de tela mojada, un **canto nupcial desafinado**, sollozo,
  el nombre mal pronunciado.
- **IA (Idle-buscando → Hunt por sonido → Interact-abrazo):** **te confunde con
  quien busca.** No te ve: te **oye**. Si te quedas quieto y en silencio, pasa de
  largo. Cargar el celular o correr la atrae.
- **Detección:** **solo sonido** (pasos, sprint, carga del celular, respiración
  agitada por estrés). Ciega a la luz.
- **Inteligencia adaptativa:** aprende tu ruido — tras perderte dos veces, amplía
  su radio de escucha. **Debilidad:** el silencio (quedarte quieto, linterna
  apagada, no cargar cerca).
- **Propósito narrativo:** el espejo de **tu** búsqueda. Tú también buscas a alguien
  cuya cara no recuerdas. Verla es verte.

### G · THE RUNNER — "El Corredor" 🔶
- **Visual:** Una silueta delgadísima y **demasiado rápida**, que solo ves **medio
  segundo**: cruza el fondo, desaparece tras un árbol, reaparece más cerca. Piel
  tensa, extremidades finas, **sin cara** (un óvalo liso). Nunca la ves quieta.
- **Proporción:** estilizada, piernas un 30% más largas, torso corto.
- **Deterioro:** tier 3, casi un borrón — su translucidez **aumenta con la
  velocidad** (motion-smear).
- **Animaciones:** *cruce* fugaz (apariciones de <1 s), *desaparición*, *sprint*
  final directo cuando por fin se compromete.
- **Sonidos:** **ultrasonido/zumbido agudo** al cruzar, respiración acelerada,
  pies golpeando rápido y seco; silencio absoluto cuando se esconde.
- **IA (apariciones → acoso → sprint):** no persigue de forma constante: **acosa**
  con apariciones para subir tu estrés, y solo se compromete a un sprint letal
  cuando tu estrés está alto (>70). Es el clímax de tensión del exterior.
- **Detección:** vista a larga distancia; te rodea por el campo visual periférico.
- **Inteligencia adaptativa:** **reacciona a tu pánico** — cuanto más corres y más
  estrés acumulas, más se compromete. Quedarte quieto y bajar el estrés la
  "desinfla". **Debilidad:** la calma. Castiga el pánico.
- **Propósito narrativo:** la prisa que causó el choque. Corre como tú corrías esa
  noche. Te obliga a **no** correr — lección invertida del horror.

---

## 2. Diagramas de máquina de estados (3 principales)

### Samara (reactiva, 4 estados)
```
        ┌─────────┐  jugador entra radio del pozo (≤5.2m) y NO bajo techo
        │ DORMANT │ ───────────────────────────────────────────────┐
        │ (oculta)│ ◄──────────────────────────────┐               ▼
        └─────────┘            se hunde del todo    │        ┌───────────┐
             ▲                                      │        │ EMERGING  │ 3.6s
             │ y < -1.9 (vuelve al agua)            │        │ sube+vuelca│
        ┌────┴─────┐  lejos del pozo >55m / bajas   │        └─────┬─────┘
        │RETREATING│  sótano / dist>30  ◄───────────┼──────────────┘ t≥1
        │ (al pozo)│                                │
        └──────────┘ ◄──────────────────────────────┤
             ▲           toque letal → DirectorCatch │
             │                                  ┌────┴─────┐ ráfaga 1s @4.9m/s
             └──────────────────────────────────│ PURSUING │  + parada 0.4s
                                                │ (tirones)│  groan al arrancar
                                                └──────────┘
```

### Shadow (la más completa, 5 estados + coordinación)
```
   ┌──────┐ patrulla   ┌─────────┐ visto<11m & reciente   ┌──────────┐
   │ IDLE │───listen──►│ ALERTED │──────────────────────►│ PURSUING │
   │patrol│◄─12s/llega─│ va a    │◄─ luz cerca duda ──────│ persigue │
   └──────┘            │lastSeen │   (frand)               │+lead 1.4m│
      ▲                └─────────┘                         └────┬─────┘
      │ lostT>6 (se rinde, justo)                    ≥2 cazando │ ▲
      │                                                         ▼ │ <2 cazando
      │   luz<7m → RECOIL ──► RETREATING ──► IDLE          ┌──────────┐
      └────────────────────────────────────────────────── │ HUNTING  │
              AlertOthers (45m) propaga PURSUING           │rol 0:caza│
                                                           │rol 1:corta│ +7m
                                                           └──────────┘
```

### Watcher (mínima, psicológica)
```
   (spawn a la espalda, Phase2+)        luz<16m  ó  dist<3.5m
        ┌──────────────┐  ───────────────────────────────────► ┌──────────┐
        │ IDLE / OBSERVA│  gira la cara hacia ti, sube estrés   │ REMOVED  │
        │ (inmóvil)     │                                       │ glitch + │
        └──────────────┘                                        │ susurro  │
              ▲  reaparece tras 45-85s a tu espalda             └──────────┘
              └─────────────────────────────────────────────────────┘
```

---

## 3. Variedad visual y diferenciación

### Tiers de deterioro (param `decay` → alpha/translucidez)
| Tier | Nombre | `decay` | Lectura visual | Entidades |
|---|---|---|---|---|
| 1 | Recién fallecida | 0.20–0.30 | Casi humana, **opaca**, piel con detalle | Crawler |
| 2 | Deteriorada | 0.45–0.65 | Piel translúcida, bordes difusos, goteo | Shadow, Samara, Hanged, Bride |
| 3 | Muy antigua | 0.80–0.95 | **Etérea**, forma apenas definida, casi niebla | Watcher, Runner |

> Regla de render: `alpha_efectivo = alpha_base × (1 − decay×0.35)`. A más antigua,
> más se la "come" la niebla y el dither. (Implementado esta sesión.)

### Tamaños (param `heightScale`, base ≈1.8 m)
| Rango | Escala | Entidades |
|---|---|---|
| Pequeña (1.2–1.5 m) | 0.78–0.83× | **Samara** (~1.47 m) |
| Humana (1.7–1.9 m) | 1.0–1.06× | Crawler, Shadow, Hanged, Bride |
| Grande (2.0 m+) | 1.12–1.15× | **Watcher** (~2.05 m), Runner (alto y fino) |

### Proporciones anormales
- **Brazos largos:** Shadow (`longArms`), Bride (dedos), Samara (muñecas).
- **Cuello/columna:** Watcher (cuello alargado), Hanged (cuello roto +40%),
  Crawler (columna torcida horizontal).
- **Cabeza desproporcionada:** Crawler (erguida y grande), Runner (óvalo sin cara).
- **Piernas:** Runner (+30% largo) para el zancada imposible.

---

## 4. Audio especializado (sonidos por entidad)

Todo es **síntesis en tiempo real** (`audio.h`, `ParanoiaSynth`): no hay .wav.
Cada entidad tiene una **voz** dedicada (`AudioEntityVoice`/`AudioEntityUpdate`).

| Entidad | Movimiento | Ataque/toque | Vocalización | Aura/ambiental |
|---|---|---|---|---|
| Samara | goteo, agua que corre | golpe húmedo + sting | gemido descendente, risa infantil corrupta | reverb de pozo, agua bajo todo |
| Shadow | pasos sordos lentos | sting grave | sollozo lejano / jadeo agitado | respiración de jauría |
| Watcher | — (silenciosa) | — (no ataca) | susurro al aparecer/irse | zumbido sub-grave de presencia |
| Crawler | arrastre de carne, uñas | grito agudo + golpe | respiración bajo la cama, llanto | crujido de somier |
| Hanged | crujido de soga/madera | estertor de ahogo | silbido por tráquea rota | balanceo, viento en cuerda |
| Bride | arrastre de tela mojada | suspiro de "abrazo" | canto nupcial desafinado, nombre mal dicho | sollozo en bucle |
| Runner | pies rápidos secos | impacto + ultrasonido | respiración acelerada | zumbido agudo al cruzar, silencio al ocultarse |

---

## 5. Tabla de especificaciones

| Entidad | Tipo | Altura | Detección | Estado base | Letal | Debilidad | Sonido clave | Build |
|---|---|---|---|---|---|---|---|---|
| **Samara** | Espectro acuático | 1.47 m | Humedad / agua (5.2 m) | Dormant | Sí (toque) | Distancia al pozo / sótano | Agua + gemido descendente | ✅ |
| **Shadow** | Acechador en jauría | 1.90 m | Vista(luz)+oído+tacto | Idle/patrol | Sí | Luz cerca + tu sprint/stamina | Pasos + sollozo | ✅ |
| **Watcher** | Observador | 2.05 m | Omnisciente (no actúa) | Idle/observa | No | Mirarla con luz (se disuelve) | Susurro + zumbido | ✅ |
| **Crawler** | Cazador por niveles | 1.75 m | Oído + cercanía (interior) | Idle bajo cama | Sí | Espacios abiertos | Respiración bajo la cama | ✅ |
| **Hanged** | Emboscada estática | 1.80 m (cuello +40%) | Proximidad-trampa | Idle-colgando | Sí | Rodear su árbol | Crujido de soga + estertor | 🔶 |
| **Bride** | Cazadora por sonido | 1.78 m | Solo sonido | Idle-buscando | Sí | Silencio / quietud | Canto desafinado + tela | 🔶 |
| **Runner** | Acosador de pánico | 1.95 m (piernas +30%) | Vista a larga dist. | Apariciones | Sí (sprint final) | La calma (bajar estrés) | Ultrasonido + pies rápidos | 🔶 |

---

## 6. Lista de sonidos para producción (18)

**Samara:** 1) agua corriendo (loop ambiental de pozo) · 2) gemido de tono
descendente · 3) risa infantil corrupta · 4) golpe húmedo de toque.
**Shadow:** 5) paso sordo lento · 6) sollozo distante · 7) jadeo agitado de caza ·
8) sting grave de embestida.
**Watcher:** 9) susurro de aparición/desaparición · 10) zumbido sub-grave de aura.
**Crawler:** 11) respiración bajo la cama · 12) arrastre de carne/uñas · 13) grito
agudo de embestida.
**Hanged:** 14) crujido de soga y madera · 15) estertor de ahogo.
**Bride:** 16) canto nupcial desafinado + nombre mal pronunciado.
**Runner:** 17) ultrasonido agudo de cruce · 18) pies rápidos secos.
*(Transversales ya existentes: thunder, rain, heart-thump, whisper, sting.)*

---

## 7. Consideraciones para animación 3D

Modelo de animación = **procedural por huesos** (no skeletal clips), salvo que se
importen `.glb` con `Skeleton3D` en Godot. Qué animar por entidad:

| Entidad | Animaciones a producir |
|---|---|
| Samara | emerger (climb+tip), reptar a tirones (burst/stop), hundirse, espasmo de cabeza, pelo mojado con física |
| Shadow | caminar pesado, retroceso/encogerse ante luz, flanqueo, manos largas colgando |
| Watcher | giro de cabeza (look-at), micro-espasmo, **disolución** (no locomoción) |
| Crawler | reptar a 4 patas (alternado), acecho idle, embestida, transición de piso (escaleras) |
| Hanged | péndulo idle, caída (drop), arrastre asfixiante, recolgarse |
| Bride | deambular con manos al frente, girar hacia el sonido, abrazo letal, vestido→niebla |
| Runner | cruce fugaz (entrada/salida rápida), sprint comprometido, smear por velocidad |

**Prioridad de animación (lo que más vende el miedo):** (1) **look-at de cabeza**
hacia el jugador en todas; (2) el **espasmo/twitch** (ya existe, da lo "antinatural");
(3) la **física de pelo** mojado de Samara y la **disolución** del Watcher.
En Godot: `Skeleton3D` + `AnimationTree` (blend locomoción↔caza), `LookAtModifier3D`
para la cabeza, partículas/`shader` para la disolución y el smear.

---

## 8. Inteligencia: resumen comparativo

| Eje | Samara | Shadow | Watcher | Crawler | Hanged | Bride | Runner |
|---|---|---|---|---|---|---|---|
| ¿Persigue? | sí, a tirones | sí, en jauría | **no** | sí, por niveles | sí, al caer | sí, por sonido | acoso→sprint |
| ¿Aprende? | no | duda con luz | no | no | no | **sí (radio oído)** | **sí (tu pánico)** |
| ¿Coordina? | no | **sí (45 m)** | no | no | no | no | no |
| Reacción a luz | indiferente | retrocede/duda | **se disuelve** | indiferente | indiferente | ciega | indiferente |
| Reacción a carga celular | — | — | — | — | — | **se acerca** | — |
| Debilidad | distancia/sótano | luz+stamina | enfrentarla | espacio abierto | rodear árbol | silencio | calma |

> **Restricción cumplida:** cada entidad tiene una **firma** única — no son
> variantes de persecución. Samara=agua a tirones; Shadow=jauría coordinada;
> Watcher=psicológica sin ataque; Crawler=caza por pisos; Hanged=emboscada;
> Bride=por sonido; Runner=por pánico.

---

## 9. Mapa de implementación

| Diseño | Archivo | Estado |
|---|---|---|
| `decay` (translucidez por antigüedad) | `game.h` Entity + `entity.h` EntityDraw | ✅ esta sesión |
| `heightScale` (tamaños reales: Samara 0.82, Watcher 1.14…) | `game.h` + `entity.h` | ✅ esta sesión |
| Diferenciación de tint/alpha por entidad | `entity.h` EntityDraw | ✅ esta sesión |
| Hanged / Bride / Runner (3 arquetipos nuevos) | nuevo `EKind` + spawn + FSM + draw | 🔶 sesiones siguientes |
| Risa infantil de Samara, auras nuevas | `audio.h` síntesis | 🔶 |
| Migración a `Skeleton3D`/`AnimationTree` | proyecto Godot | 🔶 (Fase 3) |

> **Esta sesión** implementa en el `.exe` la **variedad visual** (deterioro +
> tamaño + proporción diferenciada por entidad) sin tocar lógica de FSM/colisión →
> los 20 checks del `--probe` siguen verdes. Los 3 arquetipos nuevos y el audio
> quedan especificados para implementarse después.

---

*Fin del diseño de entidades — Sesión 3.*
