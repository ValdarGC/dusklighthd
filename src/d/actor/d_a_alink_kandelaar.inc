/**
 * d_a_alink_kandelaar.inc
 * Player Lantern action handling
 */

#include "d/actor/d_a_alink.h"
#include "d/d_com_inf_game.h"
#include "d/actor/d_a_e_sm2.h"
#include "d/actor/d_a_tag_ss_drink.h"
#include "m_Do/m_Do_lib.h"
#include "SSystem/SComponent/c_math.h"

static fopAc_ac_c* daAlink_searchTagKandelaar(fopAc_ac_c* i_actor, void* i_data) {
    UNUSED(i_data);

    if (fopAcM_GetName(i_actor) == fpcNm_TAG_LANTERN_e) {
        return i_actor;
    }

    return NULL;
}

static fopAc_ac_c* daAlink_searchTagKtOnFire(fopAc_ac_c* i_actor, void* i_data) {
    UNUSED(i_data);
    daAlink_c* player_p = daAlink_getAlinkActorClass();

    if (fopAcM_GetName(i_actor) == fpcNm_Tag_KtOnFire_e &&
        i_actor->current.pos.abs2(player_p->current.pos) < SQUARE(150.0f) &&
        fopAcM_seenActorAngleY(player_p, i_actor) < 0x3000)
    {
        return i_actor;
    }

    return NULL;
}

void daAlink_c::setKandelaarMtx(Mtx i_mtx, int param_1, int param_2) {
    if (mpKanteraModel != NULL) {
        if (checkNoResetFlg2(FLG2_UNK_1)) {
            deleteEquipItem(FALSE, TRUE);
        }

        onEndResetFlg1(ERFLG1_UNK_4);

        mpKanteraModel->setBaseTRMtx(i_mtx);
        if (param_1 == 0) {
            onEndResetFlg1(ERFLG1_UNK_8);
        }

        if (param_2 != 0) {
            onEndResetFlg1(ERFLG1_UNK_10);
        }
    }
}

BOOL daAlink_c::checkWaterInKandelaarOffset(f32 param_0) {
    f32 base_y_pos;
    if (checkModeFlg(0x40)) {
        base_y_pos = mRightFootPos.y;
    } else {
        base_y_pos = current.pos.y;
    }

    return param_0 > 65.0f + base_y_pos;
}

void daAlink_c::checkWaterInKandelaar(f32 param_0) {
    if (checkNoResetFlg2(FLG2_UNK_1) && checkWaterInKandelaarOffset(param_0)) {
        if (mEquipItem == dItemNo_KANTERA_e) {
            deleteEquipItem(TRUE, TRUE);
        } else {
            offKandelaarModel();
            seStartOnlyReverb(Z2SE_AL_ITEM_TAKEOUT);
        }
    }
}

void daAlink_c::offKandelaarModel() {
    offNoResetFlg2(daPy_FLG2(FLG2_KANDELAAR_LIGHT_OFF | FLG2_UNK_1));
    mZ2Link.setKanteraState(0);
}

bool daAlink_c::checkUseKandelaar(int param_0) {
    return (param_0 == 0 && checkNoResetFlg2(FLG2_UNK_1)) ||
           (param_0 != 0 && mEquipItem == dItemNo_KANTERA_e);
}

bool daAlink_c::checkKandelaarSwing(int param_0) const {
    if ((checkKandelaarSwingAnime() || mProcID == PROC_KANDELAAR_SWING) &&
        (dComIfGs_getOil() != 0 || param_0 == 0))
    {
        return true;
    }

    return false;
}

