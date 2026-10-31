/**
 * d_menu_collect.cpp
 * Collection Menu
 */

#include "d/dolzel.h" // IWYU pragma: keep

#include <cstring>
#include <os.h>
#include "JSystem/J2DGraph/J2DAnmLoader.h"
#include "JSystem/J2DGraph/J2DGrafContext.h"
#include "JSystem/J2DGraph/J2DTextBox.h"
#include "JSystem/J3DGraphBase/J3DMaterial.h"
#include "JSystem/J3DGraphLoader/J3DAnmLoader.h"
#include "JSystem/J3DGraphLoader/J3DModelLoader.h"
#include "JSystem/JKernel/JKRExpHeap.h"
#include "JSystem/JKernel/JKRSolidHeap.h"
#include "d/actor/d_a_alink.h"
#include "d/d_item.h"
#include "d/d_lib.h"
#include "d/d_menu_collect.h"
#include "d/d_menu_fishing.h"
#include "d/d_menu_insect.h"
#include "d/d_menu_letter.h"
#include "d/d_menu_option.h"
#include "d/d_menu_save.h"
#include "d/d_menu_skill.h"
#if TARGET_PC
#include "d/d_menu_stamp.h"
#endif
#include "d/d_menu_window.h"
#include "d/d_meter2_info.h"
#include "d/d_meter_HIO.h"
#include "d/d_msg_class.h"
#include "d/d_msg_object.h"
#include "d/d_msg_string.h"
#include "d/d_pane_class.h"
#include "d/d_select_cursor.h"
#include "dusk/version.hpp"
#include "m_Do/m_Do_graphic.h"
#include "m_Do/m_Do_mtx.h"

#if TARGET_PC
#include "dusk/game_clock.h"
#include "dusk/interp/user_interface.h"
#include "dusk/menu_pointer.h"
#include "dusk/utilities.hpp"
#include "dusk/settings.h"
#include "dusk/logging.h"

extern JKRArchive* g_tphdCollectRevoArchive;
extern JKRArchive* g_tphdMain2DRevoArchive;

namespace {
// TPHD (Wii U TV / Pro Controller) collection screen: zelda_collect_soubi_screen_revo.blo and
// zelda_collect_soubi_do_icon_parts_revo.blo from /res/LayoutRevo/clctresR.arc. Both are authored on
// a 608-unit canvas that the Wii U stretches to 16:9 (their 38 x 50 item frames come out square), so
// they are drawn scaled to the whole screen width. Pane names match the GC layout except for the
// A / B captions (cont_at / cont_bt) and the heart decoration (no uzu_01..07 / hishi).
bool collectRevo() {
    return dusk::tphd_active() && g_tphdCollectRevoArchive != NULL;
}

f32 collectRevoScaleX() {
    return (mDoGph_gInf_c::getMaxXF() - mDoGph_gInf_c::getMinXF()) / 608.0f;
}

J2DTextBox* collectIconText(J2DScreen* scr, u64 tag) {
    J2DPane* p = scr->search(tag);
    if (p == NULL && collectRevo()) {
        static const u64 kMap[][2] = {
            {MULTI_CHAR('atext1_1'), MULTI_CHAR('cont_at')},  {MULTI_CHAR('atext1_2'), MULTI_CHAR('cont_at1')},
            {MULTI_CHAR('btext1_1'), MULTI_CHAR('cont_bt')},  {MULTI_CHAR('btext1_2'), MULTI_CHAR('cont_bt1')},
        };
        for (const auto& m : kMap) {
            if (m[0] == tag) {
                p = scr->search(m[1]);
            }
        }
    }
    return (p != NULL && p->getTypeID() == 19) ? static_cast<J2DTextBox*>(p) : NULL;
}
}  // namespace
#endif

typedef void (dMenu_Collect2D_c::*initFunc)();
static DUSK_CONSTEXPR initFunc init[] = {
    &dMenu_Collect2D_c::wait_init,          &dMenu_Collect2D_c::save_open_init,
    &dMenu_Collect2D_c::save_move_init,     &dMenu_Collect2D_c::save_close_init,
    &dMenu_Collect2D_c::option_open_init,   &dMenu_Collect2D_c::option_move_init,
    &dMenu_Collect2D_c::option_close_init,  &dMenu_Collect2D_c::letter_open_init,
    &dMenu_Collect2D_c::letter_move_init,   &dMenu_Collect2D_c::letter_close_init,
    &dMenu_Collect2D_c::fishing_open_init,  &dMenu_Collect2D_c::fishing_move_init,
    &dMenu_Collect2D_c::fishing_close_init, &dMenu_Collect2D_c::skill_open_init,
    &dMenu_Collect2D_c::skill_move_init,    &dMenu_Collect2D_c::skill_close_init,
    &dMenu_Collect2D_c::insect_open_init,   &dMenu_Collect2D_c::insect_move_init,
    &dMenu_Collect2D_c::insect_close_init};

typedef void (dMenu_Collect2D_c::*processFunc)();
static DUSK_CONSTEXPR processFunc process[] = {
    &dMenu_Collect2D_c::wait_proc,          &dMenu_Collect2D_c::save_open_proc,
    &dMenu_Collect2D_c::save_move_proc,     &dMenu_Collect2D_c::save_close_proc,
    &dMenu_Collect2D_c::option_open_proc,   &dMenu_Collect2D_c::option_move_proc,
    &dMenu_Collect2D_c::option_close_proc,  &dMenu_Collect2D_c::letter_open_proc,
    &dMenu_Collect2D_c::letter_move_proc,   &dMenu_Collect2D_c::letter_close_proc,
    &dMenu_Collect2D_c::fishing_open_proc,  &dMenu_Collect2D_c::fishing_move_proc,
    &dMenu_Collect2D_c::fishing_close_proc, &dMenu_Collect2D_c::skill_open_proc,
    &dMenu_Collect2D_c::skill_move_proc,    &dMenu_Collect2D_c::skill_close_proc,
    &dMenu_Collect2D_c::insect_open_proc,   &dMenu_Collect2D_c::insect_move_proc,
    &dMenu_Collect2D_c::insect_close_proc,
};

dMenu_Collect2D_c::dMenu_Collect2D_c(JKRExpHeap* param_0, STControl* param_1, CSTControl* param_2) {
    mpHeap = param_0;
    mpStick = param_1;
    mpCStick = param_2;
    field_0xc = NULL;
    mIsWolf = daPy_py_c::checkNowWolf();
    mLinkGlobalCenterPos.set(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < 2; i++) {
        field_0x44[i] = 1.0f;
        field_0x4c[i] = 1.0f;
    }
    mBlueSmokePosX = 0.0f;
    mBlueSmokePosY = 0.0f;
    mBlueSmokeScale = 1.0f;
    mBlueSmokeAlpha = 1.0f;
    mHeartVesselPosX = 0.0f;
    mHeartVesselPosY = 0.0f;
    mHeartVesselScale = 1.0f;
    mHeartPiecePosX = 0.0f;
    mHeartPiecePosY = 0.0f;
    mHeartPieceScale = 1.0f;
#if TARGET_PC
    // The HD (Revo) Options/Save layouts load far more texture data than the GC ones; the 0x46000
    // heap overflowed ("allocation failure") when opening Options from the collection menu.
    mpSubHeap = NULL;
    if (dusk::tphd_active() && mpHeap != NULL) {
        u32 hdSize = mpHeap->getTotalFreeSize() / 3;
        if (hdSize > 0x00300000) {
            hdSize = 0x00300000;
        }
        if (hdSize > 0x00046000) {
            mpSubHeap = JKRCreateExpHeap(hdSize, mpHeap, 0);
        }
    }
    if (mpSubHeap == NULL) {
        mpSubHeap = JKRCreateExpHeap(0x00046000, mpHeap, 0);
    }
#else
    mpSubHeap = JKRCreateExpHeap(0x00046000, mpHeap, 0);
#endif
}


dMenu_Collect2D_c::~dMenu_Collect2D_c() {
    if (mpSubHeap) {
        mDoExt_destroyExpHeap(mpSubHeap);
        mpSubHeap = NULL;
    }
}

#if TARGET_PC
static dusk::utils::PaneCache mpScreenPanes[] = {
    {MULTI_CHAR('sa_tex_n'), 0.0f, 0.0f, false},
    {MULTI_CHAR('op_tex_n'), 0.0f, 0.0f, false},
    {MULTI_CHAR('heart_n'), 0.0f, 0.0f, false},
    {MULTI_CHAR('wolf_n'), 0.0f, 0.0f, false},
    {MULTI_CHAR('item_0_n'), 0.0f, 0.0f, false},
    {MULTI_CHAR('item_1_n'), 0.0f, 0.0f, false},
    {MULTI_CHAR('item_2_n'), 0.0f, 0.0f, false},
    {MULTI_CHAR('fish_3_n'), 0.0f, 0.0f, false},
    {MULTI_CHAR('lett_4_n'), 0.0f, 0.0f, false},
    {MULTI_CHAR('maki_5_n'), 0.0f, 0.0f, false},
    {MULTI_CHAR('fuku_n0'), 0.0f, 0.0f, false},
    {MULTI_CHAR('fuku_n1'), 0.0f, 0.0f, false},
    {MULTI_CHAR('fuku_n2'), 0.0f, 0.0f, false},
    {MULTI_CHAR('tate_n0'), 0.0f, 0.0f, false},
    {MULTI_CHAR('tate_n1'), 0.0f, 0.0f, false},
    {MULTI_CHAR('ken_n0'), 0.0f, 0.0f, false},
    {MULTI_CHAR('ken_n1'), 0.0f, 0.0f, false},
    {MULTI_CHAR('kabu_6n'), 0.0f, 0.0f, false},
    {MULTI_CHAR('t_t00'), 0.0f, 0.0f, false},
    {MULTI_CHAR('f_t00'), 0.0f, 0.0f, false},
    {MULTI_CHAR('itemn_n'), 0.0f, 0.0f, false},
    {MULTI_CHAR('infotxtn'), 0.0f, 0.0f, false},
    {MULTI_CHAR('sa_op_n'), 0.0f, 0.0f, false},
    {MULTI_CHAR('title_n'), 0.0f, 0.0f, false},
    {MULTI_CHAR('menu_n'), 0.0f, 0.0f, false},
    {MULTI_CHAR('w_er_n'), 0.0f, 0.0f, false},
    {MULTI_CHAR('center_n'), 0.0f, 0.0f, false},
    {MULTI_CHAR('info_n'), 0.0f, 0.0f, false},
    {MULTI_CHAR('lavel_n'), 0.0f, 0.0f, false},
    {MULTI_CHAR('modelbgn'), 0.0f, 0.0f, false},
};

void dMenu_Collect2D_c::menuCollectWide() {
    if (collectRevo()) {
        return;  // stretched to the screen width in _draw instead
    }
    static bool cachedPanes = false;
    // Get pre-scale values for each pane
    if (!cachedPanes) {
        for (dusk::utils::PaneCache& entry : mpScreenPanes) {
            J2DPane* pane = mpScreen->search(entry.tag);
            if (!entry.cached) {
                entry.origTransX = pane->getTranslateX();
                entry.origTransY = pane->getTranslateY();
                entry.cached = true;
            }
        }
        cachedPanes = true;
    }

    // Reset all panes
    mpScreen->scale(1.0f, 1.0f);
    mpScreen->translate(0.0f, 0.0f);
    for (dusk::utils::PaneCache& entry : mpScreenPanes) {
        J2DPane* pane = mpScreen->search(entry.tag);
        pane->scale(1.0f, 1.0f);
        pane->translate(entry.origTransX, entry.origTransY);
    }

    // Reset button overlay
    mpScreenIcon->translate(0.0f, 0.0f);

    switch (dusk::getSettings().game.menuScalingMode) {
    case dusk::MenuScaling::GameCube:
        // Selection Cursor
        if (mpDrawCursor) {
            mpDrawCursor->refreshAspectScale(1.0f);
        }
        break;
    case dusk::MenuScaling::Wii:
        // Main Canvas
        mpScreen->scale(mDoGph_gInf_c::hudAspectScaleUp, 1.0f);
        mpScreen->translate(mDoGph_gInf_c::getSafeMinXF(), 0.0f);

        // Button Overlay
        mpScreenIcon->translate(-mDoGph_gInf_c::getSafeMinXF(), 0.0f);

        // "Save" Text
        mpScreen->search(MULTI_CHAR('sa_tex_n'))->scale(mDoGph_gInf_c::hudAspectScaleDown, 1.0f);

        // "Options" Text
        mpScreen->search(MULTI_CHAR('op_tex_n'))->scale(mDoGph_gInf_c::hudAspectScaleDown, 1.0f);

        // Pieces of Heart
        mpScreen->search(MULTI_CHAR('heart_n'))->scale(mDoGph_gInf_c::hudAspectScaleDown, 1.0f);

        // Scents
        mpScreen->search(MULTI_CHAR('wolf_n'))->scale(mDoGph_gInf_c::hudAspectScaleDown, 1.0f);

        // Quiver
        mpScreen->search(MULTI_CHAR('item_0_n'))->scale(mDoGph_gInf_c::hudAspectScaleDown, 1.0f);

        // Wallet
        mpScreen->search(MULTI_CHAR('item_1_n'))->scale(mDoGph_gInf_c::hudAspectScaleDown, 1.0f);

        // Poes
        mpScreen->search(MULTI_CHAR('item_2_n'))->scale(mDoGph_gInf_c::hudAspectScaleDown, 1.0f);

        // Fish Bestiary
        mpScreen->search(MULTI_CHAR('fish_3_n'))->scale(mDoGph_gInf_c::hudAspectScaleDown, 1.0f);

        // Letters
        mpScreen->search(MULTI_CHAR('lett_4_n'))->scale(mDoGph_gInf_c::hudAspectScaleDown, 1.0f);

        // Hidden Skills
        mpScreen->search(MULTI_CHAR('maki_5_n'))->scale(mDoGph_gInf_c::hudAspectScaleDown, 1.0f);

        // Green Tunic
        mpScreen->search(MULTI_CHAR('fuku_n0'))->scale(mDoGph_gInf_c::hudAspectScaleDown, 1.0f);

        // Zora Armor
        mpScreen->search(MULTI_CHAR('fuku_n1'))->scale(mDoGph_gInf_c::hudAspectScaleDown, 1.0f);

        // Magic Armor
        mpScreen->search(MULTI_CHAR('fuku_n2'))->scale(mDoGph_gInf_c::hudAspectScaleDown, 1.0f);

        // Ordon Shield
        mpScreen->search(MULTI_CHAR('tate_n0'))->scale(mDoGph_gInf_c::hudAspectScaleDown, 1.0f);

        // Hylian Shield
        mpScreen->search(MULTI_CHAR('tate_n1'))->scale(mDoGph_gInf_c::hudAspectScaleDown, 1.0f);

        // Ordon Sword
        mpScreen->search(MULTI_CHAR('ken_n0'))->scale(mDoGph_gInf_c::hudAspectScaleDown, 1.0f);

        // Master Sword
        mpScreen->search(MULTI_CHAR('ken_n1'))->scale(mDoGph_gInf_c::hudAspectScaleDown, 1.0f);

        // Bugs
        mpScreen->search(MULTI_CHAR('kabu_6n'))->scale(mDoGph_gInf_c::hudAspectScaleDown, 1.0f);

        // "Collection" Text
        mpScreen->search(MULTI_CHAR('t_t00'))->scale(mDoGph_gInf_c::hudAspectScaleDown, 1.0f);
        mpScreen->search(MULTI_CHAR('f_t00'))->scale(mDoGph_gInf_c::hudAspectScaleDown, 1.0f);

        // Item Name Text
        mpScreen->search(MULTI_CHAR('itemn_n'))->scale(mDoGph_gInf_c::hudAspectScaleDown, 1.0f);

        // Item Description Text
        mpScreen->search(MULTI_CHAR('infotxtn'))->scale(mDoGph_gInf_c::hudAspectScaleDown, 1.0f);

        // Selection Cursor
        if (mpDrawCursor) {
            mpDrawCursor->refreshAspectScale(mDoGph_gInf_c::hudAspectScaleUp);
        }
        break;
    case dusk::MenuScaling::Dusklight:
        // Main Canvas
        mpScreen->scale(mDoGph_gInf_c::hudAspectScaleUp, 1.0f);
        mpScreen->translate(mDoGph_gInf_c::getSafeMinXF(), 0.0f);

        // Save/Options Buttons
        mpScreen->search(MULTI_CHAR('sa_op_n'))->scale(mDoGph_gInf_c::hudAspectScaleDown, 1.0f);

        // "Collection" Title Bar
        mpScreen->search(MULTI_CHAR('title_n'))->scale(mDoGph_gInf_c::hudAspectScaleDown, 1.0f);

        // Main Central Elements
        mpScreen->search(MULTI_CHAR('menu_n'))->scale(mDoGph_gInf_c::hudAspectScaleDown, 1.0f);
        mpScreen->search(MULTI_CHAR('w_er_n'))->scale(mDoGph_gInf_c::hudAspectScaleDown, 1.0f);
        mpScreen->search(MULTI_CHAR('center_n'))->scale(mDoGph_gInf_c::hudAspectScaleDown, 1.0f);

        const f32 leftShift = 48.0f * (mDoGph_gInf_c::hudAspectScaleUp - 1.0f);  // Shifting certain items left to keep center (> 4:3 only)

        // Item Name/Description Text
        J2DPane* info_n = mpScreen->search(MULTI_CHAR('info_n'));
        static f32 infoTransX_orig = info_n->getTranslateX();
        info_n->translate(infoTransX_orig - leftShift, info_n->getTranslateY());

        // Designs
        J2DPane* lavel_n = mpScreen->search(MULTI_CHAR('lavel_n'));
        static f32 lavelTransX_orig = lavel_n->getTranslateX();
        lavel_n->translate(lavelTransX_orig - leftShift, lavel_n->getTranslateY());

        // Fused Shadow/Mirror Background
        J2DPane* modelbgn = mpScreen->search(MULTI_CHAR('modelbgn'));
        static f32 modelbgnTransX_orig = modelbgn->getTranslateX();
        modelbgn->setBasePosition(J2DBasePosition_0);
        modelbgn->scale(mDoGph_gInf_c::hudAspectScaleDown, 1.3f);
        f32 modelbgn_scaleFactor = 1.0f + 0.16f * (mDoGph_gInf_c::hudAspectScaleDown - 1.0f);
        modelbgn->translate((modelbgnTransX_orig - 12.0f) * modelbgn_scaleFactor, modelbgn->getTranslateY());

        // Selection Cursor
        if (mpDrawCursor) {
            mpDrawCursor->refreshAspectScale(1.0f);
        }
        break;
    }
}
#endif

void dMenu_Collect2D_c::_create() {
    mpHeap->getTotalFreeSize();

    #if TARGET_PC
    mpDrawCursor = NULL;
    #endif

    mpScreen = JKR_NEW J2DScreen();
#if TARGET_PC
    const bool revo = collectRevo();
    JKRArchive* scrArc = revo ? g_tphdCollectRevoArchive : dComIfGp_getCollectResArchive();
    mpScreen->setPriority(revo ? "zelda_collect_soubi_screen_revo.blo" : "zelda_collect_soubi_screen.blo",
                          0x1020000, scrArc);
#else
    mpScreen->setPriority("zelda_collect_soubi_screen.blo", 0x1020000,
                          dComIfGp_getCollectResArchive());
#endif
    dPaneClass_showNullPane(mpScreen);
    mpScreenIcon = JKR_NEW J2DScreen();
#if TARGET_PC
    mpScreenIcon->setPriority(revo ? "zelda_collect_soubi_do_icon_parts_revo.blo"
                                   : "zelda_collect_soubi_do_icon_parts.blo",
                              0x20000, scrArc);
#else
    mpScreenIcon->setPriority("zelda_collect_soubi_do_icon_parts.blo", 0x20000,
                              dComIfGp_getCollectResArchive());
#endif
    for (int i = 0; i < 2; i++) {
        mpButtonAB[i] = NULL;
        mpButtonText[i] = NULL;
    }

    #if TARGET_PC
    if (!revo) {
        mpScreenIcon->translate(-mDoGph_gInf_c::getSafeMinXF(), 0.0f);
    }
    #endif

    dPaneClass_showNullPane(mpScreenIcon);

    #if TARGET_PC
    menuCollectWide();
    #endif

    mpDraw2DTop = JKR_NEW dMenu_Collect2DTop_c(this);
    ResTIMG* image = (ResTIMG*)dComIfGp_getMain2DArchive()->getResource('TIMG', "tt_block8x8.bti");
    mpBlackTex = JKR_NEW J2DPicture(image);
    mpBlackTex->setBlackWhite(JUtility::TColor(0, 0, 0, 0), JUtility::TColor(0, 0, 0, 255));
    mpBlackTex->setAlpha(0);
#if TARGET_PC
    // Wii U: the 4-corner cursor of main2DR.arc (HD textures).
    mpDrawCursor = JKR_NEW dSelect_cursor_c(2, 1.0f, revo ? g_tphdMain2DRevoArchive : NULL);
#else
    mpDrawCursor = JKR_NEW dSelect_cursor_c(2, 1.0f, NULL);
#endif
    mpString = JKR_NEW dMsgString_c();
    mpSaveScrn = NULL;
    mpOptionScrn = NULL;
    mpLetterScrn = NULL;
    mpFishingScrn = NULL;
    mpSkillScrn = NULL;
    mpInsectScrn = NULL;
    mProcess = 0;
    animationSet();
    screenSet();
    initialize();
    setHIO(true);
#if TARGET_PC
    if (revo) {
        hdInit();
    }
#endif
}

