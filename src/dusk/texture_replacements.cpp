#include "dusk/texture_replacements.hpp"

#include <aurora/texture.hpp>

#include "dusk/logging.h"
#include "dusk/main.h"
#include "dusk/settings.h"
#include "dusk/tphd/HdTextureExport.hpp"

namespace dusk::texture_replacements {
namespace {
aurora::texture::ReplacementGroup s_directoryGroup;
}

void reload() {
    aurora::texture::unregister_replacements(s_directoryGroup);
    s_directoryGroup.registrations.clear();
    tphd::reload_user_hd_textures();

    if (!getSettings().game.enableTextureReplacements) {
        return;
    }

    const auto root = ConfigPath / "texture_replacements";
    s_directoryGroup = aurora::texture::load_replacement_directory(
        root, {.priority = kUserTextureReplacementPriority, .synchronous = true});
    DuskLog.info("Texture replacement directory loaded: {} registration(s)",
                 s_directoryGroup.registrations.size());
}

void shutdown() {
    aurora::texture::unregister_replacements(s_directoryGroup);
    s_directoryGroup.registrations.clear();
}

}
