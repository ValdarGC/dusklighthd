#pragma once

#include "JSystem/J2DGraph/J2DScreen.h"
#include "JSystem/J2DGraph/J2DTextBox.h"
#include "JSystem/J2DGraph/J2DPicture.h"
#include "m_Do/m_Do_graphic.h"
#include "d/d_meter2_info.h"
#include "d/d_pane_class.h"
#include "d/d_com_inf_game.h"
#include "helpers/string.hpp"
#include "dusk/version.hpp"
#include "dusk/settings.h"
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
#include <map>
#include <array>
#include <filesystem>
#include "dusk/tphd/RevoCanvas.hpp"
#include <cmath>
#include <unordered_map>

namespace dusk::tphd {

inline bool getWiiUMsgString(u32 stringId, char* outBuf, size_t outSize) {
    if (outBuf == NULL || outSize == 0) return false;
    outBuf[0] = '\0';
    if (!tphd_active()) return false;

    static std::vector<u8> s_cachedBmg;
    static std::string s_cachedLang;

    const char* langDir = "Msgsp";
    if (dusk::version::isRegionJpn()) {
        langDir = "Msgjp";
    } else {
        switch (dComIfGs_getPalLanguage()) {
        case dSv_player_config_c::LANGUAGE_ENGLISH:
            langDir = dusk::version::isRegionPal() ? "Msguk" : "Msgus";
            break;
        case dSv_player_config_c::LANGUAGE_GERMAN:
            langDir = "Msgde";
            break;
        case dSv_player_config_c::LANGUAGE_FRENCH:
            langDir = "Msgfr";
            break;
        case dSv_player_config_c::LANGUAGE_SPANISH:
            langDir = "Msgsp";
            break;
        case dSv_player_config_c::LANGUAGE_ITALIAN:
            langDir = "Msgit";
            break;
        default:
            langDir = "Msgsp";
            break;
        }
    }

    if (s_cachedBmg.empty() || s_cachedLang != langDir) {
        s_cachedBmg.clear();
        s_cachedLang = langDir;

        auto path = tphd_content_path() / "res" / langDir / "bmgres.arc";
        FILE* fp = fopen(path.string().c_str(), "rb");
        if (!fp) {
            path = tphd_content_path() / "res" / "Msgsp" / "bmgres.arc";
            fp = fopen(path.string().c_str(), "rb");
        }
        if (!fp) return false;

        fseek(fp, 0, SEEK_END);
        long sz = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        if (sz > 0) {
            std::vector<u8> comp(sz);
            if (fread(comp.data(), 1, sz, fp) == (size_t)sz) {
                if (comp.size() >= 16 && comp[0] == 'Y' && comp[1] == 'a' && comp[2] == 'z' && comp[3] == '0') {
                    u32 uncompressedSize = ((u32)comp[4] << 24) | ((u32)comp[5] << 16) | ((u32)comp[6] << 8) | (u32)comp[7];
                    s_cachedBmg.resize(uncompressedSize);
                    size_t src = 16;
                    size_t dst = 0;
                    u8 validBits = 0;
                    u8 currByte = 0;
                    while (dst < uncompressedSize && src < comp.size()) {
                        if (validBits == 0) {
                            currByte = comp[src++];
                            validBits = 8;
                        }
                        if (currByte & 0x80) {
                            s_cachedBmg[dst++] = comp[src++];
                        } else {
                            if (src + 1 >= comp.size()) break;
                            u8 b1 = comp[src++];
                            u8 b2 = comp[src++];
                            u32 dist = (((u32)(b1 & 0x0F)) << 8) | b2;
                            u32 numBytes = b1 >> 4;
                            if (numBytes == 0) {
                                if (src >= comp.size()) break;
                                numBytes = (u32)comp[src++] + 0x12;
                            } else {
                                numBytes += 2;
                            }
                            if (dist >= dst) break;
                            size_t copySrc = dst - (dist + 1);
                            for (u32 k = 0; k < numBytes && dst < uncompressedSize; k++) {
                                s_cachedBmg[dst++] = s_cachedBmg[copySrc++];
                            }
                        }
                        currByte <<= 1;
                        validBits--;
                    }
                } else {
                    s_cachedBmg = std::move(comp);
                }
            }
        }
        fclose(fp);
    }

    if (s_cachedBmg.empty()) return false;

    const u8* bmg = nullptr;
    size_t bmgSize = 0;
    for (size_t i = 0; i + 8 <= s_cachedBmg.size(); i++) {
        if (std::memcmp(&s_cachedBmg[i], "MESGbmg1", 8) == 0) {
            bmg = &s_cachedBmg[i];
            bmgSize = s_cachedBmg.size() - i;
            break;
        }
    }
    if (!bmg || bmgSize < 32) return false;

    u32 fileSize = ((u32)bmg[8] << 24) | ((u32)bmg[9] << 16) | ((u32)bmg[10] << 8) | (u32)bmg[11];
    if (fileSize > bmgSize) fileSize = (u32)bmgSize;

    const u8* inf1 = nullptr;
    const u8* dat1 = nullptr;
    const u8* mid1 = nullptr;
    u32 pos = 32;
    while (pos + 8 <= fileSize) {
        u32 secSize = ((u32)bmg[pos + 4] << 24) | ((u32)bmg[pos + 5] << 16) | ((u32)bmg[pos + 6] << 8) | (u32)bmg[pos + 7];
        if (secSize == 0 || pos + secSize > fileSize) break;
        if (std::memcmp(&bmg[pos], "INF1", 4) == 0) inf1 = &bmg[pos];
        else if (std::memcmp(&bmg[pos], "DAT1", 4) == 0) dat1 = &bmg[pos];
        else if (std::memcmp(&bmg[pos], "MID1", 4) == 0) mid1 = &bmg[pos];
        pos += secSize;
    }
    if (!inf1 || !dat1 || !mid1) return false;

    u16 numMsgs = ((u16)inf1[8] << 8) | inf1[9];
    u16 entrySize = ((u16)inf1[10] << 8) | inf1[11];
    u16 numMids = ((u16)mid1[8] << 8) | mid1[9];

    int foundIdx = -1;
    for (u16 i = 0; i < numMids && i < numMsgs; i++) {
        u32 id = ((u32)mid1[16 + i * 4] << 24) | ((u32)mid1[17 + i * 4] << 16) | ((u32)mid1[18 + i * 4] << 8) | (u32)mid1[19 + i * 4];
        if (id == stringId) {
            foundIdx = i;
            break;
        }
    }
    if (foundIdx < 0) return false;

    const u8* entry = inf1 + 16 + foundIdx * entrySize;
    u32 textOffset = ((u32)entry[0] << 24) | ((u32)entry[1] << 16) | ((u32)entry[2] << 8) | (u32)entry[3];
    const u8* rawText = dat1 + 8 + textOffset;

    size_t outIdx = 0;
    size_t inIdx = 0;
    while (outIdx + 1 < outSize) {
        u8 b = rawText[inIdx];
        if (b == 0) {
            break;
        } else if (b == 0x1A) {
            u8 tagLen = rawText[inIdx + 1];
            if (tagLen < 2) break;
            u8 tagGrp = rawText[inIdx + 2];
            u16 tagCode = ((u16)rawText[inIdx + 3] << 8) | rawText[inIdx + 4];
            if (tagGrp == 6 && tagCode == 2) {
                // Male (♂) icon
                outBuf[outIdx++] = (char)(dusk::version::isRegionJpn() ? 0x81 : 0xB2);
                if (dusk::version::isRegionJpn() && outIdx + 1 < outSize) {
                    outBuf[outIdx++] = (char)0x89;
                }
            } else if (tagGrp == 6 && tagCode == 3) {
                // Female (♀) icon
                outBuf[outIdx++] = (char)(dusk::version::isRegionJpn() ? 0x81 : 0xB3);
                if (dusk::version::isRegionJpn() && outIdx + 1 < outSize) {
                    outBuf[outIdx++] = (char)0x8A;
                }
            }
            inIdx += tagLen;
        } else {
            outBuf[outIdx++] = (char)b;
            inIdx++;
        }
    }
    outBuf[outIdx] = '\0';
    return true;
}

// Stretch of a 608-wide Revo layout over the 16:9 box of the canvas (RevoCanvas.hpp).
inline f32 subscreenScaleX() {
    return revoWidth() / 608.0f;
}

struct IconInitPos {
    f32 pos[4][2];
};
inline std::map<J2DScreen*, IconInitPos>& getRevoIconInitMap() {
    static std::map<J2DScreen*, IconInitPos> s_map;
    return s_map;
}

inline void setupRevoButtonText(J2DScreen* icon) {
    if (icon == NULL) return;
    getRevoIconInitMap().erase(icon);
    const u64 kTextTags[4] = {
        MULTI_CHAR('cont_at'), MULTI_CHAR('cont_at1'),
        MULTI_CHAR('cont_bt'), MULTI_CHAR('cont_bt1')
    };
    for (u64 tag : kTextTags) {
        if (J2DPane* p = icon->search(tag)) {
            if (p->getTypeID() == 19) {
                J2DTextBox* tb = static_cast<J2DTextBox*>(p);
                tb->setFont(mDoExt_getMesgFont());
                tb->setString(0x20, "");
            }
        }
    }
    for (u64 tag : {MULTI_CHAR('a_btn_cp'), MULTI_CHAR('b_btn_cp')}) {
        if (J2DPane* p = icon->search(tag)) {
            if (p->getTypeID() == 18) {
                static_cast<J2DPicture*>(p)->setBlackWhite(
                    JUtility::TColor(0, 0, 0, 0), JUtility::TColor(128, 128, 128, 255));
            }
        }
    }
}

// i_keepLit: the badge stays white (opaque) without a caption. The RPX greys it (white colour
// 0x808080, FUN_0293bca8 / 0x0293f98c in the file select, 0x02a0556c in the save menu) but the
// file select and the save menu keep it lit like the shop menus.
inline void setRevoButtonText(J2DScreen* icon, u16 stringID, bool isAButton,
                              bool i_keepLit = false) {
    if (icon == NULL) return;
    const u64 txtTag = isAButton ? MULTI_CHAR('cont_at') : MULTI_CHAR('cont_bt');
    const u64 outTag = isAButton ? MULTI_CHAR('cont_at1') : MULTI_CHAR('cont_bt1');
    const u64 btnTag = isAButton ? MULTI_CHAR('a_btn_cp') : MULTI_CHAR('b_btn_cp');

    J2DPane* badge = icon->search(btnTag);
    if (badge != NULL && badge->getTypeID() == 18) {
        static_cast<J2DPicture*>(badge)->setBlackWhite(
            JUtility::TColor(0, 0, 0, 0),
            (stringID != 0 || i_keepLit) ? JUtility::TColor(255, 255, 255, 255)
                                         : JUtility::TColor(128, 128, 128, 255));
    }

    for (u64 tag : {txtTag, outTag}) {
        J2DPane* p = icon->search(tag);
        if (p != NULL && p->getTypeID() == 19) {
            J2DTextBox* tb = static_cast<J2DTextBox*>(p);
            if (stringID == 0) {
                SAFE_STRCPY(tb->getStringPtr(), "");
            } else {
                dMeter2Info_getStringKanji(stringID, tb->getStringPtr(), NULL);
            }
        }
    }
}

inline void drawRevoIconScreen(J2DScreen* icon, J2DGrafContext* graf_port) {
    if (icon == NULL || graf_port == NULL) return;
    static const u64 kTags[4] = {MULTI_CHAR('g_abtn_n'), MULTI_CHAR('g_bbtn_n'),
                                 MULTI_CHAR('a_text_n'), MULTI_CHAR('b_text_n')};
    static const f32 kHio[4][3] = {{-22.3f, 24.9f, 1.3f}, {-22.3f, 25.0f, 1.3f},
                                   {-24.9f, 24.9f, 1.1f}, {-27.5f, 26.0f, 1.1f}};
    auto& inits = getRevoIconInitMap();
    auto it = inits.find(icon);
    if (it == inits.end()) {
        IconInitPos init;
        for (int i = 0; i < 4; i++) {
            J2DPane* p = icon->search(kTags[i]);
            init.pos[i][0] = p != NULL ? p->getTranslateX() : 0.0f;
            init.pos[i][1] = p != NULL ? p->getTranslateY() : 0.0f;
        }
        it = inits.emplace(icon, init).first;
    }
    for (int i = 0; i < 4; i++) {
        if (J2DPane* p = icon->search(kTags[i])) {
            p->translate(it->second.pos[i][0] + kHio[i][0], it->second.pos[i][1] + kHio[i][1]);
            p->scale(kHio[i][2], kHio[i][2]);
        }
    }
    const f32 sx = subscreenScaleX();
    if (J2DPane* p = icon->search(MULTI_CHAR('abtn_n'))) {
        p->scale(1.0f / sx, 1.0f);
    }
    if (J2DPane* p = icon->search(MULTI_CHAR('bbtn_n'))) {
        p->scale(1.0f / sx, 1.0f);
    }
    icon->scale(sx, 1.0f);
    RevoClip clip;
    icon->draw(revoMinX(), 0.0f, graf_port);
}

inline bool isN43Tag(u64 tag) {
    if (tag == 0) return false;
    const u32 tag43 = MULTI_CHAR('n_43');
    const u32 hi = (u32)(tag >> 32);
    if ((hi & 0xFFFFFF00) == 0x6E5F3400 && ((hi & 0xFF) == 0x33)) return true;
    const u32 lo = (u32)(tag & 0xFFFFFFFF);
    if ((lo & 0xFFFFFF00) == 0x6E5F3400 && ((lo & 0xFF) == 0x33)) return true;
    return false;
}

// i_nested: Wii U J2DPane::draw (0x02b34334) squeezes the local matrix of EVERY 'n_43' pane, so an
// 'n_43' pane inside another one ends up squeezed twice (e.g. the button plates btnon* / btnoff*
// of the choice window, inside abc_n).
// i_wiiuPivot: the Wii U multiplies only the linear part of row 0 (m00 m01 m02), so the squeeze
// pivots on the pane translation (local x = 0), not on the J2D rotate offset like scale() does.
// Moving the rotate offset to the origin (unrotated panes, scale 1) reproduces it exactly.
inline void applyN43(J2DPane* pane, f32 invSx, bool i_nested = false, bool i_wiiuPivot = false) {
    if (pane == NULL) return;
    if (isN43Tag(pane->getUserInfo()) || isN43Tag(pane->mInfoTag)) {
        if (i_wiiuPivot && pane->getRotateZ() == 0.0f) {
            pane->mRotateOffsetX = -pane->mBounds.i.x;
        }
        pane->scale(invSx, pane->getScaleY());
        if (!i_nested) {
            return;
        }
    }
    for (J2DPane* c = pane->getFirstChildPane(); c != NULL; c = c->getNextChildPane()) {
        applyN43(c, invSx, i_nested, i_wiiuPivot);
    }
}

// Draws a TPHD (Revo) screen the way the Wii U J2DPane::draw (RPX 0x02b34334) does on a 16:9 TV:
// the 608-wide layout is stretched to the whole width, except the panes tagged 'n_43' (user info),
// which keep their proportions. i_offsetX is in layout units (unstretched).
// Squeeze of the 'n_43' panes on the Wii U TV. Its J2DPane::draw uses 608 / ortho width, and the TV
// ortho is 808 units wide over 1920 px but 448 over 1080 px (not square pixels, see the constants of
// dMeterMap_c::draw 0x02a52c00). With our square-pixel ortho (796.4 wide at 16:9) the same pixels
// need 608 / 808 at 16:9, i.e. 1 / sx * 796.44 / 808. Checked on the field map captures: the
// "L Portals" and "+ Back" positions match the TV pixel for pixel with it.
inline f32 wiiuTvN43() {
    return (1.0f / subscreenScaleX()) * (796.444f / 808.0f);
}

inline void drawRevoScreen(J2DScreen* scr, J2DGrafContext* graf, f32 i_offsetX = 0.0f,
                           f32 i_offsetY = 0.0f, bool i_wiiuPivot = false) {
    if (scr == NULL || graf == NULL) return;
    const f32 sx = subscreenScaleX();
    applyN43(scr, i_wiiuPivot ? wiiuTvN43() : 1.0f / sx, false, i_wiiuPivot);
    scr->scale(sx, 1.0f);
    RevoClip clip;
    scr->draw(revoMinX() + i_offsetX * sx, i_offsetY, graf);
}

// Centre of a CPaneMgr pane where it is drawn by drawRevoScreen: CPaneMgr::getGlobalVtxCenter
// composes the local matrices (root stretch included, also valid for hidden panes) but not the
// draw offset, which is the left edge of the canvas.
inline Vec mgrDrawnCenter(CPaneMgr* m) {
    Vec c = {0.0f, 0.0f, 0.0f};
    if (m == NULL || m->getPanePtr() == NULL) return c;
    c = m->getGlobalVtxCenter(m->getPanePtr(), false, 0);
    c.x += revoMinX();
    return c;
}

// Menu backgrounds of a Revo screen reach the whole window while the rest of the screen stays in
// the 16:9 box (RevoCanvas.hpp): every visible picture / window covering the whole drawn layout is
// scaled uniformly about the centre of the box (all of them by the same pivot, so split
// backgrounds stay joined) until it covers the canvas. Uses last frame's global matrices, so call
// it every frame before drawing.
inline void fillCanvasBackgrounds(J2DPane* pane) {
    if (pane == NULL || !pane->isVisible()) return;
    struct Fill {
        f32 sx, sy;              // pane scale before ours
        f32 x0, y0, x1, y1;      // global rectangle before ours
        f32 pivotX, pivotY;      // box centre in the pane's local (bounds) coordinates
    };
    static std::unordered_map<J2DPane*, Fill> s_fill;
    const u16 type = pane->getTypeID();
    if ((type == 18 || type == 17) && pane->getAlpha() != 0) {
        auto it = s_fill.find(pane);
        const f32 bx = (revoMinX() + revoMaxX()) * 0.5f;
        const f32 by = 224.0f;
        if (it == s_fill.end()) {
            const Vec a = pane->getGlbVtx(0);
            const Vec b = pane->getGlbVtx(3);
            const f32 w = std::fabs(b.x - a.x);
            const f32 h = std::fabs(b.y - a.y);
            if (w >= revoWidth() * 0.97f && h >= 448.0f * 0.9f && pane->getRotateZ() == 0.0f) {
                Mtx inv;
                if (MTXInverse(pane->getGlbMtx(), inv) != 0) {
                    Vec g = {bx, by, 0.0f};
                    Vec l;
                    MTXMultVec(inv, &g, &l);
                    it = s_fill.emplace(pane, Fill{pane->getScaleX(), pane->getScaleY(),
                                                   std::min(a.x, b.x), std::min(a.y, b.y),
                                                   std::max(a.x, b.x), std::max(a.y, b.y), l.x, l.y})
                             .first;
                }
            }
        }
        if (it != s_fill.end()) {
            const Fill& f = it->second;
            f32 k = 1.0f;
            auto need = [&](f32 edgeDist, f32 paneDist) {
                if (paneDist > 1.0f) k = std::max(k, edgeDist / paneDist);
            };
            need(bx - mDoGph_gInf_c::getMinXF(), bx - f.x0);
            need(mDoGph_gInf_c::getMaxXF() - bx, f.x1 - bx);
            need(by - mDoGph_gInf_c::getMinYF(), by - f.y0);
            need(mDoGph_gInf_c::getMaxYF() - by, f.y1 - by);
            pane->mRotateOffsetX = f.pivotX - pane->mBounds.i.x;
            pane->mRotateOffsetY = f.pivotY - pane->mBounds.i.y;
            pane->scale(f.sx * k, f.sy * k);
            return;
        }
    }
    for (J2DPane* c = pane->getFirstChildPane(); c != NULL; c = c->getNextChildPane()) {
        fillCanvasBackgrounds(c);
    }
}

// Centre of a pane in global (drawn) coordinates (pane drawn this frame).
inline Vec paneGlobalCenter(J2DPane* p) {
    Vec c = {0.0f, 0.0f, 0.0f};
    if (p == NULL) return c;
    const Vec a = p->getGlbVtx(0);
    const Vec b = p->getGlbVtx(3);
    c.x = (a.x + b.x) * 0.5f;
    c.y = (a.y + b.y) * 0.5f;
    return c;
}

}  // namespace dusk::tphd
