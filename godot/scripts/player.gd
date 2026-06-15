# PARANOIA — first-person player controller.
# Faithful port of src/player.js: pointer-lock look, WASD, sprint, no jump,
# analytic collision against World.colliders, and multi-floor ground resolution
# by zone priority (ramps > floors > default terrain). The node is a
# CharacterBody3D (CapsuleShape3D for future physics queries / entity raycasts)
# but horizontal movement uses the reference's analytic collide so behavior
# matches the JS source exactly.
extends CharacterBody3D

const EYE := 1.62          # camera height above feet (player.js EYE)
const RADIUS := 0.32       # collision radius (player.js RADIUS)
const WALK := 3.1          # m/s  (player.js speed)
const SPRINT := 5.4        # m/s  (player.js sprintSpeed)
const PITCH_LIMIT := 1.45  # radians (player.js clamp)

signal interacted          # E key — Director handles the response
signal stepped(sprinting: bool)

@export var sensitivity_scale := 0.0042  # * Config.mouse_sens -> rad/pixel (~0.0021 at 0.5)

var yaw := 0.0            # facing -Z (down the path) at spawn
var pitch := 0.0
var ground_y := 0.0
var sprinting := false
var moving := false
var step_t := 0.0
var enabled := true

@onready var _cam: Camera3D = $Camera3D
# Flashlight ownership moved to the Phone (Session 3) — it IS the torch.

func _ready() -> void:
	add_to_group("player")
	# spawn at the path mouth (player.js: pos = (0,0,200))
	global_position = Vector3(0.0, 0.0, 200.0)
	ground_y = _resolve_ground(0.0, 200.0)
	global_position.y = ground_y
	_apply_camera()

func _unhandled_input(event: InputEvent) -> void:
	if not enabled:
		return
	if event is InputEventMouseMotion and Input.mouse_mode == Input.MOUSE_MODE_CAPTURED:
		var sens := sensitivity_scale * Config.mouse_sens
		yaw -= event.relative.x * sens
		pitch -= event.relative.y * sens
		pitch = clampf(pitch, -PITCH_LIMIT, PITCH_LIMIT)
	elif event is InputEventMouseButton and event.pressed:
		# click to capture the pointer (player.js requestPointerLock on click)
		Input.mouse_mode = Input.MOUSE_MODE_CAPTURED
	elif event.is_action_pressed("interact"):
		interacted.emit()
	elif event.is_action_pressed("pause"):
		Input.mouse_mode = Input.MOUSE_MODE_VISIBLE

func _physics_process(delta: float) -> void:
	if not enabled:
		_apply_camera()
		return

	var fwd := 0.0
	var strafe := 0.0
	if Input.is_action_pressed("move_forward"): fwd += 1.0
	if Input.is_action_pressed("move_back"):    fwd -= 1.0
	if Input.is_action_pressed("move_left"):    strafe -= 1.0
	if Input.is_action_pressed("move_right"):   strafe += 1.0

	sprinting = Input.is_action_pressed("sprint") and fwd > 0.0
	moving = fwd != 0.0 or strafe != 0.0

	var stress: float = Director.stress
	if moving:
		var inv := 1.0 / sqrt(fwd * fwd + strafe * strafe)
		fwd *= inv; strafe *= inv
		var sp := SPRINT if sprinting else WALK
		# panic above stress 80 slows you (player.js: sp *= 0.82)
		if sprinting and stress >= 80.0:
			sp = SPRINT * 0.82
		elif stress > 80.0:
			sp *= 0.82
		var s := sin(yaw)
		var c := cos(yaw)
		var dx := (s * -fwd + c * strafe) * sp * delta
		var dz := (c * -fwd - s * strafe) * sp * delta
		var nx := global_position.x + dx
		var nz := global_position.z + dz
		var r := _collide(nx, nz)       # two passes for corners (player.js)
		r = _collide(r.x, r.y)
		global_position.x = r.x
		global_position.z = r.y

		step_t -= delta * (1.7 if sprinting else 1.0)
		if step_t <= 0.0:
			step_t = 0.52
			stepped.emit(sprinting)

	# multi-floor ground resolution, smoothed (player.js lerp 14)
	var target := _resolve_ground(global_position.x, global_position.z)
	ground_y += (target - ground_y) * minf(1.0, delta * 14.0)
	global_position.y = ground_y

	_apply_camera()

func _apply_camera() -> void:
	_cam.global_position = global_position + Vector3(0, EYE, 0)
	_cam.rotation = Vector3(pitch, yaw, 0)  # YXZ via Vector3(x=pitch,y=yaw)

func forward_dir() -> Vector3:
	return Vector3(-sin(yaw), 0.0, -cos(yaw))

# --- ground resolution (player.js _resolveGround) -------------------------
func _resolve_ground(x: float, z: float) -> float:
	var best = null
	var best_prio := -INF
	var closest := 0.0
	var closest_d := INF
	for zo in World.ground_zones:
		if not zo.contains(x, z):
			continue
		var h: float = zo.height(x, z)
		var d := absf(h - ground_y)
		if d < closest_d:
			closest_d = d; closest = h
		if d < 0.6 and float(zo.priority) > best_prio:
			best_prio = float(zo.priority); best = h
	return best if best != null else closest

# --- analytic collision (player.js _collide) ------------------------------
func _collide(nx: float, nz: float) -> Vector2:
	var y := ground_y
	for c in World.colliders:
		if c.type == "circle":
			if y > 2.0 or y < -1.0:
				continue  # trees etc. only at ground level
			var ccx: float = c.x
			var ccz: float = c.z
			var dx: float = nx - ccx
			var dz: float = nz - ccz
			var d2: float = dx * dx + dz * dz
			var rr: float = float(c.r) + RADIUS
			if d2 < rr * rr and d2 > 1e-7:
				var d: float = sqrt(d2)
				nx = ccx + (dx / d) * rr
				nz = ccz + (dz / d) * rr
		else:  # box
			var min_x: float = c.minX
			var max_x: float = c.maxX
			var min_z: float = c.minZ
			var max_z: float = c.maxZ
			if c.has("minY") and y < float(c.minY): continue
			if c.has("maxY") and y > float(c.maxY): continue
			if c.type == "box" and (y > 2.0 or y < -1.0) and not c.has("minY") and not c.has("maxY"):
				continue
			var cx: float = clampf(nx, min_x, max_x)
			var cz: float = clampf(nz, min_z, max_z)
			var dx2: float = nx - cx
			var dz2: float = nz - cz
			var d2b: float = dx2 * dx2 + dz2 * dz2
			if d2b < RADIUS * RADIUS:
				if d2b > 1e-7:
					var d: float = sqrt(d2b)
					nx = cx + (dx2 / d) * RADIUS
					nz = cz + (dz2 / d) * RADIUS
				else:
					# inside the box: push out along the smallest axis
					var push_l: float = nx - min_x + RADIUS
					var push_r: float = max_x - nx + RADIUS
					var push_b: float = nz - min_z + RADIUS
					var push_f: float = max_z - nz + RADIUS
					var mn: float = min(push_l, push_r, push_b, push_f)
					if mn == push_l: nx = min_x - RADIUS
					elif mn == push_r: nx = max_x + RADIUS
					elif mn == push_b: nz = min_z - RADIUS
					else: nz = max_z + RADIUS
	return Vector2(nx, nz)
