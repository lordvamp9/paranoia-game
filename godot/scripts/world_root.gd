# PARANOIA — World.tscn root: builds the procedural forest (port of the
# build*() functions in src/world.js) and runs per-frame world updates
# (sky-eye blinking, hidden sign code, rain follows player).
# All geometry is generated in code — no external assets.
extends Node3D

const TREE_TRUNK_TOP := 0.18
const TREE_TRUNK_BOT := 0.32
const TREE_TRUNK_H := 5.4
const TREE_CANOPY_R := 1.7
const TREE_CANOPY_H := 4.4

var _rng := RandomNumberGenerator.new()
var _eyes: Array = []          # {node, blink_t, base_op, closing}
var _sign_code: Label3D = null
var _rain: GPUParticles3D = null

func _ready() -> void:
	_rng.seed = 42                       # deterministic layout (rocks etc. seed 42)
	World.reset()
	_build_ground()
	_build_path()
	_build_trees()
	_build_fences()
	_build_rocks()
	_build_sky_eyes()
	_build_fountain()
	_build_sign()
	_build_cabin()
	# The white house (3 floors + interactables) is built by House.tscn / house.gd
	# (Session 5), instanced separately so it can own its rooms and triggers.
	_build_rain()
	_build_bounds_and_zones()
	print("[World] built: %d colliders, %d ground zones, %d sky eyes"
			% [World.colliders.size(), World.ground_zones.size(), _eyes.size()])

# ---------------------------------------------------------------- helpers
func _mat(color: Color, rough := 0.95, flat := true) -> StandardMaterial3D:
	var m := StandardMaterial3D.new()
	m.albedo_color = color
	m.roughness = rough
	m.metallic = 0.0
	if flat:
		m.shading_mode = BaseMaterial3D.SHADING_MODE_PER_PIXEL
	return m

func _add_mesh(mesh: Mesh, mat: Material, xform := Transform3D.IDENTITY) -> MeshInstance3D:
	var mi := MeshInstance3D.new()
	mi.mesh = mesh
	if mat: mi.material_override = mat
	mi.transform = xform
	add_child(mi)
	return mi

func _make_multimesh(mesh: Mesh, mat: Material, count: int) -> MultiMesh:
	var mm := MultiMesh.new()
	mm.transform_format = MultiMesh.TRANSFORM_3D
	mm.mesh = mesh
	mm.instance_count = count
	var mmi := MultiMeshInstance3D.new()
	mmi.multimesh = mm
	if mat: mmi.material_override = mat
	add_child(mmi)
	return mm

# ---------------------------------------------------------------- ground
func _build_ground() -> void:
	var pm := PlaneMesh.new()
	pm.size = Vector2(700, 700)
	pm.subdivide_width = 16
	pm.subdivide_depth = 16
	# Spec: StandardMaterial3D, color #2d4a1e
	_add_mesh(pm, _mat(Color8(0x2d, 0x4a, 0x1e)))

# ---------------------------------------------------------------- path
func _build_path() -> void:
	# Dirt ribbon along pathX(z), width +/-1.6 (world.js buildPath).
	var pts: Array[Vector3] = []
	var z := 205.0
	while z >= 55.0:
		pts.append(Vector3(World.path_x(z), 0.03, z))
		z -= 2.5
	z = 55.0
	while z >= -88.0:
		pts.append(Vector3(World.path_x(55.0) * (z + 88.0) / 143.0, 0.03, z))
		z -= 4.0

	var st := SurfaceTool.new()
	st.begin(Mesh.PRIMITIVE_TRIANGLES)
	st.set_normal(Vector3.UP)
	var verts: Array[Vector3] = []
	for i in pts.size():
		var p := pts[i]
		var dir: Vector3 = (pts[i + 1] - p) if i < pts.size() - 1 else (p - pts[i - 1])
		var side := Vector3(-dir.z, 0, dir.x).normalized() * 1.6
		verts.append(p - side)
		verts.append(p + side)
	for v in verts:
		st.set_uv(Vector2(v.x * 0.25, v.z * 0.25))
		st.add_vertex(v)
	for i in range(pts.size() - 1):
		var a := i * 2
		st.add_index(a); st.add_index(a + 1); st.add_index(a + 2)
		st.add_index(a + 1); st.add_index(a + 3); st.add_index(a + 2)
	var mesh := st.commit()
	_add_mesh(mesh, _mat(Color8(0x5c, 0x4a, 0x34)))

