//
// Translation Unit: d/d_menu_dmap_map
//

#include "d/dolzel.h" // IWYU pragma: keep

#include <cmath>
#include "d/d_com_inf_game.h"
#include "d/d_map.h"
#include "d/d_map_path_dmap.h"
#include "d/d_menu_dmap.h"
#include "d/d_menu_dmap_map.h"
#include "f_op/f_op_msg_mng.h"
#include "m_Do/m_Do_graphic.h"

#include <cstring>
#if TARGET_PC
#include "dusk/settings.h"
#include "dusk/tphd/HdMapPalette.hpp"
#endif
#if TARGET_PC
#include "dusk/interp/user_interface.h"
#endif

struct dMdm_HIO_prm_res_dst_s {
    static DUSK_GAME_DATA void* m_res;
};

bool renderingDmap_c::hasMap() const {
    return dMapInfo_n::chkGetMap();
}

bool renderingDmap_c::isDrawIconSingle2(dTres_c::data_s const* i_data, bool param_1,
                                        bool param_2, int param_3) const {
    JUT_ASSERT(995, i_data != NULL);

    bool rt = false;
    int var_r27 = dTres_c::getTypeToTypeGroupNo(i_data->mType);
    bool is_get_map = false;
    bool is_get_compass = false;

    if (dMapInfo_n::chkGetMap()) {
        is_get_map = true;
    }

    if (dMapInfo_n::chkGetCompass()) {
        is_get_compass = true;
    }

    bool var_r29 = false;
    if (is_get_compass && param_1) {
        var_r29 = true;
    }

    switch (var_r27) {
    default:
        JUT_ASSERT(1015, FALSE);
        break;
    case 0:
        if (var_r29 && i_data->mNo != 0xFF && !dComIfGs_isTbox(i_data->mNo)) {
            rt = true;
        }
        break;
    case 2:
        if (var_r29 && !dComIfGs_isTbox(i_data->mNo)) {
            rt = true;
        }
        break;
    case 1:
    case 8:
        if (param_1 && param_2) {
            rt = true;
        }
        break;
    case 4:
        JUT_ASSERT(1044, FALSE);
        break;
    case 5:
#if TARGET_PC
        if (dusk::getSettings().game.removeQuestMapMarkers &&
            dComIfGs_isEventBit(dSv_event_flag_c::saveBitLabels[0x190]))
        {
            break;
        }
#endif
        if ((i_data->mNo == 0xFF || (i_data->mNo != 0xFF && !dComIfGs_isTbox(i_data->mNo))) && (i_data->mSwBit == 0xFF || (i_data->mSwBit != 0xFF && dComIfGs_isSwitch(i_data->mSwBit, i_data->mRoomNo))) && param_1) {
            rt = true;
        }
        break;
    case 6:
        JUT_ASSERT(1070, FALSE);
        break;
    case 3:
        if (var_r29 && (i_data->mSwBit == 0xFF || (i_data->mSwBit != 0xFF && dComIfGs_isSwitch(i_data->mSwBit, i_data->mRoomNo))) && !dComIfGs_isStageBossEnemy()) {
            rt = true;
        }
        break;
    case 9:
        if (var_r29 && (i_data->mNo == 0xFF || (i_data->mNo != 0xFF && !dComIfGs_isTbox(i_data->mNo)))) {
            rt = true;
        }
        break;
    case 10:
        JUT_ASSERT(1100, FALSE);
        break;
    case 11:
        if (var_r29) {
            rt = true;
        }
        break;
    case 12:
        if (var_r29 && (i_data->mNo == 0xFF || (i_data->mNo != 0xFF && !dComIfGs_isTbox(i_data->mNo)))) {
            rt = true;
        }
        break;
    case 13:
    case 14:
        JUT_ASSERT(1123, FALSE);
        break;
    case 15:
        if (var_r29 && (i_data->mNo == 0xFF || (i_data->mNo != 0xFF && dComIfGs_isTbox(i_data->mNo)))) {
            rt = true;
        }
        break;
    case 16:
        if (!dComIfGs_isStageBossEnemy() && var_r29 && (i_data->mNo == 0xFF || (i_data->mNo != 0xFF && !dComIfGs_isTbox(i_data->mNo)))) {
            rt = true;
        }
        break;
    }

    return rt;
}

f32 renderingDmap_c::getPlayerCursorSize() {
    return 0.0f;
}

int renderingDmap_c::getLineWidthZoomBig(int param_0) {
    static const u8 l_lineWidthPatOff[] = {0, 0, 6, 0, 0};
    static const u8 l_lineWidthPatOn[] = {6, 6, 12, 0, 0};
    static const u8 l_lineWidthPatStay[] = {6, 6, 12, 0, 0};
    const u8* var_r28;

    var_r28 = l_lineWidthPatOff;
    if (mRoomNo == mRoomNoSingle) {
        var_r28 = l_lineWidthPatStay;
    } else if (dMapInfo_n::isVisitedRoom(mRoomNo)) {
        var_r28 = l_lineWidthPatOn;
    }

    return var_r28[param_0];
}

int renderingDmap_c::getLineWidthZoomSmall(int param_0) {
    static const u8 l_lineWidthPatOff[] = {0, 0, 6, 0, 0};
    static const u8 l_lineWidthPatOn[] = {0, 0, 6, 0, 0};
    static const u8 l_lineWidthPatStay[] = {0, 0, 6, 0, 0};
    const u8* var_r28;

    var_r28 = l_lineWidthPatOff;
    if (mRoomNo == mRoomNoSingle) {
        var_r28 = l_lineWidthPatStay;
    } else if (dMapInfo_n::isVisitedRoom(mRoomNo)) {
        var_r28 = l_lineWidthPatOn;
    }

    return var_r28[param_0];
}

