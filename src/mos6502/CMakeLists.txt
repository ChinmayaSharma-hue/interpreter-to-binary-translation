function(add_mos6502_core_library target_name trace_enabled_value report_smc_value)
    add_library(${target_name}
        cpu.c
            linear.c
    )

    target_include_directories(${target_name}
        PUBLIC
            ${PROJECT_SOURCE_DIR}/include
    )

    target_compile_definitions(${target_name}
        PUBLIC
            MOS6502_ENABLE_TRACE=${trace_enabled_value}
            MOS6502_ENABLE_REPORT_SELF_MODIFYING_CODE=${report_smc_value}
    )
endfunction()

add_mos6502_core_library(mos6502_core_trace
    $<BOOL:${MOS6502_ENABLE_TRACE}>
    $<BOOL:${MOS6502_ENABLE_REPORT_SELF_MODIFYING_CODE}>
)
add_mos6502_core_library(mos6502_core_no_trace
    $<BOOL:${MOS6502_ENABLE_TRACE}>
    $<BOOL:${MOS6502_ENABLE_REPORT_SELF_MODIFYING_CODE}>
)