int daAlink_c::kandelaarModelCallBack() {
    cXyz sp44;
    mDoMtx_multVecZero(J3DSys::mCurrentMtx, &sp44);

    field_0x3630 = field_0x3624;
    field_0x3624 = mKandelaarFlamePos;

    cXyz sp38 = (mKandelaarFlamePos - sp44) + field_0x3618;
    sp38.y -= 3.0f;

    cXyz sp2C;
    mDoMtx_multVec(J3DSys::mCurrentMtx, &cXyz::BaseZ, &sp2C);

    s16 var_r29 = sp2C.atan2sX_Z();
    mDoMtx_stack_c::YrotS(-var_r29);
    mDoMtx_stack_c::multVec(&sp38, &sp38);

    s16 var_r28 = cLib_minMaxLimit<s16>(cM_atan2s(-sp38.z, -sp38.y), -0x1800, 0x1800);
    s16 var_r27 = cLib_minMaxLimit<s16>(cM_atan2s(sp38.x, JMAFastSqrt(SQUARE(sp38.y) + SQUARE(sp38.z))), -0x1800, 0x1800);

    mDoMtx_stack_c::transS(sp44);
    if (mProcID != PROC_OPEN_TREASURE && (mProcID != PROC_GET_ITEM || mProcVar4.field_0x3010 == 0)) {
        mDoMtx_stack_c::ZXYrotM(var_r28, var_r29, var_r27);
    }

    static Vec const kandelaarOffset = {0.0f, -17.0f, 0.0f};
    mDoMtx_stack_c::multVec(&kandelaarOffset, &mKandelaarFlamePos);

    field_0x3618 = (mKandelaarFlamePos - field_0x3624) * 0.9f;

    if (!checkEndResetFlg1(ERFLG1_UNK_8)) {
        f32 scale = JMAFastSqrt(SQUARE(J3DSys::mCurrentMtx[0][0]) + SQUARE(J3DSys::mCurrentMtx[1][0]) + SQUARE(J3DSys::mCurrentMtx[2][0]));
        mDoMtx_stack_c::transS(J3DSys::mCurrentMtx[0][3], J3DSys::mCurrentMtx[1][3], J3DSys::mCurrentMtx[2][3]);
        mDoMtx_stack_c::ZXYrotM(var_r28, var_r29, var_r27);
        mDoMtx_stack_c::scaleM(scale, scale, scale);

        mpKanteraModel->setAnmMtx(1, mDoMtx_stack_c::get());
        cMtx_copy(mDoMtx_stack_c::get(), J3DSys::mCurrentMtx);
    }

    return 1;
}

static int daAlink_kandelaarModelCallBack(J3DJoint* i_joint, int param_1) {
    UNUSED(i_joint);

    daAlink_c* player_p = (daAlink_c*)j3dSys.getModel()->getUserArea();

    if (param_1 == 0) {
        player_p->kandelaarModelCallBack();
    }

    return 1;
}

cXyz* daAlink_c::getKandelaarFlamePos() {
    if ((!checkNoResetFlg2(daPy_FLG2(FLG2_UNK_20000 | FLG2_UNK_1)) && !checkEndResetFlg1(ERFLG1_UNK_4)) ||
        checkWolf() || !checkNoResetFlg1(FLG1_UNK_80))
    {
        return NULL;
    }

    return &mKandelaarFlamePos;
}

BOOL daAlink_c::checkKandelaarEquipAnime() const {
    return checkUpperAnime(dRes_ID_ALANM_BCK_WAITHK_e) || checkUpperAnime(dRes_ID_ALANM_BCK_WAITK_e);
}

void daAlink_c::preKandelaarDraw() {
    J3DMaterial* mat_p = mpKanteraGlowModel->getModelData()->getMaterialNodePointer(0);

    J3DGXColorS10 color;
    color.r = mpHIO->mItem.mLantern.m.mColorReg1R;
    color.g = mpHIO->mItem.mLantern.m.mColorReg1G;
    color.b = mpHIO->mItem.mLantern.m.mColorReg1B;
    color.a = 255;
    mat_p->setTevColor(1, &color);

    color.r = mpHIO->mItem.mLantern.m.mColorReg2R;
    color.g = mpHIO->mItem.mLantern.m.mColorReg2G;
    color.b = mpHIO->mItem.mLantern.m.mColorReg2B;
    mat_p->setTevColor(2, &color);

    cXyz proj;
    mDoLib_project(&mKandelaarFlamePos, &proj);

    camera_process_class* camera_p = dComIfGp_getCamera(0);
    f32 trimHeight;
    if (camera_p != NULL) {
        trimHeight = camera_p->mCamera.TrimHeight();
    } else {
        trimHeight = 0.0f;
    }

    if (proj.x > 0.0f && proj.x < FB_WIDTH && proj.y > trimHeight && proj.y < FB_HEIGHT - trimHeight) {
        dComIfGd_peekZ(proj.x, proj.y, &field_0x32c8);
    } else {
        field_0x32c8 = 0;
    }

    f32 near_ = dComIfGd_getView()->near_;
    f32 far_ = dComIfGd_getView()->far_;

    mDoLib_pos2camera(&mKandelaarFlamePos, &proj);
    proj.z += 30.0f;
    if (proj.z > -0.01f) {
        proj.z = -0.01f;
    }

    field_0x344c = ((near_ + (far_ * near_) / proj.z) / (far_ - near_) + 1.0f) * 1.6777215E7f;
}

