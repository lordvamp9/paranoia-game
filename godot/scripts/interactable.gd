# PARANOIA — generic interactable: an Area3D trigger (0.8 m) that shows an
# "[E] interact" prompt when the player is inside and emits used(id) on E.
# Director.use_interactable(id) applies the gameplay response (flags, doors).
# Optional keypad mode (cabin) captures 4 digits and checks a code.
class_name Interactable
extends Area3D

signal used(id: String)

var id := ""
var prompt := "interact"
var radius := 0.8
var floor_y := 0.0       # which level this lives on (XZ trigger + |Δy|<1.6 band)
var keypad := false
var code := "2741"
var enabled := true

var _player: Node3D
var _label: Label3D
var _inside := false
var _kp_active := false
var _kp_entry := ""

func _ready() -> void:
	add_to_group("interactable")
	var col := CollisionShape3D.new()
	var sh := SphereShape3D.new()
	sh.radius = radius
	col.shape = sh
	add_child(col)

	_label = Label3D.new()
	_label.billboard = BaseMaterial3D.BILLBOARD_ENABLED
	_label.font_size = 32
	_label.pixel_size = 0.0035
	_label.no_depth_test = true
	_label.modulate = Color(0.95, 0.95, 1.0)
	_label.outline_size = 6
	_label.position = Vector3(0, 0.45, 0)
	_label.visible = false
	add_child(_label)

	_player = get_tree().get_first_node_in_group("player")

func _physics_process(_dt: float) -> void:
	if _player == null:
		_player = get_tree().get_first_node_in_group("player")
		return
	var pp := _player.global_position
	var dxz := Vector2(pp.x - global_position.x, pp.z - global_position.z).length()
	var same_floor: bool = absf(_player.ground_y - floor_y) < 1.6
	_inside = enabled and dxz < radius and same_floor
	if _kp_active:
		_label.visible = true
		_label.text = "CODE: %s_" % _kp_entry
	else:
		_label.visible = _inside
		if _inside:
			_label.text = "[E] %s" % prompt

func _unhandled_input(event: InputEvent) -> void:
	if _kp_active:
		_handle_keypad(event)
		return
	if _inside and event.is_action_pressed("interact") and _is_nearest():
		if keypad:
			_kp_active = true
			_kp_entry = ""
		else:
			_fire()

func _fire() -> void:
	used.emit(id)
	Director.use_interactable(id)

func _handle_keypad(event: InputEvent) -> void:
	if not (event is InputEventKey and event.pressed and not event.echo):
		return
	var kc: int = event.keycode
	if kc >= KEY_0 and kc <= KEY_9:
		_kp_entry += str(kc - KEY_0)
		Audio.ui_beep()
		if _kp_entry.length() >= 4:
			if _kp_entry == code:
				_kp_active = false
				_fire()
			else:
				_kp_entry = ""    # wrong — clear and retry
	elif kc == KEY_ESCAPE:
		_kp_active = false

func _is_nearest() -> bool:
	var pp := _player.global_position
	var my := Vector2(pp.x - global_position.x, pp.z - global_position.z).length()
	for o in get_tree().get_nodes_in_group("interactable"):
		if o == self or not o.enabled:
			continue
		var od := Vector2(pp.x - o.global_position.x, pp.z - o.global_position.z).length()
		if od < o.radius and od < my and absf(_player.ground_y - o.floor_y) < 1.6:
			return false
	return true
