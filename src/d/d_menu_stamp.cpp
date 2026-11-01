/**
 * d_menu_stamp.cpp
 * Menu - Stamps (TPHD only)
 *
 * Port of the Wii U dMenu_Stamp_c (Twilight Princess HD RPX):
 *   ctor 0x02a11898, dtor 0x02a11b70, isStamp 0x02a120ec (-> 0x02aa7854), cellStamp 0x02a1214c,
 *   setInfoText 0x02a12188, setPageText 0x02a1229c, hideCellText 0x02a12334, setCursor 0x02a12438,
 *   setStampPanes 0x02a1251c, setButtonText 0x02a12aec, refresh 0x02a12b50, initCursor 0x02a12ba0,
 *   screenInitBg 0x02a13024, screenInitMenu 0x02a13220, screenInitButtons 0x02a139d8,
 *   screenInitMsg 0x02a13e04, stateInit 0x02a13ffc, create 0x02a14024, _open 0x02a140b0,
 *   moveNormal 0x02a144c0, moveUnlock 0x02a14b94, moveMessage 0x02a14d24, _move 0x02a14e48,
 *   draw 0x02a14ef0.
 * Not ported: the Miiverse "Post" cell / button (third row of the Wii U grid, 0x10123ff0 = 2) and
 * the touch screen handling.
 */

#include "d/dolzel.h" // IWYU pragma: keep

#include "d/d_menu_stamp.h"
#include "JSystem/J2DGraph/J2DPicture.h"
#include "JSystem/J2DGraph/J2DScreen.h"
#include "JSystem/J2DGraph/J2DTextBox.h"
#include "JSystem/JKernel/JKRExpHeap.h"
#include "JSystem/JKernel/JKRMemArchive.h"
#include "d/d_com_inf_game.h"
#include "d/d_lib.h"
#include "d/d_meter2_info.h"
#include "d/d_msg_string.h"
#include "d/d_pane_class.h"
#include "d/d_select_cursor.h"
#include "m_Do/m_Do_controller_pad.h"
#include "m_Do/m_Do_dvd_thread.h"
#include "m_Do/m_Do_graphic.h"
#include <cstdio>
#include <cstring>

#if TARGET_PC
#include "aurora/lib/input.hpp"
#include "dusk/action_bindings.h"
#include "dusk/settings.h"
#include "dusk/tphd/HdSubscreen.hpp"
#endif

