// Copyright (c) 2026 Tobias Bohnen
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include "ParticleSystemEditor.hpp"

using namespace std::chrono_literals;

ParticleSystemEditor::ParticleSystemEditor(game& game)
    : scene {game}
    , _system0 {true, 50000}
{
}

ParticleSystemEditor::~ParticleSystemEditor() = default;

void ParticleSystemEditor::on_start()
{
    window().ClearColor = colors::Black;
    using namespace tcob::literals;

    auto& resMgr {parent().library()};
    auto* resGrp {resMgr.get_group("res")};
    _system0.Material = resGrp->get<material>("QuadParticleMat");

    _ui                = std::make_shared<main_ui>(window(), *resGrp, _system0);
    root_node().Entity = _ui;

    _system0.start();
}

void ParticleSystemEditor::on_draw_to(render_target& target, transform const& xform)
{
    _system0.draw_to(target, xform);
}

void ParticleSystemEditor::on_update(milliseconds deltaTime)
{
    _system0.update(deltaTime);
}

void ParticleSystemEditor::on_fixed_update(milliseconds deltaTime)
{
    auto const& stats {locate_service<gfx::render_system>().statistics()};
    auto const& mouse {locate_service<input::system>().mouse().get_position()};
    window().Title = std::format("TestGame | FPS avg:{:.2f} best:{:.2f} worst:{:.2f} | x:{} y:{} | particles:{} ",
                                 stats.average_FPS(), stats.best_FPS(), stats.worst_FPS(),
                                 mouse.X, mouse.Y,
                                 _system0.particle_count());
}

void ParticleSystemEditor::on_key_down(keyboard::event const& ev)
{
    switch (ev.ScanCode) {
    case scan_code::BACKSPACE:
        parent().pop_current_scene();
        break;
    default:
        break;
    }
}
