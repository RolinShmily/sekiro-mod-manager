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

ModInfo ModManager::set_mod_preview(const fs::path& staging_dir, const std::string& mod_id,
                                    const fs::path& image_src_path) {
    const fs::path mod_dir = find_mod_dir(staging_dir, mod_id);
    std::error_code ec;
    if (!fs::exists(image_src_path, ec)) {
        fail(ErrorCode::Io, "Preview image file does not exist: " + path_to_utf8(image_src_path), image_src_path);
    }

    std::string ext = path_to_utf8(image_src_path.extension());
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    std::string filename = "preview.png";
#ifdef _WIN32
    bool converted = false;
    if (ext == ".webp" || is_webp_header(image_src_path)) {
        converted = convert_to_standard_png_wic(image_src_path, mod_dir / filename);
    }
    if (!converted) {
        if (ext == ".jpg" || ext == ".jpeg") {
            filename = "preview" + ext;
        }
        fs::copy_file(image_src_path, mod_dir / filename, fs::copy_options::overwrite_existing, ec);
    }
#else
    if (!ext.empty()) {
        filename = "preview" + ext;
    }
    fs::copy_file(image_src_path, mod_dir / filename, fs::copy_options::overwrite_existing, ec);
#endif
    if (ec) {
        fail(ErrorCode::Io, "Failed to copy preview image to staging: " + ec.message(), mod_dir / filename);
    }

    ModInfo info = ModLoader::load_mod_info(mod_dir);
    info.preview_image = filename;
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

    // Written twice on purpose: .smm_mod.json is SMM's own record, mod.json is what the wider
    // modding ecosystem reads. Both must always agree.
    for (const char* filename : {".smm_mod.json", "mod.json"}) {
        const fs::path meta_path = mod_dir / filename;
        const fs::path tmp_path = mod_dir / ("." + std::string(filename) + ".tmp");

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
            // Some filesystems or a locked target defeat the atomic path; a direct write
            // still gets the metadata onto disk.
            std::error_code cleanup_ec;
            fs::remove(tmp_path, cleanup_ec);
            std::ofstream stream(meta_path, std::ios::binary | std::ios::trunc);
            stream << payload;
            stream.flush();
            if (!stream) {
                fail(ErrorCode::Io, "Failed to write mod metadata: " + path_to_utf8(meta_path), meta_path);
            }
        }
    }
}

bool ModManager::normalize_preview_image(const fs::path& mod_dir, std::string& preview_name) {
    if (preview_name.empty()) return false;
    std::error_code ec;
    const fs::path img_path = mod_dir / preview_name;
    if (!fs::exists(img_path, ec)) return false;

#ifdef _WIN32
    if (preview_name.size() >= 5 && preview_name.substr(preview_name.size() - 5) == ".webp" || is_webp_header(img_path)) {
        const fs::path png_path = mod_dir / "preview.png";
        if (convert_to_standard_png_wic(img_path, png_path)) {
            preview_name = "preview.png";
            return true;
        }
    }
#endif
    return true;
}

} // namespace smm