namespace {
// Wii U message ids (bmgres zel_00.bmg).
const u32 kMsgTitle = 0xaf0;      // "Stamps"
const u32 kMsgPage = 0xaf2;       // "%d/%d"
const u32 kMsgComplete = 0xaf6;   // reward message for 49 stamps
const u32 kMsgStampName = 0xb4a;  // + stamp number
const u16 kMsgBack = 0x3f9;       // "Back"

// RPX constants (0x100987ac ..).
const f32 kCursorOffsetX = -1.75f;
const f32 kCursorOffsetY = -1.5f;
const f32 kSelectScale = 1.1f;
const f32 kUnselectScale = 1.0f;

// MenuPageLeft / MenuPageRight (RPX 0x02a1b0a8 / 0x02a1b158): the Pro Controller L / R shoulder
// buttons (bound to "Call Midna" / "Extra Item Button (R)"), not ZL / ZR. Without a binding the
// GC L / R triggers are used (same as the letters menu).
bool pageTrig(bool i_right) {
#if TARGET_PC
    // Physical L / R shoulder of the pad (edge-triggered).
    static bool s_prev[2] = {false, false};
    bool shoulder = false;
    int numJoysticks = 0;
    if (SDL_JoystickID* joysticks = SDL_GetGamepads(&numJoysticks)) {
        for (int i = 0; i < numJoysticks; i++) {
            if (SDL_Gamepad* gp = SDL_GetGamepadFromID(joysticks[i])) {
                if (SDL_GetGamepadButton(gp, i_right ? SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER
                                                     : SDL_GAMEPAD_BUTTON_LEFT_SHOULDER)) {
                    shoulder = true;
                    break;
                }
            }
        }
        SDL_free(joysticks);
    }
    if (!shoulder) {
        if (auto* pad = aurora::input::get_controller_for_player(0); pad != nullptr && pad->m_controller) {
            shoulder = SDL_GetGamepadButton(pad->m_controller, i_right ? SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER
                                                                       : SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
        }
    }
    const bool trig = shoulder && !s_prev[i_right ? 1 : 0];
    s_prev[i_right ? 1 : 0] = shoulder;
    if (trig) {
        return true;
    }
    const dusk::ActionBinds b =
        i_right ? dusk::ActionBinds::USE_ITEM_R : dusk::ActionBinds::CALL_MIDNA;
    if (dusk::isActionBound(b, PAD_1)) {
        return dusk::getActionBindTrig(b, PAD_1);
    }
#endif
    return i_right ? mDoCPd_c::getTrigR(PAD_1) != 0 : false;
}

u64 tag8(const char* s) {
    u64 t = 0;
    for (int i = 0; i < 8 && s[i] != 0; i++) {
        t = (t << 8) | static_cast<u8>(s[i]);
    }
    return t;
}

J2DTextBox* textBox(J2DScreen* scr, const char* tag) {
    if (scr == NULL) {
        return NULL;
    }
    J2DPane* p = scr->search(tag8(tag));
    return (p != NULL && p->getTypeID() == 19) ? static_cast<J2DTextBox*>(p) : NULL;
}

void initText(J2DTextBox* tb) {
    if (tb != NULL) {
        tb->setFont(mDoExt_getMesgFont());
        tb->setString(0x100, "");
    }
}
}  // namespace

int dMenu_Stamp_c::getStampNum() {
    int num = 0;
    for (int i = 0; i < STAMP_MAX; i++) {
        if (dComIfGs_isStamp(i)) {
            num++;
        }
    }
    return num;
}

dMenu_Stamp_c::dMenu_Stamp_c(JKRExpHeap* i_heap, STControl* i_stick, CSTControl* i_cstick,
                             u8 i_mode) {
    mpHeap = i_heap;
    mpArchive = NULL;
    mpMount = NULL;
    mpStick = i_stick;
    mpCStick = i_cstick;
    mpBgScreen = NULL;
    mpMenuScreen = NULL;
    mpButtonScreen = NULL;
    mpMsgScreen = NULL;
    mpBlackTex = NULL;
    mpString = NULL;
    mpDrawCursor = NULL;
    for (int i = 0; i < 10; i++) {
        mpCell[i] = NULL;
        mpEmpty[i] = NULL;
        mpBgNormal[i] = NULL;
        mpBgSelect[i] = NULL;
    }
    for (int i = 0; i < STAMP_MAX; i++) {
        mpStamp[i] = NULL;
    }
    mpInfoText = NULL;
    mpPageText = NULL;
    mpTitleText = NULL;
    mpMsgText = NULL;
    mpMsgAll = NULL;
    mRow = 0;
    mCol = 0;
    mPage = 0;
    mState = 0;
    mFrame = 0;
    mMode = i_mode;
    mStatus = 1;
    mOverrideFlags = 0;
}

dMenu_Stamp_c::~dMenu_Stamp_c() {
    for (int i = 0; i < 10; i++) {
        if (mpCell[i] != NULL) {
            JKR_DELETE(mpCell[i]);
            mpCell[i] = NULL;
        }
    }
    if (mpDrawCursor != NULL) {
        JKR_DELETE(mpDrawCursor);
        mpDrawCursor = NULL;
    }
    if (mpString != NULL) {
        JKR_DELETE(mpString);
        mpString = NULL;
    }
    if (mpBlackTex != NULL) {
        JKR_DELETE(mpBlackTex);
        mpBlackTex = NULL;
    }
    if (mpMsgScreen != NULL) {
        JKR_DELETE(mpMsgScreen);
        mpMsgScreen = NULL;
    }
    if (mpButtonScreen != NULL) {
        JKR_DELETE(mpButtonScreen);
        mpButtonScreen = NULL;
    }
    if (mpMenuScreen != NULL) {
        JKR_DELETE(mpMenuScreen);
        mpMenuScreen = NULL;
    }
    if (mpBgScreen != NULL) {
        JKR_DELETE(mpBgScreen);
        mpBgScreen = NULL;
    }
    if (mpMount != NULL) {
        mpMount->getArchive()->unmount();
        JKR_DELETE(mpMount);
        mpMount = NULL;
    }
    if (mpArchive != NULL) {
        mpArchive->unmount();
        mpArchive = NULL;
    }
}

bool dMenu_Stamp_c::isSync() {
    if (mpMount != NULL && mpMount->sync() == false) {
        return false;
    }
    return true;
}

bool dMenu_Stamp_c::isOwned(int i_no) const {
    if (i_no < 0 || i_no >= STAMP_MAX) {
        return false;
    }
    if (mOverrideFlags != 0) {
        return (mOverrideFlags & ((u64)1 << i_no)) != 0;
    }
    return dComIfGs_isStamp(i_no) != 0;
}

// RPX 0x02a1214c: stamp number of a cell of the current page (-1 for the "Post" row).
int dMenu_Stamp_c::cellStamp(int i_row, int i_col) const {
    if (i_row < 0 || i_row > 1 || i_col < 0 || i_col > 4) {
        return -1;
    }
    return i_col + i_row * 5 + mPage * 10;
}

// RPX 0x02a140b0
int dMenu_Stamp_c::_open() {
    if (mpArchive == NULL) {
        if (mpMount == NULL) {
            mpMount = mDoDvdThd_mountArchive_c::create("/res/LayoutRevo/stampResR.arc", 0, NULL);
        }
        if (mpMount->sync() == 0) {
            return 0;
        }
        mpArchive = mpMount->getArchive();
        JKR_DELETE(mpMount);
        mpMount = NULL;
        _create();
    }
    mStatus = 2;
    return 1;
}

int dMenu_Stamp_c::_close() {
    mStatus = 0;
    return 1;
}

// RPX 0x02a14024
void dMenu_Stamp_c::_create() {
    mpString = JKR_NEW dMsgString_c();
    screenInitBg();
    screenInitMenu();
    screenInitButtons();
    screenInitMsg();
    initCursor();
    mState = (mMode == MODE_COMPLETE) ? 1 : 0;
    stateInit();
}

// RPX 0x02a13024: zelda_stamps_background.blo, title "Stamps", black backdrop.
void dMenu_Stamp_c::screenInitBg() {
    mpBgScreen = JKR_NEW J2DScreen();
    mpBgScreen->setPriority("zelda_stamps_background.blo", 0x20000, mpArchive);
    dPaneClass_showNullPane(mpBgScreen);
    mpTitleText = textBox(mpBgScreen, "title_tx");
    initText(mpTitleText);
    if (mpTitleText != NULL) {
        mpString->getString(kMsgTitle, mpTitleText, NULL, NULL, NULL, 0);
    }

    ResTIMG* timg = (ResTIMG*)dComIfGp_getMain2DArchive()->getResource('TIMG', "tt_block8x8.bti");
    mpBlackTex = JKR_NEW J2DPicture(timg);
    mpBlackTex->setBlackWhite(JUtility::TColor(0, 0, 0, 0), JUtility::TColor(0, 0, 0, 0xff));
    mpBlackTex->setAlpha(0xff);
}

// RPX 0x02a13220: zelda_stamps_menu.blo.
void dMenu_Stamp_c::screenInitMenu() {
    mpMenuScreen = JKR_NEW J2DScreen();
    mpMenuScreen->setPriority("zelda_stamps_menu.blo", 0x20000, mpArchive);
    dPaneClass_showNullPane(mpMenuScreen);

    mpInfoText = textBox(mpMenuScreen, "info_txt");
    initText(mpInfoText);
    mpPageText = textBox(mpMenuScreen, "page_txt");
    initText(mpPageText);

    // No Miiverse here: the "Post" button is not shown.
    if (J2DPane* post = mpMenuScreen->search(tag8("N_postbt"))) {
        post->hide();
    }

    char name[16];
    for (int c = 0; c < 10; c++) {
        snprintf(name, sizeof(name), "N_it%04d", c);
        mpCell[c] = JKR_NEW CPaneMgr(mpMenuScreen, tag8(name), 0, NULL);
        snprintf(name, sizeof(name), "it_e%02d", c + 1);
        mpEmpty[c] = mpMenuScreen->search(tag8(name));
        snprintf(name, sizeof(name), "it_i%04d", c);
        mpBgNormal[c] = mpMenuScreen->search(tag8(name));
        // Selected background: it_i0010..14 for the first row, it_i0019..15 for the second.
        snprintf(name, sizeof(name), "it_i%04d", c < 5 ? 10 + c : 24 - c);
        mpBgSelect[c] = mpMenuScreen->search(tag8(name));
        // RPX 0x02a12334: the per-cell text boxes stay hidden.
        snprintf(name, sizeof(name), "it_t%04d", c);
        if (J2DPane* t = mpMenuScreen->search(tag8(name))) {
            t->hide();
        }
        for (int p = 0; p < PAGE_NUM; p++) {
            snprintf(name, sizeof(name), "it_s%02d%d%d", 0, p, c);
            mpStamp[p * 10 + c] = mpMenuScreen->search(tag8(name));
        }
    }

    mpDrawCursor = JKR_NEW dSelect_cursor_c(2, 1.0f, NULL);
    mpDrawCursor->setParam(0.88f, 0.93f, 0.1f, 0.7f, 0.875f);
    mpDrawCursor->setAlphaRate(1.0f);
    mpDrawCursor->onPlayAnime(0);
}

// RPX 0x02a139d8: zelda_stamps_buttons.blo (A / B captions).
void dMenu_Stamp_c::screenInitButtons() {
    mpButtonScreen = JKR_NEW J2DScreen();
    mpButtonScreen->setPriority("zelda_stamps_buttons.blo", 0x20000, mpArchive);
    dPaneClass_showNullPane(mpButtonScreen);
#if TARGET_PC
    dusk::tphd::setupRevoButtonText(mpButtonScreen);
#endif
}

// RPX 0x02a13e04: zelda_stamps_message.blo (49 stamps reward), hidden until needed.
void dMenu_Stamp_c::screenInitMsg() {
    mpMsgScreen = JKR_NEW J2DScreen();
    mpMsgScreen->setPriority("zelda_stamps_message.blo", 0x20000, mpArchive);
    dPaneClass_showNullPane(mpMsgScreen);
    mpMsgAll = mpMsgScreen->search(tag8("n_all"));
    if (mpMsgAll != NULL) {
        mpMsgAll->setAlpha(0);
    }
    mpMsgText = textBox(mpMsgScreen, "msg_text");
    initText(mpMsgText);
    if (mpMsgText != NULL) {
        mpString->getString(kMsgComplete, mpMsgText, NULL, NULL, NULL, 0);
    }
}

// RPX 0x02a12ba0: first owned stamp, page by page; else page 1, first row, second column.
void dMenu_Stamp_c::initCursor() {
    for (int p = 0; p < PAGE_NUM; p++) {
        mPage = p;
        for (int r = 0; r < 2; r++) {
            for (int c = 0; c < 5; c++) {
                if (isOwned(cellStamp(r, c))) {
                    mRow = r;
                    mCol = c;
                    refresh();
                    return;
                }
            }
        }
    }
    mPage = 0;
    mRow = 0;
    mCol = 1;
    refresh();
}

// RPX 0x02a12188: name of the selected stamp (empty when not owned).
void dMenu_Stamp_c::setInfoText() {
    if (mpInfoText == NULL) {
        return;
    }
    const int no = cellStamp(mRow, mCol);
    if (no >= 0 && isOwned(no)) {
        mpString->getString(kMsgStampName + no, mpInfoText, NULL, NULL, NULL, 0);
    } else {
        SAFE_STRCPY(mpInfoText->getStringPtr(), "");
    }
}

// RPX 0x02a1229c: "page / pages".
void dMenu_Stamp_c::setPageText() {
    if (mpPageText == NULL) {
        return;
    }
    mpString->getString(kMsgPage, mpPageText, NULL, NULL, NULL, 0);
    char fmt[64];
    char buf[64];
    snprintf(fmt, sizeof(fmt), "%s", (const char*)mpPageText->getStringPtr());
    if (strstr(fmt, "%d") == NULL) {
        snprintf(fmt, sizeof(fmt), "%%d/%%d");
    }
    snprintf(buf, sizeof(buf), fmt, mPage + 1, (int)PAGE_NUM);
    SAFE_STRCPY(mpPageText->getStringPtr(), buf);
}

// RPX 0x02a1251c: stamps of the current page (owned: the stamp, else the empty icon), the
// selected cell grows to 1.1 and shows its "selected" background.
void dMenu_Stamp_c::setStampPanes() {
    const int sel = mRow * 5 + mCol;
    for (int no = 0; no < STAMP_MAX; no++) {
        J2DPane* stamp = mpStamp[no];
        if (stamp == NULL) {
            continue;
        }
        if (no / 10 != mPage) {
            stamp->hide();
            continue;
        }
        const int c = no % 10;
        // The last stamp stays hidden while its unlock animation runs.
        const bool hideLast = no == STAMP_MAX - 1 && mState == 1;
        if (isOwned(no) && !hideLast) {
            stamp->show();
            if (mpEmpty[c] != NULL) {
                mpEmpty[c]->hide();
            }
        } else {
            stamp->hide();
            if (mpEmpty[c] != NULL) {
                mpEmpty[c]->show();
            }
        }
        const bool selected = c == sel;
        const f32 s = selected ? kSelectScale : kUnselectScale;
        stamp->scale(s, s);
        if (mpBgSelect[c] != NULL) {
            if (selected) {
                mpBgSelect[c]->show();
            } else {
                mpBgSelect[c]->hide();
            }
            mpBgSelect[c]->scale(s, s);
        }
        if (mpBgNormal[c] != NULL) {
            if (selected) {
                mpBgNormal[c]->hide();
            } else {
                mpBgNormal[c]->show();
            }
        }
    }
}

// RPX 0x02a12aec: B "Back"; A only on the (not ported) "Post" cell.
void dMenu_Stamp_c::setButtonText() {
#if TARGET_PC
    dusk::tphd::setRevoButtonText(mpButtonScreen, 0, true);
    dusk::tphd::setRevoButtonText(mpButtonScreen, kMsgBack, false);
#endif
}

// RPX 0x02a12b50
void dMenu_Stamp_c::refresh() {
    setInfoText();
    setPageText();
    setStampPanes();
    setButtonText();
}

// RPX 0x02a13ffc
void dMenu_Stamp_c::stateInit() {
    if (mState == 1) {
        // Unlock animation: last page, cell of the last stamp.
        mFrame = 0;
        mRow = 1;
        mPage = PAGE_NUM - 1;
        mCol = 4;
    } else if (mState == 2) {
        if (mpMsgAll != NULL) {
            mpMsgAll->setAlpha(0);
        }
        mFrame = 0;
        return;
    } else {
        mFrame = 0;
    }
    refresh();
}

// RPX 0x02a14e48
void dMenu_Stamp_c::_move() {
    if (mpArchive == NULL || mStatus != 2) {
        return;
    }
    JKRHeap* heap = mDoExt_setCurrentHeap((JKRHeap*)mpHeap);
    const int state = mState;
    if (state == 0) {
        moveNormal();
    } else if (state == 1) {
        moveUnlock();
    } else if (state == 2) {
        moveMessage();
    }
    if (state != mState) {
        stateInit();
    }
    mDoExt_setCurrentHeap(heap);
}

// RPX 0x02a144c0
void dMenu_Stamp_c::moveNormal() {
    if (mDoGph_gInf_c::getFader()->getStatus() != 1) {
        return;
    }
    const bool trigA = mDoCPd_c::getTrigA(PAD_1) != 0;
    const bool trigB = mDoCPd_c::getTrigB(PAD_1) != 0;
    const bool left = mpStick->checkLeftTrigger();
    const bool right = mpStick->checkRightTrigger();
    const bool up = mpStick->checkUpTrigger();
    const bool down = mpStick->checkDownTrigger();
    const bool pageLeft = pageTrig(false);
    const bool pageRight = pageTrig(true);

    if (mMode == MODE_VIEW) {
        mFrame++;
        // Shown after getting a stamp: any button closes it, or auto-closes after 90 frames (~3s).
        if (trigA || trigB || left || right || up || down || pageLeft || pageRight || mFrame >= 90) {
            Z2GetAudioMgr()->seStart(Z2SE_SY_CURSOR_CANCEL, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f,
                                     0);
            mStatus = 3;
        }
        return;
    }
    if (trigB) {
        Z2GetAudioMgr()->seStart(Z2SE_SY_CURSOR_CANCEL, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
        mStatus = 3;
        return;
    }

    int row = mRow;
    int col = mCol;
    int page = mPage;
    if (left) {
        col--;
    } else if (right) {
        col++;
    } else if (up) {
        row--;
    } else if (down) {
        row++;
    } else if (pageLeft) {
        page = (page + PAGE_NUM - 1) % PAGE_NUM;
    } else if (pageRight) {
        page = (page + 1) % PAGE_NUM;
    } else {
        return;
    }
    // Only the two stamp rows: the Wii U third row is the Miiverse "Post" button.
    if (col < 0 || col > 4 || row < 0 || row > 1) {
        return;
    }
    if (row == mRow && col == mCol && page == mPage) {
        return;
    }
    if (page == mPage) {
        Z2GetAudioMgr()->seStart(Z2SE_SY_CURSOR_ITEM, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
    } else {
        Z2GetAudioMgr()->seStart(Z2SE_SY_MENU_SUB_NEXT, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
    }
    mRow = row;
    mCol = col;
    mPage = page;
    refresh();
}

// RPX 0x02a14b94: the last stamp fades in on its cell, then it is given.
void dMenu_Stamp_c::moveUnlock() {
    if (mDoGph_gInf_c::getFader()->getStatus() != 1) {
        return;
    }
    mFrame++;
    J2DPane* last = mpStamp[STAMP_MAX - 1];
    if (mFrame < 30) {
        return;
    }
    if (mFrame == 30) {
        Z2GetAudioMgr()->seStart(Z2SE_SY_EXP_WIN_OPEN, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
        return;
    }
    if (mFrame < 61) {
        if (last != NULL) {
            if (mpEmpty[9] != NULL) {
                mpEmpty[9]->hide();
            }
            last->show();
            last->setAlpha((u8)(255.0f * ((f32)(mFrame - 30) / 30.0f)));
        }
        return;
    }
    if (mFrame < 91) {
        if (last != NULL) {
            last->setAlpha(0xff);
        }
        return;
    }
    dComIfGs_onStamp(STAMP_MAX - 1, 1);
    mState = 2;
}

// RPX 0x02a14d24: the reward message fades in, A / B close the album.
void dMenu_Stamp_c::moveMessage() {
    mFrame++;
    if (mFrame < 31) {
        if (mpMsgAll != NULL) {
            mpMsgAll->setAlpha((u8)(255.0f * ((f32)mFrame / 30.0f)));
        }
        return;
    }
    if (mpMsgAll != NULL) {
        mpMsgAll->setAlpha(0xff);
    }
    if (mDoCPd_c::getTrigA(PAD_1) != 0 || mDoCPd_c::getTrigB(PAD_1) != 0) {
        Z2GetAudioMgr()->seStart(Z2SE_SY_CURSOR_CANCEL, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
        mStatus = 3;
    }
}

// RPX 0x02a14ef0: black backdrop, background, album, cursor, A/B captions, message.
void dMenu_Stamp_c::_draw() {
    if (mpArchive == NULL) {
        return;
    }
    J2DGrafContext* ctx = dComIfGp_getCurrentGrafPort();
    if (mpBlackTex != NULL) {
        mpBlackTex->setAlpha(0xff);
        mpBlackTex->draw(mDoGph_gInf_c::getMinXF(), mDoGph_gInf_c::getMinYF(),
                         mDoGph_gInf_c::getWidthF(), mDoGph_gInf_c::getHeightF(), 0, 0, 0);
    }
#if TARGET_PC
    dusk::tphd::drawRevoScreen(mpBgScreen, ctx);
    dusk::tphd::drawRevoScreen(mpMenuScreen, ctx);

    // RPX 0x02a12438: cursor on the centre of the cell + (-1.75, -1.5), box = the cell.
    const int sel = mRow * 5 + mCol;
    if (mpDrawCursor != NULL && sel >= 0 && sel < 10 && mpCell[sel] != NULL &&
        mpCell[sel]->getPanePtr() != NULL)
    {
        const f32 sx = dusk::tphd::subscreenScaleX();
        const Vec c = dusk::tphd::mgrDrawnCenter(mpCell[sel]);
        mpDrawCursor->setPosRevo(c.x + kCursorOffsetX * sx, c.y + kCursorOffsetY,
                                 mpCell[sel]->getPanePtr(), false, sx, dusk::tphd::revoMinX());
        mpDrawCursor->draw();
    }

    dusk::tphd::drawRevoIconScreen(mpButtonScreen, ctx);
    if (mState >= 2 && mpMsgScreen != NULL) {
        dusk::tphd::drawRevoScreen(mpMsgScreen, ctx);
    }
#endif
}