void dMenu_Collect2D_c::_delete() {
    mpHeap->getTotalFreeSize();
    dMeter2Info_setCollectCursorPosXY(mCursorX, mCursorY);
    JKR_DELETE(mpScreen);
    mpScreen = NULL;
    JKR_DELETE(mpScreenIcon);
    mpScreenIcon = NULL;
    for (int i = 0; i < 2; i++) {
        if (mpButtonAB[i]) {
            JKR_DELETE(mpButtonAB[i]);
            mpButtonAB[i] = NULL;
        }
        if (mpButtonText[i]) {
            JKR_DELETE(mpButtonText[i]);
            mpButtonText[i] = NULL;
        }
    }
    JKR_DELETE(mpDraw2DTop);
    mpDraw2DTop = NULL;
    JKR_DELETE(mpBlackTex);
    mpBlackTex = NULL;
    JKR_DELETE(mpDrawCursor);
    mpDrawCursor = NULL;
    JKR_DELETE(mpString);
    mpString = NULL;
    JKRHeap* heap = mDoExt_setCurrentHeap(mpSubHeap);
    if (mpSaveScrn) {
        mpSaveScrn->_delete();
        JKR_DELETE(mpSaveScrn);
        mpSaveScrn = NULL;
        mpSubHeap->freeAll();
    }
    if (mpOptionScrn) {
        mpOptionScrn->_delete();
        JKR_DELETE(mpOptionScrn);
        mpOptionScrn = NULL;
        mpSubHeap->freeAll();
    }
    if (mpLetterScrn) {
        JKR_DELETE(mpLetterScrn);
        mpLetterScrn = NULL;
        mpSubHeap->freeAll();
    }
    if (mpFishingScrn) {
        JKR_DELETE(mpFishingScrn);
        mpFishingScrn = NULL;
        mpSubHeap->freeAll();
    }
    if (mpSkillScrn) {
        JKR_DELETE(mpSkillScrn);
        mpSkillScrn = NULL;
        mpSubHeap->freeAll();
    }
    if (mpInsectScrn) {
        JKR_DELETE(mpInsectScrn);
        mpInsectScrn = NULL;
        mpSubHeap->freeAll();
    }
    mDoExt_setCurrentHeap(heap);
    JKR_DELETE(mpAnmKey);
    mpAnmKey = NULL;
    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 6; j++) {
            if (mpSelPm[i][j]) {
                JKR_DELETE(mpSelPm[i][j]);
                mpSelPm[i][j] = NULL;
            }
        }
    }
    JKR_DELETE(mpLinkPm);
    mpLinkPm = NULL;
    JKR_DELETE(mpMaskPm);
    mpMaskPm = NULL;
    JKR_DELETE(mpModelBg);
    mpModelBg = NULL;
    JKR_DELETE(mpHeartParent);
    mpHeartParent = NULL;
    JKR_DELETE(mpHeartPiece);
    mpHeartPiece = NULL;
    dComIfGp_getCollectResArchive()->removeResourceAll();
}

void dMenu_Collect2D_c::initialize() {
    (this->*init[mProcess])();
}

bool dMenu_Collect2D_c::isFishIconVisible() {
    if (dComIfGs_getFishNum(0) || dComIfGs_getFishNum(1) || dComIfGs_getFishNum(2) ||
        dComIfGs_getFishNum(3) || dComIfGs_getFishNum(4) || dComIfGs_getFishNum(5) ||
        g_drawHIO.mFishListScreen.mDebug)
    {
        return true;
    }
    return false;
}

bool dMenu_Collect2D_c::isSkillIconVisible() {
        /* dSv_event_flag_c::F_0338 - Secret techniques - Obtained 1 secret techinques - Shield attack */
    if (dComIfGs_isEventBit(dSv_event_flag_c::saveBitLabels[338]) ||
        /* dSv_event_flag_c::F_0339 - Secret techniques - Obtained 2 secret techinques */
        dComIfGs_isEventBit(dSv_event_flag_c::saveBitLabels[339]) ||
        /* dSv_event_flag_c::F_0340 - Secret techniques - Obtained 3 secret techinques */
        dComIfGs_isEventBit(dSv_event_flag_c::saveBitLabels[340]) ||
        /* dSv_event_flag_c::F_0341 - Secret techniques - Obtained 4 secret techinques */
        dComIfGs_isEventBit(dSv_event_flag_c::saveBitLabels[341]) ||
        /* dSv_event_flag_c::F_0342 - Secret techniques - Obtained 5 secret techinques */
        dComIfGs_isEventBit(dSv_event_flag_c::saveBitLabels[342]) ||
        /* dSv_event_flag_c::F_0343 - Secret techniques - Obtained 6 secret techinques */
        dComIfGs_isEventBit(dSv_event_flag_c::saveBitLabels[343]) ||
        /* dSv_event_flag_c::F_0344 - Secret techniques - Obtained 7 secret techinques */
        dComIfGs_isEventBit(dSv_event_flag_c::saveBitLabels[344]))
    {
        return true;
    }
    return false;
}

bool dMenu_Collect2D_c::isInsectIconVisible() {
    return dMenu_Insect_c::getGetInsectNum();
}

static u8 const lit_3778[12] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

void dMenu_Collect2D_c::screenSet() {
    static const u64 text_sv[3] = {MULTI_CHAR('sav_0'), MULTI_CHAR('sav_1'), MULTI_CHAR('sav_2')};
    static const u64 text_op[3] = {MULTI_CHAR('opt_0'), MULTI_CHAR('opt_1'), MULTI_CHAR('opt_2'),};
    static const u64 ftext_sv[3] = {MULTI_CHAR('f_sav_0'), MULTI_CHAR('f_sav_1'), MULTI_CHAR('f_sav_2')};
    static const u64 ftext_op[3] = {MULTI_CHAR('f_opt_0'), MULTI_CHAR('f_opt_1'), MULTI_CHAR('f_opt_2')};
    static const u64 text_a_tag[5] = {MULTI_CHAR('atext1_1'), MULTI_CHAR('atext1_2'), MULTI_CHAR('atext1_3'), MULTI_CHAR('atext1_4'), MULTI_CHAR('atext1_5')};
    static const u64 text_b_tag[5] = {MULTI_CHAR('btext1_1'), MULTI_CHAR('btext1_2'), MULTI_CHAR('btext1_3'), MULTI_CHAR('btext1_4'), MULTI_CHAR('btext1_5')};

#if TARGET_PC
    if (dusk::version::isRegionJpn()) {
        static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('t_t00')))->setFont(mDoExt_getRubyFont());
        static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('t_t00')))->setString(0x20, "");
        dMeter2Info_getStringKanji(
            0x3E1, static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('t_t00')))->getStringPtr(), NULL);
        mpScreen->search(MULTI_CHAR('f_t00'))->hide();
    } else {
        static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('f_t00')))->setFont(mDoExt_getRubyFont());
        static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('f_t00')))->setString(0x20, "");
        dMeter2Info_getStringKanji(
            0x3E1, static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('f_t00')))->getStringPtr(), NULL);
        mpScreen->search(MULTI_CHAR('t_t00'))->hide();
    }
#elif REGION_JPN
    static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('t_t00')))->setFont(mDoExt_getRubyFont());
    static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('t_t00')))->setString(0x20, "");
    dMeter2Info_getStringKanji(
        0x3E1, static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('t_t00')))->getStringPtr(), NULL);
    mpScreen->search(MULTI_CHAR('f_t00'))->hide();
#else
    static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('f_t00')))->setFont(mDoExt_getRubyFont());
    static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('f_t00')))->setString(0x20, "");
    dMeter2Info_getStringKanji(
        0x3E1, static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('f_t00')))->getStringPtr(), NULL);
    mpScreen->search(MULTI_CHAR('t_t00'))->hide();
#endif

    for (int i = 0; i < 3; i++) {
#if TARGET_PC
        if (dusk::version::isRegionJpn()) {
            static_cast<J2DTextBox*>(mpScreen->search(text_sv[i]))->setFont(mDoExt_getMesgFont());
            static_cast<J2DTextBox*>(mpScreen->search(text_op[i]))->setFont(mDoExt_getMesgFont());
            static_cast<J2DTextBox*>(mpScreen->search(text_sv[i]))->setString(0x20, "");
            static_cast<J2DTextBox*>(mpScreen->search(text_op[i]))->setString(0x20, "");
            dMeter2Info_getStringKanji(
                0x60, static_cast<J2DTextBox*>(mpScreen->search(text_sv[i]))->getStringPtr(), NULL);
            dMeter2Info_getStringKanji(
                0x5F, static_cast<J2DTextBox*>(mpScreen->search(text_op[i]))->getStringPtr(), NULL);
            mpScreen->search(ftext_sv[i])->hide();
            mpScreen->search(ftext_op[i])->hide();
        } else {
            static_cast<J2DTextBox*>(mpScreen->search(ftext_sv[i]))->setFont(mDoExt_getMesgFont());
            static_cast<J2DTextBox*>(mpScreen->search(ftext_op[i]))->setFont(mDoExt_getMesgFont());
            static_cast<J2DTextBox*>(mpScreen->search(ftext_sv[i]))->setString(0x20, "");
            static_cast<J2DTextBox*>(mpScreen->search(ftext_op[i]))->setString(0x20, "");
            dMeter2Info_getStringKanji(
                0x60, static_cast<J2DTextBox*>(mpScreen->search(ftext_sv[i]))->getStringPtr(), NULL);
            dMeter2Info_getStringKanji(
                0x5F, static_cast<J2DTextBox*>(mpScreen->search(ftext_op[i]))->getStringPtr(), NULL);
            mpScreen->search(text_sv[i])->hide();
            mpScreen->search(text_op[i])->hide();
        }
#elif REGION_JPN
        static_cast<J2DTextBox*>(mpScreen->search(text_sv[i]))->setFont(mDoExt_getMesgFont());
        static_cast<J2DTextBox*>(mpScreen->search(text_op[i]))->setFont(mDoExt_getMesgFont());
        static_cast<J2DTextBox*>(mpScreen->search(text_sv[i]))->setString(0x20, "");
        static_cast<J2DTextBox*>(mpScreen->search(text_op[i]))->setString(0x20, "");
        dMeter2Info_getStringKanji(
            0x60, static_cast<J2DTextBox*>(mpScreen->search(text_sv[i]))->getStringPtr(), NULL);
        dMeter2Info_getStringKanji(
            0x5F, static_cast<J2DTextBox*>(mpScreen->search(text_op[i]))->getStringPtr(), NULL);
        mpScreen->search(ftext_sv[i])->hide();
        mpScreen->search(ftext_op[i])->hide();
#else
        static_cast<J2DTextBox*>(mpScreen->search(ftext_sv[i]))->setFont(mDoExt_getMesgFont());
        static_cast<J2DTextBox*>(mpScreen->search(ftext_op[i]))->setFont(mDoExt_getMesgFont());
        static_cast<J2DTextBox*>(mpScreen->search(ftext_sv[i]))->setString(0x20, "");
        static_cast<J2DTextBox*>(mpScreen->search(ftext_op[i]))->setString(0x20, "");
        dMeter2Info_getStringKanji(
            0x60, static_cast<J2DTextBox*>(mpScreen->search(ftext_sv[i]))->getStringPtr(), NULL);
        dMeter2Info_getStringKanji(
            0x5F, static_cast<J2DTextBox*>(mpScreen->search(ftext_op[i]))->getStringPtr(), NULL);
        mpScreen->search(text_sv[i])->hide();
        mpScreen->search(text_op[i])->hide();
#endif
    }

    for (int i = 0; i < 5; i++) {
#if TARGET_PC
        for (J2DTextBox* tb : {collectIconText(mpScreenIcon, text_a_tag[i]),
                               collectIconText(mpScreenIcon, text_b_tag[i])}) {
            if (tb != NULL) {
                tb->setFont(mDoExt_getMesgFont());
                tb->setString(0x20, "");
            }
        }
#else
        static_cast<J2DTextBox*>(mpScreenIcon->search(text_a_tag[i]))
            ->setFont(mDoExt_getMesgFont());
        static_cast<J2DTextBox*>(mpScreenIcon->search(text_b_tag[i]))
            ->setFont(mDoExt_getMesgFont());
        static_cast<J2DTextBox*>(mpScreenIcon->search(text_a_tag[i]))->setString(0x20, "");
        static_cast<J2DTextBox*>(mpScreenIcon->search(text_b_tag[i]))->setString(0x20, "");
#endif
    }

#if TARGET_PC
    if (dusk::version::isRegionJpn()) {
        static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n00')))->setFont(mDoExt_getMesgFont());
        static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n01')))->setFont(mDoExt_getMesgFont());
        static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n02')))->setFont(mDoExt_getMesgFont());
        static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n03')))->setFont(mDoExt_getMesgFont());
        static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n00')))->setString(0x20, "");
        static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n01')))->setString(0x20, "");
        static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n02')))->setString(0x20, "");
        static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n03')))->setString(0x20, "");
        static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('i_text1')))->setFont(mDoExt_getMesgFont());
        static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('i_text0')))->setFont(mDoExt_getMesgFont());
        static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('i_text1')))->setString(0x100, "");
        static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('i_text0')))->setString(0x100, "");

        mpScreen->search(MULTI_CHAR('item_n04'))->hide();
        mpScreen->search(MULTI_CHAR('item_n05'))->hide();
        mpScreen->search(MULTI_CHAR('item_n06'))->hide();
        mpScreen->search(MULTI_CHAR('item_n07'))->hide();
        mpScreen->search(MULTI_CHAR('f_text1'))->hide();
        mpScreen->search(MULTI_CHAR('f_text0'))->hide();
    } else {
        static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n04')))->setFont(mDoExt_getMesgFont());
        static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n05')))->setFont(mDoExt_getMesgFont());
        static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n06')))->setFont(mDoExt_getMesgFont());
        static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n07')))->setFont(mDoExt_getMesgFont());
        static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n04')))->setString(0x20, "");
        static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n05')))->setString(0x20, "");
        static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n06')))->setString(0x20, "");
        static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n07')))->setString(0x20, "");
        static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('f_text1')))->setFont(mDoExt_getMesgFont());
        static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('f_text0')))->setFont(mDoExt_getMesgFont());
        static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('f_text1')))->setString(0x100, "");
        static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('f_text0')))->setString(0x100, "");

        mpScreen->search(MULTI_CHAR('item_n00'))->hide();
        mpScreen->search(MULTI_CHAR('item_n01'))->hide();
        mpScreen->search(MULTI_CHAR('item_n02'))->hide();
        mpScreen->search(MULTI_CHAR('item_n03'))->hide();
        mpScreen->search(MULTI_CHAR('i_text1'))->hide();
        mpScreen->search(MULTI_CHAR('i_text0'))->hide();
    }
#elif REGION_JPN
    static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n00')))->setFont(mDoExt_getMesgFont());
    static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n01')))->setFont(mDoExt_getMesgFont());
    static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n02')))->setFont(mDoExt_getMesgFont());
    static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n03')))->setFont(mDoExt_getMesgFont());
    static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n00')))->setString(0x20, "");
    static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n01')))->setString(0x20, "");
    static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n02')))->setString(0x20, "");
    static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n03')))->setString(0x20, "");
    static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('i_text1')))->setFont(mDoExt_getMesgFont());
    static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('i_text0')))->setFont(mDoExt_getMesgFont());
    static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('i_text1')))->setString(0x100, "");
    static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('i_text0')))->setString(0x100, "");

    mpScreen->search(MULTI_CHAR('item_n04'))->hide();
    mpScreen->search(MULTI_CHAR('item_n05'))->hide();
    mpScreen->search(MULTI_CHAR('item_n06'))->hide();
    mpScreen->search(MULTI_CHAR('item_n07'))->hide();
    mpScreen->search(MULTI_CHAR('f_text1'))->hide();
    mpScreen->search(MULTI_CHAR('f_text0'))->hide();
#else
    static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n04')))->setFont(mDoExt_getMesgFont());
    static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n05')))->setFont(mDoExt_getMesgFont());
    static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n06')))->setFont(mDoExt_getMesgFont());
    static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n07')))->setFont(mDoExt_getMesgFont());
    static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n04')))->setString(0x20, "");
    static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n05')))->setString(0x20, "");
    static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n06')))->setString(0x20, "");
    static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n07')))->setString(0x20, "");
    static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('f_text1')))->setFont(mDoExt_getMesgFont());
    static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('f_text0')))->setFont(mDoExt_getMesgFont());
    static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('f_text1')))->setString(0x100, "");
    static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('f_text0')))->setString(0x100, "");

    mpScreen->search(MULTI_CHAR('item_n00'))->hide();
    mpScreen->search(MULTI_CHAR('item_n01'))->hide();
    mpScreen->search(MULTI_CHAR('item_n02'))->hide();
    mpScreen->search(MULTI_CHAR('item_n03'))->hide();
    mpScreen->search(MULTI_CHAR('i_text1'))->hide();
    mpScreen->search(MULTI_CHAR('i_text0'))->hide();
#endif

    field_0x22d[0][0] = 0;
    field_0x22d[1][0] = 0;
    field_0x22d[2][0] = 0;
    field_0x22d[3][0] = dComIfGs_isItemFirstBit(0x28) ||
                        (dComIfGs_isItemFirstBit(0x3F)
                            /* dSv_event_flag_c::F_0026 - Ordon Village - gave wooden sword to talo on 3rd day */
                        && !dComIfGs_isEventBit(0x302)) ?
                            true :
                            false;

    if (field_0x22d[3][0] != 0) {
        if (dComIfGs_isItemFirstBit(0x28)) {
            mpScreen->search(MULTI_CHAR('ken_00'))->hide();
            mpScreen->search(MULTI_CHAR('ken_01'))->show();
        } else if (dComIfGs_isItemFirstBit(0x3F)
                       /* dSv_event_flag_c::F_0026 - Ordon Village - gave wooden sword to talo on 3rd day */
                   && !dComIfGs_isEventBit(0x302)) {
            mpScreen->search(MULTI_CHAR('ken_00'))->show();
            mpScreen->search(MULTI_CHAR('ken_01'))->hide();
        }
    }

    field_0x22d[4][0] =
        dComIfGs_isItemFirstBit(0x29) || dComIfGs_isItemFirstBit(0x49) ? true : false;
    if (dComIfGs_getMaxLife() > 15) {
        field_0x22d[5][0] = 1;
    } else {
        field_0x22d[5][0] = 0;
    }
    field_0x22d[6][0] = dMenu_Collect3D_c::getMaskMdlVisible();
    field_0x22d[0][1] = 0;
    field_0x22d[1][1] = 0;
    field_0x22d[2][1] = 0;

    field_0x22d[3][1] =
        dComIfGs_isItemFirstBit(0x2B) || dComIfGs_isItemFirstBit(0x2A) ? true : false;
    if (field_0x22d[3][1] != 0) {
        if (dComIfGs_isItemFirstBit(0x2B)) {
            mpScreen->search(MULTI_CHAR('tate_00'))->show();
            mpScreen->search(MULTI_CHAR('tate_01'))->hide();
        } else if (dComIfGs_isItemFirstBit(0x2A)) {
            mpScreen->search(MULTI_CHAR('tate_00'))->hide();
            mpScreen->search(MULTI_CHAR('tate_01'))->show();
        }
    }
    field_0x22d[4][1] = dComIfGs_isItemFirstBit(0x2C);
    field_0x22d[5][1] = 0;
    field_0x22d[6][1] = 0;
    field_0x22d[0][2] = 0;
    field_0x22d[1][2] = 0;
    field_0x22d[2][2] = 0;
    if (dComIfGs_getSelectEquipClothes() == dItemNo_WEAR_CASUAL_e) {
        field_0x22d[3][2] = 0;
        field_0x22d[4][2] = 0;
        field_0x22d[5][2] = 0;
    } else {
        field_0x22d[3][2] = dComIfGs_isItemFirstBit(0x2F);
        field_0x22d[4][2] = dComIfGs_isItemFirstBit(0x31);
        field_0x22d[5][2] = dComIfGs_isItemFirstBit(0x30);
    }
    field_0x22d[6][2] = 0;
    field_0x22d[0][3] = 1;
    if (checkItemGet(dItemNo_BOW_e, 1)) {
        field_0x22d[1][3] = 1;
    } else {
        field_0x22d[1][3] = 0;
    }
    if (isInsectIconVisible()) {
        field_0x22d[2][3] = 1;
    } else {
        field_0x22d[2][3] = 0;
    }
    field_0x22d[3][3] = isSkillIconVisible();
#if TARGET_PC
    // TPHD 0x029b3988 (+0x2cc = 0x029b1990): the stamp cell is "owned" once a stamp was found.
    field_0x22d[4][3] = collectRevo() && dMenu_Stamp_c::getStampNum() != 0 ? 1 : 0;
#else
    field_0x22d[4][3] = 0;
#endif
    field_0x22d[5][3] = 0;
    field_0x22d[6][3] = 0;
    if (dComIfGs_getCollectSmell() != dItemNo_NONE_e) {
        field_0x22d[0][4] = 1;
    } else {
        field_0x22d[0][4] = 0;
    }
    field_0x22d[1][4] = dComIfGs_getPohSpiritNum();
    if (isFishIconVisible()) {
        field_0x22d[2][4] = 1;
    } else {
        field_0x22d[2][4] = 0;
    }
    if (g_drawHIO.mLetterSelectScreen.mLetterNum > 0) {
        field_0x22d[3][4] = 1;
    } else {
        if (dMeter2Info_getRecieveLetterNum()) {
            field_0x22d[3][4] = 1;
        } else {
            field_0x22d[3][4] = 0;
        }
    }
    field_0x22d[4][4] = 0;
    field_0x22d[5][4] = 0;
    field_0x22d[6][4] = 0;
    field_0x22d[0][5] = 1;
    field_0x22d[1][5] = 1;
    field_0x22d[2][5] = 0;
    field_0x22d[3][5] = 0;
    field_0x22d[4][5] = 0;
    field_0x22d[5][5] = 0;
    field_0x22d[6][5] = 0;
    field_0x184[0][0] = 0;
    field_0x184[1][0] = 0;
    field_0x184[2][0] = 0;
    if (dComIfGs_isItemFirstBit(0x28)) {
        field_0x184[3][0] = 0x18d;
    } else {
        field_0x184[3][0] = 0x1a4;
    }
    if (dComIfGs_isItemFirstBit(0x49)) {
        field_0x184[4][0] = 0x1ae;
    } else {
        field_0x184[4][0] = 0x18e;
    }
    field_0x184[5][0] = 0x186;
    if (field_0x22d[6][0] == 2) {
        if (dMenu_Collect3D_c::getMirrorNum() >= 4) {
            field_0x184[6][0] = 0x20c;
        } else {
            if (dMenu_Collect3D_c::getMirrorNum() >= 2) {
                field_0x184[6][0] = 0x20b;
            } else {
                field_0x184[6][0] = 0x20a;
            }
        }
    } else {
        if (dMenu_Collect3D_c::getCrystalNum() <= 1) {
            field_0x184[6][0] = 0x5aa;
        } else {
            field_0x184[6][0] = 0x5ab;
        }
    }
    field_0x184[0][1] = 0;
    field_0x184[1][1] = 0;
    field_0x184[2][1] = 0;
    if (dComIfGs_isItemFirstBit(0x2B)) {
        field_0x184[3][1] = 0x190;
    } else {
        field_0x184[3][1] = 0x18f;
    }
    field_0x184[4][1] = 0x191;
    field_0x184[5][1] = 0x192;
    field_0x184[6][1] = 0;
    field_0x184[0][2] = 0;
    field_0x184[1][2] = 0;
    field_0x184[2][2] = 0;
    field_0x184[3][2] = 0x194;
    field_0x184[4][2] = 0x196;
    field_0x184[5][2] = 0x195;
    field_0x184[6][2] = 0;
