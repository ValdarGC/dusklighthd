#include "HdTextureExport.hpp"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstring>
#include <fstream>
#include <mutex>
#include <string_view>
#include <system_error>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <borealis/io.hpp>
#include <borealis/log.hpp>
#include <fmt/format.h>
#include <png.h>
#include <xxhash.h>

#include "GtxParser.hpp"
#include "HdAssetLayer.hpp"
#include "TphdPack.hpp"
#include "dusk/main.h"
#include "dusk/settings.h"

constexpr borealis::Log HdExportLog{"dusk::tphd::export"};

namespace dusk::tphd {
namespace {

constexpr u8 kGxR8 = 0x41;
constexpr u8 kGxRG8 = 0x43;
constexpr u8 kGxRGBA8 = 0x46;
constexpr u8 kGxBC1 = 0x4E;

std::string lower(std::string_view s) {
    std::string out(s);
    for (auto& c : out) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return out;
}

bool ends_with(std::string_view s, std::string_view suffix) {
    return s.size() >= suffix.size() && s.substr(s.size() - suffix.size()) == suffix;
}

u64 surface_hash(const GtxSurface& s) {
    return XXH64(s.baseData.data(), s.baseData.size(), 0);
}

// ---------------------------------------------------------------------------------------------
// User packs
// ---------------------------------------------------------------------------------------------

std::mutex s_userMutex;
std::unordered_map<u64, std::filesystem::path> s_userTextures;

// "tphd_{w}x{h}_{hash}" (any case, optional "_mipN" rejected) -> hash
std::optional<u64> parse_user_stem(std::string_view stem) {
    if (!stem.starts_with("tphd_")) {
        return std::nullopt;
    }
    const size_t dims = 5;
    const size_t sep = stem.find('_', dims);
    if (sep == std::string_view::npos || stem.size() < sep + 1 + 16) {
        return std::nullopt;
    }
    const std::string_view hex = stem.substr(sep + 1, 16);
    if (stem.size() != sep + 1 + 16) {
        return std::nullopt;  // "_mipN" sidecars and anything else after the hash
    }
    u64 value = 0;
    for (const char c : hex) {
        value <<= 4;
        if (c >= '0' && c <= '9') {
            value |= u64(c - '0');
        } else if (c >= 'a' && c <= 'f') {
            value |= u64(c - 'a' + 10);
        } else {
            return std::nullopt;
        }
    }
    return value;
}

// ---------------------------------------------------------------------------------------------
// Encoders
// ---------------------------------------------------------------------------------------------

struct DdsPixelFormat {
    u32 size, flags, fourCC, rgbBitCount, rMask, gMask, bMask, aMask;
};
struct DdsHeader {
    u32 size, flags, height, width, pitchOrLinearSize, depth, mipMapCount;
    u32 reserved1[11];
    DdsPixelFormat ddspf;
    u32 caps, caps2, caps3, caps4, reserved2;
};
static_assert(sizeof(DdsHeader) == 124);

u32 level_dim(u32 base, u32 level) { return std::max(1u, base >> level); }

size_t level_size(u8 gxFormat, u32 w, u32 h) {
    switch (gxFormat) {
    case kGxR8: return size_t(w) * h;
    case kGxRG8: return size_t(w) * h * 2;
    case kGxRGBA8: return size_t(w) * h * 4;
    case kGxBC1: return size_t((w + 3) / 4) * ((h + 3) / 4) * 8;
    default: return 0;
    }
}

struct Level {
    u32 width;
    u32 height;
    std::vector<u8> bytes;
};

void rgb565(u16 c, u8* out) {
    out[0] = u8(((c >> 11) & 0x1f) * 255 / 31);
    out[1] = u8(((c >> 5) & 0x3f) * 255 / 63);
    out[2] = u8((c & 0x1f) * 255 / 31);
}

std::vector<u8> decode_bc1(std::span<const u8> in, u32 w, u32 h) {
    std::vector<u8> out(size_t(w) * h * 4);
    const u32 bw = (w + 3) / 4;
    const u32 bh = (h + 3) / 4;
    for (u32 by = 0; by < bh; ++by) {
        for (u32 bx = 0; bx < bw; ++bx) {
            const u8* b = in.data() + (size_t(by) * bw + bx) * 8;
            const u16 c0 = u16(b[0] | (b[1] << 8));
            const u16 c1 = u16(b[2] | (b[3] << 8));
            u8 pal[4][4];
            rgb565(c0, pal[0]);
            rgb565(c1, pal[1]);
            pal[0][3] = pal[1][3] = 255;
            for (int k = 0; k < 3; ++k) {
                if (c0 > c1) {
                    pal[2][k] = u8((2 * pal[0][k] + pal[1][k]) / 3);
                    pal[3][k] = u8((pal[0][k] + 2 * pal[1][k]) / 3);
                } else {
                    pal[2][k] = u8((pal[0][k] + pal[1][k]) / 2);
                    pal[3][k] = 0;
                }
            }
            pal[2][3] = 255;
            pal[3][3] = c0 > c1 ? 255 : 0;
            const u32 bits = u32(b[4]) | (u32(b[5]) << 8) | (u32(b[6]) << 16) | (u32(b[7]) << 24);
            for (u32 py = 0; py < 4; ++py) {
                for (u32 px = 0; px < 4; ++px) {
                    const u32 x = bx * 4 + px;
                    const u32 y = by * 4 + py;
                    if (x >= w || y >= h) {
                        continue;
                    }
                    const u32 idx = (bits >> (2 * (py * 4 + px))) & 3;
                    std::memcpy(&out[(size_t(y) * w + x) * 4], pal[idx], 4);
                }
            }
        }
    }
    return out;
}

// Splits the decoded mip chain; RGBA8 output for everything but BC1 when keepBc1.
std::vector<Level> split_levels(const HdDecodedTexture& t, bool keepBc1) {
    std::vector<Level> levels;
    size_t pos = 0;
    for (u32 l = 0; l < t.mipCount; ++l) {
        const u32 w = level_dim(t.width, l);
        const u32 h = level_dim(t.height, l);
        const size_t size = level_size(t.gxFormat, w, h);
        if (size == 0 || pos + size > t.bytes.size()) {
            break;
        }
        std::span<const u8> src(t.bytes.data() + pos, size);
        pos += size;

        Level lvl{w, h, {}};
        switch (t.gxFormat) {
        case kGxR8:
            lvl.bytes.resize(size * 4);
            for (size_t i = 0; i < size; ++i) {
                std::memset(&lvl.bytes[i * 4], src[i], 4);
            }
            break;
        case kGxRG8:
            // GC IA8: intensity in R, alpha in G (sampled as .rrrg).
            lvl.bytes.resize(size * 2);
            for (size_t i = 0; i < size / 2; ++i) {
                u8* o = &lvl.bytes[i * 4];
                o[0] = o[1] = o[2] = src[i * 2];
                o[3] = src[i * 2 + 1];
            }
            break;
        case kGxRGBA8:
            lvl.bytes.assign(src.begin(), src.end());
            break;
        case kGxBC1:
            lvl.bytes = keepBc1 ? std::vector<u8>(src.begin(), src.end()) : decode_bc1(src, w, h);
            break;
        }
        levels.push_back(std::move(lvl));
    }
    return levels;
}

bool write_bytes(const std::filesystem::path& path, std::span<const u8> bytes) {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) {
        return false;
    }
    f.write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size()));
    return bool(f);
}

