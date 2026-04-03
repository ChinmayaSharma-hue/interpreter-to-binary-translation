cmake_minimum_required(VERSION 3.28)
project(mos6502 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(MOS6502_ENABLE_TRACE "Enable CPU trace output" ON)
option(MOS6502_ENABLE_REPORT_SELF_MODIFYING_CODE "Enable self-modifying code reporting" OFF)

add_subdirectory(src/mos6502)

function(add_mos6502_executable target_name core_target)
    add_executable(${target_name}
        main.c
        src/emulator.c
    )

    target_include_directories(${target_name} PRIVATE ${PROJECT_SOURCE_DIR}/include)
    target_link_libraries(${target_name} PRIVATE ${core_target})
endfunction()

add_mos6502_executable(mos6502 mos6502_core_trace)
add_mos6502_executable(mos6502_no_trace mos6502_core_no_trace)