void daAlink_c::setKandelaarModel() {
    field_0x2f94 = 0;

    if (!checkNoResetFlg2(FLG2_UNK_1)) {
        onNoResetFlg2(FLG2_UNK_1);

        mDoMtx_multVecZero(mpLinkModel->getAnmMtx(mLeftItemJntNo), &mKandelaarFlamePos);
        mKandelaarFlamePos.y -= 30.0f;

        field_0x3630 = field_0x3624;
        field_0x3624 = mKandelaarFlamePos;
        field_0x3618 = cXyz::Zero;

        field_0x32c8 = 0;
        field_0x3448 = 0.0f;
        field_0x344c = -1.0f;

        mpKanteraGlowModel->setBaseScale(cXyz::Zero);
    }

    mZ2Link.setKanteraState(1);
}

void daAlink_c::resetOilBottleModel() {
    deleteEquipItem(FALSE, TRUE);

    if (mProcVar3.field_0x300e != 0) {
        setKandelaarModel();
        mEquipItem = dItemNo_KANTERA_e;
    }

    returnKeepItemData();
}

void daAlink_c::commonKandelaarPourInit() {
    if (checkNoResetFlg2(FLG2_UNK_1)) {
        mProcVar3.field_0x300e = 1;
    } else {
        mProcVar3.field_0x300e = 0;
    }

    keepItemData();
    setKandelaarModel();
    setBottleModel(getReadyItem());

    mProcVar2.field_0x300c = 0;
    dComIfGp_setPlayerStatus1(0, 0x2000);
    field_0x3194 = 10;
    dCam_getBody()->StartEventCamera(0x12, fopAcM_GetID(this), "Type", 1, &field_0x3194, nullptr);
}

int daAlink_c::commonKandelaarPour(int param_0) {
    simpleAnmPlay(field_0x0718);
    simpleAnmPlay(field_0x071c);

    daPy_frameCtrl_c* frameCtrl_p;
    u16 current_anm;
    if (param_0) {
        frameCtrl_p = &mUpperFrameCtrl[2];
        current_anm = mUpperAnmHeap[2].getIdx();
    } else {
        frameCtrl_p = &mUnderFrameCtrl[0];
        current_anm = mUnderAnmHeap[0].getIdx();
    }

    onNoResetFlg2(FLG2_KANDELAAR_LIGHT_OFF);

    if (current_anm == getMainBckData(ANM_OIL_BOTTLE_POUR_START)->m_underID) {
        if (checkAnmEnd(frameCtrl_p)) {
            if (param_0) {
                setUpperAnimeBaseMorf(getMainBckData(ANM_OIL_BOTTLE_POUR)->m_upperID, 0.0f);
            } else {
                setSingleAnimeBaseMorf(ANM_OIL_BOTTLE_POUR, 0.0f);
            }

            dComIfGp_setItemOilCount(dComIfGs_getMaxOil());
        } else if (frameCtrl_p->checkPass(10.0f)) {
            field_0x071c->setFrame(0.0f);
            mHeldItemModel->getModelData()->entryTexMtxAnimator(field_0x071c);
        }
    } else if (current_anm == getMainBckData(ANM_OIL_BOTTLE_POUR)->m_underID) {
        if (dComIfGp_getItemNowOil() == dComIfGs_getOil()) {
            if (mProcVar2.field_0x300c == 0) {
                mProcVar2.field_0x300c = 1;
                mHeldItemModel->getModelData()->entryTexMtxAnimator(field_0x0720);
            } else if (simpleAnmPlay(field_0x0720)) {
                if (param_0) {
                    setUpperAnimeParam(getMainBckData(ANM_OIL_BOTTLE_POUR_END)->m_upperID, UPPER_2,
                                       &mpHIO->mItem.mLantern.m.mEndUnkAnm);
                } else {
                    setSingleAnimeParam(ANM_OIL_BOTTLE_POUR_END,
                                        &mpHIO->mItem.mLantern.m.mEndUnkAnm);
                }

                field_0x072c->setFrame(2.0f);
                dComIfGs_setEquipBottleItemEmpty(mSelectItemId);
            }
        }
    } else if (checkAnmEnd(frameCtrl_p)) {
        if (param_0) {
            resetUpperAnime(UPPER_2, 3.0f);
        }

        resetOilBottleModel();
        resetSpecialEvent();
        offNoResetFlg2(FLG2_KANDELAAR_LIGHT_OFF);
        return 1;
    }

    return 0;
}