int renderingDmap_c::getLineWidth(int param_0) {
    int var_r31;
    if (field_0x34 > 0.5f) {
        var_r31 = getLineWidthZoomBig(param_0);
    } else {
        var_r31 = getLineWidthZoomSmall(param_0);
    }

    return var_r31;
}

ATTRIBUTE_ALIGN(32) static u32 const l_paletteDmap_m[60] = {
    0x00000000,
    0x00000000,
    0x80008000,
    0x80008000,
    0x80008000,
    0x80008000,
    0x80008000,
    0x80008000,
    0x80008000,
    0x80008000,
    0x80008000,
    0x80008000,
    0xA208A208,
    0xA208A208,
    0x80008000,
    0x80008000,
    0x80008000,
    0x80008000,
    0x81008100,
    0x81008100,
    0xA208A208,
    0xA208A208,
    0x80008000,
    0x80008000,
    0x9CE79CE7,
    0x9CE79CE7,
    0x81108110,
    0x81108110,
    0xA208A208,
    0xA208A208,
    0x98109810,
    0x98109810,
    0x98009800,
    0x98009800,
    0x82008200,
    0x82008200,
    0xC3F0C3F0,
    0xC3F0C3F0,
    0x80008000,
    0x80008000,
    0x9CE79CE7,
    0x9CE79CE7,
    0x821F821F,
    0x821F821F,
    0xC3F0C3F0,
    0xC3F0C3F0,
    0xB01FB01F,
    0xB01FB01F,
    0xB000B000,
    0xB000B000,
    0x98C698C6,
    0x98C698C6,
    0xE739E739,
    0xE739E739,
    0x98C698C6,
    0x98C698C6,
    0xE739E739,
    0xE739E739,
    0x00000000,
    0x00000000,
};

const GXColor* renderingDmap_c::getColor(int param_0) {
    static const GXColor l_dungeon_offColor[] = {
        {0x08, 0x00, 0x00, 0x00},
        {0x0C, 0x00, 0x00, 0x00},
        {0x10, 0x00, 0x00, 0x00},
        {0x04, 0x00, 0x00, 0x00},
        {0x14, 0x00, 0x00, 0x00},
        {0x18, 0x00, 0x00, 0x00},
        {0x1C, 0x00, 0x00, 0x00},
        {0x20, 0x00, 0x00, 0x00},
        {0x24, 0x00, 0x00, 0x00},
    };

    static const GXColor l_dungeon_onColor[] = {
        {0x28, 0x00, 0x00, 0x00},
        {0x2C, 0x00, 0x00, 0x00},
        {0x30, 0x00, 0x00, 0x00},
        {0x04, 0x00, 0x00, 0x00},
        {0x34, 0x00, 0x00, 0x00},
        {0x38, 0x00, 0x00, 0x00},
        {0x3C, 0x00, 0x00, 0x00},
        {0x40, 0x00, 0x00, 0x00},
        {0x44, 0x00, 0x00, 0x00},
    };

    static const GXColor l_dungeon_stayColor[] = {
        {0x48, 0x00, 0x00, 0x00},
        {0x4C, 0x00, 0x00, 0x00},
        {0x50, 0x00, 0x00, 0x00},
        {0x04, 0x00, 0x00, 0x00},
        {0x54, 0x00, 0x00, 0x00},
        {0x58, 0x00, 0x00, 0x00},
        {0x5C, 0x00, 0x00, 0x00},
        {0x60, 0x00, 0x00, 0x00},
        {0x64, 0x00, 0x00, 0x00},
    };

    const GXColor* var_r28;

    var_r28 = l_dungeon_offColor;
    if (mRoomNo == mRoomNoSingle) {
        var_r28 = l_dungeon_stayColor;
    } else if (dMapInfo_n::isVisitedRoom(mRoomNo)) {
        var_r28 = l_dungeon_onColor;
    }

    return &var_r28[param_0];
}

#if TARGET_PC
// Wii U renderingDAmap getLineColor (RPX 0x029a7d28, shared by the dungeon map menu): line type 2
// takes the line table 0x10092c94 by room state (not visited / visited / current room: black,
// grey, white), every other line the type-6 colour of the room. The GC draws the lines with the
// fill colour, so the Wii U room walls / outlines were missing. Palette entries 0xD0-0xDB.
const GXColor* renderingDmap_c::getLineColor(int param_0, int param_1) {
    if (!dusk::tphd_active()) {
        return getColor(param_0);
    }
    static GXColor color = {0, 0, 0, 0};
    if (param_1 == 2) {
        const u8 st = getColor(0)->r;  // 0x08 off, 0x28 visited, 0x48 current
        color.r = st == 0x48 ? 0xD8 : (st == 0x28 ? 0xD4 : 0xD0);
    } else {
        color = *getColor(6);
    }
    return &color;
}
#endif

void renderingDmap_c::afterDrawPath() {
    renderingPlusDoor_c::afterDrawPath();
}

DUSK_GAME_DATA void* dMdm_HIO_prm_res_dst_s::m_res;

#if TARGET_PC
// TPHD: the Wii U renders the dungeon rooms with fixed RGBA colors per polygon type and room state
// (renderingDmap getColor RPX 0x029a7dd0, table 0x10092bfc: off / visited / current room) instead
// of the CI8 palette of res-d.arc (whose Wii U copy also has an opaque black index 0, which would
// hide the map background). The GC pipeline is kept and only the palette entries it uses are
// rewritten with those colors.
// Patched in place: makeResTIMG stores the palette as a 32-bit offset from the ResTIMG (heap), so
// the palette has to stay in the archive memory next to it.

static u16 hdRgb5a3(u32 rgba) {
    const u32 r = rgba >> 24, g = (rgba >> 16) & 0xFF, b = (rgba >> 8) & 0xFF, a = rgba & 0xFF;
    if (a == 0xFF) {
        return (u16)(0x8000 | ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3));
    }
    return (u16)(((a >> 5) << 12) | ((r >> 4) << 8) | ((g >> 4) << 4) | (b >> 4));
}

