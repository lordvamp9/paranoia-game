# PARANOIA — ground zone (port of the {contains,height,priority} objects pushed
# into WORLD.groundZones in src/world.js / house.js). The player resolves which
# floor it stands on by priority (ramps > floors > default terrain). Supports an
# optional XZ exclusion rect (e.g. the stairwell hole carved out of the upstairs
# floor — the `&& false` bug in house.js:441 is fixed here so it actually works).
class_name GroundZone
extends RefCounted

var priority: int = 0

var min_x: float = -INF
var max_x: float = INF
var min_z: float = -INF
var max_z: float = INF

var base_height: float = 0.0

# optional exclusion rect (points inside it are NOT contained)
var _has_excl := false
var ex_min_x := 0.0
var ex_max_x := 0.0
var ex_min_z := 0.0
var ex_max_z := 0.0

func _init(p_priority: int = 0, p_height: float = 0.0,
		p_min_x: float = -INF, p_max_x: float = INF,
		p_min_z: float = -INF, p_max_z: float = INF) -> void:
	priority = p_priority
	base_height = p_height
	min_x = p_min_x; max_x = p_max_x
	min_z = p_min_z; max_z = p_max_z

func set_exclusion(a_min_x: float, a_max_x: float, a_min_z: float, a_max_z: float) -> GroundZone:
	_has_excl = true
	ex_min_x = a_min_x; ex_max_x = a_max_x
	ex_min_z = a_min_z; ex_max_z = a_max_z
	return self

func contains(x: float, z: float) -> bool:
	if x < min_x or x > max_x or z < min_z or z > max_z:
		return false
	if _has_excl and x >= ex_min_x and x <= ex_max_x and z >= ex_min_z and z <= ex_max_z:
		return false
	return true

func height(_x: float, _z: float) -> float:
	return base_height
