#pragma once

#include <mods/api.h>

#ifdef __cplusplus
#include <mods/service.hpp>
#endif

#define TPHD_SERVICE_ID DUSKLIGHT_SERVICE_ID_PREFIX "tphd"
#define TPHD_SERVICE_MAJOR 1u
#define TPHD_SERVICE_MINOR 0u

/*
 * TPHD mode: the game runs with the Wii U (Twilight Princess HD) content folder, its HD textures
 * and its Revo (Wii U TV) layouts. It is a setting (TPHD Mode) fixed for the session: with it off
 * the game is plain GameCube Dusklight and is_active returns false. Mods use this service to tell
 * whether TPHD mode is on and to reuse the helpers the TPHD screens are drawn with.
 *
 * Revo layouts are authored 608 units wide. The Wii U stretches every Revo screen to its 16:9 TV
 * (get_tv_stretch) and squeezes back the panes whose user info is 'n_43' (get_n43_squeeze), so text
 * and icons keep their proportions. Here the TV is the largest centred 16:9 box of the window: on
 * narrower windows the 2D canvas is taller than 448 units (negative mDoGph_gInf_c::getMinYF()).
 * apply_n43 / draw_revo_screen do all of this.
 */

/* Wii U layout archives the TPHD screens load (from <content>/res/LayoutRevo). */
typedef enum TphdArchive {
    TPHD_ARCHIVE_MAIN_2D = 0,     /* main2DR.arc: HUD, button art, Wii U fonts icons */
    TPHD_ARCHIVE_RING = 1,        /* ringresR.arc: item screen */
    TPHD_ARCHIVE_FISH = 2,        /* fishresR.arc */
    TPHD_ARCHIVE_COLLECT = 3,     /* clctresR.arc: collection screen, scent icons */
    TPHD_ARCHIVE_BUTTON = 4,      /* buttonR.arc: interaction prompts */
    TPHD_ARCHIVE_MSG_RES_0 = 5,   /* msgres00R.arc: 3-way choice window */
    TPHD_ARCHIVE_MSG_RES_1 = 6,   /* msgres01R.arc */
    TPHD_ARCHIVE_MSG_COMMON = 7,  /* msgcomR.arc */
    TPHD_ARCHIVE_OPTIONS = 8,     /* optresR.arc */
    TPHD_ARCHIVE_STAMPS = 9,      /* stampResR.arc */
    TPHD_ARCHIVE_COUNT
} TphdArchive;

typedef struct TphdService {
    ServiceHeader header;

    /* True while TPHD mode is on (a valid Wii U content folder is set). */
    bool (*is_active)(ModContext* ctx);

    /*
     * Copies the Wii U content folder (the parent of res/) as UTF-8 into buf, NUL terminated.
     * out_size receives the needed size including the NUL (may be NULL). MOD_UNAVAILABLE when
     * TPHD mode is off, MOD_INVALID_ARGUMENT when buf is too small.
     */
    ModResult (*get_content_path)(ModContext* ctx, char* buf, size_t buf_size, size_t* out_size);

    /*
     * Reads one file (Yaz0 decoded) of a Wii U archive, e.g. res_path "res/Object/Alink.arc",
     * file_path "bmdr/al.bmd". Call with buf NULL to get the size in out_size. The textures of
     * Wii U archives live in the .pack.gz next to them, not in the archive.
     */
    ModResult (*read_arc_file)(ModContext* ctx, const char* res_path, const char* file_path, void* buf,
        size_t buf_size, size_t* out_size);

    /* JKRArchive* of a loaded Wii U layout archive (NULL when not loaded / TPHD mode off). */
    void* (*get_archive)(ModContext* ctx, TphdArchive archive);

    /* Horizontal stretch of Revo screens: width of the 16:9 box of the canvas / 608. */
    float (*get_tv_stretch)(ModContext* ctx);

    /* Horizontal scale the Wii U applies to 'n_43' panes of a stretched screen. */
    float (*get_n43_squeeze)(ModContext* ctx);

    /*
     * Applies the 'n_43' squeeze to every 'n_43' pane under pane (a J2DPane* / J2DScreen*), with
     * the Wii U pivot. Call it before drawing a Revo screen stretched by get_tv_stretch.
     */
    ModResult (*apply_n43)(ModContext* ctx, void* pane);

    /*
     * Draws a Revo screen (J2DScreen*) the Wii U way: stretched over the 16:9 box from its left
     * edge, 'n_43' panes squeezed, clipped to the box. graf_context is a J2DGrafContext* (NULL: the
     * current 2D graf port). Offsets are in layout units.
     */
    ModResult (*draw_revo_screen)(ModContext* ctx, void* screen, void* graf_context, float offset_x,
        float offset_y);

    /*
     * Rescans <data folder>/texture_replacements/ for tphd_* texture packs. Textures already in
     * use keep their replacement until their archive is loaded again.
     */
    ModResult (*reload_texture_packs)(ModContext* ctx);
} TphdService;

MOD_DECLARE_SERVICE(TphdService, svc_tphd, TPHD_SERVICE_ID, TPHD_SERVICE_MAJOR, TPHD_SERVICE_MINOR);
