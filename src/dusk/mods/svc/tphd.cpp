#include "internal.hpp"
#include "registry.hpp"

#include "mods/svc/tphd.h"

#include "JSystem/J2DGraph/J2DScreen.h"
#include "d/d_com_inf_game.h"
#include "dusk/settings.h"
#include "dusk/tphd/HdAssetLayer.hpp"
#include "dusk/tphd/HdSubscreen.hpp"
#include "dusk/tphd/HdTextureExport.hpp"

#include <borealis/io.hpp>
#include <cstring>
#include <string>

extern JKRArchive* g_tphdMain2DRevoArchive;
extern JKRArchive* g_tphdRingRevoArchive;
extern JKRArchive* g_tphdFishRevoArchive;
extern JKRArchive* g_tphdCollectRevoArchive;
extern JKRArchive* g_tphdButtonRevoArchive;
extern JKRArchive* g_tphdMsgRes0RevoArchive;
extern JKRArchive* g_tphdMsgRes1RevoArchive;
extern JKRArchive* g_tphdMsgComRevoArchive;
extern JKRArchive* g_tphdOptRevoArchive;
extern JKRArchive* g_tphdStampRevoArchive;

namespace dusk::mods::svc {
namespace {

bool tphd_is_active(ModContext*) {
    return dusk::tphd_active();
}

ModResult copy_out(const void* data, size_t size, void* buf, size_t bufSize, size_t* outSize) {
    if (outSize != nullptr) {
        *outSize = size;
    }
    if (buf == nullptr) {
        return outSize != nullptr ? MOD_OK : MOD_INVALID_ARGUMENT;
    }
    if (bufSize < size) {
        return MOD_INVALID_ARGUMENT;
    }
    std::memcpy(buf, data, size);
    return MOD_OK;
}

ModResult tphd_get_content_path(ModContext*, char* buf, size_t bufSize, size_t* outSize) {
    if (!dusk::tphd_active()) {
        return MOD_UNAVAILABLE;
    }
    const std::string path = borealis::io::fs_path_to_string(dusk::tphd_content_path());
    return copy_out(path.c_str(), path.size() + 1, buf, bufSize, outSize);
}

ModResult tphd_read_arc_file(ModContext*, const char* resPath, const char* filePath, void* buf,
    size_t bufSize, size_t* outSize) {
    if (resPath == nullptr || filePath == nullptr) {
        return MOD_INVALID_ARGUMENT;
    }
    if (!dusk::tphd_active()) {
        return MOD_UNAVAILABLE;
    }
    const auto data = dusk::tphd::read_hd_arc_file(resPath, filePath);
    if (!data) {
        return MOD_UNAVAILABLE;
    }
    return copy_out(data->data(), data->size(), buf, bufSize, outSize);
}

void* tphd_get_archive(ModContext*, TphdArchive archive) {
    if (!dusk::tphd_active()) {
        return nullptr;
    }
    switch (archive) {
    case TPHD_ARCHIVE_MAIN_2D: return g_tphdMain2DRevoArchive;
    case TPHD_ARCHIVE_RING: return g_tphdRingRevoArchive;
    case TPHD_ARCHIVE_FISH: return g_tphdFishRevoArchive;
    case TPHD_ARCHIVE_COLLECT: return g_tphdCollectRevoArchive;
    case TPHD_ARCHIVE_BUTTON: return g_tphdButtonRevoArchive;
    case TPHD_ARCHIVE_MSG_RES_0: return g_tphdMsgRes0RevoArchive;
    case TPHD_ARCHIVE_MSG_RES_1: return g_tphdMsgRes1RevoArchive;
    case TPHD_ARCHIVE_MSG_COMMON: return g_tphdMsgComRevoArchive;
    case TPHD_ARCHIVE_OPTIONS: return g_tphdOptRevoArchive;
    case TPHD_ARCHIVE_STAMPS: return g_tphdStampRevoArchive;
    default: return nullptr;
    }
}

float tphd_get_tv_stretch(ModContext*) {
    return dusk::tphd::subscreenScaleX();
}

float tphd_get_n43_squeeze(ModContext*) {
    return dusk::tphd::wiiuTvN43();
}

ModResult tphd_apply_n43(ModContext*, void* pane) {
    if (pane == nullptr) {
        return MOD_INVALID_ARGUMENT;
    }
    dusk::tphd::applyN43(static_cast<J2DPane*>(pane), dusk::tphd::wiiuTvN43(), false, true);
    return MOD_OK;
}

ModResult tphd_draw_revo_screen(
    ModContext*, void* screen, void* grafContext, float offsetX, float offsetY) {
    if (screen == nullptr) {
        return MOD_INVALID_ARGUMENT;
    }
    J2DGrafContext* graf = grafContext != nullptr ? static_cast<J2DGrafContext*>(grafContext)
                                                  : dComIfGp_getCurrentGrafPort();
    dusk::tphd::drawRevoScreen(static_cast<J2DScreen*>(screen), graf, offsetX, offsetY, true);
    return MOD_OK;
}

ModResult tphd_reload_texture_packs(ModContext*) {
    dusk::tphd::reload_user_hd_textures();
    return MOD_OK;
}

constexpr TphdService s_tphdService{
    .header = SERVICE_HEADER(TphdService, TPHD_SERVICE_MAJOR, TPHD_SERVICE_MINOR),
    .is_active = tphd_is_active,
    .get_content_path = tphd_get_content_path,
    .read_arc_file = tphd_read_arc_file,
    .get_archive = tphd_get_archive,
    .get_tv_stretch = tphd_get_tv_stretch,
    .get_n43_squeeze = tphd_get_n43_squeeze,
    .apply_n43 = tphd_apply_n43,
    .draw_revo_screen = tphd_draw_revo_screen,
    .reload_texture_packs = tphd_reload_texture_packs,
};

}  // namespace

constinit const ServiceModule g_tphdModule{
    .id = TPHD_SERVICE_ID,
    .majorVersion = TPHD_SERVICE_MAJOR,
    .minorVersion = TPHD_SERVICE_MINOR,
    .service = &s_tphdService,
};

}  // namespace dusk::mods::svc