std::vector<u8> encode_dds(const HdDecodedTexture& t) {
    const bool bc1 = t.gxFormat == kGxBC1;
    std::vector<Level> levels = split_levels(t, true);
    if (levels.empty()) {
        return {};
    }
    u32 width = t.width;
    u32 height = t.height;
    if (bc1 && (width % 4 != 0 || height % 4 != 0)) {
        // Block-aligned base level only (aurora uploads BC1 with block-aligned sizes).
        width = (width + 3) & ~3u;
        height = (height + 3) & ~3u;
        levels.resize(1);
    }
    const u32 mips = u32(levels.size());

    DdsHeader h{};
    h.size = sizeof(DdsHeader);
    h.flags = 0x1 | 0x2 | 0x4 | 0x1000 | (bc1 ? 0x80000 : 0x8) | (mips > 1 ? 0x20000 : 0);
    h.width = width;
    h.height = height;
    h.pitchOrLinearSize = bc1 ? u32(levels[0].bytes.size()) : width * 4;
    h.mipMapCount = mips;
    h.ddspf.size = sizeof(DdsPixelFormat);
    if (bc1) {
        h.ddspf.flags = 0x4;
        h.ddspf.fourCC = 0x31545844;  // "DXT1"
    } else {
        h.ddspf.flags = 0x40 | 0x1;
        h.ddspf.rgbBitCount = 32;
        h.ddspf.rMask = 0x000000ff;
        h.ddspf.gMask = 0x0000ff00;
        h.ddspf.bMask = 0x00ff0000;
        h.ddspf.aMask = 0xff000000;
    }
    h.caps = 0x1000 | (mips > 1 ? 0x8 | 0x400000 : 0);

    std::vector<u8> out(4 + sizeof(h));
    std::memcpy(out.data(), "DDS ", 4);
    std::memcpy(out.data() + 4, &h, sizeof(h));
    for (const auto& l : levels) {
        out.insert(out.end(), l.bytes.begin(), l.bytes.end());
    }
    return out;
}

