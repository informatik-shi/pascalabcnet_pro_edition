cmake_minimum_required(VERSION 3.24)
project(ABCNative VERSION 0.1.0 LANGUAGES CXX)

option(ABC_BUILD_IDE "Build the Qt desktop editor" OFF)
option(ABC_BUILD_TK "Build the Tcl/Tk comparison editor" OFF)
option(ABC_SANITIZERS "Enable address and undefined behavior sanitizers" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(abc_core
    src/stdlib.cpp
    src/files.cpp
    src/file_moves.cpp
    src/text_file.cpp
    src/stdlib_text.cpp
    src/diagnostics.cpp
    src/types.cpp
    src/pascal.cpp
    src/bytecode.cpp
    src/vm.cpp)
target_include_directories(abc_core PUBLIC include)
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
    add_test(NAME core COMMAND abc_tests)
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
endif()

install(TARGETS abc RUNTIME DESTINATION bin)
install(DIRECTORY examples/ DESTINATION examples)
install(FILES README.md DESTINATION .)
install(FILES stdlib-status.json DESTINATION .)
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
