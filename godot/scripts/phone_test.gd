# PARANOIA — Session 3 test: battery drain, objective text, flashlight toggle.
extends Node3D

func _ready() -> void:
	_setup_env()
	GameState.set_state(GameState.State.PLAY)
	var player := get_tree().get_first_node_in_group("player")
	var cam: Camera3D = player.get_node("Camera3D")
	cam.current = true

	var phone := get_tree().get_first_node_in_group("phone")
	phone.set_objective("find the white house")
	phone.show_message("TRUST ME", 6.0)
	print("[PhoneTest] start: battery=%.1f light_on=%s" % [phone.battery, phone.light_on])

	if "selftest" in OS.get_cmdline_user_args():
		_run_selftest(phone)

func _run_selftest(phone) -> void:
	var b0: float = phone.battery
	await get_tree().create_timer(2.0).timeout
	print("[PhoneTest] light ON  2s: %.3f -> %.3f (drained %.3f, ~0.087/s expected)"
			% [b0, phone.battery, b0 - phone.battery])

	phone.toggle_light()
	print("[PhoneTest] toggled light -> light_on=%s" % phone.light_on)
	var b1: float = phone.battery
	await get_tree().create_timer(2.0).timeout
	print("[PhoneTest] light OFF 2s: drained %.3f (~0.025/s expected)" % (b1 - phone.battery))

	Director.stress = 80.0
	phone.toggle_light()  # back on for the screenshot
	await get_tree().create_timer(1.0).timeout
	print("[PhoneTest] stress=80 -> screen corruption active, battery=%.2f" % phone.battery)

	if DisplayServer.get_name() != "headless":
		await RenderingServer.frame_post_draw
		await RenderingServer.frame_post_draw
		get_viewport().get_texture().get_image().save_png("res://session3_proof.png")
		print("[PhoneTest] screenshot saved -> session3_proof.png")

	# battery_empty signal test
	var fired := [false]
	phone.battery_empty.connect(func(): fired[0] = true; print("[PhoneTest] >>> battery_empty SIGNAL fired"))
	phone.battery = 0.2
	phone.light_on = true
	await get_tree().create_timer(2.5).timeout
	print("[PhoneTest] final: battery=%.2f light_on=%s empty_fired=%s"
			% [phone.battery, phone.light_on, fired[0]])
	get_tree().quit(0)

func _setup_env() -> void:
	var env := Environment.new()
	env.background_mode = Environment.BG_COLOR
	env.background_color = Color(0.004, 0.004, 0.012)
	env.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	env.ambient_light_color = Color(0.10, 0.12, 0.20)
	env.ambient_light_energy = 0.5
	env.fog_enabled = true
	env.fog_light_color = Color(0.02, 0.02, 0.05)
	env.fog_density = 0.03
	var we := WorldEnvironment.new()
	we.environment = env
	add_child(we)
