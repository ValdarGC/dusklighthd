#ifndef DUSK_TPHD_HD_ASSET_LAYER_HPP
#define DUSK_TPHD_HD_ASSET_LAYER_HPP

#include <filesystem>
#include <optional>
#include <string_view>
#include <vector>

#include <dolphin/types.h>

namespace dusk::tphd {

struct GtxSurface;

// Configure the base directory for HD asset overrides. `contentPath` should
// point at a Wii-U `content/` directory (the parent of `res/`). Empty path
// disables HD overrides.
void set_hd_content_path(std::filesystem::path contentPath);

// Returns a pointer to cached HD archive bytes for packed sub-archives that
// are mounted from an already-loaded RARC rather than through the DVD layer.
// Caller must not outlive the next setHdContentPath() call.
std::optional<std::vector<u8>*> try_load_hd_archive(std::string_view gcPath);

// Called after JKRMemArchive has loaded an overlaid DVD archive into its final
// heap address so HD texture replacements can be registered against the
// pointers the game will actually use.
void register_mounted_hd_archive(s32 entryNum, void* arcBytes, size_t arcSize);

// Called after JKRArchive copies a resource (BTI item icon, BMD/BDL item
// model, etc.) into caller-owned memory. pp
void register_copied_hd_resource(s32 entryNum, std::string_view resourceName, void* buffer,
                                 size_t resourceSize);

// Register HD particle textures which live in a loose `tex/Particle/<jpcStem>.jpc.gtx.gz` sidecar
void register_hd_particle_textures(std::string_view jpcStem, void* jpcBuffer, size_t jpcSize);

// Returns bytes remaining in a registered HD archive range that contains ptr.
// Used for debug heap accounting because some HD buffers are not JKR-owned.
std::optional<size_t> find_registered_hd_archive_remaining(const void* ptr);

// Reads one file (Yaz0-decoded) from a Wii U archive (e.g. "res/FieldMap/Field0.arc",
// "f_sp121/room0.dzs"), ignoring the HD skip list. No texture registration is done.
std::optional<std::vector<u8>> read_hd_arc_file(std::string_view resPath, std::string_view filePath);

// A Wii U texture as aurora gets it: R8_PC / RG8_PC / RGBA8_PC / BC1_PC (gxFormat), all mips
// back to back, linear.
struct HdDecodedTexture {
    std::vector<u8> bytes;
    u32 width = 0;
    u32 height = 0;
    u32 mipCount = 1;
    u8 gxFormat = 0;
};
std::optional<HdDecodedTexture> decode_hd_surface(const GtxSurface& s);

const std::filesystem::path& hd_content_path();

}

#endif
