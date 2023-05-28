#include <stdint.h>
#include <stdbool.h>
#ifdef USE_WASI_SDK
#include <math.h>
#else
#define NAN (0.f/0.f)
#endif

#include "env.h"
#include "util.h"

#define SCENE_NAME_LEN 16

typedef struct {
    char scene_name[SCENE_NAME_LEN];
    int32_t stage_count;
//    bool in_game;
    float fade;

    TimerState timer_state;
    ProcessId process_id;
} state_t;

static state_t CURRENT = {
    .process_id = 0
};

static state_t OLD = {
    .process_id = 0
};

//const Address in_game_offsets[] = { 0x20DC04 };
#ifndef V1_1
const Address scene_name_offsets[] = { 0x15A95D8, 0x28, 0x00, 0x40 };
const Address fade_offsets[] = { 0x4940B8, 0x10, 0x1D0, 0x8, 0x4E0, 0x1E10, 0xD0, 0x8, 0x60, 0xC };
#else
const Address scene_name_offsets[] = { 0x15A95D8, 0x48, 0x40 };
const Address fade_offsets[] = { 0x4940B8, 0x10, 0x1D0, 0x8, 0x4E0, 0x1E88, 0x108, 0xD0, 0x8, 0x60, 0xC };
#endif
const Address stage_count_offsets[] = { 0x491DC8, 0x28, 0x50, 0x6B0 };

struct {
    PointerPath scene_name;
    PointerPath fade;
    PointerPath stage_count;
} PPIDs = {
    .scene_name = {
        .module_name = "UnityPlayer.dll",
        .num_offsets = sizeof(scene_name_offsets) / sizeof(Address),
        .offsets = scene_name_offsets

    },
    .fade = {
        .module_name = "mono-2.0-bdwgc.dll",
        .num_offsets = sizeof(fade_offsets) / sizeof(Address),
        .offsets = fade_offsets
    },
    .stage_count = {
        .module_name = "mono-2.0-bdwgc.dll",
        .num_offsets = sizeof(stage_count_offsets) / sizeof(Address),
        .offsets = stage_count_offsets
    },
};

void detach() {
    process_detach(CURRENT.process_id);
    CURRENT.process_id = 0;
    DEBUG_PRINT("Detaching from proccess.");
}

void attach() {
    ProcessId pid = process_attach("Risk of Rain 2.", 15); // "exe" is stripped off for some reason
    CURRENT.process_id = pid;
}

// Runs once when the script is loaded.
void init() {
    static bool is_initialized = false;

    if (is_initialized) {
        return;
    }

    runtime_set_tick_rate(120.0);

    is_initialized = true;
    DEBUG_PRINT("Initialized.");
}

bool should_start();
bool should_split();
bool should_reset();
bool is_loading(bool);

