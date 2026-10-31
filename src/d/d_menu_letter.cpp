#include "d/dolzel.h" // IWYU pragma: keep

#include "d/d_menu_letter.h"
#include "JSystem/J2DGraph/J2DGrafContext.h"
#include "JSystem/J2DGraph/J2DTextBox.h"
#include "JSystem/JKernel/JKRMemArchive.h"
#include <cstdio>
#include "d/d_com_inf_game.h"
#include "d/d_select_cursor.h"
#include "d/d_meter2_info.h"
#include "d/d_meter_HIO.h"
#include "d/d_msg_string.h"
#include <types.h>
#include <cstring>
#include "m_Do/m_Do_controller_pad.h"
#include "m_Do/m_Do_graphic.h"
#include "d/d_msg_scrn_arrow.h"
#include "d/d_lib.h"

#if TARGET_PC
#include "dusk/achievements.h"
#include "dusk/interp/user_interface.h"
#include "dusk/menu_pointer.h"
#include "dusk/ui/touch_controls.hpp"
#include "dusk/version.hpp"
#include "dusk/settings.h"
#include "dusk/action_bindings.h"
#include "dusk/tphd/HdSubscreen.hpp"
#endif

static void enable_turn_page_controls(bool enabled) {
    const auto controlOverride =
        enabled ? dusk::ui::ControlOverride::Action : dusk::ui::ControlOverride::Default;
    dusk::ui::set_control_override(dusk::ui::Control::L, controlOverride);
    dusk::ui::set_control_override(dusk::ui::Control::R, controlOverride);
}


#if TARGET_PC
// ---------------------------------------------------------------------------------------------
// TPHD (Wii U) letter screen, ported from the RPX dMenu_Letter_c (ctor 0x029ee75c, screen set-up
// 0x029eefb0, letter display 0x029eeba8, input 0x029ef934, update 0x029ef5d0, draw 0x029ef844)
// and the generic menu widgets it is built from (0x029cb9a0 backdrop, 0x029cbb98 screen,
// 0x029cd088 scrolling list, 0x029cc308 A/B icons).
//
// letresR.arc: zelda_letter_base.blo (letter paper on the right + page indicator),
// zelda_letter_menuItem.blo (scrolling list of up to 19 letters, 7 visible, with a slider),
// zelda_letter_title.blo ("Letters") and the A/B icons. Unlike the GameCube screen there is
// no separate reading window: the letter under the cursor is always shown on the right, L/R turn
// its pages, B goes back. The list is in letter number order.
// ---------------------------------------------------------------------------------------------
namespace {
struct LetterHdItem {
    CPaneMgr* mgr;        // menun_0X (hidden when empty / scrolled out)
    J2DPane* pane;        // menun_0X
    J2DTextBox* subject;  // menutp0X
    J2DTextBox* sender;   // menuts0X
    J2DPane* frame;       // menurg0X (normal frame)
    J2DPane* frameSel;    // menusl0X (selected frame)
    J2DPane* iconClosed;  // menuic0X
    J2DPane* iconOpen;    // menuic1X
    int letter;           // letter number, -1 when empty
};

struct LetterHd {
    bool active;
    J2DScreen* base;
    J2DScreen* list;
    J2DScreen* title;
    J2DScreen* icon;
    J2DPicture* black;
    dSelect_cursor_c* cursor;
    dMsgString_c* string;
    J2DTextBox* text;        // letter text (lett_w_t, or lett_j_t + shadow lett_j_f in Japanese)
    J2DTextBox* textShadow;
    J2DTextBox* pageText;
    J2DPane* pageTurn;
    J2DPane* arrowL;
    J2DPane* arrowR;
    J2DPane* items;          // menu_itm (scrolled)
    f32 itemsX, itemsY;
    J2DPane* slider;         // menu_sld
    J2DPane* sliderFg;       // slide_fg
    f32 sliderFgX, sliderFgY;
    LetterHdItem item[19];
    int itemMax;
    int count;
    int top;
    int index;
    int page;
    int pageMax;
    int lines;
    int curText;             // text message of the letter shown
    int curLetter;
};
LetterHd s_letterHd;

const u64 kLetterItemTag[19] = {
    MULTI_CHAR('menun_0A'), MULTI_CHAR('menun_0B'), MULTI_CHAR('menun_0C'), MULTI_CHAR('menun_0D'),
    MULTI_CHAR('menun_0E'), MULTI_CHAR('menun_0F'), MULTI_CHAR('menun_0G'), MULTI_CHAR('menun_0H'),
    MULTI_CHAR('menun_0I'), MULTI_CHAR('menun_0J'), MULTI_CHAR('menun_0K'), MULTI_CHAR('menun_0L'),
    MULTI_CHAR('menun_0M'), MULTI_CHAR('menun_0N'), MULTI_CHAR('menun_0O'), MULTI_CHAR('menun_0P'),
    MULTI_CHAR('menun_0Q'), MULTI_CHAR('menun_0R'), MULTI_CHAR('menun_0S'),
};

u64 letterItemTag(const char* prefix, int i, char digit = '0') {
    char name[9];
    std::snprintf(name, sizeof(name), "%s%c%c", prefix, digit, 'A' + i);
    u64 tag = 0;
    for (int k = 0; name[k] != '\0' && k < 8; k++) {
        tag = (tag << 8) | (u8)name[k];
    }
    return tag;
}

void letterHdVisible(J2DPane* p, bool v) {
    if (v) {
        p->show();
    } else {
        p->hide();
    }
}

// RPX 0x029cca8c: closed / open envelope, and the subject in yellow while the letter is unread.
void letterHdSetOpened(LetterHdItem& it, bool opened) {
    if (it.iconClosed != NULL) {
        letterHdVisible(it.iconClosed, !opened);
    }
    if (it.iconOpen != NULL) {
        letterHdVisible(it.iconOpen, opened);
    }
    if (it.subject != NULL) {
        const JUtility::TColor c = opened ? JUtility::TColor(0xFF, 0xFF, 0xFF, 0xFF)
                                          : JUtility::TColor(0xFF, 0xFF, 0x0F, 0xFF);
        it.subject->setCharColor(c);
        it.subject->setGradColor(c);
    }
}
}  // namespace

void dMenu_Letter_c::hdCreate() {
    LetterHd& hd = s_letterHd;
    std::memset(&hd, 0, sizeof(hd));
    hd.active = true;
    hd.curText = -1;
    hd.curLetter = -1;

    ResTIMG* timg = (ResTIMG*)dComIfGp_getMain2DArchive()->getResource('TIMG', "tt_block8x8.bti");
    hd.black = JKR_NEW J2DPicture(timg);
    hd.black->setBlackWhite(JUtility::TColor(0, 0, 0, 0), JUtility::TColor(0, 0, 0, 0xff));

    hd.string = JKR_NEW dMsgString_c();

    // Base: letter paper and page indicator.
    hd.base = JKR_NEW J2DScreen();
    hd.base->setPriority("zelda_letter_base.blo", 0x20000, mpArchive);
    dPaneClass_showNullPane(hd.base);
    hd.pageText = (J2DTextBox*)hd.base->search(MULTI_CHAR('pageText'));
    if (hd.pageText != NULL) {
        hd.pageText->setFont(mDoExt_getMesgFont());
        hd.pageText->setString(0x80, "");
    }
    hd.pageTurn = hd.base->search(MULTI_CHAR('pageTurn'));
    hd.arrowL = hd.base->search(MULTI_CHAR('PICT_LA0'));
    hd.arrowR = hd.base->search(MULTI_CHAR('PICT_RA0'));
    J2DTextBox* jt = (J2DTextBox*)hd.base->search(MULTI_CHAR('lett_j_t'));
    J2DTextBox* jf = (J2DTextBox*)hd.base->search(MULTI_CHAR('lett_j_f'));
    J2DTextBox* wt = (J2DTextBox*)hd.base->search(MULTI_CHAR('lett_w_t'));
    if (dusk::version::isRegionJpn()) {
        // RPX: Japanese -> lett_j_t with its shadow lett_j_f, 9 lines a page.
        hd.text = jt;
        hd.textShadow = jf;
        hd.lines = 9;
        if (wt != NULL) {
            wt->hide();
        }
    } else {
        // Other languages -> lett_w_t, 12 lines a page.
        hd.text = wt;
        hd.textShadow = NULL;
        hd.lines = 12;
        for (J2DTextBox* t : {jt, jf}) {
            if (t != NULL) {
                t->setString(0x10, "");
                t->hide();
            }
        }
    }
    for (J2DTextBox* t : {hd.text, hd.textShadow}) {
        if (t != NULL) {
            t->show();
            t->setFont(mDoExt_getMesgFont());
            t->setString(0x800, "");
        }
    }

    // Scrolling list (RPX 0x029cd088 / 0x029ccb74).
    hd.list = JKR_NEW J2DScreen();
    hd.list->setPriority("zelda_letter_menuItem.blo", 0x20000, mpArchive);
    dPaneClass_showNullPane(hd.list);
    hd.items = hd.list->search(MULTI_CHAR('menu_itm'));
    if (hd.items != NULL) {
        hd.itemsX = hd.items->getTranslateX();
        hd.itemsY = hd.items->getTranslateY();
    }
    hd.slider = hd.list->search(MULTI_CHAR('menu_sld'));
    hd.sliderFg = hd.list->search(MULTI_CHAR('slide_fg'));
    if (hd.sliderFg != NULL) {
        hd.sliderFgX = hd.sliderFg->getTranslateX();
        hd.sliderFgY = hd.sliderFg->getTranslateY();
    }
    hd.itemMax = 0;
    for (int i = 0; i < 19; i++) {
        if (hd.list->search(kLetterItemTag[i]) == NULL) {
            break;
        }
        LetterHdItem& it = hd.item[i];
        it.mgr = JKR_NEW CPaneMgr(hd.list, kLetterItemTag[i], 0, NULL);
        it.pane = it.mgr->getPanePtr();
        it.subject = (J2DTextBox*)hd.list->search(letterItemTag("menutp", i));
        it.sender = (J2DTextBox*)hd.list->search(letterItemTag("menuts", i));
        it.frame = hd.list->search(letterItemTag("menurg", i));
        it.frameSel = hd.list->search(letterItemTag("menusl", i));
        it.iconClosed = hd.list->search(letterItemTag("menuic", i, '0'));
        it.iconOpen = hd.list->search(letterItemTag("menuic", i, '1'));
        for (J2DTextBox* t : {it.subject, it.sender}) {
            if (t != NULL) {
                t->setFont(mDoExt_getMesgFont());
                t->setString(0x100, "");
            }
        }
        it.letter = -1;
        letterHdSetOpened(it, false);
        it.mgr->hide();
        hd.itemMax++;
    }

    // RPX 0x029eefb0: every letter received, in letter number order.
    hd.count = 0;
    for (int i = 0; i < 64 && hd.count < hd.itemMax; i++) {
        if (!dComIfGs_isLetterGetFlag(i)) {
            continue;
        }
        LetterHdItem& it = hd.item[hd.count++];
        it.letter = i;
        if (it.subject != NULL) {
            hd.string->getString(dMenu_Letter::getLetterSubject(i), it.subject, NULL, NULL, NULL, 0);
        }
        if (it.sender != NULL) {
            hd.string->getString(dMenu_Letter::getLetterName(i), it.sender, NULL, NULL, NULL, 0);
        }
        letterHdSetOpened(it, dComIfGs_isLetterReadFlag(i) != 0);
        it.mgr->show();
    }

    // Title "Letters" (RPX: message 0x4c8 in f_t00).
    hd.title = JKR_NEW J2DScreen();
    hd.title->setPriority("zelda_letter_title.blo", 0x20000, mpArchive);
    dPaneClass_showNullPane(hd.title);
    if (J2DTextBox* t = (J2DTextBox*)hd.title->search(MULTI_CHAR('t_t00'))) {
        t->hide();
    }
    if (J2DTextBox* t = (J2DTextBox*)hd.title->search(MULTI_CHAR('f_t00'))) {
        t->setFont(mDoExt_getSubFont());
        t->setString(0x200, "");
        hd.string->getString(0x4c8, t, NULL, NULL, NULL, 0);
    }

    // A (no caption, greyed) / B "Back".
    hd.icon = JKR_NEW J2DScreen();
    hd.icon->setPriority("zelda_collect_soubi_do_icon_parts_revo.blo", 0x20000, mpArchive);
    dPaneClass_showNullPane(hd.icon);
    dusk::tphd::setupRevoButtonText(hd.icon);
    dusk::tphd::setRevoButtonText(hd.icon, 0, true);
    dusk::tphd::setRevoButtonText(hd.icon, 0x3f9, false);

    // RPX 0x029cbffc / 0x029cc290: list cursor (0.95, 1, 0.03, 0.7, 0.7).
    hd.cursor = JKR_NEW dSelect_cursor_c(2, 1.0f, NULL);
    hd.cursor->setParam(0.95f, 1.0f, 0.03f, 0.7f, 0.7f);
    hd.cursor->setAlphaRate(1.0f);
    hd.cursor->setScale(1.0f);
    hd.cursor->onPlayAnime(0);

    enable_turn_page_controls(true);
    hd.index = 0;
    hd.page = 0;
    if (hd.count > 0) {
        hdShow(0, 0);
    } else if (hd.pageTurn != NULL) {
        hd.pageTurn->hide();
    }
}

void dMenu_Letter_c::hdDelete() {
    LetterHd& hd = s_letterHd;
    for (int i = 0; i < 19; i++) {
        if (hd.item[i].mgr != NULL) {
            JKR_DELETE(hd.item[i].mgr);
            hd.item[i].mgr = NULL;
        }
    }
    if (hd.cursor != NULL) {
        JKR_DELETE(hd.cursor);
    }
    if (hd.string != NULL) {
        JKR_DELETE(hd.string);
    }
    if (hd.black != NULL) {
        JKR_DELETE(hd.black);
    }
    for (J2DScreen* scr : {hd.base, hd.list, hd.title, hd.icon}) {
        if (scr != NULL) {
            JKR_DELETE(scr);
        }
    }
    std::memset(&hd, 0, sizeof(hd));
}

