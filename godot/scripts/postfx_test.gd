# PARANOIA — Session 7 test: confirm the post-processing pipeline.
# Baseline (grain + vignette), then a glitch spike + high stress (CA/desat/
# scanlines/glitch blocks), then low battery (9 Hz flicker dim).
extends Node3D

func _ready() -> void:
	_setup_env()
	GameState.set_state(GameState.State.PLAY)
	var player := get_tree().get_first_node_in_group("player")
	var cam: Camera3D = player.get_node("Camera3D")
	cam.current = true
	if "selftest" in OS.get_cmdline_user_args():
		await _run(player)

func _frames(n: int) -> void:
	for i in n:
		await get_tree().process_frame

func _run(player) -> void:
	print("[PostFXTest] PostFX autoload present = %s, shader bound = %s"
			% [str(PostFX != null), str(PostFX._mat != null)])
	# look down the path so there's geometry to grain/vignette
	player.global_position = Vector3(0, 0, 120)
	player.ground_y = 0.0
	Director.stress = 5.0
	await _frames(5)
	await _shot("session7_baseline.png")
	print("[PostFXTest] baseline: stress=5 -> grain + vignette")

	# glitch spike + high stress
	PostFX.spike_glitch(1.0)
	Director.stress = 92.0
	await _frames(2)
	print("[PostFXTest] glitch spike -> PostFX.glitch=%.2f, stress=92 (CA/desat/scanlines)" % PostFX.glitch)
	await _shot("session7_glitch.png")

	# low battery flicker
	var phone := get_tree().get_first_node_in_group("phone")
	if phone:
		phone.battery = 10.0
	Director.stress = 30.0
	await _frames(3)
	print("[PostFXTest] battery=10%% -> flicker path active (battery<0.2)")
	await _shot("session7_lowbat.png")

	# flashes
	PostFX.flash_white()
	print("[PostFXTest] flash_white=%.2f (decays 0.7/s)" % PostFX.flash_white_amt)
	print("[PostFXTest] DONE")
	get_tree().quit(0)

func _shot(name: String) -> void:
	if DisplayServer.get_name() != "headless":
		await RenderingServer.frame_post_draw
		await RenderingServer.frame_post_draw
		get_viewport().get_texture().get_image().save_png("res://" + name)
		print("[PostFXTest] screenshot -> %s" % name)

func _setup_env() -> void:
	var env := Environment.new()
	env.background_mode = Environment.BG_COLOR
	env.background_color = Color(0.020, 0.012, 0.031)
	env.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	env.ambient_light_color = Color(0.10, 0.13, 0.22)
	env.ambient_light_energy = 0.18           # a touch higher so the grain reads in the shot
	env.fog_enabled = true
	env.fog_light_color = Color(0.020, 0.012, 0.031)
	env.fog_density = 0.008
	env.glow_enabled = false
	var we := WorldEnvironment.new()
	we.environment = env
	add_child(we)
