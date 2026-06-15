# PARANOIA — the white house (exterior + 3 floors + hidden room) and all
# interactables. Port of src/house.js. House center = (HX,HZ)=(0,-110); front
# door faces +Z (the way the player arrives). Registers wall colliders and the
# multi-floor ground zones into the World singleton; spawns Interactable nodes.
#
# NOTE: house.js:441 had `&& false` disabling the stairwell-hole exclusion on the
# upstairs floor zone. The CORRECT logic (exclusion active) is ported here.
extends Node3D

const HX := 0.0
const HZ := -110.0

# door refs (opened by the Director)
var bedroom_door: MeshInstance3D
var bedroom_door_col
var basement_door: MeshInstance3D
var basement_door_col
var false_wall: MeshInstance3D
var false_wall_col
var mirror_figure_mat: StandardMaterial3D

var _ext_mat: StandardMaterial3D
var _wall_mat: StandardMaterial3D
var _floor_mat: StandardMaterial3D
var _wood_mat: StandardMaterial3D
var _bwall_mat: StandardMaterial3D
var _stair_mat: StandardMaterial3D
var _player: Node3D
var _entered := false

func _ready() -> void:
	add_to_group("house")
	_make_materials()
	_build_exterior()
	_build_ground_floor()
	_build_upstairs()
	_build_basement()
	_register_zones()
	_build_interactables()
	_player = get_tree().get_first_node_in_group("player")

# ----------------------------------------------------------------- helpers
func _make_materials() -> void:
	_ext_mat = _m(Color8(0x9a, 0x9a, 0xa0)); _ext_mat.emission_enabled = true
	_ext_mat.emission = Color8(0x20, 0x20, 0x24)
	_wall_mat = _m(Color8(0x5e, 0x5c, 0x58))
	_floor_mat = _m(Color8(0x4a, 0x45, 0x40))
	_wood_mat = _m(Color8(0x3d, 0x32, 0x26))
	_bwall_mat = _m(Color8(0x55, 0x52, 0x4a))
	_stair_mat = _m(Color8(0x4e, 0x46, 0x3c))

func _m(c: Color) -> StandardMaterial3D:
	var m := StandardMaterial3D.new()
	m.albedo_color = c
	m.roughness = 0.95
	return m

# box mesh at world pos (x and z are LOCAL to house center; y is world)
func _box(w: float, h: float, d: float, x: float, y: float, z: float,
		mat: Material, rot_y := 0.0) -> MeshInstance3D:
	var bm := BoxMesh.new(); bm.size = Vector3(w, h, d)
	var mi := MeshInstance3D.new(); mi.mesh = bm; mi.material_override = mat
	mi.position = Vector3(HX + x, y, HZ + z); mi.rotation.y = rot_y
	add_child(mi)
	return mi

func _col(min_x: float, max_x: float, min_z: float, max_z: float) -> void:
	World.add_box(HX + min_x, HX + max_x, HZ + min_z, HZ + max_z)

func _col2(min_x: float, max_x: float, min_z: float, max_z: float, miny = null, maxy = null):
	return World.add_box2(HX + min_x, HX + max_x, HZ + min_z, HZ + max_z, miny, maxy)

# a wall: visual box + ground-level collider
func _wall(w: float, h: float, d: float, x: float, y: float, z: float, mat := _wall_mat) -> MeshInstance3D:
	var mi := _box(w, h, d, x, y, z, mat)
	_col(x - w / 2, x + w / 2, z - d / 2, z + d / 2)
	return mi

