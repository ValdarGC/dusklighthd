/**
 * d_file_sel_info.cpp
 * File Select Screen File Info
 */

#include "d/dolzel.h" // IWYU pragma: keep

#include "d/d_file_sel_info.h"
#include "JSystem/J2DGraph/J2DScreen.h"
#include "JSystem/J2DGraph/J2DTextBox.h"
#include "d/d_com_inf_game.h"
#include "d/d_meter2_info.h"
#include "d/d_pane_class_alpha.h"
#include <cstdio>
#include <cstring>

#include "helpers/string.hpp"
#if TARGET_PC
#include "dusk/settings.h"

namespace {
// TPHD file select area name (Wii U dFile_info_c 0x0294c620, run through all its stage tests):
// message id of the "last location" saved in the reserve area. Unknown stage: 0x579 "Hyrule Kingdom".
struct TphdLoc {
    const char* stage;
    u16 msg;
};
const TphdLoc kTphdLoc[] = {
    {"D_MN01", 0x373},  {"D_MN01A", 0x373}, {"D_MN01B", 0x373}, {"D_MN04", 0x372},
    {"D_MN04A", 0x372}, {"D_MN04B", 0x372}, {"D_MN05", 0x371},  {"D_MN05A", 0x371},
    {"D_MN05B", 0x371}, {"D_MN06", 0x376},  {"D_MN06A", 0x376}, {"D_MN06B", 0x376},
    {"D_MN07", 0x377},  {"D_MN07A", 0x377}, {"D_MN07B", 0x377}, {"D_MN08", 0x378},
    {"D_MN08A", 0x378}, {"D_MN08B", 0x378}, {"D_MN08C", 0x378}, {"D_MN08D", 0x378},
    {"D_MN09", 0x370},  {"D_MN09A", 0x370}, {"D_MN09B", 0x370}, {"D_MN09C", 0x370},
    {"D_MN10", 0x374},  {"D_MN10A", 0x374}, {"D_MN10B", 0x374}, {"D_MN11", 0x375},
    {"D_MN11A", 0x375}, {"D_MN11B", 0x375}, {"D_SB00", 0x583},  {"D_SB01", 0x58b},
    {"D_SB02", 0x583},  {"D_SB03", 0x587},  {"D_SB04", 0x583},  {"D_SB10", 0x580},
    {"F_SP00", 0x57a},  {"F_SP102", 0x583}, {"F_SP103", 0x57b}, {"F_SP104", 0x57c},
    {"F_SP108", 0x580}, {"F_SP109", 0x581}, {"F_SP110", 0x582}, {"F_SP111", 0x589},
    {"F_SP112", 0x585}, {"F_SP113", 0x586}, {"F_SP114", 0x58a}, {"F_SP115", 0x587},
    {"F_SP116", 0x58c}, {"F_SP117", 0x58d}, {"F_SP118", 0x58b}, {"F_SP121", 0x583},
    {"F_SP122", 0x583}, {"F_SP123", 0x583}, {"F_SP124", 0x58b}, {"F_SP125", 0x5cd},
    {"F_SP126", 0x585}, {"F_SP127", 0x585}, {"F_SP128", 0x58f}, {"F_SP200", 0x579},
    {"R_SP01", 0x57b},  {"R_SP108", 0x580}, {"R_SP109", 0x581}, {"R_SP110", 0x582},
    {"R_SP116", 0x58c}, {"R_SP127", 0x585}, {"R_SP128", 0x58f}, {"R_SP160", 0x58c},
    {"R_SP161", 0x58c}, {"R_SP209", 0x581}, {"R_SP301", 0x58e},
};

u16 tphdLocationMsg(const char* i_stage, s16 i_room, s8 i_layer, s8 i_saveTbl) {
    // Grottos: the area depends on the layer.
    static const struct {
        const char* stage;
        u16 msg[5];  // layer 0..4 (other layers: 0x579)
    } kGrotto[] = {
        {"D_SB05", {0x583, 0x583, 0x580, 0x583, 0x579}},
        {"D_SB06", {0x583, 0x583, 0x58d, 0x583, 0x579}},
        {"D_SB07", {0x58b, 0x58b, 0x58a, 0x583, 0x579}},
        {"D_SB08", {0x583, 0x58b, 0x58a, 0x583, 0x579}},
        {"D_SB09", {0x583, 0x583, 0x587, 0x583, 0x587}},
    };
    for (const auto& g : kGrotto) {
        if (strcmp(i_stage, g.stage) == 0) {
            return (i_layer >= 0 && i_layer <= 4) ? g.msg[i_layer] : 0x579;
        }
    }
    if (strcmp(i_stage, "R_SP107") == 0) {
        if (i_saveTbl != 13) {
            return 0x592;
        }
        return i_room == 1 ? 0x58c : 0x370;
    }
    for (const auto& l : kTphdLoc) {
        if (strcmp(i_stage, l.stage) == 0) {
            return l.msg;
        }
    }
    return 0x579;
}

void tphdSetLocation(J2DScreen* i_scr, dSv_save_c* i_savedata) {
    J2DPane* p = i_scr != NULL ? i_scr->searchUserInfo(MULTI_CHAR('location')) : NULL;
    if (p == NULL && i_scr != NULL) {
        p = i_scr->search(MULTI_CHAR('location'));
    }
    if (p == NULL || p->getTypeID() != 19) {
        return;
    }
    J2DTextBox* tb = static_cast<J2DTextBox*>(p);
    if (i_savedata == NULL || !dusk::tphd_active()) {
        SAFE_STRCPY(tb->getStringPtr(), "");
        return;
    }
    char stage[8];
    s16 room;
    s8 layer = 0;
    s8 saveTbl = 0;
    const dSv_reserve_c& r = i_savedata->getReserve();
    if (r.hasTphdLocation()) {
        r.getTphdLocStage(stage);
        room = r.getTphdLocRoom();
        layer = r.getTphdLocLayer();
        saveTbl = r.getTphdLocSaveTbl();
    } else {
        // Saved by the GC game (no location): use the respawn place.
        dSv_player_return_place_c& rp = i_savedata->getPlayer().getPlayerReturnPlace();
        strncpy(stage, rp.getName(), 7);
        stage[7] = 0;
        room = rp.getRoomNo();
    }
    dMeter2Info_getString(tphdLocationMsg(stage, room, layer, saveTbl), tb->getStringPtr(), NULL);
}
}  // namespace
#endif
#include "dusk/version.hpp"
#include "helpers/string.hpp"
#endif

