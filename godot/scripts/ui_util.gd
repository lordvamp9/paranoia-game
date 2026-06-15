# PARANOIA — shared UI helpers (fonts + styled widgets). Pixel font is
# Press Start 2P (OFL) from assets/fonts/; falls back to the engine mono font.
class_name UiUtil
extends RefCounted

const PIXEL_PATH := "res://assets/fonts/PressStart2P-Regular.ttf"

static func pixel_font() -> Font:
	if ResourceLoader.exists(PIXEL_PATH):
		return load(PIXEL_PATH)
	return ThemeDB.fallback_font

static func mono_font() -> Font:
	# body text — a system monospace reads better than pixel font at small sizes
	var sf := SystemFont.new()
	sf.font_names = PackedStringArray(["Consolas", "Courier New", "monospace"])
	return sf

static func label(text: String, size: int, color: Color, font: Font = null,
		align := HORIZONTAL_ALIGNMENT_CENTER) -> Label:
	var l := Label.new()
	l.text = text
	l.add_theme_font_size_override("font_size", size)
	l.add_theme_color_override("font_color", color)
	l.add_theme_color_override("font_outline_color", Color.BLACK)
	l.add_theme_constant_override("outline_size", 4)
	if font:
		l.add_theme_font_override("font", font)
	l.horizontal_alignment = align
	l.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	l.mouse_filter = Control.MOUSE_FILTER_IGNORE
	return l

static func bg(color := Color.BLACK) -> ColorRect:
	var r := ColorRect.new()
	r.color = color
	r.set_anchors_preset(Control.PRESET_FULL_RECT)
	r.mouse_filter = Control.MOUSE_FILTER_IGNORE
	return r
