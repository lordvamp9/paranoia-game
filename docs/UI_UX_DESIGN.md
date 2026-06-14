# PARANOIA — UI/UX: el teléfono como HUD, inventario 3D, feedback sin números

> **Sesión 5 — Diseño UX/UI de horror inmersivo.** Filosofía: **no hay HUD**
> (ni vida, ni estrés, ni munición). El **celular ES la interfaz**; el resto es
> diegético (animación, distorsión, sonido). Anclado a `phone.h` (`PhoneDrawScreen`
> + `HudDraw`) y `ui.h`.

---

## 0. Punto de partida (lo que ya existe)

| Elemento | Estado en el build |
|---|---|
| Pantalla del teléfono (RT 384×768) | ✅ batería %, barra, reloj, objetivo, mensaje, REC + timestamp, "WATCHING", "LOW BATTERY", líneas de glitch |
| HUD 2D (`HudDraw`) | ✅ barra de inventario 5 slots, panel de lectura de notas, prompt, subtítulos, lluvia |
| Inventario | ✅ **2D**, barra inferior, selección `[1-5]`, mano izquierda inspecciona el item en **3D** |
| Feedback sin números | ✅ parcial: respiración/latido por estrés, glitch, viñeta, flicker por batería |
| Sin barras de vida/estrés | ✅ ya se cumple (solo una fina tira de stamina) |

Esta sesión: **(1)** sistematiza todo el diseño UI, **(2)** diseña el **inventario
3D interactivo** y las **apps del teléfono**, **(3)** implementa el slice de
*feedback sin números* (viñeta de peligro, señal, notificación, pips de batería).

---

## 1. Arquitectura de interfaz

### Capas (de menos a más información)
```
 PANTALLA PRINCIPAL (diegética, casi vacía)
   └─ solo el teléfono en la mano (batería + reloj SIEMPRE visibles)
   └─ feedback ambiental: viñeta de peligro, grano, respiración, latido
        │  (sin barras, sin números fuera de batería/reloj)
        ▼
 TELÉFONO — HUD PRIMARIO (mirar la pantalla)
   └─ home: reloj, batería, señal, notificaciones, fila de apps
   └─ apps: Mensajes · Linterna · Cámara · Grabadora · Mapa
        │
        ▼
 INVENTARIO 3D (tecla I)
   └─ bolsillos desplegados, items low-poly manipulables
```

### Elementos siempre visibles (y solo estos)
- **Batería** del teléfono (esquina sup. izq. de su pantalla) — único número permitido
  junto al reloj.
- **Reloj** (se vuelve `??:??` con psicosis).
- **Feedback de proximidad** = **viñeta** + audio direccional (no un radar, no un número).
- **Objetivo**: en la pantalla del teléfono, no flotando en el mundo.

> **Restricción cumplida:** la pantalla principal no lleva barras de vida/estrés/
> munición. El estado del jugador se lee en su **cuerpo** (respiración, temblor de
> manos en `HandDraw`), en la **imagen** (psicosis/glitch) y en el **sonido**.

---

## 2. Interfaz del celular — mockups por pantalla

Estética: smartphone realista pero **corrupto** — fosforescencia CRT verde sobre
negro, sans-serif limpia con artefactos ocasionales, señal **siempre débil**.

### 2.1 Home
```
┌──────────────────────────────┐
│ 41%  [████▒▒▒▒▒]   ▂▄_ 03:48 │  batería(color-tier) · señal(1-2 barras) · reloj
│                          ●   │  punto rojo = notificación nueva
│                              │
│   ┌────┐ ┌────┐ ┌────┐       │
│   │MSG●│ │TORCH│ │ CAM │      │  apps (la activa se ilumina)
│   └────┘ └────┘ └────┘       │
│   ┌────┐ ┌────┐              │
│   │ REC │ │ MAP │             │
│   └────┘ └────┘              │
│                              │
│ ● REC  00:06:20              │  grabación siempre activa (diegético)
└──────────────────────────────┘
```