# ----------------------------------------------------------------- exterior
func _build_exterior() -> void:
	# south (front) wall with door opening x[-0.6,0.6]
	_box(5.4, 5.6, 0.25, -3.3, 2.8, 5, _ext_mat)
	_box(5.4, 5.6, 0.25, 3.3, 2.8, 5, _ext_mat)
	_box(1.2, 3.4, 0.25, 0, 2.2 + 1.7, 5, _ext_mat)     # lintel over the door
	_box(12, 5.6, 0.25, 0, 2.8, -5, _ext_mat)           # back
	_box(0.25, 5.6, 10, -6, 2.8, 0, _ext_mat)           # west
	_box(0.25, 5.6, 10, 6, 2.8, 0, _ext_mat)            # east
	_col(-6.1, -0.65, 4.8, 5.2); _col(0.65, 6.1, 4.8, 5.2)
	_col(-6.1, 6.1, -5.2, -4.8)
	_col(-6.2, -5.85, -5, 5); _col(5.85, 6.2, -5, 5)

	# asymmetric roof
	var roof := CylinderMesh.new(); roof.top_radius = 0.0; roof.bottom_radius = 8.6; roof.height = 2.6; roof.radial_segments = 4
	var rmi := MeshInstance3D.new(); rmi.mesh = roof; rmi.material_override = _ext_mat
	rmi.position = Vector3(HX + 0.2, 6.7, HZ); rmi.rotation.y = PI / 4 + 0.05
	rmi.scale = Vector3(1.18, 1, 0.95)
	add_child(rmi)
	_box(0.7, 1.8, 0.7, 3.1, 7.0, -1.2, _ext_mat, 0.06)  # chimney

	# windows — semi-transparent glass #334455 a0.45, deliberately misaligned
	var glass := StandardMaterial3D.new()
	glass.albedo_color = Color(0.2, 0.267, 0.333, 0.45)   # #334455
	glass.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	glass.roughness = 0.1; glass.metallic = 0.4
	glass.emission_enabled = true; glass.emission = Color(0.07, 0.10, 0.14)
	var wins := [
		[-3.4, 1.6, 5.13, 0.0], [2.8, 1.75, 5.13, 0.0], [4.6, 1.5, 5.13, 0.0], [-1.7, 1.65, 5.13, 0.0],
		[-2.6, 4.3, 5.13, 0.03], [3.3, 4.45, 5.13, -0.04],
		[6.13, 1.6, -1.0, PI / 2], [-6.13, 1.7, 1.5, -PI / 2],
	]
	for w in wins:
		var qm := QuadMesh.new(); qm.size = Vector2(0.9, 1.2)
		var mi := MeshInstance3D.new(); mi.mesh = qm; mi.material_override = glass
		mi.position = Vector3(HX + w[0], w[1], HZ + w[2]); mi.rotation.y = w[3] + PI
		add_child(mi)

	# front door — dark red, ajar 15°, hinged on its left edge
	var door := _box(1.2, 2.2, 0.08, 0.0, 1.1, 5.0, _m(Color8(0x4a, 0x14, 0x10)))
	door.position += Vector3(-0.6, 0, 0)     # shift so hinge is at left
	door.rotation.y = deg_to_rad(15.0)       # ajar (task spec; JS used wider)

