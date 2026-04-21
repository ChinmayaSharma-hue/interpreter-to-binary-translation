add_library(mos6502_core
    cpu.c
    linear.c
)

target_include_directories(mos6502_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_definitions(mos6502_core
    PUBLIC
        MOS6502_ENABLE_REPORT_SELF_MODIFYING_CODE=$<BOOL:${MOS6502_ENABLE_REPORT_SELF_MODIFYING_CODE}>
)
