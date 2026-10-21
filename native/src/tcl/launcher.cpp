#include "abc/core.hpp"
#include <filesystem>
#include <iostream>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#define ABC_TCL_IMPORT __declspec(dllimport)
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#define ABC_TCL_IMPORT
#else
#define ABC_TCL_IMPORT
#endif

// Opaque declarations of the stable Tcl/Tk 8.6 C ABI used by this launcher.
// No Tcl object layout or private API is used. See native/README.md.
extern "C" {
struct Tcl_Interp;
ABC_TCL_IMPORT void Tcl_FindExecutable(const char* argv0);
ABC_TCL_IMPORT Tcl_Interp* Tcl_CreateInterp();
ABC_TCL_IMPORT int Tcl_Init(Tcl_Interp* interp);
ABC_TCL_IMPORT const char* Tcl_SetVar(Tcl_Interp* interp, const char* name, const char* value, int flags);
ABC_TCL_IMPORT int Tcl_EvalFile(Tcl_Interp* interp, const char* filename);
ABC_TCL_IMPORT int Tcl_EvalEx(Tcl_Interp* interp, const char* script, int length, int flags);
ABC_TCL_IMPORT const char* Tcl_GetStringResult(Tcl_Interp* interp);
ABC_TCL_IMPORT void Tcl_DeleteInterp(Tcl_Interp* interp);
ABC_TCL_IMPORT int Tk_Init(Tcl_Interp* interp);
ABC_TCL_IMPORT void Tk_MainLoop();
}

namespace {
std::string utf8(const std::filesystem::path& path) {
    const auto value = path.u8string();
    return {value.begin(), value.end()};
}

std::filesystem::path executable(const char* argv0) {
#ifdef _WIN32
    std::wstring buffer(32768, L'\0');
    const auto size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (!size || size >= buffer.size()) throw std::runtime_error("Cannot find executable path");
    buffer.resize(size);
    return buffer;
#elif defined(__APPLE__)
    std::uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::string buffer(size, '\0');
    if (_NSGetExecutablePath(buffer.data(), &size) != 0) throw std::runtime_error("Cannot find executable path");
    return std::filesystem::canonical(buffer.c_str());
#else
    std::error_code error;
    auto path = std::filesystem::read_symlink("/proc/self/exe", error);
    return error ? std::filesystem::absolute(argv0) : path;
#endif
}

int launch(const char* argv0, bool smoke) {
    const auto exe = executable(argv0);
    const auto folder = exe.parent_path();
    const auto root = folder.parent_path();
    auto script = folder / "ui" / "editor.tcl";
    if (!std::filesystem::exists(script)) script = root / "share" / "abc" / "editor.tcl";
    const auto tcl = root / "lib" / "tcl8.6";
    const auto tk = root / "lib" / "tk8.6";
    if (!std::filesystem::exists(script) || !std::filesystem::exists(tcl / "init.tcl")
        || !std::filesystem::exists(tk / "tk.tcl")) throw std::runtime_error("Portable Tcl/Tk runtime or editor script is missing");
    const auto exe_text = utf8(exe);
    Tcl_FindExecutable(exe_text.c_str());
    auto* interp = Tcl_CreateInterp();
    if (!interp) throw std::runtime_error("Cannot create Tcl interpreter");
    constexpr int global_only = 1;
    auto set = [&](const char* name, const std::string& value) {
        if (!Tcl_SetVar(interp, name, value.c_str(), global_only)) throw std::runtime_error("Cannot initialize Tcl variable");
    };
    set("tcl_library", utf8(tcl));
    set("tk_library", utf8(tk));
    set("abc_runner", utf8(folder /
#ifdef _WIN32
        "abc.exe"
#else
        "abc"
#endif
        ));
    set("abc_smoke", smoke ? "1" : "0");
    if (Tcl_Init(interp) != 0 || Tcl_EvalEx(interp, "encoding system utf-8", -1, 0) != 0
        || Tk_Init(interp) != 0 || Tcl_EvalFile(interp, utf8(script).c_str()) != 0) {
        const std::string message = Tcl_GetStringResult(interp);
        Tcl_DeleteInterp(interp);
        throw std::runtime_error(message);
    }
    Tk_MainLoop();
    Tcl_DeleteInterp(interp);
    return 0;
}
} // namespace

#ifdef _WIN32
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    int count = 0;
    auto** arguments = CommandLineToArgvW(GetCommandLineW(), &count);
    bool smoke = false;
    if (arguments) {
        for (int i = 1; i < count; ++i) if (std::wstring_view(arguments[i]) == L"--smoke-test") smoke = true;
        LocalFree(arguments);
    }
    try { return launch("abc-tk.exe", smoke); }
    catch (const std::exception& error) {
        if (!smoke) {
            const auto text = abc::from_utf8(error.what());
            const std::wstring message(text.begin(), text.end());
            MessageBoxW(nullptr, message.c_str(), L"ABC Native Tcl/Tk", MB_ICONERROR);
        }
        return 1;
    }
}
#else
int main(int argc, char** argv) {
    try { return launch(argv[0], argc > 1 && std::string_view(argv[1]) == "--smoke-test"); }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
#endif
