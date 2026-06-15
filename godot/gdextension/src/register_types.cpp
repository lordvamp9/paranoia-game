// PARANOIA GDExtension — registers native classes with Godot.
// Phase 1: skeleton (no classes yet). Phase 3 wires in ParanoiaSynth, WorldGen.
#include "register_types.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include "paranoia_synth.h"    // AudioSynth — procedural audio (port of src/audio.js)
// #include "world_gen.h"        // (Phase 2) procedural gen + collision  <- world.h/house.h

using namespace godot;

void initialize_paranoia_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) return;
    GDREGISTER_CLASS(AudioSynth);
    // GDREGISTER_CLASS(WorldGen);
}

void uninitialize_paranoia_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) return;
}

extern "C" {
// Entry symbol referenced by paranoia.gdextension.
GDExtensionBool GDE_EXPORT paranoia_library_init(
        GDExtensionInterfaceGetProcAddress p_get_proc_address,
        GDExtensionClassLibraryPtr p_library,
        GDExtensionInitialization *r_initialization) {
    GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);
    init_obj.register_initializer(initialize_paranoia_module);
    init_obj.register_terminator(uninitialize_paranoia_module);
    init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);
    return init_obj.init();
}
}