# ---------------------------------------------------------------- trees
func _build_trees() -> void:
	var positions: Array = []
	# phase-1 flanks (dense corridor)
	for i in 280:
		var z := 55.0 + _rng.randf() * 152.0
		var s := -1.0 if _rng.randf() < 0.5 else 1.0
		var x := World.path_x(z) + s * (5.0 + _rng.randf() * 34.0)
		if not _tree_reject(x, z): positions.append(Vector2(x, z))
	# forest, progressive density toward the house
	for i in 380:
		var z := -135.0 + _rng.randf() * 192.0
		var x := -125.0 + _rng.randf() * 250.0
		if z > 58.0: continue
		var density := 0.55 if z > 20.0 else (0.8 if z > -40.0 else (1.0 if z > -85.0 else 0.35))
		if _rng.randf() > density: continue
		if not _tree_reject(x, z): positions.append(Vector2(x, z))

	var n := positions.size()
	var trunk := CylinderMesh.new()
	trunk.top_radius = TREE_TRUNK_TOP
	trunk.bottom_radius = TREE_TRUNK_BOT
	trunk.height = TREE_TRUNK_H
	trunk.radial_segments = 5
	var canopy := CylinderMesh.new()
	canopy.top_radius = 0.0
	canopy.bottom_radius = TREE_CANOPY_R
	canopy.height = TREE_CANOPY_H
	canopy.radial_segments = 6

	var mm_trunk := _make_multimesh(trunk, _mat(Color8(0x2b, 0x20, 0x18)), n)
	var mm_canopy := _make_multimesh(canopy, _mat(Color8(0x16, 0x24, 0x1a)), n)

	for i in n:
		var x: float = positions[i].x
		var z: float = positions[i].y
		var s := 0.75 + _rng.randf() * 0.8
		var tilt := Basis.from_euler(Vector3(
			(_rng.randf() - 0.5) * 0.07, _rng.randf() * PI, (_rng.randf() - 0.5) * 0.07))
		var b := tilt.scaled(Vector3(s, s, s))
		# trunk centered at 2.7*s, canopy at 6.4*s (world.js baked translations)
		mm_trunk.set_instance_transform(i, Transform3D(b, Vector3(x, TREE_TRUNK_H * 0.5 * s, z)))
		mm_canopy.set_instance_transform(i, Transform3D(b, Vector3(x, 6.4 * s, z)))
		World.add_circle(x, z, 0.4 * s + 0.15)

func _tree_reject(x: float, z: float) -> bool:
	if z > 58.0:
		var d := absf(x - World.path_x(z))
		if d < 4.5: return true
		if d > 42.0: return true
		return false
	if Vector2(x, z).distance_to(Vector2(World.FOUNTAIN.x, World.FOUNTAIN.z)) < 7.0: return true
	if Vector2(x, z).distance_to(Vector2(World.SIGN.x, World.SIGN.z)) < 5.0: return true
	if Vector2(x, z).distance_to(Vector2(World.CABIN.x, World.CABIN.z)) < 9.0: return true
	if Vector2(x, z).distance_to(Vector2(World.HOUSE.x, World.HOUSE.z)) < 22.0: return true
	if z < -88.0 and absf(x) < 12.0: return true
	return false

# ---------------------------------------------------------------- fences
func _build_fences() -> void:
	var post := BoxMesh.new()
	post.size = Vector3(0.12, 1.1, 0.12)
	var xforms: Array = []
	var z := 195.0
	while z >= 90.0 and xforms.size() < 70:
		if _rng.randf() < 0.25:   # gaps — broken fence
			z -= 3.2; continue
		var side := 1.0 if fmod(z, 2.0) > 1.0 else -1.0
		var sy := 0.7 + _rng.randf() * 0.5
		var b := Basis.from_euler(Vector3((_rng.randf() - 0.5) * 0.5, 0, (_rng.randf() - 0.5) * 0.5))
		b = b.scaled(Vector3(1, sy, 1))
		var pos := Vector3(World.path_x(z) + side * 3.4, 0.55 * sy, z)
		xforms.append(Transform3D(b, pos))
		z -= 3.2
	var mm := _make_multimesh(post, _mat(Color8(0x3a, 0x30, 0x27)), xforms.size())
	for i in xforms.size():
		mm.set_instance_transform(i, xforms[i])