static void* hdDmapPalette(void* i_res) {
    if (!dusk::tphd_active() || i_res == NULL) {
        return i_res;
    }
    static const u32 kColor[9][3] = {
        {0x0000007f, 0x2c584bff, 0x58af95ff}, {0x0000007f, 0x162c2cff, 0x2c584bff},
        {0x0000007f, 0x000000ff, 0x000000ff}, {0x00000000, 0x00000000, 0x00000000},
        {0x0000007f, 0x202020ff, 0x404040ff}, {0x0000007f, 0x00407fff, 0x0080ffff},
        {0x0000007f, 0x477f7fff, 0x8fffffff}, {0x0000007f, 0x30007fff, 0x6000ffff},
        {0x0000007f, 0x300000ff, 0x600000ff},
    };
    // Palette indices of renderingDmap_c::getColor (off / on / stay).
    static const u8 kIndex[3][9] = {
        {0x08, 0x0C, 0x10, 0x04, 0x14, 0x18, 0x1C, 0x20, 0x24},
        {0x28, 0x2C, 0x30, 0x04, 0x34, 0x38, 0x3C, 0x40, 0x44},
        {0x48, 0x4C, 0x50, 0x04, 0x54, 0x58, 0x5C, 0x60, 0x64},
    };
    u8* pal = static_cast<u8*>(i_res);
    auto put = [pal](int i_entry, u16 i_col) {
        pal[i_entry * 2] = (u8)(i_col >> 8);
        pal[i_entry * 2 + 1] = (u8)i_col;
    };
    for (int i = 0; i < 4; i++) {
        put(i, 0x0000);  // outside the rooms: transparent
    }
    for (int st = 0; st < 3; st++) {
        for (int t = 0; t < 9; t++) {
            const u16 col = hdRgb5a3(kColor[t][st]);
            for (int k = 0; k < 4; k++) {
                put(kIndex[st][t] + k, col);
                dusk::tphd::mapPaletteSetExact(pal, kIndex[st][t] + k, kColor[t][st]);
            }
        }
    }
    return i_res;
}
#endif

#if TARGET_PC
// TPHD: 256-entry copy of the res-d palette. The treasure / boss / key icons drawn into the map
// texture use palette indices 0x7C-0xA7, past the 120 entries of res-d (on the GC those read the
// TLUT memory left by the minimap palette). The copy fills them from the minimap palette.
static u8* s_hdDmapPal = NULL;

static void* hdDmapBuildPalette(void* i_res) {
    if (!dusk::tphd_active() || i_res == NULL) {
        return i_res;
    }
    if (s_hdDmapPal == NULL) {
        s_hdDmapPal = JKR_NEW_ARRAY_ARGS(u8, 256 * 2, 0x20);
        if (s_hdDmapPal == NULL) {
            return hdDmapPalette(i_res);
        }
    }
    std::memset(s_hdDmapPal, 0, 256 * 2);
    if (dMap_HIO_prm_res_dst_s::m_res != NULL) {
        std::memcpy(s_hdDmapPal, dMap_HIO_prm_res_dst_s::m_res->palette_data, 51 * 4 * 2);
    }
    std::memcpy(s_hdDmapPal, i_res, 120 * 2);
    hdDmapPalette(s_hdDmapPal);
    {
        auto put4 = [](int i_entry, u32 i_rgba) {
            const u16 col = hdRgb5a3(i_rgba);
            for (int k = 0; k < 4; k++) {
                s_hdDmapPal[(i_entry + k) * 2] = (u8)(col >> 8);
                s_hdDmapPal[(i_entry + k) * 2 + 1] = (u8)col;
                dusk::tphd::mapPaletteSetExact(s_hdDmapPal, i_entry + k, i_rgba);
            }
        };
        // Background / type 3 (getBackColor 0x029aa318 without the HUD flag: transparent).
        put4(0x04, 0x00000000);
        // Line table 0x10092c94 (renderingDmap_c::getLineColor).
        put4(0xD0, 0x0000007f);
        put4(0xD4, 0x7f7f7fff);
        put4(0xD8, 0xffffffff);
        // Doors (renderingPlusDoor_c::drawDoorCommon TPHD, RPX 0x029ab068): black outline C1
        // 0x000000ff and yellow fill C1 + C2 = 0xffff00ff, drawn as indices 0xF4 / 0xF8.
        put4(0xF4, 0x000000ff);
        put4(0xF8, 0xffff00ff);
    }
    DCStoreRange(s_hdDmapPal, 256 * 2);
    return s_hdDmapPal;
}
#endif

void dMenu_DmapMap_c::_create(u16 param_0, u16 param_1, u16 param_2, u16 param_3,
                              void* res) {
    JUT_ASSERT(1569, res != NULL);
#if TARGET_PC
    res = hdDmapBuildPalette(res);
#endif
    dMdm_HIO_prm_res_dst_s::m_res = res;
    field_0x84 = 120;
    setTexture(param_0, param_1, param_2, param_3);
}

void dMenu_DmapMap_c::_delete() {
    for (int i = 0; i < 2; i++) {
        if (mMapImage_p[i] != NULL) {
#if TARGET_PC
            GXDestroyCopyTex(mMapImage_p[i]);
#endif
            JKR_DELETE_ARRAY(mMapImage_p[i]);
        }

        if (mResTIMG[i] != NULL) {
            JKR_DELETE_ARRAY(mResTIMG[i]);
        }
    }
#if TARGET_PC
    if (s_hdDmapPal != NULL) {
        if (dMdm_HIO_prm_res_dst_s::m_res == s_hdDmapPal) {
            dMdm_HIO_prm_res_dst_s::m_res = NULL;
        }
        JKR_DELETE_ARRAY(s_hdDmapPal);
        s_hdDmapPal = NULL;
    }
#endif
}

