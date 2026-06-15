# PARANOIA — phone screen drawer (CanvasItem in the phone's SubViewport).
# Port of Phone._drawScreen() in src/phone.js: camera-feed look + battery,
# clock (distorts at high stress), objective, REC, WATCHING, glitch blocks,
# low-battery warning. phone.gd pushes state here and calls queue_redraw ~12 Hz.
extends Control

const W := 320.0
const H := 640.0

# leet substitution table (phone.js CORRUPT)
const CORRUPT := {
	"a": "4", "e": "3", "i": "1", "o": "0", "s": "5", "t": "7",
	"A": "4", "E": "3", "I": "1", "O": "0", "S": "5", "T": "7",
}

var battery := 100.0
var stress := 0.0
var objective := ""
var message := ""
var message_t := 0.0
var t := 0.0          # wall-ish time for blink phases
var rec_time := 0.0   # seconds since boot (REC timecode)

var _font: Font
var _rng := RandomNumberGenerator.new()

func _ready() -> void:
	_font = ThemeDB.fallback_font
	size = Vector2(W, H)
	_rng.randomize()

func _draw() -> void:
	var s := stress / 100.0
	if battery <= 0.0:
		draw_rect(Rect2(0, 0, W, H), Color.BLACK)
		return

	# camera-feed: very dark viewport + subtle noise
	draw_rect(Rect2(0, 0, W, H), Color(0.016, 0.027, 0.039))
	for i in 40:
		draw_rect(Rect2(_rng.randf() * W, _rng.randf() * H, 2, 2), Color(1, 1, 1, 0.022))

	# --- battery (top-left) ---
	var bat := int(ceil(battery))
	var bat_col := Color(0.6, 0.875, 0.6)
	if bat < 20:
		bat_col = Color(1.0, 0.23, 0.19) if sin(t * 8.0) > 0.0 else Color(0.48, 0.11, 0.094)
	draw_string(_font, Vector2(12, 30), "%d%%" % bat, HORIZONTAL_ALIGNMENT_LEFT, -1, 20, bat_col)
	draw_rect(Rect2(70, 14, 38, 19), bat_col, false, 1.5)
	draw_rect(Rect2(73, 17, 32 * (bat / 100.0), 13), bat_col)
	draw_rect(Rect2(108, 19, 4, 9), bat_col)

	# --- signal bars (top-right) — always weak ---
	for i in 4:
		var bx := 250.0 + i * 12.0
		var bh := 6.0 + i * 5.0
		var lit := i < 2                      # only 2 bars ever
		var col := Color(0.6, 0.67, 0.78, 0.9 if lit else 0.18)
		draw_rect(Rect2(bx, 34 - bh, 8, bh), col)

	# --- clock (top center) — wrong at high stress ---
	var hh := "??" if s > 0.7 else "03"
	var mm := "??" if s > 0.7 else "%02d" % (33 + (int(t / 60.0) % 27))
	draw_string(_font, Vector2(W / 2 - 26, 30), "%s:%s" % [hh, mm],
			HORIZONTAL_ALIGNMENT_LEFT, -1, 18, Color(0.4, 0.4, 0.47))

	# --- objective (center) ---
	if objective != "":
		var txt := _corrupt(objective.to_upper())
		draw_string(_font, Vector2(0, 130), txt, HORIZONTAL_ALIGNMENT_CENTER, W, 17,
				Color(0.86, 0.86, 0.9, 0.85 - s * 0.3))

	# --- transient message ---
	if message_t > 0.0 and sin(t * 6.0) > -0.4:
		draw_string(_font, Vector2(0, H / 2), _corrupt(message), HORIZONTAL_ALIGNMENT_CENTER, W, 28,
				Color(0.92, 0.9, 0.88, 0.95))

	# --- WATCHING under stress ---
	if s > 0.45 and _rng.randf() < 0.4:
		draw_string(_font, Vector2(0, 200), "· WATCHING ·", HORIZONTAL_ALIGNMENT_CENTER, W, 15,
				Color(0.78, 0.24, 0.22, clampf((s - 0.45) * 1.2, 0, 1)))

	# --- glitch blocks (random colored rects) when stress > 0.4 ---
	if s > 0.4:
		var n := int((s - 0.4) * 14.0)
		for i in n:
			var gy := _rng.randf() * H
			var gh := 4.0 + _rng.randf() * 10.0
			var gc := Color(_rng.randf(), _rng.randf(), _rng.randf(), 0.10 + s * 0.12)
			draw_rect(Rect2((_rng.randf() - 0.5) * 30.0 * s, gy, W, gh), gc)
		if _rng.randf() < s * 0.15:
			var band := Color(1, 1, 1, 0.08) if _rng.randf() < 0.5 else Color(1, 0.16, 0.16, 0.08)
			draw_rect(Rect2(0, _rng.randf() * H, W, 10 + _rng.randf() * 40), band)

	# --- REC (bottom-left), blinking ---
	var rec_on := sin(t * 3.5) > 0.0
	draw_circle(Vector2(24, H - 36), 7, Color(1, 0.23, 0.19) if rec_on else Color(1, 0.23, 0.19, 0.25))
	draw_string(_font, Vector2(40, H - 30), "REC", HORIZONTAL_ALIGNMENT_LEFT, -1, 16, Color(0.73, 0.73, 0.73))
	var rt := int(rec_time)
	var tc := "%02d:%02d:%02d" % [(rt / 3600) % 100, (rt / 60) % 60, rt % 60]
	draw_string(_font, Vector2(96, H - 30), tc, HORIZONTAL_ALIGNMENT_LEFT, -1, 13, Color(0.33, 0.33, 0.33))

	# --- low-battery warning + red bar (battery < 20%) ---
	if bat < 20:
		if sin(t * 5.0) > 0.0:
			draw_string(_font, Vector2(0, H - 80), "LOW BATTERY", HORIZONTAL_ALIGNMENT_CENTER, W, 18,
					Color(1, 0.23, 0.19))
		draw_rect(Rect2(0, H - 8, W, 8), Color(0.8, 0.12, 0.10, 0.6 + 0.4 * absf(sin(t * 5.0))))

func _corrupt(text: String) -> String:
	var lvl := stress / 100.0
	if lvl < 0.3:
		return text
	var out := ""
	for ch in text:
		var c := ch
		if _rng.randf() < (lvl - 0.3) * 0.9 and CORRUPT.has(ch):
			c = CORRUPT[ch]
		out += c
	return out
