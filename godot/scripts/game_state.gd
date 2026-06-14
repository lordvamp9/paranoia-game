extends Node
## GameState — top-level finite state machine.
## Port of the state loop in cpp/src/main.cpp (GS_MENU … GS_CREDITS).
## Autoload singleton. Emits `state_changed` so scenes react without polling.

enum State { MENU, INTRO, PLAY, PAUSE, SETTINGS, CREDITS }

signal state_changed(from: State, to: State)

var state: State = State.MENU
var state_time: float = 0.0

func set_state(next: State) -> void:
	if next == state:
		return
	var prev := state
	state = next
	state_time = 0.0
	# State-entry hooks (mouse capture, music mode) — see main.cpp lines 191-197.
	match next:
		State.MENU, State.PAUSE, State.SETTINGS, State.CREDITS:
			Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
		State.PLAY:
			Input.mouse_mode = Input.MOUSE_MODE_CAPTURED
	state_changed.emit(prev, next)

func _process(delta: float) -> void:
	state_time += delta
	# Phase 1 (Session 2): drive per-state input/update, mirroring main.cpp.
