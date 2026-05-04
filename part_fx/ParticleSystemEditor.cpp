// Copyright (c) 2026 Tobias Bohnen
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include "ParticleSystemEditor.hpp"

using namespace std::chrono_literals;

ParticleSystemEditor::ParticleSystemEditor(game& game)
    : scene {game}
    , _system {true, 50000}
{
}

ParticleSystemEditor::~ParticleSystemEditor() = default;

void ParticleSystemEditor::on_start()
{
    window().ClearColor = colors::Black;
    using namespace tcob::literals;

    auto& resMgr {parent().library()};
    auto* resGrp {resMgr.get_group("res")};
    _system.Material = resGrp->get<material>("QuadParticleMat");

    _ui                = std::make_shared<main_ui>(window(), *resGrp);
    root_node().Entity = _ui;
    _ui->EmitterAdded.connect([&] {
        auto& emi {_system.create_emitter()};
        _emitters.push_back(&emi);
    });

    _ui->EmitterRemoved.connect([&](isize idx) {
        _system.remove_emitter(*_emitters[idx]);
        _emitters.erase(_emitters.begin() + idx);
    });

    _ui->EmitterSettingsChanged.connect([&](emitter_changed_event const& ev) {
        _emitters[ev.Index]->Settings = ev.Settings;
    });

    _ui->StartRequested.connect([&] { _system.start(); });
    _ui->StopRequested.connect([&] { _system.stop(); });
    _ui->RestartRequested.connect([&] { _system.restart(); });
}

void ParticleSystemEditor::on_draw_to(render_target& target, transform const& xform)
{
    _system.draw_to(target, xform);
}

void ParticleSystemEditor::on_update(milliseconds deltaTime)
{
    _system.update(deltaTime);
}

void ParticleSystemEditor::on_fixed_update(milliseconds deltaTime)
{
    auto const& stats {locate_service<gfx::render_system>().statistics()};
    auto const& mouse {locate_service<input::system>().mouse().get_position()};
    window().Title = std::format("TestGame | FPS avg:{:.2f} best:{:.2f} worst:{:.2f} | x:{} y:{} | particles:{} ",
                                 stats.average_FPS(), stats.best_FPS(), stats.worst_FPS(),
                                 mouse.X, mouse.Y,
                                 _system.particle_count());
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
