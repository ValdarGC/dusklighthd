/**
 * @file d_menu_ring_hd.cpp
 * @brief TPHD (Wii U, Pro Controller) item screen used in place of the GameCube item wheel.
 *
 * The Wii U game replaces dMenu_Ring_c's wheel with a "ZeldaProItems" screen built from
 * /res/LayoutRevo/ringresR.arc -> zelda_items_procon.blo (TPHD RPX: ctor 0x02b02af4, base class
 * ZeldaItemScreen 0x02afe348). It is a 7 x 3 grid of fixed cells (N_item01..21 / bg01..21), a
 * cursor frame (FrameSel), the item name (txtNameT + shadow txtNameB) and a "Set item  Y X R"
 * caption. The GC wheel logic (item list, X / Y / R assignment, swaps, combos, explain window)
 * is kept: this file only lays the items out in the Wii U grid, moves the cursor on it and draws.
 *
 * Grid filling = ZeldaItemScreen::refresh (0x02affcb4 -> 0x02aff334): each group of item slots is
 * packed, in order, into its own list of cells, skipping slots that hold no item.
 */

#include "d/dolzel.h" // IWYU pragma: keep

#if TARGET_PC

#include "d/d_menu_ring.h"
#include "JSystem/J2DGraph/J2DScreen.h"
#include "JSystem/J2DGraph/J2DTextBox.h"
#include "JSystem/J2DGraph/J2DPicture.h"
#include "JSystem/J2DGraph/J2DMaterial.h"
#include "JSystem/JUtility/JUTTexture.h"
#include "JSystem/JKernel/JKRArchive.h"
#include "JSystem/JKernel/JKRExpHeap.h"
#include "d/d_com_inf_game.h"
#include "d/d_kantera_icon_meter.h"
#include "d/d_lib.h"
#include "d/d_meter2.h"
#include "d/d_meter2_draw.h"
#include "d/d_meter2_info.h"
#include "d/d_meter_HIO.h"
#include "dusk/game_clock.h"
#include <cmath>
#include "d/d_select_cursor.h"
#include "d/d_menu_item_explain.h"
#include "m_Do/m_Do_controller_pad.h"
#include "m_Do/m_Do_ext.h"
#include "m_Do/m_Do_graphic.h"
#include "dusk/logging.h"
#include "dusk/settings.h"
#include "Z2AudioLib/Z2AudioMgr.h"
#include <cstdio>
#include <cstring>
#include <filesystem>
#include "dusk/main.h"

extern JKRArchive* g_tphdRingRevoArchive;
extern JKRArchive* g_tphdFishRevoArchive;
extern JKRArchive* g_tphdMain2DRevoArchive;
// Read by dMeter2Draw_c::drawButtonB: while the Wii U item screen is open the B button keeps its
// sword icon and shows no "Back" text (reference capture).
bool g_tphdItemScreenOpen = false;
// Read by dMeter2_c::_draw: the HUD is not drawn over the Wii U item explain window.
bool g_tphdItemExplainOpen = false;

