#ifndef D_MENU_D_MENU_STAMP_H
#define D_MENU_D_MENU_STAMP_H

#include "d/d_drawlist.h"

class CPaneMgr;
struct CSTControl;
class J2DPane;
class J2DPicture;
class J2DScreen;
class J2DTextBox;
class JKRArchive;
class JKRExpHeap;
class STControl;
class mDoDvdThd_mountArchive_c;
class dMsgString_c;
class dSelect_cursor_c;

// TPHD (Wii U) stamp album, ported from the RPX dMenu_Stamp_c (ctor 0x02a11898, vtable 0x10099060,
// methods 0x02a11b70 .. 0x02a15000). Layout: /res/LayoutRevo/stampResR.arc (background, menu,
// buttons and message screens). 5 pages of 2 x 5 stamps (50 stamps). The Miiverse "Post" cell
// (third row of the Wii U grid) is not ported.
class dMenu_Stamp_c : public dDlst_base_c {
public:
    enum {
        MODE_NORMAL = 0,    // from the collection screen
        MODE_FILESEL = 1,   // from the file select details ("+" on the stamp box), Wii U mode 1
        MODE_VIEW = 2,      // Wii U: shown after getting a stamp, any button closes it
        MODE_COMPLETE = 3,  // Wii U: 49 stamps, unlock animation of the last one
    };

    dMenu_Stamp_c(JKRExpHeap* i_heap, STControl* i_stick, CSTControl* i_cstick, u8 i_mode = 0);
    virtual ~dMenu_Stamp_c();
    virtual void draw() { _draw(); }

    int _open();
    int _close();
    void _move();
    void _draw();
    bool isSync();
    u8 getStatus() { return mStatus; }
    // RPX dMenu_Stamp_c +0x3e0/+0x3e4: when non zero, the album shows these flags (the stamps of
    // the file selected in the file select) instead of the ones of the loaded save.
    void setOverrideFlags(u64 i_flags) { mOverrideFlags = i_flags; }

    static int getStampNum();
    static const int STAMP_MAX = 50;
    static const int PAGE_NUM = 5;

private:
    void _create();
    void screenInitBg();
    void screenInitMenu();
    void screenInitButtons();
    void screenInitMsg();
    void initCursor();
    void refresh();
    void setInfoText();
    void setPageText();
    void setStampPanes();
    void setButtonText();
    void stateInit();
    void moveNormal();
    void moveUnlock();
    void moveMessage();
    bool isOwned(int i_no) const;
    int cellStamp(int i_row, int i_col) const;

    JKRExpHeap* mpHeap;
    JKRArchive* mpArchive;
    mDoDvdThd_mountArchive_c* mpMount;
    STControl* mpStick;
    CSTControl* mpCStick;
    J2DScreen* mpBgScreen;
    J2DScreen* mpMenuScreen;
    J2DScreen* mpButtonScreen;
    J2DScreen* mpMsgScreen;
    J2DPicture* mpBlackTex;
    dMsgString_c* mpString;
    dSelect_cursor_c* mpDrawCursor;
    CPaneMgr* mpCell[10];       // N_it0000 .. N_it0009
    J2DPane* mpStamp[STAMP_MAX];  // it_s00PC (page P, cell C)
    J2DPane* mpEmpty[10];       // it_e01 .. it_e10
    J2DPane* mpBgNormal[10];    // it_i000C
    J2DPane* mpBgSelect[10];    // second it_i pane of each cell
    J2DTextBox* mpInfoText;
    J2DTextBox* mpPageText;
    J2DTextBox* mpTitleText;
    J2DTextBox* mpMsgText;
    J2DPane* mpMsgAll;
    int mRow;
    int mCol;
    int mPage;
    int mState;  // RPX +0x448: 0 browse, 1 unlock animation, 2 message, (3 Miiverse post)
    int mFrame;  // RPX +0x3e8
    u8 mMode;    // RPX +0x44c
    u8 mStatus;  // 1 opening, 2 moving, 3 closing, 0 closed
    u64 mOverrideFlags;
};

#endif /* D_MENU_D_MENU_STAMP_H */
