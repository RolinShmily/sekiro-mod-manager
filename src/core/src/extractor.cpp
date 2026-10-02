#include "smm/extractor.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#include <miniz.h>

#include "smm/error.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <array>
#endif

namespace smm {
namespace {

std::string to_lower_ascii(std::string_view text) {
    std::string lower;
    lower.reserve(text.size());
    for (const unsigned char c : text) {
        lower.push_back(static_cast<char>(std::tolower(c)));
    }
    return lower;
}

bool starts_with(std::string_view text, std::string_view prefix) {
    return text.size() >= prefix.size() && text.compare(0, prefix.size(), prefix) == 0;
}

/// Opens a file for reading without going through the narrow-char Win32 path, so archives
/// whose names contain non-ASCII characters (routine for mod packs) still open.
std::FILE* open_read(const fs::path& path) {
#ifdef _WIN32
    std::FILE* file = nullptr;
    if (::_wfopen_s(&file, path.wstring().c_str(), L"rb") != 0) {
        return nullptr;
    }
    return file;
#else
    return std::fopen(path.string().c_str(), "rb");
#endif
}

size_t file_write_callback(void* opaque, mz_uint64 /*file_ofs*/, const void* buffer, size_t size) {
    auto* stream = static_cast<std::FILE*>(opaque);
    return std::fwrite(buffer, 1, size, stream);
}

/// Converts an archive entry name to a path.
/// Zip stores names as UTF-8 when its language-encoding flag is set, which is what every
/// modern packer does - and Sekiro packages routinely use non-ASCII directory names.
fs::path entry_name_to_path(const char* name) {
#ifdef _WIN32
    if (name == nullptr) {
        return {};
    }
    const int length = ::MultiByteToWideChar(CP_UTF8, 0, name, -1, nullptr, 0);
    if (length <= 1) {
        return fs::path(name);
    }
    std::wstring wide(static_cast<std::size_t>(length), L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, name, -1, wide.data(), length);
    wide.resize(static_cast<std::size_t>(length - 1));
    return fs::path(wide);
#else
    return fs::path(name == nullptr ? "" : name);
#endif
}

/// Rejects entries that would be written outside the extraction directory.
/// A malicious or malformed archive using "../" or an absolute name must never escape.
bool is_safe_entry_path(const fs::path& relative) {
    if (relative.is_absolute() || relative.has_root_name() || relative.has_root_directory()) {
        return false;
    }
    for (const auto& part : relative) {
        if (part == "..") {
            return false;
        }
    }
    return !relative.empty();
}

void create_parent_directories(const fs::path& path) {
    const fs::path parent = path.parent_path();
    if (parent.empty()) {
        return;
    }
    std::error_code ec;
    fs::create_directories(parent, ec);
    if (ec) {
        fail(ErrorCode::ExtractionError,
             "Cannot create the extraction directory: " + ec.message(), parent);
    }
}

struct ProcessResult {
    bool started{false};
    int exit_code{-1};
    std::string output;
};

#ifdef _WIN32

/// Quotes one argument for the Windows command line.
std::wstring quote_argument(const std::wstring& argument) {
    if (!argument.empty() && argument.find_first_of(L" \t\n\v\"") == std::wstring::npos) {
        return argument;
    }

    std::wstring quoted = L"\"";
    for (auto it = argument.begin();;) {
        unsigned backslashes = 0;
        while (it != argument.end() && *it == L'\\') {
            ++it;
            ++backslashes;
        }
        if (it == argument.end()) {
            quoted.append(backslashes * 2, L'\\');
            break;
        }
        if (*it == L'"') {
            quoted.append(backslashes * 2 + 1, L'\\');
            quoted.push_back(*it);
        } else {
            quoted.append(backslashes, L'\\');
            quoted.push_back(*it);
        }
        ++it;
    }
    quoted.push_back(L'"');
    return quoted;
}

/// Runs \`exe\` with \`args\` and captures stdout+stderr.
/// Arguments are passed as a vector and quoted individually: never a shell string, so an
/// archive or directory name containing spaces or quotes cannot turn into a second command.
ProcessResult run_captured(const fs::path& exe, const std::vector<std::wstring>& args) {
    ProcessResult result;

    std::wstring command_line = quote_argument(exe.wstring());
    for (const auto& arg : args) {
        command_line.push_back(L' ');
        command_line.append(quote_argument(arg));
    }
    std::vector<wchar_t> mutable_command(command_line.begin(), command_line.end());
    mutable_command.push_back(L'\0');

    SECURITY_ATTRIBUTES attributes{};
    attributes.nLength = sizeof(attributes);
    attributes.bInheritHandle = TRUE;

    HANDLE read_pipe = nullptr;
    HANDLE write_pipe = nullptr;
    if (::CreatePipe(&read_pipe, &write_pipe, &attributes, 0) == 0) {
        return result;
    }
    ::SetHandleInformation(read_pipe, HANDLE_FLAG_INHERIT, 0);

    HANDLE nul = ::CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                               &attributes, OPEN_EXISTING, 0, nullptr);

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = nul;
    startup.hStdOutput = write_pipe;
    startup.hStdError = write_pipe;

