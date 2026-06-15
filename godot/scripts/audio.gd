# PARANOIA — Audio autoload: owns the single AudioSynth GDExtension instance
# (Session 1) and exposes it to gameplay. Instantiated via ClassDB so the
# script has no compile-time dependency on the native class (keeps --import
# clean even if the extension isn't loaded during the import pass).
extends Node

var synth: Node = null

func _ready() -> void:
	if ClassDB.class_exists("AudioSynth"):
		synth = ClassDB.instantiate("AudioSynth")
		add_child(synth)
		synth.start()
	else:
		push_warning("[Audio] AudioSynth GDExtension not available — audio disabled")

func _missing() -> bool:
	return synth == null

# --- entity 3D breath (Session 4) ---
func play_entity_breath(id: int, distance: float) -> void:
	if synth: synth.play_entity_breath(id, distance)

func stop_entity_breath(id: int) -> void:
	if synth: synth.stop_entity_breath(id)

# --- ambience / one-shots ---
func set_stress(s: float) -> void:
	if synth: synth.set_stress(s)

func set_wind(level: float) -> void:
	if synth: synth.set_wind(level)

func footstep(surface: String) -> void:
	if synth: synth.play_footstep(surface)

func sting() -> void:
	if synth: synth.play_sting()

func scare() -> void:
	if synth: synth.play_scare()

func whisper() -> void:
	if synth: synth.play_whisper()

func ui_beep() -> void:
	if synth: synth.play_ui_beep()
