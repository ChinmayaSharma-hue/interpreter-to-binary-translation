cmake_minimum_required(VERSION 3.28)
project(mos6502 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(MOS6502_ENABLE_REPORT_SELF_MODIFYING_CODE "Enable self-modifying code reporting" OFF)

add_subdirectory(src/mos6502)

add_executable(mos6502
    main.c
    src/emulator.c
        include/x86_64/isa.h
        include/x86_64/emitter.h
        src/x86_64/emitter/helpers.h
        src/x86_64/emitter/mov.c
        src/x86_64/emitter/lea.c
        src/x86_64/emitter/alu.c
        src/x86_64/emitter/group.c
        src/x86_64/emitter/flags.c
)

target_include_directories(mos6502 PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(mos6502 PRIVATE mos6502_core)