# ----------------------------------------------------------------- ground
func _build_ground_floor() -> void:
	_box(12, 0.1, 10, 0, 0.02, 0, _floor_mat)            # slab
	# upstairs slab (= ceiling) with stairwell hole at x[-6,-4.2], z[-4.8,-0.2]
	_box(12, 0.18, 4.0, 0, 3.0, 3.0, _floor_mat)
	_box(12, 0.18, 1.2, 0, 3.0, 0.4, _floor_mat)
	_box(10.2, 0.18, 4.6, 0.9, 3.0, -2.5, _floor_mat)
	_box(12, 0.18, 0.2, 0, 3.0, -4.9, _floor_mat)

	# interior walls
	_wall(0.15, 3, 2.0, 2, 1.5, 1.0)                     # living/kitchen divide
	_wall(0.15, 3, 1.8, 2, 1.5, 4.1)
	_wall(2.0, 3, 0.15, -5.0, 1.5, 0)                    # hall at z=0
	_wall(5.8, 3, 0.15, 0.1, 1.5, 0)
	_wall(1.8, 3, 0.15, 5.1, 1.5, 0)

	# living room
	_wall(2.2, 0.75, 0.9, -3.5, 0.38, 3.9, _m(Color8(0x4e, 0x4e, 0x52)))   # sofa
	_box(2.2, 0.5, 0.25, -3.5, 0.95, 4.35, _m(Color8(0x44, 0x44, 0x4a)))   # sofa back
	_wall(1.1, 0.45, 0.7, -3.4, 0.23, 2.2, _wood_mat)                      # coffee table
	_box(0.06, 1.5, 0.06, -5.3, 0.7, 1.0, _wood_mat, 0.0).rotation.z = 0.5 # lamp
	for fp in [[-2.0, 1.8, 4.9], [-4.5, 1.65, 4.9], [0.5, 1.9, 4.9]]:
		_box(0.6, 0.8, 0.04, fp[0], fp[1], fp[2], _wood_mat)               # frame

	# kitchen
	_wall(0.9, 1.9, 0.8, 5.4, 0.95, 4.4, _m(Color8(0x6e, 0x6e, 0x6a)))     # fridge
	_wall(0.9, 0.95, 0.8, 5.4, 0.48, 3.3, _m(Color8(0x55, 0x54, 0x4e)))    # stove
	_wall(0.9, 0.9, 2.4, 5.4, 0.45, 1.5, _wood_mat)                        # counter
	for i in 3:
		_box(0.06, 0.22, 0.6, 4.93, 0.62, 0.75 + i * 0.72, _m(Color8(0x4a, 0x3e, 0x2e)))  # drawers
	_wall(0.9, 0.72, 0.9, 3.0, 0.36, 3.8, _wood_mat)                       # small table

	# main stairs UP (west): x[-5.6,-4.4], rises 0->3 going north
	for i in 12:
		_box(1.2, 0.25, 0.38, -5.0, 0.125 + i * 0.25, -0.4 - i * 0.367, _stair_mat)
	_col(-4.45, -4.3, -4.6, -0.2)                        # railing collider

	# basement stairs DOWN (east): x[4.4,5.6], drops 0->-2.5
	for i in 11:
		_box(1.2, 0.25, 0.4, 5.0, -0.23 - i * 0.227, -0.55 - i * 0.39, _stair_mat)
	_wall(0.15, 3, 4.6, 4.4, 1.5, -2.5)                  # stairwell wall

	# basement door (locked metal-blue) at corridor entrance
	basement_door = _box(1.1, 2.1, 0.09, 5.0, 1.05, -0.3, _m(Color8(0x33, 0x41, 0x4a)))
	basement_door_col = _col2(4.4, 5.6, -0.42, -0.18)

