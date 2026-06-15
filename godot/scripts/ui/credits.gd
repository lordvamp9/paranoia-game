# PARANOIA — end credits: black, slow upward scroll, then back to the menu.
extends Control

signal finished

var _label: Label

func _ready() -> void:
	set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(UiUtil.bg(Color.BLACK))
	_label = UiUtil.label("", 18, Color(0.78, 0.78, 0.8), UiUtil.mono_font())
	_label.size = Vector2(800, 600)
	_label.position = Vector2(get_viewport_rect().size.x / 2 - 400, get_viewport_rect().size.y)
	add_child(_label)

func play(blurb := "You made your choice.") -> void:
	_run(blurb)

func _run(blurb: String) -> void:
	var title := UiUtil.label("PARANOIA", 40, Color(0.85, 0.83, 0.81), UiUtil.pixel_font())
	# build the scroll text with the pixel title rendered separately on top
	_label.add_theme_font_override("font", UiUtil.mono_font())
	_label.text = "PARANOIA\n\n\n\nA game by vamp9\n\n\n\n\n\n%s" % blurb
	title.queue_free()  # (kept simple: mono title in the body)

	var vh := get_viewport_rect().size.y
	_label.position.y = vh + 20
	var tw := create_tween()
	tw.tween_property(_label, "position:y", -650, 14.0)
	await tw.finished
	var fade := create_tween()
	fade.tween_property(self, "modulate:a", 0.0, 1.2)
	await fade.finished
	finished.emit()
