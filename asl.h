#pragma once
#include <stdint.h>

typedef enum PointerType : uint8_t {
    U8 = 0,
    U16 = 1,
    U32 = 2,
    U64 = 3,
    I8 = 4,
    I16 = 5,
    I32 = 6,
    I64 = 7,
    F32 = 8,
    F64 = 9,
    String = 10,
} PointerType;

typedef int32_t pointer_path_id;

extern void set_process_name(const char*, int32_t);
extern void set_tick_rate(double);
extern pointer_path_id push_pointer_path(const char*, int32_t, PointerType);
extern void push_offset(pointer_path_id, int64_t);
extern void print_message(const char*, int32_t);
extern uint8_t get_u8(pointer_path_id, bool);
extern uint16_t get_u16(pointer_path_id, bool);
extern uint32_t get_u32(pointer_path_id, bool);
extern uint64_t get_u64(pointer_path_id, bool);
extern int8_t get_i8(pointer_path_id, bool);
extern int16_t get_i16(pointer_path_id, bool);
extern int32_t get_i32(pointer_path_id, bool);
extern int64_t get_i64(pointer_path_id, bool);
extern float get_f32(pointer_path_id, bool);
extern double get_f64(pointer_path_id, bool);
//extern char* get_string(pointer_path_id, bool);

