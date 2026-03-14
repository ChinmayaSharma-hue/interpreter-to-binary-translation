add_library(mos6502_core
    cpu.c
    interpreter_engine.c
)

target_include_directories(mos6502_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)
