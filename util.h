#pragma once
#include "env.h"

typedef union {
    char buf[8];
    uint64_t out;
} read_u64;

typedef read_u64 read_Address;
typedef read_u64 read_NonZeroAddress;

typedef union {
    char buf[4];
    uint32_t out;
} read_u32;

typedef union {
    char buf[4];
    int32_t out;
} read_i32;

typedef union {
    char buf[1];
    uint8_t out;
} read_u8;

typedef union {
    char buf[4];
    float out;
    uint32_t out_raw;
} read_f32;

typedef union {
    char buf[8];
    double out;
    uint64_t out_raw;
} read_f64;

typedef struct _PointerPath {
    const char* module_name;
    const size_t num_offsets;
    const Address* offsets;
} PointerPath;


#ifdef DEBUG_OUTPUT

#  define DEBUG_PRINT(x) do { runtime_print_message(x, sizeof(x) - 1); } while(0);

void reverse(char* begin, char* rbegin) {
    for (--rbegin; begin < rbegin; ++begin, --rbegin) {
        char tmp = *begin;
        *begin = *rbegin;
        *rbegin = tmp;
    }
}

#  define ITOA_MAXLEN 20
// uint64_t to base 10 string. Returns pointer past end of string.
char* itoa(uint64_t value, char* result) {
    char* out = result;

    do {
        *out++ = "0123456789"[value % 10];
        value /= 10;
    } while (value);

    reverse(result, out);

    return out;
}

#  define ITOX_LEN 18
// uint64_t to base 16 hex string. Returns pointer past end of string.
char* itox(uint64_t value, char* result) {
    *result++ = '0';
    *result++ = 'x';
    char* out = result;

    do {
        *out++ = "0123456789ABCDEF"[value % 16];
        value >>= 4;
    } while (value);

    reverse(result, out);

    return out;
}

void print_pid(ProcessId pid) {
    char pid_string[12 + ITOA_MAXLEN] = "Process ID: ";
    char* end = itoa(pid, &pid_string[12]);

    runtime_print_message(pid_string, end - pid_string);
}

void print_u64(uint64_t val) {
    char value_string[7 + ITOA_MAXLEN] = "Value: ";
    char* end = itoa(val, &value_string[7]);

    runtime_print_message(value_string, end - value_string);
}

void print_Address(Address addr) {
    char addr_string[9 + ITOX_LEN] = "Address: ";
    char* end = itox(addr, &addr_string[9]);

    runtime_print_message(addr_string, end - addr_string);
}

void print_u64_hex(uint64_t val) {
    char value_string[7 + ITOX_LEN] = "Value: ";
    char* end = itox(val, &value_string[7]);

    runtime_print_message(value_string, end - value_string);
}

#else
#  define DEBUG_PRINT(x)
#  define print_pid(x)
#  define print_u64(x)
#  define print_u64_hex(x)
#  define print_Address(x)
#endif

inline size_t strlen(const char* str) {
    size_t ret = 0;
    while (*str++)
        ret++;
    return ret;
}

inline bool str_is_equal(const char* l, const char* r) {
    int i = 0;
    for (; l[i] != '\0' && r[i] != '\0'; i++) {
        if (l[i] != r[i])
            return false;
    }
    return l[i] == r[i];
}

// Returns Address of final value in pointer path; 0 on failure
inline NonZeroAddress try_resolve_pointer_path(ProcessId pid, PointerPath pp) {
    NonZeroAddress addr = process_get_module_address(pid, pp.module_name, strlen(pp.module_name));

    // Special handling for first offset
    if (addr && pp.num_offsets) {
        addr += pp.offsets[0];
    }

    for (size_t i = 1; i < pp.num_offsets; ++i) {
        read_Address next;
        if (!process_read(pid, addr, next.buf, sizeof(Address))) {
            return 0;
        }
        addr = next.out + pp.offsets[i];
    }
    return addr;
}
