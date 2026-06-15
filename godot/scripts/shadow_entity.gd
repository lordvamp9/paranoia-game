# PARANOIA — ShadowEntity: too-tall black humanoid, jerky ~9 Hz movement,
# phases through walls, never enters the basement. Port of src/entity.js with
# the Session 4 FSM/transition spec. 3D breath via the Session 1 AudioSynth.
extends Node3D

enum State { IDLE, ALERTED, PURSUING, HUNTING, RETREATING }

signal entity_near(distance: float)              # within 6 m -> phone light recoil
signal state_changed(id: int, from: int, to: int)
signal caught(id: int)

# --- speeds (entity.js / Session 4) ---
const PATROL_SPEED := 1.6
const ALERT_SPEED := 2.2
const PURSUIT_BASE := 3.4
const HUNT_BASE := 4.6
const AGGR_MAX := 1.8

# --- configured by Director.spawn_entity before _ready ---
var entity_id := 0
var aggression := 1.0
var lethal := true

var state: State = State.IDLE
var state_t := 0.0
var home := Vector3.ZERO
var patrol: Array[Vector3] = []
var patrol_idx := 0
var pause_t := 0.0
var last_seen := Vector3.ZERO
var last_seen_t := -999.0
var lost_sight_t := 0.0
var hunt_role := "chaser"   # chaser | flanker
var recoil_t := 0.0

var _pos := Vector3.ZERO     # continuous logical position
var _vis := Vector3.ZERO     # discrete (9 Hz) visual position
var _jerk_t := 0.0
var _breath_t := 0.0
var _breathing := false

var _player: Node3D
var _limbs := {}

func set_home(h: Vector3) -> void:
	home = h
	_pos = h
	_vis = h

func _ready() -> void:
	add_to_group("shadow_entity")
	_build_mesh()
	if patrol.is_empty():
		# a small idle patrol loop around home
		patrol = [home + Vector3(6, 0, 0), home + Vector3(0, 0, 6),
				home + Vector3(-6, 0, 0), home + Vector3(0, 0, -6)]
	_player = get_tree().get_first_node_in_group("player")
	global_position = _pos

func _build_mesh() -> void:
	var mat := StandardMaterial3D.new()
	mat.albedo_color = Color(0.0, 0.0, 0.0)
	mat.roughness = 1.0
	mat.rim_enabled = true            # faint edge "subsurface" rim
	mat.rim = 0.35
	mat.rim_tint = 0.0
	var g := Node3D.new()
	add_child(g)
	g.scale = Vector3.ONE * 1.05      # ~2.1 m tall
	var box := func(w: float, h: float, d: float, pos: Vector3) -> MeshInstance3D:
		var bm := BoxMesh.new(); bm.size = Vector3(w, h, d)
		var mi := MeshInstance3D.new(); mi.mesh = bm; mi.material_override = mat; mi.position = pos
		g.add_child(mi)
		return mi
	# proportions deliberately wrong: too tall, long arms, small head
	_limbs["head"] = box.call(0.21, 0.27, 0.22, Vector3(0, 1.97, 0))
	box.call(0.46, 0.85, 0.24, Vector3(0, 1.4, 0))               # torso
	_limbs["armL"] = box.call(0.09, 1.05, 0.09, Vector3(-0.31, 1.25, 0))
	_limbs["armR"] = box.call(0.09, 1.1, 0.09, Vector3(0.31, 1.2, 0))
	_limbs["legL"] = box.call(0.13, 1.0, 0.13, Vector3(-0.12, 0.5, 0))
	_limbs["legR"] = box.call(0.13, 1.0, 0.13, Vector3(0.12, 0.5, 0))
	# shadow stain under it
	var stain := CylinderMesh.new(); stain.top_radius = 0.7; stain.bottom_radius = 0.7; stain.height = 0.01
	var smat := StandardMaterial3D.new()
	smat.albedo_color = Color(0, 0, 0, 0.55); smat.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	smat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	var smi := MeshInstance3D.new(); smi.mesh = stain; smi.material_override = smat; smi.position = Vector3(0, 0.02, 0)
	g.add_child(smi)

func _set_state(s: State) -> void:
	if s == state:
		return
	var prev := state
	state = s
	state_t = 0.0
	if s == State.HUNTING:
		aggression = minf(AGGR_MAX, aggression + 0.05)   # +0.05 per hunt event
	state_changed.emit(entity_id, prev, s)

# ----------------------------------------------------------------- senses
func _lit_by_player() -> bool:
	if _player == null:
		return false
	var phone := get_tree().get_first_node_in_group("phone")
	if phone == null or not phone.light_on or phone.battery <= 0.0:
		return false
	var to_me := _pos - _player.global_position
	var dist := to_me.length()
	if dist > 19.0:
		return false
	return _player.forward_dir().dot(to_me.normalized()) > 0.82

