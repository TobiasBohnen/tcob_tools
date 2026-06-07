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

struct atlas_shape {
    string                                      Name;
    size_f                                      Size;
    std::function<void(canvas&, rect_f const&)> Draw;
};

void ParticleSystemEditor::build_particle_atlas()
{
    constexpr size_i ATLAS_SIZE {512, 512};
    constexpr size_f ATLAS_SIZE_F {512.0f, 512.0f};
    constexpr f32    GAP {2.0f};
    constexpr f32    Y {2.0f};

    static std::array<atlas_shape, 10> const SHAPES {{
        {.Name = "circle", .Size = {64, 64}, .Draw = [](canvas& c, rect_f const& r) {
             c.circle({r.left() + (r.width() * 0.5f), r.top() + (r.height() * 0.5f)}, r.width() * 0.5f);
         }},
        {.Name = "square", .Size = {32, 32}, .Draw = [](canvas& c, rect_f const& r) {
             c.rect(r);
         }},
        {.Name = "triangle", .Size = {32, 32}, .Draw = [](canvas& c, rect_f const& r) {
             c.triangle(
                 {r.left() + (r.width() * 0.5f), r.top()},
                 {r.left(), r.top() + r.height()},
                 {r.left() + r.width(), r.top() + r.height()});
         }},
        {.Name = "star", .Size = {48, 48}, .Draw = [](canvas& c, rect_f const& r) {
             c.star(
                 {r.left() + (r.width() * 0.5f), r.top() + (r.height() * 0.5f)},
                 r.width() * 0.5f, r.width() * 0.25f, 5);
         }},
        {.Name = "ring", .Size = {48, 48}, .Draw = [](canvas& c, rect_f const& r) {
             point_f const center {r.left() + (r.width() * 0.5f), r.top() + (r.height() * 0.5f)};
             c.circle(center, r.width() * 0.5f);
             c.set_path_winding(solidity::Hole);
             c.circle(center, r.width() * 0.25f);
         }},
        {.Name = "diamond", .Size = {48, 48}, .Draw = [](canvas& c, rect_f const& r) {
             point_f const center {r.left() + (r.width() * 0.5f), r.top() + (r.height() * 0.5f)};
             c.move_to({center.X, r.top()});
             c.line_to({r.left() + r.width(), center.Y});
             c.line_to({center.X, r.top() + r.height()});
             c.line_to({r.left(), center.Y});
             c.close_path();
         }},
        {.Name = "hexagon", .Size = {48, 48}, .Draw = [](canvas& c, rect_f const& r) {
             c.regular_polygon(
                 {r.left() + (r.width() * 0.5f), r.top() + (r.height() * 0.5f)},
                 {r.width() * 0.5f, r.height() * 0.5f}, 6);
         }},
        {.Name = "sparkle", .Size = {48, 48}, .Draw = [](canvas& c, rect_f const& r) {
             c.star(
                 {r.left() + (r.width() * 0.5f), r.top() + (r.height() * 0.5f)},
                 r.width() * 0.5f, r.width() * 0.15f, 4);
         }},
        {.Name = "cross", .Size = {48, 48}, .Draw = [](canvas& c, rect_f const& r) {
             f32 const t {r.width() / 3.0f};
             f32 const x0 {r.left()};
             f32 const y0 {r.top()};
             c.move_to({x0 + t, y0});
             c.line_to({x0 + (t * 2), y0});
             c.line_to({x0 + (t * 2), y0 + t});
             c.line_to({x0 + (t * 3), y0 + t});
             c.line_to({x0 + (t * 3), y0 + (t * 2)});
             c.line_to({x0 + (t * 2), y0 + (t * 2)});
             c.line_to({x0 + (t * 2), y0 + (t * 3)});
             c.line_to({x0 + t, y0 + (t * 3)});
             c.line_to({x0 + t, y0 + (t * 2)});
             c.line_to({x0, y0 + (t * 2)});
             c.line_to({x0, y0 + t});
             c.line_to({x0 + t, y0 + t});
             c.close_path();
         }},
        {.Name = "arrow", .Size = {48, 48}, .Draw = [](canvas& c, rect_f const& r) {
             f32 const mx {r.left() + (r.width() * 0.5f)};
             f32 const my {r.top() + (r.height() * 0.5f)};
             f32 const sw {r.width() * 0.25f};              // shaft half-width
             f32 const hy {r.top() + (r.height() * 0.45f)}; // head/shaft split
             c.move_to({mx, r.top()});
             c.line_to({r.left() + r.width(), hy});
             c.line_to({mx + sw, hy});
             c.line_to({mx + sw, r.top() + r.height()});
             c.line_to({mx - sw, r.top() + r.height()});
             c.line_to({mx - sw, hy});
             c.line_to({r.left(), hy});
             c.close_path();
         }},
    }};

    canvas canvas;
    canvas.begin_frame(ATLAS_SIZE, 1.0f);
    canvas.clear(colors::Transparent);
    canvas.set_fill_style(colors::White);
    canvas.set_shape_antialias(false);
    canvas.set_edge_antialias(false);

    f32 x {GAP};
    for (auto const& shape : SHAPES) {
        rect_f const r {x, Y, shape.Size.Width, shape.Size.Height};
        canvas.begin_path();
        shape.Draw(canvas, r);
        canvas.fill();
        _texAtlas->regions()[shape.Name] = texture_region {
            .UVRect = to_uv(r, ATLAS_SIZE_F),
            .Level  = 0};
        x += shape.Size.Width + GAP;
    }

    canvas.end_frame();

    auto img {canvas.get_texture()->copy_to_image(0)};
    img.flip_vertically();
    std::ignore = img.save("atlas.png");
    _texAtlas->resize(img.info().Size, 1, texture::format::RGBA8);
    _texAtlas->update_data(img, 0);
    _system.Material->first_pass().Texture = _texAtlas;
}
