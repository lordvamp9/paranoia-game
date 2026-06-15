# PARANOIA — main menu. Black, flickering pixel title, tagline, BEGIN.
# (Port of the #menu block in index.html.) No version number — clean.
extends Control

signal begin_pressed

var _title: Label
var _begin: Button
var _t := 0.0

func _ready() -> void:
	set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(UiUtil.bg(Color.BLACK))

	var center := CenterContainer.new()
	center.set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(center)
	var vb := VBoxContainer.new()
	vb.alignment = BoxContainer.ALIGNMENT_CENTER
	vb.add_theme_constant_override("separation", 18)
	center.add_child(vb)

	_title = UiUtil.label("PARANOIA", 56, Color(0.72, 0.72, 0.75), UiUtil.pixel_font())
	vb.add_child(_title)

	var tag := UiUtil.label("You are being observed.", 13, Color(0.30, 0.30, 0.34), UiUtil.mono_font())
	vb.add_child(tag)

	var spacer := Control.new(); spacer.custom_minimum_size = Vector2(0, 40)
	vb.add_child(spacer)

	_begin = Button.new()
	_begin.text = "BEGIN"
	_begin.flat = true
	_begin.add_theme_font_override("font", UiUtil.pixel_font())
	_begin.add_theme_font_size_override("font_size", 18)
	_begin.add_theme_color_override("font_color", Color(0.6, 0.6, 0.64))
	_begin.add_theme_color_override("font_hover_color", Color(0.92, 0.92, 0.95))
	_begin.add_theme_color_override("font_focus_color", Color(0.92, 0.92, 0.95))
	_begin.custom_minimum_size = Vector2(280, 56)
	_begin.pressed.connect(func(): begin_pressed.emit())
	vb.add_child(_begin)

	var brand := UiUtil.label("vamp9", 10, Color(0.18, 0.18, 0.18), UiUtil.mono_font())
	brand.set_anchors_preset(Control.PRESET_BOTTOM_WIDE)
	brand.position.y = -28
	add_child(brand)

func _process(delta: float) -> void:
	# subtle title flicker (index.html @keyframes flicker)
	_t += delta
	var c := fmod(_t, 4.0)
	var a := 1.0
	if c > 3.76 and c < 3.80: a = 0.4
	elif c > 3.88 and c < 3.92: a = 0.2
	if _title:
		_title.modulate.a = a

func focus_begin() -> void:
	if _begin:
		_begin.grab_focus()
