# PARANOIA

> **YOU ARE BEING OBSERVED.**

A first-person psychological horror game by **vamp9**.

You wake on a dark forest road with blood on your face and a hole where your
memory used to be. There was a crash. You were driving to find someone — you
can't remember who. The only thing you have is the phone in your hand: its
flashlight, its battery, and a single message on the screen. *Trust me.*

There are no weapons. You can't fight what walks out here. You can only move,
manage your light, read what you left behind, and decide — at the very end —
whether the phone is telling the truth.

![genre](https://img.shields.io/badge/genre-psychological%20horror-8a0000)
![platform](https://img.shields.io/badge/platform-Windows%20x64-2d2d2d)
![license](https://img.shields.io/badge/license-MIT-444)

---

## 🎮 Download & Play

Grab **`PARANOIA-win64.exe`** from
[**Releases**](https://github.com/lordvamp9/paranoia-game/releases) — a single
portable executable. No installation. Double-click and play.

> 🎧 **Headphones strongly recommended.** The audio is 3D-positional — you will
> hear them before you see them.

| Input | Action |
|---|---|
| `WASD` / Mouse | Move / Look |
| `Shift` | Sprint (limited stamina) |
| `F` | Flashlight |
| `E` | Interact / Read |
| `1`–`5` | Inventory |
| `Esc` | Pause |

Controls, audio levels, mouse sensitivity and **display mode (borderless window
or fullscreen)** are all configurable in **Settings**. `F11` toggles display mode
in-game.

A single playthrough runs about 30 minutes. There are two endings.

---

## 🌑 Features

- **The smartphone is everything** — your camera, your only light, your map of
  objectives, and the voice that keeps telling you to trust it. Its screen
  glitches and its battery dies; charge it where you can.
- **A distinct visual language** — the world is rendered in a grainy, dithered,
  low-resolution style, while you, your hands, your items and the things hunting
  you are rendered sharp and real on top of it. You never belong to that place.
- **The hunted, not the hunter** — pale spectres with ruined faces drift the
  forest; a thing on all fours waits under the bed; something climbs out of the
  old well if you get too close. Each has its own voice. Light buys you seconds,
  not safety.
- **A story you reassemble** — notes in your own handwriting, scattered across
  the woods and the house, give back the memory the crash took.
- **Atmosphere** — dynamic rain and distant thunder, a piano score that turns to
  dissonance when you're watched, fog you can lose yourself in.
- **Three areas** — the forest path, the liminal woods (a fountain, a coded
  cabin, the well), and the white house with its locked rooms, basement and a
  truth hidden behind a false wall.

## 🛠 Built with

C++17 and [raylib](https://www.raylib.com/) (OpenGL). Custom rendering, a
real-time software-synthesized audio engine, and dynamic lighting with shadow
mapping. Ships as a single self-contained Windows executable.

### Build from source

```bash
git clone https://github.com/lordvamp9/paranoia-game.git
cd paranoia-game/cpp
make          # needs w64devkit (MinGW-w64) + raylib 6.0
```

## 👥 Credits

Design, code, audio and art — **vamp9**.

## 📜 License

[MIT](LICENSE) © 2026 **vamp9**

---

*TRUST ME — DON'T LOOK BACK — YOU ARE BEING OBSERVED*
