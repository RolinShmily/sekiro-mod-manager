#include "smm/manager.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <system_error>

#include "smm/error.hpp"
#include "smm/loader.hpp"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <wincodec.h>

#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")

namespace {

std::string to_lower_ascii(std::string_view text) {
    std::string lower(text);
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return lower;
}

bool is_webp_header(const smm::fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f) return false;
    char header[12];
    if (f.read(header, 12)) {
        return header[0] == 'R' && header[1] == 'I' && header[2] == 'F' && header[3] == 'F' &&
               header[8] == 'W' && header[9] == 'E' && header[10] == 'B' && header[11] == 'P';
    }
    return false;
}

bool convert_to_standard_png_wic(const smm::fs::path& src_path, const smm::fs::path& dst_path) {
    CoInitialize(NULL);
    IWICImagingFactory* pFactory = NULL;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFactory));
    if (FAILED(hr)) return false;

    IWICBitmapDecoder* pDecoder = NULL;
    hr = pFactory->CreateDecoderFromFilename(src_path.c_str(), NULL, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &pDecoder);
    if (FAILED(hr)) { pFactory->Release(); return false; }

    IWICBitmapFrameDecode* pFrame = NULL;
    hr = pDecoder->GetFrame(0, &pFrame);
    if (FAILED(hr)) { pDecoder->Release(); pFactory->Release(); return false; }

    IWICFormatConverter* pConverter = NULL;
    hr = pFactory->CreateFormatConverter(&pConverter);
    if (FAILED(hr)) { pFrame->Release(); pDecoder->Release(); pFactory->Release(); return false; }

    hr = pConverter->Initialize(pFrame, GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, NULL, 0.0, WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) { pConverter->Release(); pFrame->Release(); pDecoder->Release(); pFactory->Release(); return false; }

    IWICStream* pStream = NULL;
    hr = pFactory->CreateStream(&pStream);
    if (FAILED(hr)) { pConverter->Release(); pFrame->Release(); pDecoder->Release(); pFactory->Release(); return false; }

    hr = pStream->InitializeFromFilename(dst_path.c_str(), GENERIC_WRITE);
    if (FAILED(hr)) { pStream->Release(); pConverter->Release(); pFrame->Release(); pDecoder->Release(); pFactory->Release(); return false; }

    IWICBitmapEncoder* pEncoder = NULL;
    hr = pFactory->CreateEncoder(GUID_ContainerFormatPng, NULL, &pEncoder);
    if (FAILED(hr)) { pStream->Release(); pConverter->Release(); pFrame->Release(); pDecoder->Release(); pFactory->Release(); return false; }

    hr = pEncoder->Initialize(pStream, WICBitmapEncoderNoCache);
    if (FAILED(hr)) { pEncoder->Release(); pStream->Release(); pConverter->Release(); pFrame->Release(); pDecoder->Release(); return false; }

    IWICBitmapFrameEncode* pOutFrame = NULL;
    hr = pEncoder->CreateNewFrame(&pOutFrame, NULL);
    if (FAILED(hr)) { pEncoder->Release(); pStream->Release(); pConverter->Release(); pFrame->Release(); pDecoder->Release(); return false; }

    hr = pOutFrame->Initialize(NULL);
    if (FAILED(hr)) { pOutFrame->Release(); pEncoder->Release(); pStream->Release(); pConverter->Release(); pFrame->Release(); pDecoder->Release(); return false; }

    UINT width = 0, height = 0;
    pFrame->GetSize(&width, &height);
    pOutFrame->SetSize(width, height);

    WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA;
    pOutFrame->SetPixelFormat(&format);

    hr = pOutFrame->WriteSource(pConverter, NULL);
    if (SUCCEEDED(hr)) {
        pOutFrame->Commit();
        pEncoder->Commit();
    }

    pOutFrame->Release();
    pEncoder->Release();
    pStream->Release();
    pConverter->Release();
    pFrame->Release();
    pDecoder->Release();
    pFactory->Release();
    CoUninitialize();
    return SUCCEEDED(hr);
}

} // namespace
#endif

