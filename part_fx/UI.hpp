// Copyright (c) 2026 Tobias Bohnen
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#pragma once

#include "Common.hpp"

using namespace tcob::ui;

struct emitter_changed_event {
    isize                      Index {};
    particle_emitter::settings Settings;
};

class main_ui : public form<dock_layout> {
public:
    explicit main_ui(window& wnd, assets::group const& resGrp, std::unordered_map<string, texture_region> const& texRegions);

    signal<emitter_changed_event const> EmitterSettingsChanged;
    signal<>                            EmitterAdded;
    signal<isize const>                 EmitterRemoved;

    signal<> RestartRequested;
    signal<> SaveRequested;
    signal<> QuitRequested;

    void add_emitter(particle_emitter::settings const& settings);

private:
    void rebuild_emitter_tab(tab_container& tabs, isize emiIdx);
    void build_emitter_settings(panel& parent, isize emiIdx);
    void build_pattern_settings(panel& parent, isize emiIdx);
    void build_template_settings(panel& parent, isize emiIdx);
    void create_styles(assets::group const& resGrp);
    void notify(isize emiIdx);

    window&                                 _wnd;
    std::vector<string>                     _texRegions;
    assets::group const&                    _resGrp;
    std::vector<particle_emitter::settings> _settings;
};
