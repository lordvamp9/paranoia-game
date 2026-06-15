extends Node
## Director — the orchestrator (Session 6). Port of src/director.js.
## Drives the 3 phases, ~30 flags, scripted events, the stress model,
## checkpoints, and both endings. Autoload singleton.
##
## Phase orchestration only runs after begin_play() (Main calls it) so the
## focused test scenes (Phone/Entity/House) are unaffected by the autoload.

signal phase_changed(phase: int)
signal ending_reached(ending: String)     # "trust" | "refuse"
signal interactable_used(id: String)
signal flag_set(name: String)

const HX := 0.0
const HZ := -110.0

var phase: int = 0
var stress: float = 0.0
var game_time: float = 0.0
var orchestrate := false
var ended := false
var external_credits := false   # when true, the UI (game_root) shows credits

var checkpoint := Vector3(0, 0, 200)
var checkpoint_yaw := 0.0
var current_objective := ""

var _player: Node3D
var _phone: Node
var _house: Node
var _started := false
var _catch_cd := 0.0
var _death_wired := false

# --- the full flag set (Session 1-6). All transitions are logged. ---
var flags := {
	# phase 1
	"path_started": false, "fountain_reached": false, "sign_found": false,
	"cabin_found": false, "cabin_open": false, "charger_found": false,
	"phase1_complete": false,
	# phase 2
	"house_reached": false, "house_scare": false, "house_entered": false,
	"phase2_complete": false,
	# phase 3
	"basement_found": false, "basement_open": false, "basement_scare": false,
	"false_wall_open": false, "old_phone_found": false, "bedroom_open": false,
	"mirror_solved": false, "note_read": false,
	"ending_chosen": false, "ending_trust": false, "ending_refuse": false,
	# items / misc (Sessions 1-5)
	"water_key": false, "kitchen_key": false, "in_house": false,
	"hidden_open": false, "truth": false, "well_battery": false, "samara_done": false,
}

# HUD / FX overlay
var _hud: CanvasLayer
var _sub: Label
var _big: Label
var _prompt: Label
var _fade: ColorRect
var _credits: Control
var _credits_label: Label
var _sub_t := 0.0
var _big_t := 0.0
var _fade_decay := false

const _ENTITY_SCENE := preload("res://scenes/ShadowEntity.tscn")
const MAX_ENTITIES := 3
var _next_entity_id := 0
var _active_entities: Array = []

# ============================================================== lifecycle
func reset() -> void:
	phase = 0
	stress = 0.0
	game_time = 0.0
	ended = false
	_started = false
	for k in flags: flags[k] = false

func begin_play() -> void:
	orchestrate = true
	_build_hud()

func _process(delta: float) -> void:
	game_time += delta
	if not orchestrate or ended:
		return
	_player = _player if _player else get_tree().get_first_node_in_group("player")
	if _player == null:
		return
	if not _started:
		_start()
	_wire_death()
	_catch_cd = maxf(0.0, _catch_cd - delta)

	_update_phase()
	_phase_proximity()
	_stress_model(delta)
	_ending_prompt()
	_tick_hud(delta)
	_broadcast()

func _start() -> void:
	_started = true
	_phone = get_tree().get_first_node_in_group("phone")
	_house = get_tree().get_first_node_in_group("house")
	phase = 1
	phase_changed.emit(1)
	checkpoint = Vector3(0, 0, 200)
	Audio.set_wind(1.0)
	set_flag("path_started")
	set_objective("FOLLOW THE PATH")
	print("[Director] play started — phase 1")

# ============================================================== flags
func set_flag(name: String) -> void:
	if not flags.has(name) or flags[name]:
		return
	flags[name] = true
	print("[Director] FLAG  %-18s (phase %d, t=%.1fs)" % [name, phase, game_time])
	flag_set.emit(name)
	_on_flag(name)

func _on_flag(name: String) -> void:
	match name:
		"phase1_complete":
			_spawn_phase1_entities()
		"house_reached":
			set_objective("ENTER THE HOUSE")
		"house_entered":
			set_objective("FIND A WAY DOWN")
			_house_scare()
			_set_checkpoint(Vector3(HX, 0, HZ + 4.5), 0.0)
		"basement_found":
			set_objective("SOMETHING IS HERE")
			_basement_scare()
			_set_checkpoint(Vector3(3, -2.5, -113), 0.0)
		"bedroom_open":
			set_objective("THE MIRROR REMEMBERS")
		"mirror_solved":
			_mirror_event()
		"basement_open":
			set_objective("GO DOWN")
		"false_wall_open":
			set_flag("hidden_open")
			_set_checkpoint(Vector3(-6, -2.5, -110.5), 0.0)
		"old_phone_found":
			set_flag("truth")
			set_objective("READ THE NOTE")
		"note_read":
			_note_read_event()

