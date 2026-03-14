cmake_minimum_required(VERSION 3.28)
project(mos6502 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_subdirectory(src/mos6502)

add_executable(mos6502
    main.c
        src/app/emulator.c
)
target_include_directories(mos6502 PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(mos6502 PRIVATE mos6502_core)
