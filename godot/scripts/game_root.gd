# PARANOIA — top-level flow manager (Session 8). Drives the screen state machine
# MENU -> INTRO -> PLAY -> (SETTINGS pause) -> CREDITS -> MENU, instancing the
# gameplay (Main.tscn) for PLAY and overlaying the UI scenes.
extends Node

const MAIN_SCENE := "res://scenes/Main.tscn"
const MENU := preload("res://scenes/ui/MainMenu.tscn")
const INTRO := preload("res://scenes/ui/Intro.tscn")
const SETTINGS := preload("res://scenes/ui/Settings.tscn")
const HUD := preload("res://scenes/ui/HUD.tscn")
const CREDITS := preload("res://scenes/ui/Credits.tscn")

var _ui: CanvasLayer
var _world: Node3D
var _menu: Control
var _settings: Control
var _hud: Control
var _intro: Control
var _credits: Control
var _state := GameState.State.MENU

func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	_world = Node3D.new()
	add_child(_world)
	_ui = CanvasLayer.new()
	_ui.layer = 20                         # above PostFX(5) and Director HUD(10)
	_ui.process_mode = Node.PROCESS_MODE_ALWAYS
	add_child(_ui)

	Director.external_credits = true
	if not Director.ending_reached.is_connected(_on_ending):
		Director.ending_reached.connect(_on_ending)

	_show_menu()
	if "flowtest" in OS.get_cmdline_user_args():
		await _flow_test()

# ---------------------------------------------------------------- states
func _clear_ui() -> void:
	for c in _ui.get_children():
		c.queue_free()

func _clear_world() -> void:
	for c in _world.get_children():
		c.queue_free()

func _show_menu() -> void:
	_state = GameState.State.MENU
	GameState.set_state(GameState.State.MENU)
	get_tree().paused = false
	_clear_world()
	_clear_ui()
	_menu = MENU.instantiate()
	_menu.begin_pressed.connect(_begin)
	_ui.add_child(_menu)
	_menu.focus_begin()
	print("[Game] state -> MENU")

func _begin() -> void:
	if _state != GameState.State.MENU:
		return
	_state = GameState.State.INTRO
	GameState.set_state(GameState.State.INTRO)
	_clear_ui()
	_intro = INTRO.instantiate()
	_intro.finished.connect(_start_play)
	_ui.add_child(_intro)
	print("[Game] state -> INTRO")
	_intro.play()

func _start_play() -> void:
	if _state == GameState.State.PLAY:
		return
	_state = GameState.State.PLAY
	_clear_ui()
	# gameplay (Main.tscn brings World + House + Player + Phone + env + Director)
	var main: Node = load(MAIN_SCENE).instantiate()
	_world.add_child(main)
	_hud = HUD.instantiate()
	_ui.add_child(_hud)
	GameState.set_state(GameState.State.PLAY)
	print("[Game] state -> PLAY")

func _open_settings() -> void:
	if _state != GameState.State.PLAY:
		return
	_state = GameState.State.SETTINGS
	get_tree().paused = true
	GameState.set_state(GameState.State.SETTINGS)
	_settings = SETTINGS.instantiate()
	_settings.resume_pressed.connect(_resume)
	_settings.menu_pressed.connect(_show_menu)
	_ui.add_child(_settings)
	print("[Game] state -> SETTINGS (paused)")

func _resume() -> void:
	if _state != GameState.State.SETTINGS:
		return
	_state = GameState.State.PLAY
	if is_instance_valid(_settings):
		_settings.queue_free()
	get_tree().paused = false
	GameState.set_state(GameState.State.PLAY)
	print("[Game] state -> PLAY (resumed)")

func _on_ending(which: String) -> void:
	_state = GameState.State.CREDITS
	# let the ending flash/sound play briefly, then roll credits
	await get_tree().create_timer(2.5, true).timeout
	GameState.set_state(GameState.State.CREDITS)
	_clear_ui()
	_credits = CREDITS.instantiate()
	_credits.finished.connect(_show_menu)
	_ui.add_child(_credits)
	print("[Game] state -> CREDITS (%s)" % which)
	_credits.play("You made your choice.")

func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("pause"):
		if _state == GameState.State.PLAY:
			_open_settings()
		elif _state == GameState.State.SETTINGS:
			_resume()

# ---------------------------------------------------------------- flow test
func _flow_test() -> void:
	await _frames(3)
	await _shot("session8_menu.png")
	print("[Game] flowtest: menu shown")

	_begin()
	await _frames(40)                      # let the first intro line fade in
	await _shot("session8_intro.png")
	print("[Game] flowtest: intro shown")

	_start_play()                          # skip rest of intro
	await _frames(25)
	await _shot("session8_hud.png")
	print("[Game] flowtest: HUD/gameplay shown")

	_open_settings()
	await _frames(10)
	await _shot("session8_settings.png")
	print("[Game] flowtest: settings shown (sens=%.2f vol=%.2f)" % [Config.mouse_sens, Config.vol_master])

	_resume()
	await _frames(5)

	# reach an ending -> credits
	Director._ending("trust")
	await _frames(4)
	await get_tree().create_timer(3.0, true).timeout
	await _frames(40)
	await _shot("session8_credits.png")
	print("[Game] flowtest: credits shown, state=%d" % _state)
	print("[Game] flowtest DONE")
	get_tree().quit(0)

func _frames(n: int) -> void:
	for i in n:
		await get_tree().process_frame

func _shot(name: String) -> void:
	if DisplayServer.get_name() != "headless":
		await RenderingServer.frame_post_draw
		await RenderingServer.frame_post_draw
		var img := get_viewport().get_texture().get_image()
		img.save_png("user://" + name)          # writable in the exported build
		if OS.has_feature("editor"):
			img.save_png("res://" + name)
		print("[Game] screenshot -> %s" % name)