func set_objective(text: String) -> void:
	current_objective = text
	if _phone == null:
		_phone = get_tree().get_first_node_in_group("phone")
	if _phone:
		_phone.set_objective(text)
	print("[Director] OBJECTIVE  \"%s\"" % text)

# ============================================================== phases
func _update_phase() -> void:
	var z := _player.global_position.z
	if phase == 1 and z < 60.0:
		_enter_phase(2)
	elif phase == 2 and z < -85.0:
		_enter_phase(3)

func _enter_phase(p: int) -> void:
	phase = p
	phase_changed.emit(p)
	print("[Director] === PHASE %d ===" % p)
	if p == 2:
		set_flag("phase1_complete")
		set_objective("REACH THE WHITE HOUSE")
		_set_checkpoint(Vector3(World.path_x(58), 0, 58), 0.0)
	elif p == 3:
		set_flag("phase2_complete")
		set_flag("house_reached")
		_set_checkpoint(Vector3(0, 0, -88), 0.0)

func _phase_proximity() -> void:
	var p := _player.global_position
	if phase >= 2:
		if not flags["fountain_reached"] and p.distance_to(World.FOUNTAIN) < 8.0:
			set_flag("fountain_reached")
		if not flags["sign_found"] and Vector2(p.x - World.SIGN.x, p.z - World.SIGN.z).length() < 6.0:
			set_flag("sign_found")
		if not flags["cabin_found"] and Vector2(p.x - World.CABIN.x, p.z - World.CABIN.z).length() < 8.0:
			set_flag("cabin_found")
	if phase >= 3:
		var in_footprint: bool = absf(p.x - HX) < 6.0 and absf(p.z - HZ) < 5.0
		if in_footprint and not flags["house_entered"]:
			set_flag("house_entered")
		if _player.ground_y < -1.0 and not flags["basement_found"]:
			set_flag("basement_found")

# ============================================================== scripted events
func _spawn_phase1_entities() -> void:
	# On phase1Complete: spawn 2 entities at z=80, aggression 1.0
	spawn_entity(Vector3(World.path_x(80) - 6, 0, 80), 1.0)
	spawn_entity(Vector3(World.path_x(80) + 6, 0, 80), 1.0)
	print("[Director] EVENT  spawned 2 entities @ z=80 (aggr 1.0)")

func _house_scare() -> void:
	print("[Director] EVENT  house scare — flash + whisper")
	PostFX.flash_black()
	Audio.whisper()
	stress = minf(100.0, stress + 12.0)
	set_flag("house_scare")

func _basement_scare() -> void:
	# Spawn an entity INSIDE the house (exception); it retreats after 4 s.
	print("[Director] EVENT  basement scare — entity inside, retreats in 4s")
	set_flag("basement_scare")
	PostFX.spike_glitch(0.7)
	Audio.sting()
	stress = minf(100.0, stress + 18.0)
	var e := spawn_entity(Vector3(HX + 2, -2.5, HZ - 3), 1.2)
	if e:
		e.state = e.State.PURSUING
		var t := get_tree().create_timer(4.0)
		t.timeout.connect(func():
			if is_instance_valid(e):
				print("[Director] EVENT  basement-scare entity retreats")
				e.remove()
		)

func _mirror_event() -> void:
	# stress -> 85, glitch spike, WATCHING 6s, then show mirror figure 0.5s
	print("[Director] EVENT  mirror — stress spike + figure")
	stress = 85.0
	PostFX.spike_glitch(1.0)
	big("WATCHING", 6.0)
	if _phone: _phone.show_message("WATCHING", 6.0)
	Audio.sting()
	if _house and _house.mirror_figure_mat:
		_house.mirror_figure_mat.albedo_color = Color(0, 0, 0, 0.8)
		var t := get_tree().create_timer(0.5)
		t.timeout.connect(func():
			if _house and _house.mirror_figure_mat:
				_house.mirror_figure_mat.albedo_color = Color(0, 0, 0, 0)
		)
	set_objective("GO DOWN")