#if TARGET_PC // Since we allow changing wallet sizes, do something more robust.
    if (dComIfGs_getWalletSize() == WALLET) {
        field_0x184[0][3] = 0x199;
    } else if (dComIfGs_getWalletSize() == BIG_WALLET) {
        field_0x184[0][3] = 0x19a;
    } if (dusk::tphd_active() && dComIfGs_getWalletSize() == 3) {
        field_0x184[0][3] = 0x19c;
    } else {
        field_0x184[0][3] = 0x19b;
    }
#else
    if (dComIfGs_getRupeeMax() == WALLET_MAX) {
        field_0x184[0][3] = 0x199;
    } else if (dComIfGs_getRupeeMax() == BIG_WALLET_MAX) {
        field_0x184[0][3] = 0x19a;
    } else {
        field_0x184[0][3] = 0x19b;
    }
#endif
    if (dComIfGs_getArrowMax() == QUIVER_MAX) {
        field_0x184[1][3] = 0x1b9;
    } else if (dComIfGs_getArrowMax() == BIG_QUIVER_MAX) {
        field_0x184[1][3] = 0x1ba;
    } else {
        field_0x184[1][3] = 0x1bb;
    }
    field_0x184[2][3] = 0x5b8;
    field_0x184[3][3] = 0x5b0;
    field_0x184[4][3] = DUSK_IF_ELSE(collectRevo() ? 0xaf0 : 0, 0);  // TPHD "Stamps"
    field_0x184[5][3] = 0;
    field_0x184[6][3] = 0;
    field_0x184[0][4] = dMsgObject_getSmellTypeMessageID();
    field_0x184[1][4] = 0x245;
    field_0x184[2][4] = 0x5a1;
    field_0x184[3][4] = 0x4c8;
    field_0x184[4][4] = 0;
    field_0x184[5][4] = 0;
    field_0x184[6][4] = 0;
    field_0x184[0][5] = 0x60;
    field_0x184[1][5] = 0x5f;
    field_0x184[2][5] = 0;
    field_0x184[3][5] = 0;
    field_0x184[4][5] = 0;
    field_0x184[5][5] = 0;
    field_0x184[6][5] = 0;
    field_0x1d8[0][0] = 0;
    field_0x1d8[1][0] = 0;
    field_0x1d8[2][0] = 0;
    field_0x1d8[3][0] = field_0x184[3][0] + 0x100;
    field_0x1d8[4][0] = field_0x184[4][0] + 0x100;
    field_0x1d8[5][0] = field_0x184[5][0] + 0x100;
    if (field_0x22d[6][0] == 2) {
        if (dMenu_Collect3D_c::getMirrorNum() >= 4) {
            field_0x1d8[6][0] = 0x30c;
        } else {
            if (dMenu_Collect3D_c::getMirrorNum() >= 2) {
                field_0x1d8[6][0] = 0x30b;
            } else {
                field_0x1d8[6][0] = 0x30a;
            }
        }
    } else {
        if (dMenu_Collect3D_c::getCrystalNum() <= 1) {
            field_0x1d8[6][0] = 0x5ac;
        } else {
            field_0x1d8[6][0] = 0x5ad;
        }
    }
    field_0x1d8[0][1] = 0;
    field_0x1d8[1][1] = 0;
    field_0x1d8[2][1] = 0;
    field_0x1d8[3][1] = field_0x184[3][1] + 0x100;
    field_0x1d8[4][1] = field_0x184[4][1] + 0x100;
    field_0x1d8[5][1] = field_0x184[5][1] + 0x100;
    field_0x1d8[6][1] = 0;
    field_0x1d8[0][2] = 0;
    field_0x1d8[1][2] = 0;
    field_0x1d8[2][2] = 0;
    field_0x1d8[3][2] = field_0x184[3][2] + 0x100;
    field_0x1d8[4][2] = field_0x184[4][2] + 0x100;
    field_0x1d8[5][2] = field_0x184[5][2] + 0x100;
    field_0x1d8[6][2] = 0;
    field_0x1d8[0][3] = field_0x184[0][3] + 0x100;
    field_0x1d8[1][3] = field_0x184[1][3] + 0x100;
    field_0x1d8[2][3] = 0x5b9;
    field_0x1d8[3][3] = 0x5b1;
    field_0x1d8[4][3] = DUSK_IF_ELSE(collectRevo() ? 0xaf1 : 0, 0);  // TPHD "... %d/%d"
    field_0x1d8[5][3] = 0;
    field_0x1d8[6][3] = 0;
    field_0x1d8[0][4] = dMsgObject_getSmellTypeMessageID() + 0x100;
        /* dSv_event_flag_c::F_0456 - Castle Town - First time meeting with Jovani */
    if (dComIfGs_isEventBit(dSv_event_flag_c::saveBitLabels[456])) {
        field_0x1d8[1][4] = 0x12d8;
    } else {
        field_0x1d8[1][4] = field_0x184[1][4] + 0x100;
    }
    field_0x1d8[2][4] = 0x5a2;
    field_0x1d8[3][4] = 0x4c9;
    field_0x1d8[4][4] = 0;
    field_0x1d8[5][4] = 0;
    field_0x1d8[6][4] = 0;
    field_0x1d8[0][5] = 0x4c5;
    field_0x1d8[1][5] = 0x4c6;
    field_0x1d8[2][5] = 0;
    field_0x1d8[3][5] = 0;
    field_0x1d8[4][5] = 0;
    field_0x1d8[5][5] = 0;
    field_0x1d8[6][5] = 0;
    mCursorX = dMeter2Info_getCollectCursorPosX();
    mCursorY = dMeter2Info_getCollectCursorPosY();
    if (dMenu_Collect3D_c::getMaskMdlVisible() == 0) {
        if (mCursorX == 6 && mCursorY == 0) {
            mCursorX = 3;
            mCursorY = 0;
        }
    }
    field_0x259 = mCursorX;
    field_0x25a = mCursorY;
    mCurrentAString = 0;
    mCurrentBString = 0;
    field_0x180 = 0;
    mItemNameString = 0;
    mEquippedSword = 0xff;
    mEquippedShield = 0xff;
    mEquippedClothes = 0xff;
    mSubWindowOpenCheck = 0;
    field_0x25f = 0;
    field_0x260 = 0xff;
    field_0x261 = 0xff;
    setEquipItemFrameColorSword(-1);
    setEquipItemFrameColorShield(-1);
    setEquipItemFrameColorClothes(-1);
    u8 i_copy;
    u8 j_copy;
    bool bVar18 = false;
    bool bVar2 = false;
    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 6; j++) {
            if (getItemTag(i, j, true)) {
                mpSelPm[i][j] = JKR_NEW CPaneMgr(mpScreen, getItemTag(i, j, true), 0, NULL);
                if (!bVar18) {
                    i_copy = i;
                    j_copy = j;
                    bVar18 = true;
                }
                if (!bVar2) {
                    if (mCursorX == i && mCursorY == j) {
                        bVar2 = true;
                    }
                }
            } else {
                mpSelPm[i][j] = NULL;
            }
            if (getItemTag(i, j, false)) {
                if (field_0x22d[i][j] != 0 || (i == 5 && j == 0)) {
                    mpScreen->search(getItemTag(i, j, false))->show();
                } else {
                    mpScreen->search(getItemTag(i, j, false))->hide();
                }
            }
        }
    }
    if (!bVar2) {
        mCursorX = i_copy;
        mCursorY = j_copy;
    }
    mpLinkPm = JKR_NEW CPaneMgr(mpScreen, MULTI_CHAR('linki_n'), 0, NULL);
    mLinkGlobalCenterPos.x = mpLinkPm->getInitGlobalCenterPosX();
    mLinkGlobalCenterPos.y = mpLinkPm->getInitGlobalCenterPosY();
    mLinkGlobalCenterPos.z = -1000.0f;
    mpMaskPm = JKR_NEW CPaneMgr(mpScreen, MULTI_CHAR('kamen_n'), 0, NULL);
    mpModelBg = JKR_NEW CPaneMgr(mpScreen, MULTI_CHAR('modelbgn'), 2, NULL);
    mpHeartParent = JKR_NEW CPaneMgr(mpScreen, MULTI_CHAR('heart_n'), 0, NULL);
    mpHeartPiece = JKR_NEW CPaneMgr(mpScreen, MULTI_CHAR('heart_kn'), 0, NULL);
    setAButtonString(mCurrentAString);
    setBButtonString(mCurrentBString);
    setItemNameString(mCursorX, mCursorY);
    cursorPosSet();
    setArrowMaxNum(dComIfGs_getArrowMax());
#if TARGET_PC
    setWalletSizeNum(dComIfGs_getWalletSize());
#else
    setWalletMaxNum(dComIfGs_getRupeeMax());
#endif
    setSmellType();
    setHeartPiece();
    setPohMaxNum(dComIfGs_getPohSpiritNum());
#if TARGET_PC
    if (collectRevo()) {
        // TPHD 0x029b3850: number of stamps on the stamp cell (tens hidden below 10).
        const int num = dMenu_Stamp_c::getStampNum();
        JKRArchive* archive = dComIfGp_getMain2DArchive();
        J2DPane* tens = mpScreen->search(MULTI_CHAR('stamp10'));
        J2DPane* ones = mpScreen->search(MULTI_CHAR('stamp01'));
        if (tens != NULL && tens->getTypeID() == 18) {
            if (num > 9) {
                tens->show();
            } else {
                tens->hide();
            }
            ResTIMG* timg =
                (ResTIMG*)archive->getResource('TIMG', dMeter2Info_getNumberTextureName(num / 10));
            if (timg != NULL) {
                static_cast<J2DPicture*>(tens)->changeTexture(timg, 0);
            }
        }
        if (ones != NULL && ones->getTypeID() == 18) {
            ResTIMG* timg =
                (ResTIMG*)archive->getResource('TIMG', dMeter2Info_getNumberTextureName(num % 10));
            if (timg != NULL) {
                static_cast<J2DPicture*>(ones)->changeTexture(timg, 0);
            }
        }
    }
#endif
}

void dMenu_Collect2D_c::animationSet() {
    OSInitFastCast();
#if TARGET_PC
    void* resource =
        collectRevo() ? JKRGetNameResource("zelda_collect_soubi_screen_revo.btk", g_tphdCollectRevoArchive)
                      : JKRGetNameResource("zelda_collect_soubi_screen.btk", dComIfGp_getCollectResArchive());
#else
    void* resource =
        JKRGetNameResource("zelda_collect_soubi_screen.btk", dComIfGp_getCollectResArchive());
#endif
    mpAnmKey = (J2DAnmTextureSRTKey*)J2DAnmLoaderDataBase::load(resource);
    mpAnmKey->searchUpdateMaterialID(mpScreen);
    field_0x34 = 0.0f;
    mFrame = 0.0f;
    field_0x3c = 0.0f;
}

void dMenu_Collect2D_c::btkAnimeLoop0(J2DAnmTextureSRTKey* i_SRTKey) {
    if (i_SRTKey != NULL) {
#if TARGET_PC
        dusk::vdt::advance_looping_frame(mFrame, 1.0f, i_SRTKey->getFrameMax());
#else
        mFrame++;
        if (mFrame >= i_SRTKey->getFrameMax()) {
            mFrame -= i_SRTKey->getFrameMax();
        }
#endif
        i_SRTKey->setFrame(mFrame);
    } else {
        mFrame = 0.0f;
    }
    static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('modelbg0')))->setAnimation(i_SRTKey);
    static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('modelbg1')))->setAnimation(i_SRTKey);
    static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('modelbg2')))->setAnimation(i_SRTKey);
    static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('modelbg3')))->setAnimation(i_SRTKey);
    static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('modelbg4')))->setAnimation(i_SRTKey);
    static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('modelbg5')))->setAnimation(i_SRTKey);
}

void dMenu_Collect2D_c::setBackAlpha() {
#if TARGET_PC
    const f32 target = mProcess >= 1 && mProcess <= 18 ? 1.0f : 0.0f;
    dusk::vdt::advance_toward_frame(mBackAlpha, target, 0.2f);
    mpBlackTex->setAlpha(mBackAlpha * 150.0f);
#else
    f32 alpha = mpBlackTex->getAlpha() / 150.0f;

    switch (mProcess) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
        if (alpha < 1.0f) {
            alpha += 0.2f;
        }
        if (alpha > 1.0f) {
            alpha = 1.0f;
        }
        break;

    default:
        if (alpha > 0.0f) {
            alpha -= 0.2f;
        }
        if (alpha < 0.0f) {
            alpha = 0.0f;
        }
        break;
    }

    mpBlackTex->setAlpha(alpha * 150.0f);
#endif
}

// Not sure if this works without gotos
void dMenu_Collect2D_c::cursorMove() {
    u8 dVar1 = mCursorX;
    u8 dVar2 = mCursorY;
    mpStick->checkTrigger();
    if (mpStick->checkRightTrigger()) {
        if (mCursorX < 6) {
            if (mCursorX == 0 && mCursorY == 0 &&
                field_0x259 >= 3 && field_0x25a < 3)
            {
                mCursorX = field_0x259;
                mCursorY = field_0x25a;
            } else {
                do {
                    mCursorX++;
                    if (mCursorX == 6 && mCursorY < 3) {
                        if (field_0x22d[6][0] != 0) {
                            if (getItemTag(6, 0, false) && field_0x22d[6][0] != 0) {
                                mCursorX = 6;
                                mCursorY = 0;
                            }
                        } else {
                            mCursorX = dVar1;
                            mCursorY = dVar2;
                            goto LAB_802ba744;
                        }
                    }
                    if (dVar1 == 4 && dVar2 == 1) {
                        mCursorX = 5;
                        mCursorY = 0;
                        goto LAB_802ba744;
                    }
                    if (getItemTag(mCursorX, mCursorY, true)) {
                        goto LAB_802ba744;
                    }
                } while (mCursorX < 6);
                mCursorX = dVar1;
            }
        }
    } else {
        if (mpStick->checkLeftTrigger() && mCursorX != 0) {
            if (mCursorX == 6 &&
                mCursorY == 0 && field_0x259 >= 3 &&
                  field_0x25a < 3)
            {
                mCursorX = field_0x259;
                mCursorY = field_0x25a;
            } else if (dVar1 == 5 && dVar2 == 0 &&
                        field_0x259 == 4 &&
                       field_0x25a == 1)
            {
                mCursorX = field_0x259;
                mCursorY = field_0x25a;
            } else {
                do {
                    mCursorX--;
                    if (getItemTag(mCursorX, mCursorY, true)) {
                        goto LAB_802ba744;
                    }
                } while (mCursorX != 0);
                mCursorX = dVar1;
            }
        }
    }
LAB_802ba744:
    if (mpStick->checkUpTrigger()) {
        if (mCursorY != 0) {
        begin:
            mCursorY--;
            if (mCursorY == 2) {
                u8 local_3c[9] = {3,3,4,3,4,5,4,5,5};
                u8 local_48[9] = {2,1,2,0,1,2,0,1,0};
                for (int i = 0; i < 9; i++) {
                    if (getItemTag(local_3c[i], local_48[i], true)) {
                        mCursorX = local_3c[i];
                        mCursorY = local_48[i];
                        break;
                    }
                }
            }
            if (dVar2 == 5) {
                if (dVar1 == 0) {
                    if (field_0x25a < 5) {
                        mCursorX = field_0x259;
                        mCursorY = field_0x25a;
                        goto LAB_802bab54;
                    } else {
                        bool bVar3 = false;
                        for (int i = 0; i < 4; i++) {
                            if (getItemTag(i, mCursorY, true)) {
                                mCursorX = i;
                                bVar3 = true;
                                break;
                            }
                        }
                        if (mCursorY == 0) {
                            mCursorX = dVar1;
                            mCursorY = dVar2;
                            bVar3 = true;
                        }
                        if (bVar3) {
                            goto LAB_802bab54;
                        }
                    }
                } else if (field_0x25a < 5) {
                    mCursorX = field_0x259;
                    mCursorY = field_0x25a;
                    goto LAB_802bab54;
                } else {
                    bool bVar4 = false;
                    for (int i = 3; i < 7; i++) {
                        if (getItemTag(i, mCursorY, true)) {
                            mCursorX = i;
                            bVar4 = true;
                            break;
                        }
                    }
                    if (mCursorY == 0) {
                        mCursorX = dVar1;
                        mCursorY = dVar2;
                        bVar4 = true;
                    }
                    if (bVar4) {
                        goto LAB_802bab54;
                    }
                }
                goto begin;
            } else {
                if (getItemTag(mCursorX, mCursorY, true)) {
                    goto LAB_802bab54;
                }
                if (mCursorY != 0) {
                    goto begin;
                }
                mCursorY = dVar2;
            }
        }
    } else if (mpStick->checkDownTrigger()) {
        if (mCursorY < 4) {
            do {
                mCursorY++;
                if (mCursorY == 3) {
                    u8 local_50[8] = {3, 2, 3, 1, 2, 0, 1, 0};
                    u8 local_58[8] = {3, 3, 4, 3, 4, 3, 4, 4};
                    for (int i = 0; i < 8; i++) {
                        if (getItemTag(local_50[i], local_58[i], true)) {
                            mCursorX = local_50[i];
                            mCursorY = local_58[i];
                            break;
                        }
                    }
                }
                if (getItemTag(mCursorX, mCursorY, true)) {
                    goto LAB_802bab54;
                }
            } while (mCursorY < 4);
            mCursorY = 5;
            if (mCursorX <= 2) {
                mCursorX = 0;
            } else {
                mCursorX = 1;
            }
        } else if (mCursorY == 4) {
            mCursorY = 5;
            if (mCursorX <= 3) {
                mCursorX = 0;
            } else {
                mCursorX = 1;
            }
        }
    }

LAB_802bab54:
    if (mCursorX != dVar1 || mCursorY != dVar2) {
        field_0x259 = dVar1;
        field_0x25a = dVar2;
        if ((mCursorX == 1 && mCursorY == 5) || (mCursorX == 0 && mCursorY == 5)) {
            Z2GetAudioMgr()->seStart(Z2SE_SY_CURSOR_OPTION, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f,
                                     0);
        } else {
            Z2GetAudioMgr()->seStart(Z2SE_SY_CURSOR_ITEM, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
        }
        cursorPosSet();
        setItemNameString(mCursorX, mCursorY);
    }
    if (field_0x44[0] != g_drawHIO.mCollectScreen.mUnselectItemScale ||
        field_0x44[1] != g_drawHIO.mCollectScreen.mSelectItemScale ||
        field_0x4c[0] != g_drawHIO.mCollectScreen.mUnselectSaveOptionScale ||
        field_0x4c[1] != g_drawHIO.mCollectScreen.mSelectSaveOptionScale)
    {
        field_0x44[0] = g_drawHIO.mCollectScreen.mUnselectItemScale;
        field_0x44[1] = g_drawHIO.mCollectScreen.mSelectItemScale;
        field_0x4c[0] = g_drawHIO.mCollectScreen.mUnselectSaveOptionScale;
        field_0x4c[1] = g_drawHIO.mCollectScreen.mSelectSaveOptionScale;
        for (int i = 0; i < 7; i++) {
            for (int j = 0; j < 6; j++) {
                if (getItemTag(i, j, true)) {
                    J2DPane* pane = mpScreen->search(getItemTag(i, j, true));
                    if ((i != 0 || j != 0) && (i != 6 || j != 0)) {
                        if (j == 5) {
                            if (i == mCursorX && j == mCursorY) {
                                pane->scale(g_drawHIO.mCollectScreen.mSelectSaveOptionScale,
                                            g_drawHIO.mCollectScreen.mSelectSaveOptionScale);
                            } else {
                                pane->scale(g_drawHIO.mCollectScreen.mUnselectSaveOptionScale,
                                            g_drawHIO.mCollectScreen.mUnselectSaveOptionScale);
                            }
                        } else if (i == mCursorX && j == mCursorY) {
                            pane->scale(g_drawHIO.mCollectScreen.mSelectItemScale,
                                        g_drawHIO.mCollectScreen.mSelectItemScale);
                        } else {
                            pane->scale(g_drawHIO.mCollectScreen.mUnselectItemScale,
                                        g_drawHIO.mCollectScreen.mUnselectItemScale);
                        }
                    }
                }
            }
        }
    }
}

void dMenu_Collect2D_c::cursorPosSet() {
#if TARGET_PC
    // TPHD collect HIO (ctor 0x02a4e00c, read by 0x029b2748): unselected 1.0, selected 1.1 for every
    // cell (GC: 1.5 for items). Kept local because dMeter_drawHIO_c::updateOnWide resets g_drawHIO.
    const bool hdRevo = collectRevo();
    const f32 kSelItem = hdRevo ? 1.1f : g_drawHIO.mCollectScreen.mSelectItemScale;
    const f32 kUnselItem = hdRevo ? 1.0f : g_drawHIO.mCollectScreen.mUnselectItemScale;
    const f32 kSelSO = hdRevo ? 1.1f : g_drawHIO.mCollectScreen.mSelectSaveOptionScale;
    const f32 kUnselSO = hdRevo ? 1.0f : g_drawHIO.mCollectScreen.mUnselectSaveOptionScale;
#else
    const f32 kSelItem = g_drawHIO.mCollectScreen.mSelectItemScale;
    const f32 kUnselItem = g_drawHIO.mCollectScreen.mUnselectItemScale;
    const f32 kSelSO = g_drawHIO.mCollectScreen.mSelectSaveOptionScale;
    const f32 kUnselSO = g_drawHIO.mCollectScreen.mUnselectSaveOptionScale;
#endif
    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 6; j++) {
            if (getItemTag(i, j, true) != 0) {
                J2DPane* itemTag = mpScreen->search(getItemTag(i, j, true));
#if TARGET_PC
                // TPHD 0x029b2748 leaves the heart and the Fused Shadow / Mirror unscaled.
                if (hdRevo && (getItemTag(i, j, true) == MULTI_CHAR('heart_kn') ||
                               getItemTag(i, j, true) == MULTI_CHAR('kamen_n')))
                {
                    continue;
                }
#endif
                if ((i != 0 || j != 0) && (i != 6 || j != 0)) {
                    if (j == 5) {
                        if (i == mCursorX && j == mCursorY) {
                            itemTag->scale(kSelSO,
                                           kSelSO);
                        } else {
                            itemTag->scale(kUnselSO,
                                           kUnselSO);
                        }
                    } else if (i == mCursorX && j == mCursorY) {
                        itemTag->scale(kSelItem,
                                       kSelItem);
                    } else {
                        itemTag->scale(kUnselItem,
                                       kUnselItem);
                    }
                }
            }
        }
    }

    mpDrawCursor->setAlphaRate(1.0f);
    Vec pos = mpSelPm[mCursorX][mCursorY]->getGlobalVtxCenter(false, 0);
    mpDrawCursor->setPos(pos.x, pos.y, mpSelPm[mCursorX][mCursorY]->getPanePtr(), false);
    if (mCursorY == 5) {
        mpDrawCursor->setParam(1.1f, 0.85f, 0.05f, 0.5f, 0.5f);
    } else if (mCursorX == 6 && mCursorY == 0) {
        mpDrawCursor->setParam(0.6f, 0.85f, 0.03f, 0.6f, 0.6f);
    } else {
        mpDrawCursor->setParam(1.0f, 1.0f, 0.1f, 0.7f, 0.7f);
    }