void dMenu_DmapMap_c::setTexture(u16 width, u16 height, u16 param_2, u16 param_3) {
    for (int lp1 = 0; lp1 < 2; lp1++) {
#ifdef TARGET_PC
        u32 sz = 0x20; // No need to allocate memory for texture
#else
        u32 sz = GXGetTexBufferSize(width, height, 9, 0, 0);
#endif
        mMapImage_p[lp1] = JKR_NEW_ARRAY_ARGS(u8, sz, 0x20);
        JUT_ASSERT(1672, mMapImage_p[lp1] != NULL);
        mRend[lp1].init(mMapImage_p[lp1], width, height, param_2, param_3);

        mResTIMG[lp1] = JKR_NEW_ARGS (0x20) ResTIMG;
        JUT_ASSERT(1687, mResTIMG[lp1] != NULL);
#if TARGET_PC
        const u16 hd_pal_num = (s_hdDmapPal != NULL && dMdm_HIO_prm_res_dst_s::m_res == s_hdDmapPal) ? 64 : 30;
        mRend[lp1].makeResTIMG(mResTIMG[lp1], width, height, mMapImage_p[lp1], (u8*)dMdm_HIO_prm_res_dst_s::m_res, hd_pal_num);
#else
        mRend[lp1].makeResTIMG(mResTIMG[lp1], width, height, mMapImage_p[lp1], (u8*)dMdm_HIO_prm_res_dst_s::m_res, 30);
#endif
    }
}

void dMenu_DmapMap_c::setPos(int param_0, int param_1, f32 param_2, f32 param_3, f32 param_4,
                             bool param_5, f32 param_6) {
    mRend[0].entry(param_2, param_3, param_4, dComIfGp_roomControl_getStayNo(), param_0, param_6);
    mRend[1].entry(param_2, param_3, param_4, dComIfGp_roomControl_getStayNo(), param_1, param_6);
}

f32 dMenu_StageMapCtrl_c::getMapBlendPer() const {
    f32 var_f31;
    if (field_0x90 == 0.0f) {
        var_f31 = 1.0f;
    } else {
        var_f31 = std::fmod(field_0x90, 1.0f);
    }

    return var_f31;
}

f32 dMenu_StageMapCtrl_c::getPixelStageSizeX() const {
    f32 var_f31 = dMpath_c::getSizeX();
    return var_f31 * (1.0f / field_0xbc);
}

f32 dMenu_StageMapCtrl_c::getPixelStageSizeZ() const {
    f32 var_f31 = dMpath_c::getSizeZ();
    return var_f31 * (1.0f / field_0xbc);
}

f32 dMenu_StageMapCtrl_c::getPixelCenterX() const {
    f32 var_f31 = dMpath_c::getCenterX();
    return (1.0f / field_0xbc) * (field_0x9c - var_f31);
}

f32 dMenu_StageMapCtrl_c::getPixelCenterZ() const {
    f32 var_f31 = dMpath_c::getCenterZ();
    return (1.0f / field_0xbc) * (field_0xa0 - var_f31);
}

void dMenu_StageMapCtrl_c::initGetTreasureList(u8 param_0, s8 param_1) {
    field_0xf6 = param_0;
    field_0xf7 = param_1;
    field_0x8c = dTres_c::getFirstData(param_0);
    field_0xe4 = dTres_c::getTypeGroupNumber(param_0);
}

inline static s16 rightModeCnvRot(s16 param_0) {
    return IF_DUSK(dusk::getSettings().game.enableMirrorMode ? -param_0 :) param_0;
}

bool dMenu_StageMapCtrl_c::getTreasureList(f32* o_posX, f32* o_posY, s8* param_2, u8* o_swbit,
                                           s8* o_roomNo) {
    bool var_r30 = false;

    while (field_0xe4 != 0 && !var_r30) {
        BE(Vec) sp10;
        sp10 = *field_0x8c->getPos();

        if (getRendPointer(0)->isDrawIconSingle(field_0x8c->getDataPointer(), (s8)dComIfGp_roomControl_getStayNo(), field_0xf7, 1, true, &sp10)) {
            cnvPosTo2Dpos(sp10.x, sp10.z, o_posX, o_posY);

            if (param_2 != NULL) {
                *param_2 = rightModeCnvRot((s8)field_0x8c->getAngleY()) >> 8;
            }

            if (o_swbit != NULL) {
                *o_swbit = field_0x8c->getSwBit();
            }

            if (o_roomNo != NULL) {
                *o_roomNo = field_0x8c->getRoomNo();
            }

            var_r30 = true;
        }

        field_0x8c = dTres_c::getNextData(field_0x8c);
        field_0xe4--;
    }

    return var_r30;
}

inline static f32 rightModeCnvPos(f32 param_0) {
    return IF_DUSK(dusk::getSettings().game.enableMirrorMode ? -param_0 :) param_0;
}

void dMenu_StageMapCtrl_c::cnvPosTo2Dpos(f32 param_0, f32 param_1, f32* param_2,
                                         f32* param_3) const {
    if (param_2 != NULL) {
        *param_2 = (0.5f * field_0x94) + rightModeCnvPos((1.0f / field_0xbc) * (param_0 - field_0x9c));
    }

    if (param_3 != NULL) {
        *param_3 = (0.5f * field_0x98) + ((1.0f / field_0xbc) * (param_1 - field_0xa0));
    }
}

void dMenu_StageMapCtrl_c::getPlayerDrawInfo(f32* i_dispX, f32* i_dispY, s16* i_rotY) const {
    JUT_ASSERT(2030, i_dispX != NULL);
    JUT_ASSERT(2031, i_dispX != NULL);
    JUT_ASSERT(2032, i_rotY != NULL);

    Vec sp10 = dMapInfo_n::getMapPlayerPos();
    cnvPosTo2Dpos(sp10.x, sp10.z, i_dispX, i_dispY);

    if (i_rotY != NULL) {
        *i_rotY = rightModeCnvRot(dMapInfo_n::getMapPlayerAngleY());
    }
}

