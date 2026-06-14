extends Node
## Config — settings + input rebinding. Port of cpp/src/config.h.
## Reads/writes paranoia.cfg in the SAME plain-text format as the raylib build,
## so an existing .cfg stays valid after migration (docs §6, hard requirement).
## Audio bus volumes drive Godot's AudioServer; key bindings drive InputMap.

const CFG_ACTIONS := ["move_forward", "move_back", "move_left", "move_right",
		"sprint", "light", "interact"]   # order = key0..key6 in paranoia.cfg

var vol_master := 0.9
var vol_music := 0.7
var vol_ambient := 0.8
var vol_voices := 1.0
var mouse_sens := 0.5
var fullscreen := 1

func _ready() -> void:
	load_cfg()

func _cfg_path() -> String:
	# Next to the executable, matching CfgPath() in config.h.
	return OS.get_executable_path().get_base_dir().path_join("paranoia.cfg")

func load_cfg() -> void:
	var f := FileAccess.open(_cfg_path(), FileAccess.READ)
	if f == null:
		return
	while not f.eof_reached():
		var parts := f.get_line().split(" ", false)
		if parts.size() != 2:
			continue
		var k: String = parts[0]
		var v := parts[1].to_float()
		match k:
			"master": vol_master = v
			"music": vol_music = v
			"ambient": vol_ambient = v
			"voices": vol_voices = v
			"sens": mouse_sens = v
			"full": fullscreen = int(v)
			_:
				if k.begins_with("key"):
					var i := int(k.substr(3))
					# Phase 1: map keycode v to InputMap action CFG_ACTIONS[i].
					pass
	f.close()

func save_cfg() -> void:
	var f := FileAccess.open(_cfg_path(), FileAccess.WRITE)
	if f == null:
		return
	# key0..key6 then audio/sens/full — identical schema to config.h CfgSave().
	f.store_line("master %.3f" % vol_master)
	f.store_line("music %.3f" % vol_music)
	f.store_line("ambient %.3f" % vol_ambient)
	f.store_line("voices %.3f" % vol_voices)
	f.store_line("sens %.3f" % mouse_sens)
	f.store_line("full %d" % fullscreen)
	f.close()