#if TARGET_PC
    if (collectRevo()) {
        hdSetCursorParam();
    }
#endif
}

void dMenu_Collect2D_c::changeSword() {
    switch (mCursorX) {
    case 3:
        if (dComIfGs_isItemFirstBit(dItemNo_SWORD_e)) {
            if (dComIfGs_getSelectEquipSword() != dItemNo_SWORD_e) {
                dMeter2Info_setSword(dItemNo_SWORD_e, false);
                setEquipItemFrameColorSword(0);
                mDoAud_seStart(Z2SE_SY_ITEM_SET_X, NULL, 0, 0);
                dMeter2Info_set2DVibration();
            }
        } else if (dComIfGs_getSelectEquipSword() != dItemNo_WOOD_STICK_e) {
            dMeter2Info_setSword(dItemNo_WOOD_STICK_e, false);
            setEquipItemFrameColorSword(0);
            Z2GetAudioMgr()->seStart(Z2SE_SY_ITEM_SET_X, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
            dMeter2Info_set2DVibration();
        }
        break;
    case 4:
        if (dComIfGs_isItemFirstBit(dItemNo_LIGHT_SWORD_e)) {
            if (dComIfGs_getSelectEquipSword() != dItemNo_LIGHT_SWORD_e) {
                dMeter2Info_setSword(dItemNo_LIGHT_SWORD_e, false);
                setEquipItemFrameColorSword(1);
                Z2GetAudioMgr()->seStart(Z2SE_SY_ITEM_SET_X, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f,
                                         0);
                dMeter2Info_set2DVibration();
            }
        } else if (dComIfGs_getSelectEquipSword() != dItemNo_MASTER_SWORD_e) {
            dMeter2Info_setSword(dItemNo_MASTER_SWORD_e, false);
            setEquipItemFrameColorSword(1);
            Z2GetAudioMgr()->seStart(Z2SE_SY_ITEM_SET_X, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
            dMeter2Info_set2DVibration();
        }
        break;
    case 5:
        if (dComIfGs_getSelectEquipSword() != dItemNo_LIGHT_SWORD_e) {
            dMeter2Info_setSword(dItemNo_LIGHT_SWORD_e, false);
            setEquipItemFrameColorSword(2);
            Z2GetAudioMgr()->seStart(Z2SE_SY_ITEM_SET_X, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
            dMeter2Info_set2DVibration();
        }
        break;
    }
}

void dMenu_Collect2D_c::changeShield() {
    switch (mCursorX) {
    case 3:
        if (dComIfGs_isItemFirstBit(dItemNo_SHIELD_e)) {
            if (dComIfGs_getSelectEquipShield() != dItemNo_SHIELD_e) {
                dMeter2Info_setShield(dItemNo_SHIELD_e, false);
                setEquipItemFrameColorShield(0);
                daAlink_getAlinkActorClass()->setShieldChange();
                Z2GetAudioMgr()->seStart(Z2SE_SY_ITEM_SET_X, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f,
                                         0);
                dMeter2Info_set2DVibration();
            }
        } else if (dComIfGs_isItemFirstBit(dItemNo_WOOD_SHIELD_e)) {
            if (dComIfGs_getSelectEquipShield() != dItemNo_WOOD_SHIELD_e) {
                dMeter2Info_setShield(dItemNo_WOOD_SHIELD_e, false);
                setEquipItemFrameColorShield(0);
                daAlink_getAlinkActorClass()->setShieldChange();
                Z2GetAudioMgr()->seStart(Z2SE_SY_ITEM_SET_X, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f,
                                         0);
                dMeter2Info_set2DVibration();
            }
        }
        break;
    case 4:
        if (dComIfGs_getSelectEquipShield() != dItemNo_HYLIA_SHIELD_e) {
            dMeter2Info_setShield(dItemNo_HYLIA_SHIELD_e, false);
            setEquipItemFrameColorShield(1);
            daAlink_getAlinkActorClass()->setShieldChange();
            Z2GetAudioMgr()->seStart(Z2SE_SY_ITEM_SET_X, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
            dMeter2Info_set2DVibration();
        }
        break;
    }
}

void dMenu_Collect2D_c::changeClothe() {
    switch (mCursorX) {
    case 3:
        if (dComIfGs_getSelectEquipClothes() != dItemNo_WEAR_KOKIRI_e) {
            dMeter2Info_setCloth(dItemNo_WEAR_KOKIRI_e, false);
            setEquipItemFrameColorClothes(0);
            daPy_getPlayerActorClass()->setClothesChange(0);
            Z2GetAudioMgr()->seStart(Z2SE_SY_ITEM_SET_X, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
            dMeter2Info_set2DVibration();
        }
        break;
    case 4:
        if (dComIfGs_getSelectEquipClothes() != dItemNo_WEAR_ZORA_e) {
            dMeter2Info_setCloth(dItemNo_WEAR_ZORA_e, false);
            setEquipItemFrameColorClothes(1);
            daPy_getPlayerActorClass()->setClothesChange(0);
            Z2GetAudioMgr()->seStart(Z2SE_SY_ITEM_SET_X, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
            dMeter2Info_set2DVibration();
        }
        break;
    case 5:
        if (dComIfGs_getSelectEquipClothes() != dItemNo_ARMOR_e) {
            dMeter2Info_setCloth(dItemNo_ARMOR_e, false);
            setEquipItemFrameColorClothes(2);
            daPy_getPlayerActorClass()->setClothesChange(0);
            Z2GetAudioMgr()->seStart(Z2SE_SY_ITEM_SET_X, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
            dMeter2Info_set2DVibration();
        }
        break;
    }
}

void dMenu_Collect2D_c::setArrowMaxNum(u8 param_0) {
    switch (param_0) {
    case 0:
        mpScreen->search(MULTI_CHAR('item_0_0'))->hide();
        mpScreen->search(MULTI_CHAR('item_0_1'))->hide();
        mpScreen->search(MULTI_CHAR('item_0_2'))->hide();
        break;
    case 30:
        mpScreen->search(MULTI_CHAR('item_0_0'))->show();
        mpScreen->search(MULTI_CHAR('item_0_1'))->hide();
        mpScreen->search(MULTI_CHAR('item_0_2'))->hide();
        break;
    case 60:
        mpScreen->search(MULTI_CHAR('item_0_0'))->hide();
        mpScreen->search(MULTI_CHAR('item_0_1'))->show();
        mpScreen->search(MULTI_CHAR('item_0_2'))->hide();
        break;
    case 100:
        mpScreen->search(MULTI_CHAR('item_0_0'))->hide();
        mpScreen->search(MULTI_CHAR('item_0_1'))->hide();
        mpScreen->search(MULTI_CHAR('item_0_2'))->show();
        break;
    }
}

#if TARGET_PC
void dMenu_Collect2D_c::setWalletSizeNum(u16 i_walletSize) {
    switch (i_walletSize) {
    case WALLET:
        mpScreen->search(MULTI_CHAR('item_1_0'))->show();
        mpScreen->search(MULTI_CHAR('item_1_1'))->hide();
        mpScreen->search(MULTI_CHAR('item_1_2'))->hide();
        break;
    case BIG_WALLET:
        mpScreen->search(MULTI_CHAR('item_1_0'))->hide();
        mpScreen->search(MULTI_CHAR('item_1_1'))->show();
        mpScreen->search(MULTI_CHAR('item_1_2'))->hide();
        break;
    case GIANT_WALLET:
        mpScreen->search(MULTI_CHAR('item_1_0'))->hide();
        mpScreen->search(MULTI_CHAR('item_1_1'))->hide();
        mpScreen->search(MULTI_CHAR('item_1_2'))->show();
        break;
    }
}
#else
void dMenu_Collect2D_c::setWalletMaxNum(u16 i_walletSize) {
    switch (i_walletSize) {
    case 300:
        mpScreen->search(MULTI_CHAR('item_1_0'))->show();
        mpScreen->search(MULTI_CHAR('item_1_1'))->hide();
        mpScreen->search(MULTI_CHAR('item_1_2'))->hide();
        break;
    case 600:
        mpScreen->search(MULTI_CHAR('item_1_0'))->hide();
        mpScreen->search(MULTI_CHAR('item_1_1'))->show();
        mpScreen->search(MULTI_CHAR('item_1_2'))->hide();
        break;
    case 1000:
        mpScreen->search(MULTI_CHAR('item_1_0'))->hide();
        mpScreen->search(MULTI_CHAR('item_1_1'))->hide();
        mpScreen->search(MULTI_CHAR('item_1_2'))->show();
        break;
    }
}
#endif

void dMenu_Collect2D_c::setSmellType() {
    static const u64 smell_tag[5] = {
        MULTI_CHAR('wolf_med'), MULTI_CHAR('wolf_chi'), MULTI_CHAR('wolf_fis'), MULTI_CHAR('wolf_iri'), MULTI_CHAR('wolf_pou'),
    };
    s32 smellType = -1;
    switch (dComIfGs_getCollectSmell()) {
    case dItemNo_SMELL_YELIA_POUCH_e:
        smellType = 3;
        break;
    case dItemNo_SMELL_POH_e:
        smellType = 4;
        break;
    case dItemNo_SMELL_FISH_e:
        smellType = 2;
        break;
    case dItemNo_SMELL_CHILDREN_e:
        smellType = 1;
        break;
    case dItemNo_SMELL_MEDICINE_e:
        smellType = 0;
        break;
    }
    for (int i = 0; i < 5; i++) {
        if (i == smellType) {
            mpScreen->search(smell_tag[i])->show();
        } else {
            mpScreen->search(smell_tag[i])->hide();
        }
    }
}

void dMenu_Collect2D_c::setHeartPiece() {
    static const u64 heart_tag[4] = {
        MULTI_CHAR('heart_1n'),
        MULTI_CHAR('heart_2n'),
        MULTI_CHAR('heart_3n'),
        MULTI_CHAR('heart_4n'),
    };
    s32 life = dComIfGs_getMaxLife() % 5;
    for (int i = 0; i < MAX_VISIBLE_HEARTPIECES; i++) {
        if (i < life) {
            mpScreen->search(heart_tag[i])->show();
        } else {
            mpScreen->search(heart_tag[i])->hide();
        }
    }
}

void dMenu_Collect2D_c::setPohMaxNum(u8 i_pohNum) {
    if (i_pohNum > MAX_POH_NUM) {
        i_pohNum = MAX_POH_NUM;
    }
    if (field_0x22d[0][3] != 0) {
        int div = i_pohNum / 100;
        s32 index = i_pohNum % 100;
        u8 check = 0;
        if (div > 0) {
            JKRArchive* archive = dComIfGp_getMain2DArchive();
            const char* textureName = dMeter2Info_getNumberTextureName(div);
            ResTIMG* timg = (ResTIMG*)archive->getResource('TIMG', textureName);
            static_cast<J2DPicture*>(mpScreen->search(MULTI_CHAR('item2_3')))->changeTexture(timg, 0);
            check = 1;
        } else {
            mpScreen->search(MULTI_CHAR('item2_3'))->hide();
        }
        div = index / 10;
        index = index % 10;
        if (div != 0 || check != 0) {
            JKRArchive* archive = dComIfGp_getMain2DArchive();
            const char* textureName2 = dMeter2Info_getNumberTextureName(div);
            ResTIMG* timg2 = (ResTIMG*)archive->getResource('TIMG', textureName2);
            static_cast<J2DPicture*>(mpScreen->search(MULTI_CHAR('item2_2')))->changeTexture(timg2, 0);
        } else {
            mpScreen->search(MULTI_CHAR('item2_2'))->hide();
        }
        JKRArchive* archive = dComIfGp_getMain2DArchive();
        const char* textureName3 = dMeter2Info_getNumberTextureName(index);
        ResTIMG* timg3 = (ResTIMG*)archive->getResource('TIMG', textureName3);
        static_cast<J2DPicture*>(mpScreen->search(MULTI_CHAR('item2_1')))->changeTexture(timg3, 0);
    } else {
        mpScreen->search(MULTI_CHAR('item2_3'))->hide();
        mpScreen->search(MULTI_CHAR('item2_2'))->hide();
        mpScreen->search(MULTI_CHAR('item2_1'))->hide();
    }
}

void dMenu_Collect2D_c::setEquipItemFrameColorSword(int i_frame) {
    static const u64 tag[2] = {
        MULTI_CHAR('ken_g_0'),
        MULTI_CHAR('ken_g_1'),
    };

    if (i_frame == -1) {
        if (mEquippedSword != dComIfGs_getSelectEquipSword()) {
            mEquippedSword = dComIfGs_getSelectEquipSword();

            switch (mEquippedSword) {
            case dItemNo_SWORD_e:
                i_frame = 0;
                break;

            case dItemNo_WOOD_STICK_e:
                i_frame = 0;
                break;

            case dItemNo_MASTER_SWORD_e:
                i_frame = 1;
                break;

            case dItemNo_LIGHT_SWORD_e:
                if (dComIfGs_isItemFirstBit(dItemNo_LIGHT_SWORD_e)) {
                    i_frame = 1;
                }
                break;
            }

            for (int i = 0; i < 2; i++) {
                if (i == i_frame && field_0x22d[i + 3][0] != 0) {
                    static_cast<J2DPicture*>(mpScreen->search(tag[i]))
                        ->setBlackWhite(JUtility::TColor(0, 0, 0, 0),
                                        JUtility::TColor(255, 255, 0, 255));
                } else {
                    static_cast<J2DPicture*>(mpScreen->search(tag[i]))
                        ->setBlackWhite(JUtility::TColor(0, 0, 0, 0),
                                        JUtility::TColor(107, 107, 107, 255));
                }
            }
        }
    } else {
        for (int i = 0; i < 2; i++) {
            if (i == i_frame && field_0x22d[i + 3][0] != 0) {
                static_cast<J2DPicture*>(mpScreen->search(tag[i]))
                    ->setBlackWhite(JUtility::TColor(0, 0, 0, 0),
                                    JUtility::TColor(255, 255, 0, 255));
            } else {
                static_cast<J2DPicture*>(mpScreen->search(tag[i]))
                    ->setBlackWhite(JUtility::TColor(0, 0, 0, 0),
                                    JUtility::TColor(107, 107, 107, 255));
            }
        }
    }
}

void dMenu_Collect2D_c::setEquipItemFrameColorShield(int i_frame) {
    static const u64 tag[2] = {
        MULTI_CHAR('tate_g_0'),
        MULTI_CHAR('tate_g_1'),
    };

    if (i_frame == -1) {
        if (mEquippedShield != dComIfGs_getSelectEquipShield()) {
            mEquippedShield = dComIfGs_getSelectEquipShield();

            switch (mEquippedShield) {
            case dItemNo_SHIELD_e:
            case dItemNo_WOOD_SHIELD_e:
                i_frame = 0;
                break;

            case dItemNo_HYLIA_SHIELD_e:
                i_frame = 1;
                break;
            }

            for (int i = 0; i < 2; i++) {
                if (i == i_frame && field_0x22d[i + 3][1] != 0) {
                    static_cast<J2DPicture*>(mpScreen->search(tag[i]))
                        ->setBlackWhite(JUtility::TColor(0, 0, 0, 0),
                                        JUtility::TColor(255, 255, 0, 255));
                } else {
                    static_cast<J2DPicture*>(mpScreen->search(tag[i]))
                        ->setBlackWhite(JUtility::TColor(0, 0, 0, 0),
                                        JUtility::TColor(107, 107, 107, 255));
                }
            }
        }
    } else {
        for (int i = 0; i < 2; i++) {
            if (i == i_frame && field_0x22d[i + 3][1] != 0) {
                static_cast<J2DPicture*>(mpScreen->search(tag[i]))
                    ->setBlackWhite(JUtility::TColor(0, 0, 0, 0),
                                    JUtility::TColor(255, 255, 0, 255));
            } else {
                static_cast<J2DPicture*>(mpScreen->search(tag[i]))
                    ->setBlackWhite(JUtility::TColor(0, 0, 0, 0),
                                    JUtility::TColor(107, 107, 107, 255));
            }
        }
    }
}

void dMenu_Collect2D_c::setEquipItemFrameColorClothes(int i_frame) {
    static const u64 tag[3] = {
        MULTI_CHAR('fuku_g_0'),
        MULTI_CHAR('fuku_g_1'),
        MULTI_CHAR('fuku_g_2'),
    };

    if (i_frame == -1) {
        if (mEquippedClothes != dComIfGs_getSelectEquipClothes()) {
            mEquippedClothes = dComIfGs_getSelectEquipClothes();

            switch (mEquippedClothes) {
            case dItemNo_WEAR_KOKIRI_e:
                i_frame = 0;
                break;

            case dItemNo_WEAR_ZORA_e:
                i_frame = 1;
                break;

            case dItemNo_ARMOR_e:
                i_frame = 2;
                break;
            }

            for (int i = 0; i < 3; i++) {
                if (i == i_frame && field_0x22d[i + 3][2] != 0) {
                    static_cast<J2DPicture*>(mpScreen->search(tag[i]))
                        ->setBlackWhite(JUtility::TColor(0, 0, 0, 0),
                                        JUtility::TColor(255, 255, 0, 255));
                } else {
                    static_cast<J2DPicture*>(mpScreen->search(tag[i]))
                        ->setBlackWhite(JUtility::TColor(0, 0, 0, 0),
                                        JUtility::TColor(107, 107, 107, 255));
                }
            }
        }
    } else {
        for (int i = 0; i < 3; i++) {
            if (i == i_frame && field_0x22d[i + 3][2] != 0) {
                static_cast<J2DPicture*>(mpScreen->search(tag[i]))
                    ->setBlackWhite(JUtility::TColor(0, 0, 0, 0),
                                    JUtility::TColor(255, 255, 0, 255));
            } else {
                static_cast<J2DPicture*>(mpScreen->search(tag[i]))
                    ->setBlackWhite(JUtility::TColor(0, 0, 0, 0),
                                    JUtility::TColor(107, 107, 107, 255));
            }
        }
    }
}

#if TARGET_PC
// ---- TPHD (Wii U TV) collection screen -----------------------------------------------------------
namespace {
// RPX 0x10093e20 (read by 0x029b20c0): the Wii U grid, 9 columns x 5 rows, row-major, x = column.
const char* const kHdGrid[5][9] = {
    {NULL, NULL, NULL, NULL, "fuku_n1", "fuku_n0", "fuku_n2", NULL, NULL},
    {NULL, "heart_kn", NULL, "tate_n0", "tate_n1", NULL, "ken_n0", "ken_n1", "kamen_n"},
    {"item_1_n", "item_0_n", "item_2_n", "wolf_n", NULL, NULL, NULL, NULL, NULL},
    {"kabu_6n", "fish_3_n", "maki_5_n", "lett_4_n", "stam_7_n", NULL, NULL, NULL, NULL},
    {"save_n", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "option_n"},
};
u8 s_hdX = 0, s_hdY = 0, s_hdPX = 0xFF, s_hdPY = 0xFF;
s8 s_hdGcX[5][9], s_hdGcY[5][9];  // GC cell (mCursorX / mCursorY) holding the same pane

// Equipped / normal frame looks (RPX 0x029b14a4 reads them from ar_equip / ar_01, 0x029b1ae4
// applies them): texture, white colour and size.
struct HdFrameTex {
    const ResTIMG* tex = NULL;
    JUtility::TColor white;
    f32 w = 0.0f, h = 0.0f;
};
HdFrameTex s_hdEquipTex, s_hdNormalTex;
u8 s_hdEquip[3] = {0xFF, 0xFF, 0xFF};

u64 hdTag8(const char* s) {
    u64 t = 0;
    for (int i = 0; i < 8 && s[i] != 0; i++) {
        t = (t << 8) | static_cast<u8>(s[i]);
    }
    return t;
}

bool hdValid(int x, int y) {
    return x >= 0 && x < 9 && y >= 0 && y < 5 && kHdGrid[y][x] != NULL && s_hdGcX[y][x] >= 0;
}

HdFrameTex hdReadFrame(J2DScreen* scr, const char* tag) {
    HdFrameTex t;
    J2DPane* p = scr->search(hdTag8(tag));
    if (p != NULL && p->getTypeID() == 18) {
        J2DPicture* pic = static_cast<J2DPicture*>(p);
        if (pic->getTexture(0) != NULL) {
            t.tex = pic->getTexture(0)->getTexInfo();
        }
        t.white = pic->getWhite();
        t.w = p->getWidth();
        t.h = p->getHeight();
    }
    return t;
}

void hdApplyFrame(J2DScreen* scr, const char* tag, const HdFrameTex& t) {
    J2DPane* p = scr->search(hdTag8(tag));
    if (p == NULL || p->getTypeID() != 18 || t.tex == NULL) {
        return;
    }
    J2DPicture* pic = static_cast<J2DPicture*>(p);
    pic->changeTexture(t.tex, 0);
    pic->setBlackWhite(JUtility::TColor(0, 0, 0, 0), t.white);
    // Size: the frames are centre-anchored (base position 4); J2DPane::resize keeps the top-left
    // corner, which moved the frame off its cell (and the cursor, centred on the cell, looked
    // off-centre). Scale about the anchor instead, from the size and scale the blo gave the pane.
    struct Orig {
        J2DPane* pane;
        f32 w, h, sx, sy;
    };
    static Orig s_orig[16];
    static J2DScreen* s_origScr = NULL;
    if (s_origScr != scr) {
        s_origScr = scr;
        for (Orig& o : s_orig) {
            o.pane = NULL;
        }
    }
    Orig* o = NULL;
    for (Orig& e : s_orig) {
        if (e.pane == p) {
            o = &e;
            break;
        }
    }
    if (o == NULL) {
        for (Orig& e : s_orig) {
            if (e.pane == NULL) {
                e = Orig{p, p->getWidth(), p->getHeight(), p->getScaleX(), p->getScaleY()};
                o = &e;
                break;
            }
        }
    }
    if (o != NULL && o->w > 0.0f && o->h > 0.0f) {
        p->scale(o->sx * t.w / o->w, o->sy * t.h / o->h);
    }
}

// Equipped cells (RPX 0x029b1bc8 / 0x029b1d58 / 0x029b1ec4).
int hdSwordCell() {
    switch (dComIfGs_getSelectEquipSword()) {
    case dItemNo_SWORD_e:
    case dItemNo_WOOD_STICK_e:
        return 6;
    case dItemNo_MASTER_SWORD_e:
        return 7;
    case dItemNo_LIGHT_SWORD_e:
        return dComIfGs_isItemFirstBit(dItemNo_LIGHT_SWORD_e) ? 7 : -1;
    }
    return -1;
}
int hdShieldCell() {
    switch (dComIfGs_getSelectEquipShield()) {
    case dItemNo_WOOD_SHIELD_e:
    case dItemNo_SHIELD_e:
        return 3;
    case dItemNo_HYLIA_SHIELD_e:
        return 4;
    }
    return -1;
}
int hdClothesCell() {
    switch (dComIfGs_getSelectEquipClothes()) {
    case dItemNo_WEAR_ZORA_e:
        return 4;
    case dItemNo_WEAR_KOKIRI_e:
        return 5;
    case dItemNo_ARMOR_e:
        return 6;
    }
    return -1;
}
}  // namespace

void dMenu_Collect2D_c::hdInit() {
    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 9; x++) {
            s_hdGcX[y][x] = s_hdGcY[y][x] = -1;
            if (kHdGrid[y][x] == NULL) {
                continue;
            }
            const u64 tag = hdTag8(kHdGrid[y][x]);
            for (int i = 0; i < 7; i++) {
                for (int j = 0; j < 6; j++) {
                    if (getItemTag(i, j, true) == tag && mpSelPm[i][j] != NULL) {
                        s_hdGcX[y][x] = i;
                        s_hdGcY[y][x] = j;
                    }
                }
            }
        }
    }
    s_hdEquipTex = hdReadFrame(mpScreen, "ar_equip");
    s_hdNormalTex = hdReadFrame(mpScreen, "ar_01");
    s_hdEquip[0] = s_hdEquip[1] = s_hdEquip[2] = 0xFF;
    // Start on the Wii U cell of the GC cursor.
    bool found = false;
    for (int y = 0; y < 5 && !found; y++) {
        for (int x = 0; x < 9 && !found; x++) {
            if (hdValid(x, y) && s_hdGcX[y][x] == mCursorX && s_hdGcY[y][x] == mCursorY) {
                s_hdX = x;
                s_hdY = y;
                found = true;
            }
        }
    }
    for (int y = 0; y < 5 && !found; y++) {
        for (int x = 0; x < 9 && !found; x++) {
            if (hdValid(x, y)) {
                s_hdX = x;
                s_hdY = y;
                found = true;
                mCursorX = s_hdGcX[y][x];
                mCursorY = s_hdGcY[y][x];
            }
        }
    }
    s_hdPX = s_hdPY = 0xFF;
    hdEquipFrames();
    cursorPosSet();
    hdSetCursorParam();
    setItemNameString(mCursorX, mCursorY);
}

// RPX 0x029b2748: cursor shape per cell.
void dMenu_Collect2D_c::hdSetCursorParam() {
    const int x = s_hdX;
    const int y = s_hdY;
    if (y == 4) {
        mpDrawCursor->setParam(1.0f, 1.0f, 0.05f, 0.5f, 0.6f);
    } else if (y == 1 && x == 1) {
        mpDrawCursor->setParam(1.0f, 1.0f, 0.03f, 0.6f, 0.72f);
    } else if (y == 1 && x == 8) {
        mpDrawCursor->setParam(0.8f, 0.85f, 0.03f, 0.6f, 0.72f);
    } else if ((y == 1 && ((x >= 6 && x <= 7 && x == hdSwordCell()) ||
                           (x >= 3 && x <= 4 && x == hdShieldCell()))) ||
               (y == 0 && x >= 4 && x <= 5 && x == hdClothesCell()))
    {
        mpDrawCursor->setParam(1.21f, 0.913f, 0.1f, 0.7f, 0.84f);  // on an "equipped" frame
    } else {
        mpDrawCursor->setParam(1.1f, 0.83f, 0.1f, 0.7f, 0.84f);
    }
}

// RPX 0x029b76f4: stick navigation on the Wii U grid. The GC cursor (mCursorX / mCursorY) follows
// on the cell holding the same pane, so the GC actions (equip, save, options, letters...) keep working.
void dMenu_Collect2D_c::hdCursorMove() {
    mpStick->checkTrigger();
    const u8 ox = s_hdX;
    const u8 oy = s_hdY;
    int x = ox;
    int y = oy;
    const bool havePrev = s_hdPX != 0xFF;
    const bool right = mpStick->checkRightTrigger();
    const bool left = !right && mpStick->checkLeftTrigger();
    if (left && x != 0) {
        if (x == 8 && y == 1 && havePrev && s_hdPX > 2 && s_hdPY < 2) {
            x = s_hdPX;  // back to where the cursor came from
            y = s_hdPY;
        } else {
            int t = x;
            do {
                t--;
                if (hdValid(t, y)) {
                    x = t;
                    break;
                }
            } while (t != 0);
        }
    } else if (right && x <= 7) {
        if (x == 5 && y == 2 && havePrev && s_hdPX >= 3 && s_hdPY <= 1) {
            x = s_hdPX;
            y = s_hdPY;
        } else if (ox == 1 && oy == 1 && havePrev && s_hdPX == 3 && s_hdPY == 1) {
            x = 3;
            y = 1;
        } else {
            int t = x;
            do {
                t++;
                if (hdValid(t, y)) {
                    x = t;
                    break;
                }
            } while (t < 8);
        }
    }
    if (mpStick->checkUpTrigger()) {
        if (y != 0) {
            int cx = x;
            int cy = y;
            while (cy != 0) {
                cy--;
                if (cy == 0) {
                    if (cx <= 2) {
                        cx = 3;
                        cy = 1;
                    } else if (cx >= 8) {
                        cx = 7;
                        cy = 1;
                    }
                } else if (cy == 1) {
                    if (cx < 3) {
                        cx = 1;
                    }
                } else if (cy == 2 && cx == 4) {
                    cx = 3;
                }
                if (hdValid(cx, cy)) {
                    x = cx;
                    y = cy;
                    break;
                }
            }
        }
    } else if (mpStick->checkDownTrigger() && y <= 3) {
        if (x == 1 && y == 1 && havePrev && s_hdPY >= 2) {
            x = s_hdPX;
            y = s_hdPY;
        } else {
            int cy = y;
            bool found = false;
            while (cy < 4) {
                cy++;
                if (hdValid(x, cy)) {
                    found = true;
                    break;
                }
            }
            if (found) {
                y = cy;
            } else {
                y = 4;
                x = x > 7 ? 8 : 0;
            }
        }
    }
    if (!hdValid(x, y)) {
        x = ox;
        y = oy;
    }
    if (x != ox || y != oy) {
        s_hdPX = ox;
        s_hdPY = oy;
        s_hdX = x;
        s_hdY = y;
        field_0x259 = mCursorX;
        field_0x25a = mCursorY;
        mCursorX = s_hdGcX[y][x];
        mCursorY = s_hdGcY[y][x];
        if (y == 4) {
            Z2GetAudioMgr()->seStart(Z2SE_SY_CURSOR_OPTION, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
        } else {
            Z2GetAudioMgr()->seStart(Z2SE_SY_CURSOR_ITEM, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
        }
        cursorPosSet();
        hdSetCursorParam();
        setItemNameString(mCursorX, mCursorY);
    }
}

// Equipped frames (RPX 0x029b1bc8 / 0x029b1d58 / 0x029b1ec4): the frame of the equipped sword /
// shield / clothes gets the ornate texture, the others the plain one.
void dMenu_Collect2D_c::hdEquipFrames() {
    const u8 eq[3] = {dComIfGs_getSelectEquipSword(), dComIfGs_getSelectEquipShield(),
                      dComIfGs_getSelectEquipClothes()};
    if (eq[0] == s_hdEquip[0] && eq[1] == s_hdEquip[1] && eq[2] == s_hdEquip[2]) {
        return;
    }
    s_hdEquip[0] = eq[0];
    s_hdEquip[1] = eq[1];
    s_hdEquip[2] = eq[2];
    const int sw = hdSwordCell();
    const int sh = hdShieldCell();
    const int cl = hdClothesCell();
    hdApplyFrame(mpScreen, "sw_equip", sw == 6 ? s_hdEquipTex : s_hdNormalTex);
    hdApplyFrame(mpScreen, "sw_01", sw == 7 ? s_hdEquipTex : s_hdNormalTex);
    hdApplyFrame(mpScreen, "sh_01", sh == 3 ? s_hdEquipTex : s_hdNormalTex);
    hdApplyFrame(mpScreen, "sh_equip", sh == 4 ? s_hdEquipTex : s_hdNormalTex);
    hdApplyFrame(mpScreen, "ar_01", cl == 4 ? s_hdEquipTex : s_hdNormalTex);
    hdApplyFrame(mpScreen, "ar_equip", cl == 5 ? s_hdEquipTex : s_hdNormalTex);
    hdApplyFrame(mpScreen, "ar_02", cl == 6 ? s_hdEquipTex : s_hdNormalTex);
    if (mpDrawCursor != NULL) {
        hdSetCursorParam();
    }
}
#endif

void dMenu_Collect2D_c::setHIO(bool i_useHIO) {
    static const u64 kaz_n[9] = {
        MULTI_CHAR('uzu_00'), MULTI_CHAR('uzu_01'), MULTI_CHAR('uzu_02'), MULTI_CHAR('uzu_03'), MULTI_CHAR('uzu_04'), MULTI_CHAR('uzu_05'), MULTI_CHAR('uzu_06'), MULTI_CHAR('uzu_07'), MULTI_CHAR('hishi'),
    };

    if (mBlueSmokePosX != g_drawHIO.mCollectScreen.mBlueSmokePosX ||
        mBlueSmokePosY != g_drawHIO.mCollectScreen.mBlueSmokePosY)
    {
        mBlueSmokePosX = g_drawHIO.mCollectScreen.mBlueSmokePosX;
        mBlueSmokePosY = g_drawHIO.mCollectScreen.mBlueSmokePosY;
        mpModelBg->paneTrans(mBlueSmokePosX, mBlueSmokePosY);
    }

    if (mBlueSmokeScale != g_drawHIO.mCollectScreen.mBlueSmokeScale) {
        mBlueSmokeScale = g_drawHIO.mCollectScreen.mBlueSmokeScale;
        mpModelBg->scale(mBlueSmokeScale, mBlueSmokeScale);
    }

    if (mBlueSmokeAlpha != g_drawHIO.mCollectScreen.mBlueSmokeAlpha) {
        mBlueSmokeAlpha = g_drawHIO.mCollectScreen.mBlueSmokeAlpha;
        mpModelBg->setAlphaRate(mBlueSmokeAlpha);
    }

    if (mHeartVesselPosX != g_drawHIO.mCollectScreen.mHeartVesselPosX ||
        mHeartVesselPosY != g_drawHIO.mCollectScreen.mHeartVesselPosY)
    {
        mHeartVesselPosX = g_drawHIO.mCollectScreen.mHeartVesselPosX;
        mHeartVesselPosY = g_drawHIO.mCollectScreen.mHeartVesselPosY;
        mpHeartParent->paneTrans(mHeartVesselPosX, mHeartVesselPosY);
    }

    if (mHeartVesselScale != g_drawHIO.mCollectScreen.mHeartVesselScale) {
        mHeartVesselScale = g_drawHIO.mCollectScreen.mHeartVesselScale;
        mpHeartParent->scale(mHeartVesselScale, mHeartVesselScale);
    }

    if (mHeartPiecePosX != g_drawHIO.mCollectScreen.mHeartPiecePosX ||
        mHeartPiecePosY != g_drawHIO.mCollectScreen.mHeartPiecePosY)
    {
        mHeartPiecePosX = g_drawHIO.mCollectScreen.mHeartPiecePosX;
        mHeartPiecePosY = g_drawHIO.mCollectScreen.mHeartPiecePosY;
        mpHeartPiece->paneTrans(mHeartPiecePosX, mHeartPiecePosY);
    }

    if (mHeartPieceScale != g_drawHIO.mCollectScreen.mHeartPieceScale) {
        mHeartPieceScale = g_drawHIO.mCollectScreen.mHeartPieceScale;
        mpHeartPiece->scale(mHeartPieceScale, mHeartPieceScale);
    }

    if (g_drawHIO.mCollectScreen.mColorDebugON) {
        J2DPicture* basePicture = (J2DPicture*)mpScreen->search(MULTI_CHAR('nht_base'));
        basePicture->setBlackWhite(g_drawHIO.mCollectScreen.mVesselBack[0],
                                   g_drawHIO.mCollectScreen.mVesselFront[0]);

        for (int i = 0; i < 9; i++) {
            J2DPicture* kazPicture = (J2DPicture*)mpScreen->search(kaz_n[i]);
#if TARGET_PC
            if (kazPicture == NULL) {
                continue;  // not in the Wii U layout
            }
#endif
            kazPicture->setBlackWhite(g_drawHIO.mCollectScreen.mVesselBack[1],
                                      g_drawHIO.mCollectScreen.mVesselFront[1]);
        }
    }

    if (g_drawHIO.mCollectScreen.mButtonDebugON || i_useHIO) {
        if (mpButtonAB[0] != NULL) {
            mpButtonAB[0]->paneTrans(g_drawHIO.mCollectScreen.mButtonAPosX,
                                     g_drawHIO.mCollectScreen.mButtonAPosY);
            mpButtonAB[0]->scale(g_drawHIO.mCollectScreen.mButtonAScale,
                                 g_drawHIO.mCollectScreen.mButtonAScale);
        }
        if (mpButtonAB[1] != NULL) {
            mpButtonAB[1]->paneTrans(g_drawHIO.mCollectScreen.mButtonBPosX,
                                     g_drawHIO.mCollectScreen.mButtonBPosY);
            mpButtonAB[1]->scale(g_drawHIO.mCollectScreen.mButtonBScale,
                                 g_drawHIO.mCollectScreen.mButtonBScale);
        }
        if (mpButtonText[0] != NULL) {
            mpButtonText[0]->paneTrans(g_drawHIO.mCollectScreen.mButtonATextPosX,
                                       g_drawHIO.mCollectScreen.mButtonATextPosY);
            mpButtonText[0]->scale(g_drawHIO.mCollectScreen.mButtonATextScale,
                                   g_drawHIO.mCollectScreen.mButtonATextScale);
        }
        if (mpButtonText[1] != NULL) {
            mpButtonText[1]->paneTrans(g_drawHIO.mCollectScreen.mButtonBTextPosX,
                                       g_drawHIO.mCollectScreen.mButtonBTextPosY);
            mpButtonText[1]->scale(g_drawHIO.mCollectScreen.mButtonBTextScale,
                                   g_drawHIO.mCollectScreen.mButtonBTextScale);
        }
    }
}

u64 dMenu_Collect2D_c::getItemTag(int i_tag1, int i_tag2, bool param_3) {
    static const u64 itemTag[6][7] = {
        {0, 0, 0, MULTI_CHAR('ken_n0'), MULTI_CHAR('ken_n1'), MULTI_CHAR('heart_kn'), MULTI_CHAR('kamen_n')},
        {0, 0, 0, MULTI_CHAR('tate_n0'), MULTI_CHAR('tate_n1')},
        {
            0,
            0,
            0,
            MULTI_CHAR('fuku_n0'),
            MULTI_CHAR('fuku_n1'),
            MULTI_CHAR('fuku_n2'),
        },
        {
            MULTI_CHAR('item_1_n'),
            MULTI_CHAR('item_0_n'),
            MULTI_CHAR('kabu_6n'),
            MULTI_CHAR('maki_5_n'),
        },
        {
            MULTI_CHAR('wolf_n'),
            MULTI_CHAR('item_2_n'),
            MULTI_CHAR('fish_3_n'),
            MULTI_CHAR('lett_4_n'),
        },
        {MULTI_CHAR('save_n'), MULTI_CHAR('option_n')},
    };

    if (i_tag2 == 5 && !param_3) {
        return 0;
    }
#if TARGET_PC
    // TPHD: the stamp album cell (stam_7_n, Wii U grid x 4 / y 3) lives on the free GC cell (4, 3).
    if (collectRevo() && i_tag1 == 4 && i_tag2 == 3) {
        return MULTI_CHAR('stam_7_n');
    }
#endif
    return itemTag[i_tag2][i_tag1];
}

void dMenu_Collect2D_c::wait_init() {
    JKRHeap* heap = mDoExt_setCurrentHeap(mpSubHeap);

    if (mpSaveScrn != NULL) {
        mpSaveScrn->_delete();
        JKR_DELETE(mpSaveScrn);
        mpSaveScrn = NULL;
        mpSubHeap->freeAll();
    }

    if (mpOptionScrn != NULL) {
        mpOptionScrn->_delete();
        JKR_DELETE(mpOptionScrn);
        mpOptionScrn = NULL;
        mpSubHeap->freeAll();
    }

    if (mpLetterScrn != NULL) {
        JKR_DELETE(mpLetterScrn);
        mpLetterScrn = NULL;
        mpSubHeap->freeAll();
    }

    if (mpFishingScrn != NULL) {
        JKR_DELETE(mpFishingScrn);
        mpFishingScrn = NULL;
        mpSubHeap->freeAll();
    }

    if (mpSkillScrn != NULL) {
        JKR_DELETE(mpSkillScrn);
        mpSkillScrn = NULL;
        mpSubHeap->freeAll();
    }

    if (mpInsectScrn != NULL) {
        JKR_DELETE(mpInsectScrn);
        mpInsectScrn = NULL;
        mpSubHeap->freeAll();
    }

    mDoExt_setCurrentHeap(heap);
}

bool dMenu_Collect2D_c::isStampSelected() {
    return collectRevo() && mCursorX == 4 && mCursorY == 3 && field_0x22d[4][3] != 0;
}

void dMenu_Collect2D_c::openStampSubWindow() {
    if (field_0x22d[4][3] != 0 && mDoGph_gInf_c::getFader()->mStatus == 1) {
        mSubWindowOpenCheck = 7;
        dMeter2Info_set2DVibration();
    }
}

void dMenu_Collect2D_c::wait_proc() {
#if TARGET_PC
    if (pointerWait()) {
        return;
    }
#endif

    if (dMw_A_TRIGGER()) {
        if (mCursorX == 0 && mCursorY == 5) {
            if (mDoGph_gInf_c::getFader()->mStatus == 1) {
                mSubWindowOpenCheck = 1;
                Z2GetAudioMgr()->seStart(Z2SE_SY_MENU_CHANGE_WINDOW, NULL, 0, 0, 1.0f, 1.0f, -1.0f,
                                         -1.0f, 0);
                dMeter2Info_set2DVibrationM();
            }
        } else if (mCursorX == 1 && mCursorY == 5) {
            if (mDoGph_gInf_c::getFader()->mStatus == 1) {
                mSubWindowOpenCheck = 2;
                Z2GetAudioMgr()->seStart(Z2SE_SY_MENU_CHANGE_WINDOW, NULL, 0, 0, 1.0f, 1.0f, -1.0f,
                                         -1.0f, 0);
                dMeter2Info_set2DVibrationM();
            }
        } else if (mCursorX == 3 && mCursorY == 4) {
            if (field_0x22d[3][4] != 0 && mDoGph_gInf_c::getFader()->mStatus == 1) {
                mSubWindowOpenCheck = 3;
                dMeter2Info_set2DVibration();
            }
        } else if (mCursorX == 2 && mCursorY == 4) {
            if (isFishIconVisible() && mDoGph_gInf_c::getFader()->mStatus == 1) {
                mSubWindowOpenCheck = 4;
                dMeter2Info_set2DVibration();
            }
        } else if (mCursorX == 3 && mCursorY == 3) {
            if (isSkillIconVisible() && mDoGph_gInf_c::getFader()->mStatus == 1) {
                mSubWindowOpenCheck = 5;
                dMeter2Info_set2DVibration();
            }
        } else if (mCursorX == 2 && mCursorY == 3) {
            if (isInsectIconVisible() && mDoGph_gInf_c::getFader()->mStatus == 1) {
                mSubWindowOpenCheck = 6;
                dMeter2Info_set2DVibration();
            }
#if TARGET_PC
        } else if (isStampSelected()) {
            // TPHD: open the stamp album.
            openStampSubWindow();
#endif
        } else {
            if (field_0x22d[mCursorX][mCursorY] != 0 && !mIsWolf) {
                if ((mCursorX >= 3 && mCursorX <= 4) || (mCursorX == 5 && mCursorY == 2)) {
                    u8 cursorY = mCursorY;
                    if (cursorY == 0) {
                        if (daPy_getPlayerActorClass()->getSwordChangeWaitTimer() == 0) {
                            changeSword();
                        }
                    } else if (cursorY == 1) {
                        if (daPy_getPlayerActorClass()->getShieldChangeWaitTimer() == 0) {
                            changeShield();
                        }
                    } else if (cursorY == 2 &&
                               daPy_getPlayerActorClass()->getClothesChangeWaitTimer() == 0)
                    {
                        changeClothe();
                    }
                }
            }
        }
    } else {
        if (daPy_getPlayerActorClass()->getSwordChangeWaitTimer() == 0 &&
            daPy_getPlayerActorClass()->getShieldChangeWaitTimer() == 0 &&
            daPy_getPlayerActorClass()->getClothesChangeWaitTimer() == 0)
        {
#if TARGET_PC
            if (collectRevo()) {
                hdCursorMove();
            } else {
                cursorMove();
            }
#else
            cursorMove();
#endif
        }
#if TARGET_PC
        if (collectRevo()) {
            // TPHD 0x029b8020: A caption (and badge) per Wii U cell.
            const int x = s_hdX;
            const int y = s_hdY;
            const bool owned = field_0x22d[mCursorX][mCursorY] != 0;
            u16 a = 0;
            if ((y == 0 && x >= 4 && x <= 6) || (y == 1 && ((x >= 6 && x <= 7) || (x >= 3 && x <= 4)))) {
                a = owned && !mIsWolf ? 0x436 : 0;  // "Equip"
            } else if (y == 4 && (x == 0 || x == 8)) {
                a = 0x40C;
            } else if (y == 3 && owned) {
                bool open = false;
                switch (x) {
                case 0: open = isInsectIconVisible(); break;
                case 1: open = isFishIconVisible(); break;
                case 2: open = isSkillIconVisible(); break;
                case 3: open = field_0x22d[3][4] != 0; break;
                case 4: open = field_0x22d[4][3] != 0; break;  // stamps
                }
                a = open ? 0x3EE : 0;  // "Open"
            }
            setAButtonString(a);
        } else
#endif
        if (mCursorX >= 3 && mCursorX <= 4 && mCursorY <= 2 || (mCursorX == 5 && mCursorY == 2)) {
            if (field_0x22d[mCursorX][mCursorY] != 0 && !mIsWolf) {
                setAButtonString(0x436);
            } else {
                setAButtonString(0);
            }
        } else if ((mCursorX == 0 && mCursorY == 5) || (mCursorX == 1 && mCursorY == 5)) {
            setAButtonString(0x40C);
        } else {
            if (field_0x22d[mCursorX][mCursorY] != 0) {
                if (mCursorX == 3 && mCursorY == 4) {
                    if (field_0x22d[3][4] != 0) {
                        setAButtonString(0x3EE);
                    }
                } else if (mCursorX == 2 && mCursorY == 4) {
                    if (isFishIconVisible()) {
                        setAButtonString(0x3EE);
                    }
                } else if (mCursorX == 3 && mCursorY == 3) {
                    if (isSkillIconVisible()) {
                        setAButtonString(0x3EE);
                    }
                } else if (mCursorX == 2 && mCursorY == 3) {
                    if (isInsectIconVisible()) {
                        setAButtonString(0x3EE);
                    }
                } else if (mCursorX < 3 && mCursorY < 3) {
                    setAButtonString(0x40C);
                } else {
                    setAButtonString(0);
                }
            } else {
                setAButtonString(0);
            }
        }
        setBButtonString(0x3F9);
    }
}

#if TARGET_PC
void dMenu_Collect2D_c::pointerActivateCurrent() {
    if (mCursorX == 0 && mCursorY == 5) {
        if (mDoGph_gInf_c::getFader()->mStatus == 1) {
            mSubWindowOpenCheck = 1;
            Z2GetAudioMgr()->seStart(Z2SE_SY_MENU_CHANGE_WINDOW, NULL, 0, 0, 1.0f, 1.0f, -1.0f,
                                     -1.0f, 0);
            dMeter2Info_set2DVibrationM();
        }
    } else if (mCursorX == 1 && mCursorY == 5) {
        if (mDoGph_gInf_c::getFader()->mStatus == 1) {
            mSubWindowOpenCheck = 2;
            Z2GetAudioMgr()->seStart(Z2SE_SY_MENU_CHANGE_WINDOW, NULL, 0, 0, 1.0f, 1.0f, -1.0f,
                                     -1.0f, 0);
            dMeter2Info_set2DVibrationM();
        }
    } else if (mCursorX == 3 && mCursorY == 4) {
        if (field_0x22d[3][4] != 0 && mDoGph_gInf_c::getFader()->mStatus == 1) {
            mSubWindowOpenCheck = 3;
            dMeter2Info_set2DVibration();
        }
    } else if (mCursorX == 2 && mCursorY == 4) {
        if (isFishIconVisible() && mDoGph_gInf_c::getFader()->mStatus == 1) {
            mSubWindowOpenCheck = 4;
            dMeter2Info_set2DVibration();
        }
    } else if (mCursorX == 3 && mCursorY == 3) {
        if (isSkillIconVisible() && mDoGph_gInf_c::getFader()->mStatus == 1) {
            mSubWindowOpenCheck = 5;
            dMeter2Info_set2DVibration();
        }
    } else if (mCursorX == 2 && mCursorY == 3) {
        if (isInsectIconVisible() && mDoGph_gInf_c::getFader()->mStatus == 1) {
            mSubWindowOpenCheck = 6;
            dMeter2Info_set2DVibration();
        }
    } else if (isStampSelected()) {
        openStampSubWindow();
    } else if (field_0x22d[mCursorX][mCursorY] != 0 && !mIsWolf) {
        if ((mCursorX >= 3 && mCursorX <= 4) || (mCursorX == 5 && mCursorY == 2)) {
            u8 cursorY = mCursorY;
            if (cursorY == 0) {
                if (daPy_getPlayerActorClass()->getSwordChangeWaitTimer() == 0) {
                    changeSword();
                }
            } else if (cursorY == 1) {
                if (daPy_getPlayerActorClass()->getShieldChangeWaitTimer() == 0) {
                    changeShield();
                }
            } else if (cursorY == 2 &&
                       daPy_getPlayerActorClass()->getClothesChangeWaitTimer() == 0)
            {
                changeClothe();
            }
        }
    }
}

bool dMenu_Collect2D_c::pointerWait() {
    dusk::menu_pointer::begin_context(dusk::menu_pointer::Context::Collection);
    for (u8 y = 0; y < 6; ++y) {
        for (u8 x = 0; x < 7; ++x) {
            if (getItemTag(x, y, true) == 0 || !dusk::menu_pointer::hit_pane(mpSelPm[x][y], 8.0f)) {
                continue;
            }
            dusk::menu_pointer::set_hover_target(static_cast<dusk::menu_pointer::TargetId>(x + y * 7));
            if (mCursorX != x || mCursorY != y) {
                mDoAud_seStart(Z2SE_SY_MENU_CURSOR_COMMON, NULL, 0, 0);
                mCursorX = x;
                mCursorY = y;
                cursorPosSet();
                setItemNameString(mCursorX, mCursorY);
            }
            if (dusk::menu_pointer::consume_click()) {
                pointerActivateCurrent();
                return true;
            }
            return false;
        }
    }
    return false;
}
#endif


void dMenu_Collect2D_c::save_open_init() {
    JKRHeap* heap = mDoExt_setCurrentHeap(mpSubHeap);
    if (!mpSaveScrn) {
        mpSaveScrn = JKR_NEW dMenu_save_c();
    }
    mpSaveScrn->setUseType(1);
    mpSaveScrn->_create();
    mpSaveScrn->initialize();
    setAButtonString(0);
    setBButtonString(0);
    mpDrawCursor->offPlayAllAnime();
    mDoExt_setCurrentHeap(heap);
}

void dMenu_Collect2D_c::save_open_proc() {
    JKRHeap* heap = mDoExt_setCurrentHeap(mpSubHeap);
    if (mpSaveScrn->getSaveStatus() == 1) {
        mpSaveScrn->_open();
    }
    if (mpSaveScrn->getSaveStatus() == 2) {
        mProcess = 2;
    }
    mDoExt_setCurrentHeap(heap);
}

void dMenu_Collect2D_c::save_move_init() {
    setAButtonString(0x040c);
    setBButtonString(0);
}

void dMenu_Collect2D_c::save_move_proc() {
    JKRHeap* heap = mDoExt_setCurrentHeap(mpSubHeap);
    if (mpSaveScrn->getSaveStatus() == 2) {
        mpSaveScrn->_move();
    }
    if (mpSaveScrn->getSaveStatus() == 3) {
        mProcess = 3;
    }
    mDoExt_setCurrentHeap(heap);
}

void dMenu_Collect2D_c::save_close_init() {
    setAButtonString(0);
    setBButtonString(0);
}

void dMenu_Collect2D_c::save_close_proc() {
    JKRHeap* heap = mDoExt_setCurrentHeap(mpSubHeap);
    if (mpSaveScrn->getSaveStatus() == 3) {
        mpSaveScrn->_close();
    }
    if (mpSaveScrn->getSaveStatus() == 0) {
        mProcess = 0;
        mpDrawCursor->onPlayAllAnime();
    }
    mDoExt_setCurrentHeap(heap);
}

void dMenu_Collect2D_c::option_open_init() {
    JKRHeap* heap = mDoExt_setCurrentHeap(mpSubHeap);
    if (!mpOptionScrn) {
        mpOptionScrn = JKR_NEW dMenu_Option_c(dComIfGp_getCollectResArchive(), mpStick);
    }
    mpOptionScrn->initialize();
    setAButtonString(0);
    setBButtonString(0);
    mpDrawCursor->offPlayAllAnime();
    mDoExt_setCurrentHeap(heap);
}

void dMenu_Collect2D_c::option_open_proc() {
    JKRHeap* heap = mDoExt_setCurrentHeap(mpSubHeap);
    if (mpOptionScrn->getQuitStatus() == 1) {
        mpOptionScrn->_open();
    }
    if (mpOptionScrn->getQuitStatus() == 2) {
        mProcess = 5;
    }
    mDoExt_setCurrentHeap(heap);
}

void dMenu_Collect2D_c::option_move_init() {
    setAButtonString(0x040c);
    setBButtonString(0x03f9);
}

void dMenu_Collect2D_c::option_move_proc() {
    JKRHeap* heap = mDoExt_setCurrentHeap(mpSubHeap);
    if (mpOptionScrn->getQuitStatus() == 2) {
        mpOptionScrn->_move();
    }
    if (mpOptionScrn->getQuitStatus() == 3) {
        mProcess = 6;
    }
    mDoExt_setCurrentHeap(heap);
}

void dMenu_Collect2D_c::option_close_init() {
    setAButtonString(0);
    setBButtonString(0);
}

void dMenu_Collect2D_c::option_close_proc() {
    JKRHeap* heap = mDoExt_setCurrentHeap(mpSubHeap);
    if (mpOptionScrn->getQuitStatus() == 3) {
        mpOptionScrn->_close();
    }
    if (mpOptionScrn->getQuitStatus() == 0) {
        mProcess = 0;
        mpDrawCursor->onPlayAllAnime();
    }
    mDoExt_setCurrentHeap(heap);
}

void dMenu_Collect2D_c::letter_open_init() {
    JKRHeap* heap = mDoExt_setCurrentHeap(mpSubHeap);
    if (!mpLetterScrn) {
        mpLetterScrn = JKR_NEW dMenu_Letter_c(mpSubHeap, mpStick, mpCStick);
    }
    setAButtonString(0);
    setBButtonString(0);
    mpDrawCursor->offPlayAllAnime();
    mDoExt_setCurrentHeap(heap);
}

void dMenu_Collect2D_c::letter_open_proc() {
    JKRHeap* heap = mDoExt_setCurrentHeap(mpSubHeap);
    if (mpLetterScrn->getStatus() == 1) {
        mpLetterScrn->_open();
    }
    if (mpLetterScrn->getStatus() == 2) {
        mProcess = 8;
    }
    mDoExt_setCurrentHeap(heap);
}

void dMenu_Collect2D_c::letter_move_init() {
    setAButtonString(0x040c);
    setBButtonString(0x03f9);
}

void dMenu_Collect2D_c::letter_move_proc() {
    JKRHeap* heap = mDoExt_setCurrentHeap(mpSubHeap);
    if (mpLetterScrn->getStatus() == 2) {
        mpLetterScrn->_move();
    }
    if (mpLetterScrn->getStatus() == 3) {
        mProcess = 9;
    }
    mDoExt_setCurrentHeap(heap);
}

void dMenu_Collect2D_c::letter_close_init() {
    setAButtonString(0);
    setBButtonString(0);
}

void dMenu_Collect2D_c::letter_close_proc() {
    JKRHeap* heap = mDoExt_setCurrentHeap(mpSubHeap);
    if (mpLetterScrn->getStatus() == 3) {
        mpLetterScrn->_close();
    }
    if (mpLetterScrn->getStatus() == 0) {
        mProcess = 0;
        mpDrawCursor->onPlayAllAnime();
    }
    mDoExt_setCurrentHeap(heap);
}

void dMenu_Collect2D_c::fishing_open_init() {
    JKRHeap* heap = mDoExt_setCurrentHeap(mpSubHeap);
    if (!mpFishingScrn) {
        mpFishingScrn = JKR_NEW dMenu_Fishing_c(mpSubHeap, mpStick, mpCStick);
    }
    setAButtonString(0);
    setBButtonString(0);
    mpDrawCursor->offPlayAllAnime();
    mDoExt_setCurrentHeap(heap);
}

void dMenu_Collect2D_c::fishing_open_proc() {
    JKRHeap* heap = mDoExt_setCurrentHeap(mpSubHeap);
    if (mpFishingScrn->getStatus() == 1) {
        mpFishingScrn->_open();
    }
    if (mpFishingScrn->getStatus() == 2) {
        mProcess = 11;
    }
    mDoExt_setCurrentHeap(heap);
}

void dMenu_Collect2D_c::fishing_move_init() {
    setAButtonString(0x040c);
    setBButtonString(0x03f9);
}

void dMenu_Collect2D_c::fishing_move_proc() {
    JKRHeap* heap = mDoExt_setCurrentHeap(mpSubHeap);
    if (mpFishingScrn->getStatus() == 2) {
        mpFishingScrn->_move();
    }
    if (mpFishingScrn->getStatus() == 3) {
        mProcess = 12;
    }
    mDoExt_setCurrentHeap(heap);
}

void dMenu_Collect2D_c::fishing_close_init() {
    setAButtonString(0);
    setBButtonString(0);
}

void dMenu_Collect2D_c::fishing_close_proc() {
    JKRHeap* heap = mDoExt_setCurrentHeap(mpSubHeap);
    if (mpFishingScrn->getStatus() == 3) {
        mpFishingScrn->_close();
    }
    if (mpFishingScrn->getStatus() == 0) {
        mProcess = 0;
        mpDrawCursor->onPlayAllAnime();
    }
    mDoExt_setCurrentHeap(heap);
}

void dMenu_Collect2D_c::skill_open_init() {
    JKRHeap* heap = mDoExt_setCurrentHeap(mpSubHeap);
    if (!mpSkillScrn) {
        mpSkillScrn = JKR_NEW dMenu_Skill_c(mpSubHeap, mpStick, mpCStick);
    }
    setAButtonString(0);
    setBButtonString(0);
    mpDrawCursor->offPlayAllAnime();
    mDoExt_setCurrentHeap(heap);
}

void dMenu_Collect2D_c::skill_open_proc() {
    JKRHeap* heap = mDoExt_setCurrentHeap(mpSubHeap);
    if (mpSkillScrn->getStatus() == 1) {
        mpSkillScrn->_open();
    }
    if (mpSkillScrn->getStatus() == 2) {
        mProcess = 14;
    }
    mDoExt_setCurrentHeap(heap);
}

void dMenu_Collect2D_c::skill_move_init() {
    setAButtonString(0x040c);
    setBButtonString(0x03f9);
}

void dMenu_Collect2D_c::skill_move_proc() {
    JKRHeap* heap = mDoExt_setCurrentHeap(mpSubHeap);
    if (mpSkillScrn->getStatus() == 2) {
        mpSkillScrn->_move();
    }
    if (mpSkillScrn->getStatus() == 3) {
        mProcess = 15;
    }
    mDoExt_setCurrentHeap(heap);
}

void dMenu_Collect2D_c::skill_close_init() {
    setAButtonString(0);
    setBButtonString(0);
}

void dMenu_Collect2D_c::skill_close_proc() {
    JKRHeap* heap = mDoExt_setCurrentHeap(mpSubHeap);
    if (mpSkillScrn->getStatus() == 3) {
        mpSkillScrn->_close();
    }
    if (mpSkillScrn->getStatus() == 0) {
        mProcess = 0;
        mpDrawCursor->onPlayAllAnime();
    }
    mDoExt_setCurrentHeap(heap);
}

void dMenu_Collect2D_c::insect_open_init() {
    JKRHeap* heap = mDoExt_setCurrentHeap(mpSubHeap);
    if (!mpInsectScrn) {
        mpInsectScrn = JKR_NEW dMenu_Insect_c(mpSubHeap, mpStick, mpCStick, 0);
    }
    setAButtonString(0);
    setBButtonString(0);
    mpDrawCursor->offPlayAllAnime();
    mDoExt_setCurrentHeap(heap);
}

void dMenu_Collect2D_c::insect_open_proc() {
    JKRHeap* heap = mDoExt_setCurrentHeap(mpSubHeap);
    if (mpInsectScrn->getStatus() == 1) {
        mpInsectScrn->_open();
    }
    if (mpInsectScrn->getStatus() == 2) {
        mProcess = 17;
    }
    mDoExt_setCurrentHeap(heap);
}

void dMenu_Collect2D_c::insect_move_init() {
    setAButtonString(0x040c);
    setBButtonString(0x03f9);
}

void dMenu_Collect2D_c::insect_move_proc() {
    JKRHeap* heap = mDoExt_setCurrentHeap(mpSubHeap);
    if (mpInsectScrn->getStatus() == 2) {
        mpInsectScrn->_move();
    }
    if (mpInsectScrn->getStatus() == 3) {
        mProcess = 18;
    }
    mDoExt_setCurrentHeap(heap);
}

void dMenu_Collect2D_c::insect_close_init() {
    setAButtonString(0);
    setBButtonString(0);
}

void dMenu_Collect2D_c::insect_close_proc() {
    JKRHeap* heap = mDoExt_setCurrentHeap(mpSubHeap);
    if (mpInsectScrn->getStatus() == 3) {
        mpInsectScrn->_close();
    }
    if (mpInsectScrn->getStatus() == 0) {
        mProcess = 0;
        mpDrawCursor->onPlayAllAnime();
    }
    mDoExt_setCurrentHeap(heap);
}

void dMenu_Collect2D_c::_move() {
    u8 last_process = mProcess;
    (this->*process[last_process])();
    if (mProcess != last_process) {
        (this->*init[mProcess])();
    }
#if !TARGET_PC
    btkAnimeLoop0(mpAnmKey);
    mpScreen->animation();
    setBackAlpha();
#endif
    setHIO(false);
}


void dMenu_Collect2D_c::_draw() {
#if TARGET_PC
    btkAnimeLoop0(mpAnmKey);
    mpScreen->animation();
    setBackAlpha();
    menuCollectWide();
#endif

    J2DGrafContext* grafPort = dComIfGp_getCurrentGrafPort();
    grafPort->setup2D();
#if TARGET_PC
    if (collectRevo()) {
        hdEquipFrames();
        {
            static int s_blink = 0;
            s_blink = (s_blink + 1) % 60;
            const f32 k = s_blink < 30 ? s_blink / 30.0f : (60 - s_blink) / 30.0f;
            const bool onSave = s_hdY == 4 && s_hdX == 0;
            const bool onOpt = s_hdY == 4 && s_hdX == 8;
            if (J2DPane* p = mpScreen->search(MULTI_CHAR('wi_save1'))) {
                p->setAlpha(onSave ? static_cast<u8>(255.0f * k) : 0);
            }
            if (J2DPane* p = mpScreen->search(MULTI_CHAR('wi_opti1'))) {
                p->setAlpha(onOpt ? static_cast<u8>(255.0f * k) : 0);
            }
        }
        // The heart container art is not stretched on the Wii U (same height, 3/4 width).
        mpHeartParent->scale(g_drawHIO.mCollectScreen.mHeartVesselScale / collectRevoScaleX(),
                             g_drawHIO.mCollectScreen.mHeartVesselScale);
        // Item icons keep their shape too (only their frames are stretched): counter the stretch
        // on every item cell, keeping the selected / unselected scale set by cursorPosSet.
        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 9; x++) {
                const char* t = kHdGrid[y][x];
                if (t == NULL || (y == 1 && (x == 1 || x == 8))) {
                    continue;  // heart (handled above) and the 3D Fused Shadow / Mirror
                }
                if (J2DPane* cell = mpScreen->search(hdTag8(t))) {
                    const f32 s = cell->getScaleY();
                    cell->scale(s / collectRevoScaleX(), s);
                }
            }
        }
        const f32 invSx = 1.0f / collectRevoScaleX();
        for (u64 tag : {MULTI_CHAR('op_tex_n'), MULTI_CHAR('sa_tex_n'),
                        MULTI_CHAR('itemn_n'), MULTI_CHAR('infotxtn')}) {
            if (J2DPane* p = mpScreen->search(tag)) {
                p->scale(invSx, 1.0f);
            } else if (J2DPane* p = mpScreen->searchUserInfo(tag)) {
                p->scale(invSx, 1.0f);
            }
        }
        for (u64 tag : {MULTI_CHAR('t_t00'), MULTI_CHAR('f_t00')}) {
            if (J2DPane* p = mpScreen->search(tag)) {
                p->scale(invSx, 1.0f);
            } else if (J2DPane* p = mpScreen->searchUserInfo(tag)) {
                p->scale(invSx, 1.0f);
            }
        }
        mpScreen->scale(collectRevoScaleX(), 1.0f);
        mpScreen->draw(mDoGph_gInf_c::getMinXF(), 0.0f, grafPort);
    } else {
        mpScreen->draw(0.0f, 0.0f, grafPort);
    }
#else
    mpScreen->draw(0.0f, 0.0f, grafPort);
#endif

    if (mItemNameString == 0) {
#if TARGET_PC
        TEXT_SPAN stringPtr1;
        if (dusk::version::isRegionJpn()) {
            stringPtr1 = static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('i_text1')))->getStringPtr();
        } else {
            stringPtr1 = static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('f_text1')))->getStringPtr();
        }
#elif REGION_JPN
        TEXT_SPAN stringPtr1 = static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('i_text1')))->getStringPtr();
#else
        TEXT_SPAN stringPtr1 = static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('f_text1')))->getStringPtr();