s8 dMenu_StageMapCtrl_c::getRestartDrawInfo(f32* i_dispX, f32* i_dispY, s16* i_rotY) const {
    JUT_ASSERT(2057, i_dispX != NULL);
    JUT_ASSERT(2058, i_dispX != NULL);
    JUT_ASSERT(2059, i_rotY != NULL);

    Vec sp10 = dMapInfo_n::getMapRestartPos();
    cnvPosTo2Dpos(sp10.x, sp10.z, i_dispX, i_dispY);
    s16 var_r29 = dMapInfo_n::getMapRestartAngleY();

    if (i_rotY != NULL) {
        *i_rotY = rightModeCnvRot(var_r29);
    }

    return dMapInfo_c::calcNowStayFloorNo(sp10.y, true);
}

void dMenu_StageMapCtrl_c::setPlusNowStayFloorNo(s8 param_0, u8 param_1) {
    field_0xea = param_0;
    field_0xf0 = param_1;
}

void dMenu_StageMapCtrl_c::moveFloor() {
    if ((field_0xf3 DUSK_IF_ELSE(<= , ==) 0) && (field_0xf1 DUSK_IF_ELSE(<= , ==) 0)) {
        if (field_0xea != 0) {
            field_0xf3 = field_0xf0;
            field_0x90 = 0.0f;

            if (field_0xea > 0) {
                if (field_0xe7 < field_0xed) {
                    field_0xc0 = makeStayFloorBlendPer(field_0xe7, 0.0f);
                    field_0xc4 = makeStayFloorBlendPer(field_0xe7 + field_0xea, 0.0f);
                    field_0xe8 = field_0xe7;
                    field_0xf3 = field_0xf0;
                    field_0xf1 = field_0xf3;
                    field_0xe9 = field_0xe7;
                    field_0xe7 += field_0xea;
                }
            } else if ((field_0xea < 0) && (field_0xe7 > field_0xee)) {
                field_0xc0 = makeStayFloorBlendPer(field_0xe7, 0.0f);
                field_0xc4 = makeStayFloorBlendPer(field_0xe7 + field_0xea, 0.0f);
                field_0xe8 = field_0xe7;
                field_0xe7 += field_0xea;
                field_0xe9 = field_0xe7;
                field_0xf3 = field_0xf0;
                field_0xf1 = field_0xf3;
            }

            field_0xea = 0;
        }
    } else if (field_0xf1 DUSK_IF_ELSE(>, !=) 0 && field_0xea != 0) {
        if (field_0xea > 0) {
            if (field_0xe7 < field_0xed) {
                if ((field_0xe7 == std::floor(field_0xc4) && field_0xe8 == field_0xe7 + field_0xea) || field_0x90 > 0.5f) {
                    field_0xf1 = field_0x90 * field_0xf0;
                    field_0xf3 = field_0xf1;
                    field_0xc0 = field_0xc4;
                }

                field_0xc4 = makeStayFloorBlendPer(field_0xe7 + field_0xea, 0.0f);
                field_0xe8 = field_0xe7;
                field_0xe7 += field_0xea;
            }
        } else if (field_0xea < 0) {
            if (field_0xe7 > field_0xee) {
                if ((field_0xe7 == std::floor(field_0xc4) && field_0xe8 == field_0xe7 + field_0xea) || field_0x90 > 0.5f) {
                    field_0xf1 = field_0x90 * field_0xf0;
                    field_0xf3 = field_0xf1;
                    field_0xc0 = field_0xc4;
                }

                field_0xc4 = makeStayFloorBlendPer(field_0xe7 + field_0xea, 0.0f);
                field_0xe8 = field_0xe7;
                field_0xe7 += field_0xea;
            }
        }

        field_0x90 = 0.0f;
        field_0xea = 0;
        field_0xeb = field_0xe7;
    }

    if (field_0xf1 DUSK_IF_ELSE(>, !=) 0) {
        f32 temp_f31 = (f32)(field_0xf0 - field_0xf3) / (f32)field_0xf0;

        field_0xeb = getFloorNo(field_0xc4);
        field_0xec = getFloorNo(field_0xc0);
        field_0x90 = getBlendPer(temp_f31 - std::floor(temp_f31));

        DUSK_IF_ELSE(dusk::vdt::advance_toward_frame(field_0xf1, 0.0f, 1.0f), field_0xf1--);
        if (field_0xf1 DUSK_IF_ELSE(<=, ==) 0) {
            field_0xe8 = field_0xe7;
        }
    }

    if (field_0xf3 DUSK_IF_ELSE(>, !=) 0) {
        DUSK_IF_ELSE(dusk::vdt::advance_toward_frame(field_0xf3, 0.0f, 1.0f), field_0xf3--);
        if (field_0xf3 DUSK_IF_ELSE(>, !=) 0 && field_0xf1 DUSK_IF_ELSE(>, !=) 0) {
            f32 temp_f30 = (f32)(field_0xf0 - field_0xf3) / (f32)field_0xf0;
            field_0xeb = getFloorNo(field_0xc4);
            field_0xec = getFloorNo(field_0xc0);
            field_0x90 = temp_f30;
        } else {
            field_0x90 = 0.0f;
            field_0xeb = getFloorNo(field_0xc4);
        }
    }
}

void dMenu_StageMapCtrl_c::wait_init_proc() {
    field_0x90 = 0.0f;
    field_0xe8 = field_0xe7;
}

void dMenu_StageMapCtrl_c::wait_proc() {
    moveFloor();
}

