# PARANOIA — Session 4 test: 3 ShadowEntities spawned near the player; logs FSM
# transitions and confirms 3D breath audio plays. Player stays still so the pack
# escalates IDLE -> ALERTED -> PURSUING -> HUNTING and closes in.
extends Node3D

const STATE_NAMES := ["IDLE", "ALERTED", "PURSUING", "HUNTING", "RETREATING"]
var _caught_logged := false

func _ready() -> void:
	_setup_env()
	GameState.set_state(GameState.State.PLAY)
	var player := get_tree().get_first_node_in_group("player")
	var cam: Camera3D = player.get_node("Camera3D")
	cam.current = true

	# spawn near the path so line-of-sight to the player is clear
	var zs := [186.0, 181.0, 177.0]
	var aggrs := [1.0, 1.2, 1.5]
	for i in 3:
		var z: float = zs[i]
		var pos := Vector3(World.path_x(z), 0.0, z)
		var e := Director.spawn_entity(pos, aggrs[i])
		e.state_changed.connect(_on_state_changed)
		e.caught.connect(_on_caught)
		print("[EntityTest] spawned entity id=%d aggr=%.1f at %v (dist=%.1f)"
				% [e.entity_id, aggrs[i], pos, pos.distance_to(player.global_position)])

	if "selftest" in OS.get_cmdline_user_args():
		_run_selftest()

func _on_state_changed(id: int, from: int, to: int) -> void:
	print("[EntityTest]   entity %d: %s -> %s" % [id, STATE_NAMES[from], STATE_NAMES[to]])

func _on_caught(id: int) -> void:
	if not _caught_logged:
		_caught_logged = true
		print("[EntityTest]   >>> player CAUGHT by entity %d" % id)

func _run_selftest() -> void:
	await get_tree().create_timer(4.0).timeout
	var player := get_tree().get_first_node_in_group("player")
	print("[EntityTest] --- after 4s ---")
	for e in get_tree().get_nodes_in_group("shadow_entity"):
		print("[EntityTest] entity %d: state=%s aggr=%.2f dist=%.1f breathing=%s"
				% [e.entity_id, STATE_NAMES[e.state], e.aggression,
				e._pos.distance_to(player.global_position), str(e._breathing)])
	var synth_ok: bool = Audio.synth != null
	print("[EntityTest] AudioSynth available=%s" % str(synth_ok))

	if DisplayServer.get_name() != "headless":
		await RenderingServer.frame_post_draw
		await RenderingServer.frame_post_draw
		get_viewport().get_texture().get_image().save_png("res://session4_proof.png")
		print("[EntityTest] screenshot saved -> session4_proof.png")
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
