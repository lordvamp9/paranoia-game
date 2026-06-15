# PARANOIA — intro sequence: fade-in lines on black, then hand off to gameplay.
extends Control

signal finished

const LINES := [
	"Headlights. A curve you took too fast.",
	"The forest was not.",
]

var _label: Label

func _ready() -> void:
	set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(UiUtil.bg(Color.BLACK))
	_label = UiUtil.label("", 18, Color(0.82, 0.80, 0.78), UiUtil.mono_font())
	_label.set_anchors_preset(Control.PRESET_FULL_RECT)
	_label.autowrap_mode = TextServer.AUTOWRAP_WORD
	add_child(_label)

func play() -> void:
	_run()

func _run() -> void:
	for line in LINES:
		_label.text = line
		_label.modulate.a = 0.0
		await _fade(_label, 1.0, 1.2)
		await get_tree().create_timer(2.0).timeout
		await _fade(_label, 0.0, 1.0)
	finished.emit()

func _fade(node: CanvasItem, target: float, dur: float) -> void:
	var tw := create_tween()
	tw.tween_property(node, "modulate:a", target, dur)
	await tw.finished
