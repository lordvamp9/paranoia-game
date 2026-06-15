# PARANOIA — AudioSynth smoke test (Session 1 deliverable).
# Confirms the GDExtension loads, AudioSynth registers, and the DSP produces
# signal. render_test() is a deterministic, audio-device-free verification;
# play_wind() additionally drives the live AudioStreamGenerator path.
extends Node

func _ready() -> void:
	if not ClassDB.class_exists("AudioSynth"):
		printerr("[AudioTest] FAIL: AudioSynth class not registered (GDExtension not loaded)")
		get_tree().quit(2)
		return

	var synth := AudioSynth.new()
	add_child(synth)  # triggers _ready(): sets up AudioStreamGenerator + wind, start()

	var peak_wind: float = synth.render_test(1.0)
	print("[AudioTest] wind render peak       = %.5f" % peak_wind)

	synth.play_sting()
	var peak_sting: float = synth.render_test(0.5)
	print("[AudioTest] sting render peak      = %.5f" % peak_sting)

	synth.play_footstep("wood")
	synth.play_branch_crack()
	synth.play_whisper()
	synth.play_drip()
	synth.play_pickup()
	synth.play_unlock()
	synth.play_ui_beep()
	synth.play_entity_breath(0, 3.0)
	var peak_mix: float = synth.render_test(0.3)
	print("[AudioTest] one-shots mix peak     = %.5f" % peak_mix)

	synth.set_stress(90.0)
	synth.play_scare()
	var peak_scare: float = synth.render_test(0.5)
	print("[AudioTest] scare render peak      = %.5f" % peak_scare)

	# Live path: start wind through the AudioServer (audible in a real session).
	synth.play_wind(0.5)
	print("[AudioTest] play_wind(0.5) -> live AudioStreamGenerator running")

	var ok := peak_wind > 0.0001 and peak_sting > 0.0001 \
		and peak_mix > 0.0001 and peak_scare > 0.0001
	print("[AudioTest] RESULT: %s" % ("PASS" if ok else "FAIL"))

	await get_tree().create_timer(1.5).timeout
	get_tree().quit(0 if ok else 1)