void daAlink_c::initKandelaarSwing() {
    if (mEquipItem == dItemNo_KANTERA_e) {
        if (dComIfGs_getOil() != 0) {
            mZ2Link.getKantera().startSound(Z2SE_AL_KANTERA_SWING, 0, mVoiceReverbIntensity);
        } else {
            mZ2Link.getKantera().startSound(Z2SE_AL_KANTERA_OFF_SWING, 0, mVoiceReverbIntensity);
        }
    }

    voiceStart(Z2SE_AL_V_SWING_BOTTLE);
    mAtSph.ResetAtHit();

    if (!checkEventRun()) {
        dComIfGp_setItemOilCount(-mpHIO->mItem.mLantern.m.mShakeOilLoss);
    }

    mAtSph.OffAtSetBit();
    mAtSph.SetR(50.0f);
    mAtSph.SetAtType(AT_TYPE_LANTERN_SWING);
    mAtSph.SetAtHitMark(0);
    mAtSph.SetAtSe(dCcD_SE_NONE);
    mAtSph.SetAtAtp(0);
    mAtSph.SetAtMtrl(dCcD_MTRL_FIRE);
}

int daAlink_c::procKandelaarSwingInit() {
    fopAc_ac_c* catch_target_p = dComIfGp_att_getCatghTarget();
    if (catch_target_p != NULL) {
        s16 name = fopAcM_GetName(catch_target_p);
        BOOL is_sm2 = name == fpcNm_E_SM2_e;

        if (name == fpcNm_OBJ_OILTUBO_e ||
            (name == fpcNm_TAG_SSDRINK_e && ((daTag_SSDrink_c*)catch_target_p)->checkShopOil()) ||
            (is_sm2 && ((e_sm2_class*)catch_target_p)->checkYellow()))
        {
            return procBottleSwingInit(catch_target_p, is_sm2);
        }
    }

    if (!commonProcInitNotSameProc(PROC_KANDELAAR_SWING)) {
        return 0;
    }

    mNormalSpeed = 0.0f;
    current.angle.y = shape_angle.y;

    if ((mLinkAcch.ChkGroundHit() && getMoveBGActorName(mLinkAcch.m_gnd, FALSE) == fpcNm_OBJ_WEB1_e) ||
        (checkStageName("F_SP108") &&
         fopAcIt_Judge((fopAcIt_JudgeFunc)daAlink_searchTagKtOnFire, NULL) != NULL))
    {
        setSingleAnimeParam(ANM_BOTTLE_SWING_DOWN, &mpHIO->mItem.mBottle.m.mSwingDownAnm);
        field_0x3478 = mpHIO->mItem.mBottle.m.mSwingDownAnm.mCancelFrame;
        field_0x347c = 5.0f;
        field_0x3480 = 12.0f;
    } else {
        setSingleAnimeParam(ANM_LANTERN_SWING, &mpHIO->mItem.mLantern.m.mShakeAnm);
        field_0x3478 = mpHIO->mItem.mLantern.m.mShakeAnm.mCancelFrame;
        field_0x347c = 4.0f;
        field_0x3480 = 11.0f;
    }

    initKandelaarSwing();
    return 1;
}

