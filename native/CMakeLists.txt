cmake_minimum_required(VERSION 3.24)
project(ABCNative VERSION 0.1.0 LANGUAGES C CXX)

option(ABC_BUILD_IDE "Build the Qt desktop editor" OFF)
option(ABC_BUILD_TK "Build the Tcl/Tk comparison editor" OFF)
option(ABC_SANITIZERS "Enable address and undefined behavior sanitizers" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_subdirectory(third_party/libtommath)
add_subdirectory(third_party/pcre2)

add_library(abc_core
    src/big_integer.cpp
    src/big_integer_runtime.cpp
    src/stdlib.cpp
    src/random.cpp
    src/console_input.cpp
    src/console_output.cpp
    src/formatting.cpp
    src/files.cpp
    src/file_moves.cpp
    src/file_operations.cpp
    src/text_file.cpp
    src/stdlib_text.cpp
    src/string_arrays.cpp
    src/school.cpp
    src/school_text.cpp
    src/regex.cpp
    src/school_ip.cpp
    src/sequences.cpp
    src/real_range.cpp
    src/sequence_numeric.cpp
    src/sequence_slice.cpp
    src/sequence_concat.cpp
    src/sequence_compare.cpp
    src/sequence_transform.cpp
    src/sequence_predicate.cpp
    src/diagnostics.cpp
    src/types.cpp
    src/pascal.cpp
    src/constant.cpp
    src/bytecode.cpp
    src/vm.cpp)
target_include_directories(abc_core PUBLIC include)
target_link_libraries(abc_core PRIVATE abc_math abc_regex)
include(CheckCXXSourceCompiles)
check_cxx_source_compiles("#include <charconv>
int main() { const char text[] = \"1.5\"; double value; return std::from_chars(text, text + 3, value).ec == std::errc{} ? 0 : 1; }"
    ABC_HAS_FLOAT_FROM_CHARS)
if(ABC_HAS_FLOAT_FROM_CHARS)
    target_compile_definitions(abc_core PRIVATE ABC_HAS_FLOAT_FROM_CHARS=1)
endif()
if(MSVC)
    target_compile_options(abc_core PRIVATE /W4 /utf-8)
else()
    target_compile_options(abc_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(ABC_SANITIZERS)
    if(MSVC)
        message(FATAL_ERROR "ABC_SANITIZERS requires GCC or Clang on Unix")
    endif()
    target_compile_options(abc_core PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_compile_options(abc_math PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_compile_options(abc_regex PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(abc_core PUBLIC -fsanitize=address,undefined)
endif()

add_executable(abc src/main.cpp)
target_link_libraries(abc PRIVATE abc_core)

include(CTest)
if(BUILD_TESTING)
    add_executable(abc_tests tests/core_tests.cpp)
    target_link_libraries(abc_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_tests PRIVATE /utf-8)
    endif()
    add_executable(abc_big_integer_tests tests/big_integer_tests.cpp)
    target_link_libraries(abc_big_integer_tests PRIVATE abc_core)
    add_test(NAME core COMMAND abc_tests)
    add_executable(abc_number_fallback_tests tests/number_tests.cpp)
    add_test(NAME number_fallback COMMAND abc_number_fallback_tests)
    add_test(NAME big_integer_backend COMMAND abc_big_integer_tests)
    add_test(NAME cli_check COMMAND abc check "${CMAKE_CURRENT_SOURCE_DIR}/examples/sum.pas")
    add_test(NAME cli_run COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/hello.pas")
    set_tests_properties(cli_run PROPERTIES PASS_REGULAR_EXPRESSION "ABC Native")
    add_test(NAME stdlib_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME cli_type_error COMMAND abc check "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/type-error.pas")
    add_test(NAME text_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=text-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    set_tests_properties(cli_type_error PROPERTIES WILL_FAIL TRUE)
    add_test(NAME foundations_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=foundations-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME collections_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=collections-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME buffers_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=buffers-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME association_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=association-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME linked_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=linked-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME single_node_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=single-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME nil_string_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=nil-strings-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME string_mutation_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=mutation-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME directory_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=directories-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME rename_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=renames-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME text_file_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=text-files-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME text_method_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=text-methods-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME text_change_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=text-changes-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME loop_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=loops-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME foreach_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=foreach-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME character_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=characters-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME file_operations_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=file-operations-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME small_integer_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=small-integers-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME unsigned_integer_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=unsigned-integers-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME mutation_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=sequence-mutation-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME extended_math_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=extended-math-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME array_output_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=array-output-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME bitwise_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=bitwise-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME console_input_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=console-input-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME console_functions_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=console-functions-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME unicode_output_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=unicode-output-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME matrix_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=matrix-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME object_output_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=object-output-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME formatted_output_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=formatted-output-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME print_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=print-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME try_read_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=try-read-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME string_conversion_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=string-conversion-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME string_cursor_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=string-cursor-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME string_member_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=string-members-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME string_array_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=string-arrays-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME school_base_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=school-base-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME school_prime_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=school-prime-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME school_list_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=school-list-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME school_factor_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=school-factor-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME school_text_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=school-text-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME school_fold_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=school-fold-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME unicode_identifier_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=unicode-identifiers-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME unicode_character_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=unicode-character-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME tuple_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=tuple-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME school_minmax_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=school-minmax-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME big_integer_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=big-integer-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME school_big_integer_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=school-big-integer-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME school_truth_table_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=school-truth-table-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME string_value_method_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=string-value-method-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME school_ip_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=school-ip-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME ip_sequence_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=ip-sequence-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME sequence_view_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=sequence-view-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME random_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=random-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME school_swap_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=school-swap-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME regex_search_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=regex-search-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME sequence_list_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=sequence-list-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME sequence_query_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=sequence-query-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME default_argument_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=default-argument-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME original_params1 COMMAND $<TARGET_FILE:abc> run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/params1.pas")
    add_test(NAME original_constparam2 COMMAND $<TARGET_FILE:abc> run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/constparam2.pas")
    add_test(NAME original_params5 COMMAND $<TARGET_FILE:abc> run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/params5.pas")
    add_test(NAME original_const1 COMMAND $<TARGET_FILE:abc> run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/const1.pas")
    add_test(NAME original_const3 COMMAND $<TARGET_FILE:abc> run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/const3.pas")
    set_tests_properties(original_params1 original_constparam2 original_params5 original_const1 original_const3 PROPERTIES TIMEOUT 10)
    add_test(NAME params_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=params-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME const_out_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=const-out-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME sequence_flatmap_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=sequence-flatmap-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME sequence_predicate_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=sequence-predicate-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME sequence_callback_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=sequence-callback-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME sequence_tools_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=sequence-tools-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME string_sequence_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=string-sequence-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME sequence_concat_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=sequence-concat-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME sequence_slice_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=sequence-slice-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME numeric_aggregate_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=sequence-aggregate-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME partial_sum_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=partial-sum-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME real_range_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=real-range-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME partition_point_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=partition-point-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME big_range_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=big-range-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME stepped_range_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=stepped-range-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME sequence_factory_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=sequence-factory-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
endif()

install(TARGETS abc RUNTIME DESTINATION bin)
install(DIRECTORY examples/ DESTINATION examples)
install(FILES README.md DESTINATION .)
install(FILES stdlib-status.json DESTINATION .)
install(FILES licenses/DotNet-Runtime-MIT.txt DESTINATION licenses)
install(FILES licenses/Unicode-License.txt DESTINATION licenses)
install(FILES licenses/LibTomMath-Unlicense.txt DESTINATION licenses)
install(FILES licenses/PCRE2-LICENCE.md DESTINATION licenses)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/../doc/license.txt" DESTINATION licenses RENAME PascalABC.NET.txt)

if(ABC_BUILD_IDE)
    find_package(Qt6 6.8 REQUIRED COMPONENTS Widgets)
    qt_add_executable(abc-ide WIN32 MACOSX_BUNDLE src/ide/main.cpp src/ide/theme.cpp)
    add_dependencies(abc-ide abc)
    if(MSVC)
        target_compile_options(abc-ide PRIVATE /utf-8)
    endif()
    target_link_libraries(abc-ide PRIVATE Qt6::Widgets)
    get_target_property(abc_qmake Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(abc_qt_bin "${abc_qmake}" DIRECTORY)
    if(WIN32)
        add_custom_command(TARGET abc-ide POST_BUILD
            COMMAND "${abc_qt_bin}/windeployqt.exe" --no-translations --no-compiler-runtime "$<TARGET_FILE:abc-ide>"
            VERBATIM)
        if(BUILD_TESTING)
            add_custom_command(TARGET abc-ide POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${abc_qt_bin}/../plugins/platforms/qoffscreen.dll" "$<TARGET_FILE_DIR:abc-ide>/platforms/qoffscreen.dll"
                VERBATIM)
            install(FILES "${abc_qt_bin}/../plugins/platforms/qoffscreen.dll" DESTINATION bin/platforms)
        endif()
    endif()
    set_target_properties(abc-ide PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "ABC Native"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.abcnative.ide")
    install(TARGETS abc-ide RUNTIME DESTINATION bin BUNDLE DESTINATION .)
    if(APPLE)
        add_dependencies(abc-ide abc)
        add_custom_command(TARGET abc-ide POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:abc>" "$<TARGET_FILE_DIR:abc-ide>/abc")
    endif()
    if(BUILD_TESTING)
        add_test(NAME qt_editor COMMAND abc-ide --smoke-test -platform offscreen)
        add_test(NAME qt_runtime_error COMMAND abc-ide --smoke-runtime-error -platform offscreen)
        set_tests_properties(qt_editor qt_runtime_error PROPERTIES TIMEOUT 20)
    endif()
    if(APPLE)
        set(abc_deploy_executable "$<TARGET_FILE_NAME:abc-ide>.app")
    else()
        set(abc_deploy_executable "$<TARGET_FILE:abc-ide>")
    endif()
    # Keep executable paths quoted, including spaces and non-ASCII directories.
    qt_generate_deploy_script(TARGET abc-ide OUTPUT_SCRIPT abc_deploy
        CONTENT "qt_deploy_runtime_dependencies(
            EXECUTABLE \"${abc_deploy_executable}\"
            GENERATE_QT_CONF BIN_DIR bin PLUGINS_DIR bin
            NO_TRANSLATIONS NO_COMPILER_RUNTIME
        )")
    install(SCRIPT ${abc_deploy})
endif()

if(ABC_BUILD_TK)
    set(ABC_TCL_LIBRARY "" CACHE FILEPATH "Tcl 8.6 import/shared library")
    set(ABC_TK_LIBRARY "" CACHE FILEPATH "Tk 8.6 import/shared library")
    set(ABC_TCL_RUNTIME_DIR "" CACHE PATH "Directory containing tcl8.6 and tk8.6 script directories")
    set(ABC_TCL_DLL "" CACHE FILEPATH "Tcl 8.6 Windows DLL")
    set(ABC_TK_DLL "" CACHE FILEPATH "Tk 8.6 Windows DLL")
    set(ABC_TCL_EXTRA_DLLS "" CACHE STRING "Additional runtime DLLs required by Tcl/Tk")
    if(NOT EXISTS "${ABC_TCL_LIBRARY}" OR NOT EXISTS "${ABC_TK_LIBRARY}"
        OR NOT EXISTS "${ABC_TCL_RUNTIME_DIR}/tcl8.6/init.tcl"
        OR NOT EXISTS "${ABC_TCL_RUNTIME_DIR}/tk8.6/tk.tcl")
        message(FATAL_ERROR "Set ABC_TCL_LIBRARY, ABC_TK_LIBRARY and ABC_TCL_RUNTIME_DIR to a Tcl/Tk 8.6 distribution")
    endif()
    add_executable(abc-tk WIN32 src/tcl/launcher.cpp)
    add_custom_target(abc-tk-ui ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:abc-tk>/ui"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/src/tcl/editor.tcl" "$<TARGET_FILE_DIR:abc-tk>/ui/editor.tcl")
    add_dependencies(abc-tk abc-tk-ui)
    target_link_libraries(abc-tk PRIVATE abc_core "${ABC_TCL_LIBRARY}" "${ABC_TK_LIBRARY}")
    if(WIN32)
        target_link_libraries(abc-tk PRIVATE shell32 user32)
        if(NOT EXISTS "${ABC_TCL_DLL}" OR NOT EXISTS "${ABC_TK_DLL}")
            message(FATAL_ERROR "Set ABC_TCL_DLL and ABC_TK_DLL for a portable Windows build")
        endif()
        add_custom_command(TARGET abc-tk POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ABC_TCL_DLL}" "${ABC_TK_DLL}" "$<TARGET_FILE_DIR:abc-tk>")
        install(FILES "${ABC_TCL_DLL}" "${ABC_TK_DLL}" DESTINATION bin)
        get_filename_component(abc_tcl_bin "${ABC_TCL_DLL}" DIRECTORY)
        if(EXISTS "${abc_tcl_bin}/zlib1.dll")
            list(APPEND ABC_TCL_EXTRA_DLLS "${abc_tcl_bin}/zlib1.dll")
        endif()
        if(ABC_TCL_EXTRA_DLLS)
            add_custom_command(TARGET abc-tk POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ABC_TCL_EXTRA_DLLS} "$<TARGET_FILE_DIR:abc-tk>")
            install(FILES ${ABC_TCL_EXTRA_DLLS} DESTINATION bin)
        endif()
    endif()
    add_dependencies(abc-tk abc)
    add_custom_command(TARGET abc-tk POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:abc-tk>/ui"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/src/tcl/editor.tcl" "$<TARGET_FILE_DIR:abc-tk>/ui/editor.tcl"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${ABC_TCL_RUNTIME_DIR}/tcl8.6" "$<TARGET_FILE_DIR:abc-tk>/../lib/tcl8.6"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${ABC_TCL_RUNTIME_DIR}/tk8.6" "$<TARGET_FILE_DIR:abc-tk>/../lib/tk8.6")
    install(TARGETS abc-tk RUNTIME DESTINATION bin)
    install(FILES src/tcl/editor.tcl DESTINATION share/abc)
    install(DIRECTORY "${ABC_TCL_RUNTIME_DIR}/tcl8.6" "${ABC_TCL_RUNTIME_DIR}/tk8.6" DESTINATION lib)
    if(BUILD_TESTING)
        add_test(NAME tk_editor COMMAND abc-tk --smoke-test)
        set_tests_properties(tk_editor PROPERTIES TIMEOUT 20)
    endif()
endif()

if(MSVC)
    set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION bin)
    include(InstallRequiredSystemLibraries)
endif()

set(CPACK_GENERATOR ZIP)
set(CPACK_PACKAGE_FILE_NAME "ABC-Native-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
include(CPack)
