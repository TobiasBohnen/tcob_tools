// Copyright (c) 2026 Tobias Bohnen
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include "UI.hpp"

using namespace tcob::literals;
using namespace std::chrono_literals;

////////////////////////////////////////////////////////////

main_ui::main_ui(window& wnd, assets::group const& resGrp)
    : form<dock_layout> {{.Name = "particle_editor", .Bounds = rect_i {wnd.bounds().width() * 2 / 3, 0, wnd.bounds().width() / 3, wnd.bounds().height()}}}
    , _wnd {wnd}
    , _resGrp {resGrp}
{
    create_styles(resGrp);

    // toolbar
    auto& toolbar {create_container<panel>(dock_style::Top, "Toolbar")};
    toolbar.Flex = {.Width = 100_pct, .Height = 5_pct};
    auto& toolbarLayout {toolbar.create_layout<grid_layout>(size_i {5, 1})};

    auto& btnAddEmitter {toolbarLayout.create_widget<button>({{0, 0}, {1, 1}}, "BtnAdd")};
    btnAddEmitter.Label = "Add";

    auto& btnRemoveEmitter {toolbarLayout.create_widget<button>({{1, 0}, {1, 1}}, "BtnRemove")};
    btnRemoveEmitter.Label = "Remove";

    auto& btnStart {toolbarLayout.create_widget<button>({{2, 0}, {1, 1}}, "BtnStart")};
    btnStart.Label = "Start";

    auto& btnStop {toolbarLayout.create_widget<button>({{3, 0}, {1, 1}}, "BtnStop")};
    btnStop.Label = "Stop";

    auto& btnRestart {toolbarLayout.create_widget<button>({{4, 0}, {1, 1}}, "BtnRestart")};
    btnRestart.Label = "Restart";

    // emitter tabs
    auto& tabs {create_container<tab_container>(dock_style::Fill, "EmitterTabs")};
    tabs.Flex = {.Width = 100_pct, .Height = 95_pct};

    btnStart.Click.connect([&] { StartRequested(); });
    btnStop.Click.connect([&] { StopRequested(); });
    btnRestart.Click.connect([&] { RestartRequested(); });

    btnAddEmitter.Click.connect([&] {
        _settings.push_back({});
        isize const  emiIdx {static_cast<isize>(_settings.size()) - 1};
        string const name {"Emi_" + std::to_string(_settings.size())};
        rebuild_emitter_tab(tabs, emiIdx);
        EmitterAdded();
        notify(emiIdx);
    });

    btnRemoveEmitter.Click.connect([&, tabs = &tabs] {
        if (_settings.empty()) { return; }
        isize const activeIdx {*tabs->ActiveTabIndex};
        if (activeIdx < 0 || activeIdx >= static_cast<isize>(_settings.size())) { return; }
        tabs->remove_tab(activeIdx);
        _settings.erase(_settings.begin() + activeIdx);
        EmitterRemoved(activeIdx);
    });
}

////////////////////////////////////////////////////////////

void main_ui::add_emitter(particle_emitter::settings const& settings)
{
    auto& tabs {*dynamic_cast<tab_container*>(find_widget_by_name("EmitterTabs"))};
    _settings.push_back(settings);
    isize const emiIdx {static_cast<isize>(_settings.size()) - 1};
    rebuild_emitter_tab(tabs, emiIdx);
}

////////////////////////////////////////////////////////////

void main_ui::notify(isize emiIdx)
{
    EmitterSettingsChanged({.Index = emiIdx, .Settings = _settings[emiIdx]});
}

////////////////////////////////////////////////////////////

void main_ui::rebuild_emitter_tab(tab_container& tabs, isize emiIdx)
{
    string const tabName {"Emi_" + std::to_string(emiIdx + 1)};

    auto& tabPanel {tabs.create_tab<panel>(tabName)};
    tabPanel.ScrollEnabled = true;

    auto& layout {tabPanel.create_layout<dock_layout>()};
    auto& acc {layout.create_widget<accordion>(dock_style::Fill, "Acc_" + std::to_string(emiIdx))};
    acc.MaximizeActiveSection = true;

    {
        auto& sec {acc.create_section<panel>("Emitter", {.Text = "Emitter"})};
        build_emitter_settings(sec, emiIdx);
    }
    {
        auto& sec {acc.create_section<panel>("Pattern", {.Text = "Pattern"})};
        build_pattern_settings(sec, emiIdx);
    }
    {
        auto& sec {acc.create_section<panel>("Template", {.Text = "Particle Template"})};
        sec.ScrollEnabled = true;
        build_template_settings(sec, emiIdx);
    }
}

////////////////////////////////////////////////////////////