int daAlink_c::procKandelaarSwing() {
    daPy_frameCtrl_c* frameCtrl_p = mUnderFrameCtrl;

    if (checkAnmEnd(frameCtrl_p)) {
        if (mDemo.getDemoMode() == daPy_demo_c::DEMO_KANDELAAR_SWING_e) {
            dComIfGp_evmng_cutEnd(mAlinkStaffId);
        } else {
            checkNextAction(0);
        }
    } else if (frameCtrl_p->getFrame() > field_0x3478 && mDemo.getDemoMode() != daPy_demo_c::DEMO_KANDELAAR_SWING_e) {
        onModeFlg(4);
        checkNextAction(1);
    }

    return 1;
}

int daAlink_c::procKandelaarPourInit() {
    if (!dComIfGp_event_compulsory(this, NULL, 0xFBFF)) {
        return 0;
    }

    mDemo.setSpecialDemoType();
    commonProcInit(PROC_KANDELAAR_POUR);
    current.angle.y = shape_angle.y;
    mNormalSpeed = 0.0f;
    field_0x2f98 = 4;
    setSingleAnimeParam(ANM_OIL_BOTTLE_POUR_START, &mpHIO->mItem.mLantern.m.mBeginUnkAnm);
    commonKandelaarPourInit();
    return 1;
}

int daAlink_c::procKandelaarPour() {
    commonKandelaarPour(0);
    return 1;
}

#if TARGET_PC
static int daAlink_ghostKandelaarModelCallBack(J3DJoint* i_joint, int i_timing) {
    UNUSED(i_joint);
    if (i_timing == 0) {
        daAlink_c* link = (daAlink_c*)j3dSys.getModel()->getUserArea();
        if (link != NULL) {
            link->ghostKandelaarModelCallBack();
        }
    }
    return 1;
}

// Wii U daAlink_c::ghostKandelaarModelCallBack (RPX 0x02015c70). Same pendulum as the oil lantern,
// but the bob (+0x3710) and the flame (+0x3704) are kept apart: the flame is joint * (45, 0, 0) of
// the swung matrix, the bob joint * (0, -33, 0); the velocity (+0x371c) is (bob - previous) * 0.9.
int daAlink_c::ghostKandelaarModelCallBack() {
    cXyz jointPos;
    mDoMtx_multVecZero(J3DSys::mCurrentMtx, &jointPos);

    mGhostKandelaarPos1 = mGhostKandelaarPos0;
    mGhostKandelaarPos0 = mGhostKandelaarPos2;

    cXyz swing = (mGhostKandelaarPos2 - jointPos) + mGhostKandelaarPos3;
    swing.y -= 3.0f;

    cXyz dirZ;
    mDoMtx_multVec(J3DSys::mCurrentMtx, &cXyz::BaseZ, &dirZ);
    s16 yaw = dirZ.atan2sX_Z();
    mDoMtx_stack_c::YrotS(-yaw);
    mDoMtx_stack_c::multVec(&swing, &swing);

    s16 angleX = cLib_minMaxLimit<s16>(cM_atan2s(-swing.z, -swing.y), -0x1800, 0x1800);
    s16 angleZ = cLib_minMaxLimit<s16>(
        cM_atan2s(swing.x, JMAFastSqrt(SQUARE(swing.y) + SQUARE(swing.z))), -0x1800, 0x1800);

    mDoMtx_stack_c::transS(jointPos);
    if (mProcID != PROC_OPEN_TREASURE && (mProcID != PROC_GET_ITEM || mProcVar4.field_0x3010 == 0)) {
        mDoMtx_stack_c::ZXYrotM(angleX, yaw, angleZ);
    }
    static Vec const bobOffset = {0.0f, -33.0f, 0.0f};  // 0x10006ab8
    mDoMtx_stack_c::multVec(&bobOffset, &mGhostKandelaarPos2);
    mGhostKandelaarPos3 = (mGhostKandelaarPos2 - mGhostKandelaarPos0) * 0.9f;  // 0x1000702c

    f32 scale = JMAFastSqrt(SQUARE(J3DSys::mCurrentMtx[0][0]) + SQUARE(J3DSys::mCurrentMtx[1][0]) +
                            SQUARE(J3DSys::mCurrentMtx[2][0]));
    mDoMtx_stack_c::transS(J3DSys::mCurrentMtx[0][3], J3DSys::mCurrentMtx[1][3], J3DSys::mCurrentMtx[2][3]);
    mDoMtx_stack_c::ZXYrotM(angleX, (s16)(yaw - 0x4000), (s16)(angleZ - 0x4000));
    mDoMtx_stack_c::scaleM(scale, scale, scale);

    mGhostKandelaarAngleY = yaw;
    mGhostKandelaarAngleX = angleX;
    mGhostKandelaarAngleZ = angleZ;

    if (mpGhostKanteraModel != NULL) {
        mpGhostKanteraModel->setAnmMtx(1, mDoMtx_stack_c::get());
        cMtx_copy(mDoMtx_stack_c::get(), J3DSys::mCurrentMtx);
    }

    static Vec const flameOffset = {45.0f, 0.0f, 0.0f};  // 0x10006ac4
    mDoMtx_stack_c::multVec(&flameOffset, &mGhostKandelaarFlamePos);
    return 1;
}


