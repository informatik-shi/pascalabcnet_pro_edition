if(NOT DEFINED ABC_BUILD_DIR)
    message(FATAL_ERROR "ABC_BUILD_DIR is required")
endif()
if(NOT DEFINED ABC_BUILD_CONFIG)
    set(ABC_BUILD_CONFIG Release)
endif()
get_filename_component(build_dir "${ABC_BUILD_DIR}" ABSOLUTE)
if(NOT EXISTS "${build_dir}/CMakeCache.txt")
    message(FATAL_ERROR "Configure the native build before running Build.cmake")
endif()
execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${build_dir}" --config "${ABC_BUILD_CONFIG}" --parallel 2
    RESULT_VARIABLE build_result
    OUTPUT_VARIABLE build_output ERROR_VARIABLE build_error
    ENCODING UTF-8
    ECHO_OUTPUT_VARIABLE ECHO_ERROR_VARIABLE
)
set(build_log "${build_output}\n${build_error}")
file(WRITE "${build_dir}/native-build.log" "${build_log}")
if(NOT "${build_result}" STREQUAL "0")
    if("$ENV{GITHUB_ACTIONS}" STREQUAL "true")
        string(LENGTH "${build_log}" length)
        if(length GREATER 12000)
            math(EXPR offset "${length}-12000")
            string(SUBSTRING "${build_log}" ${offset} -1 build_log)
        endif()
        string(REPLACE "%" "%25" build_log "${build_log}")
        string(REPLACE "\r" "%0D" build_log "${build_log}")
        string(REPLACE "\n" "%0A" build_log "${build_log}")
        message("::error title=Native build diagnostics::${build_log}")
    endif()
    message(FATAL_ERROR "Native build failed (${build_result}); full diagnostics: ${build_dir}/native-build.log")
endif()
