# PARANOIA — in-game HUD. Inventory bar (5 slots, slot 1 = PHONE), no crosshair
# (horror), and a smoothly fading "[E] interact" prompt when near an interactable.
extends Control

const SLOT := 60.0
const SEL := Color(0.95, 0.95, 1.0)        # bright white border (selected)
const DIM := Color(0.18, 0.18, 0.22)       # subtle dark border (unselected)

var _slots: Array[Panel] = []
var _selected := 0
var _prompt: Label
var _prompt_a := 0.0

func _ready() -> void:
	set_anchors_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_IGNORE

	# inventory bar, bottom-left
	var bar := HBoxContainer.new()
	bar.add_theme_constant_override("separation", 8)
	bar.set_anchors_preset(Control.PRESET_BOTTOM_LEFT)
	bar.position = Vector2(20, -SLOT - 20)
	add_child(bar)
	for i in 5:
		var pnl := Panel.new()
		pnl.custom_minimum_size = Vector2(SLOT, SLOT)
		bar.add_child(pnl)
		_slots.append(pnl)
		if i == 0:
			var lbl := UiUtil.label("PHONE", 9, Color(0.85, 0.85, 0.9), UiUtil.pixel_font())
			lbl.set_anchors_preset(Control.PRESET_FULL_RECT)
			pnl.add_child(lbl)
	_refresh_slots()

	# centered interact prompt (hidden until near an interactable)
	_prompt = UiUtil.label("[E] interact", 18, Color(0.85, 0.85, 0.88), UiUtil.mono_font())
	_prompt.set_anchors_preset(Control.PRESET_CENTER)
	_prompt.position = Vector2(-200, 60)
	_prompt.size = Vector2(400, 30)
	_prompt.modulate.a = 0.0
	add_child(_prompt)
	# no crosshair — intentionally omitted

func _refresh_slots() -> void:
	for i in _slots.size():
		var sb := StyleBoxFlat.new()
		var selected := i == _selected
		sb.bg_color = Color(0.06, 0.06, 0.08, 0.65) if not selected else Color(0.12, 0.12, 0.16, 0.8)
		sb.set_border_width_all(2)
		sb.border_color = SEL if selected else DIM
		sb.set_corner_radius_all(3)
		_slots[i].add_theme_stylebox_override("panel", sb)

func _input(event: InputEvent) -> void:
	for i in 5:
		if event.is_action_pressed("slot_%d" % (i + 1)):
			_selected = i
			_refresh_slots()

func _process(delta: float) -> void:
	var want := 1.0 if _any_interactable_inside() else 0.0
	_prompt_a = lerpf(_prompt_a, want, minf(1.0, delta * 8.0))
	if _prompt:
		_prompt.modulate.a = _prompt_a

func _any_interactable_inside() -> bool:
	for it in get_tree().get_nodes_in_group("interactable"):
		if "_inside" in it and it._inside:
			return true
	return false