    PROCESS_INFORMATION process{};
    const BOOL created =
        ::CreateProcessW(exe.wstring().c_str(), mutable_command.data(), nullptr, nullptr, TRUE,
                         CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process);

    ::CloseHandle(write_pipe);
    if (nul != INVALID_HANDLE_VALUE) {
        ::CloseHandle(nul);
    }

    if (created == 0) {
        ::CloseHandle(read_pipe);
        return result;
    }

    std::array<char, 4096> buffer{};
    DWORD read = 0;
    while (::ReadFile(read_pipe, buffer.data(), static_cast<DWORD>(buffer.size()), &read, nullptr) != 0 &&
           read > 0) {
        result.output.append(buffer.data(), read);
    }
    ::CloseHandle(read_pipe);

    ::WaitForSingleObject(process.hProcess, INFINITE);
    DWORD exit_code = 0;
    ::GetExitCodeProcess(process.hProcess, &exit_code);
    ::CloseHandle(process.hThread);
    ::CloseHandle(process.hProcess);

    result.started = true;
    result.exit_code = static_cast<int>(exit_code);
    return result;
}

#else

ProcessResult run_captured(const fs::path& exe, const std::vector<std::wstring>& args) {
    ProcessResult result;
    std::string command = "'" + path_to_utf8(exe) + "'";
    for (const auto& arg : args) {
        std::string narrow;
        narrow.reserve(arg.size());
        for (const wchar_t c : arg) {
            narrow.push_back(static_cast<char>(c));
        }
        command += " '" + narrow + "'";
    }
    command += " 2>&1";

    std::FILE* pipe = ::popen(command.c_str(), "r");
    if (pipe == nullptr) {
        return result;
    }
    std::array<char, 4096> buffer{};
    while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        result.output.append(buffer.data());
    }
    const int status = ::pclose(pipe);
    result.started = true;
    result.exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    return result;
}

#endif

/// Last few lines of a tool's output, so the error message stays readable.
std::string tail_of(const std::string& text, std::size_t max_chars = 400) {
    std::string trimmed = text;
    while (!trimmed.empty() && (trimmed.back() == '\n' || trimmed.back() == '\r' ||
                                trimmed.back() == ' ' || trimmed.back() == '\t')) {
        trimmed.pop_back();
    }
    if (trimmed.size() <= max_chars) {
        return trimmed;
    }
    return "..." + trimmed.substr(trimmed.size() - max_chars);
}