// RPX 0x029eeba8 (+ list select 0x029cd5a4): selects a list entry, scrolls the list, moves the
// cursor and shows page i_page of that letter.
void dMenu_Letter_c::hdShow(int i_index, int i_page) {
    LetterHd& hd = s_letterHd;
    if (hd.count <= 0) {
        return;
    }
    hd.index = i_index;
    // Scroll: 7 entries visible, 49 units apart.
    if (hd.top + 6 < i_index) {
        hd.top = i_index - 6;
    } else if (i_index < hd.top) {
        hd.top = i_index;
    }
    if (hd.items != NULL) {
        hd.items->translate(hd.itemsX, hd.itemsY - hd.top * 49.0f);
    }
    if (hd.slider != NULL) {
        if (hd.count - 7 > 0) {
            hd.slider->show();
            if (hd.sliderFg != NULL) {
                hd.sliderFg->translate(hd.sliderFgX,
                                       hd.sliderFgY + (hd.top * 134.0f) / (f32)(hd.count - 7));
            }
        } else {
            hd.slider->hide();
        }
    }
    for (int i = 0; i < hd.count; i++) {
        LetterHdItem& it = hd.item[i];
        const bool sel = i == i_index;
        if (it.frame != NULL) {
            letterHdVisible(it.frame, !sel);
        }
        if (it.frameSel != NULL) {
            letterHdVisible(it.frameSel, sel);
            it.frameSel->scale(sel ? 1.15f : 1.0f, sel ? 1.2f : 1.0f);
        }
        if (it.pane != NULL) {
            letterHdVisible(it.pane, hd.top <= i && i < hd.top + 7);
        }
    }

    const int letter = hd.item[i_index].letter;
    hd.curLetter = letter;
    hd.curText = dMenu_Letter::getLetterText(letter);
    // Every letter with this text is read now.
    for (int i = 0; i < 64; i++) {
        if (dMenu_Letter::getLetterText(i) == hd.curText) {
            dComIfGs_onLetterReadFlag(i);
        }
    }
    hdUpdateIcons();

    hd.page = i_page;
    if (hd.text != NULL) {
        hd.string->getStringPage(hd.curText, (u8)i_page, (u8)hd.lines, hd.text, hd.textShadow, NULL,
                                 NULL, 0);
        hd.pageMax = hd.string->getPageMax(hd.lines);
    } else {
        hd.pageMax = 1;
    }
    if (hd.pageText != NULL) {
        // Message 0xaf2 is "Page %d/%d".
        char fmt[0x80] = "";
        dMeter2Info_getString(0xaf2, fmt, NULL);
        if (fmt[0] == '\0' || std::strstr(fmt, "%d") == NULL) {
            SAFE_STRCPY(fmt, "%d/%d");
        }
        char buf[0x80];
        std::snprintf(buf, sizeof(buf), fmt, i_page + 1, hd.pageMax);
        SAFE_STRCPY(hd.pageText->getStringPtr(), buf);
    }
    if (hd.pageTurn != NULL) {
        letterHdVisible(hd.pageTurn, hd.pageMax > 1);
    }
}

// RPX 0x029eea04: envelope of each entry (the letter being read keeps its look).
void dMenu_Letter_c::hdUpdateIcons() {
    LetterHd& hd = s_letterHd;
    for (int i = 0; i < hd.count; i++) {
        LetterHdItem& it = hd.item[i];
        if (it.letter < 0) {
            continue;
        }
        const int text = dMenu_Letter::getLetterText(it.letter);
        if (!dComIfGs_isLetterReadFlag(it.letter)) {
            letterHdSetOpened(it, false);
        } else if (text != hd.curText) {
            letterHdSetOpened(it, true);
        }
    }
}

// Page turn (RPX 0x02a1b0a8 / 0x02a1b158): the Pro Controller L / R shoulder buttons, not the
// ZL / ZR triggers (the GC L / R). Here those buttons are the ones bound to "Call Midna" (L) and
// "Use Item (R slot)" (R); without a binding the GC triggers still work.
static bool hdPageTrig(bool i_right) {
    const dusk::ActionBinds b =
        i_right ? dusk::ActionBinds::USE_ITEM_R : dusk::ActionBinds::CALL_MIDNA;
    if (dusk::isActionBound(b, PAD_1)) {
        return dusk::getActionBindTrig(b, PAD_1);
    }
    return i_right ? mDoCPd_c::getTrigR(PAD_1) != 0 : mDoCPd_c::getTrigL(PAD_1) != 0;
}

// RPX 0x029ef934 (input) + 0x029ef5d0 (page arrows).
void dMenu_Letter_c::hdMove() {
    LetterHd& hd = s_letterHd;
    if (mDoGph_gInf_c::getFader()->getStatus() != 1) {
        return;
    }
    if (mDoCPd_c::getTrigB(PAD_1)) {
        mStatus = 3;
        return;
    }
    if (hd.count > 0) {
        const int old = hd.index;
        int next = old;
        dusk::menu_pointer::begin_context(dusk::menu_pointer::Context::Collection);
        for (int i = hd.top; i < hd.count && i < hd.top + 7; i++) {
            if (hd.item[i].pane != NULL && dusk::menu_pointer::hit_pane(hd.item[i].pane, 4.0f)) {
                dusk::menu_pointer::set_hover_target(i);
                next = i;
                break;
            }
        }
        mpStick->checkTrigger();
        if (next == old) {
            if (mpStick->checkUpTrigger()) {
                if (old > 0) {
                    next = old - 1;
                }
            } else if (mpStick->checkDownTrigger()) {
                if (old + 1 < hd.count) {
                    next = old + 1;
                }
            }
        }
        if (next != old) {
            hdShow(next, 0);
            Z2GetAudioMgr()->seStart(Z2SE_SY_CURSOR_ITEM, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
        } else if (hdPageTrig(false)) {
            if (hd.page > 0) {
                hdShow(hd.index, hd.page - 1);
                Z2GetAudioMgr()->seStart(Z2SE_SY_MENU_SUB_NEXT, NULL, 0, 0, 1.0f, 1.0f, -1.0f,
                                         -1.0f, 0);
            }
        } else if (hdPageTrig(true)) {
            if (hd.page + 1 < hd.pageMax) {
                hdShow(hd.index, hd.page + 1);
                Z2GetAudioMgr()->seStart(Z2SE_SY_MENU_SUB_NEXT, NULL, 0, 0, 1.0f, 1.0f, -1.0f,
                                         -1.0f, 0);
            }
        }
    }
    if (hd.arrowL != NULL) {
        hd.arrowL->setAlpha(hd.page > 0 ? 0xff : 0x60);
    }
    if (hd.arrowR != NULL) {
        hd.arrowR->setAlpha(hd.page + 1 < hd.pageMax ? 0xff : 0x60);
    }
}

// RPX 0x029ef844: backdrop, letter base, list (+ cursor), title, A/B icons, then the letter text.
void dMenu_Letter_c::hdDraw() {
    LetterHd& hd = s_letterHd;
    J2DGrafContext* graf = dComIfGp_getCurrentGrafPort();
    hd.black->setAlpha(0xff);
    hd.black->draw(mDoGph_gInf_c::getMinXF(), mDoGph_gInf_c::getMinYF(),
                   mDoGph_gInf_c::getWidthF(), mDoGph_gInf_c::getHeightF(), false, false, false);
    if (hd.text != NULL && hd.curText >= 0) {
        hd.string->getStringPage(hd.curText, (u8)hd.page, (u8)hd.lines, hd.text, hd.textShadow,
                                 NULL, NULL, 0);
    }
    dusk::tphd::drawRevoScreen(hd.base, graf);
    dusk::tphd::drawRevoScreen(hd.list, graf);
    if (hd.count > 0 && hd.item[hd.index].mgr != NULL) {
        // Centre of the item as it was just drawn (its global matrix includes the stretch and
        // the 'n_43' squeeze of n_all); CPaneMgr's own bounds walk drifts on this nested
        // centre-anchored chain (n_all > menu_itm > N_menu0X > menun_0X).
        const Vec c = hd.item[hd.index].pane != NULL && hd.item[hd.index].pane->isVisible()
                          ? dusk::tphd::paneGlobalCenter(hd.item[hd.index].pane)
                          : dusk::tphd::mgrDrawnCenter(hd.item[hd.index].mgr);
        hd.cursor->setPosRevo(c.x, c.y, hd.item[hd.index].pane, false,
                              dusk::tphd::subscreenScaleX(), mDoGph_gInf_c::getMinXF());
        hd.cursor->draw();
    }
    dusk::tphd::drawRevoScreen(hd.title, graf);
    dusk::tphd::drawRevoIconScreen(hd.icon, graf);
    if (hd.text != NULL && hd.curText >= 0) {
        hd.string->drawOutFont(hd.text, -1.0f);
    }
}
#endif

#if TARGET_PC
#define D_MENU_LETTER_LINE_MAX (dusk::version::isRegionJpn() ? 9 : 12)
#elif VERSION == VERSION_GCN_JPN
#define D_MENU_LETTER_LINE_MAX 9
#else
#define D_MENU_LETTER_LINE_MAX 12
#endif

typedef void (dMenu_Letter_c::*initFunc)();
static initFunc map_init_process[] = {
    &dMenu_Letter_c::wait_init,
    &dMenu_Letter_c::slide_right_init,
    &dMenu_Letter_c::slide_left_init,
    &dMenu_Letter_c::read_open_init,
    &dMenu_Letter_c::read_move_init,
    &dMenu_Letter_c::read_next_fadeout_init,
    &dMenu_Letter_c::read_next_fadein_init,
    &dMenu_Letter_c::read_close_init,
};

typedef void (dMenu_Letter_c::*moveFunc)();
static moveFunc map_move_process[] = {
    &dMenu_Letter_c::wait_move,
    &dMenu_Letter_c::slide_right_move,
    &dMenu_Letter_c::slide_left_move,
    &dMenu_Letter_c::read_open_move,
    &dMenu_Letter_c::read_move_move,
    &dMenu_Letter_c::read_next_fadeout_move,
    &dMenu_Letter_c::read_next_fadein_move,
    &dMenu_Letter_c::read_close_move,
};

dMenu_Letter_c::dMenu_Letter_c(JKRExpHeap* i_heap, STControl* i_stick, CSTControl* i_cstick) {
    mpHeap = i_heap;
    mpMount = NULL;
    mpArchive = NULL;
    mpStick = i_stick;
    mpCStick = i_cstick;
    mStatus = 1;
    mProcess = 0;
    field_0x368 = 0;
    field_0x36a = 0;
    mIndex = 0;
    field_0x36f = 0;
    field_0x372 = 0;
    field_0x3e2 = 0;
    field_0x3e3 = 0;
    field_0x3e4 = 0;
    field_0x370 = 0xff;
    field_0x371 = 0xff;
    letter_init_calc();
    field_0x358 = 0.0f;
    field_0x35c = 0.0f;
    field_0x360 = field_0x364 = 1.0f;
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 6; j++) {
            field_0x376[i][j] = 0;
        }
    }
}


dMenu_Letter_c::~dMenu_Letter_c() {
#if TARGET_PC
    enable_turn_page_controls(false);
    if (s_letterHd.active) {
        hdDelete();
        if (mpMount != NULL) {
            JKRUnmountArchive(mpMount->getArchive());
            mpMount->destroy();
            mpMount = NULL;
        }
        if (mpArchive != NULL) {
            JKRUnmountArchive(mpArchive);
            mpArchive = NULL;
        }
        dComIfGp_getMsgArchive(0)->removeResourceAll();
        return;
    }
#endif

    JKR_DELETE(mpDrawCursor);
    mpDrawCursor = NULL;

    JKR_DELETE(mpArrow);
    mpArrow = NULL;

    JKR_DELETE(mpString);
    mpString = NULL;

    JKR_DELETE(mpMenuScreen);
    mpMenuScreen = NULL;

    JKR_DELETE(mpMenuDMYScreen);
    mpMenuDMYScreen = NULL;

    JKR_DELETE(mpBaseScreen);
    mpBaseScreen = NULL;

    JKR_DELETE(mpSdwScreen);
    mpSdwScreen = NULL;

    for (int i = 0; i < 2; i++) {
        JKR_DELETE(mpLetterScreen[i]);
        mpLetterScreen[i] = NULL;

        JKR_DELETE(field_0x2ec[i]);
        field_0x2ec[i] = NULL;

        if (field_0x2f4[i] != NULL) {
            JKR_DELETE(field_0x2f4[i]);
            field_0x2f4[i] = NULL;
        }

        JKR_DELETE(mpTextParent[i]);
        mpTextParent[i] = NULL;
    }

    JKR_DELETE(mpLineParent);
    mpLineParent = NULL;

    JKR_DELETE(mpBlackTex);
    mpBlackTex = NULL;

    for (int i = 0; i < 3; i++) {
        JKR_DELETE(mpParent[i]);
        mpParent[i] = NULL;
    }

    for (int i = 0; i < 6; i++) {
        JKR_DELETE(mpLetterParent[i]);
        mpLetterParent[i] = NULL;
    }

    JKR_DELETE(mpDMYParent);
    mpDMYParent = NULL;

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

    if (mpMount != NULL) {
        JKRUnmountArchive(mpMount->getArchive());
        mpMount->destroy();
        mpMount = NULL;
    }

    if (mpArchive != NULL) {
        JKRUnmountArchive(mpArchive);
        mpArchive = NULL;
    }
    dComIfGp_getMsgArchive(0)->removeResourceAll();
}

void dMenu_Letter_c::_create() {
    mpDrawCursor = JKR_NEW dSelect_cursor_c(2, 1.0f, NULL);
    JUT_ASSERT(231, mpDrawCursor != NULL);
    // shield prod uses same values as GCN?
    #if PLATFORM_WII || VERSION == VERSION_SHIELD_DEBUG
    mpDrawCursor->setParam(1.01f, 0.85f, 0.02f, 0.5f, 0.5f);
    #else
    mpDrawCursor->setParam(1.06f, 0.9f, 0.02f, 0.4f, 0.4f);
    #endif
    mpDrawCursor->setAlphaRate(0.0f);
    mpDrawCursor->setScale(0.0f);
    mpDrawCursor->offPlayAnime(0);

    mpString = JKR_NEW dMsgString_c();
    JUT_ASSERT(245, mpString != NULL);
    
    mpArrow = JKR_NEW dMsgScrnArrow_c();
    JUT_ASSERT(249, mpArrow != NULL);

    screenSetMenu();
    screenSetBase();
    screenSetShadow();
    screenSetLetter();
    screenSetDoIcon();
    setHIO(true);
    init();
}


