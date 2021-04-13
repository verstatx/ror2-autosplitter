#include <stdint.h>
#include <stdbool.h>
#ifdef USE_WASI_SDK
#include <math.h>
#else
#define NAN (0.f/0.f)
#endif

#include "asl.h"

#define SCENE_NAME_LEN 16

#ifndef PACKED_STRINGS
static pointer_path_id scene_name_ppids[SCENE_NAME_LEN];
#else
static pointer_path_id scene_name_ppids[SCENE_NAME_LEN/sizeof(uint64_t)];
#endif
static pointer_path_id fade_ppid;
static pointer_path_id stage_count_ppid;
static pointer_path_id in_game_ppid;

#ifdef REMOVE_LOADS
static pointer_path_id load2_ppid;
#endif

typedef struct {
    char scene_name[SCENE_NAME_LEN];
    int32_t stage_count;
    bool in_game;
#ifdef REMOVE_LOADS
    //float fade; // TODO: not implemented
    bool is_loading;
#endif

    bool is_initialized;
} state_t;

static state_t CURRENT = {
    .is_initialized = false
};

static state_t OLD = {
    .is_initialized = false
};

#ifndef PACKED_STRINGS
// Get string from array of pointer_path_ids of type U8
void str_from_ppids(pointer_path_id* ppids, char* out, int len, bool is_current) {
    for (int i = 0; i < len; i++) {
        out[i] = get_u8(ppids[i], is_current);
    }
}
#else
inline void unpack_string(uint64_t packed, char* out, int num) {
    if (num > sizeof(uint64_t))
        num = sizeof(uint64_t);
    for (int i = 0; i < num; i++) {
        out[i] = (packed >> 8 * i) & 0xFF;
    }
}

// Get string from array of pointer_path_ids of type U64
// len = expected length of string
void str_from_ppids(pointer_path_id* ppids, char* out, int len, bool is_current) {
    for (int i = 0; i < len; i+=sizeof(uint64_t)) {
        unpack_string(get_u64(ppids[i/sizeof(uint64_t)], is_current), &out[i], len - i);
    }
}
#endif

bool str_is_equal(const char* l, const char* r) {
    int i = 0;
    for (; l[i] != '\0' && r[i] != '\0'; i++) {
        if (l[i] != r[i])
            return false;
    }
    return l[i] == r[i];
}

#ifdef DEBUG_OUTPUT
void print_scene(const state_t* s) {
    unsigned int len = 0;
    while (s->scene_name[len] != '\0') {
        ++len;
    }

    print_message(s->scene_name, len);
    if (len == 0)
        print_message("WARNING: null scene name", 24); // NOTE: never occured during testing
}