/// Searches PATH plus the conventional install locations for the first matching binary.
fs::path find_in_paths(const std::vector<std::string>& binary_names,
                       const std::vector<fs::path>& extra_dirs) {
    std::error_code ec;

    std::vector<fs::path> directories;
    if (const char* path_env = std::getenv("PATH"); path_env != nullptr) {
#ifdef _WIN32
        std::string path_value(path_env);
        std::size_t start = 0;
        while (start <= path_value.size()) {
            const std::size_t end = path_value.find(';', start);
            const std::string item = path_value.substr(start, end - start);
            if (!item.empty()) {
                directories.emplace_back(item);
            }
            if (end == std::string::npos) {
                break;
            }
            start = end + 1;
        }
#else
        std::string path_value(path_env);
        std::size_t start = 0;
        while (start <= path_value.size()) {
            const std::size_t end = path_value.find(':', start);
            const std::string item = path_value.substr(start, end - start);
            if (!item.empty()) {
                directories.emplace_back(item);
            }
            if (end == std::string::npos) {
                break;
            }
            start = end + 1;
        }
#endif
    }
    directories.insert(directories.end(), extra_dirs.begin(), extra_dirs.end());

    for (const fs::path& directory : directories) {
        for (const std::string& name : binary_names) {
            const fs::path candidate = directory / name;
            if (fs::is_regular_file(candidate, ec) && !ec) {
                return candidate;
            }
        }
    }
    return {};
}

std::vector<fs::path> standard_install_dirs(const char* const* env_names,
                                            const std::vector<std::string>& suffixes) {
    std::vector<fs::path> dirs;
    for (const char* const* name = env_names; *name != nullptr; ++name) {
        const char* value = std::getenv(*name);
        if (value == nullptr || *value == '\0') {
            continue;
        }
        for (const std::string& suffix : suffixes) {
            dirs.push_back(fs::path(value) / suffix);
        }
    }
    return dirs;
}

/// Extracts \`archive\` with an external tool, translating failures into actionable errors.
void extract_with_external(const fs::path& tool, const fs::path& archive, const fs::path& dest_dir,
                           const std::vector<std::wstring>& extra_args, const char* tool_label) {
    std::vector<std::wstring> args = extra_args;
    const std::wstring out_arg = L"-o" + dest_dir.wstring();
    args.push_back(out_arg);
    args.push_back(archive.wstring());

    const ProcessResult result = run_captured(tool, args);
    if (!result.started) {
        fail(ErrorCode::ExtractionError,
             std::string("Failed to execute ") + tool_label + " (" + path_to_utf8(tool) + ").", tool);
    }
    if (result.exit_code != 0) {
        const std::string detail = tail_of(result.output);
        fail(ErrorCode::ExtractionError,
             std::string(tool_label) + " failed with exit code " +
                 std::to_string(result.exit_code) +
                 (detail.empty() ? std::string{} : ": " + detail),
             archive);
    }
}

} // namespace

std::string to_string(ArchiveFormat format) {
    switch (format) {
        case ArchiveFormat::Zip: return "zip";
        case ArchiveFormat::SevenZip: return "7z";
        case ArchiveFormat::Rar: return "rar";
        case ArchiveFormat::Unknown: return "unknown";
    }
    return "unknown";
}

ArchiveFormat detect_archive_format(const fs::path& path) {
    std::error_code ec;
    if (fs::is_directory(path, ec) && !ec) {
        return ArchiveFormat::Unknown;
    }

    const std::string extension = to_lower_ascii(path_to_utf8(path.extension()));
    if (extension == ".zip") {
        return ArchiveFormat::Zip;
    }
    if (extension == ".7z") {
        return ArchiveFormat::SevenZip;
    }
    if (extension == ".rar") {
        return ArchiveFormat::Rar;
    }

    // Extensionless or mislabelled downloads are common; the magic bytes decide.
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        return ArchiveFormat::Unknown;
    }
    unsigned char magic[8] = {};
    stream.read(reinterpret_cast<char*>(magic), sizeof(magic));
    const std::streamsize read = stream.gcount();

    if (read >= 2 && magic[0] == 0x50 && magic[1] == 0x4B) {
        return ArchiveFormat::Zip;
    }
    if (read >= 6 && magic[0] == 0x37 && magic[1] == 0x7A && magic[2] == 0xBC && magic[3] == 0xAF &&
        magic[4] == 0x27 && magic[5] == 0x1C) {
        return ArchiveFormat::SevenZip;
    }
    if (read >= 4 && magic[0] == 0x52 && magic[1] == 0x61 && magic[2] == 0x72 && magic[3] == 0x21) {
        return ArchiveFormat::Rar;
    }
    return ArchiveFormat::Unknown;
}