std::vector<u8> encode_png(const Level& l) {
    png_image image{};
    image.version = PNG_IMAGE_VERSION;
    image.width = l.width;
    image.height = l.height;
    image.format = PNG_FORMAT_RGBA;
    png_alloc_size_t size = 0;
    if (!png_image_write_to_memory(&image, nullptr, &size, 0, l.bytes.data(), 0, nullptr)) {
        return {};
    }
    std::vector<u8> out(size);
    if (!png_image_write_to_memory(&image, out.data(), &size, 0, l.bytes.data(), 0, nullptr)) {
        return {};
    }
    out.resize(size);
    return out;
}

// ---------------------------------------------------------------------------------------------
// Export
// ---------------------------------------------------------------------------------------------

struct ExportJob {
    std::filesystem::path source;
    std::filesystem::path folder;  // relative to the export root
    std::string label;
    bool particle = false;
};

std::mutex s_exportMutex;
HdTextureExportProgress s_progress;
std::atomic<bool> s_cancel{false};
std::thread s_thread;

struct ThreadJoiner {
    ~ThreadJoiner() {
        s_cancel = true;
        if (s_thread.joinable()) {
            s_thread.join();
        }
    }
} s_joiner;

std::string strip_suffix(std::string name, std::string_view suffix) {
    if (ends_with(lower(name), suffix)) {
        name.resize(name.size() - suffix.size());
    }
    return name;
}

// res/Stage/<stage>/<arc>  -> stages/<stage>/<arc>
// res/Object/<arc>         -> objects/<arc>
// res/Layout*, FieldMap, Menu, CardIcon, Font*, Msg* -> ui/<dir>/<arc>
// tex/Particle/<jpc>       -> particles/<jpc>
// anything else            -> other/<dirs>/<arc>
std::optional<std::filesystem::path> job_folder(const std::filesystem::path& rel, std::string_view stem,
                                                const HdTextureExportOptions& o) {
    std::vector<std::string> parts;
    for (const auto& p : rel.parent_path()) {
        parts.push_back(borealis::io::fs_path_to_string(p));
    }
    const std::string stemStr(stem);
    if (parts.size() >= 2 && lower(parts[0]) == "tex" && lower(parts[1]) == "particle") {
        return o.particles ? std::optional(std::filesystem::path("particles") / stemStr) : std::nullopt;
    }
    if (parts.size() >= 3 && lower(parts[0]) == "res" && lower(parts[1]) == "stage") {
        return o.stages ? std::optional(std::filesystem::path("stages") / parts[2] / stemStr) : std::nullopt;
    }
    if (parts.size() >= 2 && lower(parts[0]) == "res") {
        const std::string dir = lower(parts[1]);
        if (dir == "object") {
            return o.objects ? std::optional(std::filesystem::path("objects") / stemStr) : std::nullopt;
        }
        if (dir.starts_with("layout") || dir == "fieldmap" || dir == "menu" || dir == "cardicon" ||
            dir.starts_with("font") || dir.starts_with("msg"))
        {
            return o.ui ? std::optional(std::filesystem::path("ui") / parts[1] / stemStr) : std::nullopt;
        }
    }
    if (!o.other) {
        return std::nullopt;
    }
    std::filesystem::path folder = "other";
    for (const auto& p : parts) {
        folder /= p;
    }
    return folder / stemStr;
}