func _note_read_event() -> void:
	# all entities despawn, silence, DECIDE
	print("[Director] EVENT  note read — entities despawn, silence")
	for e in get_tree().get_nodes_in_group("shadow_entity"):
		if is_instance_valid(e):
			e.remove()
	Audio.set_wind(0.0)
	set_objective("DECIDE")
	sub("[E] TRUST THE PHONE        [Q] REFUSE", 99999.0)

# ============================================================== stress
func _stress_model(dt: float) -> void:
	var nearest := INF
	for e in get_tree().get_nodes_in_group("shadow_entity"):
		nearest = minf(nearest, _player.global_position.distance_to(e.global_position))
	# increases
	if nearest < 6.0:
		stress += 20.0 * dt
	elif nearest < 15.0:
		stress += 8.0 * dt
	else:
		stress -= 5.0 * dt                       # no entity nearby
	if _phone and not _phone.light_on:
		stress += 3.0 * dt                       # dark area without flashlight
	# inside house safe zone (ground floor footprint, not basement)
	var p := _player.global_position
	var in_house_safe: bool = absf(p.x - HX) < 6.0 and absf(p.z - HZ) < 5.0 and _player.ground_y > -1.0
	if in_house_safe:
		stress -= 2.0 * dt
	if _player.ground_y < -1.0:
		stress += 0.8 * dt                       # the basement presses on you
	stress = clampf(stress, 0.0, 100.0)

func _broadcast() -> void:
	if _phone: _phone.set_stress(stress)
	Audio.set_stress(stress)
	# Player reads Director.stress directly; PostFX is a later session.

# ============================================================== checkpoints / death
func _set_checkpoint(pos: Vector3, yaw: float) -> void:
	checkpoint = pos
	checkpoint_yaw = yaw
	print("[Director] CHECKPOINT  %v" % pos)

func _wire_death() -> void:
	if _death_wired:
		return
	if _phone and not _phone.battery_empty.is_connected(_on_battery_empty):
		_phone.battery_empty.connect(_on_battery_empty)
		_death_wired = true

func _on_battery_empty() -> void:
	_death("THE SCREEN DIED")

func _on_entity_caught(_id: int) -> void:
	_death("IT TOUCHED YOU")

func _death(reason: String) -> void:
	if _catch_cd > 0.0 or ended:
		return
	_catch_cd = 3.0
	print("[Director] DEATH (%s) -> respawn @ %v, stress->20" % [reason, checkpoint])
	PostFX.flash_black()
	Audio.scare()
	big(reason, 2.2)
	# respawn
	_player.global_position = checkpoint
	_player.ground_y = checkpoint.y
	_player.yaw = checkpoint_yaw
	stress = 20.0
	if _phone:
		_phone.battery = maxf(5.0, _phone.battery)
	# reset entities
	for e in get_tree().get_nodes_in_group("shadow_entity"):
		if is_instance_valid(e):
			e.set_home(e.home)
			e.state = e.State.IDLE

# ============================================================== endings
func _ending_prompt() -> void:
	if not flags["note_read"] or ended:
		return
	var p := _player.global_position
	var d_phone := Vector2(p.x - (HX - 6.0), p.z - (HZ - 0.5)).length()
	var d_exit := Vector2(p.x - (HX - 4.0), p.z - (HZ - 0.4)).length()
	if d_phone < 1.8:
		_prompt.text = "[E] TRUST"
	elif d_exit < 1.8:
		_prompt.text = "[E] REFUSE"
	else:
		_prompt.text = ""

func _unhandled_input(event: InputEvent) -> void:
	if not orchestrate or ended or not flags["note_read"]:
		return
	if _player == null:
		return
	var p := _player.global_position
	var d_phone := Vector2(p.x - (HX - 6.0), p.z - (HZ - 0.5)).length()
	var d_exit := Vector2(p.x - (HX - 4.0), p.z - (HZ - 0.4)).length()
	if event.is_action_pressed("interact") and d_phone < 1.8:
		_ending("trust")
	elif (event is InputEventKey and event.pressed and event.keycode == KEY_Q) and d_exit < 1.8:
		_ending("refuse")

