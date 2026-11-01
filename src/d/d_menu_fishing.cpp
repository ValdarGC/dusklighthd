/**
 * d_menu_fishing.cpp
 * Menu - Fishing Journal
 */

#include "d/dolzel.h" // IWYU pragma: keep

#include "d/d_menu_fishing.h"
#include "JSystem/J2DGraph/J2DTextBox.h"
#include "JSystem/JKernel/JKRMemArchive.h"
#include "d/d_lib.h"
#include "d/d_menu_dmap.h"
#include "d/d_meter2_info.h"
#include "d/d_meter_HIO.h"
#include "d/d_msg_string.h"
#include "m_Do/m_Do_controller_pad.h"
#include "m_Do/m_Do_graphic.h"
#include <cstring>

#if TARGET_PC
#include "dusk/version.hpp"
#include "helpers/string.hpp"
#endif

#if TARGET_PC
#include "d/d_select_cursor.h"
#include "dusk/menu_pointer.h"
#include "dusk/tphd/HdSubscreen.hpp"

extern JKRArchive* g_tphdMain2DRevoArchive;

namespace {
struct TphdFishingState {
    dSelect_cursor_c* mpDrawCursor = nullptr;
    CPaneMgr* mpFishPict[6] = {};
    CPaneMgr* mpFishThumb[6] = {};
    J2DTextBox* mpHeaderTitle = nullptr;
    J2DTextBox* mpDescTitle = nullptr;
    J2DTextBox* mpDescText = nullptr;
    J2DTextBox* mpRecordTitle = nullptr;
    J2DTextBox* mpLargestLabel = nullptr;
    J2DTextBox* mpLargestValue = nullptr;
    J2DTextBox* mpLargestUnit = nullptr;
    J2DTextBox* mpCaughtLabel = nullptr;
    J2DTextBox* mpCaughtValue = nullptr;
    J2DTextBox* mpCaughtUnit = nullptr;
    int mSelectedFish = 0;
    bool mFirstDraw = true;

    void reset() {
        if (mpDrawCursor != NULL) {
            JKR_DELETE(mpDrawCursor);
            mpDrawCursor = NULL;
        }
        for (int i = 0; i < 6; i++) {
            if (mpFishPict[i] != NULL) {
                JKR_DELETE(mpFishPict[i]);
                mpFishPict[i] = NULL;
            }
            if (mpFishThumb[i] != NULL) {
                JKR_DELETE(mpFishThumb[i]);
                mpFishThumb[i] = NULL;
            }
        }
        mpHeaderTitle = NULL;
        mpDescTitle = NULL;
        mpDescText = NULL;
        mpRecordTitle = NULL;
        mpLargestLabel = NULL;
        mpLargestValue = NULL;
        mpLargestUnit = NULL;
        mpCaughtLabel = NULL;
        mpCaughtValue = NULL;
        mpCaughtUnit = NULL;
        mSelectedFish = 0;
        mFirstDraw = true;
    }
} s_tphdFishing;
}  // namespace
#endif

typedef void (dMenu_Fishing_c::*initFunc)();
DUSK_GAME_DATA initFunc map_init_process[] = {
    &dMenu_Fishing_c::wait_init,
};

typedef void (dMenu_Fishing_c::*moveFunc)();
DUSK_GAME_DATA moveFunc map_move_process[] = {
    &dMenu_Fishing_c::wait_move,
};

dMenu_Fishing_c::dMenu_Fishing_c(JKRExpHeap* i_heap, STControl* i_stick, CSTControl* i_cStick) {
    mpHeap = i_heap;
    mpArchive = 0;
    mpMount = 0;
    mpStick = i_stick;
    mpCStick = i_cStick;
    mStatus = READY_OPEN;
    mProcess = 0;
    mFishListScreenFrames = 0;
    mpString = NULL;
    mpBlackTex = NULL;
    mpScreen = NULL;
    mpParent = NULL;
    mpIconScreen = NULL;
    mpInfoLargestString = NULL;
    mpInfoNumCaughtString = NULL;
    for (int i = 0; i < MAX_FINDABLE_FISHES; i++) {
        mpFishParent[i] = NULL;
        for (int j = 0; j < MAX_FINDABLE_FISHES; j++) {
            mpFishParts[j][i] = NULL;
        }
        mpFishNameString[i] = NULL;
        for (int k = 0; k < 2; k++) {
            field_0x10c[k][i] = NULL;
            field_0x13c[k][i] = NULL;
            field_0x16c[k][i] = NULL;
            field_0x19c[k][i] = NULL;
        }
    }
    for (int i = 0; i < 2; i++) {
        mpFishInfoParent[i] = NULL;
        mpButtonAB[i] = NULL;
        mpButtonText[i] = NULL;
    }
    for (int i = 0; i < 5; i++) {
        mpAButtonString[i] = NULL;
        mpBButtonString[i] = NULL;
    }
#if TARGET_PC
    s_tphdFishing.reset();
#endif
}

dMenu_Fishing_c::~dMenu_Fishing_c() {
#if TARGET_PC
    s_tphdFishing.reset();
#endif

    if (mpString) {
        JKR_DELETE(mpString);
        mpString = NULL;
    }

    if (mpBlackTex) {
        JKR_DELETE(mpBlackTex);
        mpBlackTex = NULL;
    }

    if (mpScreen) {
        JKR_DELETE(mpScreen);
        mpScreen = NULL;
    }

    if (mpParent) {
        JKR_DELETE(mpParent);
        mpParent = NULL;
    }

    for (int i = 0; i < MAX_FINDABLE_FISHES; i++) {
        if (mpFishParent[i]) {
            JKR_DELETE(mpFishParent[i]);
            mpFishParent[i] = NULL;
        }

        for (int j = 0; j < MAX_FINDABLE_FISHES; j++) {
            if (mpFishParts[j][i]) {
                JKR_DELETE(mpFishParts[j][i]);
                mpFishParts[j][i] = NULL;
            }
        }
    }

    for (int i = 0; i < 2; i++) {
        if (mpFishInfoParent[i]) {
            JKR_DELETE(mpFishInfoParent[i]);
            mpFishInfoParent[i] = NULL;
        }
    }

    if (mpIconScreen) {
        JKR_DELETE(mpIconScreen);
        mpIconScreen = NULL;
    }

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

    if (mpMount) {
        JKR_DELETE(mpMount);
        mpMount = NULL;
    }

    if (mpArchive) {
        mpArchive->unmount();
        mpArchive = NULL;
    }
}

