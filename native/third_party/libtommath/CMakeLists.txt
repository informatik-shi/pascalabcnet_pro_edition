# Pinned LibTomMath 1.3.0. See UPSTREAM.md and LICENSE.
include(sources.cmake)
# Conversion is implemented in C++ with a verified IEEE-754 layout on all hosts.
list(REMOVE_ITEM SOURCES bn_mp_set_double.c)
add_library(abc_math STATIC ${SOURCES})
set_target_properties(abc_math PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED YES)
target_include_directories(abc_math PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(abc_math PUBLIC MP_32BIT= MP_NO_FILE MP_FIXED_CUTOFFS MP_LOW_MEM)
if(WIN32)
    target_link_libraries(abc_math PRIVATE advapi32)
endif()