# ----------------------------------------------------------------- upstairs
func _build_upstairs() -> void:
	# wall sealing bedroom from landing at x=-4.2 (opening z[-4.7,-3.7])
	_box(0.15, 2.9, 4.9, -4.2, 4.55, -1.25, _wall_mat)
	_col2(-4.3, -4.1, -3.7, 1.0, 2.5)
	_box(0.15, 0.8, 1.0, -4.2, 5.6, -4.2, _wall_mat)     # lintel
	# bedroom door (locked — needs water key)
	bedroom_door = _box(0.09, 2.1, 1.0, -4.2, 4.1, -4.2, _m(Color8(0x55, 0x40, 0x2c)))
	bedroom_door_col = _col2(-4.32, -4.08, -4.72, -3.68, 2.5)
	# upstairs south wall (z=1)
	_box(12, 2.9, 0.15, 0, 4.55, 1.0, _wall_mat)
	_col2(-6, 6, 0.9, 1.1, 2.5)

	# bedroom furniture (floor y=3.1)
	_box(1.6, 0.5, 2.2, -1.5, 3.35, -3.6, _m(Color8(0x6a, 0x6a, 0x66)))    # bed
	_col2(-2.3, -0.7, -4.7, -2.5, 2.5)
	_box(0.7, 2.2, 1.6, 5.5, 4.2, -2.0, _m(Color8(0x33, 0x29, 0x1d)))      # wardrobe
	_col2(5.1, 6.0, -2.8, -1.2, 2.5)
	# mirror (cold metal) on east wall, facing west
	var mir := QuadMesh.new(); mir.size = Vector2(0.9, 1.7)
	var mmat := StandardMaterial3D.new(); mmat.albedo_color = Color8(0x20, 0x28, 0x30)
	mmat.roughness = 0.08; mmat.metallic = 0.9
	var mmi := MeshInstance3D.new(); mmi.mesh = mir; mmi.material_override = mmat
	mmi.position = Vector3(HX + 5.12, 4.2, HZ - 2.0); mmi.rotation.y = -PI / 2
	add_child(mmi)
	# figure that appears "in" the mirror (hidden until mirror event)
	mirror_figure_mat = StandardMaterial3D.new()
	mirror_figure_mat.albedo_color = Color(0, 0, 0, 0)
	mirror_figure_mat.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	var fig := _box(0.34, 0.8, 0.05, 5.04, 4.15, -2.0, mirror_figure_mat, -PI / 2)
	_box(0.22, 0.3, 0.05, 5.04, 4.72, -2.0, mirror_figure_mat, -PI / 2)
	# desk + rug
	_box(1.2, 0.78, 0.6, 2.5, 3.5, -4.6, _wood_mat)
	var rug := QuadMesh.new(); rug.size = Vector2(2.2, 1.6)
	var rmat := _m(Color8(0x26, 0x28, 0x30))
	var rmi := MeshInstance3D.new(); rmi.mesh = rug; rmi.material_override = rmat
	rmi.position = Vector3(HX + 1.2, 3.12, HZ - 2.0); rmi.rotation.x = -PI / 2
	add_child(rmi)

# ----------------------------------------------------------------- basement
func _build_basement() -> void:
	_box(13.5, 0.1, 10, -0.75, -2.55, 0, _m(Color8(0x37, 0x35, 0x2f)))     # floor
	_box(13.5, 0.1, 10, -0.75, -0.25, 0, _m(Color8(0x37, 0x35, 0x2f)))     # ceiling
	var mkB := func(w: float, h: float, d: float, x: float, y: float, z: float, collide := true) -> void:
		_box(w, h, d, x, y, z, _bwall_mat)
		if collide: _col2(x - w / 2, x + w / 2, z - d / 2, z + d / 2, null, 0)
	mkB.call(13.5, 2.4, 0.2, -0.75, -1.4, 5)
	mkB.call(13.5, 2.4, 0.2, -0.75, -1.4, -5)
	mkB.call(0.2, 2.4, 10, -7.5, -1.4, 0)
	mkB.call(0.2, 2.4, 10, 6, -1.4, 0)
	mkB.call(0.2, 2.4, 8.2, 4.4, -1.4, 0.9)              # seals basement from stair corridor
	# pipes
	var pipe_mat := _m(Color8(0x4a, 0x3c, 0x30))
	for p in [[-2.0, -4.6], [1.0, -4.6], [3.5, -4.6]]:
		var pipe := CylinderMesh.new(); pipe.top_radius = 0.08; pipe.bottom_radius = 0.08; pipe.height = 9; pipe.radial_segments = 6
		var pmi := MeshInstance3D.new(); pmi.mesh = pipe; pmi.material_override = pipe_mat
		pmi.position = Vector3(HX + p[0], -0.5, HZ + p[1]); pmi.rotation.z = PI / 2
		add_child(pmi)
	# boxes + dead machine
	mkB.call(1.0, 0.8, 1.0, 2.5, -2.1, 3.5)
	mkB.call(0.8, 0.6, 0.8, 3.6, -2.2, 3.2)
	_box(1.8, 1.6, 1.0, -2.5, -1.7, 4.2, _m(Color8(0x3a, 0x40, 0x38)))
	_col2(-3.4, -1.6, 3.7, 4.7, null, 0)
	# mural on north wall (procedural eyes + TRUST ME)
	var mural := QuadMesh.new(); mural.size = Vector2(6.5, 2.0)
	var mui := MeshInstance3D.new(); mui.mesh = mural
	mui.material_override = _texmat(_mural_texture())
	mui.position = Vector3(HX - 1, -1.45, HZ - 4.88)
	add_child(mui)

	# false wall (west): perimeter wall covers all but z[-1,0.2] gap
	mkB.call(0.2, 2.4, 3.8, -4, -1.4, -3.1)
	mkB.call(0.2, 2.4, 4.6, -4, -1.4, 2.5)
	false_wall = _box(0.22, 2.4, 1.3, -4, -1.4, -0.4, _bwall_mat)
	false_wall_col = _col2(-4.12, -3.88, -1.05, 0.25, null, 0)

	# hidden room (x[-7.5,-4], z[-2.5,1.5])
	mkB.call(3.5, 2.4, 0.2, -5.75, -1.4, -2.5)
	mkB.call(3.5, 2.4, 0.2, -5.75, -1.4, 1.5)
	_box(0.5, 1.0, 0.5, -6, -2.0, -0.5, _bwall_mat)      # pedestal
	# THE ORIGINAL SMARTPHONE — cracked, faintly alive
	var op := _box(0.08, 0.02, 0.16, -6, -1.48, -0.5, _m(Color8(0x0a, 0x0a, 0x0a)))
	op.rotation.y = 0.4
	var ops := QuadMesh.new(); ops.size = Vector2(0.07, 0.14)
	var opmi := MeshInstance3D.new(); opmi.mesh = ops; opmi.material_override = _texmat(_oldphone_texture())
	opmi.position = Vector3(HX - 6, -1.465, HZ - 0.5); opmi.rotation = Vector3(-PI / 2, 0, -0.4)
	add_child(opmi)
	# handwritten note
	var note := QuadMesh.new(); note.size = Vector2(0.22, 0.3)
	var nmi := MeshInstance3D.new(); nmi.mesh = note; nmi.material_override = _m(Color8(0xcf, 0xc9, 0xb8))
	nmi.position = Vector3(HX - 5.7, -1.49, HZ - 0.25); nmi.rotation = Vector3(-PI / 2, 0, 0.3)
	add_child(nmi)

