#include "files.hpp"

#include <cerrno>
#include <cstdio>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#ifdef __linux__
#include <fcntl.h>
#include <linux/fs.h>
#include <sys/syscall.h>
#endif
#ifdef __APPLE__
#include <sys/stdio.h>
#endif
#endif

namespace abc {
namespace {
namespace fs=std::filesystem;
fs::path trim_separator(fs::path path) {
    while (path.filename().empty() && path!=path.root_path()) path=path.parent_path();
    return path;
}

bool move_exclusive(const fs::path& source, const fs::path& destination, bool file) {
#ifdef _WIN32
    // Do not pass MOVEFILE_REPLACE_EXISTING: the OS checks the destination atomically.
    return ::MoveFileExW(source.c_str(),destination.c_str(),file ? MOVEFILE_COPY_ALLOWED : 0)!=0;
#else
    int failure=ENOSYS;
#if defined(__linux__) && defined(SYS_renameat2)
    if (::syscall(SYS_renameat2,AT_FDCWD,source.c_str(),AT_FDCWD,destination.c_str(),RENAME_NOREPLACE)==0) return true;
    failure=errno;
#elif defined(__APPLE__)
    if (::renamex_np(source.c_str(),destination.c_str(),RENAME_EXCL)==0) return true;
    failure=errno;
#endif
    if (failure==EEXIST && (file || source.filename()!=destination.filename())) {
        // Unix File.Move accepts two hard links to the same inode as a no-op.
        // lstat keeps distinct symbolic links distinct, even with the same target.
        struct stat a{}, b{};
        if (::lstat(source.c_str(),&a)==0 && ::lstat(destination.c_str(),&b)==0
            && a.st_dev==b.st_dev && a.st_ino==b.st_ino) return true;
    }
    if (!file) return false;
    if (failure==ENOSYS || failure==EINVAL || failure==EOPNOTSUPP || failure==ENOTSUP) {
        // Older filesystems may not implement exclusive rename. A new hard link
        // still reserves the destination without replacing anything already there.
        if (::link(source.c_str(),destination.c_str())==0) return ::unlink(source.c_str())==0;
        failure=errno;
    }
    if (failure==EXDEV || failure==EACCES || failure==EPERM || failure==EOPNOTSUPP || failure==ENOTSUP || failure==EMLINK || failure==ENOSYS) {
        std::error_code error;
        if (!fs::copy_file(source,destination,fs::copy_options::none,error) || error) return false;
        return ::unlink(source.c_str())==0;
    }
    return false;
#endif
}
}

bool FileContext::rename(std::u16string_view source, std::u16string_view destination, bool directory) {
    try {
        auto from=resolve(source,{}), to=resolve(destination,{});
        std::error_code error;
        if (!directory) {
            if (from.filename().empty() || to.filename().empty() || !fs::is_regular_file(from,error) || error) return false;
            if (from==to) return true;
            if (text_file_locked(from)) return false;
        } else {
            // Directory.Move also accepts files, but a trailing source separator
            // explicitly requires a directory. Destination separators are trimmed.
            if (from.filename().empty() && !fs::is_directory(from,error)) return false;
            from=trim_separator(std::move(from)); to=trim_separator(std::move(to));
            if (from==to || !fs::exists(fs::symlink_status(from,error)) || error) return false;
            if (text_file_locked(from,true)) return false;
        }
        std::optional<fs::path> next_directory;
        if (directory && !fs::is_symlink(from,error) && fs::is_directory(from,error)) {
            auto cursor=current_;
            fs::path suffix;
            while (!cursor.empty()) {
                error.clear();
                if (fs::equivalent(cursor,from,error) && !error) {
#ifdef _WIN32
                    // Match the Windows lock on the current directory and its ancestors.
                    return false;
#else
                    const auto parent=fs::canonical(to.parent_path(),error);
                    if (error) return false;
                    next_directory=trim_separator((parent/to.filename()/suffix).lexically_normal());
                    break;
#endif
                }
                const auto parent=cursor.parent_path();
                if (parent==cursor) break;
                suffix=cursor.filename()/suffix; cursor=parent;
            }
        }
        if (!move_exclusive(from,to,!directory)) return false;
        if (next_directory) current_=std::move(*next_directory);
        return true;
    } catch (const Error& error) {
        if (error.code!="E_IO") throw;
        return false;
    } catch (const fs::filesystem_error&) { return false; }
}
} // namespace abc
