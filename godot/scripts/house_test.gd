# PARANOIA — Session 5 test: walk ground -> upstairs -> back -> basement without
# falling through geometry (continuous ground-zone resolution), and fire every
# interactable, confirming Director flags + signals.
extends Node3D

func _ready() -> void:
	_setup_env()
	GameState.set_state(GameState.State.PLAY)
	var player := get_tree().get_first_node_in_group("player")
	var cam: Camera3D = player.get_node("Camera3D")
	cam.current = true
	Director.interactable_used.connect(func(id): print("[HouseTest]   signal interactable_used(%s)" % id))

	if "selftest" in OS.get_cmdline_user_args():
		await get_tree().process_frame   # let House._ready register zones
		_test_floors(player)
		_test_interactables()
		await _screenshot(player, cam)
		get_tree().quit(0)

func _test_floors(player) -> void:
	# named waypoints (world x,z). Path goes UP the stairs then back DOWN, then
	# into the basement — never teleporting between levels.
	var path := [
		["front door", Vector2(0, -106)],
		["living room", Vector2(-3, -106)],
		["hall", Vector2(0, -110)],
		["up-stairs foot", Vector2(-5, -110.3)],
		["up-stairs mid", Vector2(-5, -112.4)],
		["UPSTAIRS top", Vector2(-5, -114.4)],
		["up-stairs mid", Vector2(-5, -112.4)],
		["up-stairs foot", Vector2(-5, -110.3)],
		["hall", Vector2(0, -110)],
		["basement door", Vector2(5, -110.4)],
		["basement mid", Vector2(5, -112.4)],
		["basement foot", Vector2(5, -114.3)],   # descend fully before stepping off
		["BASEMENT floor", Vector2(3, -114)],
		["hidden room", Vector2(-6, -110.5)],
	]
	player.ground_y = 0.0
	var max_jump := 0.0
	var prev: Vector2 = path[0][1]
	print("[HouseTest] --- multi-floor walk ---")
	for entry in path:
		var wp: Vector2 = entry[1]
		var steps := int(maxf(1.0, prev.distance_to(wp) / 0.25))
		for s in range(1, steps + 1):
			var p: Vector2 = prev.lerp(wp, float(s) / steps)
			var target: float = player._resolve_ground(p.x, p.y)
			var jump: float = absf(target - player.ground_y)
			max_jump = maxf(max_jump, jump)
			player.ground_y = target          # fine steps -> snap (continuous)
			player.global_position = Vector3(p.x, target, p.y)
		print("[HouseTest]   %-16s -> y=%5.2f" % [entry[0], player.ground_y])
		prev = wp
	var ok := max_jump < 0.8
	print("[HouseTest] max per-step jump = %.3f m  => %s (no falling through)"
			% [max_jump, "PASS" if ok else "FAIL"])

func _test_interactables() -> void:
	print("[HouseTest] --- interactables ---")
	Director.use_interactable("kitchen_drawer")
	Director.use_interactable("bedroom_door")     # now unlocked (have water key)
	Director.use_interactable("basement_door")
	Director.use_interactable("cabin_keypad")
	Director.use_interactable("mirror")
	Director.use_interactable("note")
	Director.use_interactable("false_wall")       # locked: basement_scare false
	Director.flags["basement_scare"] = true
	Director.use_interactable("false_wall")       # now opens
	Director.use_interactable("old_phone")
	var f := Director.flags
	print("[HouseTest] flags: water_key=%s bedroom_open=%s basement_open=%s cabin_open=%s"
			% [f.water_key, f.bedroom_open, f.basement_open, f.cabin_open])
	print("[HouseTest] flags: mirror_solved=%s note_read=%s false_wall_open=%s old_phone_found=%s truth=%s"
			% [f.mirror_solved, f.note_read, f.false_wall_open, f.old_phone_found, f.truth])
	print("[HouseTest] stress after mirror+phone = %.0f" % Director.stress)

func _screenshot(player, cam: Camera3D) -> void:
	# stand in the living room looking toward the kitchen/hall
	player.global_position = Vector3(-3, 0, -107)
	player.ground_y = 0.0
	player.yaw = PI            # face -... toward house interior (+z->-z); look around
	player.pitch = -0.1
	await get_tree().process_frame
	if DisplayServer.get_name() != "headless":
		await RenderingServer.frame_post_draw
		await RenderingServer.frame_post_draw
		get_viewport().get_texture().get_image().save_png("res://session5_proof.png")
		print("[HouseTest] screenshot saved -> session5_proof.png")

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
