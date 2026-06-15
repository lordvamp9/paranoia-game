# PARANOIA — ramp ground zone: height interpolates linearly along Z between two
# (z, height) endpoints. Port of the stair ramps in house.js (main stairs 0->3.1,
# basement stairs 0->-2.5). priority 3 so a ramp wins over the flat floor it joins.
class_name RampZone
extends GroundZone

var z_a := 0.0
var h_a := 0.0
var z_b := 0.0
var h_b := 0.0

func _init(p_priority: int, p_z_a: float, p_h_a: float, p_z_b: float, p_h_b: float,
		p_min_x: float, p_max_x: float, p_min_z: float, p_max_z: float) -> void:
	super(p_priority, p_h_a, p_min_x, p_max_x, p_min_z, p_max_z)
	z_a = p_z_a; h_a = p_h_a; z_b = p_z_b; h_b = p_h_b

func height(_x: float, z: float) -> float:
	var denom := z_b - z_a
	var t := 0.0 if absf(denom) < 1e-6 else clampf((z - z_a) / denom, 0.0, 1.0)
	return lerpf(h_a, h_b, t)