#endif
        SAFE_STRCPY(stringPtr1, "");

#if TARGET_PC
        TEXT_SPAN stringPtr0;
        if (dusk::version::isRegionJpn()) {
            stringPtr0 = static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('i_text0')))->getStringPtr();
        } else {
            stringPtr0 = static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('f_text0')))->getStringPtr();
        }
#elif REGION_JPN
        TEXT_SPAN stringPtr0 = static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('i_text0')))->getStringPtr();
#else
        TEXT_SPAN stringPtr0 = static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('f_text0')))->getStringPtr();
#endif
        SAFE_STRCPY(stringPtr0, "");
    } else {
#if TARGET_PC
        J2DTextBox* textBox1;
        if (dusk::version::isRegionJpn()) {
            textBox1 = static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('i_text1')));
        } else {
            textBox1 = static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('f_text1')));
        }
#elif REGION_JPN
        J2DTextBox* textBox1 = static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('i_text1')));
#else
        J2DTextBox* textBox1 = static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('f_text1')));
#endif
        mpString->getString(mItemNameString, textBox1, NULL, NULL, NULL, 0);

#if TARGET_PC
        J2DTextBox* textBox0;
        if (dusk::version::isRegionJpn()) {
            textBox0 = static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('i_text0')));
        } else {
            textBox0 = static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('f_text0')));
        }