namespace {

constexpr int kCells = 21;
constexpr f32 kScreenW = 810.0f;  // INF1 of zelda_items_procon.blo

// ZeldaItemScreen::refresh tables (TPHD RPX 0x100a5e84..0x100a5eb3): item slots of a group and
// the cells they are packed into.
struct HdGroup {
    u8 slots[11];
    u8 nSlots;
    u8 cells[10];
    u8 nCells;
};
const HdGroup kGroups[] = {
    // fishing rod, slingshot, lantern, boomerang, iron boots, bow, clawshot, double clawshot,
    // spinner, ball and chain, dominion rod
    {{20, 23, 1, 0, 3, 4, 9, 10, 2, 6, 8}, 11, {0, 1, 2, 3, 7, 8, 9, 10, 14, 15}, 10},
    {{7}, 1, {16}, 1},
    {{5}, 1, {17}, 1},                      // hawkeye
    {{15, 16, 17}, 3, {4, 11, 18}, 3},      // bomb bags
    {{11, 12, 13, 14}, 4, {5, 6, 12, 13}, 4},  // bottles
    {{21}, 1, {19}, 1},                     // horse call
    {{19, 18, 22}, 3, {20}, 1},
};

u64 tag8(const char* s) {
    u64 v = 0;
    for (int i = 0; i < 8 && s[i] != 0; i++) {
        v = (v << 8) | static_cast<u8>(s[i]);
    }
    return v;
}

J2DPane* findCell(J2DScreen* scr, const char* fmt, int cell) {
    char name[12];
    std::snprintf(name, sizeof(name), fmt, cell + 1);
    return scr->search(tag8(name));
}

// FNT1 of zelda_items_procon.blo: 0 = reishotai_24_22.bfn (the game's "sub" font, used for the
// item name), 1 = rodan_b_24_22.bfn (message font).
void fixFonts(J2DPane* pane) {
    for (J2DPane* c = pane->getFirstChildPane(); c != NULL; c = c->getNextChildPane()) {
        if (c->getTypeID() == 19) {
            J2DTextBox* tb = static_cast<J2DTextBox*>(c);
            J2DMaterial* m = tb->getMaterial();
            const u16 fontNo = (m != NULL && m->getTevBlock() != NULL) ? m->getTevBlock()->getFontNo() : 1;
            tb->setFont(fontNo == 0 ? mDoExt_getSubFont() : mDoExt_getMesgFont());
            tb->setString(0x100, "");
        }
        fixFonts(c);
    }
}

// Frame textures (ZeldaItemScreen ctor 0x02afe348 / frame update 0x02aff960): a cell whose item
// is set on X / Y / R uses the "selected" frame (texture of FrameSel, FrameSeC for the round
// cell 21), the others the normal one (texture of Frame01, Frame21).
const ResTIMG* s_frameTex[4];  // normal, set, normal round, set round
J2DPane* s_cursorCell = NULL;
// Wii U cursor: the same dSelect_cursor_c 4-corner cursor as the wheel, but built from the TPHD
// main2DR.arc (zelda_select_cursor_4parts.blo with its HD textures), like the RPX does.
dSelect_cursor_c* s_hdCursor = NULL;

const ResTIMG* paneTex(J2DScreen* scr, const char* tag) {
    J2DPane* p = scr->search(tag8(tag));
    if (p == NULL || p->getTypeID() != 18) {
        return NULL;
    }
    J2DMaterial* m = static_cast<J2DPicture*>(p)->getMaterial();
    if (m == NULL || m->getTevBlock() == NULL || m->getTevBlock()->getTexture(0) == NULL) {
        return NULL;
    }
    return m->getTevBlock()->getTexture(0)->getTexInfo();
}

void setText(J2DScreen* scr, const char* tag, const char* str) {
    J2DPane* p = scr->search(tag8(tag));
    if (p != NULL && p->getTypeID() == 19) {
        J2DTextBox* tb = static_cast<J2DTextBox*>(p);
        if (std::strcmp(tb->getStringPtr(), str) != 0) {
            std::strncpy(tb->getStringPtr(), str, 0xff);
            tb->getStringPtr()[0xff] = 0;
        }
    }
}

void setMsg(J2DScreen* scr, const char* tag, u32 msgId) {
    J2DPane* p = scr->search(tag8(tag));
    if (p != NULL && p->getTypeID() == 19) {
        dMeter2Info_getString(msgId, static_cast<J2DTextBox*>(p)->getStringPtr(), NULL);
    }
}

// Pictures whose textures live in archives that are not mounted (title_backdrop.bti is in
// clctresR / fishresR / letresR) get a NULL texture from the material factory, which the
// renderer cannot create. Hide them.
void hideMissingTex(J2DPane* pane) {
    for (J2DPane* c = pane->getFirstChildPane(); c != NULL; c = c->getNextChildPane()) {
        if (c->getTypeID() == 18) {
            J2DMaterial* m = static_cast<J2DPicture*>(c)->getMaterial();
            if (m != NULL && m->getTevBlock() != NULL) {
                const u32 n = m->getTexGenBlock()->getTexGenNum();
                for (u32 i = 0; i < n && i < 8; i++) {
                    JUTTexture* t = m->getTevBlock()->getTexture(i);
                    const ResTIMG* ti = t != NULL ? t->getTexInfo() : NULL;
                    // Files stored Yaz0-compressed inside ringresR.arc (block8x8.bti...) come
                    // back undecoded from the memory archive: "Yaz0" read as a BTI header.
                    if (ti == NULL || ti->format > 14 || ti->width == 0 || ti->height == 0) {
                        char name[9] = {};
                        int nn = 0;
                        for (int k = 0; k < 8; k++) {
                            const char ch = static_cast<char>((c->mInfoTag >> (56 - 8 * k)) & 0xff);
                            if (ch != 0) {
                                name[nn++] = ch;
                            }
                        }
                        DuskLog.warn("HDITEM: pane '{}' has no texture {}, hidden", name, i);
                        c->hide();
                        break;
                    }
                }
            }
        }
        hideMissingTex(c);
    }
}

// ---- configmenu.txt ------------------------------------------------------------------------
// Quick tuning of the Wii U item screen. One line per entry: "name a b c" (# = comment). The file
// is in the Dusklight config folder and is re-read automatically when it changes.
//   icon_size    s          item icon size multiplier (1 = default)
//   cursor_size  sx sy      yellow corner cursor stretch (1 1 = default)
//   cursor_pos   dx dy      cursor offset in layout units (+x right, +y down)
//   count_pos    dx dy      ALL ammo counters, offset from their default corner
//   count_size   s          ALL ammo counters, digit size multiplier (1 = default)
// Defaults = values tuned in-game against TPHD captures.
struct MenuCfg {
    f32 icon = 1.2225f;
    // Cursor: measured on the 1080p captures (corners 165 x 162 px around the cell, centred on its
    // frame); with 1 1 / 0 0 ours spanned 151 x 159 px, 3.5 px right and down.
    f32 curSx = 1.09f, curSy = 1.02f;
    f32 curDx = -1.6f, curDy = -1.0f;
    f32 cntDx = -1.4f, cntDy = -1.3f;
    f32 cntSize = 1.1f;
} g_menuCfg;

void menuCfgLoad() {
    static std::filesystem::file_time_type s_stamp{};
    static bool s_init = false;
    static int s_tick = 0;
    if (s_init && (++s_tick % 30) != 0) {
        return;
    }
    const std::filesystem::path path = dusk::ConfigPath / "configmenu.txt";
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        if (FILE* f = std::fopen(path.string().c_str(), "w")) {
            std::fputs("# configmenu: name a b   (re-read automatically while the game runs)\n"
                       "# icon_size   s       item icon size multiplier\n"
                       "# cursor_size sx sy   yellow corners stretch\n"
                       "# cursor_pos  dx dy   cursor offset (layout units, +x right, +y down)\n"
                       "# count_pos   dx dy   all ammo counters offset\n"
                       "# count_size  s       all ammo counters size multiplier\n"
                       "icon_size 1.2225\n"
                       "cursor_size 1.09 1.02\n"
                       "cursor_pos -1.6 -1.0\n"
                       "count_pos -1.4 -1.3\n"
                       "count_size 1.1\n", f);
            std::fclose(f);
        }
    }
    const auto stamp = std::filesystem::last_write_time(path, ec);
    if (ec || (s_init && stamp == s_stamp)) {
        s_init = true;
        return;
    }
    s_init = true;
    s_stamp = stamp;
    if (FILE* f = std::fopen(path.string().c_str(), "r")) {
        g_menuCfg = MenuCfg();
        char line[256];
        while (std::fgets(line, sizeof(line), f)) {
            char name[64];
            float a = 0, b = 0;
            if (line[0] == '#' || std::sscanf(line, "%63s %f %f", name, &a, &b) < 2) {
                continue;
            }
            if (!std::strcmp(name, "icon_size")) {
                g_menuCfg.icon = a;
            } else if (!std::strcmp(name, "cursor_size")) {
                g_menuCfg.curSx = a;
                g_menuCfg.curSy = b;
            } else if (!std::strcmp(name, "cursor_pos")) {
                g_menuCfg.curDx = a;
                g_menuCfg.curDy = b;
            } else if (!std::strcmp(name, "count_pos")) {
                g_menuCfg.cntDx = a;
                g_menuCfg.cntDy = b;
            } else if (!std::strcmp(name, "count_size")) {
                g_menuCfg.cntSize = a;
            }
        }
        std::fclose(f);
    }
}

