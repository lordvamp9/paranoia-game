# PARANOIA — pause / settings overlay (ESC during play). Full controls list
# (no cutoff), mouse-sensitivity + volume sliders persisting to paranoia.cfg.
extends Control

signal resume_pressed
signal menu_pressed

const CONTROLS := [
	"WASD   — Move",
	"Mouse  — Look",
	"Shift  — Sprint",
	"F      — Flashlight",
	"E      — Interact",
	"ESC    — Pause",
	"1      — Phone",
]

func _ready() -> void:
	set_anchors_preset(Control.PRESET_FULL_RECT)
	var dim := UiUtil.bg(Color(0, 0, 0, 0.86))
	dim.mouse_filter = Control.MOUSE_FILTER_STOP
	add_child(dim)

	var center := CenterContainer.new()
	center.set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(center)
	var vb := VBoxContainer.new()
	vb.alignment = BoxContainer.ALIGNMENT_CENTER
	vb.add_theme_constant_override("separation", 14)
	vb.custom_minimum_size = Vector2(520, 0)
	center.add_child(vb)

	vb.add_child(UiUtil.label("PAUSED", 28, Color(0.8, 0.8, 0.84), UiUtil.pixel_font()))
	vb.add_child(_spacer(10))

	var resume := _btn("RESUME")
	resume.pressed.connect(func(): resume_pressed.emit())
	vb.add_child(resume)

	vb.add_child(_spacer(8))
	vb.add_child(UiUtil.label("CONTROLS", 13, Color(0.5, 0.5, 0.54), UiUtil.mono_font()))
	var controls := UiUtil.label("\n".join(CONTROLS), 15, Color(0.62, 0.62, 0.66), UiUtil.mono_font())
	vb.add_child(controls)
	vb.add_child(_spacer(8))

	# mouse sensitivity
	vb.add_child(UiUtil.label("MOUSE SENSITIVITY", 12, Color(0.5, 0.5, 0.54), UiUtil.mono_font()))
	var sens := HSlider.new()
	sens.min_value = 0.1; sens.max_value = 2.0; sens.step = 0.05
	sens.value = Config.mouse_sens
	sens.custom_minimum_size = Vector2(360, 20)
	sens.value_changed.connect(_on_sens)
	vb.add_child(sens)

	# master volume
	vb.add_child(UiUtil.label("VOLUME", 12, Color(0.5, 0.5, 0.54), UiUtil.mono_font()))
	var vol := HSlider.new()
	vol.min_value = 0.0; vol.max_value = 1.0; vol.step = 0.05
	vol.value = Config.vol_master
	vol.custom_minimum_size = Vector2(360, 20)
	vol.value_changed.connect(_on_volume)
	vb.add_child(vol)

	vb.add_child(_spacer(12))
	var back := _btn("BACK TO MENU")
	back.pressed.connect(func(): menu_pressed.emit())
	vb.add_child(back)

func _on_sens(v: float) -> void:
	Config.mouse_sens = v
	Config.save_cfg()

func _on_volume(v: float) -> void:
	Config.vol_master = v
	var bus := AudioServer.get_bus_index("Master")
	AudioServer.set_bus_volume_db(bus, linear_to_db(maxf(0.0001, v)))
	Config.save_cfg()

func _btn(text: String) -> Button:
	var b := Button.new()
	b.text = text
	b.flat = true
	b.add_theme_font_override("font", UiUtil.pixel_font())
	b.add_theme_font_size_override("font_size", 15)
	b.add_theme_color_override("font_color", Color(0.6, 0.6, 0.64))
	b.add_theme_color_override("font_hover_color", Color(0.92, 0.92, 0.95))
	b.custom_minimum_size = Vector2(360, 44)
	return b

func _spacer(h: int) -> Control:
	var c := Control.new(); c.custom_minimum_size = Vector2(0, h)
	return c