// Wii U Poe soul table (RPX 0x1001e8d8, 60 entries of 0x18 bytes): stage, room, layer (-1 = any),
// map region (unused here), save table of the switch, switch bit.
struct GhostPoeEntry {
    const char* stage;
    s32 room;
    s32 layer;
    s32 region;
    s32 saveTbl;
    s32 bit;
};

static const GhostPoeEntry l_ghostPoeTable[60] = {
    {"D_MN06", 2, -1, -1, 21, 25},   {"D_MN06", 5, -1, -1, 21, 24},
    {"D_MN07", 12, -1, -1, 22, 84},  {"D_MN07", 13, -1, -1, 22, 85},
    {"D_MN10", 2, -1, -1, 19, 30},   {"D_MN10", 3, -1, -1, 19, 31},
    {"D_MN10", 5, -1, -1, 19, 32},   {"D_MN10", 8, -1, -1, 19, 33},
    {"D_MN11", 0, -1, -1, 20, 114},  {"D_MN11", 0, -1, -1, 20, 21},
    {"D_MN11", 6, -1, -1, 20, 127},  {"D_SB01", 16, -1, 4, 25, 69},
    {"D_SB01", 32, -1, 4, 25, 70},   {"D_SB01", 43, -1, 4, 25, 71},
    {"D_SB02", 0, -1, 2, 25, 96},    {"D_SB03", 0, -1, 3, 26, 94},
    {"D_SB03", 0, -1, 3, 26, 95},    {"D_SB03", 0, -1, 3, 26, 93},
    {"D_SB05", 0, 3, 3, 27, 10},     {"D_SB05", 0, 3, 3, 27, 11},
    {"D_SB07", 2, 0, 4, 27, 15},     {"D_SB07", 2, 0, 4, 27, 16},
    {"F_SP108", 5, -1, 1, 2, 93},    {"F_SP109", 0, -1, 2, 3, 94},
    {"F_SP109", 0, -1, 2, 3, 95},    {"F_SP110", 0, -1, 2, 3, 89},
    {"F_SP111", 0, -1, 2, 3, 87},    {"F_SP111", 0, -1, 2, 3, 88},
    {"F_SP113", 1, -1, 3, 4, 73},    {"F_SP113", 1, -1, 3, 4, 74},
    {"F_SP114", 0, -1, 5, 8, 124},   {"F_SP114", 0, -1, 5, 8, 125},
    {"F_SP114", 0, -1, 5, 8, 123},   {"F_SP114", 1, -1, 5, 8, 126},
    {"F_SP114", 2, -1, 5, 8, 127},   {"F_SP115", 0, -1, 3, 4, 70},
    {"F_SP115", 0, -1, 3, 4, 71},    {"F_SP115", 0, -1, 3, 4, 75},
    {"F_SP115", 0, -1, 3, 4, 76},    {"F_SP115", 0, -1, 3, 4, 77},
    {"F_SP117", 1, -1, 1, 7, 15},    {"F_SP117", 2, -1, 1, 7, 30},
    {"F_SP117", 3, -1, 1, 7, 16},    {"F_SP117", 3, -1, 1, 7, 17},
    {"F_SP118", 1, -1, 4, 10, 120},  {"F_SP118", 3, -1, 4, 10, 90},
    {"F_SP121", 3, -1, 2, 6, 58},    {"F_SP121", 6, -1, 1, 6, 57},
    {"F_SP121", 10, -1, 3, 6, 51},   {"F_SP121", 13, -1, 3, 6, 59},
    {"F_SP122", 8, -1, 3, 6, 73},    {"F_SP122", 16, -1, 3, 6, 48},
    {"F_SP122", 17, -1, 3, 6, 71},   {"F_SP124", 0, -1, 4, 10, 91},
    {"F_SP124", 0, -1, 4, 10, 92},   {"F_SP124", 0, -1, 4, 10, 93},
    {"F_SP124", 0, -1, 4, 10, 51},   {"F_SP126", 0, -1, 3, 4, 72},
    {"F_SP128", 0, -1, 2, 3, 64},    {"R_SP160", 5, -1, 3, 9, 31},
};