// ZL combo glow (ZeldaItemScreen 0x02afe174 / draw 0x02b01150): frame of a type 2 drawPikari that
// keeps looping (18..28, Z2SE_SY_ITEM_COMBINE_ICON at 18) while the cursor item can be combined.
f32 s_comboPikari = 0.0f;

// Item fly (ZeldaProItems::assign 0x02b02fe0, update 0x02b02bfc, draw 0x02b02d2c): the icon of the
// cell under the cursor moves at 40 units a frame to the HUD slot of the button (N_VSlt01..03),
// shrinking from the cell size to the slot size; the assignment is applied when it arrives.
struct HdFly {
    int btn = -1;  // select-item index (0 X, 1 Y, 3 R)
    u8 slot = 0xFF;
    int ri = -1;   // wheel index of the item (its textures)
    f32 x = 0.0f, y = 0.0f;  // current centre
    f32 scale = 1.0f;        // 2 (cell 64 / slot 32) -> 1
    f32 acc = 0.0f;
};
HdFly s_fly;
constexpr f32 kFlySpeed = 40.0f;  // RPX 0x100a5f84

// N_VSlt01 / 02 / 03 sit on the HUD item icons; matched by position to the Wii U HUD badges:
// itmBtn_1 (X) <- VSlt01, itmBtn_4 (R) <- VSlt02, itmBtn_2 (Y) <- VSlt03.
const char* flySlotTag(int btn) {
    switch (btn) {
    case 0: return "N_VSlt01";
    case 1: return "N_VSlt03";
    case 3: return "N_VSlt02";
    default: return NULL;
    }
}

