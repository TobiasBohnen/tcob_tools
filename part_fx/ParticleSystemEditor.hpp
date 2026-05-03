// Copyright (c) 2026 Tobias Bohnen
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#pragma once

#include "Common.hpp"
#include "UI.hpp"

class ParticleSystemEditor : public scene {
public:
    ParticleSystemEditor(game& game);
    ~ParticleSystemEditor();

protected:
    void on_start() override;

    void on_draw_to(render_target& target, transform const& xform) override;

    void on_update(milliseconds deltaTime) override;
    void on_fixed_update(milliseconds deltaTime) override;

    void on_key_down(keyboard::event const& ev) override;

private:
    particle_system _system0;

    std::shared_ptr<main_ui> _ui;
};