dFile_info_c::dFile_info_c(JKRArchive* i_archive, u8 param_1) {
    mArchive = i_archive;
    field_0x22 = param_1;
    mFileInfo.mBasePane = NULL;
    screenSet();
    field_0x20 = 0;
}

dFile_info_c::~dFile_info_c() {
    JKR_DELETE(mFileInfo.Scr);
    JKR_DELETE(mDatBase);
    JKR_DELETE(mNoDatBase);

    mDoExt_removeMesgFont();
}

void dFile_info_c::screenSet() {
    mFileInfo.Scr = JKR_NEW J2DScreen();
    JUT_ASSERT(0, mFileInfo.Scr != NULL);

    mFileInfo.Scr->setPriority("zelda_file_select_info_text.blo", 0x1100000, mArchive);
    mFileInfo.mFont = mDoExt_getMesgFont();
    mFileInfo.Scr->search(MULTI_CHAR('w_cp_ef1'))->hide();
    mFileInfo.field_0x10 = mFileInfo.Scr->search(MULTI_CHAR('w_dat_i1'));
    mDatBase = JKR_NEW CPaneMgrAlpha(mFileInfo.Scr, MULTI_CHAR('w_dat_i1'), 2, NULL);
    mNoDatBase = JKR_NEW CPaneMgrAlpha(mFileInfo.Scr, MULTI_CHAR('w_nda_i1'), 2, NULL);

    J2DTextBox* info_text[4];
    
    #if TARGET_PC
    if (dusk::version::isRegionJpn()) {
        info_text[0] = (J2DTextBox*)mFileInfo.Scr->search(MULTI_CHAR('w_s_t_01'));
        info_text[1] = (J2DTextBox*)mFileInfo.Scr->search(MULTI_CHAR('w_p_t_01'));
        mFileInfo.Scr->search(MULTI_CHAR('f_s_t_02'))->hide();
        mFileInfo.Scr->search(MULTI_CHAR('f_p_t_02'))->hide();
    } else {
        info_text[0] = (J2DTextBox*)mFileInfo.Scr->search(MULTI_CHAR('f_s_t_02'));
        info_text[1] = (J2DTextBox*)mFileInfo.Scr->search(MULTI_CHAR('f_p_t_02'));
        mFileInfo.Scr->search(MULTI_CHAR('w_s_t_01'))->hide();
        mFileInfo.Scr->search(MULTI_CHAR('w_p_t_01'))->hide();
    }
    #elif (VERSION == VERSION_GCN_JPN) || (VERSION == VERSION_WII_JPN)
    info_text[0] = (J2DTextBox*)mFileInfo.Scr->search(MULTI_CHAR('w_s_t_01'));
    info_text[1] = (J2DTextBox*)mFileInfo.Scr->search(MULTI_CHAR('w_p_t_01'));
    mFileInfo.Scr->search(MULTI_CHAR('f_s_t_02'))->hide();
    mFileInfo.Scr->search(MULTI_CHAR('f_p_t_02'))->hide();
    #else
    info_text[0] = (J2DTextBox*)mFileInfo.Scr->search(MULTI_CHAR('f_s_t_02'));
    info_text[1] = (J2DTextBox*)mFileInfo.Scr->search(MULTI_CHAR('f_p_t_02'));
    mFileInfo.Scr->search(MULTI_CHAR('w_s_t_01'))->hide();
    mFileInfo.Scr->search(MULTI_CHAR('w_p_t_01'))->hide();
    #endif

    for (int i = 0; i < 2; i++) {
        info_text[i]->setFont(mFileInfo.mFont);
        info_text[i]->setString(0x20, "");
    }
    dMeter2Info_getString(0x3D0, info_text[0]->getStringPtr(), NULL);  // Save time
    dMeter2Info_getString(0x3D1, info_text[1]->getStringPtr(), NULL);  // Total play time

    #if TARGET_PC
    if (dusk::version::isRegionJpn()) {
        info_text[0] = (J2DTextBox*)mFileInfo.Scr->search(MULTI_CHAR('w_name01'));
        info_text[1] = (J2DTextBox*)mFileInfo.Scr->search(MULTI_CHAR('w_new_1'));
        mFileInfo.Scr->search(MULTI_CHAR('f_name01'))->hide();
        mFileInfo.Scr->search(MULTI_CHAR('f_new_1'))->hide();
    } else {
        info_text[0] = (J2DTextBox*)mFileInfo.Scr->search(MULTI_CHAR('f_name01'));
        info_text[1] = (J2DTextBox*)mFileInfo.Scr->search(MULTI_CHAR('f_new_1'));
        mFileInfo.Scr->search(MULTI_CHAR('w_name01'))->hide();
        mFileInfo.Scr->search(MULTI_CHAR('w_new_1'))->hide();
    }
    #elif (VERSION == VERSION_GCN_JPN) || (VERSION == VERSION_WII_JPN)
    info_text[0] = (J2DTextBox*)mFileInfo.Scr->search(MULTI_CHAR('w_name01'));
    info_text[1] = (J2DTextBox*)mFileInfo.Scr->search(MULTI_CHAR('w_new_1'));
    mFileInfo.Scr->search(MULTI_CHAR('f_name01'))->hide();
    mFileInfo.Scr->search(MULTI_CHAR('f_new_1'))->hide();
    #else
    info_text[0] = (J2DTextBox*)mFileInfo.Scr->search(MULTI_CHAR('f_name01'));
    info_text[1] = (J2DTextBox*)mFileInfo.Scr->search(MULTI_CHAR('f_new_1'));
    mFileInfo.Scr->search(MULTI_CHAR('w_name01'))->hide();
    mFileInfo.Scr->search(MULTI_CHAR('w_new_1'))->hide();
    #endif

    info_text[2] = (J2DTextBox*)mFileInfo.Scr->search(MULTI_CHAR('w_time01'));
    info_text[3] = (J2DTextBox*)mFileInfo.Scr->search(MULTI_CHAR('w_ptim01'));

#define INFO_TEXT_SIZE 0x40

    for (int i = 0; i < 4; i++) {
        info_text[i]->setFont(mFileInfo.mFont);
        info_text[i]->setString(INFO_TEXT_SIZE, "");
    }
    mPlayerName = info_text[0]->getStringPtr();
    mSaveStatus = info_text[1]->getStringPtr();
    mSaveDate = info_text[2]->getStringPtr();
    mPlayTime = info_text[3]->getStringPtr();
#if TARGET_PC
    // TPHD: area name of the save (Wii U 0x0294b6bc: textbox 'location').
    J2DPane* loc = mFileInfo.Scr->searchUserInfo(MULTI_CHAR('location'));
    if (loc == NULL) {
        loc = mFileInfo.Scr->search(MULTI_CHAR('location'));
    }
    if (loc != NULL && loc->getTypeID() == 19) {
        static_cast<J2DTextBox*>(loc)->setFont(mFileInfo.mFont);
        static_cast<J2DTextBox*>(loc)->setString(INFO_TEXT_SIZE, "");
    }
#endif
}

