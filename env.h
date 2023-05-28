#pragma once
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>

typedef size_t usize;
typedef uint64_t NonZeroU64;

typedef uint64_t Address;
typedef NonZeroU64 NonZeroAddress;
typedef NonZeroU64 ProcessId;
typedef enum TimerState : uint32_t {
    NOT_RUNNING = 0,
    RUNNING = 1,
    PAUSED = 2,
    ENDED = 3,
} TimerState;

/// NOTE: All strings are assumed to NOT be null-terminated!

extern TimerState timer_get_state();
extern void timer_start();
extern void timer_split();
extern void timer_reset();
extern void timer_set_variable(const char*, usize, const char*, usize);
extern void timer_set_game_time(int64_t, int32_t);
extern void timer_pause_game_time();
extern void timer_resume_game_time();

extern ProcessId process_attach(const char*, usize); // FIXME -> Option<ProcessId>!!!
extern void process_detach(ProcessId);
extern bool process_is_open(ProcessId);
extern bool process_read(ProcessId, Address, char*, usize);
extern NonZeroAddress process_get_module_address(ProcessId, const char*, usize); // FIXME -> Option<NonZeroAddress>!!!

extern void runtime_set_tick_rate(double);
extern void runtime_print_message(const char*, usize);