// RPX 0x0227d3c0: the switch is read from the live memory bits when the entry belongs to the
// current save table, otherwise from the saved stage memory.
static bool ghostPoeCollected(const GhostPoeEntry& i_entry) {
    if (dStage_stagInfo_GetSaveTbl(dComIfGp_getStage()->getStagInfo()) == i_entry.saveTbl) {
        return dComIfGs_isSaveSwitch(i_entry.bit);
    }
    return dComIfGs_isSaveSwitch(i_entry.saveTbl, i_entry.bit);
}

// RPX 0x0227d69c: any uncollected Poe anywhere in the stage (dungeon-like stages).
bool daAlink_c::checkPoeExistStage(const char* i_stageName) {
    for (const GhostPoeEntry& e : l_ghostPoeTable) {
        if (strcmp(i_stageName, e.stage) == 0 && !ghostPoeCollected(e)) {
            return true;
        }
    }
    return false;
}

// RPX 0x0227d7d8: uncollected Poe in the current room (and layer). F_SP128 only counts once the
// events 0x2320 and 0x5f80 are set. The Poes of D_SB05 / D_SB07 / D_SB01 are also announced from
// F_SP121 room 10, F_SP124 room 0 and D_SB01 room 0.
bool daAlink_c::checkPoeExistRoom(const char* i_stageName, int i_roomNo, int i_layerNo) {
    for (const GhostPoeEntry& e : l_ghostPoeTable) {
        if (ghostPoeCollected(e)) {
            continue;
        }
        if (e.room == i_roomNo && strcmp(i_stageName, e.stage) == 0) {
            if (strcmp(i_stageName, "F_SP128") != 0 ||
                (dComIfGs_isEventBit(0x2320) && dComIfGs_isEventBit(0x5f80)))
            {
                if (e.layer < 0 || e.layer == i_layerNo) {
                    return true;
                }
            }
            continue;
        }
        if (strncmp(e.stage, "D_SB", 3) == 0) {
            if (strcmp(e.stage + 4, "05") == 0) {
                if (strcmp(i_stageName, "F_SP121") == 0 && i_roomNo == 10) {
                    return true;
                }
            } else if (strcmp(e.stage + 4, "07") == 0) {
                if (strcmp(i_stageName, "F_SP124") == 0 && i_roomNo == 0) {
                    return true;
                }
            } else if (strcmp(e.stage + 4, "01") == 0) {
                if (strcmp(i_stageName, "D_SB01") == 0 && i_roomNo == 0) {
                    return true;
                }
            }
        }
    }
    return false;
}