int dFile_info_c::setSaveData(dSv_save_c* i_savedata, BOOL i_validChksum, u8 i_dataNo) {
    int result;
#if TARGET_PC
    tphdSetLocation(mFileInfo.Scr, NULL);
#endif
    if (i_validChksum) {
        char* player_name = i_savedata->getPlayer().getPlayerInfo().getPlayerName();
        if (*player_name == 0) {
            if (field_0x22 == 1 && i_dataNo == dComIfGs_getDataNum()) {
                i_savedata->getPlayer().getPlayerStatusA().setLife(dComIfGs_getLife());
                setHeartCnt(i_savedata);
                i_savedata->getPlayer().getPlayerStatusA().setLife(12);
                SAFE_STRCPY(mPlayerName, dComIfGs_getPlayerName());
                SAFE_STRCPY(mSaveDate, "");
                SAFE_STRCPY(mPlayTime, "");
                dMeter2Info_getString(0x4D, mSaveStatus, NULL);  // New Quest Log
                result = 2;
            } else {
                dMeter2Info_getString(0x4D, mSaveStatus, NULL);  // New Quest Log
                result = 1;
            }
        } else {
            setHeartCnt(i_savedata);
            SAFE_STRCPY(mPlayerName, player_name);
            setSaveDate(i_savedata);
            setPlayTime(i_savedata);
#if TARGET_PC
            tphdSetLocation(mFileInfo.Scr, i_savedata);
#endif
            result = 0;
        }
    } else {
        dMeter2Info_getString(0x51, mSaveStatus, NULL);  // This Quest Log is Corrupted
        result = -1;
    }
    return result;
}