void dMenu_Letter_c::_move() {
#if TARGET_PC
    if (s_letterHd.active) {
        JKRHeap* hdHeap = mDoExt_setCurrentHeap((JKRHeap*)mpHeap);
        hdMove();
        mDoExt_setCurrentHeap(hdHeap);
        return;
    }
#endif
    JKRHeap* heap = mDoExt_setCurrentHeap((JKRHeap*)mpHeap);
    u8 process = mProcess;
    (this->*map_move_process[mProcess])();
    if (process != mProcess) {
        (this->*map_init_process[mProcess])();
    }
    setHIO(false);
    mDoExt_setCurrentHeap(heap);
}

void dMenu_Letter_c::_draw() {
    if (mpArchive == NULL) {
        return;
    }
    IF_DUSK(presentAnims());
#if TARGET_PC
    if (s_letterHd.active) {
        hdDraw();
        return;
    }
#endif
    J2DGrafContext* grafContext = dComIfGp_getCurrentGrafPort();
    u8 prevAlpha = mpBlackTex->getAlpha();
    mpBlackTex->setAlpha(0xff);
    mpBlackTex->draw(mDoGph_gInf_c::getMinXF(), mDoGph_gInf_c::getMinYF(),
                     mDoGph_gInf_c::getWidthF(), mDoGph_gInf_c::getHeightF(), false, false, false);
    mpBlackTex->setAlpha(prevAlpha);
    mpBaseScreen->draw(0.0f, 0.0f, grafContext);
    u32 left, top, width, height;
    GXGetScissor(&left, &top, &width, &height);
    CPaneMgr afStack_138;
    Mtx mtx;
    Vec local_178;
    local_178 = afStack_138.getGlobalVtx(field_0x1ec, &mtx, 0, false, 0);
    f32 x1 = local_178.x;
    f32 y1 = local_178.y;
    Vec local_184;
    local_184 = afStack_138.getGlobalVtx(field_0x1ec, &mtx, 3, false, 0);
    f32 dVar17 = mDoGph_gInf_c::getWidthF() / FB_WIDTH;
    f32 dVar16 = mDoGph_gInf_c::getHeightF() / FB_HEIGHT;
    f32 fVar1 = (x1 - mDoGph_gInf_c::getMinXF()) / dVar17;
    f32 fVar2 = y1 / dVar16;
    grafContext->scissor(fVar1, fVar2,
                         (local_184.x - x1) / dVar17,
                         (local_184.y - y1) / dVar16);
    grafContext->setScissor();
    mpMenuScreen->draw(field_0x358, 0.0f, grafContext);
    if (mProcess == 2 || mProcess == 1) {
        mpMenuDMYScreen->draw(field_0x358 + field_0x35c, 0.0f, grafContext);
    }
    grafContext->scissor(
        left,
        top,
        width,
        height);
    grafContext->setScissor();
    if (mProcess != 2 && mProcess != 1) {
        mpDrawCursor->draw();
    }
    if (mProcess == 3 || mProcess == 4 || mProcess == 5 || mProcess == 6 || mProcess == 7) {
        mpBlackTex->draw(
            mDoGph_gInf_c::getMinXF(), mDoGph_gInf_c::getMinYF(), mDoGph_gInf_c::getWidthF(),
            mDoGph_gInf_c::getHeightF(), 0, false, 0);
        u8 dVar1 = field_0x3ac[field_0x36f * 6 + mIndex] - 1;
        for (int i = 0; i < 2; i++) {
            mpLetterScreen[i]->draw(0.0f, 0.0f, grafContext);
        }
        for (int i = 0; i < 2; i++) {
            J2DTextBox* uVar10 = NULL;
            if (field_0x2f4[i] != NULL) {
                uVar10 = (J2DTextBox*)field_0x2f4[i]->getPanePtr();
            }
            J2DTextBox* uVar18 = (J2DTextBox*)field_0x2ec[i]->getPanePtr();
            mpString->getStringPage(dMenu_Letter::getLetterText(dVar1), field_0x3e3 - 1,
                                    D_MENU_LETTER_LINE_MAX, uVar18, uVar10, NULL, NULL, 0);
        }
        mpString->drawOutFont((J2DTextBox*)field_0x2ec[1]->getPanePtr(), -1.0f);
        if (mProcess == 4) {
            mpArrow->draw();
        }
    }
    mpIconScreen->draw(0.0f, 0.0f, grafContext);
}

bool dMenu_Letter_c::isSync() {
    if (mpMount != NULL && mpMount->sync() == false) {
        return 0;
    }
    return 1;
}

void dMenu_Letter_c::letter_init_calc() {
    m_letter_num = getLetterNum();
    for (int i = 0; i < 54; i++) {
        if (i < m_letter_num) {
            field_0x3ac[i] = dComIfGs_getGetNumber(m_letter_num - i - 1);
        } else {
            field_0x3ac[i] = 0xff;
        }
    }
    if (m_letter_num < 6) {
        field_0x373 = m_letter_num;
    } else {
        field_0x373 = 6;
    }
    if (m_letter_num % 6 == 0) {
        field_0x374 = m_letter_num / 6;
    } else {
        field_0x374 = m_letter_num / 6 + 1;
    }
}

void dMenu_Letter_c::init() {
    setPageText();
    changeActiveColor();
    (this->*map_init_process[mProcess])();
}

int dMenu_Letter_c::_open() {
#if TARGET_PC
    if (dusk::tphd_active()) {
        // RPX 0x029ef4f4: mounts letresR.arc, builds the screen and is open at once.
        if (mpMount == NULL && mpArchive == NULL) {
            mpMount = mDoDvdThd_mountArchive_c::create("/res/LayoutRevo/letresR.arc", 0, NULL);
        }
        if (mpArchive == NULL) {
            if (mpMount->sync() == 0) {
                return 0;
            }
            mpArchive = (JKRArchive*)mpMount->getArchive();
            JKR_DELETE(mpMount);
            mpMount = NULL;
            JKRHeap* hdHeap = mDoExt_setCurrentHeap((JKRHeap*)mpHeap);
            hdCreate();
            mDoExt_setCurrentHeap(hdHeap);
        }
        mStatus = 2;
        return 1;
    }
#endif
    if (mpMount == NULL) {
        mpMount = mDoDvdThd_mountArchive_c::create("/res/Layout/letres.arc", 0, NULL);
    }
    if (!mpArchive) {
        if (mpMount->sync() != 0) {
            if (!mpArchive) {
                mpArchive = (JKRArchive*)mpMount->getArchive();
                JKR_DELETE(mpMount);
                mpMount = NULL;
                _create();
            }
        } else {
            return 0;
        }
    }

    s16 openWindowFrame =
        g_drawHIO.mLetterSelectScreen.mOpenFrame[dMeter_drawLetterHIO_c::WINDOW_FRAME];
    s16 closeWindowFrame =
        g_drawHIO.mLetterSelectScreen.mCloseFrame[dMeter_drawLetterHIO_c::WINDOW_FRAME];
    field_0x368 = g_drawHIO.mLetterSelectScreen.mOpenFrame[dMeter_drawLetterHIO_c::WINDOW_FRAME];
    if (field_0x368 >= openWindowFrame) {
        field_0x368 = closeWindowFrame;
        mStatus = 2;
        for (int i = 0; i < 3; i++) {
            mpParent[i]->scale(g_drawHIO.mLetterSelectScreen.mWindowScale,
                               g_drawHIO.mLetterSelectScreen.mWindowScale);
            mpParent[i]->setAlphaRate(1.0f);
        }
        setCursorPos();
        mpDrawCursor->setAlphaRate(1.0f);
        mpDrawCursor->setScale(1.0f);
        mpDrawCursor->onPlayAnime(0);
        return 1;
    } else {
        f32 div = (f32)field_0x368 / (f32)openWindowFrame;
        for (int i = 0; i < 3; i++) {
            mpParent[i]->scale(g_drawHIO.mLetterSelectScreen.mWindowScale * div,
                               g_drawHIO.mLetterSelectScreen.mWindowScale * div);
            mpParent[i]->setAlphaRate(div);
        }
        setCursorPos();
        mpDrawCursor->setAlphaRate(div);
        mpDrawCursor->setScale(div);
        return 0;
    }
}

int dMenu_Letter_c::_close() {
#if TARGET_PC
    enable_turn_page_controls(false);
    if (s_letterHd.active) {
        mStatus = 0;
        return 1;
    }
#endif

    s16 closeWindowFrame =
        g_drawHIO.mLetterSelectScreen.mCloseFrame[dMeter_drawLetterHIO_c::WINDOW_FRAME];
    field_0x368 = 0;
    if (field_0x368 <= 0) {
        field_0x368 = 0;
        mStatus = 0;
        for (int i = 0; i < 3; i++) {
            mpParent[i]->scale(0.0f, 0.0f);
            mpParent[i]->setAlphaRate(0.0f);
        }
        setCursorPos();
        mpDrawCursor->setAlphaRate(0.0f);
        mpDrawCursor->setScale(0.0f);
        return 1;
    } else {
        f32 div = (f32)field_0x368 / (f32)closeWindowFrame;
        for (int i = 0; i < 3; i++) {
            mpParent[i]->scale(g_drawHIO.mLetterSelectScreen.mWindowScale * div,
                               g_drawHIO.mLetterSelectScreen.mWindowScale * div);
            mpParent[i]->setAlphaRate(div);
        }
        setCursorPos();
        mpDrawCursor->setAlphaRate(div);
        mpDrawCursor->setScale(div);
        return 0;
    }
}

void dMenu_Letter_c::wait_init() {
#if TARGET_PC
    enable_turn_page_controls(field_0x374 > 1);
#endif

    setAButtonString(0x40c);
    setBButtonString(0x3f9);
}