bool is_supported_archive(const fs::path& path) {
    return detect_archive_format(path) != ArchiveFormat::Unknown;
}

fs::path find_external_7z() {
    const std::vector<std::string> names = {"7z.exe", "7za.exe", "7zr.exe", "7z", "7za", "7zr"};

    std::vector<fs::path> extra;
    const char* env_names[] = {"ProgramFiles", "ProgramFiles(x86)", "ProgramW6432", nullptr};
    for (const fs::path& dir : standard_install_dirs(env_names, {"7-Zip"})) {
        extra.push_back(dir);
    }
    if (const char* local = std::getenv("LOCALAPPDATA"); local != nullptr && *local != '\0') {
        extra.push_back(fs::path(local) / "Programs" / "7-Zip");
    }
    extra.emplace_back("C:/Program Files/7-Zip");
    extra.emplace_back("C:/Program Files (x86)/7-Zip");
    extra.emplace_back("/usr/bin");
    extra.emplace_back("/usr/local/bin");

    return find_in_paths(names, extra);
}

fs::path find_external_unrar() {
    const std::vector<std::string> names = {"unrar.exe", "rar.exe", "unrar", "rar"};

    std::vector<fs::path> extra;
    const char* env_names[] = {"ProgramFiles", "ProgramFiles(x86)", "ProgramW6432", nullptr};
    for (const fs::path& dir : standard_install_dirs(env_names, {"WinRAR"})) {
        extra.push_back(dir);
    }
    extra.emplace_back("C:/Program Files/WinRAR");
    extra.emplace_back("C:/Program Files (x86)/WinRAR");

    return find_in_paths(names, extra);
}

void extract_zip(const fs::path& archive, const fs::path& dest_dir,
                 const ExtractProgressCallback& on_progress) {
    std::FILE* file = open_read(archive);
    if (file == nullptr) {
        fail(ErrorCode::Io, "Cannot open the archive for reading", archive);
    }

    std::error_code ec;
    const uint64_t archive_size = static_cast<uint64_t>(fs::file_size(archive, ec));
    if (ec) {
        std::fclose(file);
        fail(ErrorCode::Io, "Cannot determine the archive size: " + ec.message(), archive);
    }

    mz_zip_archive zip{};
    if (mz_zip_reader_init_cfile(&zip, file, archive_size, 0) == MZ_FALSE) {
        std::fclose(file);
        fail(ErrorCode::Archive, "Not a readable ZIP archive (it may be truncated)", archive);
    }

    const mz_uint entries = mz_zip_reader_get_num_files(&zip);
    for (mz_uint index = 0; index < entries; ++index) {
        mz_zip_archive_file_stat info{};
        if (mz_zip_reader_file_stat(&zip, index, &info) == MZ_FALSE) {
            continue;
        }

        if (on_progress) {
            on_progress(index, entries);
        }

        const fs::path relative = entry_name_to_path(info.m_filename);
        if (!is_safe_entry_path(relative)) {
            // Silently skipping is deliberate: refusing the whole archive over one hostile
            // entry would let a single junk path block an otherwise good mod.
            continue;
        }

        const fs::path target = dest_dir / relative;
        if (info.m_is_directory) {
            fs::create_directories(target, ec);
            continue;
        }

        create_parent_directories(target);

        std::FILE* out = nullptr;
#ifdef _WIN32
        if (::_wfopen_s(&out, target.wstring().c_str(), L"wb") != 0) {
            out = nullptr;
        }
#else
        out = std::fopen(target.string().c_str(), "wb");
#endif
        if (out == nullptr) {
            mz_zip_reader_end(&zip);
            std::fclose(file);
            fail(ErrorCode::ExtractionError, "Cannot create the extracted file", target);
        }

        const mz_bool ok = mz_zip_reader_extract_to_callback(&zip, index, file_write_callback,
                                                            out, 0);
        std::fclose(out);

        if (ok == MZ_FALSE) {
            mz_zip_reader_end(&zip);
            std::fclose(file);
            fail(ErrorCode::ExtractionError, "Failed to extract an entry from the ZIP archive",
                 target);
        }
    }

    if (on_progress) {
        on_progress(entries, entries);
    }

    mz_zip_reader_end(&zip);
    std::fclose(file);
}