### 2.2 App MENSAJES (conversación con "SISTEMA")
```
┌──────────────────────────────┐
│ ‹ MENSAJES            SISTEMA │  monospace tipo terminal
│ ──────────────────────────── │
│  03:31  follow the path       │  viejos = contexto (gris)
│  03:39  TRUST ME              │  voz = MAYÚS (orden)
│  03:44  she was in the car.   │  tu cabeza = minús (azul frío)
│  03:47 ▸ REACH THE WHITE      │  ► nuevo = OBJETIVO actual (blanco)
│         HOUSE                 │
│                              │
│  [escribiendo...]            │  respuesta NO instantánea (lag realista)
└──────────────────────────────┘
```

### 2.3 App LINTERNA
```
┌──────────────────────────────┐
│ ‹ LINTERNA                    │
│ ──────────────────────────── │
│        ( ON )   ◐             │  toggle grande
│                              │
│  intensidad  [██████▒▒▒]      │  slider 0-100% (sin número: barra)
│                              │
│  ○ NORMAL      −3.7 %/min     │  3 modos (consumo como barra, no número)
│  ● BAJO CONSUMO −1.8 %/min    │
│  ○ INFRARROJO  (ve Watchers)  │
│                              │
│  consumo ahora ▸ ▮▮▮▮▯▯▯      │  drenaje en vivo como barrita
└──────────────────────────────┘
```

### 2.4 App CÁMARA
```
┌──────────────────────────────┐
│ ‹ CÁMARA            ● REC      │
│ ┌──────────────────────────┐ │
│ │   [visor con grano]      │ │  apuntar a estructuras revela lore
│ │      ◎ enfocar           │ │  "registrar" = +1%/min, y…
│ └──────────────────────────┘ │  …¿te delata? (las entidades notan el visor)
│  [ CAPTURAR ]   GALERÍA ▸     │  capturas revisables en galería
└──────────────────────────────┘
```
> **Diseño de riesgo:** usar la cámara **registra** detalles narrativos (texto que
> aparece al enfocar una estructura) pero el visor encendido **sube tu detección**
> (como cargar): ver = exponerte. Coherente con "el teléfono es la correa".

### 2.5 App GRABADORA / MAPA
- **Grabadora:** reproduce tus propias notas de voz (los flashbacks de Sesión 2);
  reproducir consume batería. Lista de clips desbloqueados.
- **Mapa:** un plano **incompleto y a mano** del campo; solo marca lo ya visitado y
  el objetivo como un punto que **parpadea mal**. Nunca da coordenadas (sin números).

---

## 3. Inventario 3D interactivo

### Acceso y transición
- Tecla **I** (o mantener el teléfono y deslizar): la cámara baja y el **bolsillo/
  mochila se despliega** en una vista cercana (transición ≤0.3 s, con un micro-lag).
- El mundo no se pausa del todo: sigues oyendo (tensión — abrir inventario es
  arriesgado, como en horror clásico).

### Representación (mockup)
```
┌──────────────────────── INVENTARIO ───────────────────────┐
│   (vista 3/4, tela oscura, luz del teléfono rasante)       │
│                                                            │
│    ╔═══╗     ╔═══╗     ╔═══╗     ╔═══╗     ╔═══╗            │
│    ║🔑 ║     ║🔑 ║     ║▢ ║     ║🔋 ║     ║📷 ║            │
│    ╚═══╝     ╚═══╝     ╚═══╝     ╚═══╝     ╚═══╝            │
│   llave    llave-2    nota     batería×●●  foto            │
│   agua     cocina                                          │
│                                                            │
│   ▸ seleccionado: BATERÍA EXTERNA                          │
│     "Alguien la dejó cargada. ¿Quién?"   [rota con RMB]    │
│   [LMB] usar   [RMB] examinar   [drag] reordenar   [I] cerrar│
└────────────────────────────────────────────────────────────┘
```