#elif REGION_JPN
        J2DTextBox* textBox0 = static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('i_text0')));
#else
        J2DTextBox* textBox0 = static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('f_text0')));
#endif
        mpString->getString(mItemNameString, textBox0, NULL, NULL, NULL, 0);
#if TARGET_PC
        if (collectRevo() && mItemNameString == 0xaf1) {
            // TPHD 0x029b6814: "... Stamps found: %d/%d" gets the number of stamps / 50.
            const int num = dMenu_Stamp_c::getStampNum();
            for (J2DTextBox* tb : {textBox1, textBox0}) {
                if (tb == NULL) {
                    continue;
                }
                char fmt[512];
                char buf[512];
                SAFE_STRCPY(fmt, (const char*)tb->getStringPtr());
                snprintf(buf, sizeof(buf), fmt, num, (int)dMenu_Stamp_c::STAMP_MAX);
                SAFE_STRCPY(tb->getStringPtr(), buf);
            }
        }
#endif

#if TARGET_PC
        if (dusk::version::isRegionJpn()) {
            textBox0 = static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('i_text0')));
        } else {
            textBox0 = static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('f_text0')));
        }
#elif REGION_JPN
        textBox0 = static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('i_text0')));
#else
        textBox0 = static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('f_text0')));
#endif
        mpString->drawOutFontLocal(textBox0, -1.0f);
    }