void dMenu_Fishing_c::_create() {
#if TARGET_PC
    if (dusk::tphd_active()) {
        s_tphdFishing.mpDrawCursor = JKR_NEW dSelect_cursor_c(2, 1.0f, g_tphdMain2DRevoArchive);
        s_tphdFishing.mpDrawCursor->setParam(1.0f, 1.0f, 0.1f, 0.7f, 0.7f);
        s_tphdFishing.mpDrawCursor->setAlphaRate(0.0f);
        s_tphdFishing.mpDrawCursor->setScale(0.0f);
        s_tphdFishing.mpDrawCursor->offPlayAnime(0);
    }
#endif
    mpString = JKR_NEW dMsgString_c();
    screenSetBase();
    screenSetDoIcon();
    setHIO(true);
    init();
}

void dMenu_Fishing_c::_move() {
    JKRHeap* heap = mDoExt_setCurrentHeap((JKRHeap*)mpHeap);
    u8 old_process = mProcess;
    (this->*map_move_process[mProcess])();
    if (old_process != mProcess) {
        (this->*map_init_process[mProcess])();
    }
    setHIO(false);
    mDoExt_setCurrentHeap(heap);
}

void dMenu_Fishing_c::_draw() {
    if (mpArchive && mpScreen) {
        J2DGrafContext* grafPort = dComIfGp_getCurrentGrafPort();
        if (mpBlackTex) {
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
            // RPX 0x029cf404: black backdrop, journal, A/B icons, then the cursor.
            dusk::tphd::drawRevoScreen(mpScreen, grafPort);
            dusk::tphd::drawRevoIconScreen(mpIconScreen, grafPort);
            setCursorPos(s_tphdFishing.mSelectedFish);
            if (s_tphdFishing.mpDrawCursor != NULL) {
                s_tphdFishing.mpDrawCursor->draw();
            }
        } else {
            mpScreen->draw(0.0f, 0.0f, grafPort);
            mpIconScreen->draw(0.0f, 0.0f, grafPort);
        }
#else
        mpScreen->draw(0.0f, 0.0f, grafPort);
        mpIconScreen->draw(0.0f, 0.0f, grafPort);
#endif
    }
}

bool dMenu_Fishing_c::isSync() {
    if (mpMount != NULL && !mpMount->sync()) {
        return false;
    }
    return true;
}

void dMenu_Fishing_c::init() {
#if TARGET_PC
    if (dusk::tphd_active()) {
        // RPX 0x029ced70: only the fish caught at least once are on the page.
        for (int i = 0; i < 6; i++) {
            if (mpFishParent[i] != NULL) {
                if (dComIfGs_getFishNum(i) != 0) {
                    mpFishParent[i]->show();
                } else {
                    mpFishParent[i]->hide();
                }
            }
        }
        // RPX 0x029cf1f0: first fish in page order (top left, then down the left column...).
        static const int kFirstOrder[6] = {5, 3, 2, 0, 4, 1};
        int first = 5;
        for (int i = 0; i < 6; i++) {
            if (dComIfGs_getFishNum(kFirstOrder[i]) != 0) {
                first = kFirstOrder[i];
                break;
            }
        }
        selectFish(first);
        (this->*map_init_process[mProcess])();
        return;
    }
#endif
    #if TARGET_PC || VERSION == VERSION_GCN_PAL
    BOOL isEnglish = FALSE;
    if (dusk::version::isRegionPal() && dComIfGs_getPalLanguage() == dSv_player_config_c::LANGUAGE_ENGLISH) {
        isEnglish = TRUE;
    }
    #endif

    for (int i = 0; i < MAX_FINDABLE_FISHES; i++) {
        if (dComIfGs_getFishNum(i) != 0) {
            // Fish has been caught once, display it along with it's params
            #if TARGET_PC || VERSION == VERSION_GCN_PAL
            if (isEnglish) {
                setFishParam(i, dComIfGs_getFishNum(i), dComIfGs_getFishSize(i) / 2.54f);
            } else {
                setFishParam(i, dComIfGs_getFishNum(i), dComIfGs_getFishSize(i));
            }
            #else
            setFishParam(i, dComIfGs_getFishNum(i), dComIfGs_getFishSize(i));
            #endif
            mpFishParent[i]->show();
        } else {
            // Fish hasn't been caught yet, don't display it and also hide params
            mpFishParent[i]->hide();
        }
    }
    (this->*map_init_process[mProcess])();
}

