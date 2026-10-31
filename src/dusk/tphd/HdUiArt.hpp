#pragma once

// TPHD (Wii U) button art for GC-layout screens.
//
// The GC layouts draw their button prompts with GC textures (kidney-shaped Y, X, L/R
// shoulder halves, the 3D control stick...). The Wii U art (tr_*_btn_00, tr_lstick_00...)
// lives in the Revo HUD archive main2DR.arc, mounted by d_s_logo into
// g_tphdMain2DRevoArchive. These helpers find pictures by the texture they currently show
// and swap it for the Wii U one, or hide the GC letter overlays the Wii U art already
// contains.

#include "JSystem/J2DGraph/J2DPane.h"
#include "JSystem/J2DGraph/J2DPicture.h"
#include "JSystem/JKernel/JKRArchive.h"
#include "JSystem/JUtility/JUTTexture.h"
#include "dusk/logging.h"

extern JKRArchive* g_tphdMain2DRevoArchive;

namespace dusk::tphd {

enum class HdArtOp : u8 {
    Swap,  // replace by the Wii U texture, square, unrotated and untinted
    Hide,  // GC letter overlay: the Wii U art already has the letter
};

struct HdArtRule {
    const char* gcName;  // texture name in the screen's own archive
    const char* hdName;  // texture name in main2DR.arc (unused for Hide)
    HdArtOp op;
};

inline const ResTIMG* hdRevoTexture(const char* name) {
    if (g_tphdMain2DRevoArchive == NULL || name == NULL) {
        return NULL;
    }
    return static_cast<const ResTIMG*>(g_tphdMain2DRevoArchive->getResource('TIMG', name));
}

// Makes a picture square (side = shorter edge), keeps its center and removes rotation.
inline void hdSquarePane(J2DPane* p) {
    const f32 w = p->getWidth();
    const f32 h = p->getHeight();
    const f32 s = w < h ? w : h;
    const JGeometry::TBox2<f32> before = p->getBounds();
    p->resize(s, s);
    const JGeometry::TBox2<f32> after = p->getBounds();
    p->add((before.i.x + before.f.x - after.i.x - after.f.x) * 0.5f,
           (before.i.y + before.f.y - after.i.y - after.f.y) * 0.5f);
    p->rotate(0.0f);
}

inline void hdSwapPicture(J2DPicture* pic, const ResTIMG* hd) {
    pic->changeTexture(hd, 0);
    pic->setBlackWhite(JUtility::TColor(0, 0, 0, 0), JUtility::TColor(255, 255, 255, 255));
    pic->setCornerColor(JUtility::TColor(255, 255, 255, 255));
    hdSquarePane(pic);
}

struct HdArtResolved {
    const ResTIMG* gc;
    const ResTIMG* hd;
    HdArtOp op;
};

inline void hdApplyArtRecursive(J2DPane* pane, const HdArtResolved* rules, int n) {
    if (pane->getTypeID() == 18) {
        J2DPicture* pic = static_cast<J2DPicture*>(pane);
        JUTTexture* tex = pic->getTextureCount() != 0 ? pic->getTexture(0) : NULL;
        const ResTIMG* cur = tex != NULL ? tex->getTexInfo() : NULL;
        if (cur != NULL) {
            for (int i = 0; i < n; i++) {
                if (rules[i].gc != cur) {
                    continue;
                }
                if (rules[i].op == HdArtOp::Hide) {
                    pane->hide();
                } else if (rules[i].hd != NULL) {
                    hdSwapPicture(pic, rules[i].hd);
                }
                break;
            }
        }
    }
    for (JSUTreeIterator<J2DPane> it = pane->getFirstChild(); it != pane->getEndChild(); ++it) {
        hdApplyArtRecursive(it.getObject(), rules, n);
    }
}

// Safe to call every frame: swapped pictures no longer match (their texture changed) and
// hidden overlays are simply hidden again if the game showed them.
inline void hdApplyButtonArt(J2DPane* root, JKRArchive* gcArc, const HdArtRule* rules, int n) {
    if (root == NULL || gcArc == NULL || g_tphdMain2DRevoArchive == NULL) {
        return;
    }
    HdArtResolved resolved[32];
    int count = 0;
    for (int i = 0; i < n && count < 32; i++) {
        const ResTIMG* gc = static_cast<const ResTIMG*>(gcArc->getResource('TIMG', rules[i].gcName));
        if (gc == NULL) {
            continue;
        }
        resolved[count].gc = gc;
        resolved[count].hd = rules[i].op == HdArtOp::Swap ? hdRevoTexture(rules[i].hdName) : NULL;
        resolved[count].op = rules[i].op;
        if (rules[i].op == HdArtOp::Swap && resolved[count].hd == NULL) {
            continue;
        }
        count++;
    }
    if (count != 0) {
        hdApplyArtRecursive(root, resolved, count);
    }
}

// Button prompts of the GC item ring / collection screens.
inline const HdArtRule kHdRingButtonRules[] = {
    {"tt_zelda_button_y_base.bti", "tr_y_btn_00.bti", HdArtOp::Swap},
    {"tt_zelda_button_y.bti", "tr_y_btn_00.bti", HdArtOp::Swap},
    {"tt_zelda_button_y_text.bti", NULL, HdArtOp::Hide},
    {"tt_zelda_button_x_base.bti", "tr_x_btn_00.bti", HdArtOp::Swap},
    {"tt_zelda_button_x.bti", "tr_x_btn_00.bti", HdArtOp::Swap},
    {"tt_zelda_button_x_text.bti", NULL, HdArtOp::Hide},
    {"tt_zelda_button_l_base.bti", "tr_l_btn_00.bti", HdArtOp::Swap},
    {"tt_zelda_button_l_text.bti", NULL, HdArtOp::Hide},
    {"tt_zelda_button_r_base.bti", "tr_r_btn_00.bti", HdArtOp::Swap},
    {"tt_zelda_button_r_text.bti", NULL, HdArtOp::Hide},
    {"tt_zelda_button_a_8ia.bti", "tr_a_btn_00.bti", HdArtOp::Swap},
    {"tt_zelda_button_a_text.bti", NULL, HdArtOp::Hide},
    {"tt_zelda_button_b_text.bti", NULL, HdArtOp::Hide},
    {"tt_ia4_3d_btn.bti", "tr_lstick_00.bti", HdArtOp::Swap},
};
inline constexpr int kHdRingButtonRuleCount = sizeof(kHdRingButtonRules) / sizeof(kHdRingButtonRules[0]);

}  // namespace dusk::tphd
