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
    _system.Material = resGrp->get<material>("ParticleMat");

    build_particle_atlas();

    _ui                = std::make_shared<main_ui>(window(), *resGrp, _texAtlas->regions());
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

    _ui->RestartRequested.connect([&] { _system.restart(); });
    _ui->SaveRequested.connect([&] {
        data::array save;
        for (usize i {0}; i < _emitters.size(); ++i) {
            save.add(_emitters[i]->Settings);
        }
        save.save("save.json");
    });

    _ui->QuitRequested.connect([&] { parent().pop_current_scene(); });
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

static auto to_uv(rect_f const& px, size_f const& atlasSize) -> rect_f
{
    return rect_f {
        px.left() / atlasSize.Width,
        px.top() / atlasSize.Height,
        px.width() / atlasSize.Width,
        px.height() / atlasSize.Height};
}

void ParticleSystemEditor::build_particle_atlas()
{
    constexpr size_i ATLAS_SIZE {512, 512};
    constexpr size_f ATLAS_SIZE_F {512.0f, 512.0f};
    constexpr f32    GAP {2};

    canvas canvas;

    canvas.begin_frame(ATLAS_SIZE, 1.0f);
    canvas.clear(colors::Transparent);

    canvas.set_fill_style(colors::White);
    canvas.set_shape_antialias(false);
    canvas.set_edge_antialias(false);

    f32 x {2};

    {
        rect_f r {x, 2.0f, 64.0f, 64.0f};

        canvas.begin_path();
        canvas.circle(
            {r.left() + (r.width() * 0.5f), r.top() + (r.height() * 0.5f)},
            r.width() * 0.5f);
        canvas.fill();

        _texAtlas->regions()["circle"] = texture_region {
            .UVRect = to_uv(r, ATLAS_SIZE_F),
            .Level  = 0};

        x += r.width() + GAP;
    }
    {
        rect_f r {x, 2.0f, 32.0f, 32.0f};

        canvas.begin_path();
        canvas.rect(r);
        canvas.fill();

        _texAtlas->regions()["square"] = texture_region {
            .UVRect = to_uv(r, ATLAS_SIZE_F),
            .Level  = 0};

        x += r.width() + GAP;
    }
    {
        rect_f r {x, 2.0f, 32.0f, 32.0f};

        canvas.begin_path();
        canvas.triangle(
            {r.left() + (r.width() * 0.5f), r.top()},
            {r.left(), r.top() + r.height()},
            {r.left() + r.width(), r.top() + r.height()});
        canvas.fill();

        _texAtlas->regions()["triangle"] = texture_region {
            .UVRect = to_uv(r, ATLAS_SIZE_F),
            .Level  = 0};

        x += r.width() + GAP;
    }
    {
        rect_f r {x, 2.0f, 48.0f, 48.0f};

        canvas.begin_path();
        canvas.star(
            {r.left() + (r.width() * 0.5f), r.top() + (r.height() * 0.5f)},
            r.width() * 0.5f,
            r.width() * 0.25f,
            5);
        canvas.fill();

        _texAtlas->regions()["star"] = texture_region {
            .UVRect = to_uv(r, ATLAS_SIZE_F),
            .Level  = 0};

        x += r.width() + GAP;
    }

    canvas.end_frame();

    auto texAtlas {canvas.get_texture()->copy_to_image(0)};
    texAtlas.flip_vertically();
    std::ignore = texAtlas.save("atlas.png");

    _texAtlas->resize(texAtlas.info().Size, 1, texture::format::RGBA8);
    _texAtlas->update_data(texAtlas, 0);

    _system.Material->first_pass().Texture = _texAtlas;
}