void dMenu_StageMapCtrl_c::zoomIn_init_proc() {
    field_0xf4 = field_0xf2;
    field_0xd8 = 0.0f;
}

void dMenu_DmapMapCtrl_c::getZoomMinMaxCheck(f32* param_0, f32* param_1, f32* param_2,
                                             f32* param_3, bool* param_4, bool* param_5) {
    f32 var_f31 = FLT_MAX;
    f32 var_f29 = -FLT_MAX;
    f32 var_f30 = FLT_MAX;
    f32 var_f28 = -FLT_MAX;

    f32 sp8 = getZoomCmPerPixel();
    for (int i = 0; i < 64; i++) {
        if (dMpath_c::getRoomPointer(0, i) || dMpath_c::getRoomPointer(1, i)) {
            dStage_FileList2_dt_c* fileList2_p = dStage_roomControl_c::getFileList2(i);
            JUT_ASSERT(2360, fileList2_p != NULL);

            if (fileList2_p != NULL && (dMapInfo_n::chkGetMap() || dMapInfo_n::chkGetCompass() || dMapInfo_n::isVisitedRoom(i) || field_0xe6 == i)) {
                f32 sp18, sp14, sp10, spC;
                dMapInfo_n::getRoomMinMaxXZ(i, &sp18, &sp10, &sp14, &spC);

                if (sp18 < var_f31) {
                    var_f31 = sp18;
                }
                if (sp14 > var_f29) {
                    var_f29 = sp14;
                }
                if (sp10 < var_f30) {
                    var_f30 = sp10;
                }
                if (spC > var_f28) {
                    var_f28 = spC;
                }
            }
        }
    }

    if (var_f29 - var_f31 < 3600.0f) {
        var_f31 = 0.5f * (var_f31 + var_f29);
        var_f29 = var_f31;
    }

    if (var_f28 - var_f30 < 3600.0f) {
        var_f30 = 0.5f * (var_f30 + var_f28);
        var_f28 = var_f30;
    }

    if (param_0 != NULL) {
        *param_0 = var_f31;
    }

    if (param_1 != NULL) {
        *param_1 = var_f29;
    }

    if (param_2 != NULL) {
        *param_2 = var_f30;
    }

    if (param_3 != NULL) {
        *param_3 = var_f28;
    }

    if (param_4 != NULL) {
        if (var_f31 == var_f29) {
            *param_4 = 1;
        } else {
            *param_4 = 0;
        }
    }

    if (param_5 != NULL) {
        if (var_f30 == var_f28) {
            *param_5 = 1;
        } else {
            *param_5 = 0;
        }
    }
}

void dMenu_DmapMapCtrl_c::calcZoomCenter(f32* param_0, f32* param_1) {
    f32 spC = 0.0f;
    f32 sp8 = 0.0f;

    dStage_FileList2_dt_c* fileList2_p = dStage_roomControl_c::getFileList2(field_0xe6);
    JUT_ASSERT(2437, fileList2_p != NULL);

    if (fileList2_p != NULL) {
        dMapInfo_n::getRoomCenter(field_0xe6, &spC, &sp8);
    }

    if (param_0 != NULL) {
        *param_0 = spC;
    }

    if (param_1 != NULL) {
        *param_1 = sp8;
    }
}

f32 dMenu_DmapMapCtrl_c::getZoomCmPerPixel() {
    f32 var_f30 = field_0x98 < field_0x94 ? field_0x98 : field_0x94;

    stage_stag_info_class* pstag = dComIfGp_getStage()->getStagInfo();
    JUT_ASSERT(2467, pstag != NULL);

    f32 var_f29 = 10800.0f / var_f30;
    f32 var_f31 = 0.0f;

    if (pstag != NULL) {
        var_f31 = pstag->field_0x24;
        if (var_f31 == 0.0f) {
            var_f31 = 1.5f * pstag->field_0x20;
        }
    }

    if (var_f31 != 0.0f) {
        var_f29 = var_f31 / var_f30;
    }

    return var_f29;
}

ATTRIBUTE_ALIGN(32) static u32 l_data[61] = {
    0x80008000,
    0x80008000,
    0x00000000,
    0x00000000,
    0x80008000,
    0x80008000,
    0x80008000,
    0x80008000,
    0x80008000,
    0x80008000,
    0x80008000,
    0x80008000,
    0x80008000,
    0x80008000,
    0xA208A208,
    0xA208A208,
    0x80008000,
    0x80008000,
    0x80008000,
    0x80008000,
    0x81008100,
    0x81008100,
    0xA208A208,
    0xA208A208,
    0x80008000,
    0x80008000,
    0x9CE79CE7,
    0x9CE79CE7,
    0x81108110,
    0x81108110,
    0xA208A208,
    0xA208A208,
    0x98109810,
    0x98109810,
    0x98009800,
    0x98009800,
    0x82008200,
    0x82008200,
    0xC3F0C3F0,
    0xC3F0C3F0,
    0x80008000,
    0x80008000,
    0x9CE79CE7,
    0x9CE79CE7,
    0x821F821F,
    0x821F821F,
    0xC3F0C3F0,
    0xC3F0C3F0,
    0xB01FB01F,
    0xB01FB01F,
    0xB000B000,
    0xB000B000,
    0x98C698C6,
    0x98C698C6,
    0xE739E739,
    0xE739E739,
    0x98C698C6,
    0x98C698C6,
    0xE739E739,
    0xE739E739,
    0x45610000,
};

typedef void (dMenu_StageMapCtrl_c::*process_fn)();
static process_fn init_process[] = {
    &dMenu_StageMapCtrl_c::wait_init_proc,
    &dMenu_StageMapCtrl_c::zoomIn_init_proc,
    &dMenu_StageMapCtrl_c::zoomWait_init_proc,
    &dMenu_StageMapCtrl_c::zoomOut_init_proc,
};