Vec centerOf(J2DPane* p) {
    const Vec a = p->getGlbVtx(0);
    const Vec b = p->getGlbVtx(3);
    return Vec{(a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f, 0.0f};
}

}  // namespace

bool dMenu_Ring_c::hdActive() const {
    return mpHdScreen != NULL;
}

void dMenu_Ring_c::hdCreate() {
    mpHdScreen = NULL;
    mHdCursor = 0;
    mHdEmpty = false;
    std::memset(mHdCellSlot, 0xFF, sizeof(mHdCellSlot));
    if (!dusk::tphd_active() || g_tphdRingRevoArchive == NULL) {
        return;
    }
    // The wheel's own heap is sized for the GC wheel only; the Wii U screen (~200 panes and
    // materials) lives on the J2D heap (+16 MB for TPHD UI).
    JKRExpHeap* uiHeap = mDoExt_getJ2dHeap();
    JKRHeap* oldHeap = mDoExt_setCurrentHeap(uiHeap);
    J2DScreen* scr = JKR_NEW J2DScreen();
    if (!scr->setPriority("zelda_items_procon.blo", 0x20000, g_tphdRingRevoArchive)) {
        DuskLog.warn("HDITEM: zelda_items_procon.blo failed to load, keeping the GC item wheel");
        JKR_DELETE(scr);
        mDoExt_setCurrentHeap(oldHeap);
        return;
    }
    mpHdScreen = scr;
    fixFonts(scr);
    // Item-name backdrop (m / m1 / m2 / m3 use title_backdrop.bti, which on Wii U comes from the
    // collection screen archive; fishresR.arc holds the same texture).
    if (g_tphdFishRevoArchive != NULL) {
        const ResTIMG* bd =
            (const ResTIMG*)g_tphdFishRevoArchive->getResource('TIMG', "title_backdrop.bti");
        if (bd != NULL) {
            static const char* const kBd[] = {"m", "m1", "m2", "m3"};
            for (const char* t : kBd) {
                J2DPane* p = scr->search(tag8(t));
                if (p == NULL || p->getTypeID() != 18) {
                    continue;
                }
                J2DMaterial* m = static_cast<J2DPicture*>(p)->getMaterial();
                if (m == NULL || m->getTevBlock() == NULL) {
                    continue;
                }
                JUTTexture* tex = m->getTevBlock()->getTexture(0);
                if (tex != NULL) {
                    tex->storeTIMG(bd, (u8)0);
                } else {
                    m->getTevBlock()->setTexture(0, bd);
                }
            }
        }
    }
    hideMissingTex(scr);
    if (g_tphdMain2DRevoArchive != NULL) {
        s_hdCursor = JKR_NEW dSelect_cursor_c(0, 1.0f, g_tphdMain2DRevoArchive);
        s_hdCursor->setAlphaRate(1.0f);
    }
    mDoExt_setCurrentHeap(oldHeap);
    setMsg(scr, "Set_item", 0x380);  // "Set item"
    setMsg(scr, "infoTxt", 0x5d);    // "Info"
    s_frameTex[0] = paneTex(scr, "Frame01");
    s_frameTex[1] = paneTex(scr, "FrameSel");
    s_frameTex[2] = paneTex(scr, "Frame21");
    s_frameTex[3] = paneTex(scr, "FrameSeC");
    // Texture holders and pieces the Wii U screen drives itself that are not handled yet.
    static const char* const kOff[] = {"FrameSel", "FrameSeC", "invldBtn"};
    for (const char* t : kOff) {
        if (J2DPane* p = scr->search(tag8(t))) {
            p->hide();
        }
    }
    hdRefresh();
    for (int c = 0; c < kCells; c++) {
        if (mHdCellSlot[c] != 0xFF) {
        }
    }
    for (int s : {18, 19, 22}) {
    }
    // Start on the cell of the wheel's current item.
    const u8 cur = mItemSlots[mCurrentSlot];
    for (int c = 0; c < kCells; c++) {
        if (mHdCellSlot[c] != 0xFF && mHdCellSlot[c] == cur) {
            mHdCursor = c;
            break;
        }
    }
    hdSyncCursor();
    g_tphdItemScreenOpen = true;
}

