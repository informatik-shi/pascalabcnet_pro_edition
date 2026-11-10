cmake_minimum_required(VERSION 3.24)
project(ABCNative VERSION 0.1.0 LANGUAGES C CXX)

option(ABC_BUILD_IDE "Build the Qt desktop editor" OFF)
option(ABC_BUILD_TK "Build the Tcl/Tk comparison editor" OFF)
option(ABC_SANITIZERS "Enable address and undefined behavior sanitizers" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(Threads REQUIRED)
add_subdirectory(third_party/libtommath)
add_subdirectory(third_party/pcre2)

add_library(abc_core
    src/big_integer.cpp
    src/big_integer_runtime.cpp
    src/stdlib.cpp
    src/random.cpp
    src/random_sequence.cpp
    src/console_input.cpp
    src/console_output.cpp
    src/formatting.cpp
    src/print.cpp
    src/files.cpp
    src/file_names.cpp
    src/file_reader.cpp
    src/file_writer.cpp
    src/binary_stream.cpp
    src/binary_codec.cpp
    src/binary_file.cpp
    src/native_resource.cpp
    src/file_lines.cpp
    src/directory_sequence.cpp
    src/file_encoding.cpp
    src/sbcs.cpp
    src/dbcs.cpp
    src/gb18030.cpp
    src/iscii.cpp
    src/iso2022.cpp
    src/utf7.cpp
    src/text_encoder.cpp
    src/encoding.cpp
    src/encoding_metadata.cpp
    src/enumeration.cpp
    src/comparable.cpp
    src/comparison.cpp
    src/equality.cpp
    src/sorting.cpp
    src/ini_file.cpp
    src/compound.cpp
    src/hash_set.cpp
    src/value_hash.cpp
    src/dictionary.cpp
    src/enumerator.cpp
    src/interfaces.cpp
    src/collection_interface.cpp
    src/record_interface.cpp
    src/record_initialization.cpp
    src/ordinal_range.cpp
    src/real_interval.cpp
    src/new_set.cpp
    src/cp1251.cpp
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
    src/generators.cpp
    src/real_range.cpp
    src/sequence_numeric.cpp
    src/sequence_slice.cpp
    src/sequence_concat.cpp
    src/sequence_compare.cpp
    src/sequence_set.cpp
    src/sequence_transform.cpp
    src/sequence_join.cpp
    src/sequence_predicate.cpp
    src/sequence_zip.cpp
    src/sequence_cartesian.cpp
    src/sequence_combinatorics.cpp
    src/array_views.cpp
    src/matrix_operations.cpp
    src/array_operations.cpp
    src/collection_operations.cpp
    src/matrix_construction.cpp
    src/list_operations.cpp
    src/list_bulk_user.cpp
    src/list_storage.cpp
    src/timing.cpp
    src/single.cpp
    src/type_names.cpp
    src/type_properties.cpp
    src/callable_identity.cpp
    src/environment.cpp
    src/disk_space.cpp
    src/diagnostics.cpp
    src/compiler_diagnostics.cpp
    src/types.cpp
    src/pascal.cpp
    src/constant.cpp
    src/bytecode.cpp
    src/vm.cpp)
target_include_directories(abc_core PUBLIC include)
target_link_libraries(abc_core PRIVATE abc_math abc_regex Threads::Threads)
if(NOT WIN32)
    find_package(ICU 60 REQUIRED COMPONENTS i18n uc data)
    target_link_libraries(abc_core PRIVATE ICU::i18n ICU::uc ICU::data)
endif()
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
if(MSVC)
    target_compile_options(abc PRIVATE /utf-8)
endif()

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
    add_executable(abc_single_tests tests/single_tests.cpp)
    target_link_libraries(abc_single_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_single_tests PRIVATE /utf-8)
    endif()
    add_test(NAME single_precision COMMAND abc_single_tests)
    add_executable(abc_dictionary_tests tests/dictionary_tests.cpp)
    target_link_libraries(abc_dictionary_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_dictionary_tests PRIVATE /utf-8)
    endif()
    add_test(NAME dictionary_compatibility COMMAND abc_dictionary_tests)
    add_test(NAME dictionary_example COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=dictionary-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME original_dictionary_array COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/new_templ_arr.pas")
    add_test(NAME original_dictionary_keys COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/nested_class1.pas")
    add_test(NAME original_dictionary_view_addition COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/extensionmethods30.pas")
    add_test(NAME original_dictionary_actions COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/foreach14.pas")
    add_test(NAME original_dictionary_values_enumerator COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/dValues.pas")
    add_executable(abc_dictionary_enumerator_tests tests/dictionary_enumerator_tests.cpp)
    target_link_libraries(abc_dictionary_enumerator_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_dictionary_enumerator_tests PRIVATE /utf-8)
    endif()
    add_test(NAME dictionary_enumerators COMMAND abc_dictionary_enumerator_tests)
    add_executable(abc_generic_enumerator_tests tests/generic_enumerator_tests.cpp)
    target_link_libraries(abc_generic_enumerator_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_generic_enumerator_tests PRIVATE /utf-8)
    endif()
    add_test(NAME generic_enumerators COMMAND abc_generic_enumerator_tests)
    add_executable(abc_collection_enumerator_tests tests/collection_enumerator_tests.cpp)
    target_link_libraries(abc_collection_enumerator_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_collection_enumerator_tests PRIVATE /utf-8)
    endif()
    add_test(NAME collection_enumerators COMMAND abc_collection_enumerator_tests)
    add_executable(abc_user_interface_tests tests/user_interface_tests.cpp)
    target_link_libraries(abc_user_interface_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_user_interface_tests PRIVATE /utf-8)
    endif()
    add_test(NAME user_interfaces COMMAND abc_user_interface_tests)
    add_executable(abc_user_enumerable_tests tests/user_enumerable_tests.cpp)
    target_link_libraries(abc_user_enumerable_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_user_enumerable_tests PRIVATE /utf-8)
    endif()
    add_test(NAME user_enumerables COMMAND abc_user_enumerable_tests)
    add_executable(abc_comparer_tests tests/comparer_tests.cpp)
    target_link_libraries(abc_comparer_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_comparer_tests PRIVATE /utf-8)
    endif()
    add_test(NAME comparers COMMAND abc_comparer_tests)
    add_executable(abc_sorting_tests tests/sorting_tests.cpp)
    target_link_libraries(abc_sorting_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_sorting_tests PRIVATE /utf-8)
    endif()
    add_test(NAME typed_sorting COMMAND abc_sorting_tests)
    add_executable(abc_list_sorting_tests tests/list_sorting_tests.cpp)
    target_link_libraries(abc_list_sorting_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_list_sorting_tests PRIVATE /utf-8)
    endif()
    add_executable(abc_array_sorting_tests tests/array_sorting_tests.cpp)
    target_link_libraries(abc_array_sorting_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_array_sorting_tests PRIVATE /utf-8)
    endif()
    add_executable(abc_paired_sorting_tests tests/paired_sorting_tests.cpp)
    target_link_libraries(abc_paired_sorting_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_paired_sorting_tests PRIVATE /utf-8)
    endif()
    add_executable(abc_equality_tests tests/equality_tests.cpp)
    target_link_libraries(abc_equality_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_equality_tests PRIVATE /utf-8)
    endif()
    add_executable(abc_collection_search_tests tests/collection_search_tests.cpp)
    target_link_libraries(abc_collection_search_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_collection_search_tests PRIVATE /utf-8)
    endif()
    add_executable(abc_sequence_equality_tests tests/sequence_equality_tests.cpp)
    target_link_libraries(abc_sequence_equality_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_sequence_equality_tests PRIVATE /utf-8)
    endif()
    add_executable(abc_sequence_set_tests tests/sequence_set_tests.cpp)
    target_link_libraries(abc_sequence_set_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_sequence_set_tests PRIVATE /utf-8)
    endif()
    add_test(NAME sequence_sets COMMAND abc_sequence_set_tests)
    add_executable(abc_file_operations_tests tests/file_operations_tests.cpp)
    target_link_libraries(abc_file_operations_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_file_operations_tests PRIVATE /utf-8)
    endif()
    add_test(NAME file_operations_reference COMMAND abc_file_operations_tests)
    add_test(NAME file_streams_example COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/file-streams.pas")
    add_executable(abc_array_generics_tests tests/array_generics_tests.cpp)
    target_link_libraries(abc_array_generics_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_array_generics_tests PRIVATE /utf-8)
    endif()
    add_test(NAME array_generics COMMAND abc_array_generics_tests)
    add_executable(abc_collection_interface_tests tests/collection_interface_tests.cpp)
    target_link_libraries(abc_collection_interface_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_collection_interface_tests PRIVATE /utf-8)
    endif()
    add_test(NAME collection_interfaces COMMAND abc_collection_interface_tests)
    add_executable(abc_list_interface_tests tests/list_interface_tests.cpp)
    target_link_libraries(abc_list_interface_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_list_interface_tests PRIVATE /utf-8)
    endif()
    add_test(NAME list_interfaces COMMAND abc_list_interface_tests)
    add_test(NAME list_interfaces_example COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/list-interfaces.pas")
    add_executable(abc_dictionary_property_tests tests/dictionary_property_tests.cpp)
    target_link_libraries(abc_dictionary_property_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_dictionary_property_tests PRIVATE /utf-8)
    endif()
    add_test(NAME dictionary_property COMMAND abc_dictionary_property_tests)
    add_executable(abc_dictionary_interface_tests tests/dictionary_interface_tests.cpp)
    target_link_libraries(abc_dictionary_interface_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_dictionary_interface_tests PRIVATE /utf-8)
    endif()
    add_test(NAME dictionary_interfaces COMMAND abc_dictionary_interface_tests)
    add_executable(abc_explicit_interface_tests tests/explicit_interface_tests.cpp)
    target_link_libraries(abc_explicit_interface_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_explicit_interface_tests PRIVATE /utf-8)
    endif()
    add_test(NAME explicit_interfaces COMMAND abc_explicit_interface_tests)
    add_executable(abc_set_interface_tests tests/set_interface_tests.cpp)
    target_link_libraries(abc_set_interface_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_set_interface_tests PRIVATE /utf-8)
    endif()
    add_test(NAME set_interfaces COMMAND abc_set_interface_tests)
    add_executable(abc_explicit_property_tests tests/explicit_property_tests.cpp)
    target_link_libraries(abc_explicit_property_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_explicit_property_tests PRIVATE /utf-8)
    endif()
    add_test(NAME explicit_properties COMMAND abc_explicit_property_tests)
    add_executable(abc_range_interface_tests tests/range_interface_tests.cpp)
    target_link_libraries(abc_range_interface_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_range_interface_tests PRIVATE /utf-8)
    endif()
    add_test(NAME range_interfaces COMMAND abc_range_interface_tests)
    add_test(NAME range_interfaces_example COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/range-interfaces.pas")
    add_executable(abc_real_range_interface_tests tests/real_range_interface_tests.cpp)
    target_link_libraries(abc_real_range_interface_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_real_range_interface_tests PRIVATE /utf-8)
    endif()
    add_test(NAME real_range_interfaces COMMAND abc_real_range_interface_tests)
    add_test(NAME real_range_interfaces_example COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/real-range-interfaces.pas")
    add_executable(abc_record_method_tests tests/record_method_tests.cpp)
    target_link_libraries(abc_record_method_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_record_method_tests PRIVATE /utf-8)
    endif()
    add_test(NAME record_methods COMMAND abc_record_method_tests)
    add_test(NAME record_methods_example COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/record-methods.pas")
    add_executable(abc_record_initialization_tests tests/record_initialization_tests.cpp)
    target_link_libraries(abc_record_initialization_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_record_initialization_tests PRIVATE /utf-8)
    endif()
    add_test(NAME record_initialization COMMAND abc_record_initialization_tests)
    add_test(NAME record_initialization_example COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/record-initialization.pas")
    add_test(NAME original_records13 COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/records13.pas")
    add_test(NAME original_records15 COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/records15.pas")
    add_executable(abc_list_user_bulk_tests tests/list_user_bulk_tests.cpp)
    target_link_libraries(abc_list_user_bulk_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_list_user_bulk_tests PRIVATE /utf-8)
    endif()
    add_test(NAME list_user_bulk COMMAND abc_list_user_bulk_tests)
    add_test(NAME list_user_bulk_example COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/list-user-bulk.pas")
    add_executable(abc_callable_family_tests tests/callable_family_tests.cpp)
    target_link_libraries(abc_callable_family_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_callable_family_tests PRIVATE /utf-8)
    endif()
    add_test(NAME callable_families COMMAND abc_callable_family_tests)
    add_test(NAME callable_families_example COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/callable-families.pas")
    add_executable(abc_type_property_tests tests/type_property_tests.cpp)
    target_link_libraries(abc_type_property_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_type_property_tests PRIVATE /utf-8)
    endif()
    add_test(NAME type_properties COMMAND abc_type_property_tests)
    add_executable(abc_named_callable_tests tests/named_callable_tests.cpp)
    target_link_libraries(abc_named_callable_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_named_callable_tests PRIVATE /utf-8)
    endif()
    add_test(NAME named_callables COMMAND abc_named_callable_tests)
    add_executable(abc_predicate_generic_tests tests/predicate_generic_tests.cpp)
    target_link_libraries(abc_predicate_generic_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_predicate_generic_tests PRIVATE /utf-8)
    endif()
    add_test(NAME predicate_generics COMMAND abc_predicate_generic_tests)
    add_executable(abc_sbcs_tests tests/sbcs_tests.cpp)
    target_link_libraries(abc_sbcs_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_sbcs_tests PRIVATE /utf-8)
    endif()
    add_test(NAME single_byte_encodings COMMAND abc_sbcs_tests)
    add_executable(abc_dbcs_tests tests/dbcs_tests.cpp)
    target_link_libraries(abc_dbcs_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_dbcs_tests PRIVATE /utf-8)
    endif()
    add_test(NAME double_byte_encodings COMMAND abc_dbcs_tests)
    add_executable(abc_gb18030_tests tests/gb18030_tests.cpp)
    target_link_libraries(abc_gb18030_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_gb18030_tests PRIVATE /utf-8)
    endif()
    add_test(NAME gb18030_encodings COMMAND abc_gb18030_tests)
    add_executable(abc_iscii_tests tests/iscii_tests.cpp)
    target_link_libraries(abc_iscii_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_iscii_tests PRIVATE /utf-8)
    endif()
    add_test(NAME iscii_encodings COMMAND abc_iscii_tests)
    add_executable(abc_iso2022_tests tests/iso2022_tests.cpp)
    target_link_libraries(abc_iso2022_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_iso2022_tests PRIVATE /utf-8)
    endif()
    add_test(NAME iso2022_encodings COMMAND ${CMAKE_COMMAND} -DABC_RUNNER=$<TARGET_FILE:abc_iso2022_tests> "-DABC_ARCHIVE=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/iso2022-reference.zip" "-DABC_TEST_DIR=${CMAKE_CURRENT_BINARY_DIR}" -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/iso2022-reference.cmake")
    add_test(NAME iso2022_encodings_example COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/iso2022-encodings.pas")
    add_executable(abc_utf7_tests tests/utf7_tests.cpp)
    target_link_libraries(abc_utf7_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_utf7_tests PRIVATE /utf-8)
    endif()
    add_test(NAME utf7_encoding COMMAND ${CMAKE_COMMAND} -DABC_RUNNER=$<TARGET_FILE:abc_utf7_tests> "-DABC_ARCHIVE=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/utf7-reference.zip" "-DABC_TEST_DIR=${CMAKE_CURRENT_BINARY_DIR}" -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/utf7-reference.cmake")
    add_test(NAME utf7_encoding_example COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/utf7-encoding.pas")
    add_executable(abc_encoding_metadata_tests tests/encoding_metadata_tests.cpp)
    target_link_libraries(abc_encoding_metadata_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_encoding_metadata_tests PRIVATE /utf-8)
    endif()
    add_test(NAME encoding_metadata COMMAND abc_encoding_metadata_tests)
    add_test(NAME encoding_metadata_example COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/encoding-metadata.pas")
    add_executable(abc_binary_stream_tests tests/binary_stream_tests.cpp)
    target_link_libraries(abc_binary_stream_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_binary_stream_tests PRIVATE /utf-8)
    endif()
    add_test(NAME binary_stream COMMAND abc_binary_stream_tests)
    add_executable(abc_binary_codec_tests tests/binary_codec_tests.cpp)
    target_link_libraries(abc_binary_codec_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_binary_codec_tests PRIVATE /utf-8)
    endif()
    add_test(NAME binary_codec COMMAND ${CMAKE_COMMAND} -DABC_RUNNER=$<TARGET_FILE:abc_binary_codec_tests> "-DABC_ARCHIVE=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/binary-codec-reference.zip" "-DABC_TEST_DIR=${CMAKE_CURRENT_BINARY_DIR}" -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/binary-codec-reference.cmake")
    add_executable(abc_binary_file_tests tests/binary_file_tests.cpp)
    target_link_libraries(abc_binary_file_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_binary_file_tests PRIVATE /utf-8)
    endif()
    add_test(NAME binary_files COMMAND abc_binary_file_tests)
    add_test(NAME binary_files_example COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/binary-files.pas")
    add_executable(abc_nested_routine_tests tests/nested_routine_tests.cpp)
    target_link_libraries(abc_nested_routine_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_nested_routine_tests PRIVATE /utf-8)
    endif()
    add_test(NAME nested_routines COMMAND abc_nested_routine_tests)
    add_test(NAME nested_routines_example COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/nested-routines.pas")
    add_test(NAME original_scalar_typed_files COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/fileof1.pas")
    add_test(NAME original_nested_methods COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/nested1.pas")
    add_test(NAME original_nested_record_reference COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/nested3.pas")
    add_test(NAME original_nested_class_reference COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/nested4.pas")
    add_test(NAME original_nested_result_shadow COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/fResult3.pas")
    add_test(NAME iscii_encodings_example COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/iscii-encodings.pas")
    add_test(NAME gb18030_encoding_example COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/gb18030-encoding.pas")
    add_test(NAME double_byte_encodings_example COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/double-byte-encodings.pas")
    add_test(NAME single_byte_encodings_example COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/single-byte-encodings.pas")
    add_test(NAME predicate_generics_example COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/predicate-generics.pas")
    add_test(NAME named_callables_example COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/named-callables.pas")
    add_test(NAME type_properties_example COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/type-properties.pas")
    add_test(NAME type_properties_cli COMMAND ${CMAKE_COMMAND} -DABC=$<TARGET_FILE:abc> -DABC_TEST_DIR=${CMAKE_CURRENT_BINARY_DIR} -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/type-properties-cli.cmake")
    add_test(NAME property_interfaces_example COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/property-interfaces.pas")
    add_test(NAME set_interfaces_example COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/set-interfaces.pas")
    add_test(NAME explicit_interfaces_example COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/explicit-interfaces.pas")
    add_test(NAME dictionary_interfaces_example COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/dictionary-interfaces.pas")
    add_test(NAME collection_property_writes_example COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/collection-property-writes.pas")
    add_test(NAME collection_interfaces_example COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/collection-interfaces.pas")
    add_test(NAME array_generics_example COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/array-generics.pas")
    add_test(NAME sequence_sets_example COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/sequence-sets.pas")
    add_test(NAME sequence_equality COMMAND abc_sequence_equality_tests)
    add_test(NAME sequence_equality_example COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/sequence-equality.pas")
    add_test(NAME collection_search COMMAND abc_collection_search_tests)
    add_test(NAME collection_search_example COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/collection-search.pas")
    add_executable(abc_dictionary_comparer_tests tests/dictionary_comparer_tests.cpp)
    target_link_libraries(abc_dictionary_comparer_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_dictionary_comparer_tests PRIVATE /utf-8)
    endif()
    add_test(NAME dictionary_comparers COMMAND abc_dictionary_comparer_tests)
    add_test(NAME dictionary_comparers_example COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/dictionary-comparers.pas")
    add_executable(abc_hashset_comparer_tests tests/hashset_comparer_tests.cpp)
    target_link_libraries(abc_hashset_comparer_tests PRIVATE abc_core)
    if(MSVC)
        target_compile_options(abc_hashset_comparer_tests PRIVATE /utf-8)
    endif()
    add_test(NAME hashset_comparers COMMAND abc_hashset_comparer_tests)
    add_test(NAME hashset_comparers_example COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/hashset-comparers.pas")
    add_test(NAME typed_equality COMMAND abc_equality_tests)
    add_test(NAME paired_sorting COMMAND abc_paired_sorting_tests)
    add_test(NAME paired_sorting_example COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/paired-sorting.pas")
    add_test(NAME system_array_sorting COMMAND abc_array_sorting_tests)
    add_test(NAME array_sorting_example COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/array-sorting.pas")
    add_test(NAME list_sorting COMMAND abc_list_sorting_tests)
    add_test(NAME list_sorting_example COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/list-sorting.pas")
    add_test(NAME sorting_example COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/sorting.pas")
    add_test(NAME comparers_example COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/comparers.pas")
    add_test(NAME user_enumerables_example COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/user-enumerables.pas")
    add_test(NAME user_interfaces_example COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/examples/user-interfaces.pas")
    add_test(NAME original_interface_class COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/interface1.pas")
    add_test(NAME single_numbers_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=single-numbers-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME original_realsingle1 COMMAND abc run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/realsingle1.pas")
    add_test(NAME cli_bilingual_diagnostics COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/examples/sum.pas"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/cli-diagnostics.cmake")
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
    add_test(NAME halt_cli_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/halt-cli.pas"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/halt-cli.cmake")
    add_test(NAME type_names_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=type-names-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME system_services_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=system-services-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME arguments_cli_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/arguments-cli.pas"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/arguments-cli.cmake")
    add_test(NAME file_names_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=file-names-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME directory_sequences_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=directory-sequences-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME console_tuples_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=console-tuples-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
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
    add_test(NAME original_in1 COMMAND $<TARGET_FILE:abc> run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/in1.pas")
    add_test(NAME original_notin COMMAND $<TARGET_FILE:abc> run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/notin.pas")
    add_test(NAME original_arithm7 COMMAND $<TARGET_FILE:abc> run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/arithm7.pas")
    add_test(NAME original_set9 COMMAND $<TARGET_FILE:abc> run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/set9.pas")
    add_test(NAME original_hset1 COMMAND $<TARGET_FILE:abc> run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/hset1.pas")
    add_test(NAME original_typed_range COMMAND $<TARGET_FILE:abc> run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/typedRange.pas")
    add_test(NAME random_sequences_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=random-sequences-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME real_range_random_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=real-range-random-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME ordinal_ranges_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=ordinal-ranges-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME hash_set_factories_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=hash-set-factories-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME print_extensions_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=print-extensions-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME original_set10 COMMAND $<TARGET_FILE:abc> run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/set10.pas")
    add_test(NAME original_set11 COMMAND $<TARGET_FILE:abc> run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/set11.pas")
    add_test(NAME set_literals_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=set-literals-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME new_sets_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=new-sets-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME hash_sets_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=hash-sets-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME hash_set_extra_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=hash-set-extra-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME membership_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=membership-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME array_operators_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=array-operators-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME compound_assignments_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=compound-assignments-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME original_params1 COMMAND $<TARGET_FILE:abc> run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/params1.pas")
    add_test(NAME original_constparam2 COMMAND $<TARGET_FILE:abc> run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/constparam2.pas")
    add_test(NAME original_params5 COMMAND $<TARGET_FILE:abc> run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/params5.pas")
    add_test(NAME original_const1 COMMAND $<TARGET_FILE:abc> run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/const1.pas")
    add_test(NAME original_const3 COMMAND $<TARGET_FILE:abc> run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/const3.pas")
    add_test(NAME original_delegates1 COMMAND $<TARGET_FILE:abc> run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/delegates1.pas")
    add_test(NAME original_delegates2 COMMAND $<TARGET_FILE:abc> run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/delegates2.pas")
    add_test(NAME original_delegates6 COMMAND $<TARGET_FILE:abc> run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/delegates6.pas")
    add_test(NAME original_lambda_explicit_types COMMAND $<TARGET_FILE:abc> run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/LambdaWithExplicitType.pas")
    add_test(NAME original_lambda_local_const COMMAND $<TARGET_FILE:abc> run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/lambdas_closures_15_const.pas")
    add_test(NAME original_list_factory_reverse COMMAND $<TARGET_FILE:abc> run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/extensionmethods25.pas")
    add_test(NAME original_person_external COMMAND $<TARGET_FILE:abc> run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/CompilationSamples/PersonExternal.pas")
    set_tests_properties(original_person_external PROPERTIES PASS_REGULAR_EXPRESSION "Имя: Иванов  Возраст: 20" TIMEOUT 10)
    add_test(NAME original_short_fun COMMAND $<TARGET_FILE:abc> run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/short_fun.pas")
    set_tests_properties(original_short_fun PROPERTIES PASS_REGULAR_EXPRESSION "133" TIMEOUT 10)
    add_test(NAME original_enum3 COMMAND $<TARGET_FILE:abc> run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/enum3.pas")
    add_test(NAME original_enum4 COMMAND $<TARGET_FILE:abc> run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/enum4.pas")
    add_test(NAME original_enum7 COMMAND $<TARGET_FILE:abc> run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/enum7.pas")
    add_test(NAME original_enum8 COMMAND $<TARGET_FILE:abc> run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/enum8.pas")
    add_test(NAME original_enum9 COMMAND $<TARGET_FILE:abc> run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/enum9.pas")
    add_test(NAME original_foreachenum1 COMMAND $<TARGET_FILE:abc> run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/foreachenum1.pas")
    add_test(NAME original_for_step1 COMMAND $<TARGET_FILE:abc> run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/for_step1.pas")
    add_test(NAME ini_files_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=ini-files-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME comparable_ranges_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=comparable-ranges-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME enumerations_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=enumerations-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME original_for_step2 COMMAND $<TARGET_FILE:abc> run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/for_step2.pas")
    add_test(NAME for_step_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=for-step-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME numeric_literals_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=numeric-literals-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME text_writer_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=text-writer-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME text_lines_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=text-lines-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME streaming_text_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=streaming-text-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME streaming_lines_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=streaming-lines-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME file_lines_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=file-lines-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME default_encoding_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=default-encoding-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME text_encodings_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=text-encodings-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME windows_characters_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=windows-characters-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME short_class_bodies_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=short-class-bodies-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME class_field_initializers_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=class-field-initializers-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME class_external_methods_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=class-external-methods-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME class_constructors_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=class-constructors-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME class_properties_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=class-properties-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME class_methods_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=class-methods-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME class_fields_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=class-fields-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME list_copy_to_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=list-copy-to-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME explicit_list_types_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=explicit-list-types-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME list_factories_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=list-factories-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME list_capacity_compatibility COMMAND ${CMAKE_COMMAND}
        "-DABC_RUNNER=$<TARGET_FILE:abc>" "-DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
        -DABC_FIXTURE_NAME=list-capacity-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME timing_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=timing-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME sequence_text_flatten_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=sequence-text-flatten-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME list_bulk_ranges_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=list-bulk-ranges-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME list_ranges_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=list-ranges-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME replace_copy_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=replace-copy-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME original_if_expression COMMAND $<TARGET_FILE:abc> run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/ifexpr2.pas")
    add_test(NAME original_question_expression COMMAND $<TARGET_FILE:abc> run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/question1.pas")
    add_test(NAME original_ternary_byte COMMAND $<TARGET_FILE:abc> run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/ternary_byte_value.pas")
    add_test(NAME original_question_biginteger COMMAND $<TARGET_FILE:abc> run "${CMAKE_CURRENT_SOURCE_DIR}/../TestSuite/question8.pas")
    add_test(NAME conditional_expressions_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=conditional-expressions-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME matrix_construction_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=matrix-construction-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME matrix_slices_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=matrix-slices-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME collection_callbacks_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=collection-callbacks-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME array_operations_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=array-operations-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME matrix_lines_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=matrix-lines-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME array_views_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=array-views-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME combinatorics_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=combinatorics-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME cartesian_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=cartesian-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME generators_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=generators-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME lambdas_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=lambdas-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME library_callables_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=library-callables-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME callable_values_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=callable-values-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    set_tests_properties(original_params1 original_constparam2 original_params5 original_const1 original_const3 original_delegates1 original_delegates2 original_delegates6 original_lambda_explicit_types original_lambda_local_const PROPERTIES TIMEOUT 10)
    add_test(NAME params_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=params-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME const_out_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=const-out-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME sequence_prefix_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=sequence-prefix-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
    add_test(NAME sequence_zip_compatibility COMMAND ${CMAKE_COMMAND}
        -DABC_RUNNER=$<TARGET_FILE:abc> -DABC_FIXTURE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
        -DABC_FIXTURE_NAME=sequence-zip-compat -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/stdlib.cmake")
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
        add_test(NAME qt_type_properties COMMAND abc-ide --smoke-type-properties -platform offscreen)
        set_tests_properties(qt_editor qt_runtime_error qt_type_properties PROPERTIES TIMEOUT 20)
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