void dMenu_StageMapCtrl_c::initZoomIn(u8 param_0) {
    f32 sp10 = 0.0f;
    f32 spC = 0.0f;
    calcZoomCenter(&sp10, &spC);

    field_0xac = sp10;
    field_0xb0 = spC;
    field_0xe0 = getZoomCmPerPixel();
    field_0xf2 = param_0;
    field_0xf5 = 1;

    (this->*init_process[field_0xf5])();
}

void dMenu_StageMapCtrl_c::initZoomIn(u8 param_0, f32 param_1, f32 param_2) {
    field_0xe0 = getZoomCmPerPixel();
    field_0xac = field_0xa4 + (field_0xdc * param_1);
    field_0xb0 = field_0xa8 + (field_0xdc * param_2);

    if (field_0xac < field_0xc8) {
        field_0xac = field_0xc8;
    } else if (field_0xac > field_0xcc) {
        field_0xac = field_0xcc;
    }

    if (field_0xb0 < field_0xd0) {
        field_0xb0 = field_0xd0;
    } else if (field_0xb0 > field_0xd4) {
        field_0xb0 = field_0xd4;
    }

    field_0xf2 = param_0;
    field_0xf5 = 1;

    (this->*init_process[field_0xf5])();
}

void dMenu_StageMapCtrl_c::initZoomInCenterHold(u8 param_0) {
    field_0xe0 = getZoomCmPerPixel();
    field_0xf2 = param_0;
    field_0xf5 = 1;

    (this->*init_process[field_0xf5])();
}

void dMenu_StageMapCtrl_c::zoomCalcSet(f32 param_0) {
    field_0x9c = field_0xa4 + (param_0 * (field_0xac - field_0xa4));
    field_0xa0 = field_0xa8 + (param_0 * (field_0xb0 - field_0xa8));
    field_0xbc = field_0xdc + (param_0 * (field_0xe0 - field_0xdc));
}

void dMenu_StageMapCtrl_c::zoomIn_proc() {
    IF_DUSK(dusk::vdt::advance_toward_frame(field_0xf4, 0.0f, 1.0f));
    f32 temp_f1 = DUSK_IF_ELSE(dusk::vdt::present_sine_ease(field_0xf2, field_0xf2 - field_0xf4),
                               fopMsgM_valueIncrease(field_0xf2, field_0xf2 - field_0xf4, 4));
    field_0xd8 = temp_f1;
    zoomCalcSet(temp_f1);

    IF_NOT_DUSK(field_0xf4--);
    if (field_0xf4 DUSK_IF_ELSE(<=, <) 0) {
        field_0xf5 = 2;
    }
}

void dMenu_StageMapCtrl_c::initZoomOut(u8 param_0) {
    field_0xf2 = param_0;
    field_0xf5 = 3;

    (this->*init_process[field_0xf5])();
}

void dMenu_StageMapCtrl_c::initZoomWait(f32 param_0, f32 param_1) {
    field_0xac = param_0;
    field_0xb0 = param_1;

    field_0x9c = field_0xac;
    field_0xa0 = field_0xb0;
    field_0xe0 = getZoomCmPerPixel();
    field_0xbc = getZoomCmPerPixel();
    field_0xf5 = 2;

    (this->*init_process[field_0xf5])();
}

void dMenu_StageMapCtrl_c::zoomWait_init_proc() {
    field_0xb4 = 0.0f;
    field_0xb8 = 0.0f;
    field_0xd8 = 1.0f;
    field_0x90 = 0.0f;
    field_0xe8 = field_0xe7;
}

bool dMenu_DmapMapCtrl_c::isEnableZoomMove() const {
    return field_0xf3 DUSK_IF_ELSE(<=, ==) 0 && (!getDisableZoomMoveFlgX() || !getDisableZoomMoveFlgZ());
}

void dMenu_StageMapCtrl_c::zoomWait_proc() {
    if (isEnableZoomMove()) {
        if (field_0xf8 == 0 && fabsf(field_0xb4) > 0.01f) {
            field_0xac += field_0xb4;
        }
        if (field_0xf9 == 0 && fabsf(field_0xb8) > 0.01f) {
            field_0xb0 += field_0xb8;
        }
    }

    if (field_0xf8 == 0) {
        if (field_0xac <= field_0xc8) {
            field_0xac = field_0xc8;
        }
        if (field_0xac >= field_0xcc) {
            field_0xac = field_0xcc;
        }
    }

    if (field_0xf9 == 0) {
        if (field_0xb0 <= field_0xd0) {
            field_0xb0 = field_0xd0;
        }
        if (field_0xb0 >= field_0xd4) {
            field_0xb0 = field_0xd4;
        }
    }

    field_0x9c = field_0xac;
    field_0xa0 = field_0xb0;
    field_0xb4 = 0.0f;
    field_0xb8 = 0.0f;
    moveFloor();
}

void dMenu_StageMapCtrl_c::zoomOut_init_proc() {
    field_0xf4 = field_0xf2;
    field_0xd8 = 1.0f;
}

void dMenu_StageMapCtrl_c::zoomOut_proc() {
    IF_DUSK(dusk::vdt::advance_toward_frame(field_0xf4, 0.0f, 1.0f));
    f32 temp_f1 = DUSK_IF_ELSE(dusk::vdt::present_sine_ease(field_0xf2, field_0xf2 - field_0xf4),
                               fopMsgM_valueIncrease(field_0xf2, field_0xf2 - field_0xf4, 4));
    zoomCalcSet(1.0f - temp_f1);
    field_0xd8 = 1.0f - temp_f1;

    IF_NOT_DUSK(field_0xf4--);
    if (field_0xf4 DUSK_IF_ELSE(<=, <) 0) {
        field_0xf5 = 0;
    }
}

