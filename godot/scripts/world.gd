# PARANOIA — world data singleton (autoload). Mirrors the exported WORLD object
# in src/world.js: collider list + ground zones read by the player, plus the
# canonical layout constants. The geometry itself is built by world_root.gd
# (the World.tscn root), which pushes colliders/zones in here as it builds.
extends Node

# Colliders: {type:"circle", x,z,r} | {type:"box", minX,maxX,minZ,maxZ, [minY,maxY]}
var colliders: Array = []
var ground_zones: Array = []   # Array[GroundZone]

# references set during world build, toggled by the Director (door opening, etc.)
var cabin_door: Node3D = null
var cabin_door_collider = null

# --- canonical layout (meters) — exact constants from world.js ---
const FOUNTAIN := Vector3(-35, 0, 20)
const SIGN := Vector3(10, 0, -10)
const CABIN := Vector3(60, 0, -30)
const HOUSE := Vector3(0, 0, -110)

# Winding path: pathX(z) = 6 * sin((200 - z) * 0.055)
func path_x(z: float) -> float:
	return 6.0 * sin((200.0 - z) * 0.055)

func reset() -> void:
	colliders.clear()
	ground_zones.clear()

func add_circle(x: float, z: float, r: float) -> void:
	colliders.append({"type": "circle", "x": x, "z": z, "r": r})

func add_box(min_x: float, max_x: float, min_z: float, max_z: float) -> void:
	colliders.append({"type": "box", "minX": min_x, "maxX": max_x, "minZ": min_z, "maxZ": max_z})

# box2: collides only within an optional Y band (minY/maxY). Used by house floors
# (basement walls maxY=0, upstairs walls minY=2.5) so they don't block other levels.
# Returns the dict so callers can keep a reference to disable it later (open doors).
func add_box2(min_x: float, max_x: float, min_z: float, max_z: float,
		min_y = null, max_y = null) -> Dictionary:
	var c := {"type": "box2", "minX": min_x, "maxX": max_x, "minZ": min_z, "maxZ": max_z}
	if min_y != null: c["minY"] = min_y
	if max_y != null: c["maxY"] = max_y
	colliders.append(c)
	return c

func add_zone(zone: GroundZone) -> void:
	ground_zones.append(zone)
