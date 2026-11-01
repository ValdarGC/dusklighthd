/**
 * d_menu_insect.cpp
 * Menu - Insect List
 */

#include "d/dolzel.h" // IWYU pragma: keep

#include "d/d_menu_insect.h"
#include "JSystem/J2DGraph/J2DTextBox.h"
#include "JSystem/JKernel/JKRExpHeap.h"
#include "JSystem/JKernel/JKRMemArchive.h"
#include "JSystem/JUtility/JUTTexture.h"
#include "d/d_lib.h"
#include "d/d_select_cursor.h"
#include "d/d_com_inf_game.h"
#include "d/d_meter2_info.h"
#include "d/d_meter_HIO.h"
#include "d/d_msg_scrn_3select.h"
#include "d/d_msg_string.h"
#include "m_Do/m_Do_controller_pad.h"
#include "m_Do/m_Do_graphic.h"
#include <cstdio>
#include <cstring>

#if TARGET_PC
#include "dusk/game_clock.h"
#include "dusk/menu_pointer.h"
#include "dusk/version.hpp"
#include "dusk/tphd/HdSubscreen.hpp"

extern JKRArchive* g_tphdMain2DRevoArchive;

namespace {
struct TphdInsectState {
    CPaneMgr* mpBox[MAX_INSECT_NUM] = {};
    CPaneMgr* mpBoxSelected[MAX_INSECT_NUM] = {};
    CPaneMgr* mpINSParentSel[MAX_INSECT_NUM] = {};
    CPaneMgr* mpTitleText = nullptr;
    bool mFirstDraw = true;
    u8 mInfoShown = 0xff;  // insect item shown on the right page (RPX +0x229)
    u8 mInfoState = 0;     // 0 set up, 1 fade in, 2 shown, 3 fade out (RPX +0x22b)
    s16 mInfoFrame = 0;

    void reset() {
        for (int i = 0; i < MAX_INSECT_NUM; i++) {
            if (mpBox[i] != NULL) {
                JKR_DELETE(mpBox[i]);
                mpBox[i] = NULL;
            }
            if (mpBoxSelected[i] != NULL) {
                JKR_DELETE(mpBoxSelected[i]);
                mpBoxSelected[i] = NULL;
            }
            if (mpINSParentSel[i] != NULL) {
                JKR_DELETE(mpINSParentSel[i]);
                mpINSParentSel[i] = NULL;
            }
        }
        if (mpTitleText != NULL) {
            JKR_DELETE(mpTitleText);
            mpTitleText = NULL;
        }
        mFirstDraw = true;
        mInfoShown = 0xff;
        mInfoState = 0;
        mInfoFrame = 0;
    }
} s_tphdInsect;
}  // namespace
#endif

typedef void (dMenu_Insect_c::*initFunc)();
static initFunc map_init_process[] = {
    &dMenu_Insect_c::wait_init,          &dMenu_Insect_c::explain_open_init,
    &dMenu_Insect_c::explain_move_init,  &dMenu_Insect_c::select_move_init,
    &dMenu_Insect_c::explain_close_init,
};

typedef void (dMenu_Insect_c::*moveFunc)();
static moveFunc map_move_process[] = {
    &dMenu_Insect_c::wait_move,          &dMenu_Insect_c::explain_open_move,
    &dMenu_Insect_c::explain_move_move,  &dMenu_Insect_c::select_move_move,
    &dMenu_Insect_c::explain_close_move,
};

dMenu_Insect_c::dMenu_Insect_c(JKRExpHeap* i_heap, STControl* i_stick, CSTControl* i_cstick,
                               u8 param_3) {
    mpHeap = i_heap;
    mpArchive = NULL;
    mpMount = NULL;
    mpStick = i_stick;
    mpCStick = i_cstick;
    mStatus = 1;
    field_0xf3 = 0;
    field_0xf0 = 0;
    field_0xf4 = 0;
    field_0xf5 = 0;
    field_0xf6 = param_3;
    field_0xf8 = 0xff;
    field_0xf9 = 0xff;
    field_0xfa = 0xff;
    field_0xfb = 0xff;
    field_0xf7 = 1;
    ResTIMG* image = (ResTIMG*)dComIfGp_getMain2DArchive()->getResource('TIMG', "tt_block8x8.bti");
    mpBlackTex = JKR_NEW J2DPicture(image);
    mpBlackTex->setBlackWhite(JUtility::TColor(0, 0, 0, 0), JUtility::TColor(0, 0, 0, 0xff));
    field_0xfc = 0;
    mpDrawCursor = JKR_NEW dSelect_cursor_c(2, 1.0f, NULL);
    mpDrawCursor->setParam(1.0f, 1.0f, 0.1f, 0.7f, 0.7f);
    mpDrawCursor->setAlphaRate(0.0f);
    mpDrawCursor->setScale(0.0f);
    mpDrawCursor->offPlayAnime(0);
    mpSelect_c = JKR_NEW dMsgScrn3Select_c();
    mpExpItemTex = (ResTIMG*)mpHeap->alloc(0xc00, 0x20);
#if TARGET_PC
    s_tphdInsect.reset();
#endif
}

dMenu_Insect_c::~dMenu_Insect_c() {
    JKR_DELETE(mpBlackTex);
    mpBlackTex = NULL;

    JKR_DELETE(mpDrawCursor);
    mpDrawCursor = NULL;

    JKR_DELETE(mpSelect_c);
    mpSelect_c = NULL;

    mpHeap->free(mpExpItemTex);
    mpExpItemTex = NULL;

    JKR_DELETE(mpString);
    mpString = NULL;

    JKR_DELETE(mpScreen);
    mpScreen = NULL;

    JKR_DELETE(mpParent);
    mpParent = NULL;

    for (int i = 0; i < MAX_INSECT_NUM; i++) {
        JKR_DELETE(mpINSParent[i]);
        mpINSParent[i] = NULL;
    }

    JKR_DELETE(mpExpScreen);
    mpExpScreen = NULL;

    JKR_DELETE(mpExpParent);
    mpExpParent = NULL;

    for (int i = 0; i < 2; i++) {
        JKR_DELETE(mpExpSubWin[i]);
        mpExpSubWin[i] = NULL;
    }

    JKR_DELETE(mpInfoText);
    mpInfoText = NULL;

    JKR_DELETE(mpIconScreen);
    mpIconScreen = NULL;

    for (int i = 0; i < 2; i++) {
        if (mpButtonAB[i] != NULL) {
            JKR_DELETE(mpButtonAB[i]);
            mpButtonAB[i] = NULL;
        }

        if (mpButtonText[i] != NULL) {
            JKR_DELETE(mpButtonText[i]);
            mpButtonText[i] = NULL;
        }
    }

#if TARGET_PC
    s_tphdInsect.reset();
#endif

    if (mpMount != NULL) {
        mpMount->getArchive()->unmount();
        JKR_DELETE(mpMount);
        mpMount = NULL;
    }

    if (mpArchive != NULL) {
        mpArchive->unmount();
        mpArchive = NULL;
    }

    dComIfGp_getMsgArchive(0)->removeResourceAll();
}

void dMenu_Insect_c::_create() {
#if TARGET_PC
    if (dusk::tphd_active()) {
        if (g_tphdMain2DRevoArchive != NULL) {
            JKR_DELETE(mpDrawCursor);
            mpDrawCursor = JKR_NEW dSelect_cursor_c(2, 1.0f, g_tphdMain2DRevoArchive);
        }
        mpDrawCursor->setParam(0.85f, 0.975f, 0.1f, 0.7f, 0.7f);
        mpDrawCursor->setAlphaRate(0.0f);
        mpDrawCursor->setScale(0.0f);
        mpDrawCursor->offPlayAnime(0);
    }
#endif
    mpString = JKR_NEW dMsgString_c();
    screenSetBase();
    screenSetExplain();
    screenSetDoIcon();
    setHIO(true);
    init();
}

void dMenu_Insect_c::_move() {
    JKRHeap* heap = mDoExt_setCurrentHeap((JKRHeap*)mpHeap);
    u8 process = field_0xf3;
    (this->*map_move_process[field_0xf3])();
    if (process != field_0xf3) {
        (this->*map_init_process[field_0xf3])();
    }
    setHIO(false);
#if TARGET_PC
    if (dusk::tphd_active()) {
        moveInsectInfo();
    }
#endif
    mDoExt_setCurrentHeap(heap);
}