void main_ui::build_emitter_settings(panel& parent, isize emiIdx)
{
    auto  notify {[this, emiIdx] { this->notify(emiIdx); }};
    auto& s {_settings[emiIdx]};
    auto& gl {parent.create_layout<grid_layout>(size_i {0, 17}, true)};

    // Lifetime
    {
        auto& lbl {gl.create_widget<label>({{0, 0}, {1, 1}}, "LblEmiLife")};
        lbl.Label = "Lifetime(ms)";
        auto& tog {gl.create_widget<toggle>({{1, 0}, {1, 1}}, "HasLifetime")};
        tog.Checked = s.Lifetime.has_value();
        auto& spn {gl.create_widget<spinner>({{2, 0}, {1, 1}}, "SpnLifetime")};
        spn.Min   = 0;
        spn.Max   = 60000;
        spn.Step  = 100;
        spn.Value = s.Lifetime ? static_cast<f32>(s.Lifetime->count()) : 1000.f;
        spn.Value.Changed.connect([&s, &tog, notify](f32 v) {
            if (*tog.Checked) { s.Lifetime = milliseconds {v}; }
            notify();
        });
        tog.Checked.Changed.connect([&s, &spn, notify](bool v) {
            s.Lifetime = v ? std::optional<milliseconds> {milliseconds {*spn.Value}} : std::nullopt;
            notify();
        });
    }
    // Spawn X,Y
    {
        auto& lbl {gl.create_widget<label>({{0, 1}, {1, 1}}, "LblSpawnPos")};
        lbl.Label = "Spawn X,Y";
        auto& spnX {gl.create_widget<spinner>({{1, 1}, {1, 1}}, "SpawnX")};
        spnX.Min   = -10000;
        spnX.Max   = 10000;
        spnX.Step  = 1;
        spnX.Value = s.SpawnArea.left();
        auto& spnY {gl.create_widget<spinner>({{2, 1}, {1, 1}}, "SpawnY")};
        spnY.Min   = -10000;
        spnY.Max   = 10000;
        spnY.Step  = 1;
        spnY.Value = s.SpawnArea.top();
        spnX.Value.Changed.connect([&s, &spnY, notify](f32 v) {
            s.SpawnArea = {v, static_cast<f32>(*spnY.Value), s.SpawnArea.width(), s.SpawnArea.height()};
            notify();
        });
        spnY.Value.Changed.connect([&s, &spnX, notify](f32 v) {
            s.SpawnArea = {static_cast<f32>(*spnX.Value), v, s.SpawnArea.width(), s.SpawnArea.height()};
            notify();
        });
    }
    // Spawn W,H
    {
        auto& lbl {gl.create_widget<label>({{0, 2}, {1, 1}}, "LblSpawnSize")};
        lbl.Label = "Spawn W,H";
        auto& spnW {gl.create_widget<spinner>({{1, 2}, {1, 1}}, "SpawnW")};
        spnW.Min   = 0;
        spnW.Max   = 10000;
        spnW.Step  = 1;
        spnW.Value = s.SpawnArea.width();
        auto& spnH {gl.create_widget<spinner>({{2, 2}, {1, 1}}, "SpawnH")};
        spnH.Min   = 0;
        spnH.Max   = 10000;
        spnH.Step  = 1;
        spnH.Value = s.SpawnArea.height();
        spnW.Value.Changed.connect([&s, &spnH, notify](f32 v) {
            s.SpawnArea = {s.SpawnArea.left(), s.SpawnArea.top(), v, static_cast<f32>(*spnH.Value)};
            notify();
        });
        spnH.Value.Changed.connect([&s, &spnW, notify](f32 v) {
            s.SpawnArea = {s.SpawnArea.left(), s.SpawnArea.top(), static_cast<f32>(*spnW.Value), v};
            notify();
        });
    }
}

////////////////////////////////////////////////////////////

