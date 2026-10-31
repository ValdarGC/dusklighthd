#ifndef DUSK_TPHD_HD_MAP_PALETTE_HPP
#define DUSK_TPHD_HD_MAP_PALETTE_HPP

#include <dolphin/types.h>

struct ResTIMG;

// TPHD map palettes with 8-bit alpha. The GC map code keeps its RGB5A3 palettes (and keeps writing
// them), but with TPHD active dRenderingMap_c::makeResTIMG points the map texture at a 32-bit
// RGBA8 copy (aurora GX_TL_RGBA8_PC). Entries given an exact Wii U RGBA colour keep it while the
// RGB5A3 entry still holds the value written with it; every other entry is decoded from RGB5A3.
namespace dusk::tphd {

// Exact RGBA8 (0xRRGGBBAA) for one palette entry of the RGB5A3 palette i_src (entry = u16 index).
// Call it right after writing the RGB5A3 value of that entry.
void mapPaletteSetExact(const void* i_src, int i_entry, u32 i_rgba);
// Same for the 4 entries of a dMpath_RGB5A3_palDt_s (i_palDt = palette index).
void mapPaletteSetExactPalDt(const void* i_src, int i_palDt, u32 i_rgba);
// Same, with the RGB5A3 value given explicitly (the palette i_src gets it later, e.g. by a copy).
void mapPaletteSetExactPalDtValue(const void* i_src, int i_palDt, u16 i_written, u32 i_rgba);
// Rebuilds the RGBA8 copy of the palette used by i_timg (no-op when it has none).
void mapPaletteSync(const ResTIMG* i_timg);
// Set around a makeResTIMG call that remakes an existing ResTIMG of a live map (the minimap's
// per-frame refreshTextureSize): its RGBA8 palette copy is reused instead of allocated again.
extern bool g_mapPaletteReuse;

}  // namespace dusk::tphd

#endif