func _light_on_within(r: float) -> bool:
	var phone := get_tree().get_first_node_in_group("phone")
	if phone == null or not phone.light_on or phone.battery <= 0.0:
		return false
	return _pos.distance_to(_player.global_position) < r

func _has_los(target: Vector3) -> bool:
	# segment(_pos -> target) blocked by any circle collider (trees/buildings)?
	var a := Vector2(_pos.x, _pos.z)
	var b := Vector2(target.x, target.z)
	for c in World.colliders:
		if c.type != "circle":
			continue
		var ctr := Vector2(c.x, c.z)
		if _seg_point_dist(a, b, ctr) < float(c.r):
			return false
	return true

func _seg_point_dist(a: Vector2, b: Vector2, p: Vector2) -> float:
	var ab := b - a
	var len2 := ab.length_squared()
	if len2 < 1e-6:
		return a.distance_to(p)
	var tt := clampf((p - a).dot(ab) / len2, 0.0, 1.0)
	return (a + ab * tt).distance_to(p)

# ----------------------------------------------------------------- update
func _physics_process(dt: float) -> void:
	if _player == null:
		_player = get_tree().get_first_node_in_group("player")
		return
	state_t += dt
	recoil_t = maxf(0.0, recoil_t - dt)

	var dist := _pos.distance_to(_player.global_position)
	var player_in_basement: bool = _player.ground_y < -1.0
	var lit := _lit_by_player()

	# light recoil signal (Session 4: within 6 m -> phone flicker)
	if dist < 6.0:
		entity_near.emit(dist)
		if lit and recoil_t <= 0.0:
			recoil_t = 2.2
			_set_state(State.RETREATING)

	_sense_transitions(dist, lit, player_in_basement)

	var speed := PATROL_SPEED
	var target = null

	match state:
		State.IDLE:
			if pause_t > 0.0:
				pause_t -= dt
			elif not patrol.is_empty():
				target = patrol[patrol_idx]
				if _pos.distance_to(target) < 1.2:
					patrol_idx = (patrol_idx + 1) % patrol.size()
					if randf() < 0.3:
						pause_t = 1.5 + randf() * 2.5
		State.ALERTED:
			speed = ALERT_SPEED
			target = last_seen
			if state_t > 12.0 or (_pos.distance_to(last_seen) < 1.5 and state_t > 4.0):
				_set_state(State.IDLE)
		State.PURSUING:
			speed = PURSUIT_BASE * aggression
			var lead: Vector3 = (_player.forward_dir() * 1.6) if _player.moving else Vector3.ZERO
			target = _player.global_position + lead
			last_seen = _player.global_position
			if dist > 26.0: lost_sight_t += dt
			else: lost_sight_t = 0.0
			if lost_sight_t > 20.0:
				lost_sight_t = 0.0
				_set_state(State.IDLE)
		State.HUNTING:
			speed = HUNT_BASE * aggression
			target = _hunt_target()
		State.RETREATING:
			speed = ALERT_SPEED * 1.4
			target = home
			if _pos.distance_to(home) < 2.0 and state_t > 3.0:
				if state_t > 6.0: _set_state(State.IDLE)
				else: target = null

	if target != null:
		_move_toward(target, speed, dt)

	# catch
	if lethal and dist < 0.95 and not player_in_basement \
			and (state == State.PURSUING or state == State.HUNTING):
		caught.emit(entity_id)

	_update_breath(dt, dist)
	_render(dt, speed, target != null)

func _sense_transitions(dist: float, lit: bool, player_in_basement: bool) -> void:
	# entities never follow into the basement
	if player_in_basement and (state == State.PURSUING or state == State.HUNTING):
		_set_state(State.RETREATING)
		return
	match state:
		State.IDLE:
			# IDLE -> ALERTED: within 25 m, OR light on within 35 m
			if dist < 25.0 or _light_on_within(35.0):
				last_seen = _player.global_position
				last_seen_t = Director.game_time
				_set_state(State.ALERTED)
		State.ALERTED:
			last_seen = _player.global_position
			# ALERTED -> PURSUING: line of sight confirmed
			if dist < 35.0 and _has_los(_player.global_position):
				last_seen_t = Director.game_time
				_set_state(State.PURSUING)
				_alert_others()
		State.PURSUING, State.HUNTING:
			# PURSUING -> HUNTING when 2+ entities pursue simultaneously
			var pack := _pack()
			if pack.size() >= 2:
				_elect_pack(pack)
			elif state == State.HUNTING:
				_set_state(State.PURSUING)
			# RETREATING when far enough that it loses the player entirely
			if dist > 60.0:
				_set_state(State.RETREATING)
		State.RETREATING:
			# RETREATING -> IDLE: distance > 60 m from player
			if dist > 60.0 and recoil_t <= 0.0:
				_set_state(State.IDLE)