void extract_7z(const fs::path& archive, const fs::path& dest_dir) {
    const fs::path tool = find_external_7z();
    if (tool.empty()) {
        fail(ErrorCode::ExtractionError,
             "Extracting .7z archives needs 7-Zip, which was not found. Install it from "
             "https://www.7-zip.org and try again.",
             archive);
    }

    std::error_code ec;
    fs::create_directories(dest_dir, ec);
    extract_with_external(tool, archive, dest_dir, {L"x", L"-y", L"-bso0", L"-bsp0"}, "7-Zip");
}

void extract_rar(const fs::path& archive, const fs::path& dest_dir) {
    std::error_code ec;
    fs::create_directories(dest_dir, ec);

    // 7-Zip understands RAR far more reliably than the (often absent) WinRAR CLI, so it is
    // tried first when both are installed.
    if (const fs::path seven_zip = find_external_7z(); !seven_zip.empty()) {
        extract_with_external(seven_zip, archive, dest_dir, {L"x", L"-y", L"-bso0", L"-bsp0"},
                              "7-Zip");
        return;
    }

    if (const fs::path unrar = find_external_unrar(); !unrar.empty()) {
        std::vector<std::wstring> args = {L"x", L"-y"};
        const std::wstring out_arg = dest_dir.wstring() + L"/";
        args.push_back(out_arg);
        args.push_back(archive.wstring());

        const ProcessResult result = run_captured(unrar, args);
        if (!result.started) {
            fail(ErrorCode::ExtractionError,
                 "Failed to execute the unrar tool (" + path_to_utf8(unrar) + ").", unrar);
        }
        if (result.exit_code != 0) {
            const std::string detail = tail_of(result.output);
            fail(ErrorCode::ExtractionError,
                 "unrar failed with exit code " + std::to_string(result.exit_code) +
                     (detail.empty() ? std::string{} : ": " + detail),
                 archive);
        }
        return;
    }

    fail(ErrorCode::ExtractionError,
         "Extracting .rar archives needs 7-Zip or WinRAR, neither of which was found. Install "
         "7-Zip from https://www.7-zip.org and try again.",
         archive);
}

void extract_archive(const fs::path& archive, const fs::path& dest_dir,
                     const ExtractProgressCallback& on_progress) {
    std::error_code ec;
    if (!fs::exists(archive, ec)) {
        fail(ErrorCode::Io, "Archive file not found", archive);
    }

    const ArchiveFormat format = detect_archive_format(archive);
    if (format == ArchiveFormat::Unknown) {
        fail(ErrorCode::UnsupportedArchive,
             "Unrecognised archive format. SMM can open .zip, .7z and .rar files, or an already "
             "unpacked directory.",
             archive);
    }

    fs::create_directories(dest_dir, ec);

    switch (format) {
        case ArchiveFormat::Zip:
            extract_zip(archive, dest_dir, on_progress);
            break;
        case ArchiveFormat::SevenZip:
            extract_7z(archive, dest_dir);
            break;
        case ArchiveFormat::Rar:
            extract_rar(archive, dest_dir);
            break;
        case ArchiveFormat::Unknown:
            break;
    }
}

} // namespace smm
