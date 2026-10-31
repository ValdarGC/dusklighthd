#ifndef DUSK_TPHD_HD_TEXTURE_EXPORT_HPP
#define DUSK_TPHD_HD_TEXTURE_EXPORT_HPP

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

#include <dolphin/types.h>

namespace dusk::tphd {

struct GtxSurface;

// TPHD texture packs.
//
// Every Wii U texture is named after the XXH64 hash of its GTX base level:
//   tphd_{width}x{height}_{hash:016x}.dds|.png   (+ "_mipN" sidecars for PNG)
// Files with that name anywhere under <data folder>/texture_replacements/ replace the HD texture
// with the same hash. They are loaded synchronously the first time the texture is drawn.

std::string hd_texture_name(const GtxSurface& s);

// Rescans <data folder>/texture_replacements/ for "tphd_" files. Textures already registered keep
// their current replacement until their archive is loaded again.
void reload_user_hd_textures();
std::optional<std::filesystem::path> find_user_hd_texture(const GtxSurface& s);

struct HdTextureExportOptions {
    bool stages = true;
    bool objects = true;
    bool ui = true;
    bool particles = true;
    bool other = true;
    bool png = false;  // DDS (with mipmaps) otherwise
};

struct HdTextureExportProgress {
    bool running = false;
    bool finished = false;
    u32 packsDone = 0;
    u32 packsTotal = 0;
    u32 texturesWritten = 0;
    u64 bytesWritten = 0;
    std::string current;
    std::string error;
};

std::filesystem::path hd_texture_export_root();
void start_hd_texture_export(const HdTextureExportOptions& options);
void cancel_hd_texture_export();
HdTextureExportProgress hd_texture_export_progress();

}

#endif
