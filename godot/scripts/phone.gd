# PARANOIA — the smartphone: flashlight + objective screen + battery + stress.
# Held in front of the camera (lower right), always visible. Port of src/phone.js,
# with the battery tuning overrides from the Session 3 brief (drain reduced so it
# doesn't die in ~2 min). Owns the only real light in the world.
extends Node3D

signal battery_empty

# --- battery tuning (Session 3 brief — NOT the raw phone.js numbers) ---
const DRAIN_BASE := 0.025          # per second, idle
const DRAIN_LIGHT := 0.062         # per second, flashlight ON
const STRESS_DIV := 150.0          # mult = 1 + stress/150  (JS used /100 — too fast)
const ENTITY_PER := 0.06           # mult = 1 + 0.06 * nearby (JS 0.15 — too fast)

# --- flashlight (Session 3 brief) ---
const LIGHT_ANGLE := 28.0
const LIGHT_RANGE := 18.0
const LIGHT_ENERGY := 2.2

const BASE_POS := Vector3(0.23, -0.26, -0.55)
const BASE_ROT := Vector3(-0.12, -0.06, 0.03)

var battery := 100.0
var light_on := true
var stress := 0.0
var objective := ""

var _message := ""
var _message_t := 0.0
var _bob := 0.0
var _ui_clock := 0.0
var _flicker_t := 0.0
var _flicker_off := false
var _recoil_t := 0.0
var _empty_fired := false
var _rec_time := 0.0

var _viewport: SubViewport
var _screen_ui: Control
var _spot: SpotLight3D
var _fill: OmniLight3D
var _rng := RandomNumberGenerator.new()
var _player: Node3D

func _ready() -> void:
	add_to_group("phone")
	_rng.randomize()
	position = BASE_POS
	rotation = BASE_ROT
	_build_phone()
	_build_hand()
	_build_light()
	_player = get_tree().get_first_node_in_group("player")

# ----------------------------------------------------------------- build
func _build_phone() -> void:
	var body := BoxMesh.new()
	body.size = Vector3(0.165, 0.34, 0.018)
	var bmat := StandardMaterial3D.new()
	bmat.albedo_color = Color8(0x16, 0x16, 0x1a)
	bmat.roughness = 0.35
	bmat.metallic = 0.7
	var bmi := MeshInstance3D.new()
	bmi.mesh = body
	bmi.material_override = bmat
	add_child(bmi)

	# live screen via SubViewport
	_viewport = SubViewport.new()
	_viewport.size = Vector2i(320, 640)
	_viewport.transparent_bg = false
	_viewport.disable_3d = true
	_viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	add_child(_viewport)
	_screen_ui = preload("res://scripts/phone_screen.gd").new()
	_viewport.add_child(_screen_ui)

	var screen := QuadMesh.new()
	screen.size = Vector2(0.15, 0.325)
	var smat := StandardMaterial3D.new()
	smat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	smat.albedo_texture = _viewport.get_texture()
	smat.texture_filter = BaseMaterial3D.TEXTURE_FILTER_NEAREST
	var smi := MeshInstance3D.new()
	smi.mesh = screen
	smi.material_override = smat
	smi.position = Vector3(0, 0, 0.0101)
	add_child(smi)

	# rear camera bump — it faces the world: it IS recording
	var bump := CylinderMesh.new()
	bump.top_radius = 0.012; bump.bottom_radius = 0.012; bump.height = 0.006; bump.radial_segments = 10
	var cmat := StandardMaterial3D.new()
	cmat.albedo_color = Color8(0x06, 0x07, 0x0a); cmat.roughness = 0.1; cmat.metallic = 0.8
	var cmi := MeshInstance3D.new()
	cmi.mesh = bump; cmi.material_override = cmat
	cmi.rotation = Vector3(PI / 2, 0, 0); cmi.position = Vector3(0.05, 0.13, -0.012)
	add_child(cmi)

func _build_hand() -> void:
	var skin := StandardMaterial3D.new()
	skin.albedo_color = Color8(0x8a, 0x6a, 0x52)
	skin.roughness = 0.75
	var add_part := func(mesh: Mesh, pos: Vector3, rot: Vector3) -> void:
		var mi := MeshInstance3D.new()
		mi.mesh = mesh; mi.material_override = skin
		mi.position = pos; mi.rotation = rot
		add_child(mi)
	var palm := BoxMesh.new(); palm.size = Vector3(0.07, 0.14, 0.055)
	add_part.call(palm, Vector3(0.085, -0.1, -0.012), Vector3(0, 0, 0.18))
	var thumb := CapsuleMesh.new(); thumb.radius = 0.014; thumb.height = 0.098
	add_part.call(thumb, Vector3(0.045, -0.11, 0.02), Vector3(0, 0, 1.15))
	for i in 4:
		var f := CapsuleMesh.new(); f.radius = 0.0125; f.height = 0.085
		add_part.call(f, Vector3(0.07 - i * 0.001, -0.045 - i * 0.038, -0.028), Vector3(0, 0.15, PI / 2 - 0.25))
	var wrist := CapsuleMesh.new(); wrist.radius = 0.034; wrist.height = 0.228
	add_part.call(wrist, Vector3(0.13, -0.26, -0.04), Vector3(-0.3, 0, 0.45))