void dMenu_Ring_c::hdDelete() {
    if (mpHdScreen != NULL) {
        JKR_DELETE(mpHdScreen);
        mpHdScreen = NULL;
    }
    if (s_hdCursor != NULL) {
        JKR_DELETE(s_hdCursor);
        s_hdCursor = NULL;
    }
    g_tphdItemScreenOpen = false;
    g_tphdItemExplainOpen = false;
    s_fly = HdFly();
    s_comboPikari = 0.0f;
}

void dMenu_Ring_c::hdRefresh() {
    std::memset(mHdCellSlot, 0xFF, sizeof(mHdCellSlot));
    for (const HdGroup& g : kGroups) {
        int cell = 0;
        for (int i = 0; i < g.nSlots && cell < g.nCells; i++) {
            const u8 slot = g.slots[i];
            if (dComIfGs_getItem(slot, false) != dItemNo_NONE_e) {
                mHdCellSlot[g.cells[cell++]] = slot;
            }
        }
    }
}

// Puts the wheel's cursor (mCurrentSlot) on the item of the grid cell under the HD cursor, so the
// GC logic (X / Y / R assignment, explain window, combos, name) works on that item.
void dMenu_Ring_c::hdSyncCursor() {
    const u8 slot = mHdCellSlot[mHdCursor];
    mHdEmpty = true;
    if (slot == 0xFF) {
        return;
    }
    for (int i = 0; i < mItemsTotal; i++) {
        if (mItemSlots[i] == slot) {
            mCurrentSlot = i;
            mHdEmpty = false;
            return;
        }
    }
}