# ---------------------------------------------------------------- rocks
func _build_rocks() -> void:
	var rock := SphereMesh.new()
	rock.radius = 0.5
	rock.height = 1.0
	rock.radial_segments = 6
	rock.rings = 3
	var mm := _make_multimesh(rock, _mat(Color8(0x33, 0x33, 0x2f)), 60)
	for i in 60:
		var x := -120.0 + _rng.randf() * 240.0
		var z := -130.0 + _rng.randf() * 330.0
		var s := 0.4 + _rng.randf() * 1.3
		var b := Basis.from_euler(Vector3(0, _rng.randf() * PI, 0)).scaled(Vector3(s, s * 0.7, s))
		mm.set_instance_transform(i, Transform3D(b, Vector3(x, 0.1, z)))
		if s > 0.8: World.add_circle(x, z, s * 0.5)

# ---------------------------------------------------------------- sky eyes
func _build_sky_eyes() -> void:
	var tex := _eye_texture()
	for i in 14:
		var mat := StandardMaterial3D.new()
		mat.albedo_texture = tex
		mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
		mat.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
		mat.disable_receive_shadows = true
		mat.no_depth_test = false
		var op := 0.05 + _rng.randf() * 0.1
		mat.albedo_color = Color(1, 1, 1, op)
		var w := 26.0 + _rng.randf() * 40.0
		var qm := QuadMesh.new()
		qm.size = Vector2(w, w * 0.5)
		var mi := MeshInstance3D.new()
		mi.mesh = qm
		mi.material_override = mat
		mi.position = Vector3(-260 + _rng.randf() * 520, 110 + _rng.randf() * 90, -260 + _rng.randf() * 520)
		# face down toward the player (world.js eye.rotation.x = PI/2 + jitter)
		mi.rotation = Vector3(PI / 2 + (_rng.randf() - 0.5) * 0.5, 0, _rng.randf() * TAU)
		add_child(mi)
		_eyes.append({"node": mi, "mat": mat, "blink_t": _rng.randf() * 14.0,
				"base_op": op, "closing": 0.0, "base_sy": 1.0})

func _eye_texture() -> ImageTexture:
	var s := 128
	var img := Image.create(s * 2, s, false, Image.FORMAT_RGBA8)
	img.fill(Color(0, 0, 0, 0))
	var cx := float(s)      # 128
	var cy := float(s) * 0.5
	for y in s:
		for x in s * 2:
			var nx := (x - cx) / 110.0
			var ny := (y - cy) / 46.0
			if nx * nx + ny * ny <= 1.0:                       # sclera ellipse
				var col := Color(0.74, 0.74, 0.78, 0.85)
				var ir := Vector2(x - cx, y - cy).length()
				if ir < 34.0:                                   # iris
					col = Color(0.1, 0.1, 0.13, 0.95)
				if ir < 14.0:                                   # pupil
					col = Color(0, 0, 0, 1)
				img.set_pixel(x, y, col)
	return ImageTexture.create_from_image(img)

func _update_eyes(dt: float, stress: float) -> void:
	for e in _eyes:
		e.blink_t -= dt
		if e.blink_t < 0.0:
			e.blink_t = 6.0 + _rng.randf() * 16.0
			e.closing = 0.28
		var node: MeshInstance3D = e.node
		if e.closing > 0.0:
			e.closing -= dt
			var f := absf(sin((e.closing / 0.28) * PI))
			node.scale.y = maxf(0.05, f)
		else:
			node.scale.y = 1.0
		e.mat.albedo_color.a = e.base_op + (stress / 100.0) * 0.16

# ---------------------------------------------------------------- fountain
func _build_fountain() -> void:
	var g := Node3D.new()
	g.position = World.FOUNTAIN
	add_child(g)
	var stone := _mat(Color8(0x4a, 0x4f, 0x44))
	g.add_child(_cyl(2.5, 2.3, 0.85, stone, Vector3(0, 0.42, 0), 10))
	var rim := TorusMesh.new(); rim.inner_radius = 2.14; rim.outer_radius = 2.46; rim.rings = 10; rim.ring_segments = 5
	var rmi := MeshInstance3D.new(); rmi.mesh = rim; rmi.material_override = stone; rmi.position = Vector3(0, 0.86, 0)
	g.add_child(rmi)
	g.add_child(_cyl(0.22, 0.3, 1.6, stone, Vector3(0, 1.4, 0), 7))
	g.add_child(_cyl(0.85, 0.5, 0.35, stone, Vector3(0, 2.25, 0), 9))
	# dark reflective water disc
	var water := CylinderMesh.new(); water.top_radius = 2.1; water.bottom_radius = 2.1; water.height = 0.02
	var wmat := StandardMaterial3D.new(); wmat.albedo_color = Color8(0x06, 0x09, 0x0c); wmat.roughness = 0.15; wmat.metallic = 0.6
	var wmi := MeshInstance3D.new(); wmi.mesh = water; wmi.material_override = wmat; wmi.position = Vector3(0, 0.72, 0)
	g.add_child(wmi)
	World.add_circle(World.FOUNTAIN.x, World.FOUNTAIN.z, 2.7)

