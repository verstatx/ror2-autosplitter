#!/bin/sh

#clang --target=wasm32-wasi --sysroot /opt/wasi-sdk/wasi-sysroot/ -nostdlib -Wl,--no-entry -Wl,--allow-undefined \
#    -DUSE_WASI_SDK \
clang --target=wasm32 -nostdlib -Wl,--no-entry -Wl,--allow-undefined \
    -Wl,--export=configure \
    -Wl,--export=update \
    -Wl,--export=hooked \
    -Wl,--export=unhooked \
    -Wl,--export=should_start \
    -Wl,--export=should_split \
    -Wl,--export=should_reset \
    -Wl,--export=is_loading \
    -Wl,--export=game_time \
    -O3 \
    -DREMOVE_LOADS \
    -DPACKED_STRINGS \
    -o RoR2-autosplitter.wasm autosplitter.c
    #-DDEBUG_OUTPUT \
    #-DV1_0_3_1 -o RoR2-autosplitter-v1_0_3_1.wasm autosplitter.c