void dFile_info_c::setHeartCnt(dSv_save_c* i_savedata) {
    static u64 l_htag[] = {
        MULTI_CHAR('hear_20'), MULTI_CHAR('hear_21'), MULTI_CHAR('hear_22'), MULTI_CHAR('hear_23'), MULTI_CHAR('hear_24'), MULTI_CHAR('hear_25'), MULTI_CHAR('hear_26'),
        MULTI_CHAR('hear_27'), MULTI_CHAR('hear_28'), MULTI_CHAR('hear_29'), MULTI_CHAR('hear_30'), MULTI_CHAR('hear_31'), MULTI_CHAR('hear_32'), MULTI_CHAR('hear_33'),
        MULTI_CHAR('hear_34'), MULTI_CHAR('hear_35'), MULTI_CHAR('hear_36'), MULTI_CHAR('hear_37'), MULTI_CHAR('hear_38'), MULTI_CHAR('hear_39'),
    };

    static const char* amariheartTex[] = {
        "tt_heart_00.bti",
        "tt_heart_00.bti",
        "tt_heart_00.bti",
        "tt_heart_00.bti",
    };

    u16 life = i_savedata->getPlayer().getPlayerStatusA().getLife();
    s32 count = (life & 0xFFFF) / 5;
    s32 quarter_count = life % 5;
    if (quarter_count != 0) {
        count++;
    }

    J2DPicture* heart_tex[20];
    for (int i = 0; i < 20; i++) {
        heart_tex[i] = (J2DPicture*)mFileInfo.Scr->search(l_htag[i]);

        if (i < i_savedata->getPlayer().getPlayerStatusA().getMaxLife() / 5) {
            heart_tex[i]->show();
            if (i < count) {
                if (quarter_count != 0 && i == count - 1) {
                    heart_tex[i]->changeTexture(amariheartTex[quarter_count - 1], 0);
                } else {
                    heart_tex[i]->changeTexture("tt_heart_00.bti", 0);
                }
            } else {
                heart_tex[i]->changeTexture("tt_heart_00.bti", 0);
            }
        } else {
            heart_tex[i]->hide();
        }
    }
}

typedef void (dFile_info_c::*procFunc)();
static procFunc fileWarningProc[] = {&dFile_info_c::modeWait, &dFile_info_c::modeMove};