### Interacción
| Acción | Resultado | Sonido |
|---|---|---|
| **Hover** | el item se eleva y rota lento | roce suave |
| **LMB** (usar) | usa el item (batería→carga, llave→equipa) | plástico/metal según material |
| **RMB** (examinar) | zoom + rotación libre + texto de lore | "click" + paso de página |
| **Drag & drop** | reordena los slots | arrastre de tela |
| **I / Esc** | cierra (transición inversa) | cierre de cremallera |

> Reusa la infraestructura ya existente: `HandDraw` ya **dibuja en 3D** la llave,
> la nota y la batería en la mano izquierda con rotación (`wob`). El inventario 3D
> es esa misma renderización llevada a una vista dedicada.

---

## 4. Iconos low-poly (12)

Estilo: minimalista, 10–50 tris, reconocible a la silueta, material mate con un
specular tenue (la luz del teléfono los lame). Reusan los meshes generados del juego
donde es posible.

| # | Icono | Geometría (low-poly) | ~Tris | Material | En build |
|---|---|---|---|---|---|
| 1 | Llave de agua | cilindro (shaft) + toro (bow) + cubo (tooth) | 40 | hierro oxidado mate | ✅ `mshKey*` |
| 2 | Llave de cocina | igual, tinte acero | 40 | acero mate | ✅ |
| 3 | Batería externa | cubo + 2 bandas + borne | 16 | plástico verde + metal | ✅ `mshBattery` |
| 4 | Nota/carta | quad con textura manuscrita | 2 | papel | ✅ `mshNoteQ` |
| 5 | Cargador USB | cable (tubo curvo, pocas aristas) + conector (cubo) | 22 | goma negra + metal | 🔶 |
| 6 | Linterna/teléfono | prisma + quad emisivo (pantalla) | 12 | vidrio + plástico | ✅ `mshPhoneBody` |
| 7 | Foto de *ella* | marco quad + imagen interior | 6 | cartón + foto | 🔶 |
| 8 | Botella de agua | cilindro + tapa | 18 | plástico translúcido | 🔶 |
| 9 | Medicina/pastillas | cilindro corto + tapa | 14 | plástico ámbar | 🔶 |
| 10 | Mechero | prisma + ruedita | 16 | metal | 🔶 |
| 11 | Trozo de espejo | quad triangulado irregular | 4 | espejo (reflectante) | 🔶 |
| 12 | Llave inglesa/herramienta | barra + cabeza | 20 | acero | 🔶 |

> **Restricción cumplida:** hermosos pero funcionales — la **silueta** identifica
> cada uno sin etiqueta; la etiqueta es refuerzo, no requisito.

---

## 5. Feedback sin números (tabla)

| Estado | Feedback VISUAL (sin número) | Feedback AUDITIVO | Fuente en build |
|---|---|---|---|
| Vitalidad normal | postura erguida, mano firme | respiración calmada | `HandDraw` tremor=f(stress) |
| Vitalidad débil | mano temblorosa, balanceo, dip al aterrizar | respiración acelerada | `tr = stress*0.004` |
| Psicosis leve (50-75%) | grano sutil, borde apenas teñido | ambiente más presente | post `s` |
| Psicosis severa (25-50%) | glitch de bandas, desaturación, CA | susurros, latido | post `g`, `SfxWhisper` |
| Ruptura (<25%) | artefactos, flicker, viñeta cerrada | respiración amplificada, tinnitus | `lowBat` (Sesión 4) |
| **Entidad cerca** | **viñeta ligeramente roja** | latido cardiaco | `D.danger` (esta sesión) |
| **Entidad AQUÍ** | **viñeta totalmente roja** | latido rápido + audio direccional | `D.danger→1` |
| Seguro | pantalla limpia, viñeta neutra | viento, lluvia | — |
| Dirección de amenaza | — (no flecha) | **audio 3D posicional** | `AudioEntityUpdate` (paneo) |

> El **audio direccional** sustituye a cualquier indicador de dirección en pantalla:
> oyes **dónde** está la amenaza (sistema posicional ya implementado). Sin flechas,
> sin radar, sin minimapa de enemigos.

---

## 6. Diseño visual

