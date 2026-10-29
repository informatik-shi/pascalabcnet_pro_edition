# Audit only the staged application, never the developer's build tools.
if(NOT DEFINED ABC_PACKAGE_DIR OR NOT IS_DIRECTORY "${ABC_PACKAGE_DIR}")
    message(FATAL_ERROR "Set ABC_PACKAGE_DIR to the staged application directory")
endif()
file(GLOB_RECURSE entries LIST_DIRECTORIES true RELATIVE "${ABC_PACKAGE_DIR}" "${ABC_PACKAGE_DIR}/*")
foreach(entry IN LISTS entries)
    string(TOLOWER "${entry}" name)
    if(name MATCHES "[.](py|pyw|pyc|pyo|pyd|whl|egg)$"
        OR name MATCHES "(^|/)(python([0-9.]+)?([.]exe|[.]dll|[.]zip)?|python[0-9.]+[a-z]*[.]dll|libpython[^/]*|python[.]framework)(/|$)"
        OR name MATCHES "(^|/)(__pycache__|site-packages|dist-packages)(/|$)")
        message(FATAL_ERROR "Python runtime or module is forbidden in the application package: ${entry}")
    endif()
    if(name MATCHES "(^|/)(dotnet([.]exe)?|mono([.]exe)?|coreclr[.][^/]*|libcoreclr[.][^/]*|clr[.]dll|mscorlib[.]dll|system[.]private[.]corelib[.]dll|libmonosgen-[^/]*|mono-2[.]0[.][^/]*)(/|$)"
        OR name MATCHES "[.]runtimeconfig[.]json$")
        message(FATAL_ERROR ".NET/Mono runtime is forbidden in the application package: ${entry}")
    endif()
endforeach()
message(STATUS "Application package contains no Python modules or Python/.NET/Mono runtime hosts")