func _ending(which: String) -> void:
	if ended:
		return
	ended = true
	set_flag("ending_chosen")
	_prompt.text = ""
	_sub.text = ""
	if _player: _player.enabled = false
	print("[Director] ===== ENDING: %s =====" % which.to_upper())
	if which == "trust":
		set_flag("ending_trust")
		if _phone: _phone.show_message("TRUST ME", 30.0)
		Audio.set_wind(0.0)
		PostFX.flash_white()
		flash_hold(Color(1, 1, 1), 1.0)         # white flash, hold
		if not external_credits:
			var t := get_tree().create_timer(2.5)
			t.timeout.connect(func(): _show_credits("TRUST", "You stayed."))
	else:
		set_flag("ending_refuse")
		if _phone:
			_phone.battery = 0.0
			_phone.light_on = false
		Audio.scare()
		Audio.sting()
		Audio.set_wind(0.0)
		PostFX.flash_black()
		flash_hold(Color(0, 0, 0), 1.0)         # black flash, hold
		big("THEY SEE YOU NOW", 4.0)
		if not external_credits:
			var t := get_tree().create_timer(2.5)
			t.timeout.connect(func(): _show_credits("REFUSE", "You ran."))
	ending_reached.emit(which)

# ============================================================== interactables (Session 5)
func use_interactable(id: String) -> void:
	match id:
		"kitchen_drawer":
			if not flags["water_key"]:
				set_flag("water_key")
				_msg("WATER KEY ACQUIRED"); Audio.ui_beep()
				set_objective("THE BEDROOM UPSTAIRS")
		"bedroom_door":
			if flags["water_key"]:
				if not flags["bedroom_open"]:
					set_flag("bedroom_open")
					if _house: _house.open_bedroom_door()
					_msg("BEDROOM UNLOCKED"); Audio.ui_beep()
			else:
				_msg("LOCKED — A KEY IS NEEDED")
		"cabin_keypad":
			set_flag("cabin_open")
			_open_cabin(); _msg("ACCESS GRANTED")
		"basement_door":
			if not flags["basement_open"]:
				set_flag("basement_open")
				if _house: _house.open_basement_door()
				_msg("BASEMENT OPEN"); Audio.ui_beep()
		"false_wall":
			if flags["basement_scare"]:
				set_flag("false_wall_open")
				if _house: _house.open_false_wall()
				_msg("THE WALL GIVES WAY")
			else:
				_msg("IT WON'T MOVE — NOT YET")
		"old_phone":
			set_flag("old_phone_found")
			stress = minf(100.0, stress + 20.0)
			_msg("YOU ARE THE RECORDING", 6.0); Audio.sting()
		"note":
			set_flag("note_read")
		"mirror":
			set_flag("mirror_solved")
	interactable_used.emit(id)

func _msg(text: String, secs := 4.0) -> void:
	if _phone == null:
		_phone = get_tree().get_first_node_in_group("phone")
	if _phone:
		_phone.show_message(text, secs)

func _open_cabin() -> void:
	if World.cabin_door:
		World.cabin_door.position.z -= 0.95
		World.cabin_door.rotation.y = 0.9
	if World.cabin_door_collider:
		var c = World.cabin_door_collider
		c["minX"] = 99999.0; c["maxX"] = 99999.0; c["minZ"] = 99999.0; c["maxZ"] = 99999.0

# ============================================================== entity spawning (Session 4)
func spawn_entity(pos: Vector3, p_aggression: float = 1.0) -> Node3D:
	# Performance cap: at most MAX_ENTITIES (3) active at once — retire the
	# oldest still-living one before adding a new entity.
	_active_entities = _active_entities.filter(func(x): return is_instance_valid(x))
	while _active_entities.size() >= MAX_ENTITIES:
		var old = _active_entities.pop_front()
		if is_instance_valid(old):
			old.remove()
	var e := _ENTITY_SCENE.instantiate()
	e.entity_id = _next_entity_id % 6
	e.aggression = p_aggression
	e.set_home(pos)
	_next_entity_id += 1
	var parent := get_tree().current_scene
	if parent:
		parent.add_child(e)
	e.global_position = pos
	_active_entities.append(e)
	var phone := get_tree().get_first_node_in_group("phone")
	if phone and not e.entity_near.is_connected(phone.on_entity_near):
		e.entity_near.connect(phone.on_entity_near)
	if not e.caught.is_connected(_on_entity_caught):
		e.caught.connect(_on_entity_caught)
	return e