# ----------------------------------------------------------------- zones
func _register_zones() -> void:
	# ground floor (y=0, prio 2)
	World.add_zone(GroundZone.new(2, 0.0, HX - 6, HX + 6, HZ - 5, HZ + 5))
	# upstairs floor (y=3.1, prio 2) — stairwell hole carved out (the &&false fix)
	var up := GroundZone.new(2, 3.1, HX - 6, HX + 6, HZ - 5, HZ + 1)
	up.set_exclusion(-INF, HX - 4.2, HZ - 4.8, HZ - 0.2)
	World.add_zone(up)
	# main stairs ramp (0 -> 3.1, prio 3)
	World.add_zone(RampZone.new(3, HZ - 0.2, 0.0, HZ - 4.6, 3.1,
			HX - 5.6, HX - 4.4, HZ - 4.75, HZ - 0.1))
	# basement stairs ramp (0 -> -2.5, prio 3)
	World.add_zone(RampZone.new(3, HZ - 0.3, 0.0, HZ - 4.6, -2.5,
			HX + 4.4, HX + 5.6, HZ - 4.75, HZ - 0.25))
	# basement floor (y=-2.5, prio 2)
	World.add_zone(GroundZone.new(2, -2.5, HX - 7.5, HX + 6, HZ - 5, HZ + 5))