void main_ui::build_pattern_settings(panel& parent, isize emiIdx)
{
    auto  notify {[this, emiIdx] { this->notify(emiIdx); }};
    auto& s {_settings[emiIdx]};
    auto& gl {parent.create_layout<grid_layout>(size_i {0, 17}, true)};

    i32 row {0};

    auto& lbl {gl.create_widget<label>({{0, row}, {1, 1}}, "LblPattern")};
    lbl.Label = "Pattern";
    auto& cyc {gl.create_widget<cycle_button>({{1, row}, {2, 1}}, "PatternCycle")};
    cyc.Items.mutate([](auto& items) {
        items.push_back({"Linear"});
        items.push_back({"Burst"});
    });
    cyc.SelectedItemIndex = std::holds_alternative<particle_emitter::emit_burst>(s.Pattern) ? 1 : 0;
    ++row;

    auto& lblRate {gl.create_widget<label>({{0, row}, {1, 1}}, "LblRate")};
    lblRate.Label = "Rate";
    auto& spnRate {gl.create_widget<spinner>({{1, row}, {2, 1}}, "SpnRate")};
    spnRate.Min  = 0;
    spnRate.Max  = 5000;
    spnRate.Step = 10;
    if (auto* lin {std::get_if<particle_emitter::emit_linear>(&s.Pattern)}) {
        spnRate.Value = lin->Rate;
    }
    spnRate.Value.Changed.connect([&s, notify](f32 v) {
        if (auto* lin {std::get_if<particle_emitter::emit_linear>(&s.Pattern)}) {
            lin->Rate = v;
        }
        notify();
    });
    ++row;

    auto& lblCount {gl.create_widget<label>({{0, row}, {1, 1}}, "LblCount")};
    lblCount.Label = "Count";
    auto& spnCount {gl.create_widget<spinner>({{1, row}, {2, 1}}, "SpnCount")};
    spnCount.Min  = 1;
    spnCount.Max  = 10000;
    spnCount.Step = 10;
    ++row;

    auto& lblInterval {gl.create_widget<label>({{0, row}, {1, 1}}, "LblInterval")};
    lblInterval.Label = "Interval(ms)";
    auto& spnInterval {gl.create_widget<spinner>({{1, row}, {2, 1}}, "SpnInterval")};
    spnInterval.Min  = 0;
    spnInterval.Max  = 60000;
    spnInterval.Step = 100;
    ++row;

    auto& lblRepeats {gl.create_widget<label>({{0, row}, {1, 1}}, "LblRepeats")};
    lblRepeats.Label = "Repeats";
    auto& spnRepeats {gl.create_widget<spinner>({{1, row}, {2, 1}}, "SpnRepeats")};
    spnRepeats.Min  = 1;
    spnRepeats.Max  = 10000;
    spnRepeats.Step = 1;
    ++row;

    if (auto* burst {std::get_if<particle_emitter::emit_burst>(&s.Pattern)}) {
        spnCount.Value    = burst->Count;
        spnInterval.Value = static_cast<f32>(burst->Interval.count());
        spnRepeats.Value  = static_cast<f32>(burst->Repeats);
    }

    auto syncBurst {[&s, &spnCount, &spnInterval, &spnRepeats, notify] {
        if (auto* burst {std::get_if<particle_emitter::emit_burst>(&s.Pattern)}) {
            burst->Count    = *spnCount.Value;
            burst->Interval = milliseconds {*spnInterval.Value};
            burst->Repeats  = static_cast<i32>(*spnRepeats.Value);
        }
        notify();
    }};
    spnCount.Value.Changed.connect([syncBurst](f32) { syncBurst(); });
    spnInterval.Value.Changed.connect([syncBurst](f32) { syncBurst(); });
    spnRepeats.Value.Changed.connect([syncBurst](f32) { syncBurst(); });

    auto updateVisibility {[&spnRate, &lblRate,
                            &spnCount, &lblCount,
                            &spnInterval, &lblInterval,
                            &spnRepeats, &lblRepeats](isize idx) {
        bool const isLinear {idx == 0};
        isLinear ? spnRate.show() : spnRate.hide();
        isLinear ? lblRate.show() : lblRate.hide();
        isLinear ? spnCount.hide() : spnCount.show();
        isLinear ? lblCount.hide() : lblCount.show();
        isLinear ? spnInterval.hide() : spnInterval.show();
        isLinear ? lblInterval.hide() : lblInterval.show();
        isLinear ? spnRepeats.hide() : spnRepeats.show();
        isLinear ? lblRepeats.hide() : lblRepeats.show();
    }};

    updateVisibility(*cyc.SelectedItemIndex);

    cyc.SelectedItemIndex.Changed.connect([&s, updateVisibility, notify](isize idx) {
        if (idx == 0) {
            s.Pattern = particle_emitter::emit_linear {};
        } else {
            s.Pattern = particle_emitter::emit_burst {};
        }
        updateVisibility(idx);
        notify();
    });
}

////////////////////////////////////////////////////////////

static void make_minmax_row(
    grid_layout& gl, i32 row,
    string const& lbl, f32 lo, f32 hi, f32 minVal, f32 maxVal, f32 step,
    auto&& onChanged, auto&& notify)
{
    auto& l {gl.create_widget<label>({{0, row}, {1, 1}}, "Lbl_" + lbl)};
    l.Label = lbl;

    auto& spnLo {gl.create_widget<spinner>({{1, row}, {1, 1}}, "SpnLo_" + lbl)};
    spnLo.Min   = minVal;
    spnLo.Max   = maxVal;
    spnLo.Step  = step;
    spnLo.Value = lo;

    auto& spnHi {gl.create_widget<spinner>({{2, row}, {1, 1}}, "SpnHi_" + lbl)};
    spnHi.Min   = minVal;
    spnHi.Max   = maxVal;
    spnHi.Step  = step;
    spnHi.Value = hi;

    spnLo.Value.Changed.connect([&spnHi, onChanged, notify](f32 v) {
        spnHi.Min = v;
        onChanged(v, *spnHi.Value);
        notify();
    });
    spnHi.Value.Changed.connect([&spnLo, onChanged, notify](f32 v) {
        onChanged(*spnLo.Value, v);
        notify();
    });
}

////////////////////////////////////////////////////////////

