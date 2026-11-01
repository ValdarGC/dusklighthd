#ifndef DUSK_TPHD_REVO_CANVAS_HPP
#define DUSK_TPHD_REVO_CANVAS_HPP

#include <algorithm>

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

}  // namespace dusk::tphd

#endif