func _pack() -> Array:
	var out: Array = []
	for e in get_tree().get_nodes_in_group("shadow_entity"):
		if e.state == State.PURSUING or e.state == State.HUNTING:
			out.append(e)
	return out

func _elect_pack(pack: Array) -> void:
	# leader = closest to player (chaser); others flank
	var pp := _player.global_position
	pack.sort_custom(func(a, b): return a._pos.distance_to(pp) < b._pos.distance_to(pp))
	for i in pack.size():
		var e = pack[i]
		if e.state != State.HUNTING:
			e._set_state(State.HUNTING)
		e.hunt_role = "chaser" if i == 0 else "flanker"

func _hunt_target() -> Vector3:
	var pp := _player.global_position
	if hunt_role == "chaser":
		return pp
	# flanker: cut off the player's forward escape at ~±45°
	var fwd: Vector3 = _player.forward_dir()
	var side := 1.0 if (entity_id % 2 == 0) else -1.0
	var flank: Vector3 = fwd.rotated(Vector3.UP, deg_to_rad(45.0) * side) * 7.0
	return pp + flank

func _alert_others() -> void:
	for e in get_tree().get_nodes_in_group("shadow_entity"):
		if e == self:
			continue
		if e._pos.distance_to(_pos) < 45.0 and (e.state == State.IDLE or e.state == State.ALERTED):
			e.last_seen = _player.global_position
			e._set_state(State.PURSUING)

func _move_toward(target: Vector3, speed: float, dt: float) -> void:
	var dir := target - _pos
	dir.y = 0.0
	var d := dir.length()
	if d <= 0.05:
		return
	dir = dir / d
	var nx := _pos.x + dir.x * speed * dt
	var nz := _pos.z + dir.z * speed * dt
	# slide around tree/building circle colliders (they phase walls, not trunks)
	for c in World.colliders:
		if c.type != "circle":
			continue
		var ddx: float = nx - c.x
		var ddz: float = nz - c.z
		var d2: float = ddx * ddx + ddz * ddz
		var r: float = float(c.r) + 0.3
		if d2 < r * r and d2 > 1e-6:
			var dd := sqrt(d2)
			nx = c.x + (ddx / dd) * r
			nz = c.z + (ddz / dd) * r
	_pos.x = nx
	_pos.z = nz
	_pos.y = 0.0          # glide on the ground plane (never the basement)
	# face travel direction (yaw only)
	rotation.y = atan2(dir.x, dir.z)

func _update_breath(dt: float, dist: float) -> void:
	var active := state != State.IDLE and state != State.RETREATING
	# audio cap: only the 3 nearest active entities breathe
	if active and _audio_rank() < 3:
		_breath_t -= dt
		if _breath_t <= 0.0:
			_breath_t = 0.9 + randf() * 1.6
			Audio.play_entity_breath(entity_id, dist)
			_breathing = true
	elif _breathing:
		Audio.stop_entity_breath(entity_id)
		_breathing = false

func _audio_rank() -> int:
	var pp := _player.global_position
	var my := _pos.distance_to(pp)
	var rank := 0
	for e in get_tree().get_nodes_in_group("shadow_entity"):
		if e == self:
			continue
		if e.state != State.IDLE and e.state != State.RETREATING:
			if e._pos.distance_to(pp) < my:
				rank += 1
	return rank

func _render(dt: float, speed: float, moving: bool) -> void:
	# jerky discrete updates (~9 Hz) — like missing frames
	_jerk_t -= dt
	if _jerk_t <= 0.0:
		_jerk_t = 0.09 + randf() * 0.04
		_vis = _pos
		if state != State.IDLE:
			_vis.x += (randf() - 0.5) * 0.1
			_vis.z += (randf() - 0.5) * 0.1
		var a := randf() * PI - PI / 2.0
		if moving and speed > 0.0:
			_limbs["armL"].rotation.x = a * 0.7
			_limbs["armR"].rotation.x = -a * 0.8
			_limbs["legL"].rotation.x = -a * 0.5
			_limbs["legR"].rotation.x = a * 0.5
			_limbs["head"].rotation.y = (randf() - 0.5) * 0.5
		else:
			_limbs["head"].rotation.y = sin(Director.game_time * 0.3 + entity_id) * 0.8
	global_position = _vis

func remove() -> void:
	if _breathing:
		Audio.stop_entity_breath(entity_id)
	queue_free()
