# PARANOIA — Session 6 test: drive a full playthrough through all 3 phases,
# log flag/phase transitions, then reach an ending ("trust" or "refuse" via
# cmdline arg). Confirms the Director orchestration + both endings.
extends Node3D

func _ready() -> void:
	_setup_env()
	GameState.set_state(GameState.State.PLAY)
	var player := get_tree().get_first_node_in_group("player")
	var cam: Camera3D = player.get_node("Camera3D")
	cam.current = true
	Director.begin_play()
	if "selftest" in OS.get_cmdline_user_args():
		await _run(player)

func _frames(n: int) -> void:
	for i in n:
		await get_tree().process_frame

func _run(player) -> void:
	var which := "refuse" if "refuse" in OS.get_cmdline_user_args() else "trust"
	print("[PlayTest] ====== PLAYTHROUGH (ending: %s) ======" % which.to_upper())

	# --- Phase 1: forest path ---
	player.global_position = Vector3(0, 0, 200); player.ground_y = 0.0
	await _frames(3)
	print("[PlayTest] phase=%d objective=\"%s\"" % [Director.phase, Director.current_objective])

	# walk south past z=60 -> Phase 2
	player.global_position = Vector3(World.path_x(50), 0, 50); player.ground_y = 0.0
	await _frames(3)
	print("[PlayTest] phase=%d objective=\"%s\"" % [Director.phase, Director.current_objective])

	# past z=-85 -> Phase 3
	player.global_position = Vector3(0, 0, -90); player.ground_y = 0.0
	await _frames(3)
	print("[PlayTest] phase=%d objective=\"%s\"" % [Director.phase, Director.current_objective])

	# enter house footprint
	player.global_position = Vector3(0, 0, -108); player.ground_y = 0.0
	await _frames(3)
	print("[PlayTest] objective=\"%s\" in_house=%s" % [Director.current_objective, Director.flags.house_entered])

	# descend to basement (safe spot away from the scripted scare spawn)
	player.global_position = Vector3(4, -2.5, -112); player.ground_y = -2.5
	await _frames(3)
	print("[PlayTest] objective=\"%s\" basement_found=%s basement_scare=%s"
			% [Director.current_objective, Director.flags.basement_found, Director.flags.basement_scare])

	# --- interactable chain (Session 5 wired through the Director) ---
	player.global_position = Vector3(-6, -2.5, -110.5); player.ground_y = -2.5
	await _frames(2)
	print("[PlayTest] --- interactables ---")
	Director.use_interactable("kitchen_drawer")
	Director.use_interactable("bedroom_door")
	Director.use_interactable("mirror")
	Director.use_interactable("basement_door")
	Director.use_interactable("false_wall")
	Director.use_interactable("old_phone")
	Director.use_interactable("note")
	await _frames(2)
	print("[PlayTest] objective=\"%s\" note_read=%s entities_left=%d"
			% [Director.current_objective, Director.flags.note_read,
			get_tree().get_nodes_in_group("shadow_entity").size()])

	# --- ending choice ---
	player.global_position = Vector3(-6, -2.5, -110.5)  # at the old phone
	await _frames(2)
	print("[PlayTest] ending prompt = \"%s\"" % Director._prompt.text)
	await _screenshot()

	Director._ending(which)
	await get_tree().create_timer(3.2).timeout   # credits appear after ~2.5s
	var f := Director.flags
	print("[PlayTest] ended=%s ending_chosen=%s ending_trust=%s ending_refuse=%s"
			% [Director.ended, f.ending_chosen, f.ending_trust, f.ending_refuse])
	print("[PlayTest] credits visible = %s" % str(Director._credits.visible))
	print("[PlayTest] ====== DONE (%s) ======" % which.to_upper())
	get_tree().quit(0)

func _screenshot() -> void:
	if DisplayServer.get_name() != "headless":
		await RenderingServer.frame_post_draw
		await RenderingServer.frame_post_draw
		get_viewport().get_texture().get_image().save_png("res://session6_proof.png")
		print("[PlayTest] screenshot saved -> session6_proof.png")

func _setup_env() -> void:
	var env := Environment.new()
	env.background_mode = Environment.BG_COLOR
	env.background_color = Color(0.004, 0.004, 0.012)
	env.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	env.ambient_light_color = Color(0.12, 0.14, 0.22)
	env.ambient_light_energy = 0.6
	env.fog_enabled = true
	env.fog_light_color = Color(0.02, 0.02, 0.05)
	env.fog_density = 0.02
	var we := WorldEnvironment.new()
	we.environment = env
	add_child(we)