// RPX 0x0201baa8: dungeon-like stages (stag info +0xa & 7 == 1 or 7) check the whole stage, the
// others the current room and layer. The result is kept in +0x334c.
bool daAlink_c::checkPoeExist() {
    const char* stageName = dComIfGp_getStartStageName();
    const u32 type = dComIfGp_getStage()->getStagInfo()->field_0x0a & 7;
    if (type == 1 || type == 7) {
        return checkPoeExistStage(stageName);
    }
    return checkPoeExistRoom(stageName, fopAcM_GetRoomNo(this), dComIfG_play_c::getLayerNo(0));
}

void daAlink_c::preGhostKandelaarDraw() {
    if (mpGhostKanteraFlameModel == NULL) return;
    J3DMaterial* mat_p = mpGhostKanteraFlameModel->getModelData()->getMaterialNodePointer(0);
    if (mat_p == NULL) return;

    J3DGXColorS10 color;
    color.r = 20;
    color.g = 40;
    color.b = 80;
    color.a = 255;
    mat_p->setTevColor(1, &color);

    color.r = 10;
    color.g = 30;
    color.b = 40;
    color.a = 255;
    mat_p->setTevColor(2, &color);

    // Same depth test as the oil lantern (preKandelaarDraw): peekZ at the flame (+0x3348) against
    // its projected depth (+0x3514); the flame shrinks away when something covers it.
    cXyz proj;
    mDoLib_project(&mGhostKandelaarFlamePos, &proj);

    camera_process_class* camera_p = dComIfGp_getCamera(0);
    f32 trimHeight = (camera_p != NULL) ? camera_p->mCamera.TrimHeight() : 0.0f;

    if (proj.x > 0.0f && proj.x < FB_WIDTH && proj.y > trimHeight && proj.y < FB_HEIGHT - trimHeight) {
        dComIfGd_peekZ(proj.x, proj.y, &field_ghost_0x3348);
    } else {
        field_ghost_0x3348 = 0;
    }

    f32 near_ = dComIfGd_getView()->near_;
    f32 far_ = dComIfGd_getView()->far_;

    mDoLib_pos2camera(&mGhostKandelaarFlamePos, &proj);
    proj.z += 30.0f;
    if (proj.z > -0.01f) {
        proj.z = -0.01f;
    }
    mGhostFlameDepth = ((near_ + (far_ * near_) / proj.z) / (far_ - near_) + 1.0f) * 1.6777215E7f;

    if (mpGhostKanteraFlameBtk != NULL) {
        mpGhostKanteraFlameModel->getModelData()->entryTexMtxAnimator(mpGhostKanteraFlameBtk);
    }
}

// Wii U daAlink_c::setGhostKandelaarModel (RPX 0x0201bb3c).
void daAlink_c::setGhostKandelaarModel() {
    field_0x2f94 = 0;

    static Vec const bobOffset = {0.0f, -33.0f, 0.0f};
    mDoMtx_stack_c::copy(mpLinkModel->getAnmMtx(mLeftItemJntNo));
    mDoMtx_stack_c::multVec(&bobOffset, &mGhostKandelaarPos2);
    mGhostKandelaarPos0 = mGhostKandelaarPos2;
    mGhostKandelaarPos1 = mGhostKandelaarPos2;
    mGhostKandelaarFlamePos = mGhostKandelaarPos2;
    mGhostKandelaarPos3 = cXyz::Zero;

    field_ghost_0x3348 = 0;
    mGhostFlameScale = 0.0f;
    mGhostFlameDepth = -1.0f;
    mGhostPoeExist = checkPoeExist();
    mGhostPoeRoom = fopAcM_GetRoomNo(this);

    if (mpGhostKanteraFlameModel != NULL) {
        mpGhostKanteraFlameModel->setBaseScale(cXyz::Zero);
    }

    mZ2Link.setGhostKanteraState(1);
}
#endif