void dFile_info_c::setSaveDate(dSv_save_c* i_savedata) {
    OSCalendarTime time;
    OSTicksToCalendarTime(i_savedata->getPlayer().getPlayerStatusB().getDateIpl(), &time);

    #if TARGET_PC
    if (dusk::version::isRegionJpn()) {
        SAFE_SPRINTF(mSaveDate, "%d.%02d.%02d %02d:%02d", time.year, time.mon + 1, time.mday,
            time.hour, time.min);
    } else if (dusk::version::isRegionPal() && dComIfGs_getPalLanguage() != dSv_player_config_c::LANGUAGE_ENGLISH) {
        SAFE_SPRINTF(mSaveDate, "%02d/%02d/%d %02d:%02d", time.mday, time.mon + 1, time.year, time.hour,
            time.min);
    } else {
        SAFE_SPRINTF(mSaveDate, "%02d/%02d/%d %02d:%02d", time.mon + 1, time.mday, time.year, time.hour,
            time.min);
    }
    #elif (VERSION == VERSION_GCN_JPN) || (VERSION == VERSION_WII_JPN)
    sprintf(mSaveDate, "%d.%02d.%02d %02d:%02d", time.year, time.mon + 1, time.mday,
            time.hour, time.min);
    #elif VERSION == VERSION_GCN_PAL
    if (dComIfGs_getPalLanguage() == dSv_player_config_c::LANGUAGE_ENGLISH) {
        sprintf(mSaveDate, "%02d/%02d/%d %02d:%02d", time.mon + 1, time.mday, time.year, time.hour,
                time.min);
    } else {
        sprintf(mSaveDate, "%02d/%02d/%d %02d:%02d", time.mday, time.mon + 1, time.year, time.hour,
                time.min);
    }
#else
    sprintf(mSaveDate, "%02d/%02d/%d %02d:%02d", time.mon + 1, time.mday, time.year,
            time.hour, time.min);
    #endif
}

void dFile_info_c::setPlayTime(dSv_save_c* i_savedata) {
    s64 time = i_savedata->getPlayer().getPlayerInfo().getTotalTime() / (OS_BUS_CLOCK / 4);

    // 3599940 = 999:59 in seconds
    if (time >= 3599940) {
        SAFE_SPRINTF(mPlayTime, "999:59");
    } else {
        u32 min = (time % 3600) / 60;
        u32 hours = time / 3600;
        SAFE_SPRINTF(mPlayTime, "%d:%02d", hours, min);
    }
}

void dFile_info_c::modeWait() {}

void dFile_info_c::modeMove() {}

#if TARGET_PC

static u64 const heart_tags[] = {
    MULTI_CHAR('hear_20'),
    MULTI_CHAR('hear_21'),
    MULTI_CHAR('hear_22'),
    MULTI_CHAR('hear_23'),
    MULTI_CHAR('hear_24'),
    MULTI_CHAR('hear_25'),
    MULTI_CHAR('hear_26'),
    MULTI_CHAR('hear_27'),
    MULTI_CHAR('hear_28'),
    MULTI_CHAR('hear_29'),
    MULTI_CHAR('hear_30'),
    MULTI_CHAR('hear_31'),
    MULTI_CHAR('hear_32'),
    MULTI_CHAR('hear_33'),
    MULTI_CHAR('hear_34'),
    MULTI_CHAR('hear_35'),
    MULTI_CHAR('hear_36'),
    MULTI_CHAR('hear_37'),
    MULTI_CHAR('hear_38'),
    MULTI_CHAR('hear_39'),
};

void dFile_info_c::fileSelectWide() {
    // TPHD Exclusions
    if (dusk::tphd_active()) {
        mFileInfo.Scr->search(MULTI_CHAR('t_w_01'))->hide();

        for (int i = 0; i < 20; i++) {
            static_cast<J2DPicture*>(mFileInfo.Scr->search(heart_tags[i]))
                ->setBlackWhite(
                    JUtility::TColor(231, 25, 0, 0), JUtility::TColor(255, 255, 255, 255));
        }
    }
}
#endif

void dFile_info_c::_draw() {
    fileSelectWide();
    dComIfGd_set2DOpa(&mFileInfo);
}

void dDlst_FileInfo_c::draw() {
    Mtx m;
    J2DGrafContext* graf_ctx = dComIfGp_getCurrentGrafPort();

    if (mBasePane != NULL) {
        // fake match?
        MtxP glb_mtx = (MtxP)&mBasePane->getGlbMtx()[0][0];

        MTXScale(m, mBasePane->getWidth() / field_0x10->getWidth(),
                 mBasePane->getHeight() / field_0x10->getHeight(), 1.0f);
        MTXConcat(glb_mtx, m, glb_mtx);
        Scr->search(MULTI_CHAR('Nm_02'))->setMtx(glb_mtx);
    }

    Scr->draw(0.0f, 0.0f, graf_ctx);
}
