// Copyright (c) 2026 Tobias Bohnen
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#pragma once

// IWYU pragma: always_keep

#include <tcob/tcob.hpp>
using namespace tcob;
using namespace tcob::ai;
using namespace tcob::gfx;
using namespace tcob::assets;
using namespace tcob::scripting;
using namespace tcob::physics;
using namespace tcob::audio;
using namespace tcob::data;
using namespace tcob::input;

////////////////////////////////////////////////////////////

class basic_cam {
public:
    std::optional<rect_f> LimitBounds;

    void on_key_down(keyboard::event const& ev) const
    {
        f32 moveFactor {14.f};
        if (ev.ScanCode == scan_code::A) {
            move_by({-moveFactor, 0});
        } else if (ev.ScanCode == scan_code::D) {
            move_by({moveFactor, 0});
        } else if (ev.ScanCode == scan_code::S) {
            move_by({0.0f, moveFactor});
        } else if (ev.ScanCode == scan_code::W) {
            move_by({0.0f, -moveFactor});
        }
    }

    void on_mouse_button_down(mouse::button_event const& ev)
    {
        _mouseDown = false;
        if (ev.Pressed && ev.Button == mouse::button::Left) {
            _mouseDown = true;
        }
    }

    void on_mouse_button_up(mouse::button_event const& ev)
    {
        if (ev.Button == mouse::button::Left) {
            _mouseDown = false;
        }
    }

    void on_mouse_motion(mouse::motion_event const& ev) const
    {
        if (_mouseDown) {
            auto const zoom {camera().Zoom};
            move_by(-(point_f {ev.RelativeMotion} / point_f {zoom.Width, zoom.Height}));
        }
    }

    void on_mouse_wheel(mouse::wheel_event const& ev)
    {
        _zoomStage = std::clamp(_zoomStage - static_cast<i32>(ev.Scroll.Y), 0, 6);
        constexpr std::array<size_f, 7> zoomLevels {{{5.f, 5.f}, {3.f, 3.f}, {2.f, 2.f}, {1.f, 1.f}, {0.75f, 0.75f}, {0.5f, 0.5f}, {0.25f, 0.25f}}};
        camera().Zoom = zoomLevels[_zoomStage];
    }

    auto screen_to_world(point_i pos) const -> point_f
    {
        return camera().convert_screen_to_world(pos);
    };

private:
    auto camera() const -> camera&
    {
        return locate_service<render_system>().window().camera();
    }

    void move_by(point_f off) const
    {
        auto& cam {camera()};
        if (!LimitBounds || LimitBounds->contains(cam.Position + off)) {
            cam.move_by(off);
        }
    }

    bool _mouseDown {false};
    i32  _zoomStage {3};
};