// Assumes `out` size >= 2
void itox(uint8_t i, char* out) {
    static const char hex[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    int high_nibble = (i >> 4) & 0xF;
    int low_nibble = i & 0xF;

    out[0] = hex[high_nibble];
    out[1] = hex[low_nibble];
}
#endif

/* Note: configure cannot access any values from any pointer path.
 * livesplit-core internally runs update_values() only after it has
 * been configured. This makes version detection impossible at runtime until
 * an explicit way is added to the API.
 */
void configure() {
    set_process_name("Risk of Rain 2.", 15); // "exe" is stripped off for some reason
    set_tick_rate(120.f);

    // Risk of Rain 2 - version 1.0.0 memory locations
    // String not implemented yet
    //int id = push_pointer_path("UnityPlayer.dll", 15, String);

    // this pointer path does not work in v1.0.0.5
    /*
    fade_ppid = push_pointer_path("UnityPlayer.dll", 15, F32);
    push_offset(fade_ppid, 0x15637F8);
    push_offset(fade_ppid, 0x128);
    push_offset(fade_ppid, 0xD0);
    push_offset(fade_ppid, 0x18);
    push_offset(fade_ppid, 0xD0);
    push_offset(fade_ppid, 0x38);
    push_offset(fade_ppid, 0x60);
    push_offset(fade_ppid, 0xC);
    */

    // NOTE: some initial offsets are adjusted due to duplicate /proc/pid/maps file entries
    // livesplit-core uses the last entry, but the ASL script uses the first.
    stage_count_ppid = push_pointer_path("mono-2.0-bdwgc.dll", 18, I32);
    push_offset(stage_count_ppid, 0x491DC8 - 0x2AF000);
    push_offset(stage_count_ppid, 0x28);
    push_offset(stage_count_ppid, 0x50);
#ifndef V1_0_3_1
    push_offset(stage_count_ppid, 0x6B0);
#else
    push_offset(stage_count_ppid, 0x660);
#endif


    in_game_ppid = push_pointer_path("AkSoundEngine.dll", 17, I32);
    push_offset(in_game_ppid, 0x20DC04 - 0x7000);

#ifndef PACKED_STRINGS
    for (int i = 0; i < SCENE_NAME_LEN; i++) {
        pointer_path_id scene_name_ppid = push_pointer_path("UnityPlayer.dll", 15, U8);
        push_offset(scene_name_ppid, 0x15A95D8);
        push_offset(scene_name_ppid, 0x28);
        push_offset(scene_name_ppid, 0x00);
        push_offset(scene_name_ppid, 0x40 + i);
        scene_name_ppids[i] = scene_name_ppid;
    }
#else
    for (int i = 0; i < SCENE_NAME_LEN; i+=sizeof(uint64_t)) {
        pointer_path_id scene_name_ppid = push_pointer_path("UnityPlayer.dll", 15, U64);
        push_offset(scene_name_ppid, 0x15A95D8);
        push_offset(scene_name_ppid, 0x28);
        push_offset(scene_name_ppid, 0x00);
        push_offset(scene_name_ppid, 0x40 + i);
        scene_name_ppids[i / sizeof(uint64_t)] = scene_name_ppid;
    }
#endif

#ifdef REMOVE_LOADS
    load2_ppid = push_pointer_path("mono-2.0-bdwgc.dll", 18, I32);
    push_offset(load2_ppid, 0x491DC8 - 0x2AF000);
    push_offset(load2_ppid, 0x58);
    push_offset(load2_ppid, 0x160);
    push_offset(load2_ppid, 0x160);
    push_offset(load2_ppid, 0x160);
    push_offset(load2_ppid, 0x160);
    push_offset(load2_ppid, 0x160);
    push_offset(load2_ppid, 0xBF0);
#endif

}

/* Note: we cannot rely on livesplit-core to track the old state properly
 * instead, we cache the old state ourselves but only if CURRENT has been
 * initialized in order to avoid unexpected behaviour due to some assumed
 * initial state. (eg. timer was started while in-game, but we assume a
 * default value of not in game, therefore new != old, hence start a run)
 *
 * livesplit-core constantly unhooks whenever a pointer path is incorrect
 * in doing so, any old state is cleared, so it effectively becomes the same
 * as current. In our case, scene_name seems to be the culprit.
 */
void update() {
    // Cache OLD state
    if (!CURRENT.is_initialized) {
        CURRENT.is_initialized = true;
        OLD.is_initialized = true;

        str_from_ppids(scene_name_ppids, OLD.scene_name, SCENE_NAME_LEN, false);
        OLD.stage_count = get_i32(stage_count_ppid, false);
        OLD.in_game = (get_i32(in_game_ppid, false) != 0);
#ifdef REMOVE_LOADS
        OLD.is_loading = (get_i32(load2_ppid, false) == 1);
#endif
    } else {
        for (int i = 0; i < SCENE_NAME_LEN; i++) {
            OLD.scene_name[i] = CURRENT.scene_name[i];
        }
        OLD.stage_count = CURRENT.stage_count;
        OLD.in_game = CURRENT.in_game;
#ifdef REMOVE_LOADS
        OLD.is_loading = CURRENT.is_loading;
#endif
    }

    // Update CURRENT state
    str_from_ppids(scene_name_ppids, CURRENT.scene_name, SCENE_NAME_LEN, true);
    CURRENT.stage_count = get_i32(stage_count_ppid, true);
    CURRENT.in_game = (get_i32(in_game_ppid, true) != 0);
#ifdef REMOVE_LOADS
    CURRENT.is_loading = (get_i32(load2_ppid, true) == 1);
#endif

#ifdef DEBUG_OUTPUT
    // Debug output
    if (!str_is_equal(CURRENT.scene_name, OLD.scene_name)
       || CURRENT.in_game != OLD.in_game
       || CURRENT.stage_count != OLD.stage_count
       || CURRENT.is_loading != OLD.is_loading) {
        print_scene(&OLD);
        print_scene(&CURRENT);

        char in_game[4] = "0x";
        for (int i = 0; i < 1; i++) {
            itox(((uint32_t)OLD.in_game >> 8*i) & 0xFF, &in_game[2+(2*i)]);
        }
        print_message(in_game, 4);
        for (int i = 0; i < 1; i++) {
            itox(((uint32_t)CURRENT.in_game >> 8*i) & 0xFF, &in_game[2+(2*i)]);
        }
        print_message(in_game, 4);

        char stage_count[10] = "0x";
        for (int i = 0; i < 4; i++) {
            itox(((uint32_t)OLD.stage_count >> 8*i) & 0xFF, &stage_count[2+(2*i)]);
        }
        print_message(stage_count, 10);
        for (int i = 0; i < 4; i++) {
            itox(((uint32_t)CURRENT.stage_count >> 8*i) & 0xFF, &stage_count[2+(2*i)]);
        }
        print_message(stage_count, 10);
    }
#endif
}

void hooked() {
}

void unhooked() {
}

bool should_start() {
    return (CURRENT.in_game && !OLD.in_game);
}

bool should_split() {
    if (CURRENT.stage_count > 1 && CURRENT.stage_count == OLD.stage_count + 1) {
        return true;
    }

    if (!str_is_equal(CURRENT.scene_name, OLD.scene_name)
        && !str_is_equal(CURRENT.scene_name, "splash")
        && !str_is_equal(CURRENT.scene_name, "title")
        && !str_is_equal(CURRENT.scene_name, "lobby")
        && !str_is_equal(CURRENT.scene_name, "crystalworld"))
    {
        return str_is_equal(OLD.scene_name, "bazaar");
    }

    return false;
}

/* This logic was modified from the base ASL script
 * should_reset is always called after should_start, so should_start would be
 * called too late to detect the start condition
 *
 * FIXME: This is unreliable when resetting too quickly. Some pointer path
 * becomes invalid for a bit, so the states don't get updated while in the
 * lobby before livesplit-core re-hooks to the process.
 */
bool should_reset() {
    return (!CURRENT.in_game && OLD.in_game && OLD.stage_count < 4);
}

bool is_loading() {
#ifdef REMOVE_LOADS
    return CURRENT.is_loading;
#else
    return false;
#endif
}

// sets the game time each tick
// returning NaN makes this ignored
double game_time() {
    return NAN;
    //return 42.f;
}