void dMenu_Ring_c::hdMove() {
    if (mStatus != STATUS_WAIT && mStatus != STATUS_MOVE) {
        return;
    }
    mStatus = STATUS_WAIT;
    mpStick->checkTrigger();
    int c = mHdCursor;
    const int col = c % 7;
    // STControl also reads the d-pad on PC (the screen opens / closes with START in TPHD).
    if (mpStick->checkUpTrigger()) {
        if (c >= 7) {
            c -= 7;
        }
    } else if (mpStick->checkDownTrigger()) {
        if (c + 7 < kCells) {
            c += 7;
        }
    } else if (mpStick->checkLeftTrigger() || mDoCPd_c::getTrigLeft(PAD_1)) {
        if (col > 0) {
            c -= 1;
        }
    } else if (mpStick->checkRightTrigger() || mDoCPd_c::getTrigRight(PAD_1)) {
        if (col < 6) {
            c += 1;
        }
    }
    if (c != mHdCursor) {
        mHdCursor = c;
        Z2GetAudioMgr()->seStart(Z2SE_ITEM_RING_ROLL, NULL, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
    }
    hdSyncCursor();
}

void dMenu_Ring_c::hdDraw() {
    J2DScreen* scr = mpHdScreen;
    menuCfgLoad();
    // X / Y / ZL changes: the GC wheel applies them at the end of its own slide (drawSelectItem,
    // GC draw path only). The Wii U flies the icon of the cursor cell to the button first when the
    // item set is the one under the cursor (0x02b030f4), otherwise it applies the change at once.
    if ((s_fly.btn == 0 || s_fly.btn == 1) && field_0x674[s_fly.btn] == 0) {
        s_fly.btn = -1;  // applied elsewhere (another button press, closing)
    }
    for (int i = 0; i < 2; i++) {
        if (field_0x674[i] == 0 || s_fly.btn == i) {
            continue;
        }
        const u8 cur = mHdEmpty ? 0xFF : mItemSlots[mCurrentSlot];
        if (field_0x6b4[i] != cur || !hdStartFly(i, cur)) {
            setSelectItemForce(i);
        }
    }
    setSelectItemForce(2);
    g_tphdItemExplainOpen = mpItemExplain->getStatus() != 0;
    J2DGrafContext* graf = dComIfGp_getCurrentGrafPort();
    const f32 minX = mDoGph_gInf_c::getMinXF();
    const f32 maxX = mDoGph_gInf_c::getMaxXF();
    scr->scale((maxX - minX) / kScreenW, 1.0f);

    if (J2DPane* all = scr->search(tag8("n_all"))) {
        all->setAlpha(static_cast<u8>(255.0f * mAlphaRate));
    }
    // Cell frames: "set" look for the items on X / Y / R.
    for (int c = 0; c < kCells; c++) {
        J2DPane* fr = findCell(scr, "Frame%02d", c);
        if (fr == NULL || fr->getTypeID() != 18) {
            continue;
        }
        const u8 slot = mHdCellSlot[c];
        bool set = false;
        if (slot != 0xFF) {
            for (int i = 0; i < 2; i++) {
                set |= dComIfGs_getSelectItemIndex(i) == slot || dComIfGs_getMixItemIndex(i) == slot;
            }
            set |= dComIfGs_getSelectItemIndex(3) == slot || dComIfGs_getMixItemIndex(3) == slot;
        }
        const ResTIMG* t = s_frameTex[(c == 20 ? 2 : 0) + (set ? 1 : 0)];
        if (t != NULL) {
            static_cast<J2DPicture*>(fr)->changeTexture(t, 0);
        }
    }
    // The yellow 4-corner cursor (dSelect_cursor_c, as on Wii U) goes on the selected cell; it is
    // drawn in the second pass (hdDrawCursor).
    s_cursorCell = findCell(scr, "bg%02d", mHdCursor);
    // Item name (the wheel keeps it up to date in setScale / setNameString).
    char name[0x100] = "";
    if (!mHdEmpty) {
        J2DTextBox* src = static_cast<J2DTextBox*>(mpCenterScreen->search(MULTI_CHAR('fitem_n1')));
        if (src != NULL && src->getStringPtr() != NULL) {
            std::strncpy(name, src->getStringPtr(), sizeof(name) - 1);
        }
    }
    setText(scr, "txtNameT", name);
    setText(scr, "txtNameB", name);

    // ZL combo (ZeldaItemScreen 0x02afe174 / 0x02afdbb8): the button is shown in the disabled look
    // unless the item under the cursor can be combined with the bow on X / Y (bombs, Hawkeye),
    // or the combo of that button can be turned off; the caption says which. Info (A) is
    // disabled on an empty cell.
    {
        u32 comboMsg = 0;
        if (!mHdEmpty && !mPlayerIsWolf) {
            if (isMixItemOff()) {
                comboMsg = 0x4d3;  // "Combo OFF"
            } else if (isMixItemOn()) {
                comboMsg = 0x4d2;  // "Bow & ... Combo"
            }
        }
        static u32 s_comboMsg = 0xFFFFFFFF;
        J2DPane* ct = scr->search(tag8("comboTxt"));
        if (ct != NULL) {
            if (comboMsg != 0 && comboMsg != s_comboMsg) {
                setMsg(scr, "comboTxt", comboMsg);
            }
            ct->show();
            if (comboMsg == 0) {
                ct->hide();
            }
        }
        s_comboMsg = comboMsg;
        if (comboMsg != 0) {
            if (s_comboPikari <= 0.0f) {
                s_comboPikari = 18.0f - g_ringHIO.mPikariAnimSpeed;
            }
        } else {
            s_comboPikari = 0.0f;
        }
        const u8 dimA = static_cast<u8>(255.0f * 0.4f);
        if (J2DPane* cb = scr->search(tag8("comboBtn"))) {
            cb->setAlpha(comboMsg != 0 ? 255 : dimA);
        }
        for (const char* t : {"infoBtn", "infoTxt"}) {
            if (J2DPane* p = scr->search(tag8(t))) {
                p->setAlpha(!mHdEmpty ? 255 : dimA);
            }
        }
    }

    scr->draw(minX, 0.0f, graf);

    // Item icons, ammo counters and lantern oil, drawn with the wheel's own textures.
    for (int c = 0; c < kCells; c++) {
        const u8 slot = mHdCellSlot[c];
        J2DPane* cell = findCell(scr, "N_item%02d", c);
        if (slot == 0xFF || cell == NULL) {
            continue;
        }
        int ri = -1;
        for (int i = 0; i < mItemsTotal; i++) {
            if (mItemSlots[i] == slot) {
                ri = i;
                break;
            }
        }
        if (ri < 0) {
            continue;
        }
        const Vec ctr = centerOf(cell);
        // The wheel enlarges its selected item (setItemScale); the Wii U grid does not.
        for (int j = 0; j < 3; j++) {
            if (mpItemTex[ri][j] != NULL) {
                mpItemTex[ri][j]->scale(1.0f, 1.0f);
            }
        }
        // Reference capture: icons span ~35 of the 796-unit wide screen (wheel: 48).
        const f32 w = mItemSlotParam1[ri] * 34.0f * mHdIconScale * g_menuCfg.icon;
        const f32 h = mItemSlotParam2[ri] * 34.0f * mHdIconScale * g_menuCfg.icon;
        const f32 x = ctr.x - w * 0.5f;
        const f32 y = ctr.y - h * 0.5f;
        const u8 item = dComIfGs_getItem(slot, false);
        for (int j = 0; j < 3; j++) {
            if (mpItemTex[ri][j] == NULL) {
                continue;
            }
            mpItemTex[ri][j]->setAlpha(static_cast<u8>(255.0f * mAlphaRate));
            mpItemTex[ri][j]->draw(x, y, w, h, 0, 0, 0);
            if ((j == 0 && item != dItemNo_BEE_CHILD_e) || (j == 2 && item == dItemNo_BEE_CHILD_e)) {
                const u8 num = getItemNum(slot);
                const u8 maxNum = getItemMaxNum(slot);
                if (maxNum != 0) {
                    hdDrawNumber(num, maxNum, ctr.x + 25.0f + g_menuCfg.cntDx,
                                 ctr.y + 21.0f + g_menuCfg.cntDy, 12.0f * g_menuCfg.cntSize);
                }
            }
            if (j == 0 && item == dItemNo_KANTERA_e) {
                setKanteraPos(ctr.x - 2.0f, ctr.y + 19.0f);
                mpKanteraMeter->setScale(0.72f, 0.72f);
                mpKanteraMeter->setNowGauge(dComIfGs_getMaxOil(), dComIfGs_getOil());
                mpKanteraMeter->setAlphaRate(mAlphaRate);
                mpKanteraMeter->drawSelf();
            }
        }
    }

    // Item fly: one step per game frame towards the HUD slot.
    if (s_fly.btn >= 0) {
        J2DPane* dst = scr->search(tag8(flySlotTag(s_fly.btn)));
        if (dst == NULL) {
            hdFlyFinish();
        } else {
            const Vec t = centerOf(dst);
            s_fly.acc += dusk::game_clock::original_frames();
            const f32 frame = 1.0f;  // one original (30 fps) game frame
            while (s_fly.btn >= 0 && s_fly.acc >= frame) {
                s_fly.acc -= frame;
                const f32 dx = t.x - s_fly.x;
                const f32 dy = t.y - s_fly.y;
                const f32 d = std::sqrt(dx * dx + dy * dy);
                if (d <= kFlySpeed) {
                    hdFlyFinish();
                    break;
                }
                const f32 r = kFlySpeed / d;
                s_fly.x += dx * r;
                s_fly.y += dy * r;
                s_fly.scale += (1.0f - s_fly.scale) * r;
            }
        }
    }

    // ZL glow: drawn on the combo button while the explain window is closed.
    if (s_comboPikari > 0.0f && mpItemExplain->getStatus() == 0) {
        if (J2DPane* cb = scr->search(tag8("comboBtn"))) {
            const Vec c = centerOf(cb);
            dMeter2Info_getMeterClass()->getMeterDrawPtr()->drawPikari(
                c.x, c.y, &s_comboPikari, g_ringHIO.mPikariScale, g_ringHIO.mPikariFrontOuter,
                g_ringHIO.mPikariFrontInner, g_ringHIO.mPikariBackOuter,
                g_ringHIO.mPikariBackInner, g_ringHIO.mPikariAnimSpeed, 2);
        }
    }
}

bool dMenu_Ring_c::hdStartFly(int i_btn, u8 i_slot) {
    if (!hdActive() || i_slot == 0xFF || flySlotTag(i_btn) == NULL) {
        return false;
    }
    int cell = -1;
    if (mHdCellSlot[mHdCursor] == i_slot) {
        cell = mHdCursor;
    } else {
        for (int c = 0; c < kCells; c++) {
            if (mHdCellSlot[c] == i_slot) {
                cell = c;
                break;
            }
        }
    }
    J2DPane* src = cell >= 0 ? findCell(mpHdScreen, "N_item%02d", cell) : NULL;
    if (src == NULL || mpHdScreen->search(tag8(flySlotTag(i_btn))) == NULL) {
        return false;
    }
    int ri = -1;
    for (int i = 0; i < mItemsTotal; i++) {
        if (mItemSlots[i] == i_slot) {
            ri = i;
            break;
        }
    }
    if (ri < 0) {
        return false;
    }
    if (s_fly.btn >= 0) {
        hdFlyFinish();  // 0x02b02fe0 first completes the previous fly
    }
    const Vec c = centerOf(src);
    s_fly.btn = i_btn;
    s_fly.slot = i_slot;
    s_fly.ri = ri;
    s_fly.x = c.x;
    s_fly.y = c.y;
    s_fly.scale = 2.0f;  // cell 64 / slot 32
    s_fly.acc = 0.0f;
    return true;
}

void dMenu_Ring_c::hdFlyFinish() {
    const int btn = s_fly.btn;
    s_fly.btn = -1;
    if (btn == 3) {
        hdApplyR(s_fly.slot);
    } else if (btn >= 0) {
        setSelectItemForce(btn);
    }
}

void dMenu_Ring_c::hdFlyForceR() {
    if (s_fly.btn == 3) {
        hdFlyFinish();
    }
}

void dMenu_Ring_c::hdDrawFly() {
    if (s_fly.btn < 0 || s_fly.ri < 0 || s_fly.ri >= mItemsTotal) {
        return;
    }
    const int ri = s_fly.ri;
    // Grid icon size at the start (scale 2), half of it on the slot (scale 1).
    const f32 base = 34.0f * mHdIconScale * g_menuCfg.icon * s_fly.scale * 0.5f;
    const f32 w = mItemSlotParam1[ri] * base;
    const f32 h = mItemSlotParam2[ri] * base;
    for (int j = 0; j < 3; j++) {
        if (mpItemTex[ri][j] == NULL) {
            continue;
        }
        mpItemTex[ri][j]->setAlpha(static_cast<u8>(255.0f * mAlphaRate));
        mpItemTex[ri][j]->draw(s_fly.x - w * 0.5f, s_fly.y - h * 0.5f, w, h, 0, 0, 0);
    }
}

// Ammo counter of a cell, right-aligned at (right, bottom): same colours as the wheel's
// drawNumber (gold when full, grey when empty) but at the Wii U grid size.
void dMenu_Ring_c::hdDrawNumber(int num, int maxNum, f32 right, f32 bottom, f32 size) {
    if (num > 100) {
        num = 100;
    }
    JUtility::TColor black, white;
    if (num == maxNum) {
        black.set(30, 30, 30, 0);
        white.set(255, 200, 50, 255);
    } else if (num == 0) {
        black.set(30, 30, 30, 0);
        white.set(180, 180, 180, 255);
    } else {
        black.set(0, 0, 0, 0);
        white.set(255, 255, 255, 255);
    }
    int digits[3];
    int nd = 0;
    if (num >= 100) {
        digits[nd++] = num / 100;
    }
    digits[nd++] = (num / 10) % 10;
    digits[nd++] = num % 10;
    const f32 step = size * 0.9f;
    for (int i = 0; i < nd; i++) {
        J2DPicture* p = mpItemNumTex[i];
        p->changeTexture((ResTIMG*)dComIfGp_getMain2DArchive()->getResource(
                             'TIMG', dMeter2Info_getNumberTextureName(digits[i])),
                         0);
        p->setBlackWhite(black, white);
        p->show();
        p->setAlpha(static_cast<u8>(255.0f * mAlphaRate));
        p->draw(right - step * (nd - i) - (size - step), bottom - size, size, size, 0, 0, 0);
    }
}

void dMenu_Ring_c::hdDrawCursor() {
    if (s_cursorCell == NULL || mHdEmpty && false) {
        return;
    }
    dSelect_cursor_c* cur = s_hdCursor != NULL ? s_hdCursor : mpDrawCursor;
    const Vec c = centerOf(s_cursorCell);
    // ZeldaItemScreen ctor: cursor params (0.7, 0.72, 0.03, 0.8).
    // setParam(0.7, 0.72, 0.03, 0.8, 0.8): RPX 0x02afed00..0x02afed20 (f5 = f4 = 0.8). The fifth
    // value is the Y ratio of the corner pieces; 0.5 squashed them vertically.
    cur->setParam(0.7f * g_menuCfg.curSx, 0.72f * g_menuCfg.curSy, 0.03f, 0.8f, 0.8f);
    // The pane call only sizes the corners to the cell; the second call drops the pane again,
    // otherwise update() re-reads its position through CPaneMgr::getGlobalVtxCenter, which does
    // not include the screen's draw offset (minX) and put the cursor one cell to the right.
    cur->setPos(c.x + g_menuCfg.curDx, c.y + g_menuCfg.curDy, s_cursorCell, true);
    cur->setPos(c.x + g_menuCfg.curDx, c.y + g_menuCfg.curDy, NULL, false);
    cur->setAlphaRate(mAlphaRate);
    cur->draw();
    hdDrawFly();  // ZeldaProItems draw 0x02b02d2c: after the base screen and its cursor
}

#endif  // TARGET_PC