func _cyl(top: float, bot: float, h: float, mat: Material, pos: Vector3, seg := 8) -> MeshInstance3D:
	var c := CylinderMesh.new(); c.top_radius = top; c.bottom_radius = bot; c.height = h; c.radial_segments = seg
	var mi := MeshInstance3D.new(); mi.mesh = c; mi.material_override = mat; mi.position = pos
	return mi

# ---------------------------------------------------------------- sign
func _build_sign() -> void:
	var wood := _mat(Color8(0x3d, 0x32, 0x26))
	var post := BoxMesh.new(); post.size = Vector3(0.18, 2.6, 0.18)
	_add_mesh(post, wood, Transform3D(Basis.IDENTITY, Vector3(World.SIGN.x, 1.3, World.SIGN.z)))
	World.add_circle(World.SIGN.x, World.SIGN.z, 0.3)

	var board := BoxMesh.new(); board.size = Vector3(2.0, 0.62, 0.07)
	var bmi := _add_mesh(board, wood, Transform3D(Basis(Vector3.FORWARD, 0.04),
			Vector3(World.SIGN.x, 1.9, World.SIGN.z + 0.13)))

	# weathered visible text
	var lbl := Label3D.new()
	lbl.text = "CASA  BLANCA  →\nel agua recuerda"
	lbl.font_size = 28
	lbl.modulate = Color(0.78, 0.74, 0.66, 0.5)
	lbl.position = Vector3(0, 0, 0.05)
	lbl.pixel_size = 0.0045
	bmi.add_child(lbl)

	# hidden code "2 · 7 · 4 · 1" — only visible at a precise viewing angle
	_sign_code = Label3D.new()
	_sign_code.text = "2 · 7 · 4 · 1"
	_sign_code.font_size = 48
	_sign_code.modulate = Color(0.88, 0.86, 0.78, 0.0)
	_sign_code.pixel_size = 0.006
	_sign_code.position = Vector3(World.SIGN.x, 1.9, World.SIGN.z + 0.18)
	add_child(_sign_code)

func _update_sign(cam: Camera3D) -> void:
	if _sign_code == null or cam == null:
		return
	# board faces +Z; reveal the code when the camera looks at it nearly head-on.
	var to_cam := (cam.global_position - _sign_code.global_position)
	to_cam.y = 0.0
	to_cam = to_cam.normalized()
	var align := to_cam.dot(Vector3(0, 0, 1))          # 1 == directly in front (+Z)
	var dist := cam.global_position.distance_to(_sign_code.global_position)
	var vis := 0.0
	if align > 0.985 and dist < 9.0:
		vis = clampf((align - 0.985) / 0.015, 0.0, 1.0) * 0.95
	_sign_code.modulate.a = lerpf(_sign_code.modulate.a, vis, 0.2)