void dMenu_Insect_c::_draw() {
    if (mpArchive != NULL && mpScreen != NULL) {
#if TARGET_PC
        if (dusk::game_clock::is_presentation_frame()) {
            mpExpParent->presentAnime();
        }
#endif
        J2DGrafContext* grafPort = dComIfGp_getCurrentGrafPort();
        if (mpBlackTex != NULL) {
            mpBlackTex->setAlpha(0xff);

            #if TARGET_PC
            mpBlackTex->draw(mDoGph_gInf_c::getMinXF(), mDoGph_gInf_c::getMinYF(),
                             mDoGph_gInf_c::getWidthF(), mDoGph_gInf_c::getHeightF(), 0, 0, 0);
            #else
            mpBlackTex->draw(0.0f, 0.0f, FB_WIDTH, FB_HEIGHT, 0, 0, 0);
            #endif
        }

#if TARGET_PC
        if (dusk::tphd_active()) {
            // RPX 0x029e9444: black backdrop, grid, cursor, description page, give window,
            // A/B icons.
            dusk::tphd::drawRevoScreen(mpScreen, grafPort);

            // RPX 0x029e960c: cursor on the centre of the insect icon pane (50x50, 'n_43') of the
            // cell, moved by (-2, -2.5), box = pane size.
            const int index = field_0xf4 + field_0xf5 * 6;
            if (mpDrawCursor != NULL && index < MAX_INSECT_NUM && mpINSParent[index] != NULL) {
                const Vec c = dusk::tphd::mgrDrawnCenter(mpINSParent[index]);
                const f32 sx = dusk::tphd::subscreenScaleX();
                mpDrawCursor->setPosRevo(c.x - 2.0f, c.y - 2.5f, mpINSParent[index]->getPanePtr(),
                                         false, sx, dusk::tphd::revoMinX());
                mpDrawCursor->draw();
            }

            if (mpExpScreen != NULL) {
                dusk::tphd::drawRevoScreen(mpExpScreen, grafPort);
            }

            // RPX: dMsgScrn3Select_c with the 16:9 HIO values (offset 6, at (6 + 486, 129 + 209)).
            if (field_0xf6 == 1 && field_0xf3 == 3 && mpSelect_c != NULL) {
                mpSelect_c->setOffsetX(6.0f);
                mpSelect_c->translate(6.0f + 486.0f, 129.0f + 209.0f);
                mpSelect_c->draw(0.0f, 0.0f);
            }
            if (mpIconScreen != NULL) {
                dusk::tphd::drawRevoIconScreen(mpIconScreen, grafPort);
            }
        } else {
            mpScreen->draw(0.0f, 0.0f, grafPort);
            mpDrawCursor->draw();
            field_0xfc = mpExpParent->getAlphaRate() * 150.0f;
            mpBlackTex->setAlpha(field_0xfc);
            mpBlackTex->draw(mDoGph_gInf_c::getMinXF(), mDoGph_gInf_c::getMinYF(),
                             mDoGph_gInf_c::getWidthF(), mDoGph_gInf_c::getHeightF(), 0, 0, 0);
            mpExpScreen->draw(0.0f, 0.0f, grafPort);
            mpSelect_c->setOffsetX(g_drawHIO.mInsectListScreen.mConfirmOptionPosX_4x3);
            mpSelect_c->translate(g_drawHIO.mInsectListScreen.mConfirmOptionPosX_4x3 + 486.0f,
                                  g_drawHIO.mInsectListScreen.mConfirmOptionPosY_4x3 + 209.0f);
            mpSelect_c->draw(0.0f, 0.0f);
            mpIconScreen->draw(0.0f, 0.0f, grafPort);
        }
#else
        mpScreen->draw(0.0f, 0.0f, grafPort);
        mpDrawCursor->draw();
        field_0xfc = mpExpParent->getAlphaRate() * 150.0f;
        mpBlackTex->setAlpha(field_0xfc);
        mpBlackTex->draw(0.0f, 0.0f, FB_WIDTH, FB_HEIGHT, 0, 0, 0);
        mpExpScreen->draw(0.0f, 0.0f, grafPort);
        mpSelect_c->setOffsetX(g_drawHIO.mInsectListScreen.mConfirmOptionPosX_4x3);
        mpSelect_c->translate(g_drawHIO.mInsectListScreen.mConfirmOptionPosX_4x3 + 486.0f,
                              g_drawHIO.mInsectListScreen.mConfirmOptionPosY_4x3 + 209.0f);
        IF_DUSK(mpSelect_c->presentAnims());
        mpSelect_c->draw(0.0f, 0.0f);
        mpIconScreen->draw(0.0f, 0.0f, grafPort);
#endif
    }
}

bool dMenu_Insect_c::isSync() {
    if (mpMount != NULL && mpMount->sync() == false) {
        return 0;
    }
    return 1;
}

void dMenu_Insect_c::init() {
    (this->*map_init_process[field_0xf3])();
}

int dMenu_Insect_c::_open() {
    if (mpMount == NULL) {
#if TARGET_PC
        const char* arcPath = dusk::tphd_active() ? "/res/LayoutRevo/insectResR.arc"
                                                  : "/res/Layout/insectRes.arc";
        mpMount = mDoDvdThd_mountArchive_c::create(arcPath, 0, NULL);
#else
        mpMount = mDoDvdThd_mountArchive_c::create("/res/Layout/insectRes.arc", 0, NULL);
#endif
    }
    if (mpArchive == NULL) {
        if (mpMount->sync() != 0) {
            if (mpArchive == NULL) {
                mpArchive = (JKRArchive*)mpMount->getArchive();
                JKR_DELETE(mpMount);
                mpMount = NULL;
                _create();
            }
        } else {
            return 0;
        }
    }

    s16 openFrame = g_drawHIO.mInsectListScreen.mOpenFrame;
    s16 closeFrame = g_drawHIO.mInsectListScreen.mCloseFrame;
    field_0xf0 = g_drawHIO.mInsectListScreen.mOpenFrame;
    if (field_0xf0 >= openFrame) {
        field_0xf0 = closeFrame;
        mStatus = 2;
        mpParent->scale(1.0f, 1.0f);
        mpParent->setAlphaRate(1.0f);
        setCursorPos();
#if TARGET_PC
        if (dusk::tphd_active()) {
            updateInsectInfo();
        }
#endif
        mpDrawCursor->setAlphaRate(1.0f);
        mpDrawCursor->setScale(1.0f);
        mpDrawCursor->onPlayAnime(0);
        return 1;
    } else {
        f32 div = (f32)field_0xf0 / (f32)openFrame;
        mpParent->scale(div, div);
        mpParent->setAlphaRate(div);
        setCursorPos();
        mpDrawCursor->setAlphaRate(div);
        mpDrawCursor->setScale(div);
        return 0;
    }
}

int dMenu_Insect_c::_close() {
    s16 closeFrame = g_drawHIO.mInsectListScreen.mCloseFrame;
    field_0xf0 = 0;
    if (field_0xf0 <= 0) {
        field_0xf0 = 0;
        mStatus = 0;
        mpParent->scale(0.0f, 0.0f);
        mpParent->setAlphaRate(0.0f);
#if TARGET_PC
        if (dusk::tphd_active()) {
            if (mpExpParent != NULL) {
                mpExpParent->scale(0.0f, 0.0f);
                mpExpParent->setAlphaRate(0.0f);
            }
            if (mpDrawCursor != NULL) {
                mpDrawCursor->setAlphaRate(0.0f);
                mpDrawCursor->setScale(0.0f);
            }
        }
#endif
        setCursorPos();
        mpDrawCursor->setAlphaRate(0.0f);
        mpDrawCursor->setScale(0.0f);
        return 1;
    } else {
        f32 div = (f32)field_0xf0 / (f32)closeFrame;
        mpParent->scale(div, div);
        mpParent->setAlphaRate(div);
        setCursorPos();
        mpDrawCursor->setAlphaRate(div);
        mpDrawCursor->setScale(div);
        return 0;
    }
    return field_0xf0 <= 0;
}

