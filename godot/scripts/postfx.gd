# PARANOIA — PostFX autoload: drives the stress-based post-processing shader.
# Port of the PostFX class in src/postfx.js. A fullscreen ColorRect on a
# CanvasLayer samples the rendered frame (hint_screen_texture) and runs
# post.gdshader; this autoload decays the glitch/flash values and pushes
# uniforms each frame (stress from Director, battery from the phone).
#
# (CompositorEffect would require a RenderingDevice compute pass; this canvas
# screen-shader achieves the identical effect more simply and reliably.)
extends Node

var glitch := 0.0
var flash_white_amt := 0.0
var flash_black_amt := 0.0
var enabled := true

var _layer: CanvasLayer
var _rect: ColorRect
var _mat: ShaderMaterial
var _time := 0.0

func _ready() -> void:
	_layer = CanvasLayer.new()
	_layer.layer = 5                       # above the 3D scene, below Director HUD (10)
	add_child(_layer)
	_rect = ColorRect.new()
	_rect.set_anchors_preset(Control.PRESET_FULL_RECT)
	_rect.mouse_filter = Control.MOUSE_FILTER_IGNORE
	var sh: Shader = load("res://shaders/post.gdshader")
	if sh:
		_mat = ShaderMaterial.new()
		_mat.shader = sh
		_rect.material = _mat
	else:
		push_warning("[PostFX] post.gdshader missing — post-processing disabled")
	_layer.add_child(_rect)

# --- scripted spikes (Director calls these) ---
func spike_glitch(amount := 1.0) -> void:
	glitch = minf(1.0, glitch + amount)

func flash_white() -> void:
	flash_white_amt = 1.0

func flash_black() -> void:
	flash_black_amt = 1.0

func _process(dt: float) -> void:
	_time += dt
	# decay rates from postfx.js
	glitch = maxf(0.0, glitch - dt * 1.8)
	flash_white_amt = maxf(0.0, flash_white_amt - dt * 0.7)
	flash_black_amt = maxf(0.0, flash_black_amt - dt * 1.2)
	if _mat == null or not enabled:
		return

	var stress: float = Director.stress
	var battery := 1.0
	var phone := get_tree().get_first_node_in_group("phone")
	if phone:
		battery = phone.battery / 100.0
	# above 80 stress, baseline glitch creeps in (postfx.js render())
	var glitch_u: float = glitch + (maxf(0.0, stress - 80.0) / 100.0)

	_mat.set_shader_parameter("u_time", _time)
	_mat.set_shader_parameter("u_stress", stress / 100.0)
	_mat.set_shader_parameter("u_glitch", glitch_u)
	_mat.set_shader_parameter("u_flash_white", flash_white_amt)
	_mat.set_shader_parameter("u_flash_black", flash_black_amt)
	_mat.set_shader_parameter("u_battery", battery)