void dMenu_Letter_c::wait_move() {
    u8 oldIndex = mIndex;
    if (mDoGph_gInf_c::getFader()->getStatus() == 1) {
#if TARGET_PC
        if (pointerWait()) {
            return;
        }
#endif

        if (mDoCPd_c::getTrigB(PAD_1) != 0) {
            mpDrawCursor->offPlayAnime(0);
            mStatus = 3;
        } else if (mDoCPd_c::getTrigA(PAD_1)) {
            mProcess = 3;
            Z2GetAudioMgr()->seStart(Z2SE_SY_LETTER_OPEN, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
            dMeter2Info_set2DVibration();
        } else if (mpStick->checkUpTrigger()) {
            if (mIndex) {
                mIndex--;
                Z2GetAudioMgr()->seStart(Z2SE_SY_CURSOR_ITEM, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f,
                                         0);
            }

        } else if (mpStick->checkDownTrigger()) {
            if (mIndex < field_0x373 - 1) {
                mIndex++;
                Z2GetAudioMgr()->seStart(Z2SE_SY_CURSOR_ITEM, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f,
                                         0);
            }
        } else if (mDoCPd_c::getTrigR(PAD_1)) {
            if (field_0x36f < field_0x374 - 1) {
                field_0x372 = field_0x36f;
                field_0x36f++;
                mProcess = 2;
                Z2GetAudioMgr()->seStart(Z2SE_SY_MENU_SUB_NEXT, NULL, 0, 0, 1.0f, 1.0f, -1.0f,
                                         -1.0f, 0);
            }
        } else if (mDoCPd_c::getTrigL(PAD_1)) {
            if (field_0x36f) {
                field_0x372 = field_0x36f;
                field_0x36f--;
                mProcess = 1;
                Z2GetAudioMgr()->seStart(Z2SE_SY_MENU_SUB_NEXT, NULL, 0, 0, 1.0f, 1.0f, -1.0f,
                                         -1.0f, 0);
            }
        }

        if (mProcess == 1 || mProcess == 2) {
#if TARGET_PC
            J2DTextBox* textBox;
            if (dusk::version::isRegionJpn()) {
                textBox = (J2DTextBox*)mpBaseScreen->search(MULTI_CHAR('t_t00'));
                mpBaseScreen->search(MULTI_CHAR('f_t_00'))->hide();
            } else {
                textBox = (J2DTextBox*)mpBaseScreen->search(MULTI_CHAR('f_t_00'));
                mpBaseScreen->search(MULTI_CHAR('t_t00'))->hide();
            }
#elif VERSION == VERSION_GCN_JPN
            J2DTextBox* textBox = (J2DTextBox*)mpBaseScreen->search(MULTI_CHAR('t_t00'));
            mpBaseScreen->search(MULTI_CHAR('f_t_00'))->hide();
#else
            J2DTextBox* textBox = (J2DTextBox*)mpBaseScreen->search(MULTI_CHAR('f_t_00'));
            mpBaseScreen->search(MULTI_CHAR('t_t00'))->hide();
#endif
            dComIfGp_setMessageCountNumber(field_0x374 + (field_0x36f + 1) * 100);
            mpString->getString(0x4d6, textBox, NULL, NULL, NULL, 0);
        }
        if (oldIndex != mIndex) {
            changeActiveColor();
        }
    }
}

#if TARGET_PC
bool dMenu_Letter_c::pointerWait() {
    dusk::menu_pointer::begin_context(dusk::menu_pointer::Context::Collection);
    for (u8 i = 0; i < field_0x373; ++i) {
        if (!dusk::menu_pointer::hit_pane(mpLetterParent[i], 8.0f)) {
            continue;
        }
        dusk::menu_pointer::set_hover_target(i);

        if (mIndex != i) {
            mIndex = i;
            changeActiveColor();
            Z2GetAudioMgr()->seStart(Z2SE_SY_CURSOR_ITEM, NULL, 0, 0, 1.0f, 1.0f, -1.0f,
                                     -1.0f, 0);
        }
        if (dusk::menu_pointer::consume_click()) {
            mProcess = 3;
            Z2GetAudioMgr()->seStart(Z2SE_SY_LETTER_OPEN, NULL, 0, 0, 1.0f, 1.0f, -1.0f,
                                     -1.0f, 0);
            dMeter2Info_set2DVibration();
            return true;
        }
        return false;
    }

    return false;
}
#endif


void dMenu_Letter_c::slide_right_init() {
#if TARGET_PC
    enable_turn_page_controls(false);
#endif

    field_0x358 = -field_0x1ec->getWidth() * mDoGph_gInf_c::getInvScale();
    field_0x35c = field_0x1ec->getWidth() IF_NOT_DUSK(* mDoGph_gInf_c::getInvScale());
    changePageLight();
    copyDMYMenu();
    setAButtonString(0);
    setBButtonString(0);
}

void dMenu_Letter_c::slide_right_move() {
    IF_NOT_DUSK(cLib_addCalc2(&field_0x358, 0.0f, 0.5f, 50.0f));
    if (fabsf(field_0x358) < 0.1f) {
        field_0x358 = 0.0f;
        mProcess = 0;
    }
}

void dMenu_Letter_c::slide_left_init() {
#if TARGET_PC
    enable_turn_page_controls(false);
#endif

    field_0x358 = field_0x1ec->getWidth() * mDoGph_gInf_c::getInvScale();
    field_0x35c = -field_0x1ec->getWidth() IF_NOT_DUSK(* mDoGph_gInf_c::getInvScale());
    changePageLight();
    copyDMYMenu();
    setAButtonString(0);
    setBButtonString(0);
}

void dMenu_Letter_c::slide_left_move() {
    IF_NOT_DUSK(cLib_addCalc2(&field_0x358, 0.0f, 0.5f, 50.0f));
    if (fabsf(field_0x358) < 0.1f) {
        field_0x358 = 0.0f;
        mProcess = 0;
    }
}

void dMenu_Letter_c::read_open_init() {
#if TARGET_PC
    enable_turn_page_controls(false);
#endif

    field_0x36a = 0;
    u8 idx = field_0x3ac[field_0x36f * 6 + mIndex] - 1;
    field_0x3e3 = 1;
    for (int i = 0; i < 2; i++) {
        mpTextParent[i]->scale(0.0f, 0.0f);
        mpTextParent[i]->setAlphaRate(0.0f);
        J2DTextBox* text1 = NULL;
        if (field_0x2f4[i] != 0) {
            text1 = (J2DTextBox*)field_0x2f4[i]->getPanePtr();
        }
        J2DTextBox* text2 = (J2DTextBox*)field_0x2ec[i]->getPanePtr();
        mpString->getStringPage(dMenu_Letter::getLetterText(idx), field_0x3e3 - 1,
                                D_MENU_LETTER_LINE_MAX, text2, text1, NULL, NULL, 0);
    }
    field_0x3e2 = mpString->getPageMax(D_MENU_LETTER_LINE_MAX);
    if (field_0x3e2 > 1) {
        char acStack_30[20];
        SAFE_SPRINTF(acStack_30, "%d/%d", field_0x3e3, field_0x3e2);
        for (int i = 0; i < 2; i++) {
            field_0x1e4[i]->show();
            SAFE_STRCPY(field_0x1e4[i]->getStringPtr(), acStack_30);
        }
    } else {
        for (int i = 0; i < 2; i++) {
            field_0x1e4[i]->hide();
        }
    }
    if (!dComIfGs_isLetterReadFlag(idx)) {
        dComIfGs_onLetterReadFlag(idx);
        field_0x34[mIndex][4]->hide();
    }
    setAButtonString(0);
    setBButtonString(0);
    mpBlackTex->setAlpha(0);

    #ifdef TARGET_PC
        dusk::AchievementSystem::get().signal("open_letter");
    #endif
}

void dMenu_Letter_c::read_open_move() {
    s16 openLetterFrame =
        g_drawHIO.mLetterSelectScreen.mOpenFrame[dMeter_drawLetterHIO_c::LETTER_FRAME];
    IF_NOT_DUSK(field_0x36a++);
    if (field_0x36a >= openLetterFrame) {
        mProcess = 4;
        for (int i = 0; i < 2; i++) {
            mpTextParent[i]->scale(g_drawHIO.mLetterSelectScreen.mLetterWindowScale,
                                   g_drawHIO.mLetterSelectScreen.mLetterWindowScale);
            mpTextParent[i]->setAlphaRate(1.0f);
        }
        mpBlackTex->setAlpha(g_drawHIO.mLetterSelectScreen.mWindowBGAlpha);
    } else {
        f32 div = (f32)field_0x36a / (f32)openLetterFrame;
        for (int i = 0; i < 2; i++) {
            mpTextParent[i]->scale(g_drawHIO.mLetterSelectScreen.mLetterWindowScale * div,
                                   g_drawHIO.mLetterSelectScreen.mLetterWindowScale * div);
            mpTextParent[i]->setAlphaRate(div);
        }
        mpBlackTex->setAlpha(g_drawHIO.mLetterSelectScreen.mWindowBGAlpha * div);
    }
}

void dMenu_Letter_c::read_move_init() {
    mpArrow->arwAnimeInit();
    mpArrow->dotAnimeInit();
    if (field_0x3e2 > 1) {
        if (field_0x3e3 == field_0x3e2) {
            setAButtonString(0);
            setBButtonString(0x3f9);
        } else {
            setAButtonString(0x408);
            setBButtonString(0x3f9);
        }
        CPaneMgr paneMgr;
        J2DPane* pane = mpLetterScreen[0]->search(MULTI_CHAR('set_ya_n'));
        Vec pos = paneMgr.getGlobalVtxCenter(pane, false, 0);
        mpArrow->setPos(pos.x, pos.y);
    } else {
        setAButtonString(0);
        setBButtonString(0x3f9);
    }
}

void dMenu_Letter_c::read_move_move() {
    if (mDoCPd_c::getTrigA(PAD_1) != 0) {
        if (field_0x3e3 == field_0x3e2) {
            Z2GetAudioMgr()->seStart(Z2SE_SY_LETTER_CLOSE, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
            dMeter2Info_set2DVibration();
            mProcess = 7;
        } else {
            mProcess = 5;
            Z2GetAudioMgr()->seStart(Z2SE_SY_LETTER_NEXT_PAGE, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f,
                                     0);
            dMeter2Info_set2DVibration();
            field_0x3e4 = 1;
        }
    } else if (mDoCPd_c::getTrigB(PAD_1) != 0) {
        if (field_0x3e3 <= 1) {
            mProcess = 7;
            Z2GetAudioMgr()->seStart(Z2SE_SY_LETTER_CLOSE, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
            dMeter2Info_set2DVibration();
        } else {
            mProcess = 5;
            Z2GetAudioMgr()->seStart(Z2SE_SY_LETTER_NEXT_PAGE, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f,
                                     0);
            dMeter2Info_set2DVibration();
            field_0x3e4 = 0;
        }
    } else if (field_0x3e2 > 1) {
        if (field_0x3e3 == field_0x3e2) {
            mpArrow->dotAnimeMove();
        } else {
            mpArrow->arwAnimeMove();
        }
        CPaneMgr paneMgr;
        J2DPane* pane = mpLetterScreen[0]->search(MULTI_CHAR('set_ya_n'));
        Vec pos = paneMgr.getGlobalVtxCenter(pane, false, 0);
        mpArrow->setPos(pos.x, pos.y);
    }
}

void dMenu_Letter_c::read_next_fadeout_init() {
    field_0x36a = g_drawHIO.mLetterSelectScreen.mCloseFrame[dMeter_drawLetterHIO_c::LETTER_FRAME];
    setAButtonString(0);
    setBButtonString(0);
}

void dMenu_Letter_c::read_next_fadeout_move() {
    s16 closeWindowFrame =
        g_drawHIO.mLetterSelectScreen.mCloseFrame[dMeter_drawLetterHIO_c::WINDOW_FRAME];
    IF_NOT_DUSK(field_0x36a--);
    if (field_0x36a <= 0) {
        mProcess = 6;
        for (int i = 0; i < 2; i++) {
            field_0x2ec[i]->setAlphaRate(0.0f);

            if (field_0x2f4[i] != NULL) {
                field_0x2f4[i]->setAlphaRate(0.0f);
            }
        }
        if (field_0x3e4 != 0) {
            field_0x3e3++;
        } else {
            field_0x3e3--;
        }
    } else {
        f32 div = (f32)field_0x36a / (f32)closeWindowFrame;
        for (int i = 0; i < 2; i++) {
            field_0x2ec[i]->setAlphaRate(div);
            if (field_0x2f4[i]) {
                field_0x2f4[i]->setAlphaRate(div);
            }
        }
    }
}

void dMenu_Letter_c::read_next_fadein_init() {
    field_0x36a = 0;
    u8 idx = field_0x3ac[field_0x36f * 6 + mIndex] - 1;
    for (int i = 0; i < 2; i++) {
        J2DTextBox* text1 = NULL;
        if (field_0x2f4[i] != 0) {
            text1 = (J2DTextBox*)field_0x2f4[i]->getPanePtr();
        }
        J2DTextBox* text2 = (J2DTextBox*)field_0x2ec[i]->getPanePtr();
        mpString->getStringPage(dMenu_Letter::getLetterText(idx), field_0x3e3 - 1,
                                D_MENU_LETTER_LINE_MAX, text2, text1, NULL, NULL, 0);
    }
    char acStack_30[10];
    SAFE_SPRINTF(acStack_30, "%d/%d", field_0x3e3, field_0x3e2);
    for (int i = 0; i < 2; i++) {
        field_0x1e4[i]->show();
        SAFE_STRCPY(field_0x1e4[i]->getStringPtr(), acStack_30);
    }
}

void dMenu_Letter_c::read_next_fadein_move() {
    s16 openLetterFrame =
        g_drawHIO.mLetterSelectScreen.mOpenFrame[dMeter_drawLetterHIO_c::LETTER_FRAME];
    IF_NOT_DUSK(field_0x36a++);
    if (field_0x36a >= openLetterFrame) {
        mProcess = 4;
        for (int i = 0; i < 2; i++) {
            field_0x2ec[i]->setAlphaRate(1.0f);
            if (field_0x2f4[i] != NULL) {
                field_0x2f4[i]->setAlphaRate(1.0f);
            }
        }
    } else {
        f32 div = (f32)field_0x36a / (f32)openLetterFrame;
        for (int i = 0; i < 2; i++) {
            field_0x2ec[i]->setAlphaRate(div);
            if (field_0x2f4[i] != NULL) {
                field_0x2f4[i]->setAlphaRate(div);
            }
        }
    }
}

void dMenu_Letter_c::read_close_init() {
    field_0x36a = g_drawHIO.mLetterSelectScreen.mCloseFrame[dMeter_drawLetterHIO_c::LETTER_FRAME];
    setAButtonString(0);
    setBButtonString(0);
}

void dMenu_Letter_c::read_close_move() {
    s16 closeLetterFrame =
        g_drawHIO.mLetterSelectScreen.mCloseFrame[dMeter_drawLetterHIO_c::LETTER_FRAME];
    IF_NOT_DUSK(field_0x36a--);
    if (field_0x36a <= 0) {
        mProcess = 0;
        for (int i = 0; i < 2; i++) {
            mpTextParent[i]->scale(0.0f, 0.0f);
            mpTextParent[i]->setAlphaRate(0.0f);
        }
        mpBlackTex->setAlpha(g_drawHIO.mLetterSelectScreen.mWindowBGAlpha);
    } else {
        f32 div = (f32)field_0x36a / (f32)closeLetterFrame;
        for (int i = 0; i < 2; i++) {
            mpTextParent[i]->scale(g_drawHIO.mLetterSelectScreen.mLetterWindowScale * div,
                                   g_drawHIO.mLetterSelectScreen.mLetterWindowScale * div);
            mpTextParent[i]->setAlphaRate(div);
        }
        mpBlackTex->setAlpha(g_drawHIO.mLetterSelectScreen.mWindowBGAlpha * div);
    }
}

#if TARGET_PC
void dMenu_Letter_c::presentAnims() {
    if (mProcess == 1 || mProcess == 2) {
        dusk::vdt::present_addCalc2(&field_0x358, 0.0f, 0.5f, 50.0f, 0.1f);
        return;
    }

    const auto& hio = g_drawHIO.mLetterSelectScreen;
    const bool opening = mProcess == 3 || mProcess == 6;
    const bool page = mProcess == 5 || mProcess == 6;
    if (!opening && mProcess != 5 && mProcess != 7) {
        return;
    }
    const f32 duration = opening ? hio.mOpenFrame[dMeter_drawLetterHIO_c::LETTER_FRAME] :
        hio.mCloseFrame[page ? dMeter_drawLetterHIO_c::WINDOW_FRAME : dMeter_drawLetterHIO_c::LETTER_FRAME];
    dusk::vdt::advance_toward_frame(field_0x36a, opening ? duration : 0.0f, 1.0f);
    const f32 alpha = duration <= 0.0f ? (opening ? 1.0f : 0.0f) :
                                       dusk::vdt::clamped_fraction(field_0x36a, duration);
    for (int i = 0; i < 2; i++) {
        if (page) {
            field_0x2ec[i]->setAlphaRate(alpha);
            if (field_0x2f4[i] != NULL) {
                field_0x2f4[i]->setAlphaRate(alpha);
            }
        } else {
            mpTextParent[i]->scale(hio.mLetterWindowScale * alpha, hio.mLetterWindowScale * alpha);
            mpTextParent[i]->setAlphaRate(alpha);
        }
    }
    if (!page) {
        mpBlackTex->setAlpha(hio.mWindowBGAlpha * alpha);
    }
}
#endif

void dMenu_Letter_c::screenSetMenu() {
    static const u64 tag_sub0[6] = {
        MULTI_CHAR('menu_t0s'), MULTI_CHAR('menu_t1s'), MULTI_CHAR('menu_t2s'), MULTI_CHAR('menu_t3s'), MULTI_CHAR('menu_t4s'), MULTI_CHAR('menu_t5s'),
    };
    static const u64 tag_sub1[6] = {
        MULTI_CHAR('menu_t0'), MULTI_CHAR('menu_t1'), MULTI_CHAR('menu_t2'), MULTI_CHAR('menu_t3'), MULTI_CHAR('menu_t4'), MULTI_CHAR('menu_t5'),
    };
    static const u64 tag_name0[6] = {
        MULTI_CHAR('menu_t6s'), MULTI_CHAR('menu_f7s'), MULTI_CHAR('menu_f8s'), MULTI_CHAR('menu_t9s'), MULTI_CHAR('menu_10s'), MULTI_CHAR('menu_11s'),
    };
    static const u64 tag_name1[6] = {
        MULTI_CHAR('menu_f6'), MULTI_CHAR('menu_f7'), MULTI_CHAR('menu_t8'), MULTI_CHAR('menu_t9'), MULTI_CHAR('menu_t10'), MULTI_CHAR('menu_t11'),
    };
    static const u64 ftag_sub0[6] = {
        MULTI_CHAR('fenu_t0s'), MULTI_CHAR('fenu_t1s'), MULTI_CHAR('fenu_t2s'), MULTI_CHAR('fenu_t3s'), MULTI_CHAR('fenu_t4s'), MULTI_CHAR('fenu_t5s'),
    };
    static const u64 ftag_sub1[6] = {
        MULTI_CHAR('fenu_t0'), MULTI_CHAR('fenu_t1'), MULTI_CHAR('fenu_t2'), MULTI_CHAR('fenu_t3'), MULTI_CHAR('fenu_t4'), MULTI_CHAR('fenu_t5'),
    };
    static const u64 ftag_name0[6] = {
        MULTI_CHAR('fenu_t6s'), MULTI_CHAR('fenu_t7s'), MULTI_CHAR('fenu_f8s'), MULTI_CHAR('fenu_t9s'), MULTI_CHAR('fenu_10s'), MULTI_CHAR('fenu_11s'),
    };
    static const u64 ftag_name1[6] = {
        MULTI_CHAR('fenu_t6'), MULTI_CHAR('fenu_t7'), MULTI_CHAR('fenu_t8'), MULTI_CHAR('fenu_t9'), MULTI_CHAR('fenu_t10'), MULTI_CHAR('fenu_t11'),
    };
    static const u64 tag_letter[6] = {
        MULTI_CHAR('let_00_n'), MULTI_CHAR('let_01_n'), MULTI_CHAR('let_02_n'), MULTI_CHAR('let_03_n'), MULTI_CHAR('let_04_n'), MULTI_CHAR('let_05_n'),
    };
    static const u64 tag_frame[6] = {
        MULTI_CHAR('flame_00'), MULTI_CHAR('flame_01'), MULTI_CHAR('flame_02'), MULTI_CHAR('flame_03'), MULTI_CHAR('flame_04'), MULTI_CHAR('flame_05'),
    };

#if TARGET_PC
    static const u64 tag_menu0_jpn[6] = {
        MULTI_CHAR('menu_t0'), MULTI_CHAR('menu_t1'), MULTI_CHAR('menu_t2'), MULTI_CHAR('menu_t3'), MULTI_CHAR('menu_t4'), MULTI_CHAR('menu_t5'),
    };
    static const u64 tag_menu0[6] = {
        MULTI_CHAR('fenu_t0'), MULTI_CHAR('fenu_t1'), MULTI_CHAR('fenu_t2'), MULTI_CHAR('fenu_t3'), MULTI_CHAR('fenu_t4'), MULTI_CHAR('fenu_t5'),
    };
#else
    static const u64 tag_menu0[6] = {
#if VERSION == VERSION_GCN_JPN
        MULTI_CHAR('menu_t0'), MULTI_CHAR('menu_t1'), MULTI_CHAR('menu_t2'), MULTI_CHAR('menu_t3'), MULTI_CHAR('menu_t4'), MULTI_CHAR('menu_t5'),
#else
        MULTI_CHAR('fenu_t0'), MULTI_CHAR('fenu_t1'), MULTI_CHAR('fenu_t2'), MULTI_CHAR('fenu_t3'), MULTI_CHAR('fenu_t4'), MULTI_CHAR('fenu_t5'),
#endif
    };
#endif

#if TARGET_PC
    static const u64 tag_menu1_jpn[6] = {
        MULTI_CHAR('menu_f6'), MULTI_CHAR('menu_f7'), MULTI_CHAR('menu_t8'), MULTI_CHAR('menu_t9'), MULTI_CHAR('menu_t10'), MULTI_CHAR('menu_t11'),
    };
    static const u64 tag_menu1[6] = {
        MULTI_CHAR('fenu_t6'), MULTI_CHAR('fenu_t7'), MULTI_CHAR('fenu_t8'), MULTI_CHAR('fenu_t9'), MULTI_CHAR('fenu_t10'), MULTI_CHAR('fenu_t11'),
    };
#else
    static const u64 tag_menu1[6] = {
#if VERSION == VERSION_GCN_JPN
        MULTI_CHAR('menu_f6'), MULTI_CHAR('menu_f7'), MULTI_CHAR('menu_t8'), MULTI_CHAR('menu_t9'), MULTI_CHAR('menu_t10'), MULTI_CHAR('menu_t11'),
#else
        MULTI_CHAR('fenu_t6'), MULTI_CHAR('fenu_t7'), MULTI_CHAR('fenu_t8'), MULTI_CHAR('fenu_t9'), MULTI_CHAR('fenu_t10'), MULTI_CHAR('fenu_t11'),
#endif
    };
#endif

    static const u64 tag_midoku[6] = {
        MULTI_CHAR('midoku_0'), MULTI_CHAR('midoku_1'), MULTI_CHAR('midoku_2'), MULTI_CHAR('midoku_3'), MULTI_CHAR('midoku_4'), MULTI_CHAR('midoku_5'),
    };

    mpMenuScreen = JKR_NEW J2DScreen();
    JUT_ASSERT(1146, mpMenuScreen != NULL);
    bool fg = mpMenuScreen->setPriority("zelda_letter_select_6menu.blo", 0x20000, mpArchive);
    JUT_ASSERT(1156, fg != false);
    dPaneClass_showNullPane(mpMenuScreen);
    mpParent[0] = JKR_NEW CPaneMgr(mpMenuScreen, MULTI_CHAR('n_all'), 2, NULL);
    JUT_ASSERT(1161, mpParent[0] != NULL);
    mpParent[0]->setAlphaRate(0.0f);
    for (int i = 0; i < 6; i++) {
#if TARGET_PC
        if (dusk::version::isRegionJpn()) {
            field_0x124[i][0] = (J2DTextBox*)mpMenuScreen->search(tag_sub0[i]);
            field_0x124[i][1] = (J2DTextBox*)mpMenuScreen->search(tag_sub1[i]);
            field_0x124[i][2] = (J2DTextBox*)mpMenuScreen->search(tag_name0[i]);
            field_0x124[i][3] = (J2DTextBox*)mpMenuScreen->search(tag_name1[i]);
            mpMenuScreen->search(ftag_sub0[i])->hide();
            mpMenuScreen->search(ftag_sub1[i])->hide();
            mpMenuScreen->search(ftag_name0[i])->hide();
            mpMenuScreen->search(ftag_name1[i])->hide();
        } else {
            field_0x124[i][0] = (J2DTextBox*)mpMenuScreen->search(ftag_sub0[i]);
            field_0x124[i][1] = (J2DTextBox*)mpMenuScreen->search(ftag_sub1[i]);
            field_0x124[i][2] = (J2DTextBox*)mpMenuScreen->search(ftag_name0[i]);
            field_0x124[i][3] = (J2DTextBox*)mpMenuScreen->search(ftag_name1[i]);
            mpMenuScreen->search(tag_sub0[i])->hide();
            mpMenuScreen->search(tag_sub1[i])->hide();
            mpMenuScreen->search(tag_name0[i])->hide();
            mpMenuScreen->search(tag_name1[i])->hide();
        }
#elif VERSION == VERSION_GCN_JPN
        field_0x124[i][0] = (J2DTextBox*)mpMenuScreen->search(tag_sub0[i]);
        field_0x124[i][1] = (J2DTextBox*)mpMenuScreen->search(tag_sub1[i]);
        field_0x124[i][2] = (J2DTextBox*)mpMenuScreen->search(tag_name0[i]);
        field_0x124[i][3] = (J2DTextBox*)mpMenuScreen->search(tag_name1[i]);
        mpMenuScreen->search(ftag_sub0[i])->hide();
        mpMenuScreen->search(ftag_sub1[i])->hide();
        mpMenuScreen->search(ftag_name0[i])->hide();
        mpMenuScreen->search(ftag_name1[i])->hide();
#else
        field_0x124[i][0] = (J2DTextBox*)mpMenuScreen->search(ftag_sub0[i]);
        field_0x124[i][1] = (J2DTextBox*)mpMenuScreen->search(ftag_sub1[i]);
        field_0x124[i][2] = (J2DTextBox*)mpMenuScreen->search(ftag_name0[i]);
        field_0x124[i][3] = (J2DTextBox*)mpMenuScreen->search(ftag_name1[i]);
        mpMenuScreen->search(tag_sub0[i])->hide();
        mpMenuScreen->search(tag_sub1[i])->hide();
        mpMenuScreen->search(tag_name0[i])->hide();
        mpMenuScreen->search(tag_name1[i])->hide();
#endif
        for (int j = 0; j < 4; j++) {
            field_0x124[i][j]->setFont(mDoExt_getMesgFont());
            field_0x124[i][j]->setString(0x40, "");
        }
    }
    for (int i = 0; i < 6; i++) {
        mpLetterParent[i] = JKR_NEW CPaneMgr(mpMenuScreen, tag_letter[i], 0, NULL);
        JUT_ASSERT(1212, mpLetterParent[i] != NULL);
    }
    for (int i = 0; i < 6; i++) {
        field_0x34[i][0] = mpMenuScreen->search(tag_frame[i]);
        field_0x34[i][1] = mpMenuScreen->search(DUSK_IF_ELSE(dusk::version::isRegionJpn() ? tag_menu0_jpn[i] : tag_menu0[i], tag_menu0[i]));
        field_0x34[i][2] = mpMenuScreen->search(DUSK_IF_ELSE(dusk::version::isRegionJpn() ? tag_menu1_jpn[i] : tag_menu1[i], tag_menu1[i]));
        field_0x34[i][3] = mpMenuScreen->search(tag_letter[i]);
        field_0x34[i][4] = mpMenuScreen->search(tag_midoku[i]);
        if (i < field_0x373) {
            field_0x34[i][3]->show();
            field_0x34[i][4]->show();
        } else {
            field_0x34[i][3]->hide();
            field_0x34[i][4]->hide();
        }
    }
    for (int i = 0; i < 4; i++) {
        if (i == 0) {
            field_0x328[i] = ((J2DPicture*)field_0x34[0][i])->getBlack();
            field_0x348[i] = ((J2DPicture*)field_0x34[0][i])->getWhite();
            field_0x318[i] = ((J2DPicture*)field_0x34[1][i])->getBlack();
            field_0x338[i] = ((J2DPicture*)field_0x34[1][i])->getWhite();
            field_0x328[i].a = 0xff;
            field_0x318[i].a = 0xff;
        } else if (i == 3) {
            field_0x328[i] = field_0x124[i][1]->getBlack();
            field_0x348[i] = field_0x124[i][1]->getWhite();
            field_0x318[i] = field_0x124[i][1]->getBlack();
            field_0x338[i] = field_0x124[i][1]->getWhite();
        } else {
            // Unclear what type is field_0x34
            field_0x328[i] = ((J2DTextBox*)field_0x34[0][i])->getBlack();
            field_0x348[i] = ((J2DTextBox*)field_0x34[0][i])->getWhite();
            field_0x318[i] = ((J2DTextBox*)field_0x34[1][i])->getBlack();
            field_0x338[i] = ((J2DTextBox*)field_0x34[1][i])->getWhite();
        }
    }
    mpMenuDMYScreen = JKR_NEW J2DScreen();
    JUT_ASSERT(1270, mpMenuDMYScreen != NULL);
    fg = mpMenuDMYScreen->setPriority("zelda_letter_select_6menu.blo", 0x20000, mpArchive);
    JUT_ASSERT(1280, fg != false);
    dPaneClass_showNullPane(mpMenuDMYScreen);
    mpDMYParent = JKR_NEW CPaneMgr(mpMenuDMYScreen, MULTI_CHAR('n_all'), 2, NULL);
    JUT_ASSERT(1285, mpDMYParent != NULL);
    for (int i = 0; i < 6; i++) {
#if TARGET_PC
        if (dusk::version::isRegionJpn()) {
            field_0x184[i][0] = (J2DTextBox*)mpMenuDMYScreen->search(tag_sub0[i]);
            field_0x184[i][1] = (J2DTextBox*)mpMenuDMYScreen->search(tag_sub1[i]);
            field_0x184[i][2] = (J2DTextBox*)mpMenuDMYScreen->search(tag_name0[i]);
            field_0x184[i][3] = (J2DTextBox*)mpMenuDMYScreen->search(tag_name1[i]);
            mpMenuDMYScreen->search(ftag_sub0[i])->hide();
            mpMenuDMYScreen->search(ftag_sub1[i])->hide();
            mpMenuDMYScreen->search(ftag_name0[i])->hide();
            mpMenuDMYScreen->search(ftag_name1[i])->hide();
        } else {
            field_0x184[i][0] = (J2DTextBox*)mpMenuDMYScreen->search(ftag_sub0[i]);
            field_0x184[i][1] = (J2DTextBox*)mpMenuDMYScreen->search(ftag_sub1[i]);
            field_0x184[i][2] = (J2DTextBox*)mpMenuDMYScreen->search(ftag_name0[i]);
            field_0x184[i][3] = (J2DTextBox*)mpMenuDMYScreen->search(ftag_name1[i]);
            mpMenuDMYScreen->search(tag_sub0[i])->hide();
            mpMenuDMYScreen->search(tag_sub1[i])->hide();
            mpMenuDMYScreen->search(tag_name0[i])->hide();
            mpMenuDMYScreen->search(tag_name1[i])->hide();
        }
#elif VERSION == VERSION_GCN_JPN
        field_0x184[i][0] = (J2DTextBox*)mpMenuDMYScreen->search(tag_sub0[i]);
        field_0x184[i][1] = (J2DTextBox*)mpMenuDMYScreen->search(tag_sub1[i]);
        field_0x184[i][2] = (J2DTextBox*)mpMenuDMYScreen->search(tag_name0[i]);
        field_0x184[i][3] = (J2DTextBox*)mpMenuDMYScreen->search(tag_name1[i]);
        mpMenuDMYScreen->search(ftag_sub0[i])->hide();
        mpMenuDMYScreen->search(ftag_sub1[i])->hide();
        mpMenuDMYScreen->search(ftag_name0[i])->hide();
        mpMenuDMYScreen->search(ftag_name1[i])->hide();
#else
        field_0x184[i][0] = (J2DTextBox*)mpMenuDMYScreen->search(ftag_sub0[i]);
        field_0x184[i][1] = (J2DTextBox*)mpMenuDMYScreen->search(ftag_sub1[i]);
        field_0x184[i][2] = (J2DTextBox*)mpMenuDMYScreen->search(ftag_name0[i]);
        field_0x184[i][3] = (J2DTextBox*)mpMenuDMYScreen->search(ftag_name1[i]);
        mpMenuDMYScreen->search(tag_sub0[i])->hide();
        mpMenuDMYScreen->search(tag_sub1[i])->hide();
        mpMenuDMYScreen->search(tag_name0[i])->hide();
        mpMenuDMYScreen->search(tag_name1[i])->hide();
#endif
        for (int j = 0; j < 4; j = j + 1) {
            field_0x184[i][j]->setFont(mDoExt_getMesgFont());
            field_0x184[i][j]->setString(0x40, "");
        }
    }
    for (int i = 0; i < 6; i++) {
        field_0xac[i][0] = (J2DTextBox*)mpMenuDMYScreen->search(tag_frame[i]);
        field_0xac[i][1] = (J2DTextBox*)mpMenuDMYScreen->search(tag_menu0[i]);
        field_0xac[i][2] = (J2DTextBox*)mpMenuDMYScreen->search(tag_menu1[i]);
        field_0xac[i][3] = (J2DTextBox*)mpMenuDMYScreen->search(tag_letter[i]);
        field_0xac[i][4] = (J2DTextBox*)mpMenuDMYScreen->search(tag_midoku[i]);
    }
}

void dMenu_Letter_c::screenSetBase() {
    static const u64 tag_pip[9] = {
        MULTI_CHAR('pi_00_n'), MULTI_CHAR('pi_01_n'), MULTI_CHAR('pi_02_n'), MULTI_CHAR('pi_03_n'), MULTI_CHAR('pi_04_n'),
        MULTI_CHAR('pi_05_n'), MULTI_CHAR('pi_06_n'), MULTI_CHAR('pi_07_n'), MULTI_CHAR('pi_08_n'),
    };
    static const u64 tag_pil[9] = {
        MULTI_CHAR('pi_l_00'), MULTI_CHAR('pi_l_01'), MULTI_CHAR('pi_l_02'), MULTI_CHAR('pi_l_03'), MULTI_CHAR('pi_l_04'),
        MULTI_CHAR('pi_l_05'), MULTI_CHAR('pi_l_06'), MULTI_CHAR('pi_l_07'), MULTI_CHAR('pi_l_08'),
    };
    static const u64 tag_pii[9] = {
        MULTI_CHAR('pi_00'), MULTI_CHAR('pi_01'), MULTI_CHAR('pi_02'), MULTI_CHAR('pi_03'), MULTI_CHAR('pi_04'), MULTI_CHAR('pi_05'), MULTI_CHAR('pi_06'), MULTI_CHAR('pi_07'), MULTI_CHAR('pi_08'),
    };

    mpBaseScreen = JKR_NEW J2DScreen();
    JUT_ASSERT(1357, mpBaseScreen != NULL);
    bool fg = mpBaseScreen->setPriority("zelda_letter_select_base.blo", 0x20000, mpArchive);
    JUT_ASSERT(1361, fg != false);
    dPaneClass_showNullPane(mpBaseScreen);
    mpBaseScreen->search(MULTI_CHAR('wi_btn_n'))->hide();
    mpParent[1] = JKR_NEW CPaneMgr(mpBaseScreen, MULTI_CHAR('n_all'), 2, NULL);
    JUT_ASSERT(1372, mpParent[1] != NULL);
    mpParent[1]->setAlphaRate(0.0f);
#if TARGET_PC
    J2DTextBox* piVar9;
    if (dusk::version::isRegionJpn()) {
        piVar9 = (J2DTextBox*)mpBaseScreen->search(MULTI_CHAR('t_t00'));
        mpBaseScreen->search(MULTI_CHAR('f_t_00'))->hide();
    } else {
        piVar9 = (J2DTextBox*)mpBaseScreen->search(MULTI_CHAR('f_t_00'));
        mpBaseScreen->search(MULTI_CHAR('t_t00'))->hide();
    }
#elif VERSION == VERSION_GCN_JPN
    J2DTextBox* piVar9 = (J2DTextBox*)mpBaseScreen->search(MULTI_CHAR('t_t00'));
    mpBaseScreen->search(MULTI_CHAR('f_t_00'))->hide();
#else
    J2DTextBox* piVar9 = (J2DTextBox*)mpBaseScreen->search(MULTI_CHAR('f_t_00'));
    mpBaseScreen->search(MULTI_CHAR('t_t00'))->hide();
#endif
    piVar9->setString(0x40, "");
    piVar9->setFont(mDoExt_getSubFont());
    dComIfGp_setMessageCountNumber(field_0x374 + (field_0x36f + 1) * 100);
    mpString->getString(0x4d6, piVar9, NULL, NULL, NULL, 0);
#if TARGET_PC
    J2DTextBox* text1;
    J2DTextBox* text2;
    J2DTextBox* text3;
    J2DTextBox* text4;
    if (dusk::version::isRegionJpn()) {
        text1 = (J2DTextBox*)mpBaseScreen->search(MULTI_CHAR('wps_text'));
        text2 = (J2DTextBox*)mpBaseScreen->search(MULTI_CHAR('w_p_text'));
        text3 = (J2DTextBox*)mpBaseScreen->search(MULTI_CHAR('g_ps_txt'));
        text4 = (J2DTextBox*)mpBaseScreen->search(MULTI_CHAR('g_p_text'));
        mpBaseScreen->search(MULTI_CHAR('fwpstex1'))->hide();
        mpBaseScreen->search(MULTI_CHAR('fwp_tex1'))->hide();
        mpBaseScreen->search(MULTI_CHAR('fgps_tx1'))->hide();
        mpBaseScreen->search(MULTI_CHAR('fgp_tex1'))->hide();
    } else {
        text1 = (J2DTextBox*)mpBaseScreen->search(MULTI_CHAR('fwpstex1'));
        text2 = (J2DTextBox*)mpBaseScreen->search(MULTI_CHAR('fwp_tex1'));
        text3 = (J2DTextBox*)mpBaseScreen->search(MULTI_CHAR('fgps_tx1'));
        text4 = (J2DTextBox*)mpBaseScreen->search(MULTI_CHAR('fgp_tex1'));
        mpBaseScreen->search(MULTI_CHAR('wps_text'))->hide();
        mpBaseScreen->search(MULTI_CHAR('w_p_text'))->hide();
        mpBaseScreen->search(MULTI_CHAR('g_ps_txt'))->hide();
        mpBaseScreen->search(MULTI_CHAR('g_p_text'))->hide();
    }
#elif VERSION == VERSION_GCN_JPN
    J2DTextBox* text1 = (J2DTextBox*)mpBaseScreen->search(MULTI_CHAR('wps_text'));
    J2DTextBox* text2 = (J2DTextBox*)mpBaseScreen->search(MULTI_CHAR('w_p_text'));
    J2DTextBox* text3 = (J2DTextBox*)mpBaseScreen->search(MULTI_CHAR('g_ps_txt'));
    J2DTextBox* text4 = (J2DTextBox*)mpBaseScreen->search(MULTI_CHAR('g_p_text'));
    mpBaseScreen->search(MULTI_CHAR('fwpstex1'))->hide();
    mpBaseScreen->search(MULTI_CHAR('fwp_tex1'))->hide();
    mpBaseScreen->search(MULTI_CHAR('fgps_tx1'))->hide();
    mpBaseScreen->search(MULTI_CHAR('fgp_tex1'))->hide();
#else
    J2DTextBox* text1 = (J2DTextBox*)mpBaseScreen->search(MULTI_CHAR('fwpstex1'));
    J2DTextBox* text2 = (J2DTextBox*)mpBaseScreen->search(MULTI_CHAR('fwp_tex1'));
    J2DTextBox* text3 = (J2DTextBox*)mpBaseScreen->search(MULTI_CHAR('fgps_tx1'));
    J2DTextBox* text4 = (J2DTextBox*)mpBaseScreen->search(MULTI_CHAR('fgp_tex1'));
    mpBaseScreen->search(MULTI_CHAR('wps_text'))->hide();
    mpBaseScreen->search(MULTI_CHAR('w_p_text'))->hide();
    mpBaseScreen->search(MULTI_CHAR('g_ps_txt'))->hide();
    mpBaseScreen->search(MULTI_CHAR('g_p_text'))->hide();
#endif
    text1->setString(0x40, "");
    text2->setString(0x40, "");
    text3->setString(0x40, "");
    text4->setString(0x40, "");
    text1->setFont(mDoExt_getMesgFont());
    text2->setFont(mDoExt_getMesgFont());
    text3->setFont(mDoExt_getMesgFont());
    text4->setFont(mDoExt_getMesgFont());
    dMeter2Info_getStringKanji(0x4d7, text1->getStringPtr(), 0);
    dMeter2Info_getStringKanji(0x4d7, text2->getStringPtr(), 0);
    dMeter2Info_getStringKanji(0x4d7, text3->getStringPtr(), 0);
    dMeter2Info_getStringKanji(0x4d7, text4->getStringPtr(), 0);
    ((J2DTextBox*)mpBaseScreen->search(MULTI_CHAR('pi_no_00')))->setFont(mDoExt_getMesgFont());
    ((J2DTextBox*)mpBaseScreen->search(MULTI_CHAR('pi_no_01')))->setFont(mDoExt_getMesgFont());
    ((J2DTextBox*)mpBaseScreen->search(MULTI_CHAR('pi_no_02')))->setFont(mDoExt_getMesgFont());
    ((J2DTextBox*)mpBaseScreen->search(MULTI_CHAR('pi_no_03')))->setFont(mDoExt_getMesgFont());
    ((J2DTextBox*)mpBaseScreen->search(MULTI_CHAR('pi_no_04')))->setFont(mDoExt_getMesgFont());
    ((J2DTextBox*)mpBaseScreen->search(MULTI_CHAR('pi_no_05')))->setFont(mDoExt_getMesgFont());
    ((J2DTextBox*)mpBaseScreen->search(MULTI_CHAR('pi_no_06')))->setFont(mDoExt_getMesgFont());
    ((J2DTextBox*)mpBaseScreen->search(MULTI_CHAR('pi_no_07')))->setFont(mDoExt_getMesgFont());
    ((J2DTextBox*)mpBaseScreen->search(MULTI_CHAR('pi_no_08')))->setFont(mDoExt_getMesgFont());
    field_0x1ec = (J2DTextBox*)mpBaseScreen->search(MULTI_CHAR('let_area'));
    for (int i = 0; i < 9; i++) {
        field_0x1f0[i] = (J2DTextBox*)mpBaseScreen->search(tag_pip[i]);
        if (field_0x374 > 1 && i < field_0x374) {
            field_0x1f0[i]->show();
        } else {
            field_0x1f0[i]->hide();
        }
        mpPageLight[i] = (J2DPicture*)mpBaseScreen->search(tag_pil[i]);
        if (i == field_0x36f) {
            mpPageLight[i]->show();
        } else {
            mpPageLight[i]->hide();
        }
        field_0x238[i] = (J2DTextBox*)mpBaseScreen->search(tag_pii[i]);
    }
    if (field_0x374 > 1) {
        J2DPane* pJVar6 = mpBaseScreen->search('pi_n');
        f32 x1 = field_0x1f0[1]->getBounds().i.x;
        f32 dVar18 = x1 - field_0x1f0[0]->getBounds().i.x;
        f32 dVar17 = dVar18 * (field_0x374 - 1);
        f32 dVar16 = (pJVar6->getWidth() / 2) - (dVar17 / 2);
        for (int i = 0; i < 9; i++) {
            f32 y = field_0x1f0[i]->getBounds().i.y;
            field_0x1f0[i]->move(dVar16 + dVar18 * i, y);
        }
    }
}

void dMenu_Letter_c::screenSetShadow() {
    mpSdwScreen = JKR_NEW J2DScreen();
    mpSdwScreen->setPriority("zelda_letter_select_shadow.blo", 0x20000, mpArchive);
    dPaneClass_showNullPane(mpSdwScreen);
    mpParent[2] = JKR_NEW CPaneMgr(mpSdwScreen, MULTI_CHAR('n_all'), 2, NULL);
    mpParent[2]->setAlphaRate(0.0f);
}

void dMenu_Letter_c::screenSetLetter() {
    mpLetterScreen[0] = JKR_NEW J2DScreen();
    JUT_ASSERT(1507, mpLetterScreen[0] != NULL);
    bool fg = mpLetterScreen[0]->setPriority("zelda_letter_window_base.blo", 0x20000, mpArchive);
    JUT_ASSERT(1511, fg != false);
    dPaneClass_showNullPane(mpLetterScreen[0]);

#if TARGET_PC
    if (dusk::version::isRegionJpn()) {
        if (dComIfGs_getOptRuby() == 0) {
            field_0x2ec[0] = JKR_NEW CPaneMgr(mpLetterScreen[0], MULTI_CHAR('t3f_s'), 0, NULL);
            field_0x2ec[1] = JKR_NEW CPaneMgr(mpLetterScreen[0], MULTI_CHAR('mg_3flin'), 0, NULL);
            field_0x2f4[0] = JKR_NEW CPaneMgr(mpLetterScreen[0], MULTI_CHAR('mg_3f_s'), 0, NULL);
            field_0x2f4[1] = JKR_NEW CPaneMgr(mpLetterScreen[0], MULTI_CHAR('mg_3f'), 0, NULL);
            mpLetterScreen[0]->search('t4_s')->hide();
            mpLetterScreen[0]->search(MULTI_CHAR('mg_e4lin'))->hide();
            mpLetterScreen[0]->search('t3_s')->hide();
            mpLetterScreen[0]->search(MULTI_CHAR('mg_3line'))->hide();
            mpLineParent = JKR_NEW CPaneMgr(mpLetterScreen[0], MULTI_CHAR('jp_fri_n'), 0, NULL);
            JUT_ASSERT(1530, mpLineParent != NULL);
            for (int i = 0; i < 12; i++) {
                static u64 const line_tag[12] = {
                    MULTI_CHAR('line00'), MULTI_CHAR('line01'), MULTI_CHAR('line02'), MULTI_CHAR('line03'), MULTI_CHAR('line04'), MULTI_CHAR('line05'),
                    MULTI_CHAR('line06'), MULTI_CHAR('line07'), MULTI_CHAR('line08'),
                };

                if (line_tag[i] != 0) {
                    field_0x25c[i] = (J2DTextBox*)mpLetterScreen[0]->search(line_tag[i]);
                } else {
                    field_0x25c[i] = NULL;
                }
            }
            mpLetterScreen[0]->search('jp_n')->hide();
            mpLetterScreen[0]->search('us_n')->hide();

            ((J2DTextBox*)field_0x2f4[0]->getPanePtr())->setLineSpace(((J2DTextBox*)field_0x2ec[0]->getPanePtr())->getLineSpace());
            ((J2DTextBox*)field_0x2f4[1]->getPanePtr())->setLineSpace(((J2DTextBox*)field_0x2ec[1]->getPanePtr())->getLineSpace());
        } else {
            field_0x2ec[0] = JKR_NEW CPaneMgr(mpLetterScreen[0], 't3_s', 0, NULL);
            field_0x2ec[1] = JKR_NEW CPaneMgr(mpLetterScreen[0], MULTI_CHAR('mg_3line'), 0, NULL);
            for (int i = 0; i < 2; i++) {
                field_0x2f4[i] = NULL;
            }
            mpLetterScreen[0]->search('t4_s')->hide();
            mpLetterScreen[0]->search(MULTI_CHAR('mg_e4lin'))->hide();
            mpLetterScreen[0]->search(MULTI_CHAR('t3f_s'))->hide();
            mpLetterScreen[0]->search(MULTI_CHAR('mg_3flin'))->hide();
            mpLetterScreen[0]->search(MULTI_CHAR('mg_3f_s'))->hide();
            mpLetterScreen[0]->search(MULTI_CHAR('mg_3f'))->hide();
            mpLineParent = JKR_NEW CPaneMgr(mpLetterScreen[0], 'jp_n', 0, NULL);
            JUT_ASSERT(1530, mpLineParent != NULL);
            for (int i = 0; i < 12; i++) {
                static u64 const line_tag[12] = {
                    MULTI_CHAR('line21'), MULTI_CHAR('line22'), MULTI_CHAR('line23'), MULTI_CHAR('line24'), MULTI_CHAR('line25'), MULTI_CHAR('line26'),
                    MULTI_CHAR('line27'), MULTI_CHAR('line28'), MULTI_CHAR('line29'),
                };

                if (line_tag[i] != 0) {
                    field_0x25c[i] = (J2DTextBox*)mpLetterScreen[0]->search(line_tag[i]);
                } else {
                    field_0x25c[i] = NULL;
                }
            }

            mpLetterScreen[0]->search(MULTI_CHAR('jp_fri_n'))->hide();
            mpLetterScreen[0]->search('us_n')->hide();
        }
    } else {
        static u64 const line_tag[12] = {
            MULTI_CHAR('line09'), MULTI_CHAR('line10'), MULTI_CHAR('line11'), MULTI_CHAR('line12'), MULTI_CHAR('line13'), MULTI_CHAR('line14'),
            MULTI_CHAR('line15'), MULTI_CHAR('line16'), MULTI_CHAR('line17'), MULTI_CHAR('line18'), MULTI_CHAR('line19'), MULTI_CHAR('line20'),
        };

        field_0x2ec[0] = JKR_NEW CPaneMgr(mpLetterScreen[0], 't4_s', 0, NULL);
        field_0x2ec[1] = JKR_NEW CPaneMgr(mpLetterScreen[0], MULTI_CHAR('mg_e4lin'), 0, NULL);
        for (int i = 0; i < 2; i++) {
            field_0x2f4[i] = NULL;
        }
        mpLetterScreen[0]->search(MULTI_CHAR('t3f_s'))->hide();
        mpLetterScreen[0]->search(MULTI_CHAR('mg_3flin'))->hide();
        mpLetterScreen[0]->search(MULTI_CHAR('mg_3f_s'))->hide();
        mpLetterScreen[0]->search(MULTI_CHAR('mg_3f'))->hide();
        mpLetterScreen[0]->search('t3_s')->hide();
        mpLetterScreen[0]->search(MULTI_CHAR('mg_3line'))->hide();
        mpLineParent = JKR_NEW CPaneMgr(mpLetterScreen[0], 'us_n', 0, NULL);
        JUT_ASSERT(1530, mpLineParent != NULL);
        for (int i = 0; i < 12; i++) {
            if (line_tag[i] != 0) {
                field_0x25c[i] = (J2DTextBox*)mpLetterScreen[0]->search(line_tag[i]);
            } else {
                field_0x25c[i] = NULL;
            }
        }
        mpLetterScreen[0]->search(MULTI_CHAR('jp_fri_n'))->hide();
        mpLetterScreen[0]->search('jp_n')->hide();
    }
#elif VERSION == VERSION_GCN_JPN
    if (dComIfGs_getOptRuby() == 0) {
        field_0x2ec[0] = JKR_NEW CPaneMgr(mpLetterScreen[0], MULTI_CHAR('t3f_s'), 0, NULL);
        field_0x2ec[1] = JKR_NEW CPaneMgr(mpLetterScreen[0], MULTI_CHAR('mg_3flin'), 0, NULL);
        field_0x2f4[0] = JKR_NEW CPaneMgr(mpLetterScreen[0], MULTI_CHAR('mg_3f_s'), 0, NULL);
        field_0x2f4[1] = JKR_NEW CPaneMgr(mpLetterScreen[0], MULTI_CHAR('mg_3f'), 0, NULL);
        mpLetterScreen[0]->search('t4_s')->hide();
        mpLetterScreen[0]->search(MULTI_CHAR('mg_e4lin'))->hide();
        mpLetterScreen[0]->search('t3_s')->hide();
        mpLetterScreen[0]->search(MULTI_CHAR('mg_3line'))->hide();
        mpLineParent = JKR_NEW CPaneMgr(mpLetterScreen[0], MULTI_CHAR('jp_fri_n'), 0, NULL);
        JUT_ASSERT(1530, mpLineParent != NULL);
        for (int i = 0; i < 12; i++) {
            static u64 const line_tag[12] = {
                MULTI_CHAR('line00'), MULTI_CHAR('line01'), MULTI_CHAR('line02'), MULTI_CHAR('line03'), MULTI_CHAR('line04'), MULTI_CHAR('line05'),
                MULTI_CHAR('line06'), MULTI_CHAR('line07'), MULTI_CHAR('line08'),
            };

            if (line_tag[i] != 0) {
                field_0x25c[i] = (J2DTextBox*)mpLetterScreen[0]->search(line_tag[i]);
            } else {
                field_0x25c[i] = NULL;
            }
        }
        mpLetterScreen[0]->search('jp_n')->hide();
        mpLetterScreen[0]->search('us_n')->hide();

        ((J2DTextBox*)field_0x2f4[0]->getPanePtr())->setLineSpace(((J2DTextBox*)field_0x2ec[0]->getPanePtr())->getLineSpace());
        ((J2DTextBox*)field_0x2f4[1]->getPanePtr())->setLineSpace(((J2DTextBox*)field_0x2ec[1]->getPanePtr())->getLineSpace());
    } else {
        field_0x2ec[0] = JKR_NEW CPaneMgr(mpLetterScreen[0], 't3_s', 0, NULL);
        field_0x2ec[1] = JKR_NEW CPaneMgr(mpLetterScreen[0], MULTI_CHAR('mg_3line'), 0, NULL);
        for (int i = 0; i < 2; i++) {
            field_0x2f4[i] = NULL;
        }
        mpLetterScreen[0]->search('t4_s')->hide();
        mpLetterScreen[0]->search(MULTI_CHAR('mg_e4lin'))->hide();
        mpLetterScreen[0]->search(MULTI_CHAR('t3f_s'))->hide();
        mpLetterScreen[0]->search(MULTI_CHAR('mg_3flin'))->hide();
        mpLetterScreen[0]->search(MULTI_CHAR('mg_3f_s'))->hide();
        mpLetterScreen[0]->search(MULTI_CHAR('mg_3f'))->hide();
        mpLineParent = JKR_NEW CPaneMgr(mpLetterScreen[0], 'jp_n', 0, NULL);
        JUT_ASSERT(1530, mpLineParent != NULL);
        for (int i = 0; i < 12; i++) {
            static u64 const line_tag[12] = {
                MULTI_CHAR('line21'), MULTI_CHAR('line22'), MULTI_CHAR('line23'), MULTI_CHAR('line24'), MULTI_CHAR('line25'), MULTI_CHAR('line26'),
                MULTI_CHAR('line27'), MULTI_CHAR('line28'), MULTI_CHAR('line29'),
            };

            if (line_tag[i] != 0) {
                field_0x25c[i] = (J2DTextBox*)mpLetterScreen[0]->search(line_tag[i]);
            } else {
                field_0x25c[i] = NULL;
            }
        }

        mpLetterScreen[0]->search(MULTI_CHAR('jp_fri_n'))->hide();
        mpLetterScreen[0]->search('us_n')->hide();
    }
#else
    static u64 const line_tag[12] = {
        MULTI_CHAR('line09'), MULTI_CHAR('line10'), MULTI_CHAR('line11'), MULTI_CHAR('line12'), MULTI_CHAR('line13'), MULTI_CHAR('line14'),
        MULTI_CHAR('line15'), MULTI_CHAR('line16'), MULTI_CHAR('line17'), MULTI_CHAR('line18'), MULTI_CHAR('line19'), MULTI_CHAR('line20'),
    };

    field_0x2ec[0] = JKR_NEW CPaneMgr(mpLetterScreen[0], 't4_s', 0, NULL);
    field_0x2ec[1] = JKR_NEW CPaneMgr(mpLetterScreen[0], MULTI_CHAR('mg_e4lin'), 0, NULL);
    for (int i = 0; i < 2; i++) {
        field_0x2f4[i] = NULL;
    }
    mpLetterScreen[0]->search(MULTI_CHAR('t3f_s'))->hide();
    mpLetterScreen[0]->search(MULTI_CHAR('mg_3flin'))->hide();
    mpLetterScreen[0]->search(MULTI_CHAR('mg_3f_s'))->hide();
    mpLetterScreen[0]->search(MULTI_CHAR('mg_3f'))->hide();
    mpLetterScreen[0]->search('t3_s')->hide();
    mpLetterScreen[0]->search(MULTI_CHAR('mg_3line'))->hide();
    mpLineParent = JKR_NEW CPaneMgr(mpLetterScreen[0], 'us_n', 0, NULL);
    JUT_ASSERT(1530, mpLineParent != NULL);
    for (int i = 0; i < 12; i++) {
        if (line_tag[i] != 0) {
            field_0x25c[i] = (J2DTextBox*)mpLetterScreen[0]->search(line_tag[i]);
        } else {
            field_0x25c[i] = NULL;
        }
    }
    mpLetterScreen[0]->search(MULTI_CHAR('jp_fri_n'))->hide();
    mpLetterScreen[0]->search('jp_n')->hide();
#endif

#if TARGET_PC
#define STR_BUF_LEN 528
#elif VERSION == VERSION_GCN_JPN
#define STR_BUF_LEN 528
#else
#define STR_BUF_LEN 512
#endif

    for (int i = 0; i < 2; i++) {
        ((J2DTextBox*)field_0x2ec[i]->getPanePtr())->setFont(mDoExt_getMesgFont());
        ((J2DTextBox*)field_0x2ec[i]->getPanePtr())->setString(STR_BUF_LEN, "");
        if (field_0x2f4[i] != NULL) {
            ((J2DTextBox*)field_0x2f4[i]->getPanePtr())->setFont(mDoExt_getMesgFont());
            ((J2DTextBox*)field_0x2f4[i]->getPanePtr())->setString(STR_BUF_LEN, "");
        }
    }
    field_0x1e4[0] = (J2DTextBox*)mpLetterScreen[0]->search(MULTI_CHAR('p_texts'));
    field_0x1e4[1] = (J2DTextBox*)mpLetterScreen[0]->search(MULTI_CHAR('p_text'));
    for (int i = 0; i < 2; i++) {
        field_0x1e4[i]->setFont(mDoExt_getSubFont());
        field_0x1e4[i]->setString(0x10, "");
    }
    mpLetterScreen[1] = JKR_NEW J2DScreen();
    JUT_ASSERT(1627, mpLetterScreen[1] != NULL);
    fg = mpLetterScreen[1]->setPriority("zelda_letter_window_spot.blo", 0x20000, mpArchive);
    JUT_ASSERT(1631, fg != false);
    dPaneClass_showNullPane(mpLetterScreen[1]);
    for (int i = 0; i < 2; i++) {
        mpTextParent[i] = JKR_NEW CPaneMgr(mpLetterScreen[i], MULTI_CHAR('n_all'), 2, NULL);
        JUT_ASSERT(1636, mpTextParent[i] != NULL);
        mpTextParent[i]->scale(0.0f, 0.0f);
        mpTextParent[i]->setAlphaRate(0.0f);
    }
    ResTIMG* local_30 = (ResTIMG*)dComIfGp_getMain2DArchive()->getResource('TIMG', "tt_block8x8.bti");
    mpBlackTex = JKR_NEW J2DPicture(local_30);
    JUT_ASSERT(1644, mpBlackTex != NULL);
    mpBlackTex->setBlackWhite(JUtility::TColor(0, 0, 0, 0), JUtility::TColor(0, 0, 0, 255));
    mpBlackTex->setAlpha(0);
}

void dMenu_Letter_c::screenSetDoIcon() {
    static const u64 text_a_tag[5] = {
        MULTI_CHAR('atext1_1'), MULTI_CHAR('atext1_2'), MULTI_CHAR('atext1_3'), MULTI_CHAR('atext1_4'), MULTI_CHAR('atext1_5'),
    };
    static const u64 text_b_tag[5] = {
        MULTI_CHAR('btext1_1'), MULTI_CHAR('btext1_2'), MULTI_CHAR('btext1_3'), MULTI_CHAR('btext1_4'), MULTI_CHAR('btext1_5'),
    };

    mpIconScreen = JKR_NEW J2DScreen();
    mpIconScreen->setPriority("zelda_collect_soubi_do_icon_parts.blo", 0x20000, mpArchive);
    for (int i = 0; i < 2; i++) {
        mpButtonAB[i] = NULL;
        mpButtonText[i] = NULL;
    }
    dPaneClass_showNullPane(mpIconScreen);
    for (int i = 0; i < 5; i++) {
        mpAButtonString[i] = (J2DTextBox*)mpIconScreen->search(text_a_tag[i]);
        mpBButtonString[i] = (J2DTextBox*)mpIconScreen->search(text_b_tag[i]);
        mpAButtonString[i]->setFont(mDoExt_getMesgFont());
        mpBButtonString[i]->setFont(mDoExt_getMesgFont());
        mpAButtonString[i]->setString(0x20, "");
        mpBButtonString[i]->setString(0x20, "");
    }
}


void dMenu_Letter_c::setCursorPos() {
    Vec pos = mpLetterParent[mIndex]->getGlobalVtxCenter(mpLetterParent[mIndex]->mPane, false, 0);
    mpDrawCursor->setPos(pos.x, pos.y, mpLetterParent[mIndex]->getPanePtr(), false);
}

void dMenu_Letter_c::changeActiveColor() {
    setCursorPos();
    for (int i = 0; (int)i < 6; i++) {
        if (i == mIndex) {
            ((J2DPicture*)field_0x34[i][0])->setBlackWhite(field_0x318[0], field_0x338[0]);
            ((J2DTextBox*)field_0x34[i][1])->setBlackWhite(field_0x318[1], field_0x338[1]);
            ((J2DTextBox*)field_0x34[i][2])->setBlackWhite(field_0x318[2], field_0x338[2]);
            field_0x124[i][1]->setBlackWhite(field_0x318[3], field_0x338[3]);
            field_0x124[i][3]->setBlackWhite(field_0x318[3], field_0x338[3]);
            mpLetterParent[i]->scale(field_0x360, field_0x360);
        } else {
            ((J2DPicture*)field_0x34[i][0])->setBlackWhite(field_0x328[0], field_0x348[0]);
            ((J2DTextBox*)field_0x34[i][1])->setBlackWhite(field_0x328[1], field_0x348[1]);
            ((J2DTextBox*)field_0x34[i][2])->setBlackWhite(field_0x328[2], field_0x348[2]);
            field_0x124[i][1]->setBlackWhite(field_0x328[3], field_0x348[3]);
            field_0x124[i][3]->setBlackWhite(field_0x328[3], field_0x348[3]);
            mpLetterParent[i]->scale(field_0x364, field_0x364);
        }
    }
}

void dMenu_Letter_c::changePageLight() {
    for (int i = 0; i < 9; i++) {
        if (i == field_0x36f) {
            mpPageLight[i]->show();
        } else {
            mpPageLight[i]->hide();
        }
    }
}

void dMenu_Letter_c::setPageText() {
    int dVar1 = field_0x36f * 6;
    for (int i = 0; i < field_0x373; i++) {
        u8 idx = field_0x3ac[i + dVar1] - 1;
        mpString->getString(dMenu_Letter::getLetterSubject(idx),field_0x124[i][0], NULL, NULL, NULL, 0);
        mpString->getString(dMenu_Letter::getLetterSubject(idx),field_0x124[i][1], NULL, NULL, NULL, 0);
        mpString->getString(dMenu_Letter::getLetterName(idx),field_0x124[i][2], NULL, NULL, NULL, 0);
        mpString->getString(dMenu_Letter::getLetterName(idx),field_0x124[i][3], NULL, NULL, NULL, 0);
    }
    for (int i = 0; i < 6; i++) {
        if (i < field_0x373) {
            u8 idx = field_0x3ac[i + dVar1] - 1;
            if (!dComIfGs_isLetterReadFlag(idx)) {
                field_0x34[i][4]->show();
            } else {
                field_0x34[i][4]->hide();
            }
        } else {
            field_0x34[i][4]->hide();
        }
    }
}

void dMenu_Letter_c::setDMYPageText() {
    int dVar1 = field_0x372 * 6;
    for (int i = 0; i < field_0x373; i++) {
        u8 idx = field_0x3ac[i + dVar1] - 1;
        mpString->getString(dMenu_Letter::getLetterSubject(idx),field_0x184[i][0], NULL, NULL, NULL, 0);
        mpString->getString(dMenu_Letter::getLetterSubject(idx),field_0x184[i][1], NULL, NULL, NULL, 0);
        mpString->getString(dMenu_Letter::getLetterName(idx),field_0x184[i][2], NULL, NULL, NULL, 0);
        mpString->getString(dMenu_Letter::getLetterName(idx),field_0x184[i][3], NULL, NULL, NULL, 0);
    }
    for (int i = 0; i < 6; i++) {
        if (i < field_0x373) {
            u8 idx = field_0x3ac[i + dVar1] - 1;
            if (!dComIfGs_isLetterReadFlag(idx)) {
                field_0xac[i][4]->show();
            } else {
                field_0xac[i][4]->hide();
            }
        } else {
            field_0xac[i][4]->hide();
        }
    }
}

void dMenu_Letter_c::copyDMYMenu() {
    setDMYPageText();
    for (int i = 0; i < 6; i++) {
        if (i == mIndex) {
            ((J2DPicture*)field_0xac[i][0])->setBlackWhite(field_0x318[0], field_0x338[0]);
            field_0xac[i][1]->setBlackWhite(field_0x318[1], field_0x338[1]);
            field_0xac[i][2]->setBlackWhite(field_0x318[2], field_0x338[2]);
            field_0x184[i][1]->setBlackWhite(field_0x318[3], field_0x338[3]);
            field_0x184[i][3]->setBlackWhite(field_0x318[3], field_0x338[3]);
            field_0xac[i][3]->scale(field_0x360, field_0x360);
        } else {
            ((J2DPicture*)field_0xac[i][0])->setBlackWhite(field_0x328[0], field_0x348[0]);
            field_0xac[i][1]->setBlackWhite(field_0x328[1], field_0x348[1]);
            field_0xac[i][2]->setBlackWhite(field_0x328[2], field_0x348[2]);
            field_0x184[i][1]->setBlackWhite(field_0x328[3], field_0x348[3]);
            field_0x184[i][3]->setBlackWhite(field_0x328[3], field_0x348[3]);
            field_0xac[i][3]->scale(field_0x364, field_0x364);
        }
        if (i < field_0x373) {
            field_0xac[i][3]->show();
        } else {
            field_0xac[i][3]->hide();
        }
    }
    mIndex = 0;
    int dVar1 = (m_letter_num - 6 * field_0x36f) & 0xff;
    if (dVar1 < 6) {
        field_0x373 = dVar1;
    } else {
        field_0x373 = 6;
    }
    changeActiveColor();
    setPageText();
    for (int i = 0; i < 6; i++) {
        if (i < field_0x373) {
            field_0x34[i][3]->show();
        } else {
            field_0x34[i][3]->hide();
        }
    }
}

void dMenu_Letter_c::setAButtonString(u16 i_stringID) {
    if (i_stringID == 0) {
        for (int i = 0; i < 5; i++) {
            SAFE_STRCPY(mpAButtonString[i]->getStringPtr(), "");
        }
    } else {
        for (int i = 0; i < 5; i++) {
            dMeter2Info_getStringKanji(i_stringID, mpAButtonString[i]->getStringPtr(), NULL);
        }
    }
}


void dMenu_Letter_c::setBButtonString(u16 i_stringID) {
    if (i_stringID == 0) {
        for (int i = 0; i < 5; i++) {
            SAFE_STRCPY(mpBButtonString[i]->getStringPtr(), "");
        }
    } else {
        for (int i = 0; i < 5; i++) {
            dMeter2Info_getStringKanji(i_stringID, mpBButtonString[i]->getStringPtr(), NULL);
        }
    }
}


u8 dMenu_Letter_c::getLetterNum() {
    u8 letterNum = 0;
    if (g_drawHIO.mLetterSelectScreen.mLetterNum > 0) {
        for (int i = 0; i < g_drawHIO.mLetterSelectScreen.mLetterNum; i++) {
            if (dMenu_Letter::getLetterSubject(i) != 0 || dMenu_Letter::getLetterName(i) != 0 ||
                dMenu_Letter::getLetterText(i) != 0)
            {
                dComIfGs_onLetterGetFlag(i);
                dComIfGs_setGetNumber(letterNum, i + 1);
                letterNum++;
            }
        }
    } else {
        letterNum = dMeter2Info_getRecieveLetterNum();
    }
    return letterNum;
}

void dMenu_Letter_c::setHIO(bool i_useHIO) {
    if (i_useHIO || g_drawHIO.mLetterSelectScreen.mDebugON) {
        for (int i = 0; i < 3; i++) {
            mpParent[i]->paneTrans(g_drawHIO.mLetterSelectScreen.mWindowPosX,
                                   g_drawHIO.mLetterSelectScreen.mWindowPosY);
            mpParent[i]->scale(g_drawHIO.mLetterSelectScreen.mWindowScale,
                               g_drawHIO.mLetterSelectScreen.mWindowScale);
        }
        mpDMYParent->paneTrans(g_drawHIO.mLetterSelectScreen.mWindowPosX,
                               g_drawHIO.mLetterSelectScreen.mWindowPosY);
        mpDMYParent->scale(g_drawHIO.mLetterSelectScreen.mWindowScale,
                           g_drawHIO.mLetterSelectScreen.mWindowScale);
        for (int i = 0; i < 9; i++) {
            mpPageLight[i]->setBlackWhite(g_drawHIO.mLetterSelectScreen.mSelectPageIconBack,
                                          g_drawHIO.mLetterSelectScreen.mSelectPageIconFront);
            if (i == field_0x36f) {
                field_0x238[i]->setAlpha(0xff);
            } else {
                field_0x238[i]->setAlpha(g_drawHIO.mLetterSelectScreen.mUnselectPageIconAlpha);
            }
        }
        field_0x318[0].set(g_drawHIO.mLetterSelectScreen.mSelectBarBack);
        field_0x338[0].set(g_drawHIO.mLetterSelectScreen.mSelectBarFront);
        field_0x318[3].set(g_drawHIO.mLetterSelectScreen.mSelectTextBack);
        field_0x338[3].set(g_drawHIO.mLetterSelectScreen.mSelectTextFront);
        field_0x360 = g_drawHIO.mLetterSelectScreen.mSelectBarScale;
        field_0x364 = g_drawHIO.mLetterSelectScreen.mUnselectBarScale;
        for (int i = 0; i < 6; i++) {
            if (i == mIndex) {
                ((J2DPicture*)field_0x34[i][0])->setBlackWhite(field_0x318[0], field_0x338[0]);
                field_0x124[i][1]->setBlackWhite(field_0x318[3], field_0x338[3]);
                field_0x124[i][3]->setBlackWhite(field_0x318[3], field_0x338[3]);
                mpLetterParent[i]->scale(field_0x360, field_0x360);
            } else {
                mpLetterParent[i]->scale(field_0x364, field_0x364);
            }
        }
        for (int i = 0; i < 2; i++) {
            mpTextParent[i]->paneTrans(g_drawHIO.mLetterSelectScreen.mLetterWindowPosX,
                                       g_drawHIO.mLetterSelectScreen.mLetterWindowPosY);
            if (mProcess == 4 || mProcess == 5 || mProcess == 6) {
                mpTextParent[i]->scale(g_drawHIO.mLetterSelectScreen.mLetterWindowScale,
                                       g_drawHIO.mLetterSelectScreen.mLetterWindowScale);
            }
        }
        if (((mProcess == 4) || (mProcess == 5)) ||
            (mProcess == 6))
        {
            mpBlackTex->setAlpha(g_drawHIO.mLetterSelectScreen.mWindowBGAlpha);
        }
        mpLineParent->paneTrans(g_drawHIO.mLetterSelectScreen.mLetterLinePosX,
                                g_drawHIO.mLetterSelectScreen.mLetterLinePosY);
        for (int i = 0; i < 12; i++) {
            if (field_0x25c[i] != NULL) {
                field_0x25c[i]->setAlpha(g_drawHIO.mLetterSelectScreen.mLetterLineAlpha);
            }
        }
        ((J2DTextBox*)field_0x2ec[1]->getPanePtr())->setBlackWhite(g_drawHIO.mLetterSelectScreen.mLetterTextBack, g_drawHIO.mLetterSelectScreen.mLetterTextFront);
    }
    if (g_drawHIO.mCollectScreen.mButtonDebugON != 0 || i_useHIO) {
        if (mpButtonAB[0] != NULL) {
            mpButtonAB[0]->paneTrans(g_drawHIO.mCollectScreen.mButtonAPosX, g_drawHIO.mCollectScreen.mButtonAPosY);
            mpButtonAB[0]->scale(g_drawHIO.mCollectScreen.mButtonAScale, g_drawHIO.mCollectScreen.mButtonAScale);
        }
        if (mpButtonAB[1] != NULL) {
            mpButtonAB[1]->paneTrans(g_drawHIO.mCollectScreen.mButtonBPosX, g_drawHIO.mCollectScreen.mButtonBPosY);
            mpButtonAB[1]->scale(g_drawHIO.mCollectScreen.mButtonBScale, g_drawHIO.mCollectScreen.mButtonBScale);
        }
        if (mpButtonText[0] != NULL) {
            mpButtonText[0]->paneTrans(g_drawHIO.mCollectScreen.mButtonATextPosX, g_drawHIO.mCollectScreen.mButtonATextPosY);
            mpButtonText[0]->scale(g_drawHIO.mCollectScreen.mButtonATextScale, g_drawHIO.mCollectScreen.mButtonATextScale);
        }
        if (mpButtonText[1] != NULL) {
            mpButtonText[1]->paneTrans(g_drawHIO.mCollectScreen.mButtonBTextPosX, g_drawHIO.mCollectScreen.mButtonBTextPosY);
            mpButtonText[1]->scale(g_drawHIO.mCollectScreen.mButtonBTextScale, g_drawHIO.mCollectScreen.mButtonBTextScale);
        }
    }
}
