extends Node
## Director — the orchestrator. Port of cpp/src/director.h.
## Drives phases, ~30 progress flags, puzzles (code 2741, mirror, false wall,
## keys), the two endings, stress level and the respawn checkpoint.
## Autoload singleton. This is where most gameplay iteration happens, so it lives
## in GDScript (fast iteration, signal-based events) — see docs §4.2.

signal phase_changed(phase: int)
signal ending_reached(ending: int)   # 1 = trust, 2 = refuse

var phase: int = 0
var stress: float = 0.0
var game_time: float = 0.0

# Respawn checkpoint (in-memory in the original; persisted to paranoia.sav here — docs §6).
var checkpoint := Vector3(0, 0, 200)
var checkpoint_yaw: float = 0.0

# Progress flags (subset; full set mirrors Director struct in cpp/src/game.h).
var flags := {
	"water_key": false, "cabin_open": false, "in_house": false,
	"kitchen_key": false, "bedroom_open": false, "basement_open": false,
	"mirror_solved": false, "hidden_open": false, "truth": false,
	"note_read": false, "well_battery": false, "samara_done": false,
}

func reset() -> void:
	phase = 0
	stress = 0.0
	game_time = 0.0
	for k in flags: flags[k] = false

func start() -> void:
	reset()
	phase_changed.emit(phase)

func _process(delta: float) -> void:
	game_time += delta
	# Phase 3 (later session): port DirectorUpdate — puzzle checks, spawns, endings.