# ----------------------------------------------------------------- interactables
func _build_interactables() -> void:
	_spawn("kitchen_drawer", "search drawer", Vector3(HX + 4.93, 0.62, HZ + 1.47), 0.0)
	_spawn("bedroom_door", "open door", Vector3(HX - 4.2, 3.1, HZ - 4.2), 3.1)
	_spawn("cabin_keypad", "enter code", Vector3(World.CABIN.x - 2.55, 1.4, World.CABIN.z + 0.85), 0.0, true)
	_spawn("basement_door", "open door", Vector3(HX + 5.0, 0.0, HZ - 0.3), 0.0)
	_spawn("false_wall", "push wall", Vector3(HX - 4.0, -1.4, HZ - 0.4), -2.5)
	_spawn("old_phone", "take phone", Vector3(HX - 6.0, -1.48, HZ - 0.5), -2.5)
	_spawn("note", "read note", Vector3(HX - 5.7, -1.49, HZ - 0.25), -2.5)
	_spawn("mirror", "look", Vector3(HX + 1.2, 3.1, HZ - 2.0), 3.1)

func _spawn(id: String, prompt: String, pos: Vector3, floor_y: float, keypad := false) -> void:
	var it := Interactable.new()
	it.id = id
	it.prompt = prompt
	it.floor_y = floor_y
	it.keypad = keypad
	add_child(it)
	it.global_position = pos

# ----------------------------------------------------------------- door opening
func open_bedroom_door() -> void:
	if bedroom_door:
		bedroom_door.position.z += 0.9
		bedroom_door.rotation.y = 1.2
	_disable(bedroom_door_col)

func open_basement_door() -> void:
	if basement_door:
		basement_door.position.x -= 0.95
		basement_door.rotation.y = 1.2
	_disable(basement_door_col)

func open_false_wall() -> void:
	if false_wall:
		false_wall.position.y -= 2.1
	_disable(false_wall_col)

func _disable(c) -> void:
	if c:
		c["minX"] = 99999.0; c["maxX"] = 99999.0
		c["minZ"] = 99999.0; c["maxZ"] = 99999.0

# ----------------------------------------------------------------- front-door objective
func _process(_dt: float) -> void:
	if _entered or _player == null:
		return
	var d := _player.global_position.distance_to(Vector3(HX, 0, HZ + 5))
	if d < 7.0:
		var phone := get_tree().get_first_node_in_group("phone")
		if phone:
			phone.set_objective("enter the house")
	if Vector2(_player.global_position.x - HX, _player.global_position.z - HZ).length() < 5.0:
		_entered = true
		Director.flags["in_house"] = true

# ----------------------------------------------------------------- textures
func _texmat(t: Texture2D) -> StandardMaterial3D:
	var m := StandardMaterial3D.new()
	m.albedo_texture = t
	m.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	return m

func _mural_texture() -> ImageTexture:
	var img := Image.create(512, 160, false, Image.FORMAT_RGBA8)
	img.fill(Color8(0x1c, 0x1a, 0x18))
	var rng := RandomNumberGenerator.new(); rng.seed = 7
	for row in 4:
		for col in 9:
			var cx := 30 + col * 52 + (row % 2) * 26
			var cy := 18 + row * 34
			for a in range(0, 360, 12):
				var rad := deg_to_rad(a)
				var px := cx + int(cos(rad) * 18)
				var py := cy + int(sin(rad) * 8)
				if px >= 0 and px < 512 and py >= 0 and py < 160:
					img.set_pixel(px, py, Color(0.66, 0.59, 0.51))
			for rr in 4:
				for a2 in range(0, 360, 30):
					var px2 := cx + int(cos(deg_to_rad(a2)) * rr)
					var py2 := cy + int(sin(deg_to_rad(a2)) * rr)
					if px2 >= 0 and px2 < 512 and py2 >= 0 and py2 < 160:
						img.set_pixel(px2, py2, Color(0.59, 0.12, 0.10))
	return ImageTexture.create_from_image(img)

func _oldphone_texture() -> ImageTexture:
	var img := Image.create(64, 128, false, Image.FORMAT_RGBA8)
	img.fill(Color8(0x05, 0x06, 0x08))
	# faint green glow text rows (YOU ARE THE RECORDING)
	for y in range(52, 84):
		for x in range(6, 58):
			if (x + y) % 3 == 0:
				img.set_pixel(x, y, Color(0.14, 0.55, 0.18, 0.8))
	return ImageTexture.create_from_image(img)