namespace smm {

fs::path ModManager::find_mod_dir(const fs::path& staging_dir, const std::string& mod_id) {
    std::error_code ec;
    if (!fs::exists(staging_dir, ec)) {
        fail(ErrorCode::Io,
             "Staging directory does not exist: " + path_to_utf8(staging_dir), staging_dir);
    }

    // Fast path: the directory is named after the id.
    const fs::path direct = staging_dir / mod_id;
    if (ModLoader::is_mod_directory(direct)) {
        try {
            const ModInfo info = ModLoader::load_mod_info(direct);
            if (info.id == mod_id) {
                return direct;
            }
        } catch (const SmmError&) {
            // Fall through to the full scan: a broken direct hit must not shadow a good one.
        }
    }

    fs::path case_insensitive_candidate;
    for (const auto& entry : fs::directory_iterator(staging_dir, ec)) {
        if (ec) {
            break;
        }
        const fs::path path = entry.path();
        if (!ModLoader::is_mod_directory(path)) {
            continue;
        }
        ModInfo info;
        try {
            info = ModLoader::load_mod_info(path);
        } catch (const SmmError&) {
            continue;
        }
        if (info.id == mod_id) {
            return path;
        }
        if (case_insensitive_candidate.empty() && info.id.size() == mod_id.size() &&
            std::equal(info.id.begin(), info.id.end(), mod_id.begin(), [](char a, char b) {
                return std::tolower(static_cast<unsigned char>(a)) ==
                       std::tolower(static_cast<unsigned char>(b));
            })) {
            case_insensitive_candidate = path;
        }
    }

    if (!case_insensitive_candidate.empty()) {
        return case_insensitive_candidate;
    }

    fail(ErrorCode::ModNotFound, "Mod '" + mod_id + "' not found in staging", staging_dir);
}

ModInfo ModManager::load_mod_info(const fs::path& mod_dir) {
    return ModLoader::load_mod_info(mod_dir);
}

ModInfo ModManager::set_mod_enabled(const fs::path& staging_dir, const std::string& mod_id, bool enabled) {
    const fs::path mod_dir = find_mod_dir(staging_dir, mod_id);
    ModInfo info = ModLoader::load_mod_info(mod_dir);
    info.enabled = enabled;
    save_mod_info(mod_dir, info);
    info.root_path = mod_dir;
    return info;
}

ModInfo ModManager::set_mod_priority(const fs::path& staging_dir, const std::string& mod_id, uint32_t priority) {
    const fs::path mod_dir = find_mod_dir(staging_dir, mod_id);
    ModInfo info = ModLoader::load_mod_info(mod_dir);
    info.priority = priority;
    save_mod_info(mod_dir, info);
    info.root_path = mod_dir;
    return info;
}

ModInfo ModManager::set_asset_enabled(const fs::path& staging_dir, const std::string& mod_id,
                                      const std::string& rel_path, bool enabled) {
    const fs::path mod_dir = find_mod_dir(staging_dir, mod_id);
    ModInfo info = ModLoader::load_mod_info(mod_dir);

    std::string norm_path = rel_path;
    std::replace(norm_path.begin(), norm_path.end(), '\\', '/');

    auto it = std::find(info.disabled_assets.begin(), info.disabled_assets.end(), norm_path);
    if (!enabled) {
        if (it == info.disabled_assets.end()) {
            info.disabled_assets.push_back(norm_path);
        }
    } else {
        if (it != info.disabled_assets.end()) {
            info.disabled_assets.erase(it);
        }
    }

    save_mod_info(mod_dir, info);
    info.root_path = mod_dir;
    return info;
}

#ifdef _WIN32
static bool run_silent_cmd(const std::wstring& cmd) {
    STARTUPINFOW si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    std::vector<wchar_t> cmd_buf(cmd.begin(), cmd.end());
    cmd_buf.push_back(L'\0');

    if (!CreateProcessW(NULL, cmd_buf.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        return false;
    }

    WaitForSingleObject(pi.hProcess, 15000);
    DWORD exit_code = 1;
    GetExitCodeProcess(pi.hProcess, &exit_code);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return exit_code == 0;
}
#endif

bool ModManager::compress_image_to_webp(const fs::path& src_path, const fs::path& dst_path,
                                       int max_width, int max_height, int quality) {
    std::error_code ec;
    if (!fs::exists(src_path, ec)) return false;

    const fs::path temp_dst = dst_path.wstring() + L".tmp.webp";
    fs::remove(temp_dst, ec);

#ifdef _WIN32
    // 方案 1: ImageMagick (magick) 优先，支持所有常见格式自动降采样高质量编码 WebP
    const std::wstring magick_cmd = L"magick \"" + src_path.wstring() + L"\" -auto-orient -resize "
                                  + std::to_wstring(max_width) + L"x" + std::to_wstring(max_height)
                                  + L"> -quality " + std::to_wstring(quality) + L" \"" + temp_dst.wstring() + L"\"";
    if (run_silent_cmd(magick_cmd) && fs::exists(temp_dst, ec) && fs::file_size(temp_dst, ec) > 0) {
        fs::rename(temp_dst, dst_path, ec);
        if (!ec) return true;
        fs::copy_file(temp_dst, dst_path, fs::copy_options::overwrite_existing, ec);
        fs::remove(temp_dst, ec);
        return !ec;
    }

    // 方案 2: FFmpeg (ffmpeg)，同样原生内置 libwebp 编码支持
    const std::wstring ffmpeg_cmd = L"ffmpeg -y -i \"" + src_path.wstring() + L"\" -vf \"scale='min("
                                  + std::to_wstring(max_width) + L",iw)':-1\" -c:v libwebp -quality "
                                  + std::to_wstring(quality) + L" \"" + temp_dst.wstring() + L"\"";
    if (run_silent_cmd(ffmpeg_cmd) && fs::exists(temp_dst, ec) && fs::file_size(temp_dst, ec) > 0) {
        fs::rename(temp_dst, dst_path, ec);
        if (!ec) return true;
        fs::copy_file(temp_dst, dst_path, fs::copy_options::overwrite_existing, ec);
        fs::remove(temp_dst, ec);
        return !ec;
    }

    // 方案 3: cwebp (官方 WebP 独立 CLI)
    const std::wstring cwebp_cmd = L"cwebp -resize " + std::to_wstring(max_width) + L" 0 -q "
                                 + std::to_wstring(quality) + L" \"" + src_path.wstring() + L"\" -o \"" + temp_dst.wstring() + L"\"";
    if (run_silent_cmd(cwebp_cmd) && fs::exists(temp_dst, ec) && fs::file_size(temp_dst, ec) > 0) {
        fs::rename(temp_dst, dst_path, ec);
        if (!ec) return true;
        fs::copy_file(temp_dst, dst_path, fs::copy_options::overwrite_existing, ec);
        fs::remove(temp_dst, ec);
        return !ec;
    }
#endif

    // 兜底保障：若源图片本身已是 WebP，直接规范化拷贝
    const std::string ext = to_lower_ascii(path_to_utf8(src_path.extension()));
    if (ext == ".webp" || is_webp_header(src_path)) {
        if (!fs::equivalent(src_path, dst_path, ec)) {
            fs::copy_file(src_path, dst_path, fs::copy_options::overwrite_existing, ec);
        }
        return !ec;
    }

    return false;
}

ModInfo ModManager::set_mod_preview(const fs::path& staging_dir, const std::string& mod_id,
                                    const fs::path& image_src_path) {
    const fs::path mod_dir = find_mod_dir(staging_dir, mod_id);
    std::error_code ec;
    if (!fs::exists(image_src_path, ec)) {
        fail(ErrorCode::Io, "Preview image file does not exist: " + path_to_utf8(image_src_path), image_src_path);
    }

    ModInfo info = ModLoader::load_mod_info(mod_dir);

    // 统一改名为 preview.webp
    const fs::path dst_path = mod_dir / "preview.webp";

    bool converted = compress_image_to_webp(image_src_path, dst_path);
    if (!converted) {
        if (!fs::equivalent(image_src_path, dst_path, ec)) {
            fs::copy_file(image_src_path, dst_path, fs::copy_options::overwrite_existing, ec);
        }
    }

    if (ec) {
        fail(ErrorCode::Io, "Failed to copy preview image to staging: " + ec.message(), dst_path);
    }

    // 清理该 mod 目录下除 preview.webp 外的所有旧背景图，确保 mod 根目录绝对整洁
    for (const auto& entry : fs::directory_iterator(mod_dir, ec)) {
        if (!entry.is_regular_file(ec)) continue;
        const auto p = entry.path();
        if (p.filename() == "preview.webp") continue;
        const std::string fn_ext = to_lower_ascii(path_to_utf8(p.extension()));
        if (fn_ext == ".png" || fn_ext == ".jpg" || fn_ext == ".jpeg" || fn_ext == ".bmp" || fn_ext == ".webp") {
            const std::string stem = to_lower_ascii(path_to_utf8(p.stem()));
            if (stem.find("preview") != std::string::npos || stem.find("cover") != std::string::npos ||
                stem.find("banner") != std::string::npos || (info.preview_image && *info.preview_image == path_to_utf8(p.filename()))) {
                fs::remove(p, ec);
            }
        }
    }

    info.preview_image = "preview.webp";
    save_mod_info(mod_dir, info);
    info.root_path = mod_dir;
    return info;
}

ModInfo ModManager::update_mod_info(const fs::path& staging_dir, const std::string& mod_id, const ModInfo& updated) {
    const fs::path mod_dir = find_mod_dir(staging_dir, mod_id);
    ModInfo current = ModLoader::load_mod_info(mod_dir);

    // id and root_path are identity, not user data: they are never taken from the caller.
    current.name = updated.name;
    current.version = updated.version;
    current.author = updated.author;
    current.category = updated.category;
    current.description = updated.description;
    current.homepage = updated.homepage;
    current.source_url = updated.source_url;
    current.license = updated.license;
    current.enabled = updated.enabled;
    current.priority = updated.priority;
    current.tags = updated.tags;
    current.root_path = mod_dir;

    save_mod_info(mod_dir, current);
    return current;
}

void ModManager::delete_mod(const fs::path& staging_dir, const std::string& mod_id) {
    const fs::path mod_dir = find_mod_dir(staging_dir, mod_id);

    std::error_code ec;
    const fs::path canonical_staging = fs::canonical(staging_dir, ec);
    if (ec) {
        fail(ErrorCode::Io, "Cannot resolve staging directory: " + ec.message(), staging_dir);
    }
    const fs::path canonical_mod = fs::canonical(mod_dir, ec);
    if (ec) {
        fail(ErrorCode::Io, "Cannot resolve mod directory: " + ec.message(), mod_dir);
    }

    // Guard rail: a metadata file can point anywhere, so deleting must never escape staging.
    const std::string staging_str = canonical_staging.string();
    const std::string mod_str = canonical_mod.string();
    const bool inside = mod_str.size() > staging_str.size() &&
                        mod_str.compare(0, staging_str.size(), staging_str) == 0 &&
                        (mod_str[staging_str.size()] == '/' || mod_str[staging_str.size()] == '\\');
    if (!inside) {
        fail(ErrorCode::Io,
             "Refusing to delete a path outside the staging directory: " + mod_dir.string(), mod_dir);
    }

    fs::remove_all(canonical_mod, ec);
    if (ec) {
        fail(ErrorCode::Io, "Failed to delete mod directory: " + ec.message(), mod_dir);
    }
}

StagedMod ModManager::get_mod_details(const fs::path& staging_dir, const std::string& mod_id) {
    return ModLoader::scan_mod(find_mod_dir(staging_dir, mod_id));
}

void ModManager::save_mod_info(const fs::path& mod_dir, const ModInfo& info) {
    ModInfo portable = info;
    portable.root_path.reset();

    const std::string payload = json(portable).dump(2);

    // 统一使用 .smm_mod.json 作为权威元数据载体
    const fs::path meta_path = mod_dir / ".smm_mod.json";
    const fs::path tmp_path = mod_dir / "._smm_mod.json.tmp";

    std::error_code ec;
    {
        std::ofstream stream(tmp_path, std::ios::binary | std::ios::trunc);
        if (stream) {
            stream << payload;
            stream.flush();
        }
        if (!stream) {
            ec = std::make_error_code(std::errc::io_error);
        }
    }

    if (!ec) {
        fs::remove(meta_path, ec);
        ec.clear();
        fs::rename(tmp_path, meta_path, ec);
    }

    if (ec) {
        std::error_code cleanup_ec;
        fs::remove(tmp_path, cleanup_ec);
        std::ofstream stream(meta_path, std::ios::binary | std::ios::trunc);
        stream << payload;
        stream.flush();
        if (!stream) {
            fail(ErrorCode::Io, "Failed to write mod metadata: " + path_to_utf8(meta_path), meta_path);
        }
    }

    // 清理历史遗留的 mod.json，避免双元数据文件冲突
    const fs::path legacy = mod_dir / "mod.json";
    if (fs::exists(legacy, ec)) {
        fs::remove(legacy, ec);
    }
}

bool ModManager::normalize_preview_image(const fs::path& mod_dir, std::string& preview_name) {
    if (preview_name.empty()) return false;
    std::error_code ec;
    const fs::path img_path = mod_dir / preview_name;
    if (!fs::exists(img_path, ec)) return false;

    const fs::path target_webp = mod_dir / "preview.webp";

    // 若已经是 preview.webp 且大小在 350KB 以下，无需重复压缩
    if (preview_name == "preview.webp" && fs::file_size(img_path, ec) < 350 * 1024) {
        return true;
    }

    if (compress_image_to_webp(img_path, target_webp)) {
        if (!fs::equivalent(img_path, target_webp, ec)) {
            fs::remove(img_path, ec);
        }
        preview_name = "preview.webp";
        return true;
    }

    return true;
}

std::pair<size_t, uint64_t> ModManager::optimize_all_previews(const fs::path& staging_dir) {
    size_t count = 0;
    uint64_t saved_bytes = 0;
    std::error_code ec;

    if (!fs::exists(staging_dir, ec) || !fs::is_directory(staging_dir, ec)) {
        return {0, 0};
    }

    for (const auto& entry : fs::directory_iterator(staging_dir, ec)) {
        if (!entry.is_directory(ec)) continue;
        const fs::path mod_dir = entry.path();
        const std::string mod_name = path_to_utf8(mod_dir.filename());
        if (mod_name.empty() || mod_name[0] == '.') continue;

        fs::path meta_path = mod_dir / ".smm_mod.json";
        if (!fs::exists(meta_path, ec)) {
            meta_path = mod_dir / "mod.json";
        }
        if (!fs::exists(meta_path, ec)) continue;

        ModInfo info = ModLoader::load_mod_info(mod_dir);
        std::string current_preview;
        if (info.preview_image && !info.preview_image->empty()) {
            current_preview = *info.preview_image;
        } else {
            for (const char* cand : {"preview.png", "preview.jpg", "preview.jpeg", "preview.webp",
                                     "cover.png", "cover.jpg", "banner.png", "banner.jpg"}) {
                if (fs::exists(mod_dir / cand, ec)) {
                    current_preview = cand;
                    break;
                }
            }
        }

        if (current_preview.empty()) {
            for (const auto& f : fs::directory_iterator(mod_dir, ec)) {
                if (!f.is_regular_file(ec)) continue;
                const std::string fext = to_lower_ascii(path_to_utf8(f.path().extension()));
                if (fext == ".png" || fext == ".jpg" || fext == ".jpeg" || fext == ".bmp" || fext == ".webp") {
                    current_preview = path_to_utf8(f.path().filename());
                    break;
                }
            }
        }

        if (!current_preview.empty()) {
            const fs::path orig_file = mod_dir / current_preview;
            const uint64_t orig_sz = fs::exists(orig_file, ec) ? fs::file_size(orig_file, ec) : 0;

            std::string norm_preview = current_preview;
            if (normalize_preview_image(mod_dir, norm_preview)) {
                const fs::path new_file = mod_dir / "preview.webp";
                const uint64_t new_sz = fs::exists(new_file, ec) ? fs::file_size(new_file, ec) : 0;

                info.preview_image = "preview.webp";
                save_mod_info(mod_dir, info);

                if (orig_sz > new_sz) {
                    saved_bytes += (orig_sz - new_sz);
                }
                count++;
            }
        }
    }

    return {count, saved_bytes};
}

} // namespace smm
