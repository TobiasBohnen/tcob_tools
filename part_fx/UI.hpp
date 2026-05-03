// Copyright (c) 2026 Tobias Bohnen
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#pragma once

#include "Common.hpp"

using namespace tcob::ui;

class main_ui : public form<dock_layout> {
public:
    explicit main_ui(window& wnd, assets::group const& resGrp, particle_system& system);

private:
    void rebuild_emitter_tab(tab_container& tabs, particle_emitter& emi, string const& tabName);
    void build_template_settings(panel& parent, particle_emitter& emi);
    void build_pattern_settings(panel& parent, particle_emitter& emi);
    void build_emitter_settings(panel& parent, particle_emitter& emi);

    void create_styles(assets::group const& resGrp);

    window&                        _wnd;
    assets::group const&           _resGrp;
    particle_system&               _system;
    std::vector<particle_emitter*> _emitters;
};