std::vector<ExportJob> collect_jobs(const std::filesystem::path& content, const HdTextureExportOptions& o) {
    std::vector<ExportJob> jobs;
    std::error_code ec;
    for (std::filesystem::recursive_directory_iterator it(
             content, std::filesystem::directory_options::skip_permission_denied, ec);
         it != std::filesystem::recursive_directory_iterator(); it.increment(ec))
    {
        if (ec) {
            break;
        }
        if (!it->is_regular_file(ec)) {
            continue;
        }
        const std::string name = borealis::io::fs_path_to_string(it->path().filename());
        const std::string lname = lower(name);
        bool particle = false;
        std::string stem;
        if (ends_with(lname, ".pack.gz")) {
            if (lname == "shaders.pack.gz") {
                continue;
            }
            stem = strip_suffix(name, ".pack.gz");
        } else if (ends_with(lname, ".jpc.gtx.gz")) {
            stem = strip_suffix(name, ".jpc.gtx.gz");
            particle = true;
        } else {
            continue;
        }
        const auto rel = std::filesystem::relative(it->path(), content, ec);
        if (ec) {
            continue;
        }
        if (auto folder = job_folder(rel, stem, o)) {
            jobs.push_back({it->path(), std::move(*folder), borealis::io::fs_path_to_string(rel), particle});
        }
    }
    std::sort(jobs.begin(), jobs.end(), [](const auto& a, const auto& b) { return a.label < b.label; });
    return jobs;
}

std::vector<u8> read_all(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary);
    return std::vector<u8>(std::istreambuf_iterator<char>(f), {});
}

const char* format_name(u8 gxFormat) {
    switch (gxFormat) {
    case kGxR8: return "I8";
    case kGxRG8: return "IA8";
    case kGxRGBA8: return "RGBA8";
    case kGxBC1: return "CMPR";
    default: return "?";
    }
}

void export_thread(std::filesystem::path content, std::filesystem::path root, HdTextureExportOptions o) {
    const auto setError = [](std::string error) {
        std::lock_guard lk{s_exportMutex};
        s_progress.error = std::move(error);
    };

    auto jobs = collect_jobs(content, o);
    {
        std::lock_guard lk{s_exportMutex};
        s_progress.packsTotal = u32(jobs.size());
    }

    // Shared textures are written once, where they are first found (index.txt points to it).
    std::unordered_map<u64, std::string> written;
    const char* ext = o.png ? ".png" : ".dds";

    for (const auto& job : jobs) {
        if (s_cancel) {
            break;
        }
        {
            std::lock_guard lk{s_exportMutex};
            s_progress.current = job.label;
        }

        std::error_code ec;
        const auto space = std::filesystem::space(root, ec);
        if (!ec && space.available < (u64(256) << 20)) {
            setError("Less than 256 MB of free disk space left, export stopped.");
            break;
        }

        // Entries: (name inside the pack, GTX bytes)
        std::vector<std::pair<std::string, std::span<const u8>>> gtxs;
        std::optional<TphdPack> pack;
        std::optional<std::vector<u8>> particleGtx;
        if (job.particle) {
            const auto gz = read_all(job.source);
            particleGtx = decompressGzip(gz);
            if (particleGtx) {
                gtxs.emplace_back(job.label, std::span<const u8>(*particleGtx));
            }
        } else {
            pack = TphdPack::loadFromFile(job.source);
            if (pack) {
                for (const auto& e : pack->entries()) {
                    if (e.data.size() >= 4 && std::memcmp(e.data.data(), "Gfx2", 4) == 0) {
                        gtxs.emplace_back(std::string(e.name), e.data);
                    }
                }
            }
        }

        const auto folder = root / job.folder;
        const std::string folderStr = borealis::io::fs_path_to_string(job.folder);
        std::string index;
        bool madeFolder = false;
        for (const auto& [entryName, bytes] : gtxs) {
            const auto surfaces = parseGtx(bytes);
            for (size_t i = 0; i < surfaces.size(); ++i) {
                const auto& s = surfaces[i];
                if (s.baseData.empty()) {
                    continue;
                }
                const std::string name = hd_texture_name(s);
                const u64 hash = surface_hash(s);
                if (const auto it = written.find(hash); it != written.end()) {
                    index += fmt::format("{}\t{}[{}]\tsame as {}\n", name, entryName, i, it->second);
                    continue;
                }
                auto decoded = decode_hd_surface(s);
                if (!decoded) {
                    continue;
                }
                if (!madeFolder) {
                    std::filesystem::create_directories(folder, ec);
                    madeFolder = true;
                }

                u64 fileBytes = 0;
                bool ok = true;
                if (o.png) {
                    const auto levels = split_levels(*decoded, false);
                    for (size_t l = 0; l < levels.size() && ok; ++l) {
                        const auto png = encode_png(levels[l]);
                        const auto file =
                            folder / (l == 0 ? name + ext : fmt::format("{}_mip{}{}", name, l, ext));
                        ok = !png.empty() && write_bytes(file, png);
                        fileBytes += png.size();
                    }
                } else {
                    const auto dds = encode_dds(*decoded);
                    ok = !dds.empty() && write_bytes(folder / (name + ext), dds);
                    fileBytes = dds.size();
                }
                if (!ok) {
                    HdExportLog.warn("Could not write {} in {}", name, folderStr);
                    continue;
                }

                written.emplace(hash, folderStr + "/" + name + ext);
                index += fmt::format("{}\t{}[{}]\t{}x{} {} {} mip(s)\n", name, entryName, i, decoded->width,
                                     decoded->height, format_name(decoded->gxFormat), decoded->mipCount);
                std::lock_guard lk{s_exportMutex};
                ++s_progress.texturesWritten;
                s_progress.bytesWritten += fileBytes;
            }
        }
        if (!index.empty()) {
            std::filesystem::create_directories(folder, ec);
            const std::string text =
                fmt::format("# Textures of {}\n# file\tsource[surface]\tinfo\n{}", job.label, index);
            write_bytes(folder / "index.txt",
                        std::span<const u8>(reinterpret_cast<const u8*>(text.data()), text.size()));
        }

        std::lock_guard lk{s_exportMutex};
        ++s_progress.packsDone;
    }

    std::lock_guard lk{s_exportMutex};
    s_progress.running = false;
    s_progress.finished = true;
    s_progress.current.clear();
    HdExportLog.info("TPHD texture export finished: {} textures, {} bytes", s_progress.texturesWritten,
                     s_progress.bytesWritten);
}

}  // namespace