void main_ui::build_template_settings(panel& parent, isize emiIdx)
{
    parent.ScrollEnabled = true;
    auto  notify {[this, emiIdx] { this->notify(emiIdx); }};
    auto& t {_settings[emiIdx].Template};
    auto& gl {parent.create_layout<grid_layout>(size_i {0, 17}, true)};

    i32 row {0};

    // TextureRegion
    {
        auto& lbl {gl.create_widget<label>({{0, row}, {1, 1}}, "LblTexReg")};
        lbl.Label = "TexRegion";
        auto& ddl {gl.create_widget<drop_down_list>({{1, row}, {2, 1}}, "TexRegion")};
        ddl.Items.mutate([](auto& items) {
            items.push_back({"2x2"});
            items.push_back({"snowflake"});
            items.push_back({"particle"});
        });
        t.TextureRegion       = "2x2";
        ddl.SelectedItemIndex = 0;
        ddl.SelectedItemIndex.Changed.connect([&t, &ddl, notify](isize idx) {
            if (idx >= 0 && idx < static_cast<isize>(ddl.Items->size())) {
                t.TextureRegion = (*ddl.Items)[idx].Text;
                notify();
            }
        });
        ++row;
    }

    // Colors
    {
        auto& lbl {gl.create_widget<label>({{0, row}, {1, 1}}, "LblColors")};
        lbl.Label = "Colors";
        auto&  tb {gl.create_widget<text_box>({{1, row}, {2, 1}}, "Colors")};
        string colStr;
        for (auto const& c : t.Colors) {
            if (!colStr.empty()) { colStr += ','; }
            colStr += color::ToString(c);
        }
        tb.Text = colStr;
        auto parseColors {[&t, &tb, notify] {
            t.Colors.clear();
            string             token;
            std::istringstream ss {*tb.Text};
            while (std::getline(ss, token, ',')) {
                t.Colors.push_back(color::FromString(token));
            }
            notify();
        }};
        tb.Submit.connect([parseColors](auto const&) { parseColors(); });
        tb.FocusLost.connect([parseColors](auto const&) { parseColors(); });
        ++row;
    }

    // Size
    {
        auto& lbl {gl.create_widget<label>({{0, row}, {1, 1}}, "LblSizeW")};
        lbl.Label = "Size (W,H)";
        auto& spnW {gl.create_widget<spinner>({{1, row}, {1, 1}}, "SizeW")};
        spnW.Min     = 0;
        spnW.Max     = 1000;
        spnW.Step    = 1;
        spnW.Value   = t.Size.Width;
        t.Size.Width = *spnW.Value;
        spnW.Value.Changed.connect([&t, notify](f32 v) {
            t.Size.Width = v;
            notify();
        });

        auto& spnH {gl.create_widget<spinner>({{2, row}, {1, 1}}, "SizeH")};
        spnH.Min      = 0;
        spnH.Max      = 1000;
        spnH.Step     = 1;
        spnH.Value    = t.Size.Height;
        t.Size.Height = *spnH.Value;
        spnH.Value.Changed.connect([&t, notify](f32 v) {
            t.Size.Height = v;
            notify();
        });
        ++row;
    }

    make_minmax_row(gl, row++, "Speed", t.Speed.first, t.Speed.second, 0, 2000, 1, [&t](f32 lo, f32 hi) { t.Speed = std::minmax(lo, hi); }, notify);

    make_minmax_row(gl, row++, "Direction", t.Direction.first.Value, t.Direction.second.Value, -360, 360, 1, [&t](f32 lo, f32 hi) { t.Direction = std::minmax(degree_f {lo}, degree_f {hi}); }, notify);

    make_minmax_row(gl, row++, "LinAccel", t.LinearAcceleration.first, t.LinearAcceleration.second, -500, 500, 1, [&t](f32 lo, f32 hi) { t.LinearAcceleration = std::minmax(lo, hi); }, notify);

    make_minmax_row(gl, row++, "LinDamping", t.LinearDamping.first, t.LinearDamping.second, 0, 2, 0.01f, [&t](f32 lo, f32 hi) { t.LinearDamping = std::minmax(lo, hi); }, notify);

    make_minmax_row(gl, row++, "RadialAccel", t.RadialAcceleration.first, t.RadialAcceleration.second, -500, 500, 1, [&t](f32 lo, f32 hi) { t.RadialAcceleration = std::minmax(lo, hi); }, notify);

    make_minmax_row(gl, row++, "TanAccel", t.TangentialAcceleration.first, t.TangentialAcceleration.second, -500, 500, 1, [&t](f32 lo, f32 hi) { t.TangentialAcceleration = std::minmax(lo, hi); }, notify);

    make_minmax_row(gl, row++, "Gravity X", t.Gravity.first.X, t.Gravity.second.X, -500, 500, 1, [&t](f32 lo, f32 hi) { t.Gravity.first.X = lo; t.Gravity.second.X = hi; }, notify);

    make_minmax_row(gl, row++, "Gravity Y", t.Gravity.first.Y, t.Gravity.second.Y, -500, 500, 1, [&t](f32 lo, f32 hi) { t.Gravity.first.Y = lo; t.Gravity.second.Y = hi; }, notify);

    make_minmax_row(gl, row++, "Transparency", t.Transparency.first, t.Transparency.second, 0, 1, 0.01f, [&t](f32 lo, f32 hi) { t.Transparency = std::minmax(lo, hi); }, notify);

    make_minmax_row(gl, row++, "Lifetime(ms)", static_cast<f32>(t.Lifetime.first.count()), static_cast<f32>(t.Lifetime.second.count()), 0, 30000, 100, [&t](f32 lo, f32 hi) { t.Lifetime = std::minmax(milliseconds {lo}, milliseconds {hi}); }, notify);

    make_minmax_row(gl, row++, "Scale", t.Scale.first, t.Scale.second, 0, 10, 0.05f, [&t](f32 lo, f32 hi) { t.Scale = std::minmax(lo, hi); }, notify);

    make_minmax_row(gl, row++, "Spin", t.Spin.first.Value, t.Spin.second.Value, -360, 360, 1, [&t](f32 lo, f32 hi) { t.Spin = std::minmax(degree_f {lo}, degree_f {hi}); }, notify);

    make_minmax_row(gl, row++, "Rotation", t.Rotation.first.Value, t.Rotation.second.Value, -360, 360, 1, [&t](f32 lo, f32 hi) { t.Rotation = std::minmax(degree_f {lo}, degree_f {hi}); }, notify);
}