#if TARGET_PC
    if (collectRevo() && mpSelPm[mCursorX][mCursorY] != NULL) {
        // The cursor follows the drawn (stretched) cell: its global vertices include the screen
        // scale and offset, which CPaneMgr::getGlobalVtxCenter (dSelect_cursor_c::update) does not.
        J2DPane* cell = mpSelPm[mCursorX][mCursorY]->getPanePtr();
        const Vec a = cell->getGlbVtx(0);
        const Vec b = cell->getGlbVtx(3);
        // RPX 0x029b2748: the cursor sits at the cell centre + (-1.75, -1.5) on the 608-unit
        // canvas, and its screen is stretched like the collection screen.
        const f32 sx = collectRevoScaleX();
        const f32 minX = mDoGph_gInf_c::getMinXF();
        // The offset is applied before the un-stretch (Wii U setPos 0x02aad0ec maps the
        // stretched x back), so it is -1.75 drawn units.
        const f32 cx = ((a.x + b.x) * 0.5f - 1.75f - minX) / sx;
        const f32 cy = (a.y + b.y) * 0.5f - 1.5f;
        // Cursor box = pane size x its own scale, drawn on the stretched cursor screen. The item
        // cells already carry the 1/sx counter-stretch; the heart and the Fused Shadow / Mirror
        // ('n_43' groups the Wii U keeps 1:1) do not, so their box would come out sx times too
        // wide: give the cursor their unstretched width.
        const bool unstretched = cell->mInfoTag == MULTI_CHAR('heart_kn') ||
                                 cell->mInfoTag == MULTI_CHAR('kamen_n');
        const f32 keepSX = cell->getScaleX();
        if (unstretched) {
            cell->scale(keepSX / sx, cell->getScaleY());
        }
        mpDrawCursor->setPos(cx, cy, cell, true);
        if (unstretched) {
            cell->scale(keepSX, cell->getScaleY());
        }
        mpDrawCursor->setPos(cx, cy, NULL, false);
        mpDrawCursor->setDrawStretch(sx, minX);
        {
            static J2DPane* s_logCell = NULL;
            if (cell != s_logCell) {
                s_logCell = cell;
                char tag[9] = {};
                for (int k = 0, n = 0; k < 8; k++) {
                    const char c = static_cast<char>((cell->mInfoTag >> (56 - k * 8)) & 0xFF);
                    if (c != 0) {
                        tag[n++] = c;
                    }
                }
            }
        }
    }
#endif
    mpDrawCursor->draw();
}

void dMenu_Collect2D_c::drawTop() {
    dComIfGd_set2DOpaTop(mpDraw2DTop);
}

bool dMenu_Collect2D_c::isKeyCheck() {
    if (mProcess || field_0x25f) {
        return true;
    }
    return false;
}

bool dMenu_Collect2D_c::isOutCheck() {
    return false;
}

void dMenu_Collect2D_c::setAButtonString(u16 i_stringID) {
#if TARGET_PC
    // TPHD 0x029b20e0 / 0x029b22cc: the badge is grey (white = 0x808080) while the button has
    // nothing to do, white otherwise.
    if (collectRevo()) {
        J2DPane* badge = mpScreenIcon->search(MULTI_CHAR('a_btn_cp'));
        if (badge != NULL && badge->getTypeID() == 18) {
            static_cast<J2DPicture*>(badge)->setBlackWhite(
                JUtility::TColor(0, 0, 0, 0),
                i_stringID != 0 ? JUtility::TColor(255, 255, 255, 255) : JUtility::TColor(128, 128, 128, 255));
        }
    }
#endif
    static const u64 text_a_tag[5] = {
        MULTI_CHAR('atext1_1'), MULTI_CHAR('atext1_2'), MULTI_CHAR('atext1_3'), MULTI_CHAR('atext1_4'), MULTI_CHAR('atext1_5'),
    };

    if (i_stringID != mCurrentAString) {
        mCurrentAString = i_stringID;

        if (i_stringID == 0) {
            for (int i = 0; i < 5; i++) {
#if TARGET_PC
                J2DTextBox* tb = collectIconText(mpScreenIcon, text_a_tag[i]);
                if (tb == NULL) {
                    continue;
                }
                TEXT_SPAN stringPtr = tb->getStringPtr();
#else
                TEXT_SPAN stringPtr =
                    static_cast<J2DTextBox*>(mpScreenIcon->search(text_a_tag[i]))->getStringPtr();
#endif
                SAFE_STRCPY(stringPtr, "");
            }
        } else {
            for (int i = 0; i < 5; i++) {
#if TARGET_PC
                J2DTextBox* tb = collectIconText(mpScreenIcon, text_a_tag[i]);
                if (tb == NULL) {
                    continue;
                }
                TEXT_SPAN stringPtr = tb->getStringPtr();
#else
                TEXT_SPAN stringPtr =
                    static_cast<J2DTextBox*>(mpScreenIcon->search(text_a_tag[i]))->getStringPtr();
#endif
                dMeter2Info_getStringKanji(i_stringID, stringPtr, NULL);
            }
        }
    }
}

void dMenu_Collect2D_c::setBButtonString(u16 i_stringID) {
#if TARGET_PC
    // TPHD 0x029b20e0 / 0x029b22cc: the badge is grey (white = 0x808080) while the button has
    // nothing to do, white otherwise.
    if (collectRevo()) {
        J2DPane* badge = mpScreenIcon->search(MULTI_CHAR('b_btn_cp'));
        if (badge != NULL && badge->getTypeID() == 18) {
            static_cast<J2DPicture*>(badge)->setBlackWhite(
                JUtility::TColor(0, 0, 0, 0),
                i_stringID != 0 ? JUtility::TColor(255, 255, 255, 255) : JUtility::TColor(128, 128, 128, 255));
        }
    }
#endif
    static const u64 text_b_tag[5] = {
        MULTI_CHAR('btext1_1'), MULTI_CHAR('btext1_2'), MULTI_CHAR('btext1_3'), MULTI_CHAR('btext1_4'), MULTI_CHAR('btext1_5'),
    };

    if (i_stringID != mCurrentBString) {
        mCurrentBString = i_stringID;

        if (i_stringID == 0) {
            for (int i = 0; i < 5; i++) {
#if TARGET_PC
                J2DTextBox* tb = collectIconText(mpScreenIcon, text_b_tag[i]);
                if (tb == NULL) {
                    continue;
                }
                TEXT_SPAN stringPtr = tb->getStringPtr();
#else
                TEXT_SPAN stringPtr =
                    static_cast<J2DTextBox*>(mpScreenIcon->search(text_b_tag[i]))->getStringPtr();
#endif
                SAFE_STRCPY(stringPtr, "");
            }
        } else {
            for (int i = 0; i < 5; i++) {
#if TARGET_PC
                J2DTextBox* tb = collectIconText(mpScreenIcon, text_b_tag[i]);
                if (tb == NULL) {
                    continue;
                }
                TEXT_SPAN stringPtr = tb->getStringPtr();
#else
                TEXT_SPAN stringPtr =
                    static_cast<J2DTextBox*>(mpScreenIcon->search(text_b_tag[i]))->getStringPtr();
#endif
                dMeter2Info_getStringKanji(i_stringID, stringPtr, NULL);
            }
        }
    }
}

void dMenu_Collect2D_c::setItemNameString(u8 param_0, u8 param_1) {
    if (field_0x22d[param_0][param_1] == 0) {
        setItemNameStringNull();
    } else {
        u32 uVar6 = field_0x184[param_0][param_1];
        mItemNameString = field_0x1d8[param_0][param_1];
        field_0x180 = uVar6;
        if (uVar6 == 0) {
            setItemNameStringNull();
        } else {
#if TARGET_PC
            if (dusk::version::isRegionJpn()) {
                TEXT_SPAN stringPtr =
                static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n00')))->getStringPtr();
                dMeter2Info_getStringKanji(uVar6, stringPtr, NULL);
                stringPtr = static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n01')))->getStringPtr();
                dMeter2Info_getStringKanji(uVar6, stringPtr, NULL);
                stringPtr = static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n02')))->getStringPtr();
                dMeter2Info_getStringKanji(uVar6, stringPtr, NULL);
                stringPtr = static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n03')))->getStringPtr();
                dMeter2Info_getStringKanji(uVar6, stringPtr, NULL);
            } else {
                TEXT_SPAN stringPtr =
                static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n04')))->getStringPtr();
                dMeter2Info_getStringKanji(uVar6, stringPtr, NULL);
                stringPtr = static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n05')))->getStringPtr();
                dMeter2Info_getStringKanji(uVar6, stringPtr, NULL);
                stringPtr = static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n06')))->getStringPtr();
                dMeter2Info_getStringKanji(uVar6, stringPtr, NULL);
                stringPtr = static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n07')))->getStringPtr();
                dMeter2Info_getStringKanji(uVar6, stringPtr, NULL);
            }
#elif REGION_JPN
            TEXT_SPAN stringPtr =
                static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n00')))->getStringPtr();
            dMeter2Info_getStringKanji(uVar6, stringPtr, NULL);
            stringPtr = static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n01')))->getStringPtr();
            dMeter2Info_getStringKanji(uVar6, stringPtr, NULL);
            stringPtr = static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n02')))->getStringPtr();
            dMeter2Info_getStringKanji(uVar6, stringPtr, NULL);
            stringPtr = static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n03')))->getStringPtr();
            dMeter2Info_getStringKanji(uVar6, stringPtr, NULL);
#else
            TEXT_SPAN stringPtr =
                static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n04')))->getStringPtr();
            dMeter2Info_getStringKanji(uVar6, stringPtr, NULL);
            stringPtr = static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n05')))->getStringPtr();
            dMeter2Info_getStringKanji(uVar6, stringPtr, NULL);
            stringPtr = static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n06')))->getStringPtr();
            dMeter2Info_getStringKanji(uVar6, stringPtr, NULL);
            stringPtr = static_cast<J2DTextBox*>(mpScreen->search(MULTI_CHAR('item_n07')))->getStringPtr();
            dMeter2Info_getStringKanji(uVar6, stringPtr, NULL);
#endif
        }
    }
}

void dMenu_Collect2D_c::setItemNameStringNull() {
    mItemNameString = 0;
#if TARGET_PC
    J2DTextBox* textBox;
    if (dusk::version::isRegionJpn()) {
        textBox = (J2DTextBox*)mpScreen->search(MULTI_CHAR('item_n00'));
        SAFE_STRCPY(textBox->getStringPtr(), "");
        textBox = (J2DTextBox*)mpScreen->search(MULTI_CHAR('item_n01'));
        SAFE_STRCPY(textBox->getStringPtr(), "");
        textBox = (J2DTextBox*)mpScreen->search(MULTI_CHAR('item_n02'));
        SAFE_STRCPY(textBox->getStringPtr(), "");
        textBox = (J2DTextBox*)mpScreen->search(MULTI_CHAR('item_n03'));
    } else {
        textBox = (J2DTextBox*)mpScreen->search(MULTI_CHAR('item_n04'));
        SAFE_STRCPY(textBox->getStringPtr(), "");
        textBox = (J2DTextBox*)mpScreen->search(MULTI_CHAR('item_n05'));
        SAFE_STRCPY(textBox->getStringPtr(), "");
        textBox = (J2DTextBox*)mpScreen->search(MULTI_CHAR('item_n06'));
        SAFE_STRCPY(textBox->getStringPtr(), "");
        textBox = (J2DTextBox*)mpScreen->search(MULTI_CHAR('item_n07'));
    }
#elif REGION_JPN
    J2DTextBox* textBox = (J2DTextBox*)mpScreen->search(MULTI_CHAR('item_n00'));
    SAFE_STRCPY(textBox->getStringPtr(), "");
    textBox = (J2DTextBox*)mpScreen->search(MULTI_CHAR('item_n01'));
    SAFE_STRCPY(textBox->getStringPtr(), "");
    textBox = (J2DTextBox*)mpScreen->search(MULTI_CHAR('item_n02'));
    SAFE_STRCPY(textBox->getStringPtr(), "");
    textBox = (J2DTextBox*)mpScreen->search(MULTI_CHAR('item_n03'));
#else
    J2DTextBox* textBox = (J2DTextBox*)mpScreen->search(MULTI_CHAR('item_n04'));
    SAFE_STRCPY(textBox->getStringPtr(), "");
    textBox = (J2DTextBox*)mpScreen->search(MULTI_CHAR('item_n05'));
    SAFE_STRCPY(textBox->getStringPtr(), "");
    textBox = (J2DTextBox*)mpScreen->search(MULTI_CHAR('item_n06'));
    SAFE_STRCPY(textBox->getStringPtr(), "");
    textBox = (J2DTextBox*)mpScreen->search(MULTI_CHAR('item_n07'));
#endif
    SAFE_STRCPY(textBox->getStringPtr(), "");
}

dMenu_Collect3D_c::dMenu_Collect3D_c(JKRExpHeap* param_0, dMenu_Collect2D_c* param_1,
                                     CSTControl* param_2) {
    mIsWolf = daPy_py_c::checkNowWolf();
    mpHeap = param_0;
    mpSolidHeap = NULL;
    mpCStick = param_2;
    mpCollect2D = param_1;
    if (mIsWolf) {
        mLinkAngle = -20748;
    } else {
        mLinkAngle = -23324;
    }
#if TARGET_PC
    if (collectRevo()) {
        // TPHD ctor 0x029b9ab4: Link starts at the HIO angle (0x8eb8 / wolf 0x9e58), so he does
        // not spin into place when the menu opens.
        mLinkAngle = mIsWolf ? (s16)0x9E58 : (s16)0x8EB8;
    }
#endif
    mMaskMirrorAnmFrameBrk = 0.0f;
    mMaskMirrorAnmFrameBck = 0.0f;
    mMaskMirrorOffsetY = 0.0f;
    mMaskMirrorOffsetX = 0.0f;
    mMaskMirrorScale = 1.0f;
}


dMenu_Collect3D_c::~dMenu_Collect3D_c() {
    if (mpSolidHeap) {
        mDoExt_destroySolidHeap(mpSolidHeap);
        mpSolidHeap = NULL;
    }
}


void dMenu_Collect3D_c::_create() {
    mpHeap->getTotalFreeSize();
    mpSolidHeap = mDoExt_createSolidHeapToCurrent(0x25800, mpHeap, 0x20);
    JKRHEAP_NAME(mpSolidHeap, "dMenu_Collect3D_c::mpSolidHeap");
    mDoExt_setCurrentHeap((JKRHeap*)mpSolidHeap);
    daAlink_c* linkActor = daAlink_getAlinkActorClass();
    if (linkActor != NULL) {
        linkActor->initStatusWindow();
    }
    u8 maskMdl = getMaskMdlVisible();
    if (maskMdl == 2) {
        createMirrorModel();
    } else if (maskMdl == 1) {
        createMaskModel();
    } else {
        mpModel = NULL;
        mpBckAnm = NULL;
        mpBrkAnm = NULL;
    }
    mpSolidHeap->adjustSize();
    mDoExt_restoreCurrentHeap();
    if (mpModel != NULL) {
        dKy_tevstr_init(&mTevStr, -1, 0xff);
        set_mtx();
    }
    _move(mpCollect2D->getCursorX(), mpCollect2D->getCursorY());
}

void dMenu_Collect3D_c::_delete() {
    if (daAlink_getAlinkActorClass()) {
        daAlink_getAlinkActorClass()->resetStatusWindow();
    }
}

void dMenu_Collect3D_c::_move(u8 param_0, u8 param_1) {
    cXyz itemPos;
    Vec linkPos = mpCollect2D->getpLink()->getGlobalVtxCenter(false, 0);
    f32 posY;
    f32 posZ;
#if TARGET_PC
    const bool revo = collectRevo();
    if (revo) {
        // TPHD 0x029ba9bc + HIO defaults 0x029bc8d4: Link sits on linki_n (608-unit canvas) plus
        // (0, 90) and the HIO offset (0, 30), depth 400; wolf: (0, 60), (-15, 30), 400. The extra
        // (+-1, -3) of 0x029ba9bc only applies when the byte at 0x1012667d is 0, i.e. in the
        // mirrored Hero Mode (0x029ba694 then flips the view matrix X), so it is not used here.
        // The global vertices include the screen stretch; the Wii U works on the unstretched canvas.
        const f32 lx = linkPos.x / collectRevoScaleX();
        if (mIsWolf != 0) {
            linkPos.x = lx - 15.0f;
            posY = linkPos.y + 60.0f + 30.0f;
        } else {
            linkPos.x = lx;
            posY = linkPos.y + 90.0f + 30.0f;
        }
        posZ = 400.0f;
    } else
#endif
    if (mIsWolf != 0) {
        posY = linkPos.y + 60.0f;
        posZ = 580.0f;
    } else {
        posY = linkPos.y + 90.0f;
        posZ = 550.0f;
    }
    toItem3Dpos(linkPos.x, posY, posZ, &itemPos);

#if TARGET_PC
    if (dusk::getSettings().game.enableLinkDollRotation) {
        const f32 angle = mDoCPd_c::getSubStickX3D(PAD_1) * 2048;
        ANGLE_ADD(mLinkAngle, angle);
    } else
#endif
#if TARGET_PC
    if (revo) {
        // TPHD 0x029ba9bc: no idle spin; Link turns to the HIO angle 0x8eb8 (wolf 0x9e58). The
        // -0x27ad / -0x423d correction and the negation are the mirrored Hero Mode branch.
        const s16 target = mIsWolf != 0 ? (s16)0x9E58 : (s16)0x8EB8;
        cLib_addCalcAngleS(&mLinkAngle, target, 4, 0x800, 0x80);
    } else
#endif
    if (param_0 == 0 && param_1 == 0) {
        f32 temp = 450.0f;
        ANGLE_ADD(mLinkAngle, temp);
    } else {
        s16 target = mIsWolf != 0 ? (s16)-0x510C : (s16)-0x5B1C;
        cLib_addCalcAngleS(&mLinkAngle, target, 4, 0x800, 0x80);
    }
    if (daAlink_getAlinkActorClass() != NULL) {
        daAlink_getAlinkActorClass()->statusWindowExecute(&itemPos, mLinkAngle);
    }
    if (mpModel != NULL) {
        cXyz modelPos;
        Vec maskPos = mpCollect2D->getpMask()->getGlobalVtxCenter(false, 0);
#if TARGET_PC
        if (revo) {
            maskPos.x /= collectRevoScaleX();  // unstretched canvas, see above
        }
#endif
        f32 offsetX = 0.0f;
        f32 offsetY = 0.0f;

#if TARGET_PC
        // TPHD 0x029ba9bc places the Fused Shadow / Mirror with only the per-count table offset
        // (mMaskMirrorOffsetX / Y) and the HIO position: no extra per-count nudge.
        if (revo) {
        } else
#endif
        if (getMaskMdlVisible() == 2) {
            u8 mirrorNum = getMirrorNum();
            if (mirrorNum == 1 || mirrorNum == 2 || mirrorNum == 3 || mirrorNum == 4) {
                offsetX = -3.0f;
                offsetY = -5.0f;
            }
        } else {
            u8 crystalNum = getCrystalNum();
            if (crystalNum == 1) {
                offsetX = 16.0f;
                offsetY = 0.0f;
            } else if (crystalNum == 2) {
                offsetX = 8.0f;
                offsetY = -3.0f;
            } else if (crystalNum == 3) {
                offsetX = 2.0f;
                offsetY = -4.0f;
            } else if (crystalNum == 4) {
                offsetX = -5.0f;
                offsetY = -10.0f;
            }
        }
        f32 offsetPosX =
            maskPos.x + g_drawHIO.mCollectScreen.mMaskMirrorPos.x + mMaskMirrorOffsetX + offsetX;
        f32 offsetPosY =
            maskPos.y + g_drawHIO.mCollectScreen.mMaskMirrorPos.y + mMaskMirrorOffsetY + offsetY;
        toItem3Dpos(offsetPosX, offsetPosY, 600.0f, &modelPos);
        mMaskMirrorPos.set(modelPos);
        mMaskMirrorAngle.set(g_drawHIO.mCollectScreen.mMaskMirrorAngle.x,
                        g_drawHIO.mCollectScreen.mMaskMirrorAngle.y,
                        g_drawHIO.mCollectScreen.mMaskMirrorAngle.z);
        animePlay();
        set_mtx();
    }
}