### Paleta
| Rol | Color | Uso |
|---|---|---|
| Primario | verde oscuro CRT `#0A1A12`/`#96E696` | pantalla del teléfono, apps |
| Secundario | negro `#000000` | fondo, vacío |
| Acento claro | hueso `#EEE8E2` | la "voz" (mensajes en mayús) |
| Acento frío | azul `#96AAC8` | tu cabeza (mensajes en minús) |
| Warning | rojo `#FF463C` | batería <25%, viñeta de peligro, REC |
| Corrupción | magenta/cian | aberración cromática, glitch RGB |
| Estados de batería | verde/amarillo/naranja/rojo | 4 tiers (Sesión 4) |

### Tipografía
- **Monoespaciada** para Mensajes (terminal/CRT) y timestamps.
- **Sans-serif** limpia para UI principal (objetivo, apps).
- **Glitch tipográfico** ocasional (`CorruptText`: a→4, e→3, o→0…) escalado por
  psicosis — ya implementado.

### Transiciones
- Suaves pero con **micro-lag** deliberado (≤0.3 s, a veces un frame de tirón):
  el teléfono no es perfecto. Sonido de "click" realista al tocar.

---

## 7. Flujo de navegación UI

```
        [JUEGO]
           │ mirar el teléfono (siempre en mano)
           ▼
        [HOME del teléfono] ──tap──► [MENSAJES] ──‹── back
           │  │  │  │                [LINTERNA] ─ toggle/slider/modo
           │  │  │  └────────────────[CÁMARA] ── enfocar/capturar/galería
           │  │  └───────────────────[GRABADORA] ─ reproducir clips
           │  └──────────────────────[MAPA] ─ ver visitado
           │
           │ tecla I (en cualquier momento)
           ▼
        [INVENTARIO 3D] ◄──► hover/LMB usar/RMB examinar/drag
           │ I / Esc
           ▼
        [JUEGO]
        ─────────────────────────────────────────
        Esc ► [PAUSA] ► [AJUSTES] (rebind, audio, dificultad)
```

---

## 8. Accesibilidad y claridad

- **Crítico siempre accesible:** batería y reloj nunca se ocultan; objetivo a un tap;
  inventario a una tecla.
- **Complejidad escalonada:** pantalla principal casi vacía → detalle en teléfono →
  detalle total en inventario. El jugador novato no se abruma; el atento profundiza.
- **Sin tutoriales explícitos:** las instrucciones llegan como **mensajes
  diegéticos** ("the light is a door"), no como pop-ups de tutorial.
- **Opciones propuestas:** alto contraste de subtítulos, tamaño de fuente del
  teléfono, reducir glitch (accesibilidad fotosensible — importante con tanto post-FX),
  recordatorio de batería opcional (pip sonoro al cruzar 25%).

---

## 9. Mapa de implementación

| Diseño | Archivo | Estado |
|---|---|---|
| Viñeta de peligro roja por proximidad | `shaders.h` (uDanger) + `gfx.h` + `director.h` (D.danger) | ✅ esta sesión |
| Barras de señal + punto de notificación | `phone.h` PhoneDrawScreen | ✅ esta sesión |
| Conteo de baterías como **pips** (sin número) | `phone.h` HudDraw | ✅ esta sesión |
| Apps del teléfono (Mensajes/Linterna/Cámara/…) | `phone.h` (estados de app) + input | 🔶 |
| Inventario 3D interactivo (estado nuevo) | nuevo `GS_INVENTORY` + render | 🔶 |
| Iconos low-poly nuevos (USB, foto, botella…) | `phone.h`/`entity.h` mesh gen | 🔶 |
| App cámara (registrar/galería) | nuevo módulo | 🔶 |

> **Esta sesión** implementa el núcleo de "**feedback sin números**": el peligro se
> comunica con una **viñeta roja** (no un medidor), el teléfono gana **señal** y
> **notificación** (HUD diegético), y el inventario deja de mostrar el número de
> baterías (lo muestra como **puntos**). Sin tocar lógica → `--probe` sigue verde.

---

*Fin del diseño UI/UX — Sesión 5.*
