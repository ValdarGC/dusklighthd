#ifndef DUSK_TPHD_REVO_CANVAS_HPP
#define DUSK_TPHD_REVO_CANVAS_HPP

#include <algorithm>

#include <dolphin/gx.h>

#include "m_Do/m_Do_graphic.h"

namespace dusk::tphd {

// The Wii U TV is 16:9: its 608 x 448 Revo layouts are stretched to 808 x 448 TV units. On other
// window shapes the Wii U UI (HUD, menus, file select, messages...) keeps that aspect inside the
// largest centred 16:9 box of the 2D canvas, while menu backgrounds still cover the whole canvas.
// The canvas is always 448 tall and at least 16:9 wide in TPHD mode (taller than 448 on narrower
// windows, see mDoGph_gInf_c::setTvSize), so the box is only ever narrower than the canvas.
constexpr f32 kRevoTvWidth = 448.0f * 16.0f / 9.0f;  // 796.44

inline f32 revoWidth() {
    return std::min(mDoGph_gInf_c::getMaxXF() - mDoGph_gInf_c::getMinXF(), kRevoTvWidth);
}

inline f32 revoMinX() {
    return (mDoGph_gInf_c::getMinXF() + mDoGph_gInf_c::getMaxXF() - revoWidth()) * 0.5f;
}

inline f32 revoMaxX() {
    return revoMinX() + revoWidth();
}

// Clips the Wii U screens drawn in its scope to the 16:9 box. The TV never shows what the Revo
// layouts keep outside it (slide-in panes, oversized ornaments), which would otherwise appear in
// the side / top / bottom margins. GX scissor units: the logical 608 x 448 framebuffer spanning
// the whole canvas.
class RevoClip {
public:
    RevoClip() {
        GXGetScissor(&mX, &mY, &mW, &mH);
        const f32 kx = FB_WIDTH / mDoGph_gInf_c::getWidthF();
        const f32 ky = FB_HEIGHT / mDoGph_gInf_c::getHeightF();
        const f32 left = (revoMinX() - mDoGph_gInf_c::getMinXF()) * kx;
        const f32 top = (0.0f - mDoGph_gInf_c::getMinYF()) * ky;
        GXSetScissor(static_cast<u32>(left), static_cast<u32>(top),
                     static_cast<u32>(revoWidth() * kx + 0.5f), static_cast<u32>(448.0f * ky + 0.5f));
    }
    ~RevoClip() { GXSetScissor(mX, mY, mW, mH); }
    RevoClip(const RevoClip&) = delete;
    RevoClip& operator=(const RevoClip&) = delete;

private:
    u32 mX, mY, mW, mH;
};

}  // namespace dusk::tphd

#endif