# ============================================================== HUD / FX
func _build_hud() -> void:
	if _hud:
		return
	_hud = CanvasLayer.new()
	_hud.layer = 10
	add_child(_hud)

	_sub = _mk_label(20, Color(0.6, 0.6, 0.64), HORIZONTAL_ALIGNMENT_CENTER)
	_sub.set_anchors_preset(Control.PRESET_BOTTOM_WIDE)
	_sub.position.y = -90
	_hud.add_child(_sub)

	_prompt = _mk_label(22, Color(0.81, 0.81, 0.84), HORIZONTAL_ALIGNMENT_CENTER)
	_prompt.set_anchors_preset(Control.PRESET_CENTER_BOTTOM)
	_prompt.position = Vector2(-200, -160); _prompt.size = Vector2(400, 30)
	_hud.add_child(_prompt)

	_big = _mk_label(46, Color(0.85, 0.83, 0.81), HORIZONTAL_ALIGNMENT_CENTER)
	_big.set_anchors_preset(Control.PRESET_CENTER)
	_big.position = Vector2(-400, -40); _big.size = Vector2(800, 80)
	_big.modulate.a = 0.0
	_hud.add_child(_big)

	_fade = ColorRect.new()
	_fade.color = Color(0, 0, 0, 0)
	_fade.set_anchors_preset(Control.PRESET_FULL_RECT)
	_fade.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_hud.add_child(_fade)

	_credits = Control.new()
	_credits.set_anchors_preset(Control.PRESET_FULL_RECT)
	_credits.visible = false
	var bg := ColorRect.new(); bg.color = Color.BLACK
	bg.set_anchors_preset(Control.PRESET_FULL_RECT)
	_credits.add_child(bg)
	_credits_label = _mk_label(18, Color(0.7, 0.7, 0.7), HORIZONTAL_ALIGNMENT_CENTER)
	_credits_label.set_anchors_preset(Control.PRESET_CENTER)
	_credits_label.position = Vector2(-400, 0); _credits_label.size = Vector2(800, 400)
	_credits.add_child(_credits_label)
	_hud.add_child(_credits)

func _mk_label(fs: int, col: Color, align: int) -> Label:
	var l := Label.new()
	l.add_theme_font_size_override("font_size", fs)
	l.add_theme_color_override("font_color", col)
	l.add_theme_color_override("font_outline_color", Color.BLACK)
	l.add_theme_constant_override("outline_size", 6)
	l.horizontal_alignment = align
	l.mouse_filter = Control.MOUSE_FILTER_IGNORE
	return l

func sub(text: String, seconds := 4.0) -> void:
	if _sub: _sub.text = text
	_sub_t = seconds

func big(text: String, seconds := 3.0) -> void:
	if _big:
		_big.text = text
		_big.modulate.a = 1.0
	_big_t = seconds

func flash(color: Color, alpha: float) -> void:
	if _fade:
		_fade.color = Color(color.r, color.g, color.b, alpha)
		_fade_decay = true

func flash_hold(color: Color, alpha: float) -> void:
	if _fade:
		_fade.color = Color(color.r, color.g, color.b, alpha)
		_fade_decay = false

func _tick_hud(dt: float) -> void:
	_sub_t -= dt
	if _sub_t <= 0.0 and _sub and not flags["note_read"]:
		_sub.text = ""
	_big_t -= dt
	if _big_t <= 0.0 and _big and _big.modulate.a > 0.0:
		_big.modulate.a = maxf(0.0, _big.modulate.a - dt * 1.2)
	if _fade_decay and _fade and _fade.color.a > 0.0:
		_fade.color.a = maxf(0.0, _fade.color.a - dt * 1.5)

func _show_credits(title: String, blurb: String) -> void:
	print("[Director] CREDITS (%s) — %s" % [title, blurb])
	if _credits == null:
		return
	_credits.visible = true
	_credits_label.text = "PARANOIA\n\nENDING — %s\n\n%s\n\na game by vamp9\n\nprocedural · code · sound\n\n[R] RESTART" % [title, blurb]
	# slow upward scroll
	_credits_label.position.y = 200
	var tw := create_tween()
	tw.tween_property(_credits_label, "position:y", -120, 12.0)
	GameState.set_state(GameState.State.CREDITS)