int dMenu_Fishing_c::_open() {
    if (mpMount == NULL) {
#if TARGET_PC
        const char* arcPath = dusk::tphd_active() ? "/res/LayoutRevo/fishresR.arc"
                                                  : "/res/Layout/fishres.arc";
        mpMount = mDoDvdThd_mountArchive_c::create(arcPath, 0, NULL);
#else
        mpMount = mDoDvdThd_mountArchive_c::create("/res/Layout/fishres.arc", 0, NULL);
#endif
    }
    if (mpArchive == NULL) {
        if (mpMount->sync()) {
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
    s16 openFrames = g_drawHIO.mFishListScreen.mOpenFrames;
    s16 closeFrames = g_drawHIO.mFishListScreen.mCloseFrames;
    mFishListScreenFrames = g_drawHIO.mFishListScreen.mOpenFrames;
    if (mFishListScreenFrames >= openFrames) {
        mFishListScreenFrames = closeFrames;
        mStatus = READY_MOVE;
        mpParent->scale(1.0f, 1.0f);
        mpParent->setAlphaRate(1.0f);
#if TARGET_PC
        if (dusk::tphd_active() && s_tphdFishing.mpDrawCursor != NULL) {
            setCursorPos(s_tphdFishing.mSelectedFish);
            s_tphdFishing.mpDrawCursor->setAlphaRate(1.0f);
            s_tphdFishing.mpDrawCursor->setScale(1.0f);
            s_tphdFishing.mpDrawCursor->onPlayAnime(0);
            s_tphdFishing.mpDrawCursor->onPlayAnime(1);
        }
#endif
        return 1;
    } else {
        f32 div = mFishListScreenFrames / (f32)openFrames;
        mpParent->scale(div, div);
        mpParent->setAlphaRate(div);
#if TARGET_PC
        if (dusk::tphd_active() && s_tphdFishing.mpDrawCursor != NULL) {
            setCursorPos(s_tphdFishing.mSelectedFish);
            s_tphdFishing.mpDrawCursor->setAlphaRate(div);
            s_tphdFishing.mpDrawCursor->setScale(div);
        }
#endif
        return 0;
    }
}

int dMenu_Fishing_c::_close() {
    s16 closeFrames = g_drawHIO.mFishListScreen.mCloseFrames;
    mFishListScreenFrames = 0;
    if (mFishListScreenFrames <= 0) {
        mFishListScreenFrames = 0;
        mStatus = CLOSED;
        mpParent->scale(0.0f, 0.0f);
        mpParent->setAlphaRate(0.0f);
#if TARGET_PC
        if (dusk::tphd_active() && s_tphdFishing.mpDrawCursor != NULL) {
            s_tphdFishing.mpDrawCursor->setAlphaRate(0.0f);
            s_tphdFishing.mpDrawCursor->setScale(0.0f);
        }
#endif
        return 1;
    } else {
        f32 div = mFishListScreenFrames / (f32)closeFrames;
        mpParent->scale(div, div);
        mpParent->setAlphaRate(div);
#if TARGET_PC
        if (dusk::tphd_active() && s_tphdFishing.mpDrawCursor != NULL) {
            s_tphdFishing.mpDrawCursor->setAlphaRate(div);
            s_tphdFishing.mpDrawCursor->setScale(div);
        }
#endif
        return 0;
    }
}

// While the fishing menu is open, 
void dMenu_Fishing_c::wait_init() {
    setAButtonString(0);
    setBButtonString(0x3F9); // "Back"
}

void dMenu_Fishing_c::wait_move() {
#if TARGET_PC
    if (dusk::tphd_active()) {
        if (mDoGph_gInf_c::getFader()->getStatus() == 1) {
            if (mDoCPd_c::getTrigB(PAD_1)) {
                mStatus = READY_CLOSE;
                return;
            }

            dusk::menu_pointer::begin_context(dusk::menu_pointer::Context::Collection);
            for (int i = 0; i < 6; i++) {
                if (dComIfGs_getFishNum(i) == 0) {
                    continue;
                }
                CPaneMgr* targetPane = s_tphdFishing.mpFishThumb[i] ? s_tphdFishing.mpFishThumb[i] : mpFishParent[i];
                if (targetPane != NULL && dusk::menu_pointer::hit_pane(targetPane, 8.0f)) {
                    dusk::menu_pointer::set_hover_target(i);
                    if (s_tphdFishing.mSelectedFish != i) {
                        Z2GetAudioMgr()->seStart(Z2SE_SY_CURSOR_ITEM, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
                        selectFish(i);
                    }
                    break;
                }
            }

            mpStick->checkTrigger();
            int next = s_tphdFishing.mSelectedFish;
            if (mpStick->checkLeftTrigger() || mDoCPd_c::getTrigLeft(PAD_1)) {
                next = checkLeft();
            } else if (mpStick->checkRightTrigger() || mDoCPd_c::getTrigRight(PAD_1)) {
                next = checkRight();
            } else if (mpStick->checkUpTrigger() || mDoCPd_c::getTrigUp(PAD_1)) {
                next = checkUp();
            } else if (mpStick->checkDownTrigger() || mDoCPd_c::getTrigDown(PAD_1)) {
                next = checkDown();
            }

            if (next != s_tphdFishing.mSelectedFish && next >= 0 && next < 6) {
                Z2GetAudioMgr()->seStart(Z2SE_SY_CURSOR_ITEM, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
                selectFish(next);
            }
        }
        return;
    }
#endif
    if (mDoGph_gInf_c::getFader()->getStatus() == 1 && mDoCPd_c::getTrigB(PAD_1)) {
        mStatus = READY_CLOSE;
    }
}

void dMenu_Fishing_c::screenSetBase() {
    static const u64 fish_n[6] = {
        MULTI_CHAR('fish_n_6'), MULTI_CHAR('fish_n_5'), MULTI_CHAR('fish_n_3'), MULTI_CHAR('fish_n_1'), MULTI_CHAR('fish_n_2'), MULTI_CHAR('fish_n_4'),
    };

    static const u64 fish_p0[6] = {
        MULTI_CHAR('fi_pa_6n'), MULTI_CHAR('fi_pa_5n'), MULTI_CHAR('fi_pa_3n'), MULTI_CHAR('fi_pa_1n'), MULTI_CHAR('fi_pa_2n'), MULTI_CHAR('fi_pa_4n'),
    };

    static const u64 fish_p1[6] = {
        MULTI_CHAR('fi_na_6n'), MULTI_CHAR('fi_na_5n'), MULTI_CHAR('fi_na_3n'), MULTI_CHAR('fi_na_1n'), MULTI_CHAR('fi_na_2n'), MULTI_CHAR('fi_na_4n'),
    };

    static const u64 fish_p2[6] = {MULTI_CHAR('fi_li_6n'), MULTI_CHAR('fi_li_5n'), MULTI_CHAR('fi_li_3n'),
                                   MULTI_CHAR('fi_li_1n'), MULTI_CHAR('fi_li_2n'), MULTI_CHAR('fi_li_4n')};

    static const u64 fish_p3[6] = {
        MULTI_CHAR('b_box_6n'), MULTI_CHAR('b_box_5n'), MULTI_CHAR('b_box_3n'), MULTI_CHAR('b_box_1n'), MULTI_CHAR('b_box_2n'), MULTI_CHAR('b_box_4n'),
    };

    static const u64 fish_p4[6] = {
        MULTI_CHAR('r_box_6n'), MULTI_CHAR('r_box_5n'), MULTI_CHAR('r_box_3n'), MULTI_CHAR('r_box_1n'), MULTI_CHAR('r_box_2n'), MULTI_CHAR('r_box_4n'),
    };

    static const u64 fish_p5[6] = {
        MULTI_CHAR('info_6_n'), MULTI_CHAR('info_5_n'), MULTI_CHAR('info_3_n'), MULTI_CHAR('info_1_n'), MULTI_CHAR('info_2_n'), MULTI_CHAR('info_4_n'),
    };

    static const u64 size_1[6] = {
        MULTI_CHAR('size_t_6'), MULTI_CHAR('size_t_5'), MULTI_CHAR('size_t_3'), MULTI_CHAR('size_t_1'), MULTI_CHAR('size_t_2'), MULTI_CHAR('size_t_4'),
    };

    static const u64 size_unit_1[6] = {
        MULTI_CHAR('cm_t_6'), MULTI_CHAR('cm_t_5'), MULTI_CHAR('cm_t_3'), MULTI_CHAR('cm_t_1'), MULTI_CHAR('cm_t_2'), MULTI_CHAR('cm_t_4'),
    };

    static const u64 count_1[6] = {
        MULTI_CHAR('count_t6'), MULTI_CHAR('count_t5'), MULTI_CHAR('count_t3'), MULTI_CHAR('count_t1'), MULTI_CHAR('count_t2'), MULTI_CHAR('count_t4'),
    };

    static const u64 count_unit_1[6] = {
        MULTI_CHAR('cou_t_6'), MULTI_CHAR('cou_t_5'), MULTI_CHAR('cou_t_3'), MULTI_CHAR('cou_t_1'), MULTI_CHAR('cou_t_2'), MULTI_CHAR('count_t4'),
    };

    static const u64 name_0[6] = {
        MULTI_CHAR('name_6'), MULTI_CHAR('name_5'), MULTI_CHAR('name_3'), MULTI_CHAR('name_1'), MULTI_CHAR('name_2'), MULTI_CHAR('name_4'),
    };

    static const u64 fname_0[6] = {
        MULTI_CHAR('f_name_6'), MULTI_CHAR('f_name_5'), MULTI_CHAR('f_name_3'), MULTI_CHAR('f_name_1'), MULTI_CHAR('f_name_2'), MULTI_CHAR('f_name_4'),
    };

    static const u32 name_id[6] = {
        0x59E, 0x59D, 0x59B, 0x599, 0x59A, 0x59C,
    };

    ResTIMG* TIMG = (ResTIMG*)dComIfGp_getMain2DArchive()->getResource('TIMG', "tt_block8x8.bti");
    mpBlackTex = JKR_NEW J2DPicture(TIMG);

    mpBlackTex->setBlackWhite(JUtility::TColor(0, 0, 0, 0), JUtility::TColor(0, 0, 0, 0xFF));
    mpScreen = JKR_NEW J2DScreen();
    mpScreen->setPriority("zelda_fish_window.blo", 0x20000, mpArchive);
    dPaneClass_showNullPane(mpScreen);
    mpParent = JKR_NEW CPaneMgr(mpScreen, MULTI_CHAR('n_all'), 2, NULL);
    mpParent->setAlphaRate(0.0f);

#if TARGET_PC
    if (dusk::tphd_active()) {
        static const u64 fish_cells[6] = {
            MULTI_CHAR('N_fish_6'), MULTI_CHAR('N_fish_5'), MULTI_CHAR('N_fish_3'),
            MULTI_CHAR('N_fish_1'), MULTI_CHAR('N_fish_2'), MULTI_CHAR('N_fish_4'),
        };
        static const u64 fish_thumbs[6] = {
            MULTI_CHAR('PICT_008'), MULTI_CHAR('PICT_001'), MULTI_CHAR('PICT_009'),
            MULTI_CHAR('PICT_006'), MULTI_CHAR('PICT_010'), MULTI_CHAR('PICT_007'),
        };
        static const u64 fish_picts[6] = {
            MULTI_CHAR('PICT_014'), MULTI_CHAR('PICT_011'), MULTI_CHAR('PICT_015'),
            MULTI_CHAR('PICT_012'), MULTI_CHAR('PICT_002'), MULTI_CHAR('PICT_013'),
        };
        static const u64 fish_fnames[6] = {
            MULTI_CHAR('f_name_6'), MULTI_CHAR('f_name_5'), MULTI_CHAR('f_name_3'),
            MULTI_CHAR('f_name_1'), MULTI_CHAR('f_name_2'), MULTI_CHAR('f_name_4'),
        };
        static const u64 fish_jnames[6] = {
            MULTI_CHAR('name_6'), MULTI_CHAR('name_5'), MULTI_CHAR('name_3'),
            MULTI_CHAR('name_1'), MULTI_CHAR('name_2'), MULTI_CHAR('name_4'),
        };

        // Window Title
        J2DTextBox* titleBox;
        if (dusk::version::isRegionJpn()) {
            titleBox = (J2DTextBox*)mpScreen->search(MULTI_CHAR('t_t00'));
            mpScreen->search(MULTI_CHAR('f_t00'))->hide();
        } else {
            titleBox = (J2DTextBox*)mpScreen->search(MULTI_CHAR('f_t00'));
            mpScreen->search(MULTI_CHAR('t_t00'))->hide();
        }
        if (titleBox != NULL) {
            titleBox->setFont(mDoExt_getSubFont());
            titleBox->setString(0x200, "");
            mpString->getString(0x5A1, titleBox, NULL, NULL, NULL, 0);
        }

        // 6 Fish Cells
        for (int i = 0; i < 6; i++) {
            mpFishParent[i] = JKR_NEW CPaneMgr(mpScreen, fish_cells[i], 0, NULL);
            s_tphdFishing.mpFishThumb[i] = JKR_NEW CPaneMgr(mpScreen, fish_thumbs[i], 0, NULL);
            s_tphdFishing.mpFishPict[i] = JKR_NEW CPaneMgr(mpScreen, fish_picts[i], 0, NULL);

            J2DTextBox* nameBox;
            if (dusk::version::isRegionJpn()) {
                nameBox = (J2DTextBox*)mpScreen->search(fish_jnames[i]);
                mpScreen->search(fish_fnames[i])->hide();
            } else {
                nameBox = (J2DTextBox*)mpScreen->search(fish_fnames[i]);
                mpScreen->search(fish_jnames[i])->hide();
            }
            if (nameBox != NULL) {
                // RPX 0x029ce0e8: the fish names use the sub font (0x02addfbc), like the title.
                nameBox->setFont(mDoExt_getSubFont());
                nameBox->setString(0x20, "");
                mpString->getString(name_id[i], nameBox, NULL, NULL, NULL, 0);
            }
        }

        // Illustration header
        s_tphdFishing.mpHeaderTitle = (J2DTextBox*)mpScreen->search(MULTI_CHAR('f_name_7'));
        if (s_tphdFishing.mpHeaderTitle != NULL) {
            s_tphdFishing.mpHeaderTitle->setFont(mDoExt_getMesgFont());
            s_tphdFishing.mpHeaderTitle->setString(0x40, "");
        }

        // Description header
        s_tphdFishing.mpDescTitle = (J2DTextBox*)mpScreen->search(MULTI_CHAR('f_name_8'));
        if (s_tphdFishing.mpDescTitle != NULL) {
            s_tphdFishing.mpDescTitle->setFont(mDoExt_getMesgFont());
            s_tphdFishing.mpDescTitle->setString(0x20, "");
            mpString->getString(0x5CA, s_tphdFishing.mpDescTitle, NULL, NULL, NULL, 0);
        }

        // Description text
        s_tphdFishing.mpDescText = (J2DTextBox*)mpScreen->search(MULTI_CHAR('f_name10'));
        if (s_tphdFishing.mpDescText != NULL) {
            s_tphdFishing.mpDescText->setFont(mDoExt_getMesgFont());
            s_tphdFishing.mpDescText->setString(0x400, "");
        }

        // Record header
        s_tphdFishing.mpRecordTitle = (J2DTextBox*)mpScreen->search(MULTI_CHAR('f_name_9'));
        if (s_tphdFishing.mpRecordTitle != NULL) {
            s_tphdFishing.mpRecordTitle->setFont(mDoExt_getMesgFont());
            s_tphdFishing.mpRecordTitle->setString(0x20, "");
            mpString->getString(0x5CB, s_tphdFishing.mpRecordTitle, NULL, NULL, NULL, 0);
        }

        // Largest label & unit
        s_tphdFishing.mpLargestLabel = (J2DTextBox*)mpScreen->search(MULTI_CHAR('size_inf'));
        if (s_tphdFishing.mpLargestLabel != NULL) {
            s_tphdFishing.mpLargestLabel->setFont(mDoExt_getMesgFont());
            s_tphdFishing.mpLargestLabel->setString(0x20, "");
            mpString->getString(0x59F, s_tphdFishing.mpLargestLabel, NULL, NULL, NULL, 0);
        }
        s_tphdFishing.mpLargestValue = (J2DTextBox*)mpScreen->search(MULTI_CHAR('size_cou'));
        if (s_tphdFishing.mpLargestValue != NULL) {
            s_tphdFishing.mpLargestValue->setFont(mDoExt_getMesgFont());
            s_tphdFishing.mpLargestValue->setString(0x20, "");
        }
        s_tphdFishing.mpLargestUnit = (J2DTextBox*)mpScreen->search(MULTI_CHAR('size_suf'));
        if (s_tphdFishing.mpLargestUnit != NULL) {
            s_tphdFishing.mpLargestUnit->setFont(mDoExt_getMesgFont());
            s_tphdFishing.mpLargestUnit->setString(0x20, "");
            mpString->getString(0x5A5, s_tphdFishing.mpLargestUnit, NULL, NULL, NULL, 0);
        }

        // Caught label & unit
        s_tphdFishing.mpCaughtLabel = (J2DTextBox*)mpScreen->search(MULTI_CHAR('numb_inf'));
        if (s_tphdFishing.mpCaughtLabel != NULL) {
            s_tphdFishing.mpCaughtLabel->setFont(mDoExt_getMesgFont());
            s_tphdFishing.mpCaughtLabel->setString(0x20, "");
            mpString->getString(0x5A0, s_tphdFishing.mpCaughtLabel, NULL, NULL, NULL, 0);
        }
        s_tphdFishing.mpCaughtValue = (J2DTextBox*)mpScreen->search(MULTI_CHAR('numb_cou'));
        if (s_tphdFishing.mpCaughtValue != NULL) {
            s_tphdFishing.mpCaughtValue->setFont(mDoExt_getMesgFont());
            s_tphdFishing.mpCaughtValue->setString(0x20, "");
        }
        s_tphdFishing.mpCaughtUnit = (J2DTextBox*)mpScreen->search(MULTI_CHAR('numb_suf'));
        if (s_tphdFishing.mpCaughtUnit != NULL) {
            s_tphdFishing.mpCaughtUnit->setFont(mDoExt_getMesgFont());
            s_tphdFishing.mpCaughtUnit->setString(0x20, "");
            mpString->getString(0x5A4, s_tphdFishing.mpCaughtUnit, NULL, NULL, NULL, 0);
        }
        return;
    }
#endif

    for (int i = 0; i < MAX_FINDABLE_FISHES; i++) {
        mpFishParent[i] = JKR_NEW CPaneMgr(mpScreen, fish_n[i], 0, NULL);
        mpFishParts[0][i] = JKR_NEW CPaneMgr(mpScreen, fish_p0[i], 0, NULL);
        mpFishParts[1][i] = JKR_NEW CPaneMgr(mpScreen, fish_p1[i], 0, NULL);
        mpFishParts[2][i] = JKR_NEW CPaneMgr(mpScreen, fish_p2[i], 0, NULL);
        mpFishParts[3][i] = JKR_NEW CPaneMgr(mpScreen, fish_p3[i], 0, NULL);
        mpFishParts[4][i] = JKR_NEW CPaneMgr(mpScreen, fish_p4[i], 0, NULL);
        mpFishParts[5][i] = JKR_NEW CPaneMgr(mpScreen, fish_p5[i], 0, NULL);
    }
    mpFishInfoParent[0] = JKR_NEW CPaneMgr(mpScreen, MULTI_CHAR('info_blu'), 0, NULL);
    mpFishInfoParent[1] = JKR_NEW CPaneMgr(mpScreen, MULTI_CHAR('info_red'), 0, NULL);

    #if TARGET_PC
    J2DTextBox* textBox;
    if (dusk::version::isRegionJpn()) {
        textBox = (J2DTextBox*)mpScreen->search(MULTI_CHAR('t_t00'));
        mpScreen->search(MULTI_CHAR('f_t00'))->hide();
    } else {
        textBox = (J2DTextBox*)mpScreen->search(MULTI_CHAR('f_t00'));
        mpScreen->search(MULTI_CHAR('t_t00'))->hide();
    }
    #elif (VERSION == VERSION_GCN_JPN) || (VERSION == VERSION_WII_JPN)
    J2DTextBox* textBox = (J2DTextBox*)mpScreen->search(MULTI_CHAR('t_t00'));
    mpScreen->search(MULTI_CHAR('f_t00'))->hide();
    #else
    J2DTextBox* textBox = (J2DTextBox*)mpScreen->search(MULTI_CHAR('f_t00'));
    mpScreen->search(MULTI_CHAR('t_t00'))->hide();
    #endif

    textBox->setFont(mDoExt_getSubFont());
    textBox->setString(DUSK_IF_ELSE(0x400, 0x200), "");
    mpString->getString(0x5a1, textBox, NULL, NULL, NULL, 0);
    for (int i = 0; i < MAX_FINDABLE_FISHES; i++) {
        field_0x10c[1][i] = (J2DTextBox*)mpScreen->search(size_1[i]);
        field_0x10c[1][i]->setFont(mDoExt_getSubFont());
        field_0x10c[1][i]->setString(0x20, "");

        field_0x16c[1][i] = (J2DTextBox*)mpScreen->search(size_unit_1[i]);
        field_0x16c[1][i]->setFont(mDoExt_getSubFont());
        field_0x16c[1][i]->setString(0x20, "");

        field_0x13c[1][i] = (J2DTextBox*)mpScreen->search(count_1[i]);
        field_0x13c[1][i]->setFont(mDoExt_getSubFont());
        field_0x13c[1][i]->setString(0x20, "");

        field_0x19c[1][i] = (J2DTextBox*)mpScreen->search(count_unit_1[i]);
        field_0x19c[1][i]->setFont(mDoExt_getSubFont());
        field_0x19c[1][i]->setString(0x20, "");

        mpFishNameString[i] = (J2DTextBox*)mpScreen->search(name_0[i]);
        IF_DUSK_BLOCK(dusk::version::isGcn() && !dusk::tphd_active())
        mpScreen->search(fname_0[i])->hide();
        IF_DUSK_BLOCK_END
        mpFishNameString[i]->setFont(mDoExt_getSubFont());
        mpFishNameString[i]->setString(0x20, "");
        dMeter2Info_getStringKanji(name_id[i], mpFishNameString[i]->getStringPtr(), NULL);
    }
    mpInfoLargestString = (J2DTextBox*)mpScreen->search(MULTI_CHAR('inf_size'));
    mpInfoNumCaughtString = (J2DTextBox*)mpScreen->search(MULTI_CHAR('inf_cou'));
    mpInfoLargestString->setFont(mDoExt_getMesgFont());
    mpInfoNumCaughtString->setFont(mDoExt_getMesgFont());
    mpInfoLargestString->setString(0x20, "");
    dMeter2Info_getStringKanji(0x59f, mpInfoLargestString->getStringPtr(), NULL); // "Largest"
    mpInfoNumCaughtString->setString(0x20, "");
    dMeter2Info_getStringKanji(0x5a0, mpInfoNumCaughtString->getStringPtr(), NULL); // "No. Caught"
}

void dMenu_Fishing_c::screenSetDoIcon() {
    static const u64 text_a_tag[5] = {MULTI_CHAR('atext1_1'), MULTI_CHAR('atext1_2'), MULTI_CHAR('atext1_3'), MULTI_CHAR('atext1_4'), MULTI_CHAR('atext1_5')};
    static const u64 text_b_tag[5] = {MULTI_CHAR('btext1_1'), MULTI_CHAR('btext1_2'), MULTI_CHAR('btext1_3'), MULTI_CHAR('btext1_4'), MULTI_CHAR('btext1_5')};

    mpIconScreen = JKR_NEW J2DScreen();
#if TARGET_PC
    const char* iconBlo = dusk::tphd_active() ? "zelda_collect_soubi_do_icon_parts_revo.blo"
                                              : "zelda_collect_soubi_do_icon_parts.blo";
    mpIconScreen->setPriority(iconBlo, 0x20000, mpArchive);
#else
    mpIconScreen->setPriority("zelda_collect_soubi_do_icon_parts.blo", 0x20000, mpArchive);
#endif
    for (int i = 0; i < 2; i++) {
        mpButtonAB[i] = NULL;
        mpButtonText[i] = NULL;
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

void dMenu_Fishing_c::setAButtonString(u16 i_stringID) {
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

void dMenu_Fishing_c::setBButtonString(u16 i_stringID) {
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

#if TARGET_PC
void dMenu_Fishing_c::selectFish(int index) {
    if (index < 0 || index >= 6) {
        return;
    }
    s_tphdFishing.mSelectedFish = index;

    static const u32 title_id[6] = {
        0x59E, 0x59D, 0x59B, 0x599, 0x59A, 0x59C,
    };
    static const u32 desc_id[6] = {
        0x5C9, 0x5C8, 0x5C6, 0x5C4, 0x5C5, 0x5C7,
    };

    if (s_tphdFishing.mpHeaderTitle != NULL) {
        mpString->getString(title_id[index], s_tphdFishing.mpHeaderTitle, NULL, NULL, NULL, 0);
    }

    if (s_tphdFishing.mpDescText != NULL) {
        mpString->getString(desc_id[index], s_tphdFishing.mpDescText, NULL, NULL, NULL, 0);
    }

    for (int i = 0; i < 6; i++) {
        if (s_tphdFishing.mpFishPict[i] != NULL) {
            if (i == index) {
                s_tphdFishing.mpFishPict[i]->show();
            } else {
                s_tphdFishing.mpFishPict[i]->hide();
            }
        }
        if (s_tphdFishing.mpFishThumb[i] != NULL) {
            if (i == index) {
                s_tphdFishing.mpFishThumb[i]->show();
            } else {
                s_tphdFishing.mpFishThumb[i]->hide();
            }
        }
    }

    bool isEnglish = false;
    if (dusk::version::isRegionPal() && dComIfGs_getPalLanguage() == dSv_player_config_c::LANGUAGE_ENGLISH) {
        isEnglish = true;
    }

    u16 num = dComIfGs_getFishNum(index);
    if (s_tphdFishing.mpCaughtValue != NULL) {
        snprintf(s_tphdFishing.mpCaughtValue->getStringPtr(), 0x20, "%d", num);
    }

    u8 size = dComIfGs_getFishSize(index);
    if (s_tphdFishing.mpLargestValue != NULL) {
        if (isEnglish) {
            snprintf(s_tphdFishing.mpLargestValue->getStringPtr(), 0x20, "%d", (int)(size / 2.54f));
        } else {
            snprintf(s_tphdFishing.mpLargestValue->getStringPtr(), 0x20, "%d", (int)size);
        }
    }

    setCursorPos(index);
}

void dMenu_Fishing_c::setCursorPos(int index) {
    if (s_tphdFishing.mpDrawCursor == NULL || index < 0 || index >= 6) {
        return;
    }
    const f32 sx = dusk::tphd::subscreenScaleX();
    const f32 minX = dusk::tphd::revoMinX();
    CPaneMgr* targetPane = mpFishParent[index];
    if (targetPane == NULL) {
        return;
    }
    J2DPane* pane = targetPane->getPanePtr();
    if (pane == NULL) {
        return;
    }
    // RPX 0x029ceefc: centre of N_fish_N, box = pane size, params (1, 1, 0.1, 0.7, 0.7).
    // Only valid once the journal has been drawn this frame (called from _draw).
    const Vec c = dusk::tphd::mgrDrawnCenter(targetPane);
    s_tphdFishing.mpDrawCursor->setPosRevo(c.x, c.y, pane, false, sx, minX);
    s_tphdFishing.mpDrawCursor->setParam(1.0f, 1.0f, 0.1f, 0.7f, 0.7f);
}

// Cursor moves of the RPX (0x029cfcc4 left, 0x029cfdcc right, 0x029cfee8 up, 0x029cffe0 down).
// Fish index -> page slot: 0 top right, 1 bottom right, 2 bottom left, 3 middle left,
// 4 middle right, 5 top left. No wrap-around; hidden (not caught) fish are skipped.
namespace {
static inline bool fishShown(int i) {
    return i >= 0 && i < 6 && dComIfGs_getFishNum(i) != 0;
}
}  // namespace

int dMenu_Fishing_c::checkLeft() {
    const int cur = s_tphdFishing.mSelectedFish;
    if (cur == 0) {
        if (fishShown(5)) return 5;
        if (fishShown(3)) return 3;
    } else if (cur == 1) {
        if (fishShown(2)) return 2;
        if (fishShown(3)) return 3;
        if (fishShown(5)) return 5;
        return cur;
    } else if (cur == 4) {
        if (fishShown(3)) return 3;
        if (fishShown(5)) return 5;
    } else {
        return cur;
    }
    if (fishShown(2)) return 2;
    return cur;
}

int dMenu_Fishing_c::checkRight() {
    const int cur = s_tphdFishing.mSelectedFish;
    if (cur == 2) {
        if (fishShown(1)) return 1;
        if (fishShown(4)) return 4;
        if (fishShown(0)) return 0;
        return cur;
    } else if (cur == 3) {
        if (fishShown(4)) return 4;
        if (fishShown(0)) return 0;
    } else if (cur == 5) {
        if (fishShown(0)) return 0;
        if (fishShown(4)) return 4;
    } else {
        return cur;
    }
    if (fishShown(1)) return 1;
    return cur;
}

int dMenu_Fishing_c::checkUp() {
    const int cur = s_tphdFishing.mSelectedFish;
    if (cur == 1) {
        if (fishShown(4)) return 4;
        if (fishShown(0)) return 0;
    } else if (cur == 2) {
        if (fishShown(3)) return 3;
        if (fishShown(5)) return 5;
    } else if (cur == 3) {
        if (fishShown(5)) return 5;
    } else if (cur == 4) {
        if (fishShown(0)) return 0;
    }
    return cur;
}

int dMenu_Fishing_c::checkDown() {
    const int cur = s_tphdFishing.mSelectedFish;
    if (cur == 0) {
        if (fishShown(4)) return 4;
        if (fishShown(1)) return 1;
    } else if (cur == 3) {
        if (fishShown(2)) return 2;
    } else if (cur == 4) {
        if (fishShown(1)) return 1;
    } else if (cur == 5) {
        if (fishShown(3)) return 3;
        if (fishShown(2)) return 2;
    }
    return cur;
}
#endif

u8 dMenu_Fishing_c::getFigure(int param_0) {
    if (param_0 < 0) {
        param_0 *= -1;
    }
    if (1000 <= param_0) {
        param_0 = 999;
    }
    if (100 <= param_0) {
        return 3;
    }
    if (param_0 >= 10) {
        return 2;
    }
    return 1;
}

void dMenu_Fishing_c::setFishParam(int i_fishIdx, u16 i_fishCount, u8 i_fishSize) {
    u8 fishCountFigure = getFigure(i_fishCount);
    u8 fishSizeFigure = getFigure(i_fishSize);
    char strBuff1[32];
    char strBuff2[32];
    for (int i = 1; i < 2; i++) {
        // part one, i_fishSize
        dComIfGp_setMessageCountNumber(i_fishSize);
        mpString->getString(0x597, field_0x10c[i][i_fishIdx], NULL, NULL, NULL, 0); // "inches"
        char* stringPtr = field_0x10c[i][i_fishIdx]->getStringPtr();
        SAFE_STRCPY(strBuff1, stringPtr);
        int j;
        for (j = 0; strBuff1[j + fishSizeFigure] != 0; j++) {
            strBuff2[j] = strBuff1[j + fishSizeFigure];
        }
        strBuff2[j] = 0;
        strBuff1[fishSizeFigure] = 0;
        SAFE_STRCPY(field_0x10c[i][i_fishIdx]->getStringPtr(), strBuff1);
        SAFE_STRCPY(field_0x16c[i][i_fishIdx]->getStringPtr(), strBuff2);

        // part two, i_fishCount
        dComIfGp_setMessageCountNumber(i_fishCount);
        mpString->getString(0x598, field_0x13c[i][i_fishIdx], NULL, NULL, NULL, 0); // "fish"
        stringPtr = field_0x13c[i][i_fishIdx]->getStringPtr();
        SAFE_STRCPY(strBuff1, stringPtr);
        int k;
        for (k = 0; strBuff1[k + fishCountFigure] != 0; k++) {
            strBuff2[k] = strBuff1[k + fishCountFigure];
        }
        strBuff2[k] = 0;
        strBuff1[fishCountFigure] = 0;
        SAFE_STRCPY(field_0x13c[i][i_fishIdx]->getStringPtr(), strBuff1);
        SAFE_STRCPY(field_0x19c[i][i_fishIdx]->getStringPtr(), strBuff2);
    }
}

void dMenu_Fishing_c::setHIO(bool i_useHIO) {
#if TARGET_PC
    if (dusk::tphd_active()) {
        return;
    }
#endif
    if (i_useHIO || g_drawHIO.mFishListScreen.mDebug) {
        for (int i = 0; i < MAX_FINDABLE_FISHES; i++) {
            mpFishParent[i]->paneTrans(g_drawHIO.mFishListScreen.mFishInfoPosX[i],
                                       g_drawHIO.mFishListScreen.mFishInfoPosY[i]);
            mpFishParent[i]->scale(g_drawHIO.mFishListScreen.mFishInfoScale[i],
                                   g_drawHIO.mFishListScreen.mFishInfoScale[i]);

            mpFishParts[0][i]->paneTrans(g_drawHIO.mFishListScreen.mFishIconPosX[i],
                                         g_drawHIO.mFishListScreen.mFishIconPosY[i]);
            mpFishParts[0][i]->scale(g_drawHIO.mFishListScreen.mFishIconScale[i],
                                     g_drawHIO.mFishListScreen.mFishIconScale[i]);

            mpFishParts[1][i]->paneTrans(g_drawHIO.mFishListScreen.mFishNamePosX[i],
                                         g_drawHIO.mFishListScreen.mFishNamePosY[i]);
            mpFishParts[1][i]->scale(g_drawHIO.mFishListScreen.mFishNameScale[i],
                                     g_drawHIO.mFishListScreen.mFishNameScale[i]);

            mpFishParts[2][i]->paneTrans(g_drawHIO.mFishListScreen.mFishLinePosX[i],
                                         g_drawHIO.mFishListScreen.mFishLinePosY[i]);
            mpFishParts[2][i]->scale(g_drawHIO.mFishListScreen.mFishLineScale[i],
                                     g_drawHIO.mFishListScreen.mFishLineScale[i]);

            mpFishParts[3][i]->paneTrans(g_drawHIO.mFishListScreen.mFishSizePosX[i],
                                         g_drawHIO.mFishListScreen.mFishSizePosY[i]);
            mpFishParts[3][i]->scale(g_drawHIO.mFishListScreen.mFishSizeScale[i],
                                     g_drawHIO.mFishListScreen.mFishSizeScale[i]);

            mpFishParts[4][i]->paneTrans(g_drawHIO.mFishListScreen.mFishCountPosX[i],
                                         g_drawHIO.mFishListScreen.mFishCountPosY[i]);
            mpFishParts[4][i]->scale(g_drawHIO.mFishListScreen.mFishCountScale[i],
                                     g_drawHIO.mFishListScreen.mFishCountScale[i]);

            mpFishParts[5][i]->paneTrans(g_drawHIO.mFishListScreen.mFishCountSizePosX[i],
                                         g_drawHIO.mFishListScreen.mFishCountSizePosY[i]);
            mpFishParts[5][i]->scale(g_drawHIO.mFishListScreen.mFishCountSizeScale[i],
                                     g_drawHIO.mFishListScreen.mFishCountSizeScale[i]);
        }
        for (int i = 0; i < 2; i++) {
            mpFishInfoParent[i]->paneTrans(g_drawHIO.mFishListScreen.mPosX[i],
                                           g_drawHIO.mFishListScreen.mPosY[i]);
            mpFishInfoParent[i]->scale(g_drawHIO.mFishListScreen.mScale[i],
                                       g_drawHIO.mFishListScreen.mScale[i]);
        }
    }
    if (g_drawHIO.mCollectScreen.mButtonDebugON || i_useHIO) {
        if (mpButtonAB[A_BUTTON] != NULL) {
            mpButtonAB[A_BUTTON]->paneTrans(g_drawHIO.mCollectScreen.mButtonAPosX,
                                     g_drawHIO.mCollectScreen.mButtonAPosY);
            mpButtonAB[A_BUTTON]->scale(g_drawHIO.mCollectScreen.mButtonAScale,
                                 g_drawHIO.mCollectScreen.mButtonAScale);
        }
        if (mpButtonAB[B_BUTTON] != NULL) {
            mpButtonAB[B_BUTTON]->paneTrans(g_drawHIO.mCollectScreen.mButtonBPosX,
                                     g_drawHIO.mCollectScreen.mButtonBPosY);
            mpButtonAB[B_BUTTON]->scale(g_drawHIO.mCollectScreen.mButtonBScale,
                                 g_drawHIO.mCollectScreen.mButtonBScale);
        }
        if (mpButtonText[A_BUTTON] != NULL) {
            mpButtonText[A_BUTTON]->paneTrans(g_drawHIO.mCollectScreen.mButtonATextPosX,
                                       g_drawHIO.mCollectScreen.mButtonATextPosY);
            mpButtonText[A_BUTTON]->scale(g_drawHIO.mCollectScreen.mButtonATextScale,
                                   g_drawHIO.mCollectScreen.mButtonATextScale);
        }
        if (mpButtonText[B_BUTTON] != NULL) {
            mpButtonText[B_BUTTON]->paneTrans(g_drawHIO.mCollectScreen.mButtonBTextPosX,
                                       g_drawHIO.mCollectScreen.mButtonBTextPosY);
            mpButtonText[B_BUTTON]->scale(g_drawHIO.mCollectScreen.mButtonBTextScale,
                                   g_drawHIO.mCollectScreen.mButtonBTextScale);
        }
    }
}
