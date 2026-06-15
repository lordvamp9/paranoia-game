# PARANOIA — Main scene root (Session 2 playable slice).
# Hosts the night environment, the procedural World and the Player, captures the
# mouse and routes the player's interact signal to the Director. Entities and the
# phone arrive in later sessions.
extends Node3D

func _ready() -> void:
	_setup_environment()
	GameState.set_state(GameState.State.PLAY)   # captures the mouse
	Director.begin_play()                        # start phase orchestration

	var player := get_tree().get_first_node_in_group("player")
	if player:
		if not player.interacted.is_connected(_on_player_interact):
			player.interacted.connect(_on_player_interact)
		player.stepped.connect(_on_player_step)
		var cam: Camera3D = player.get_node("Camera3D")
		cam.current = true
		print("[Main] player spawned at %v (ground_y=%.2f), camera current=%s"
				% [player.global_position, player.ground_y, str(cam.current)])

	# Headless self-test: `... res://scenes/Main.tscn -- selftest` runs a few
	# seconds then quits cleanly (used for CI/verification).
	if "selftest" in OS.get_cmdline_user_args():
		await get_tree().create_timer(2.5).timeout
		if DisplayServer.get_name() != "headless":
			await RenderingServer.frame_post_draw
			var img := get_viewport().get_texture().get_image()
			var path := "res://session2_proof.png"
			img.save_png(path)
			print("[Main] screenshot saved -> %s" % ProjectSettings.globalize_path(path))
		print("[Main] selftest complete — quitting")
		get_tree().quit(0)

func _setup_environment() -> void:
	# Night (Session 7 spec): no sun/moon, very low blue ambient, thin fog, no
	# glow — the shader grain/vignette carries the look. Flashlight is the light.
	var env := Environment.new()
	env.background_mode = Environment.BG_COLOR
	env.background_color = Color(0.020, 0.012, 0.031)   # #050308
	env.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	env.ambient_light_color = Color(0.10, 0.13, 0.22)
	env.ambient_light_energy = 0.02
	env.fog_enabled = true
	env.fog_light_color = Color(0.020, 0.012, 0.031)    # #050308
	env.fog_density = 0.008
	env.glow_enabled = false
	env.tonemap_mode = Environment.TONE_MAPPER_FILMIC
	var we := WorldEnvironment.new()
	we.environment = env
	add_child(we)

func _on_player_interact() -> void:
	# E near an interactable is handled by Interactable nodes; this is a fallback log.
	pass

func _on_player_step(sprinting: bool) -> void:
	# Player footstep -> AudioSynth (Session 1). Surface picks hard (indoor wood/
	# stone) vs soft (forest) timbre.
	var p := get_tree().get_first_node_in_group("player")
	var surface := "grass"
	if p:
		var indoor: bool = p.ground_y < -1.0 \
			or (absf(p.global_position.x) < 6.0 and absf(p.global_position.z + 110.0) < 5.0)
		surface = "wood" if indoor else "grass"
	Audio.footstep(surface)