# ---------------------------------------------------------------- cabin
func _build_cabin() -> void:
	var cx := World.CABIN.x
	var cz := World.CABIN.z
	var wall := _mat(Color8(0x4a, 0x42, 0x38))
	var g := Node3D.new(); g.position = World.CABIN; add_child(g)

	var mk := func(w: float, h: float, d: float, x: float, y: float, z: float) -> void:
		var bm := BoxMesh.new(); bm.size = Vector3(w, h, d)
		var mi := MeshInstance3D.new(); mi.mesh = bm; mi.material_override = wall; mi.position = Vector3(x, y, z)
		g.add_child(mi)
		World.add_box(cx + x - w / 2, cx + x + w / 2, cz + z - d / 2, cz + z + d / 2)
	mk.call(5, 2.6, 0.2, 0, 1.3, -2)    # north
	mk.call(5, 2.6, 0.2, 0, 1.3, 2)     # south
	mk.call(0.2, 2.6, 4, 2.5, 1.3, 0)   # east
	mk.call(0.2, 2.6, 1.5, -2.5, 1.3, -1.25)  # west (door gap)
	mk.call(0.2, 2.6, 1.5, -2.5, 1.3, 1.25)
	# lintel (visual only)
	var lin := BoxMesh.new(); lin.size = Vector3(0.2, 0.6, 1.0)
	var lmi := MeshInstance3D.new(); lmi.mesh = lin; lmi.material_override = wall; lmi.position = Vector3(-2.5, 2.3, 0)
	g.add_child(lmi)
	# roof
	var roof := CylinderMesh.new(); roof.top_radius = 0.0; roof.bottom_radius = 4.2; roof.height = 1.6; roof.radial_segments = 4
	var rmi := MeshInstance3D.new(); rmi.mesh = roof; rmi.material_override = wall
	rmi.position = Vector3(0, 3.4, 0); rmi.rotation = Vector3(0, PI / 4, 0)
	g.add_child(rmi)
	# floor
	var fl := BoxMesh.new(); fl.size = Vector3(5, 0.08, 4)
	var fmi := MeshInstance3D.new(); fmi.mesh = fl; fmi.material_override = _mat(Color8(0x35, 0x30, 0x2a)); fmi.position = Vector3(0, 0.04, 0)
	g.add_child(fmi)
	# door (closed; opens with cabin keypad code 2741)
	var door := BoxMesh.new(); door.size = Vector3(0.08, 2.0, 1.0)
	var dmi := MeshInstance3D.new(); dmi.mesh = door; dmi.material_override = _mat(Color8(0x55, 0x40, 0x2c))
	dmi.position = Vector3(-2.5, 1.0, 0)
	g.add_child(dmi)
	var dcol := {"type": "box", "minX": cx - 2.6, "maxX": cx - 2.4, "minZ": cz - 0.5, "maxZ": cz + 0.5}
	World.colliders.append(dcol)
	World.cabin_door = dmi
	World.cabin_door_collider = dcol
	# keypad by the door (interactable lives in house.gd / Director)
	var pad := BoxMesh.new(); pad.size = Vector3(0.04, 0.3, 0.2)
	var pmat := _mat(Color8(0x22, 0x22, 0x26)); pmat.emission_enabled = true; pmat.emission = Color(0.06, 0.12, 0.16)
	var pmi := MeshInstance3D.new(); pmi.mesh = pad; pmi.material_override = pmat
	pmi.position = Vector3(-2.55, 1.4, 0.85)
	g.add_child(pmi)

# ---------------------------------------------------------------- rain
func _build_rain() -> void:
	_rain = GPUParticles3D.new()
	_rain.amount = 2000
	_rain.lifetime = 1.1
	_rain.preprocess = 1.0
	_rain.visibility_aabb = AABB(Vector3(-25, -20, -25), Vector3(50, 40, 50))
	var pm := ParticleProcessMaterial.new()
	pm.emission_shape = ParticleProcessMaterial.EMISSION_SHAPE_BOX
	pm.emission_box_extents = Vector3(22, 0.5, 22)
	pm.direction = Vector3(0, -1, 0)
	pm.spread = 0.0
	pm.initial_velocity_min = 16.0
	pm.initial_velocity_max = 20.0
	# angled ~15° off vertical (world.js rain tilt)
	pm.gravity = Vector3(sin(deg_to_rad(15.0)) * 9.8, -cos(deg_to_rad(15.0)) * 9.8, 0) * 1.5
	_rain.process_material = pm
	var streak := BoxMesh.new(); streak.size = Vector3(0.012, 0.38, 0.012)
	var smat := StandardMaterial3D.new()
	smat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	smat.albedo_color = Color(0.6, 0.7, 0.85, 0.35)
	smat.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	streak.material = smat
	_rain.draw_pass_1 = streak
	_rain.local_coords = false
	_rain.position = Vector3(0, 14, 0)
	add_child(_rain)

# ---------------------------------------------------------------- bounds
func _build_bounds_and_zones() -> void:
	World.add_box(-160, -140, -200, 230)
	World.add_box(140, 160, -200, 230)
	World.add_box(-160, 160, -200, -155)
	World.add_box(-160, 160, 212, 230)
	# default outdoor ground zone (y = 0, lowest priority)
	World.add_zone(GroundZone.new(0, 0.0))

# ---------------------------------------------------------------- update
func _process(delta: float) -> void:
	var stress: float = Director.stress
	_update_eyes(delta, stress)
	var player := get_tree().get_first_node_in_group("player")
	if player:
		var cam: Camera3D = player.get_node_or_null("Camera3D")
		_update_sign(cam)
		if _rain:
			_rain.global_position = Vector3(player.global_position.x, player.global_position.y + 14.0, player.global_position.z)