void dMenu_Insect_c::wait_init() {
#if TARGET_PC
    if (dusk::tphd_active()) {
        if (field_0xf6 == 1) {
            if (isGetInsect(field_0xf4, field_0xf5) && !isGiveInsect(field_0xf4, field_0xf5)) {
                setAButtonString(0x40c);
            } else {
                setAButtonString(0);
            }
        } else {
            setAButtonString(0);
        }
        setBButtonString(0x3f9);
        updateInsectInfo();
        return;
    }
#endif
    if (isGetInsect(field_0xf4, field_0xf5) != 0) {
        setAButtonString(0x368);
    } else {
        setAButtonString(0);
    }
    setBButtonString(0x3f9);
}

void dMenu_Insect_c::wait_move() {
    if (mDoGph_gInf_c::getFader()->getStatus() == 1) {
#if TARGET_PC
        if (pointerWait()) {
            return;
        }
#endif

        if (mDoCPd_c::getTrigB(PAD_1) || field_0xf7 == 0) {
            if (mDoCPd_c::getTrigB(PAD_1) && field_0xf6 == 1) {
                dMeter2Info_setInsectSelectType(0);
            }
            mStatus = 3;
            mpDrawCursor->offPlayAnime(0);
        } else if (mDoCPd_c::getTrigA(PAD_1)) {
#if TARGET_PC
            if (dusk::tphd_active()) {
                if (field_0xf6 == 1) {
                    if (isGetInsect(field_0xf4, field_0xf5) && !isGiveInsect(field_0xf4, field_0xf5)) {
                        field_0xf3 = 1;
                        Z2GetAudioMgr()->seStart(Z2SE_SY_EXP_WIN_OPEN, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
                        dMeter2Info_set2DVibration();
                    } else {
                        Z2GetAudioMgr()->seStart(Z2SE_SYS_ERROR, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
                    }
                }
                return;
            }
#endif
            if (isGetInsect(field_0xf4, field_0xf5)) {
                field_0xf3 = 1;
                Z2GetAudioMgr()->seStart(Z2SE_SY_EXP_WIN_OPEN, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f,
                                         0);
                dMeter2Info_set2DVibration();
            } else {
                Z2GetAudioMgr()->seStart(Z2SE_SYS_ERROR, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
            }
        } else {
            cursorMove();
        }
    }
}

#if TARGET_PC
bool dMenu_Insect_c::pointerWait() {
    dusk::menu_pointer::begin_context(dusk::menu_pointer::Context::Collection);
    for (u8 y = 0; y < 4; ++y) {
        for (u8 x = 0; x < 6; ++x) {
            const int index = x + y * 6;
            CPaneMgr* target = (dusk::tphd_active() && s_tphdInsect.mpBox[index] != NULL)
                                   ? s_tphdInsect.mpBox[index]
                                   : mpINSParent[index];
            if (target == NULL || !dusk::menu_pointer::hit_pane(target, 4.0f)) {
                continue;
            }
            dusk::menu_pointer::set_hover_target(index);

            if (field_0xf4 != x || field_0xf5 != y) {
                field_0xf4 = x;
                field_0xf5 = y;
                setCursorPos();
                if (dusk::tphd_active()) {
                    updateInsectInfo();
                    if (field_0xf6 == 1) {
                        setAButtonString((isGetInsect(x, y) && !isGiveInsect(x, y)) ? 0x40c : 0);
                    } else {
                        setAButtonString(0);
                    }
                } else {
                    setAButtonString(isGetInsect(x, y) ? 0x368 : 0);
                }
                Z2GetAudioMgr()->seStart(Z2SE_SY_CURSOR_ITEM, NULL, 0, 0, 1.0f, 1.0f, -1.0f,
                                         -1.0f, 0);
            }
            if (dusk::menu_pointer::consume_click()) {
                if (dusk::tphd_active()) {
                    if (field_0xf6 == 1 && isGetInsect(x, y) && !isGiveInsect(x, y)) {
                        field_0xf3 = 1;
                        Z2GetAudioMgr()->seStart(Z2SE_SY_EXP_WIN_OPEN, NULL, 0, 0, 1.0f, 1.0f, -1.0f,
                                                 -1.0f, 0);
                        dMeter2Info_set2DVibration();
                        return true;
                    }
                    return false;
                }
                if (isGetInsect(x, y)) {
                    field_0xf3 = 1;
                    Z2GetAudioMgr()->seStart(Z2SE_SY_EXP_WIN_OPEN, NULL, 0, 0, 1.0f, 1.0f, -1.0f,
                                             -1.0f, 0);
                    dMeter2Info_set2DVibration();
                    return true;
                }
            }
            return false;
        }
    }

    return false;
}
#endif

void dMenu_Insect_c::explain_open_init() {
    char local_78[32];
    char local_98[32];
    char local_b8[32];
    char cStack_d8[32];

    setAButtonString(0);
    setBButtonString(0x3F9);
    u8 insectItemId = getInsectItemID(field_0xf4, field_0xf5);
    s32 i_textureNum = dMeter2Info_readItemTexture(insectItemId, mpExpItemTex, NULL, NULL, NULL,
                                                   NULL, NULL, NULL, NULL, -1);
    f32 tex = mpExpItemTex->width / 48.0f;
    f32 tex2 = mpExpItemTex->height / 48.0f;

    for (int i = 0; i < 2; i++) {
        field_0x54[i]->changeTexture(mpExpItemTex, 0);
        field_0x54[i]->scale(tex, tex2);
    }
#if TARGET_PC
    if (dusk::tphd_active()) {
        J2DPane* wani = mpExpScreen->search(MULTI_CHAR('waning_n'));
        if (wani != NULL) {
            wani->show();
        }
    } else {
        mpString->getString(insectItemId + 0x265, (J2DTextBox*)mpInfoText->getPanePtr(), NULL, NULL, NULL, 0);
    }
#else
    mpString->getString(insectItemId + 0x265, (J2DTextBox*)mpInfoText->getPanePtr(), NULL, NULL, NULL, 0);
#endif
    if (field_0xf6 == 1) {
        if (!isGiveInsect(field_0xf4, field_0xf5)) {
            mpString->getString(0x5BB, field_0x5c, NULL, NULL, NULL, 0);
            local_78[0] = 0;
            local_98[0] = 0;
            local_b8[0] = 0;
            cStack_d8[1] = 0;
            field_0xf7 = 0;
            dMeter2Info_getString(0x4BD, local_78, NULL);
            dMeter2Info_getString(0x4BE, local_98, NULL);
            f32 stringLength1 =
                dMeter2Info_getStringLength(mDoExt_getMesgFont(), mpSelect_c->getFontSize(),
                                            mpSelect_c->getCharSpace(), local_78);
            f32 stringLength2 =
                dMeter2Info_getStringLength(mDoExt_getMesgFont(), mpSelect_c->getFontSize(),
                                            mpSelect_c->getCharSpace(), local_98);
            f32 length;
            if (stringLength1 < stringLength2) {
                length = stringLength2;
            } else {
                length = stringLength1;
            }
            f32 textBoxWidth = mpSelect_c->getTextBoxWidth();
            if (length < textBoxWidth) {
                length = mpSelect_c->getTextBoxWidth();
            }

            snprintf(local_b8, 32,
                     "\x1B"
                     "CR[%d]",
                     (int)(0.5f * (length - stringLength1)));
            SAFE_STRCAT(local_b8, local_78);
            snprintf(cStack_d8, 32,
                     "\x1B"
                     "CR[%d]",
                     (int)(0.5f * (length - stringLength2)));
            SAFE_STRCAT(cStack_d8, local_98);
            mpSelect_c->setString("", local_b8, cStack_d8);
            mpSelect_c->setRubyString("", "", "");
            mpSelect_c->selAnimeInit(2, field_0xf7 + 1, 0, length, 0);
        } else {
            mpString->getString(0x5BC, field_0x5c, NULL, NULL, NULL, 0);
        }
    }
    mpExpParent->alphaAnimeStart(0);
}

void dMenu_Insect_c::explain_open_move() {
#if TARGET_PC
    if (dusk::tphd_active()) {
        if (field_0xf6 == 1 && isGiveInsect(field_0xf4, field_0xf5) == 0) {
            field_0xf3 = 3;
        } else {
            field_0xf3 = 2;
        }
        return;
    }
#endif
    if (mpExpParent->alphaAnime(10, 0, 0xff, 1)) {
        if (field_0xf6 == 1 && isGiveInsect(field_0xf4, field_0xf5) == 0) {
            field_0xf3 = 3;
        } else {
            field_0xf3 = 2;
        }
    }
}

void dMenu_Insect_c::explain_move_init() {
    /* empty function */
}

void dMenu_Insect_c::explain_move_move() {
    if (mDoCPd_c::getTrigA(PAD_1) == 0 && mDoCPd_c::getTrigB(PAD_1) == 0) {
        return;
    }
    field_0xf3 = 4;
}

void dMenu_Insect_c::select_move_init() {
    dMeter2Info_setInsectSelectType(0xff);
    setAButtonString(0x40c);
    setBButtonString(0x3f9);
}

void dMenu_Insect_c::select_move_move() {
    mpStick->checkTrigger();
    if (mDoCPd_c::getTrigA(PAD_1)) {
        field_0xf3 = 4;
        if (field_0xf7 == 0) {
            Z2GetAudioMgr()->seStart(Z2SE_SY_CURSOR_OK, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
            dMeter2Info_set2DVibration();
        } else {
            Z2GetAudioMgr()->seStart(Z2SE_SY_CURSOR_CANCEL, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f,
                                     0);
        }
    } else if (mDoCPd_c::getTrigB(PAD_1)) {
        field_0xf3 = 4;
        field_0xf7 = 1;
        Z2GetAudioMgr()->seStart(Z2SE_SY_CURSOR_CANCEL, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
        dMeter2Info_set2DVibration();
    } else if (mpSelect_c->isSelect() && mpStick->checkUpTrigger()) {
        if (field_0xf7 == 1) {
            field_0xf7 = 0;
            Z2GetAudioMgr()->seStart(Z2SE_SY_MENU_CURSOR_COMMON, NULL, 0, 0, 1.0f, 1.0f, -1.0f,
                                     -1.0f, 0);
        }

    } else if (mpSelect_c->isSelect() && mpStick->checkDownTrigger()) {
        if (field_0xf7 == 0) {
            field_0xf7 = 1;
            Z2GetAudioMgr()->seStart(Z2SE_SY_MENU_CURSOR_COMMON, NULL, 0, 0, 1.0f, 1.0f, -1.0f,
                                     -1.0f, 0);
        }
    }
    if (field_0xf3 == 3) {
        mpSelect_c->selAnimeMove(2, field_0xf7 + 1, false);
    } else {
        mpSelect_c->selAnimeEnd();
    }
}

void dMenu_Insect_c::explain_close_init() {
    setAButtonString(0);
    setBButtonString(0x3f9);
    if (field_0xf6 == 1) {
        dMeter2Info_setInsectSelectType(getInsectItemID(field_0xf4, field_0xf5));
    }
    Z2GetAudioMgr()->seStart(Z2SE_SY_EXP_WIN_CLOSE, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
    dMeter2Info_set2DVibration();
#if TARGET_PC
    if (dusk::tphd_active()) {
        J2DPane* wani = mpExpScreen->search(MULTI_CHAR('waning_n'));
        if (wani != NULL) {
            wani->hide();
        }
    } else {
        mpExpParent->alphaAnimeStart(0);
    }
#else
    mpExpParent->alphaAnimeStart(0);
#endif
}

void dMenu_Insect_c::explain_close_move() {
#if TARGET_PC
    if (dusk::tphd_active()) {
        bool cVar2 = (field_0xf6 == 1 && isGiveInsect(field_0xf4, field_0xf5) == 0) ? mpSelect_c->selAnimeEnd() : 1;
        if (cVar2) {
            field_0xf3 = 0;
        }
        return;
    }
#endif
    bool alphaAnime = mpExpParent->alphaAnime(10, 0xff, 0, 1);
    bool cVar2;
    if (field_0xf6 == 1 && isGiveInsect(field_0xf4, field_0xf5) == 0) {
        cVar2 = mpSelect_c->selAnimeEnd();
    } else {
        cVar2 = 1;
    }
    if (alphaAnime && cVar2 != 0) {
        field_0xf3 = 0;
    }
}

void dMenu_Insect_c::screenSetBase() {
    static const u64 insect_tag[MAX_INSECT_NUM] = {
        MULTI_CHAR('ari_os'),  MULTI_CHAR('ari_ms'),   MULTI_CHAR('kag_os'),  MULTI_CHAR('kag_ms'),   MULTI_CHAR('kab_os'), MULTI_CHAR('kab_ms'),  MULTI_CHAR('kam_os'),  MULTI_CHAR('kam_mes'),
        MULTI_CHAR('kuwa_os'), MULTI_CHAR('kuwa_mes'), MULTI_CHAR('dan_os'),  MULTI_CHAR('dan_mes'),  MULTI_CHAR('cho_os'), MULTI_CHAR('cho_ms'),  MULTI_CHAR('tent_os'), MULTI_CHAR('tent_mes'),
        MULTI_CHAR('kata_os'), MULTI_CHAR('kata_mes'), MULTI_CHAR('nana_os'), MULTI_CHAR('nana_mes'), MULTI_CHAR('bat_os'), MULTI_CHAR('bat_mes'), MULTI_CHAR('tonb_os'), MULTI_CHAR('tonb_mes'),
    };
    static const u64 ageha_tag[MAX_INSECT_NUM] = {
        MULTI_CHAR('ageha00'), MULTI_CHAR('ageha01'), MULTI_CHAR('ageha02'), MULTI_CHAR('ageha03'), MULTI_CHAR('ageha04'), MULTI_CHAR('ageha05'), MULTI_CHAR('ageha06'), MULTI_CHAR('ageha07'),
        MULTI_CHAR('ageha08'), MULTI_CHAR('ageha09'), MULTI_CHAR('ageha10'), MULTI_CHAR('ageha11'), MULTI_CHAR('ageha12'), MULTI_CHAR('ageha13'), MULTI_CHAR('ageha14'), MULTI_CHAR('ageha15'),
        MULTI_CHAR('ageha16'), MULTI_CHAR('ageha17'), MULTI_CHAR('ageha18'), MULTI_CHAR('ageha19'), MULTI_CHAR('ageha20'), MULTI_CHAR('ageha21'), MULTI_CHAR('ageha22'), MULTI_CHAR('ageha23'),
    };

#if TARGET_PC
    static const u64 insect_sel_tag[MAX_INSECT_NUM] = {
        MULTI_CHAR('ari_os1'),  MULTI_CHAR('ari_ms1'),   MULTI_CHAR('kag_os1'),  MULTI_CHAR('kag_ms1'),
        MULTI_CHAR('kab_os1'),  MULTI_CHAR('kab_ms1'),   MULTI_CHAR('kam_os1'),  MULTI_CHAR('kam_mes1'),
        MULTI_CHAR('kuwa_os1'), MULTI_CHAR('kuwa_me1'),  MULTI_CHAR('dan_os1'),  MULTI_CHAR('dan_mes1'),
        MULTI_CHAR('cho_os1'),  MULTI_CHAR('cho_ms1'),   MULTI_CHAR('tent_os1'), MULTI_CHAR('tent_me1'),
        MULTI_CHAR('kata_os1'), MULTI_CHAR('kata_me1'),  MULTI_CHAR('nana_os1'), MULTI_CHAR('nana_me1'),
        MULTI_CHAR('bat_os1'),  MULTI_CHAR('bat_mes1'),  MULTI_CHAR('tonb_os1'), MULTI_CHAR('tonb_me1'),
    };
    static const u64 box_tag[MAX_INSECT_NUM] = {
        MULTI_CHAR('gr_w_00'), MULTI_CHAR('gr_w_01'), MULTI_CHAR('gr_w_02'), MULTI_CHAR('gr_w_03'),
        MULTI_CHAR('gr_w_04'), MULTI_CHAR('gr_w_05'), MULTI_CHAR('gr_w_06'), MULTI_CHAR('gr_w_07'),
        MULTI_CHAR('gr_w_08'), MULTI_CHAR('gr_w_09'), MULTI_CHAR('gr_w_10'), MULTI_CHAR('gr_w_11'),
        MULTI_CHAR('gr_w_12'), MULTI_CHAR('gr_w_13'), MULTI_CHAR('gr_w_14'), MULTI_CHAR('gr_w_15'),
        MULTI_CHAR('gr_w_16'), MULTI_CHAR('gr_w_17'), MULTI_CHAR('gr_w_18'), MULTI_CHAR('gr_w_19'),
        MULTI_CHAR('gr_w_20'), MULTI_CHAR('gr_w_21'), MULTI_CHAR('gr_w_22'), MULTI_CHAR('gr_w_23'),
    };
    static const u64 box_sel_tag[MAX_INSECT_NUM] = {
        MULTI_CHAR('gr_w_24'), MULTI_CHAR('gr_w_25'), MULTI_CHAR('gr_w_26'), MULTI_CHAR('gr_w_27'),
        MULTI_CHAR('gr_w_28'), MULTI_CHAR('gr_w_29'), MULTI_CHAR('gr_w_30'), MULTI_CHAR('gr_w_31'),
        MULTI_CHAR('gr_w_32'), MULTI_CHAR('gr_w_33'), MULTI_CHAR('gr_w_34'), MULTI_CHAR('gr_w_35'),
        MULTI_CHAR('gr_w_36'), MULTI_CHAR('gr_w_37'), MULTI_CHAR('gr_w_38'), MULTI_CHAR('gr_w_39'),
        MULTI_CHAR('gr_w_40'), MULTI_CHAR('gr_w_41'), MULTI_CHAR('gr_w_42'), MULTI_CHAR('gr_w_43'),
        MULTI_CHAR('gr_w_44'), MULTI_CHAR('gr_w_45'), MULTI_CHAR('gr_w_46'), MULTI_CHAR('gr_w_47'),
    };
#endif

    mpScreen = JKR_NEW J2DScreen();
#if TARGET_PC
    const char* bloName = dusk::tphd_active() ? "zelda_gold_insects_revo.blo"
                                              : "zelda_gold_insects.blo";
    mpScreen->setPriority(bloName, 0x20000, mpArchive);
#else
    mpScreen->setPriority("zelda_gold_insects.blo", 0x20000, mpArchive);
#endif
    dPaneClass_showNullPane(mpScreen);
    mpParent = JKR_NEW CPaneMgr(mpScreen, MULTI_CHAR('n_all'), 2, NULL);
    mpParent->setAlphaRate(0.0f);
#if TARGET_PC
    if (dusk::tphd_active()) {
        J2DPane* wani = mpScreen->search(MULTI_CHAR('waning_n'));
        if (wani != NULL) {
            wani->hide();
        }
    }
#endif
    for (int i = 0; i < MAX_INSECT_NUM; i++) {
        mpINSParent[i] = JKR_NEW CPaneMgr(mpScreen, insect_tag[i], 0, NULL);
#if TARGET_PC
        if (dusk::tphd_active()) {
            s_tphdInsect.mpINSParentSel[i] = JKR_NEW CPaneMgr(mpScreen, insect_sel_tag[i], 0, NULL);
            s_tphdInsect.mpBox[i] = JKR_NEW CPaneMgr(mpScreen, box_tag[i], 0, NULL);
            s_tphdInsect.mpBoxSelected[i] = JKR_NEW CPaneMgr(mpScreen, box_sel_tag[i], 0, NULL);
            s_tphdInsect.mpBoxSelected[i]->hide();
            s_tphdInsect.mpBox[i]->show();
            s_tphdInsect.mpINSParentSel[i]->hide();
        } else {
            s_tphdInsect.mpINSParentSel[i] = NULL;
            s_tphdInsect.mpBox[i] = NULL;
            s_tphdInsect.mpBoxSelected[i] = NULL;
        }
#endif
    }
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 4; j++) {
            int iterateSum = i + j * 6;
            if (isGetInsect(i, j) != 0) {
                mpINSParent[iterateSum]->show();
            } else {
                mpINSParent[iterateSum]->hide();
            }
            if (isGiveInsect(i, j) != 0) {
                mpScreen->search(ageha_tag[iterateSum])->show();
            } else {
                mpScreen->search(ageha_tag[iterateSum])->hide();
            }
        }
    }
#if TARGET_PC
    J2DTextBox* textBox;
    if (dusk::version::isRegionJpn()) {
        textBox = (J2DTextBox*)mpScreen->search(MULTI_CHAR('t_t00'));
        mpScreen->search(MULTI_CHAR('f_t00'))->hide();
    } else {
        textBox = (J2DTextBox*)mpScreen->search(MULTI_CHAR('f_t00'));
        mpScreen->search(MULTI_CHAR('t_t00'))->hide();
    }
#elif VERSION == VERSION_GCN_JPN
    J2DTextBox* textBox = (J2DTextBox*)mpScreen->search(MULTI_CHAR('t_t00'));
    mpScreen->search(MULTI_CHAR('f_t00'))->hide();
#else
    J2DTextBox* textBox = (J2DTextBox*)mpScreen->search(MULTI_CHAR('f_t00'));
    mpScreen->search(MULTI_CHAR('t_t00'))->hide();
#endif
    textBox->setFont(mDoExt_getSubFont());
    textBox->setString(0x200, "");
    mpString->getString(0x5ba, textBox, NULL, NULL, NULL, 0);
}

void dMenu_Insect_c::screenSetExplain() {
    mpExpScreen = JKR_NEW J2DScreen();
    mpExpScreen->setPriority("zelda_gold_insects_info.blo", 0x20000, mpArchive);
    dPaneClass_showNullPane(mpExpScreen);
    mpExpParent = JKR_NEW CPaneMgr(mpExpScreen, MULTI_CHAR('n_all'), 2, NULL);
    mpExpParent->setAlphaRate(0.0f);
    mpExpSubWin[0] = JKR_NEW CPaneMgr(mpExpScreen, MULTI_CHAR('in_win_n'), 0, NULL);
    mpExpSubWin[1] = JKR_NEW CPaneMgr(mpExpScreen, MULTI_CHAR('w_d_mo_n'), 0, NULL);
    if (field_0xf6 == 0) {
        mpExpSubWin[1]->hide();
    }
#if TARGET_PC
    if (dusk::tphd_active()) {
        // RPX 0x029e8ccc with the HIO of 0x02a4ea38: page at (0, 77), give window at (20, 43)
        // on a 16:9 TV.
        if (mpExpSubWin[0] != NULL) {
            mpExpSubWin[0]->paneTrans(0.0f, 77.0f);
        }
        mpInfoText = JKR_NEW CPaneMgr(mpExpScreen, MULTI_CHAR('mg_3line'), 0, NULL);
        s_tphdInsect.mpTitleText = JKR_NEW CPaneMgr(mpExpScreen, MULTI_CHAR('mg_3lin1'), 0, NULL);
        J2DPane* e4 = mpExpScreen->search(MULTI_CHAR('n_e4line'));
        if (e4 != NULL) {
            e4->hide();
        }
        J2DPane* wani = mpExpScreen->search(MULTI_CHAR('waning_n'));
        if (wani != NULL) {
            wani->hide();
        }
        field_0x5c = (J2DTextBox*)mpExpScreen->search(MULTI_CHAR('ms_for_2'));
        if (field_0x5c == NULL) {
            field_0x5c = (J2DTextBox*)mpExpScreen->search(MULTI_CHAR('w_msg_jp'));
        }
        J2DPane* ms3 = mpExpScreen->search(MULTI_CHAR('ms_for_3'));
        if (ms3 != NULL) {
            ms3->hide();
        }
        field_0x54[0] = (J2DPicture*)mpExpScreen->search(MULTI_CHAR('insects'));
        field_0x54[1] = (J2DPicture*)mpExpScreen->search(MULTI_CHAR('insectss'));

        // RPX 0x029e8748: description (mg_3line) and name (mg_3lin1), message font, layout size.
        J2DTextBox* tbDesc = (J2DTextBox*)mpInfoText->getPanePtr();
        if (tbDesc != NULL) {
            tbDesc->setFont(mDoExt_getMesgFont());
            tbDesc->setCharSpace(0.0f);  // RPX: +0x10c (Wii U J2DTextBox char spacing) = 0
            tbDesc->setString(0x400, "");
        }
        J2DTextBox* tbTitle = (J2DTextBox*)s_tphdInsect.mpTitleText->getPanePtr();
        if (tbTitle != NULL) {
            tbTitle->setFont(mDoExt_getMesgFont());
            tbTitle->setString(0x400, "");
        }
        if (J2DPane* wani = mpExpScreen->search(MULTI_CHAR('waning_n'))) {
            wani->translate(wani->getTranslateX() + 20.0f, wani->getTranslateY() + 43.0f);
        }
        if (field_0x5c != NULL) {
            field_0x5c->setFont(mDoExt_getMesgFont());
            field_0x5c->setString(0x100, "");
        }
        return;
    }
    s_tphdInsect.mpTitleText = NULL;
    if (dusk::version::isRegionJpn()) {
        mpInfoText = JKR_NEW CPaneMgr(mpExpScreen, MULTI_CHAR('mg_3line'), 0, NULL);
        mpExpScreen->search(MULTI_CHAR('n_e4line'))->hide();
        field_0x5c = (J2DTextBox*)mpExpScreen->search(MULTI_CHAR('w_msg_jp'));
        mpExpScreen->search(MULTI_CHAR('ms_for_2'))->hide();
    } else {
        mpInfoText = JKR_NEW CPaneMgr(mpExpScreen, MULTI_CHAR('mg_e4lin'), 0, NULL);
        mpExpScreen->search(MULTI_CHAR('n_3line'))->hide();
        field_0x5c = (J2DTextBox*)mpExpScreen->search(MULTI_CHAR('ms_for_2'));
        mpExpScreen->search(MULTI_CHAR('w_msg_jp'))->hide();
    }
#elif VERSION == VERSION_GCN_JPN
    mpInfoText = JKR_NEW CPaneMgr(mpExpScreen, MULTI_CHAR('mg_3line'), 0, NULL);
    mpExpScreen->search(MULTI_CHAR('n_e4line'))->hide();
    field_0x5c = (J2DTextBox*)mpExpScreen->search(MULTI_CHAR('w_msg_jp'));
    mpExpScreen->search(MULTI_CHAR('ms_for_2'))->hide();
#else
    mpInfoText = JKR_NEW CPaneMgr(mpExpScreen, MULTI_CHAR('mg_e4lin'), 0, NULL);
    mpExpScreen->search(MULTI_CHAR('n_3line'))->hide();
    field_0x5c = (J2DTextBox*)mpExpScreen->search(MULTI_CHAR('ms_for_2'));
    mpExpScreen->search(MULTI_CHAR('w_msg_jp'))->hide();
#endif
    mpExpScreen->search(MULTI_CHAR('ms_for_3'))->hide();
    field_0x54[0] = (J2DPicture*)mpExpScreen->search(MULTI_CHAR('insects'));
    field_0x54[1] = (J2DPicture*)mpExpScreen->search(MULTI_CHAR('insectss'));
    J2DTextBox* textBox = (J2DTextBox*)mpInfoText->getPanePtr();
    textBox->setFont(mDoExt_getMesgFont());
    J2DTextBox* textBox2 = (J2DTextBox*)mpInfoText->getPanePtr();
    textBox2->setString(DUSK_IF_ELSE(0x400, 0x200), "");
    field_0x5c->setFont(mDoExt_getMesgFont());
    field_0x5c->setString(0x100, "");
}

void dMenu_Insect_c::screenSetDoIcon() {
    static const u64 text_a_tag[5] = {
        MULTI_CHAR('atext1_1'), MULTI_CHAR('atext1_2'), MULTI_CHAR('atext1_3'), MULTI_CHAR('atext1_4'), MULTI_CHAR('atext1_5'),
    };
    static const u64 text_b_tag[5] = {
        MULTI_CHAR('btext1_1'), MULTI_CHAR('btext1_2'), MULTI_CHAR('btext1_3'), MULTI_CHAR('btext1_4'), MULTI_CHAR('btext1_5'),
    };

    mpIconScreen = JKR_NEW J2DScreen();
#if TARGET_PC
    const char* iconBlo = dusk::tphd_active() ? "zelda_collect_soubi_do_icon_parts_revo.blo"
                                              : "zelda_collect_soubi_do_icon_parts.blo";
    mpIconScreen->setPriority(iconBlo, 0x20000, mpArchive);
#else
    mpIconScreen->setPriority("zelda_collect_soubi_do_icon_parts.blo", 0x20000, mpArchive);
#endif
    for (int i = 0; i < 2; i++) {
        mpButtonAB[i] = 0;
        mpButtonText[i] = 0;
    }
    dPaneClass_showNullPane(mpIconScreen);
#if TARGET_PC
    if (dusk::tphd_active()) {
        dusk::tphd::setupRevoButtonText(mpIconScreen);
    }
#endif
    for (int i = 0; i < 5; i++) {
        mpAButtonString[i] = (J2DTextBox*)mpIconScreen->search(text_a_tag[i]);
        mpBButtonString[i] = (J2DTextBox*)mpIconScreen->search(text_b_tag[i]);
        if (mpAButtonString[i] != NULL) {
            mpAButtonString[i]->setFont(mDoExt_getMesgFont());
            mpAButtonString[i]->setString(0x20, "");
        }
        if (mpBButtonString[i] != NULL) {
            mpBButtonString[i]->setFont(mDoExt_getMesgFont());
            mpBButtonString[i]->setString(0x20, "");
        }
    }
}

u8 dMenu_Insect_c::getGetInsectNum() {
    static u8 l_itemno[MAX_INSECT_NUM] = {
        dItemNo_M_BEETLE_e,      dItemNo_F_BEETLE_e,      dItemNo_M_BUTTERFLY_e, dItemNo_F_BUTTERFLY_e, dItemNo_M_STAG_BEETLE_e, dItemNo_F_STAG_BEETLE_e,
        dItemNo_M_GRASSHOPPER_e, dItemNo_F_GRASSHOPPER_e, dItemNo_M_NANAFUSHI_e, dItemNo_F_NANAFUSHI_e, dItemNo_M_DANGOMUSHI_e,  dItemNo_F_DANGOMUSHI_e,
        dItemNo_M_MANTIS_e,      dItemNo_F_MANTIS_e,      dItemNo_M_LADYBUG_e,   dItemNo_F_LADYBUG_e,   dItemNo_M_SNAIL_e,       dItemNo_F_SNAIL_e,
        dItemNo_M_DRAGONFLY_e,   dItemNo_F_DRAGONFLY_e,   dItemNo_M_ANT_e,       dItemNo_F_ANT_e,       dItemNo_M_MAYFLY_e,      dItemNo_F_MAYFLY_e,
    };

    u8 insectNum = 0;
    u8* insectList = l_itemno;
    for (u32 i = 0; i < MAX_INSECT_NUM; i++) {
        u8 insectId = *insectList;
        insectList++;

        if (dComIfGs_isItemFirstBit(insectId) != 0) {
            insectNum++;
        }
    }
    return insectNum;
}

u8 dMenu_Insect_c::getInsectItemID(int param_0, int param_1) {
    static u8 l_itemno[MAX_INSECT_NUM] = {
        dItemNo_M_ANT_e,       dItemNo_F_ANT_e,       dItemNo_M_MAYFLY_e,      dItemNo_F_MAYFLY_e,      dItemNo_M_BEETLE_e,     dItemNo_F_BEETLE_e,
        dItemNo_M_MANTIS_e,    dItemNo_F_MANTIS_e,    dItemNo_M_STAG_BEETLE_e, dItemNo_F_STAG_BEETLE_e, dItemNo_M_DANGOMUSHI_e, dItemNo_F_DANGOMUSHI_e,
        dItemNo_M_BUTTERFLY_e, dItemNo_F_BUTTERFLY_e, dItemNo_M_LADYBUG_e,     dItemNo_F_LADYBUG_e,     dItemNo_M_SNAIL_e,      dItemNo_F_SNAIL_e,
        dItemNo_M_NANAFUSHI_e, dItemNo_F_NANAFUSHI_e, dItemNo_M_GRASSHOPPER_e, dItemNo_F_GRASSHOPPER_e, dItemNo_M_DRAGONFLY_e,  dItemNo_F_DRAGONFLY_e,
    };

    int index = param_0 + param_1 * 6;
    return l_itemno[index];
}

bool dMenu_Insect_c::isGetInsect(int param_0, int param_1) {
    return dComIfGs_isItemFirstBit(getInsectItemID(param_0, param_1)) != 0;
}

bool dMenu_Insect_c::isGiveInsect(int param_0, int param_1) {
    static const u32 i_evtID[MAX_INSECT_NUM] = {
        0x1A5, /* dSv_event_flag_c::F_0421 - Misc. - Ant (M) */
        0x1A6, /* dSv_event_flag_c::F_0422 - Misc. - Ant (F) */
        0x1A7, /* dSv_event_flag_c::F_0423 - Misc. - Dayfly (M) */
        0x1A8, /* dSv_event_flag_c::F_0424 - Misc. - Dayfly (F) */
        0x191, /* dSv_event_flag_c::F_0401 - Misc. - Beetle (M) */
        0x192, /* dSv_event_flag_c::F_0402 - Misc. - Beetle (F) */
        0x19D, /* dSv_event_flag_c::F_0413 - Misc. - Mantis (M) */
        0x19E, /* dSv_event_flag_c::F_0414 - Misc. - Mantis (F) */
        0x195, /* dSv_event_flag_c::F_0405 - Misc. - Stag beetle (M) */
        0x196, /* dSv_event_flag_c::F_0406 - Misc. - Stag beetle (F) */
        0x19B, /* dSv_event_flag_c::F_0411 - Misc. - Pill bug (M) */
        0x19C, /* dSv_event_flag_c::F_0412 - Misc. - Pill bug (F) */
        0x193, /* dSv_event_flag_c::F_0403 - Misc. - Butterfly (M) */
        0x194, /* dSv_event_flag_c::F_0404 - Misc. - Butterfly (F) */
        0x19F, /* dSv_event_flag_c::F_0415 - Misc. - Ladybug (M) */
        0x1A0, /* dSv_event_flag_c::F_0416 - Misc. - Ladybug (F) */
        0x1A1, /* dSv_event_flag_c::F_0417 - Misc. - Snail (M) */
        0x1A2, /* dSv_event_flag_c::F_0418 - Misc. - Snail (F) */
        0x199, /* dSv_event_flag_c::F_0409 - Misc. - Phasmid (M) */
        0x19A, /* dSv_event_flag_c::F_0410 - Misc. - Phasmid (F) */
        0x197, /* dSv_event_flag_c::F_0407 - Misc. - Grasshopper (M) */
        0x198, /* dSv_event_flag_c::F_0408 - Misc. - Grasshopper (F) */
        0x1A3, /* dSv_event_flag_c::F_0419 - Misc. - Dragonfly (M) */
        0x1A4, /*dSv_event_flag_c::F_0420 - Misc. - Dragonfly (F) */
    };

    return dComIfGs_isEventBit(dSv_event_flag_c::saveBitLabels[i_evtID[param_0 + param_1 * 6]]) != 0;
}

bool dMenu_Insect_c::isCatchInsect(u8 i_insectID) {
    if (i_insectID >= dItemNo_M_BEETLE_e && i_insectID <= dItemNo_F_MAYFLY_e) {
        return dComIfGs_isItemFirstBit(i_insectID);
    }
    return false;
}

bool dMenu_Insect_c::isGiveInsect(u8 i_insectID) {
    if (i_insectID >= dItemNo_M_BEETLE_e && i_insectID <= dItemNo_F_MAYFLY_e) {
        int label = i_insectID + 0xd1;
        return dComIfGs_isEventBit(dSv_event_flag_c::saveBitLabels[label]);
    }
    return false;
}

bool dMenu_Insect_c::isCatchNotGiveInsect(u8 param_0) {
    if (isCatchInsect(param_0) != 0 && isGiveInsect(param_0) == 0) {
        return 1;
    }
    return 0;
}

void dMenu_Insect_c::cursorMove() {
    u8 oldF4 = field_0xf4;
    u8 oldF5 = field_0xf5;
    u8 dpd = 0;
    mpStick->checkTrigger();
    if (mpStick->checkRightTrigger()) {
        if (field_0xf4 < 5) {
            field_0xf4++;
        }
    } else if (mpStick->checkLeftTrigger()) {
        if (field_0xf4 != 0) {
            field_0xf4--;
        }
    }
    if (mpStick->checkUpTrigger()) {
        if (field_0xf5 != 0) {
            field_0xf5--;
        }
    } else if (mpStick->checkDownTrigger()) {
        if (field_0xf5 < 3) {
            field_0xf5++;
        }
    }
    if (oldF4 == field_0xf4 && oldF5 == field_0xf5) {
        field_0xf8 = 0xff;
        field_0xf9 = 0xff;
        if (mpStick->getValueStick() <= 0.75f) {
            dpd = dpdMove();
        }
        if (field_0xf8 != 0xff && field_0xf9 != 0xff) {
            if (field_0xf8 != field_0xfa || field_0xf9 != field_0xfb) {
                if (field_0xf8 != field_0xf4 || field_0xf9 != field_0xf5) {
                    field_0xf4 = field_0xf8;
                    field_0xf5 = field_0xf9;
                    field_0xfa = field_0xf8;
                    field_0xfb = field_0xf9;
                }
            }
        }
    }
    if (oldF4 == field_0xf4 && oldF5 == field_0xf5) {
        return;
    }
    setCursorPos();
#if TARGET_PC
    if (dusk::tphd_active()) {
        updateInsectInfo();
        if (field_0xf6 == 1) {
            if (isGetInsect(field_0xf4, field_0xf5) && !isGiveInsect(field_0xf4, field_0xf5)) {
                setAButtonString(0x40c);
            } else {
                setAButtonString(0);
            }
        } else {
            setAButtonString(0);
        }
    } else {
        if (isGetInsect(field_0xf4, field_0xf5) != 0) {
            setAButtonString(0x368);
        } else {
            setAButtonString(0);
        }
    }
#else
    if (isGetInsect(field_0xf4, field_0xf5) != 0) {
        setAButtonString(0x368);
    } else {
        setAButtonString(0);
    }
#endif
    if (dpd != 0) {
        Z2GetAudioMgr()->seStart(Z2SE_SY_CURSOR_ITEM, NULL, 1, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
        dMeter2Info_set2DVibration();
    } else {
        Z2GetAudioMgr()->seStart(Z2SE_SY_CURSOR_ITEM, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
    }
}

void dMenu_Insect_c::setCursorPos() {
    int index = field_0xf4 + field_0xf5 * 6;
#if TARGET_PC
    if (dusk::tphd_active()) {
        for (int i = 0; i < MAX_INSECT_NUM; i++) {
            const bool caught = isGetInsect(i % 6, i / 6);
            if (i == index) {
                if (s_tphdInsect.mpBox[i] != NULL) {
                    s_tphdInsect.mpBox[i]->hide();
                }
                if (s_tphdInsect.mpBoxSelected[i] != NULL) {
                    s_tphdInsect.mpBoxSelected[i]->show();
                }
                if (caught) {
                    if (mpINSParent[i] != NULL) {
                        mpINSParent[i]->hide();
                    }
                    if (s_tphdInsect.mpINSParentSel[i] != NULL) {
                        s_tphdInsect.mpINSParentSel[i]->show();
                    }
                }
            } else {
                if (s_tphdInsect.mpBox[i] != NULL) {
                    s_tphdInsect.mpBox[i]->show();
                }
                if (s_tphdInsect.mpBoxSelected[i] != NULL) {
                    s_tphdInsect.mpBoxSelected[i]->hide();
                }
                if (caught) {
                    if (mpINSParent[i] != NULL) {
                        mpINSParent[i]->show();
                    }
                    if (s_tphdInsect.mpINSParentSel[i] != NULL) {
                        s_tphdInsect.mpINSParentSel[i]->hide();
                    }
                }
            }
        }

        // The cursor itself is placed on the drawn grid in _draw.
        return;
    }
#endif
    Vec pos = mpINSParent[index]->getGlobalVtxCenter(false, 0);
    mpDrawCursor->setPos(pos.x, pos.y, mpINSParent[index]->getPanePtr(), false);
    for (int i = 0; i < MAX_INSECT_NUM; i++) {
        if (i == index) {
            mpINSParent[i]->scale(g_drawHIO.mInsectListScreen.mSelectInsectScale,
                                  g_drawHIO.mInsectListScreen.mSelectInsectScale);
        } else {
            mpINSParent[i]->scale(g_drawHIO.mInsectListScreen.mUnselectInsectScale,
                                  g_drawHIO.mInsectListScreen.mUnselectInsectScale);
        }
    }
}

u8 dMenu_Insect_c::dpdMove() {
    return 0;
}

void dMenu_Insect_c::setAButtonString(u16 i_stringID) {
#if TARGET_PC
    if (dusk::tphd_active()) {
        dusk::tphd::setRevoButtonText(mpIconScreen, i_stringID, true);
        return;
    }
#endif
    if (i_stringID == 0) {
        for (int i = 0; i < 5; i++) {
            if (mpAButtonString[i] != NULL) {
                SAFE_STRCPY(mpAButtonString[i]->getStringPtr(), "");
            }
        }
    } else {
        for (int i = 0; i < 5; i++) {
            if (mpAButtonString[i] != NULL) {
                dMeter2Info_getStringKanji(i_stringID, mpAButtonString[i]->getStringPtr(), NULL);
            }
        }
    }
}

void dMenu_Insect_c::setBButtonString(u16 i_stringID) {
#if TARGET_PC
    if (dusk::tphd_active()) {
        dusk::tphd::setRevoButtonText(mpIconScreen, i_stringID, false);
        return;
    }
#endif
    if (i_stringID == 0) {
        for (int i = 0; i < 5; i++) {
            if (mpBButtonString[i] != NULL) {
                SAFE_STRCPY(mpBButtonString[i]->getStringPtr(), "");
            }
        }
    } else {
        for (int i = 0; i < 5; i++) {
            if (mpBButtonString[i] != NULL) {
                dMeter2Info_getStringKanji(i_stringID, mpBButtonString[i]->getStringPtr(), NULL);
            }
        }
    }
}

void dMenu_Insect_c::setHIO(bool i_useHIO) {
#if TARGET_PC
    if (dusk::tphd_active()) {
        return;
    }
#endif
    if (i_useHIO || g_drawHIO.mInsectListScreen.mDebugON) {
        mpExpSubWin[0]->paneTrans(g_drawHIO.mInsectListScreen.mDescWindowPosX,
                                  g_drawHIO.mInsectListScreen.mDescWindowPosY);
        mpExpSubWin[1]->paneTrans(g_drawHIO.mInsectListScreen.mGiveOptionPosX_4x3,
                                  g_drawHIO.mInsectListScreen.mGiveOptionPosY_4x3);
    }
    if (g_drawHIO.mCollectScreen.mButtonDebugON != false || i_useHIO) {
        if (mpButtonAB[0]) {
            mpButtonAB[0]->paneTrans(g_drawHIO.mCollectScreen.mButtonAPosX,
                                     g_drawHIO.mCollectScreen.mButtonAPosY);
            mpButtonAB[0]->scale(g_drawHIO.mCollectScreen.mButtonAScale,
                                 g_drawHIO.mCollectScreen.mButtonAScale);
        }
        if (mpButtonAB[1]) {
            mpButtonAB[1]->paneTrans(g_drawHIO.mCollectScreen.mButtonBPosX,
                                     g_drawHIO.mCollectScreen.mButtonBPosY);
            mpButtonAB[1]->scale(g_drawHIO.mCollectScreen.mButtonBScale,
                                 g_drawHIO.mCollectScreen.mButtonBScale);
        }
        if (mpButtonText[0]) {
            mpButtonText[0]->paneTrans(g_drawHIO.mCollectScreen.mButtonATextPosX,
                                       g_drawHIO.mCollectScreen.mButtonATextPosY);
            mpButtonText[0]->scale(g_drawHIO.mCollectScreen.mButtonATextScale,
                                   g_drawHIO.mCollectScreen.mButtonATextScale);
        }
        if (mpButtonText[1]) {
            mpButtonText[1]->paneTrans(g_drawHIO.mCollectScreen.mButtonBTextPosX,
                                       g_drawHIO.mCollectScreen.mButtonBTextPosY);
            mpButtonText[1]->scale(g_drawHIO.mCollectScreen.mButtonBTextScale,
                                   g_drawHIO.mCollectScreen.mButtonBTextScale);
        }
    }
}

#if TARGET_PC
// RPX 0x029e8f2c: puts the insect of the cell on the right page (big icon, name, description).
void dMenu_Insect_c::updateInsectInfo() {
    if (!dusk::tphd_active() || mpExpScreen == NULL) {
        return;
    }
    // The page cross-fades in moveInsectInfo when the cell changes.
}

void dMenu_Insect_c::setInsectInfo(u8 i_itemNo) {
    s32 i_textureNum = dMeter2Info_readItemTexture(i_itemNo, mpExpItemTex, NULL, NULL, NULL, NULL,
                                                   NULL, NULL, NULL, -1);
    (void)i_textureNum;
    const f32 tex = mpExpItemTex->width / 48.0f;
    const f32 tex2 = mpExpItemTex->height / 48.0f;
    for (int i = 0; i < 2; i++) {
        if (field_0x54[i] != NULL) {
            field_0x54[i]->changeTexture(mpExpItemTex, 0);
            field_0x54[i]->scale(tex, tex2);
        }
    }
    if (mpInfoText != NULL && mpInfoText->getPanePtr() != NULL) {
        mpString->getString(i_itemNo + 0x265, (J2DTextBox*)mpInfoText->getPanePtr(), NULL, NULL,
                            NULL, 0);
    }
    if (s_tphdInsect.mpTitleText != NULL && s_tphdInsect.mpTitleText->getPanePtr() != NULL) {
        mpString->getString(i_itemNo + 0x165, (J2DTextBox*)s_tphdInsect.mpTitleText->getPanePtr(),
                            NULL, NULL, NULL, 0);
    }
}

// RPX 0x029e9228 / 0x029e9188 / 0x029e91d8: the page fades out (10 frames) when the cursor
// leaves the insect shown, and the new one fades in (10 frames) if it has been caught.
void dMenu_Insect_c::moveInsectInfo() {
    if (mpExpParent == NULL) {
        return;
    }
    const u8 item = getInsectItemID(field_0xf4, field_0xf5);
    TphdInsectState& st = s_tphdInsect;
    if (st.mInfoState == 0) {
        if (isGetInsect(field_0xf4, field_0xf5)) {
            setInsectInfo(item);
            st.mInfoShown = item;
            st.mInfoState = 1;
            st.mInfoFrame = 0;
        } else {
            st.mInfoShown = item;
        }
    } else if ((st.mInfoState == 1 || st.mInfoState == 2) && item != st.mInfoShown) {
        st.mInfoState = 3;
    }
    if (st.mInfoState == 0 && item != st.mInfoShown) {
        st.mInfoShown = 0xff;
    }

    if (st.mInfoState == 1) {
        if (++st.mInfoFrame >= 10) {
            st.mInfoFrame = 10;
            st.mInfoState = 2;
        }
        mpExpParent->setAlphaRate(st.mInfoFrame / 10.0f);
    } else if (st.mInfoState == 3) {
        if (--st.mInfoFrame <= 0) {
            st.mInfoFrame = 0;
            st.mInfoState = 0;
            st.mInfoShown = 0xff;
        }
        mpExpParent->setAlphaRate(st.mInfoFrame / 10.0f);
    } else if (st.mInfoState == 0) {
        mpExpParent->setAlphaRate(0.0f);
    }
}
#endif