std::string hd_texture_name(const GtxSurface& s) {
    return fmt::format("tphd_{}x{}_{:016x}", s.width, s.height, surface_hash(s));
}

void reload_user_hd_textures() {
    std::unordered_map<u64, std::filesystem::path> found;
    if (getSettings().game.enableTextureReplacements) {
        const auto root = ConfigPath / "texture_replacements";
        std::error_code ec;
        for (std::filesystem::recursive_directory_iterator it(
                 root,
                 std::filesystem::directory_options::skip_permission_denied |
                     std::filesystem::directory_options::follow_directory_symlink,
                 ec);
             it != std::filesystem::recursive_directory_iterator(); it.increment(ec))
        {
            if (ec) {
                break;
            }
            if (!it->is_regular_file(ec)) {
                continue;
            }
            const std::string ext = lower(borealis::io::fs_path_to_string(it->path().extension()));
            if (ext != ".dds" && ext != ".png") {
                continue;
            }
            const auto hash = parse_user_stem(lower(borealis::io::fs_path_to_string(it->path().stem())));
            if (!hash) {
                continue;
            }
            // DDS wins over PNG when a texture has both.
            auto [entry, inserted] = found.try_emplace(*hash, it->path());
            if (!inserted && ext == ".dds") {
                entry->second = it->path();
            }
        }
    }
    HdExportLog.info("TPHD texture pack: {} texture(s)", found.size());
    std::lock_guard lk{s_userMutex};
    s_userTextures = std::move(found);
}

std::optional<std::filesystem::path> find_user_hd_texture(const GtxSurface& s) {
    std::lock_guard lk{s_userMutex};
    if (s_userTextures.empty() || s.baseData.empty()) {
        return std::nullopt;
    }
    if (const auto it = s_userTextures.find(surface_hash(s)); it != s_userTextures.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::filesystem::path hd_texture_export_root() { return ConfigPath / "texture_dumps" / "tphd"; }

void start_hd_texture_export(const HdTextureExportOptions& options) {
    std::lock_guard lk{s_exportMutex};
    if (s_progress.running) {
        return;
    }
    if (s_thread.joinable()) {
        s_thread.join();
    }
    s_progress = {};
    auto content = hd_content_path().empty() ? tphd_content_path() : hd_content_path();
    if (content.empty()) {
        s_progress.finished = true;
        s_progress.error = "No valid TPHD content folder is configured.";
        return;
    }
    const auto root = hd_texture_export_root();
    std::error_code ec;
    std::filesystem::create_directories(root, ec);
    s_cancel = false;
    s_progress.running = true;
    s_thread = std::thread(export_thread, std::move(content), root, options);
}

void cancel_hd_texture_export() { s_cancel = true; }

HdTextureExportProgress hd_texture_export_progress() {
    std::lock_guard lk{s_exportMutex};
    return s_progress;
}

}