void update() {
    static bool was_loading = false;
    init();

    if (!CURRENT.process_id) {
        attach();
    }

    if (CURRENT.process_id) {
        if (process_is_open(CURRENT.process_id)) {
#ifdef DEBUG_OUTPUT
            if (!OLD.process_id || CURRENT.process_id != OLD.process_id) {
                DEBUG_PRINT("Attached to process.");
                print_pid(CURRENT.process_id);

                DEBUG_PRINT("Immediate address of module `UnityPlayer.dll`:");
                NonZeroAddress up_dll = process_get_module_address(CURRENT.process_id, PPIDs.scene_name.module_name, strlen(PPIDs.scene_name.module_name));
                print_Address(up_dll);

                DEBUG_PRINT("Immediate address of `scene_name` (char* <= [16]):");
                NonZeroAddress addr_scene_name = try_resolve_pointer_path(CURRENT.process_id, PPIDs.scene_name);
                print_Address(addr_scene_name);

                DEBUG_PRINT("Immediate scene name:");
                if (addr_scene_name) {
                    char sn[SCENE_NAME_LEN] = "";
                    process_read(CURRENT.process_id, addr_scene_name, sn, SCENE_NAME_LEN);
                    runtime_print_message(sn, strlen(sn));
                } else {
                    DEBUG_PRINT("[unknown]");
                }

                DEBUG_PRINT("Immediate address of module `mono-2.0-bdwgc.dll`:");
                NonZeroAddress m20bdwgc_dll = process_get_module_address(CURRENT.process_id, PPIDs.fade.module_name, strlen(PPIDs.fade.module_name));
                print_Address(m20bdwgc_dll);

                DEBUG_PRINT("Immediate Address of `fade`:");
                NonZeroAddress addr_fade = try_resolve_pointer_path(CURRENT.process_id, PPIDs.fade);
                print_Address(addr_fade);

                DEBUG_PRINT("Immediate value of `fade` (f32):");
                read_f32 fade_raw;
                process_read(CURRENT.process_id, addr_fade, fade_raw.buf, sizeof(float));
                print_u64_hex(fade_raw.out_raw);

                DEBUG_PRINT("Immediate Address of `stage_count`:");
                NonZeroAddress addr_stage_count = try_resolve_pointer_path(CURRENT.process_id, PPIDs.stage_count);
                print_Address(addr_stage_count);

                DEBUG_PRINT("Immediate value of `stage_count` (i32):");
                read_i32 stage_count_raw;
                process_read(CURRENT.process_id, addr_stage_count, stage_count_raw.buf, sizeof(float));
                print_u64(stage_count_raw.out);

            }
#endif

            CURRENT.timer_state = timer_get_state();

            NonZeroAddress addr_scene_name = try_resolve_pointer_path(CURRENT.process_id, PPIDs.scene_name);
            if (addr_scene_name) {
                process_read(CURRENT.process_id, addr_scene_name, CURRENT.scene_name, SCENE_NAME_LEN);
            }

            NonZeroAddress addr_fade = try_resolve_pointer_path(CURRENT.process_id, PPIDs.fade);
            if (addr_fade) {
                read_f32 raw_fade;
                if (process_read(CURRENT.process_id, addr_fade, raw_fade.buf, sizeof(float))) {
                    CURRENT.fade = raw_fade.out;
                }
            }

            NonZeroAddress addr_stage_count = try_resolve_pointer_path(CURRENT.process_id, PPIDs.stage_count);
            if (addr_stage_count) {
                read_i32 raw_stage_count;
                if (process_read(CURRENT.process_id, addr_stage_count, raw_stage_count.buf, sizeof(int32_t))) {
                    CURRENT.stage_count = raw_stage_count.out;
                }
            }

            if (should_start()) {
                DEBUG_PRINT("Timer should start now.");
                timer_start();
                was_loading = true;
            }

            if (should_reset()) {
                DEBUG_PRINT("Timer should reset now.");
                timer_reset();
            }

            if (should_split()) {
                DEBUG_PRINT("Timer should split now.");
                timer_split();
            }

            if (is_loading(was_loading)) {
                if (!was_loading) {
                    DEBUG_PRINT("Game is loading.");
                    was_loading = true;
                }
                timer_pause_game_time();
            } else {
                if (was_loading) {
                    DEBUG_PRINT("Game is not loading.");
                    was_loading = false;
                }
                timer_resume_game_time();
            }

#ifdef DEBUG_OUTPUT
            if (!str_is_equal(CURRENT.scene_name, OLD.scene_name) || CURRENT.stage_count != OLD.stage_count) {
                DEBUG_PRINT("OLD/CURRENT `scene_name`:");
                runtime_print_message(OLD.scene_name, strlen(OLD.scene_name));
                runtime_print_message(CURRENT.scene_name, strlen(CURRENT.scene_name));

                DEBUG_PRINT("OLD/CURRENT `stage_count`:");
                print_u64(OLD.stage_count);
                print_u64(CURRENT.stage_count);

                DEBUG_PRINT("OLD/CURRENT `fade`:");
                print_u64_hex(((read_f32)OLD.fade).out_raw);
                print_u64_hex(((read_f32)CURRENT.fade).out_raw);

                DEBUG_PRINT("");
            }
#endif

            ///OLD = CURRENT;
            for (size_t i = 0; i < SCENE_NAME_LEN; ++i) {
                OLD.scene_name[i] = CURRENT.scene_name[i];
            }
            OLD.stage_count = CURRENT.stage_count;
            OLD.fade = CURRENT.fade;
            OLD.timer_state = CURRENT.timer_state;
            OLD.process_id = CURRENT.process_id;

        } else {
            detach();
        }
    }
}

bool should_start() {
    return (CURRENT.timer_state == NOT_RUNNING && str_is_equal(OLD.scene_name, "lobby") && (
               str_is_equal(CURRENT.scene_name, "golemplains")
            || str_is_equal(CURRENT.scene_name, "golemplains2")
            || str_is_equal(CURRENT.scene_name, "blackbeach")
            || str_is_equal(CURRENT.scene_name, "blackbeach2")
        ) && (
            CURRENT.fade < 1.f && OLD.fade >= 1.f
        )
    );
}

bool should_split() {
    if (CURRENT.timer_state == RUNNING) {
        if (CURRENT.stage_count > 1 && CURRENT.stage_count == OLD.stage_count + 1) {
            return true;
        }
#ifdef BAZAAR_SPLIT
        if (!str_is_equal(CURRENT.scene_name, OLD.scene_name)) {
            return str_is_equal(OLD.scene_name, "bazaar");
        }
#endif
    }

    return false;
}

bool should_reset() {
    return (CURRENT.timer_state == RUNNING && (
           str_is_equal(CURRENT.scene_name, "lobby")
        || str_is_equal(CURRENT.scene_name, "title")
        || str_is_equal(CURRENT.scene_name, "crystalworld")
        || str_is_equal(CURRENT.scene_name, "eclipseworld")
    ));
}

bool is_loading(bool last_known) {
    if (CURRENT.fade > OLD.fade) {
        return true;
    }
    if (CURRENT.fade < OLD.fade && CURRENT.fade > 0.f) {
        return false;
    }
    return last_known;
}