static process_fn move_process[] = {
    &dMenu_StageMapCtrl_c::wait_proc,
    &dMenu_StageMapCtrl_c::zoomIn_proc,
    &dMenu_StageMapCtrl_c::zoomWait_proc,
    &dMenu_StageMapCtrl_c::zoomOut_proc,
};

void dMenu_StageMapCtrl_c::move() {
    u8 temp_r30 = field_0xf5;
    (this->*move_process[field_0xf5])();

    if (field_0xf5 != temp_r30) {
        (this->*init_process[field_0xf5])();
    }
}

void dMenu_DmapMapCtrl_c::draw() {
    if (field_0xef != 0) {
        setPos(field_0xeb, field_0xec, field_0x9c,
            field_0xa0, field_0xbc, true, field_0xd8);
    }
}

s8 dMenu_StageMapCtrl_c::getPlayerStayFloorNo() const {
    s8 var_r31 = 0;
    if (dMapInfo_c::getNowStayFloorNoDecisionFlg()) {
        var_r31 = dMapInfo_c::getNowStayFloorNo();
    }

    return var_r31;
}

void dMenu_StageMapCtrl_c::_create(u16 param_0, u16 param_1, u16 param_2, u16 param_3,
                                   void* param_4) {
    s8 var_r31;
    if (dMapInfo_c::getNowStayFloorNoDecisionFlg()) {
        var_r31 = dMapInfo_c::getNowStayFloorNo();
    }

    _create(param_0, param_1, param_2, param_3, var_r31, param_4);
}

DUSK_GAME_DATA f32 dMenu_StageMapCtrl_c::m_zoomCenterMinX;

DUSK_GAME_DATA f32 dMenu_StageMapCtrl_c::m_zoomCenterMaxX;

DUSK_GAME_DATA f32 dMenu_StageMapCtrl_c::m_zoomCenterMinZ;

DUSK_GAME_DATA f32 dMenu_StageMapCtrl_c::m_zoomCenterMaxZ;

void dMenu_StageMapCtrl_c::_create(u16 width, u16 height, u16 param_2, u16 param_3,
                                   s8 param_4, void* param_5) {
    field_0xe6 = dComIfGp_roomControl_getStayNo();
    field_0xe7 = param_4;
    field_0xeb = field_0xe7;
    field_0xec = field_0xe7;
    dMpath_c::getTopBottomFloorNo(&field_0xed, &field_0xee);

    int temp_r30 = (field_0xed - field_0xee) + 1;
    if (temp_r30 > 0) {
        field_0xef = temp_r30;
    }

    field_0x94 = param_2;
    field_0x98 = param_3;
    f32 var_f26 = field_0x98 > field_0x94 ? field_0x98 : field_0x94;

    dMenu_DmapMap_c::_create(width, height, param_2, param_3, param_5);
    
    getInitDispCenter(&field_0x9c, &field_0xa0);
    field_0xa4 = field_0x9c;
    field_0xa8 = field_0xa0;

    getInitWholeMapScale(&field_0xdc, 1.2f * dMpath_c::getSizeX(), 1.2f * dMpath_c::getSizeZ(), 288.0f, 297.0f);
    field_0xbc = field_0xdc;

    getZoomMinMaxCheck(&field_0xc8, &field_0xcc, &field_0xd0, &field_0xd4, &field_0xf8, &field_0xf9);

    m_zoomCenterMinX = field_0xc8;
    m_zoomCenterMaxX = field_0xcc;
    m_zoomCenterMinZ = field_0xd0;
    m_zoomCenterMaxZ = field_0xd4;

    field_0xf5 = 0;
    (this->*init_process[field_0xf5])();
}

void dMenu_StageMapCtrl_c::_delete() {
    dMenu_DmapMap_c::_delete();
}

bool dMenu_StageMapCtrl_c::isEnableZoomIn() {
    bool var_r30 = 0;
    if (field_0xf3 DUSK_IF_ELSE(<=, ==) 0 && dStage_roomControl_c::getFileList2(field_0xe6) != NULL) {
        var_r30 = 1;
    }

    return var_r30;
}

bool dMenu_StageMapCtrl_c::isEnableZoomOut() {
    return field_0xf3 DUSK_IF_ELSE(<=, ==) 0;
}

void dMenu_StageMapCtrl_c::setPlusZoomCenterX(f32 param_0) {
    field_0xb4 = param_0;
}

void dMenu_StageMapCtrl_c::setPlusZoomCenterZ(f32 param_0) {
    field_0xb8 = param_0;
}

void dMenu_DmapMapCtrl_c::getInitWholeMapScale(f32* param_0, f32 param_1, f32 param_2,
                                               f32 param_3, f32 param_4) {
    if (param_0 != NULL) {
        f32 temp_f31 = param_1 / param_3;
        f32 temp_f30 = param_2 / param_4;

        f32 var_f29;
        if (temp_f31 > temp_f30) {
            var_f29 = temp_f31;
        } else {
            var_f29 = temp_f30;
        }

        *param_0 = var_f29;
    }
}

void dMenu_DmapMapCtrl_c::getInitDispCenter(f32* param_0, f32* param_1) {
    if (param_0 != NULL) {
        *param_0 = dMpath_c::getCenterX();
    }

    if (param_1 != NULL) {
        *param_1 = dMpath_c::getCenterZ();
    }
}

void renderingDmap_c::beforeDrawPath() {}

f32 renderingDmap_c::getRestartCursorSize() {
    return getPlayerCursorSize();
}

bool renderingDmap_c::isRendAllRoom() const {
    return true;
}

bool renderingDmap_c::isRendDoor() const {
    return true;
}

bool renderingDmap_c::isCheckFloor() const {
    return true;
}

bool renderingDmap_c::isRendRestart() const {
    return true;
}

bool renderingDmap_c::isRendCursor() const {
    return true;
}

bool renderingDmap_c::isRendIcon() const {
    return true;
}