func _build_light() -> void:
	var cam := get_parent()   # phone sits under the camera
	_spot = SpotLight3D.new()
	_spot.light_color = Color(0.749, 0.824, 0.91)
	_spot.light_energy = LIGHT_ENERGY
	_spot.spot_range = LIGHT_RANGE
	_spot.spot_angle = LIGHT_ANGLE
	_spot.spot_angle_attenuation = 0.9
	_spot.shadow_enabled = true
	_spot.position = Vector3(0.2, -0.2, -0.3)
	if cam: cam.add_child.call_deferred(_spot)
	# small fill so the hand/phone stay readable
	_fill = OmniLight3D.new()
	_fill.light_color = Color(0.541, 0.588, 0.722)
	_fill.light_energy = 0.18
	_fill.omni_range = 1.2
	_fill.position = Vector3(0.15, -0.2, -0.4)
	if cam: cam.add_child.call_deferred(_fill)

# ----------------------------------------------------------------- input/api
func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("light"):
		toggle_light()

func toggle_light() -> bool:
	if battery > 0.0:
		light_on = not light_on
	return light_on

func set_objective(text: String) -> void:
	objective = text

func set_stress(value: float) -> void:
	stress = value

func show_message(text: String, seconds := 4.0) -> void:
	_message = text
	_message_t = seconds

func recharge(amount := 100.0) -> void:
	battery = minf(100.0, battery + amount)
	if amount >= 50.0:
		light_on = true
		_empty_fired = false

# Entity light-recoil: called via the entity_near signal (and proximity fallback).
func on_entity_near(_distance: float) -> void:
	_recoil_t = 0.18

# ----------------------------------------------------------------- update
func _process(delta: float) -> void:
	_rec_time += delta
	_message_t = maxf(0.0, _message_t - delta)
	if _player == null:
		_player = get_tree().get_first_node_in_group("player")

	stress = Director.stress
	var near_count := _count_entities_near()
	_drain_tick(delta, near_count)
	_update_light(delta, near_count)
	_update_bob(delta)
	_update_screen(delta)

func _drain_tick(dt: float, near_count: int) -> void:
	var rate := DRAIN_BASE
	if light_on:
		rate += DRAIN_LIGHT
	rate *= 1.0 + stress / STRESS_DIV
	rate *= 1.0 + ENTITY_PER * near_count
	battery = maxf(0.0, battery - rate * dt)
	if battery <= 0.0:
		light_on = false
		if not _empty_fired:
			_empty_fired = true
			battery_empty.emit()

func _update_light(dt: float, near_count: int) -> void:
	# recoil flicker when an entity is within 6 m (random 0.3..2.2 at ~8 Hz)
	if near_count > 0 and _has_entity_within(6.0):
		_recoil_t = 0.18
	_recoil_t = maxf(0.0, _recoil_t - dt)

	_flicker_t -= dt
	if _flicker_t <= 0.0:
		var chance := 0.5 if battery < 20.0 else (0.12 if battery < 50.0 else 0.03)
		var bump := 0.15 if stress > 60.0 else 0.0
		_flicker_off = _rng.randf() < chance + bump
		_flicker_t = (0.05 + _rng.randf() * 0.12) if _flicker_off else (0.4 + _rng.randf() * 1.6)

	var bat_factor := 0.0 if battery <= 0.0 else 0.45 + 0.55 * minf(1.0, battery / 50.0)
	var on := light_on and not _flicker_off and battery > 0.0
	var target := 0.0
	if on:
		target = LIGHT_ENERGY * bat_factor
		if _recoil_t > 0.0:
			target = lerpf(0.3, LIGHT_ENERGY, 0.5 + 0.5 * sin(_rec_time * 50.0)) * bat_factor
	if _spot:
		_spot.light_energy = lerpf(_spot.light_energy, target, minf(1.0, dt * 22.0))
		_spot.visible = target > 0.001
	if _fill:
		_fill.light_energy = 0.18 if on else 0.04

func _update_bob(dt: float) -> void:
	var moving := false
	var sprinting := false
	if _player:
		moving = _player.moving
		sprinting = _player.sprinting
	_bob += dt * (11.0 if sprinting else 7.0) * (1.0 if moving else 0.25)
	var amt := (0.009 if sprinting else 0.005) if moving else 0.0018
	var tremor := (stress / 100.0) * 0.004
	position.x = BASE_POS.x + sin(_bob * 0.5) * amt + (_rng.randf() - 0.5) * tremor
	position.y = BASE_POS.y + absf(sin(_bob)) * amt + (_rng.randf() - 0.5) * tremor
	rotation.z = BASE_ROT.z + sin(_bob * 0.5) * 0.01 + (_rng.randf() - 0.5) * tremor

func _update_screen(dt: float) -> void:
	_ui_clock -= dt
	if _ui_clock <= 0.0:
		_ui_clock = 1.0 / 12.0
		if _screen_ui:
			_screen_ui.battery = battery
			_screen_ui.stress = stress
			_screen_ui.objective = objective
			_screen_ui.message = _message
			_screen_ui.message_t = _message_t
			_screen_ui.t = _rec_time
			_screen_ui.rec_time = _rec_time
			_screen_ui.queue_redraw()

# ----------------------------------------------------------------- helpers
func _count_entities_near(radius := 14.0) -> int:
	if _player == null:
		return 0
	var n := 0
	for e in get_tree().get_nodes_in_group("shadow_entity"):
		if _player.global_position.distance_to(e.global_position) < radius:
			n += 1
	return n

func _has_entity_within(radius: float) -> bool:
	if _player == null:
		return false
	for e in get_tree().get_nodes_in_group("shadow_entity"):
		if _player.global_position.distance_to(e.global_position) < radius:
			return true
	return false