void dMenu_Collect3D_c::draw() {
    dComIfGd_setListItem3D();
    if (daAlink_getAlinkActorClass()) {
        daAlink_getAlinkActorClass()->statusWindowDraw();
    }
    if (mpModel) {
        g_env_light.settingTevStruct(13, &mMaskMirrorPos, &mTevStr);
        g_env_light.setLightTevColorType_MAJI(mpModel, &mTevStr);
        animeEntry();
        mDoExt_modelUpdateDL(mpModel);
    }
    dComIfGd_setList();
}

void dMenu_Collect3D_c::setJ3D(const char* param_0, const char* param_1, const char* param_2) {
    JKRArchive* resArchive = dComIfGp_getCollectResArchive();
    J3DModelData* modelData = J3DModelLoaderDataBase::load(
        resArchive->getResource('BMD ', param_0), 0x51020010);

    for (u16 i = 0; i < modelData->getMaterialNum(); i++) {
        J3DMaterialAnm* matAnm = JKR_NEW J3DMaterialAnm();
        modelData->getMaterialNodePointer(i)->change();
        modelData->getMaterialNodePointer(i)->setMaterialAnm(matAnm);
    }
    mpModel = JKR_NEW J3DModel(modelData, 0, 1);
    if (param_1 != NULL) {
        J3DAnmTransform* pbck = (J3DAnmTransform*)J3DAnmLoaderDataBase::load(
            resArchive->getResource('BCK ', param_1), J3DLOADER_UNK_FLAG0);
        mpBckAnm = JKR_NEW mDoExt_bckAnm();
        if (mpBckAnm == NULL || !mpBckAnm->init(pbck, 1, 2, 1.0f, 0, -1, false)) {
            return;
        }
    }
    if (param_2 != NULL) {
        J3DAnmTevRegKey* pbrk = (J3DAnmTevRegKey*)J3DAnmLoaderDataBase::load(
            resArchive->getResource('BRK ', param_2), J3DLOADER_UNK_FLAG0);
        pbrk->searchUpdateMaterialID(modelData);
        mpBrkAnm = JKR_NEW mDoExt_brkAnm();
        if (mpBrkAnm == NULL || !mpBrkAnm->init(modelData, pbrk, -1, 2, 1.0f, 0, -1)) {
            return;
        }
    }
}

void dMenu_Collect3D_c::set_mtx() {
    f32 scale = 0.0f;
#if TARGET_PC
    // TPHD set_mtx (0x029ba5f8): base scale = table scale + HIO scale only (no per-count shrink,
    // so 1-2 Fused Shadows are 2.6 instead of the GC 2.0).
    if (collectRevo()) {
    } else
#endif
    if (getMaskMdlVisible() == 2) {
        u8 mirrorNum = getMirrorNum();
        if (mirrorNum == 2 || mirrorNum == 3 || mirrorNum == 4) {
            scale = -0.1f;
        }
    } else {
        u8 crystalNum = getCrystalNum();
        if (crystalNum == 1 || crystalNum == 2) {
            scale = -0.6f;
        } else if (crystalNum == 3) {
            scale = -0.4f;
        } else if (crystalNum == 4) {
            scale = -0.3f;
        }
    }
    f32 finalScale = g_drawHIO.mCollectScreen.mMaskMirrorScale + scale + mMaskMirrorScale;
    cXyz baseScale;
    baseScale.z = finalScale;
    baseScale.y = finalScale;
    baseScale.x = finalScale;
    mpModel->setBaseScale(baseScale);
    mDoMtx_stack_c::transS(mMaskMirrorPos.x, mMaskMirrorPos.y, mMaskMirrorPos.z);
    mDoMtx_stack_c::YrotM(mMaskMirrorAngle.y);
    mDoMtx_stack_c::XrotM(mMaskMirrorAngle.x);
    mDoMtx_stack_c::ZrotM(mMaskMirrorAngle.z);
    mpModel->setBaseTRMtx(mDoMtx_stack_c::get());
}


void dMenu_Collect3D_c::animePlay() {
    if (mpBrkAnm != NULL) {
        mMaskMirrorAnmFrameBrk += g_drawHIO.mCollectScreen.mMaskMirrorAnimSpeed;
        if (mMaskMirrorAnmFrameBrk >= mpBrkAnm->getEndFrame()) {
            mMaskMirrorAnmFrameBrk -= mpBrkAnm->getEndFrame();
        }
        mpBrkAnm->setFrame(mMaskMirrorAnmFrameBrk);
        mpBrkAnm->play();
    }
    if (mpBckAnm != NULL) {
        mMaskMirrorAnmFrameBck += g_drawHIO.mCollectScreen.mMaskMirrorAnimSpeed;
        if (mMaskMirrorAnmFrameBck >= mpBckAnm->getEndFrame()) {
            mMaskMirrorAnmFrameBck -= mpBckAnm->getEndFrame();
        }
        mpBckAnm->setFrame(mMaskMirrorAnmFrameBck);
        mpBckAnm->play();
    }
}


void dMenu_Collect3D_c::animeEntry() {
    if (mpBrkAnm != NULL) {
        mpBrkAnm->entry(mpModel->getModelData());
    }
    if (mpBckAnm != NULL) {
        mpBckAnm->entry(mpModel->getModelData());
    }
}

void dMenu_Collect3D_c::createMaskModel() {
    static const f32 m_kamen_offset_x[5] = {-14.0f, -14.0f, -14.0f, 1.3f, 6.5f};
    static const f32 m_kamen_offset_y[5] = {0.0f, 0.0f, 0.0f, 22.0f, 30.0f};
    static const f32 m_kamen_scale[5] = {2.6f, 2.6f, 2.2f, 1.8f, 1.8f};
    static DUSK_CONSTEXPR char DUSK_CONST* bck_name[4] = {
        "md_mask_parts_spin_1.bck",
        "md_mask_parts_spin_2.bck",
        "md_mask_parts_spin_3.bck",
        "md_mask_parts_spin_4.bck",
    };
    static DUSK_CONSTEXPR char DUSK_CONST* brk_name[4] = {
        "md_mask_parts_spin_1.brk",
        "md_mask_parts_spin_2_3.brk",
        "md_mask_parts_spin_2_3.brk",
        "md_mask_parts_spin_4.brk",
    };

    u8 crystalNum = getCrystalNum();
    mMaskMirrorOffsetX = m_kamen_offset_x[crystalNum];
    mMaskMirrorOffsetY = m_kamen_offset_y[crystalNum];
    mMaskMirrorScale = m_kamen_scale[crystalNum];
    mMaskMirrorPos.set(0.0f, 0.0f, 0.0f);
    mMaskMirrorAngle.set(0, 0, 0);
    mpModel = NULL;
    mpBckAnm = NULL;
    mpBrkAnm = NULL;
    if (crystalNum != 0) {
        setJ3D("md_mask_UI.bmd", bck_name[crystalNum - 1], brk_name[crystalNum - 1]);
        switch (crystalNum) {
        case 1:
            mpModel->getModelData()->getMaterialNodePointer(0)->getShape()->hide();
            mpModel->getModelData()->getMaterialNodePointer(1)->getShape()->hide();
        case 2:
            mpModel->getModelData()->getMaterialNodePointer(2)->getShape()->hide();
            mpModel->getModelData()->getMaterialNodePointer(3)->getShape()->hide();
        case 3:
            mpModel->getModelData()->getMaterialNodePointer(6)->getShape()->hide();
            mpModel->getModelData()->getMaterialNodePointer(7)->getShape()->hide();
            break;
        }
    }
}

void dMenu_Collect3D_c::createMirrorModel() {
    static const f32 m_mirror_offset_x[5] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    static const f32 m_mirror_offset_y[5] = {4.0f, 4.0f, 4.0f, 4.0f, 4.0f};
    static const f32 m_mirror_scale[5] = {0.6f, 0.6f, 0.6f, 0.6f, 0.6f};
    static DUSK_CONSTEXPR char DUSK_CONST* bck_name[4] = {
        "kageri_mirrer_spin_1.bck",
        "kageri_mirrer_spin_2.bck",
        "kageri_mirrer_spin_3.bck",
        "kageri_mirrer_spin_4.bck",
    };
    static DUSK_CONSTEXPR char DUSK_CONST* brk_name[4] = {
        "kageri_mirrer_spin_1.brk",
        "kageri_mirrer_spin_2_3_4.brk",
        "kageri_mirrer_spin_2_3_4.brk",
        "kageri_mirrer_spin_2_3_4.brk",
    };

    u8 mirrorNum = getMirrorNum();
    mMaskMirrorOffsetX = m_mirror_offset_x[mirrorNum];
    mMaskMirrorOffsetY = m_mirror_offset_y[mirrorNum];
    mMaskMirrorScale = m_mirror_scale[mirrorNum];
    mMaskMirrorPos.set(0.0f, 0.0f, 0.0f);
    mMaskMirrorAngle.set(0, 0, 0);
    mpModel = NULL;
    mpBckAnm = NULL;
    mpBrkAnm = NULL;
    if (mirrorNum != 0) {
        setJ3D("kageri_mirrer_UI.bmd", bck_name[mirrorNum - 1], brk_name[mirrorNum - 1]);
        switch (mirrorNum) {
        case 1:
            mpModel->getModelData()->getMaterialNodePointer(4)->getShape()->hide();
            mpModel->getModelData()->getMaterialNodePointer(5)->getShape()->hide();
            mpModel->getModelData()->getMaterialNodePointer(6)->getShape()->hide();
            mpModel->getModelData()->getMaterialNodePointer(7)->getShape()->hide();
        case 2:
            mpModel->getModelData()->getMaterialNodePointer(8)->getShape()->hide();
            mpModel->getModelData()->getMaterialNodePointer(9)->getShape()->hide();
            mpModel->getModelData()->getMaterialNodePointer(10)->getShape()->hide();
            mpModel->getModelData()->getMaterialNodePointer(11)->getShape()->hide();
        case 3:
            mpModel->getModelData()->getMaterialNodePointer(12)->getShape()->hide();
            mpModel->getModelData()->getMaterialNodePointer(13)->getShape()->hide();
            mpModel->getModelData()->getMaterialNodePointer(14)->getShape()->hide();
            mpModel->getModelData()->getMaterialNodePointer(15)->getShape()->hide();
            break;
        }
    }
}

u8 dMenu_Collect3D_c::getCrystalNum() {
    u8 count = 0;
    for (int i = 0; i < 4; i++) {
        if (!dComIfGs_isCollectCrystal(i)) {
            break;
        }
        count++;
    }
    return count;
}

u8 dMenu_Collect3D_c::getMirrorNum() {
    u8 count = 0;
    for (int i = 0; i < 4; i++) {
        if (!dComIfGs_isCollectMirror(i)) {
            break;
        }
        count++;
    }
    return count;
}

u8 dMenu_Collect3D_c::getMaskMdlVisible() {
    if ((getCrystalNum() == 0 && getMirrorNum() == 0) ||
         /* dSv_event_flag_c::M_071 - Cutscene - [cutscene: 20] Zant appears (during Midna's desperate hour) */
        (dComIfGs_isEventBit(0xC01) && !dComIfGs_isCollectMirror(0)) ||
         /* dSv_event_flag_c::F_0354 - Cutscene - [cutscene] Mirror complete */
        (dComIfGs_isEventBit(0x2B08) && !dComIfGs_isCollectCrystal(3)))
    {
        return 0;
    }
    if (dComIfGs_isCollectCrystal(3) || getMirrorNum() == 0) {
        return 1;
    }
    return 2;
}

DUSK_GAME_DATA f32 dMenu_Collect3D_c::mViewOffsetY = -100.0f;

void dMenu_Collect3D_c::setupItem3D(Mtx param_0) {
#if TARGET_PC
    if (collectRevo()) {
        // TPHD 0x029bb124: full viewport (no -100 offset, so Link's feet are not cut) and a
        // 608 / 448 projection that the Wii U scales to the TV *together with the framebuffer*.
        // Here we render straight to the real framebuffer, so the projection must use the real
        // aspect or the models get stretched horizontally.
        GXSetViewport(0.0f, 0.0f, FB_WIDTH, FB_HEIGHT, 0.0f, 1.0f);
        mViewOffsetY = -100.0f;
        Mtx44 projection;
        C_MTXPerspective(projection, 45.0f, mDoGph_gInf_c::getAspect(), 1.0f, 100000.0f);
        GXSetProjection(projection, GX_PERSPECTIVE);
        calcViewMtx(param_0);
        return;
    }
#endif
    GXSetViewport(0.0f, mViewOffsetY, FB_WIDTH, FB_HEIGHT, 0.0f, 1.0f);
    IF_NOT_DUSK(mViewOffsetY = -100.0f);
    Mtx44 projection;
    C_MTXPerspective(projection, 45.0f, mDoGph_gInf_c::getAspect(), 1.0f, 100000.0f);
    GXSetProjection(projection, GX_PERSPECTIVE);
    calcViewMtx(param_0);
}

#pragma push
#pragma optimization_level 2
void dMenu_Collect3D_c::toItem3Dpos(f32 param_0, f32 param_1, f32 param_2, cXyz* param_3) {
    Mtx adStack_98;
    Mtx auStack_c8;
#if TARGET_PC
    if (collectRevo()) {
        // TPHD 0x029ba760: unstretched 608 x 448 canvas, aspect 608 / 448 (= 1.35714).
        const f32 nx = 2.0f * (param_0 / 608.0f) - 1.0f;
        const f32 ny = 2.0f * (param_1 / 448.0f) - 1.0f;
        calcViewMtx(adStack_98);
        MTXInverse(adStack_98, auStack_c8);
        const f32 t = tan(0.39269909262657166);
        cXyz v(nx * param_2 * mDoGph_gInf_c::getAspect() * t, t * (ny * -param_2), -param_2);
        MTXMultVec(auStack_c8, &v, param_3);
        return;
    }
    param_0 =
        (2.0f * ((param_0 - mDoGph_gInf_c::getSafeMinXF()) / mDoGph_gInf_c::getSafeWidthF()) -
         1.0f);
#else
    param_0 =
        (2.0f * ((param_0 - mDoGph_gInf_c::getMinXF()) / mDoGph_gInf_c::getWidthF()) - 1.0f);
#endif
    param_1 = (2.0f * ((param_1 - -100.0f) / FB_HEIGHT_BASE) - 1.0f);
    calcViewMtx(adStack_98);
    MTXInverse(adStack_98, auStack_c8);
    f32 tangent = tan(0.39269909262657166);
    f32 dVar12 = -param_2;
    cXyz cStack_d4((param_0 * param_2) * (mDoGph_gInf_c::getAspect() * tangent),
                   (tangent * (param_1 * dVar12)), dVar12);
    MTXMultVec(auStack_c8, &cStack_d4, param_3);
}
#pragma pop

void dMenu_Collect3D_c::calcViewMtx(Mtx param_0) {
    cXyz pos1(0.0f, 0.0f, -1000.0f);
    cXyz pos2(0.0f, 1.0f, 0.0f);
    mDoMtx_lookAt(param_0, &pos1, &cXyz::Zero, &pos2, 0);
}


void dMenu_Collect2DTop_c::draw() {
    J2DGrafContext* graf_port = dComIfGp_getCurrentGrafPort();
    graf_port->setup2D();
    mpCollect2D->getBlackTex()->draw(mDoGph_gInf_c::getMinXF(), mDoGph_gInf_c::getMinYF(),
                                     mDoGph_gInf_c::getWidthF(), mDoGph_gInf_c::getHeightF(),
                                     false, false, false);
    if (mpCollect2D->getSaveScreen() != NULL) {
        mpCollect2D->getSaveScreen()->_draw();
    }
    if (mpCollect2D->getOptionScreen() != NULL) {
        mpCollect2D->getOptionScreen()->_draw();
    }
    if (mpCollect2D->getLetterScreen() != NULL) {
        mpCollect2D->getLetterScreen()->_draw();
    }
    if (mpCollect2D->getFishingScreen() != NULL) {
        mpCollect2D->getFishingScreen()->_draw();
    }
    if (mpCollect2D->getSkillScreen() != NULL) {
        mpCollect2D->getSkillScreen()->_draw();
    }
    if (mpCollect2D->getInsectScreen() != NULL) {
        mpCollect2D->getInsectScreen()->_draw();
    }
    if (mpCollect2D->getIconScreen() != NULL) {
#if TARGET_PC
        if (collectRevo()) {
            // TPHD (screenSet 0x029b5a28 + setHIO, drawn by Top::draw at (0, 0)): the layer is a
            // 608-unit canvas stretched to 16:9 like the collection screen. abtn_n / bbtn_n carry
            // user info 'n_43', so Wii U J2DPane::draw (0x02b34334) squeezes their local X axis by
            // 608 / orthoWidth around their anchor: the anchors follow the stretch, the badges and
            // captions stay 1:1. CPaneMgr g_abtn_n / g_bbtn_n / a_text_n / b_text_n get the collect
            // HIO (ctor 0x02a4e00c) through paneTrans (initial translate + offset) and scale:
            // A (-22.3, 24.9) x1.3, B (-22.3, 25.0) x1.3, A text (-24.9, 24.9) x1.1,
            // B text (-27.5, 26.0) x1.1.
            J2DScreen* icon = mpCollect2D->getIconScreen();
            static J2DScreen* s_icon = NULL;
            static f32 s_init[4][2];
            static const u64 kTags[4] = {MULTI_CHAR('g_abtn_n'), MULTI_CHAR('g_bbtn_n'),
                                         MULTI_CHAR('a_text_n'), MULTI_CHAR('b_text_n')};
            static const f32 kHio[4][3] = {{-22.3f, 24.9f, 1.3f}, {-22.3f, 25.0f, 1.3f},
                                           {-24.9f, 24.9f, 1.1f}, {-27.5f, 26.0f, 1.1f}};
            if (icon != s_icon) {
                s_icon = icon;
                for (int i = 0; i < 4; i++) {
                    J2DPane* p = icon->search(kTags[i]);
                    s_init[i][0] = p != NULL ? p->getTranslateX() : 0.0f;
                    s_init[i][1] = p != NULL ? p->getTranslateY() : 0.0f;
                }
            }
            for (int i = 0; i < 4; i++) {
                if (J2DPane* p = icon->search(kTags[i])) {
                    p->translate(s_init[i][0] + kHio[i][0], s_init[i][1] + kHio[i][1]);
                    p->scale(kHio[i][2], kHio[i][2]);
                }
            }
            const f32 sx = collectRevoScaleX();
            if (J2DPane* p = icon->search(MULTI_CHAR('abtn_n'))) {
                p->scale(1.0f / sx, 1.0f);
            }
            if (J2DPane* p = icon->search(MULTI_CHAR('bbtn_n'))) {
                p->scale(1.0f / sx, 1.0f);
            }
            icon->scale(sx, 1.0f);
            icon->draw(mDoGph_gInf_c::getMinXF(), 0.0f, graf_port);
        } else {
            mpCollect2D->getIconScreen()->draw(0.0f, 0.0f, graf_port);
        }
#else
        mpCollect2D->getIconScreen()->draw(0.0f, 0.0f, graf_port);
#endif
    }
}

dMenu_Collect_c::dMenu_Collect_c(JKRExpHeap* param_0, STControl* param_1, CSTControl* param_2) {
    mpCollect2D = JKR_NEW dMenu_Collect2D_c(param_0, param_1, param_2);
    mpCollect3D = JKR_NEW dMenu_Collect3D_c(param_0, mpCollect2D, param_2);
}

dMenu_Collect_c::~dMenu_Collect_c() {
    if (mpCollect2D) {
        JKR_DELETE(mpCollect2D);
        mpCollect2D = NULL;
    }
    if (mpCollect3D) {
        JKR_DELETE(mpCollect3D);
        mpCollect3D = NULL;
    }
}


void dMenu_Collect_c::_create() {
    mpCollect2D->_create();
    mpCollect3D->_create();
}

void dMenu_Collect_c::_delete() {
    mpCollect2D->_delete();
    mpCollect3D->_delete();
}

void dMenu_Collect_c::_move() {
    mpCollect2D->_move();
    mpCollect3D->_move(mpCollect2D->getCursorX(), mpCollect2D->getCursorY());
}

void dMenu_Collect_c::draw() {
    dComIfGd_set2DOpa(mpCollect2D);
    IF_DUSK_BLOCK(dusk::game_clock::is_sim_frame())
    mpCollect3D->draw();
    IF_DUSK_BLOCK_END
    mpCollect2D->drawTop();
}
