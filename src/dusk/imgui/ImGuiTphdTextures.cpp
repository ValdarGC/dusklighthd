#include "ImGuiMenuTools.hpp"

#include "dusk/data.hpp"
#include "dusk/texture_replacements.hpp"
#include "dusk/tphd/HdTextureExport.hpp"

#include <borealis/io.hpp>
#include <fmt/format.h>
#include <imgui.h>

#include <system_error>

namespace dusk {
namespace {
tphd::HdTextureExportOptions s_exportOptions;

std::string format_bytes(u64 bytes) {
    if (bytes >= (u64(1) << 30)) {
        return fmt::format("{:.2f} GB", double(bytes) / double(u64(1) << 30));
    }
    return fmt::format("{:.1f} MB", double(bytes) / double(u64(1) << 20));
}
}  // namespace

void ImGuiMenuTools::ShowTphdTextures() {
    if (!m_showTphdTextures) {
        return;
    }
    ImGui::SetNextWindowSize(ImVec2(460, 0), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("TPHD Textures", &m_showTphdTextures)) {
        ImGui::End();
        return;
    }

    const auto progress = tphd::hd_texture_export_progress();
    const auto root = tphd::hd_texture_export_root();

    ImGui::TextWrapped("Exports the Wii U textures to:");
    ImGui::TextWrapped("%s", borealis::io::fs_path_to_string(root).c_str());
    std::error_code ec;
    const auto space = std::filesystem::space(root.parent_path().parent_path(), ec);
    if (!ec) {
        ImGui::Text("Free disk space: %s", format_bytes(space.available).c_str());
        if (space.available < (u64(4) << 30)) {
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f),
                               "A full export can take a few GB (PNG takes more than DDS).");
        }
    }
    ImGui::Separator();

    ImGui::BeginDisabled(progress.running);
    ImGui::TextUnformatted("Export:");
    ImGui::Checkbox("Stages", &s_exportOptions.stages);
    ImGui::SameLine();
    ImGui::Checkbox("Objects / characters", &s_exportOptions.objects);
    ImGui::SameLine();
    ImGui::Checkbox("UI", &s_exportOptions.ui);
    ImGui::Checkbox("Particles", &s_exportOptions.particles);
    ImGui::SameLine();
    ImGui::Checkbox("Other", &s_exportOptions.other);
    ImGui::SameLine();
    if (ImGui::SmallButton("All")) {
        s_exportOptions.stages = s_exportOptions.objects = s_exportOptions.ui = s_exportOptions.particles =
            s_exportOptions.other = true;
    }

    int format = s_exportOptions.png ? 1 : 0;
    ImGui::TextUnformatted("Format:");
    ImGui::SameLine();
    ImGui::RadioButton("DDS (with mipmaps)", &format, 0);
    ImGui::SameLine();
    ImGui::RadioButton("PNG (+ _mipN files)", &format, 1);
    s_exportOptions.png = format == 1;

    const bool anySelected = s_exportOptions.stages || s_exportOptions.objects || s_exportOptions.ui ||
                             s_exportOptions.particles || s_exportOptions.other;
    ImGui::BeginDisabled(!anySelected);
    if (ImGui::Button("Export textures")) {
        tphd::start_hd_texture_export(s_exportOptions);
    }
    ImGui::EndDisabled();
    ImGui::EndDisabled();

    if (progress.running) {
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            tphd::cancel_hd_texture_export();
        }
    }
#if DUSK_CAN_OPEN_DATA_FOLDER
    ImGui::SameLine();
    if (ImGui::Button("Open data folder")) {
        data::open_data_path();
    }
#endif

    if (progress.running || progress.finished) {
        const float fraction =
            progress.packsTotal != 0 ? float(progress.packsDone) / float(progress.packsTotal) : 0.0f;
        ImGui::ProgressBar(fraction, ImVec2(-1, 0),
                           fmt::format("{}/{}", progress.packsDone, progress.packsTotal).c_str());
        ImGui::Text("%u textures, %s", progress.texturesWritten, format_bytes(progress.bytesWritten).c_str());
        if (!progress.current.empty()) {
            ImGui::TextWrapped("%s", progress.current.c_str());
        }
        if (progress.finished && progress.error.empty()) {
            ImGui::TextUnformatted("Done.");
        }
    }
    if (!progress.error.empty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", progress.error.c_str());
    }

    ImGui::Separator();
    ImGui::TextWrapped("Texture packs: copy edited tphd_*.dds / .png files (any subfolder) into "
                       "texture_replacements in the data folder. They replace the HD texture with the "
                       "same hash and are loaded when the area is entered again.");
    if (ImGui::Button("Reload texture packs")) {
        texture_replacements::reload();
    }

    ImGui::End();
}
}  // namespace dusk