void main_ui::create_styles(assets::group const& resGrp)
{
    using namespace tcob::literals;
    using namespace std::chrono_literals;

    style_collection styles;

    {
        auto style {styles.create<button>("button", {})};
        style->Background        = color {30, 30, 35, 255};
        style->Border.Background = color {60, 60, 70, 255};
        style->Border.Type       = border_type::Solid;
        style->Border.Size       = 1_px;
        style->Border.Radius     = 3_px;
        style->DropShadow.Color  = color {0, 0, 0, 80};
        style->Text.Font         = resGrp.get<font_family>("Poppins");
        style->Text.Size         = 100_pct;
        style->Text.Color        = color {210, 210, 220, 255};
        style->Text.AutoSize     = auto_size_mode::OnlyShrink;
        style->Text.Alignment    = {.Horizontal = horizontal_alignment::Centered, .Vertical = vertical_alignment::Middle};
        style->Margin            = {3_px};
        style->Padding           = {2_px};
        style->EasingFunc        = easing_func::QuadOut;

        auto hoverStyle {styles.create<button>("button", {.Hover = true})};
        *hoverStyle                   = *style;
        hoverStyle->Background        = color {45, 45, 55, 255};
        hoverStyle->Border.Background = color {80, 150, 220, 255};

        auto focusStyle {styles.create<button>("button", {.Focus = true})};
        *focusStyle                   = *style;
        focusStyle->Border.Background = color {100, 170, 240, 255};

        auto focusHoverStyle {styles.create<button>("button", {.Focus = true, .Hover = true})};
        *focusHoverStyle            = *focusStyle;
        focusHoverStyle->Background = color {45, 45, 55, 255};

        auto activeStyle {styles.create<button>("button", {.Focus = true, .Active = true})};
        *activeStyle                   = *style;
        activeStyle->Background        = color {35, 70, 120, 255};
        activeStyle->Border.Background = color {80, 150, 220, 255};
        activeStyle->Margin            = {4_px, 4_px, 2_px, 2_px};
    }
    {
        auto style {styles.create<label>("label", {})};
        style->Text.Font      = resGrp.get<font_family>("Poppins");
        style->Text.Size      = 40_pct;
        style->Text.Color     = color {210, 210, 220, 255};
        style->Text.AutoSize  = auto_size_mode::OnlyShrink;
        style->Text.Alignment = {.Horizontal = horizontal_alignment::Left, .Vertical = vertical_alignment::Middle};
    }
    {
        auto style {styles.create<panel>("panel", {})};
        style->Background                       = color {22, 22, 28, 255};
        style->Border.Background                = color {60, 60, 70, 255};
        style->Border.Size                      = 1_px;
        style->Border.Radius                    = 3_px;
        style->Padding                          = {3_px};
        style->VScrollBar.ThumbClass            = "scrollbar_thumb";
        style->VScrollBar.Bar.Size              = 20_px;
        style->VScrollBar.Bar.Border.Size       = 1_px;
        style->VScrollBar.Bar.Border.Background = color {60, 60, 70, 255};
        style->VScrollBar.Bar.LowerBackground   = color {50, 100, 160, 255};
        style->VScrollBar.Bar.HigherBackground  = color {80, 150, 220, 255};
        style->VScrollBar.Bar.Delay             = 150ms;
        style->HScrollBar                       = style->VScrollBar;

        auto hoverStyle {styles.create<panel>("panel", {.Hover = true})};
        *hoverStyle                   = *style;
        hoverStyle->Border.Background = color {80, 150, 220, 255};
    }
    {
        auto style {styles.create<toggle>("toggle", {})};
        style->Background        = color {30, 30, 35, 255};
        style->Border.Background = color {60, 60, 70, 255};
        style->Border.Size       = 1_px;
        style->Border.Radius     = 12_px;
        style->DropShadow.Color  = color {0, 0, 0, 80};
        style->Margin            = {3_px};
        style->Padding           = {2_px};
        style->Tick.Type         = tick_type::Circle;
        style->Tick.Size         = 80_pct;
        style->Tick.Foreground   = color {80, 80, 90, 255};
        style->EasingFunc        = easing_func::QuadInOut;

        auto checkedStyle {styles.create<toggle>("toggle", {.Checked = true})};
        *checkedStyle                 = *style;
        checkedStyle->Tick.Size       = 100_pct;
        checkedStyle->Tick.Foreground = color {80, 200, 120, 255};

        auto hoverStyle {styles.create<toggle>("toggle", {.Hover = true})};
        *hoverStyle                   = *style;
        hoverStyle->Background        = color {45, 45, 55, 255};
        hoverStyle->Border.Background = color {80, 150, 220, 255};

        auto hoverCheckedStyle {styles.create<toggle>("toggle", {.Hover = true, .Checked = true})};
        *hoverCheckedStyle                 = *hoverStyle;
        hoverCheckedStyle->Tick.Size       = 100_pct;
        hoverCheckedStyle->Tick.Foreground = color {80, 200, 120, 255};

        auto focusStyle {styles.create<toggle>("toggle", {.Focus = true})};
        *focusStyle                   = *style;
        focusStyle->Border.Background = color {100, 170, 240, 255};

        auto focusCheckedStyle {styles.create<toggle>("toggle", {.Focus = true, .Checked = true})};
        *focusCheckedStyle                 = *focusStyle;
        focusCheckedStyle->Tick.Size       = 100_pct;
        focusCheckedStyle->Tick.Foreground = color {80, 200, 120, 255};

        auto focusHoverStyle {styles.create<toggle>("toggle", {.Focus = true, .Hover = true})};
        *focusHoverStyle            = *focusStyle;
        focusHoverStyle->Background = color {45, 45, 55, 255};

        auto focusHoverCheckedStyle {styles.create<toggle>("toggle", {.Focus = true, .Hover = true, .Checked = true})};
        *focusHoverCheckedStyle                 = *focusHoverStyle;
        focusHoverCheckedStyle->Tick.Size       = 100_pct;
        focusHoverCheckedStyle->Tick.Foreground = color {80, 200, 120, 255};

        auto activeStyle {styles.create<toggle>("toggle", {.Focus = true, .Active = true})};
        *activeStyle                   = *style;
        activeStyle->Background        = color {35, 70, 120, 255};
        activeStyle->Border.Background = color {80, 150, 220, 255};

        auto activeCheckedStyle {styles.create<toggle>("toggle", {.Focus = true, .Active = true, .Checked = true})};
        *activeCheckedStyle                 = *activeStyle;
        activeCheckedStyle->Tick.Size       = 100_pct;
        activeCheckedStyle->Tick.Foreground = color {80, 200, 120, 255};
    }
    {
        auto style {styles.create<spinner>("spinner", {})};
        style->Background        = color {30, 30, 35, 255};
        style->Border.Background = color {60, 60, 70, 255};
        style->Border.Size       = 1_px;
        style->Border.Radius     = 3_px;
        style->DropShadow.Color  = color {0, 0, 0, 80};
        style->Text.Font         = resGrp.get<font_family>("Poppins");
        style->Text.Size         = 40_pct;
        style->Text.AutoSize     = auto_size_mode::Always;
        style->Text.Color        = color {210, 210, 220, 255};
        style->Text.Alignment    = {.Horizontal = horizontal_alignment::Left, .Vertical = vertical_alignment::Middle};
        style->Margin            = {3_px};
        style->Padding           = {3_px};
        style->NavArrowClass     = "nav_arrows";

        auto hoverStyle {styles.create<spinner>("spinner", {.Hover = true})};
        *hoverStyle                   = *style;
        hoverStyle->Background        = color {45, 45, 55, 255};
        hoverStyle->Border.Background = color {80, 150, 220, 255};
    }
    {
        auto style {styles.create<text_box>("text_box", {})};
        style->Background        = color {30, 30, 35, 255};
        style->Border.Background = color {60, 60, 70, 255};
        style->Border.Type       = border_type::Solid;
        style->Border.Size       = 1_px;
        style->Border.Radius     = 3_px;
        style->DropShadow.Color  = color {0, 0, 0, 80};
        style->Text.Font         = resGrp.get<font_family>("Poppins");
        style->Text.Size         = 32_px;
        style->Text.AutoSize     = auto_size_mode::OnlyShrink;
        style->Text.Color        = color {210, 210, 220, 255};
        style->Text.Alignment    = {.Horizontal = horizontal_alignment::Left, .Vertical = vertical_alignment::Middle};
        style->Margin            = {3_px};
        style->Padding           = {3_px};
        style->Caret.BlinkRate   = 500ms;
        style->Caret.Color       = color {180, 180, 200, 255};

        auto hoverStyle {styles.create<text_box>("text_box", {.Hover = true})};
        *hoverStyle                   = *style;
        hoverStyle->Background        = color {45, 45, 55, 255};
        hoverStyle->Border.Background = color {80, 150, 220, 255};

        auto focusStyle {styles.create<text_box>("text_box", {.Focus = true})};
        *focusStyle                   = *style;
        focusStyle->Border.Background = color {100, 170, 240, 255};
    }
    {
        auto style {styles.create<drop_down_list>("drop_down_list", {})};
        style->Background                       = color {30, 30, 35, 255};
        style->Border.Background                = color {60, 60, 70, 255};
        style->Border.Size                      = 1_px;
        style->Border.Radius                    = 3_px;
        style->DropShadow.Color                 = color {0, 0, 0, 80};
        style->Text.Font                        = resGrp.get<font_family>("Poppins");
        style->Text.Size                        = 40_pct;
        style->Text.Color                       = color {210, 210, 220, 255};
        style->Text.Alignment                   = {.Horizontal = horizontal_alignment::Centered, .Vertical = vertical_alignment::Middle};
        style->ItemHeight                       = 130_pct;
        style->ItemClass                        = "list_items";
        style->VScrollBar.ThumbClass            = "scrollbar_thumb";
        style->VScrollBar.Bar.Size              = 20_pct;
        style->VScrollBar.Bar.Border.Size       = 1_px;
        style->VScrollBar.Bar.Border.Background = color {60, 60, 70, 255};
        style->VScrollBar.Bar.LowerBackground   = color {50, 100, 160, 255};
        style->VScrollBar.Bar.HigherBackground  = color {80, 150, 220, 255};
        style->VScrollBar.Bar.Delay             = 150ms;
        style->MaxVisibleItems                  = 1;
        style->Margin                           = {3_px};
        style->Padding                          = {3_px};

        auto hoverStyle {styles.create<drop_down_list>("drop_down_list", {.Hover = true})};
        *hoverStyle                   = *style;
        hoverStyle->Background        = color {45, 45, 55, 255};
        hoverStyle->Border.Background = color {80, 150, 220, 255};
    }
    {
        auto style {styles.create<cycle_button>("cycle_button", {})};
        style->Background            = color {30, 30, 35, 255};
        style->Border.Background     = color {60, 60, 70, 255};
        style->Border.Size           = 1_px;
        style->Border.Radius         = 3_px;
        style->DropShadow.Color      = color {0, 0, 0, 80};
        style->Bar.LowerBackground   = color {50, 100, 160, 255};
        style->Bar.HigherBackground  = color {80, 150, 220, 255};
        style->Bar.Border.Background = color {60, 60, 70, 255};
        style->Bar.Size              = 40_pct;
        style->Bar.Border.Size       = 1_px;
        style->Bar.Border.Radius     = 3_px;
        style->Bar.Delay             = 200ms;
        style->GapRatio              = 0.8f;
        style->Margin                = {3_px};
        style->Padding               = {3_px};
        style->EasingFunc            = easing_func::QuadIn;

        auto hoverStyle {styles.create<cycle_button>("cycle_button", {.Hover = true})};
        *hoverStyle                      = *style;
        hoverStyle->Background           = color {45, 45, 55, 255};
        hoverStyle->Border.Background    = color {80, 150, 220, 255};
        hoverStyle->Bar.LowerBackground  = color {40, 80, 130, 255};
        hoverStyle->Bar.HigherBackground = color {60, 120, 190, 255};
        hoverStyle->GapRatio             = 0.5f;

        auto activeStyle {styles.create<cycle_button>("cycle_button", {.Focus = true, .Active = true})};
        *activeStyle                   = *style;
        activeStyle->Background        = color {35, 70, 120, 255};
        activeStyle->Border.Background = color {80, 150, 220, 255};
    }
    {
        auto style {styles.create<tab_container>("tab_container", {})};
        style->Background        = color {22, 22, 28, 255};
        style->Border.Background = color {60, 60, 70, 255};
        style->Border.Size       = 1_px;
        style->Border.Radius     = 3_px;
        style->DropShadow.Color  = color {0, 0, 0, 80};
        style->Margin            = {3_px};
        style->Padding           = {3_px};
        style->Bar.Size          = 4_pct;
        style->TabItemClass      = "tab_items";
        style->Bar.Position      = position::Top;
        style->Bar.Mode          = tab_container::header_mode::Fill;
        style->Bar.Rows          = 1;

        auto hoverStyle {styles.create<tab_container>("tab_container", {.Hover = true})};
        *hoverStyle                   = *style;
        hoverStyle->Border.Background = color {80, 150, 220, 255};
    }
    {
        auto style {styles.create<thumb_style>("range_slider_thumb", {})};
        style->Thumb.Background        = color {80, 130, 200, 255};
        style->Thumb.Border.Background = color {60, 60, 70, 255};
        style->Thumb.Type              = thumb_type::Rect;
        style->Thumb.LongSide          = 12_pct;
        style->Thumb.ShortSide         = 100_pct;
        style->Thumb.Border.Size       = 1_px;
        style->Thumb.Border.Radius     = 2_px;

        auto hoverStyle {styles.create<thumb_style>("range_slider_thumb", {.Hover = true})};
        hoverStyle->Thumb            = style->Thumb;
        hoverStyle->Thumb.Background = color {100, 160, 230, 255};

        auto activeStyle {styles.create<thumb_style>("range_slider_thumb", {.Active = true})};
        activeStyle->Thumb            = style->Thumb;
        activeStyle->Thumb.Background = color {140, 190, 255, 255};
    }
    {
        auto style {styles.create<thumb_style>("scrollbar_thumb", {})};
        style->Thumb.Background        = color {80, 130, 200, 255};
        style->Thumb.Border.Background = color {60, 60, 70, 255};
        style->Thumb.Type              = thumb_type::Rect;
        style->Thumb.LongSide          = 30_pct;
        style->Thumb.ShortSide         = 75_pct;
        style->Thumb.Border.Size       = 1_px;

        auto hoverStyle {styles.create<thumb_style>("scrollbar_thumb", {.Hover = true})};
        hoverStyle->Thumb            = style->Thumb;
        hoverStyle->Thumb.Background = color {100, 160, 230, 255};

        auto activeStyle {styles.create<thumb_style>("scrollbar_thumb", {.Active = true})};
        activeStyle->Thumb            = style->Thumb;
        activeStyle->Thumb.Background = color {140, 190, 255, 255};
    }
    {
        auto style {styles.create<nav_arrows_style>("nav_arrows", {})};
        style->NavArrow.Foreground        = color {180, 180, 200, 255};
        style->NavArrow.UpBackground      = color {80, 150, 220, 255};
        style->NavArrow.DownBackground    = color {80, 150, 220, 255};
        style->NavArrow.Border.Background = color {60, 60, 70, 255};
        style->NavArrow.Size.Height       = {0.75f, length::type::Relative};
        style->NavArrow.Size.Width        = {0.10f, length::type::Relative};
        style->NavArrow.Border.Size       = 1_px;
        style->NavArrow.Border.Radius     = 2_px;
        style->NavArrow.Padding           = 1_px;

        auto hoverStyle {styles.create<nav_arrows_style>("nav_arrows", {.Hover = true})};
        hoverStyle->NavArrow                = style->NavArrow;
        hoverStyle->NavArrow.UpBackground   = color {120, 180, 255, 255};
        hoverStyle->NavArrow.DownBackground = color {120, 180, 255, 255};

        auto activeStyle {styles.create<nav_arrows_style>("nav_arrows", {.Active = true})};
        activeStyle->NavArrow                = style->NavArrow;
        activeStyle->NavArrow.UpBackground   = color {140, 200, 255, 255};
        activeStyle->NavArrow.DownBackground = color {140, 200, 255, 255};
    }
    {
        auto style {styles.create<item_style>("tab_items", {})};
        style->Item.Background        = color {40, 40, 48, 255};
        style->Item.Border.Background = color {60, 60, 70, 255};
        style->Item.Text.Color        = color {210, 210, 220, 255};
        style->Item.Padding           = {3_px};
        style->Item.Text.Font         = resGrp.get<font_family>("Poppins");
        style->Item.Text.Size         = 80_pct;
        style->Item.Text.AutoSize     = auto_size_mode::OnlyShrink;
        style->Item.Text.Alignment    = {.Horizontal = horizontal_alignment::Centered, .Vertical = vertical_alignment::Middle};
        style->Item.Border.Size       = 1_px;

        auto hoverStyle {styles.create<item_style>("tab_items", {.Hover = true})};
        hoverStyle->Item                   = style->Item;
        hoverStyle->Item.Background        = color {55, 55, 68, 255};
        hoverStyle->Item.Border.Background = color {80, 150, 220, 255};

        auto activeStyle {styles.create<item_style>("tab_items", {.Active = true})};
        activeStyle->Item                   = style->Item;
        activeStyle->Item.Background        = color {35, 70, 120, 255};
        activeStyle->Item.Border.Background = color {80, 150, 220, 255};
    }
    {
        auto style {styles.create<item_style>("items", {})};
        style->Item.Background        = color {40, 40, 48, 255};
        style->Item.Border.Background = color {60, 60, 70, 255};
        style->Item.Text.Color        = color {210, 210, 220, 255};
        style->Item.Padding           = {3_px};
        style->Item.Text.Font         = resGrp.get<font_family>("Poppins");
        style->Item.Text.Size         = 80_pct;
        style->Item.Text.AutoSize     = auto_size_mode::OnlyShrink;
        style->Item.Text.Alignment    = {.Horizontal = horizontal_alignment::Centered, .Vertical = vertical_alignment::Middle};
        style->Item.Border.Size       = 1_px;

        auto hoverStyle {styles.create<item_style>("items", {.Hover = true})};
        hoverStyle->Item                   = style->Item;
        hoverStyle->Item.Background        = color {55, 55, 68, 255};
        hoverStyle->Item.Border.Background = color {80, 150, 220, 255};

        auto activeStyle {styles.create<item_style>("items", {.Active = true})};
        activeStyle->Item                   = style->Item;
        activeStyle->Item.Background        = color {35, 70, 120, 255};
        activeStyle->Item.Border.Background = color {80, 150, 220, 255};
    }
    {
        auto style {styles.create<item_style>("list_items", {})};
        style->Item.Background        = color {40, 40, 48, 255};
        style->Item.Border.Background = color {60, 60, 70, 255};
        style->Item.Text.Color        = color {210, 210, 220, 255};
        style->Item.Padding           = {3_px};
        style->Item.Text.Font         = resGrp.get<font_family>("Poppins");
        style->Item.Text.Size         = 40_pct;
        style->Item.Text.Alignment    = {.Horizontal = horizontal_alignment::Left, .Vertical = vertical_alignment::Middle};
        style->Item.Border.Size       = 1_px;

        auto hoverStyle {styles.create<item_style>("list_items", {.Hover = true})};
        hoverStyle->Item                   = style->Item;
        hoverStyle->Item.Background        = color {55, 55, 68, 255};
        hoverStyle->Item.Border.Background = color {80, 150, 220, 255};

        auto activeStyle {styles.create<item_style>("list_items", {.Active = true})};
        activeStyle->Item                   = style->Item;
        activeStyle->Item.Background        = color {35, 70, 120, 255};
        activeStyle->Item.Border.Background = color {80, 150, 220, 255};
    }
    {
        auto style {styles.create<accordion>("accordion", {})};
        style->Background        = color {22, 22, 28, 255};
        style->Border.Background = color {60, 60, 70, 255};
        style->Border.Size       = 1_px;
        style->Border.Radius     = 3_px;
        style->DropShadow.Color  = color {0, 0, 0, 80};
        style->Padding           = {3_px};
        style->SectionBarHeight  = 5_pct;
        style->SectionItemClass  = "section_items";
        style->ExpandDuration    = 200ms;

        auto hoverStyle {styles.create<accordion>("accordion", {.Hover = true})};
        *hoverStyle                   = *style;
        hoverStyle->Border.Background = color {80, 150, 220, 255};
    }
    {
        auto style {styles.create<item_style>("section_items", {})};
        style->Item.Background        = color {35, 35, 45, 255};
        style->Item.Border.Background = color {60, 60, 70, 255};
        style->Item.Text.Color        = color {210, 210, 220, 255};
        style->Item.Padding           = {4_px};
        style->Item.Text.Font         = resGrp.get<font_family>("Poppins");
        style->Item.Text.Size         = 80_pct;
        style->Item.Text.AutoSize     = auto_size_mode::OnlyShrink;
        style->Item.Text.Alignment    = {.Horizontal = horizontal_alignment::Left, .Vertical = vertical_alignment::Middle};
        style->Item.Border.Size       = 1_px;

        auto hoverStyle {styles.create<item_style>("section_items", {.Hover = true})};
        hoverStyle->Item                   = style->Item;
        hoverStyle->Item.Background        = color {55, 55, 68, 255};
        hoverStyle->Item.Border.Background = color {80, 150, 220, 255};

        auto activeStyle {styles.create<item_style>("section_items", {.Active = true})};
        activeStyle->Item                   = style->Item;
        activeStyle->Item.Background        = color {35, 70, 120, 255};
        activeStyle->Item.Border.Background = color {80, 150, 220, 255};
    }

    Styles = styles;
}
