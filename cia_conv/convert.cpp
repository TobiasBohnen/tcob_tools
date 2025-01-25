// Copyright (c) 2025 Tobias Bohnen
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include "common.hpp"

auto convert_config(std::shared_ptr<io::ifstream>& in, std::string const& src, std::string const& srcExt, std::string const& dst) -> int
{
    using namespace tcob::data::config;
    std::cout << "converting config file: " << src << " to " << dst << "\n";

    in->seek(0, io::seek_dir::Begin);

    object obj;
    if (obj.load(*in, srcExt) == load_status::Error) {
        return print_error("error loading config: " + src);
    }

    if (!obj.save(dst)) {
        return print_error("error saving config: " + dst);
    }

    std::cout << "done!\n";
    return 0;
}

auto convert_image(std::shared_ptr<io::ifstream>& in, std::string const& src, std::string const& srcExt, std::string const& dst) -> int
{
    using namespace tcob::gfx;
    std::cout << "converting image: " << src << " to " << dst << "\n";

    in->seek(0, io::seek_dir::Begin);

    image img;
    if (img.load(*in, srcExt) == load_status::Error) {
        return print_error("error loading image: " + src);
    }

    auto const& info {img.info()};
    std::cout << std::format("source info: BPP: {}, Width: {}, Height: {} \n",
                             (info.Format == image::format::RGBA ? 4 : 3), info.Size.Width, info.Size.Width);

    if (!img.save(dst)) {
        return print_error("error saving image: " + dst);
    }

    std::cout << "done!\n";
    return 0;
}

auto convert_audio(std::shared_ptr<io::ifstream>& in, std::string const& src, std::string const& srcExt, std::string const& dst, std::string const& ctx) -> int
{
    using namespace tcob::audio;
    std::cout << "converting audio: " << src << " to " << dst << "\n";

    in->seek(0, io::seek_dir::Begin);

    std::any context {0};

#if defined(TCOB_ENABLE_ADDON_AUDIO_TINYSOUNDFONT)
    assets::owning_asset_ptr<sound_font> sf;
    if (!ctx.empty()) {
        if (sf->load(ctx) != load_status::Ok) {
            return print_error("error loading sound font: " + ctx);
        }
        context = assets::asset_ptr<sound_font> {sf};
    }
#endif

    buffer bfr;
    if (bfr.load(in, srcExt, context) == load_status::Error) {
        return print_error("error loading image: " + src);
    }

    auto const& info {bfr.info()};
    std::cout << std::format("source info: Channels: {}, Frames: {}, Sample Rate: {} \n",
                             info.Channels, info.FrameCount, info.SampleRate);

    if (!bfr.save(dst)) {
        return print_error("error saving image: " + dst);
    }

    std::cout << "done!\n";
    return 0;
}

auto static convert_rfx(std::shared_ptr<io::ifstream>& in, std::string const& src, std::string const& dst) -> int
{
    std::cout << "converting rfx: " << src << " to " << dst << "\n";

    struct RFXFile {
        std::array<byte, 4> signature;
        u16                 version;
        u16                 length;
        i32                 randSeed;
        i32                 waveTypeValue;
        f32                 attackTimeValue;
        f32                 sustainTimeValue;
        f32                 sustainPunchValue;
        f32                 decayTimeValue;
        f32                 startFrequencyValue;
        f32                 minFrequencyValue;
        f32                 slideValue;
        f32                 deltaSlideValue;
        f32                 vibratoDepthValue;
        f32                 vibratoSpeedValue;
        f32                 changeAmountValue;
        f32                 changeSpeedValue;
        f32                 squareDutyValue;
        f32                 dutySweepValue;
        f32                 repeatSpeedValue;
        f32                 phaserOffsetValue;
        f32                 phaserSweepValue;
        f32                 lpfCutoffValue;
        f32                 lpfCutoffSweepValue;
        f32                 lpfResonanceValue;
        f32                 hpfCutoffValue;
        f32                 hpfCutoffSweepValue;
    };

    RFXFile const rfx {in->read<RFXFile>()};

    if ((rfx.signature != std::array<byte, 4> {'r', 'F', 'X', ' '}) || rfx.version != 200 || rfx.length != 96) {
        return print_error("unsupported rfx file: " + src);
    }

    audio::sound_wave wave {
        .RandomSeed                = static_cast<u64>(rfx.randSeed),
        .WaveType                  = static_cast<audio::sound_wave::type>(rfx.waveTypeValue),
        .AttackTime                = rfx.attackTimeValue,
        .SustainTime               = rfx.sustainTimeValue,
        .SustainPunch              = rfx.sustainPunchValue,
        .DecayTime                 = rfx.decayTimeValue,
        .StartFrequency            = rfx.startFrequencyValue,
        .MinFrequency              = rfx.minFrequencyValue,
        .Slide                     = rfx.slideValue,
        .DeltaSlide                = rfx.deltaSlideValue,
        .VibratoDepth              = rfx.vibratoDepthValue,
        .VibratoSpeed              = rfx.vibratoSpeedValue,
        .ChangeAmount              = rfx.changeAmountValue,
        .ChangeSpeed               = rfx.changeSpeedValue,
        .SquareDuty                = rfx.squareDutyValue,
        .DutySweep                 = rfx.dutySweepValue,
        .RepeatSpeed               = rfx.repeatSpeedValue,
        .PhaserOffset              = rfx.phaserOffsetValue,
        .PhaserSweep               = rfx.phaserSweepValue,
        .LowPassFilterCutoff       = rfx.lpfCutoffValue,
        .LowPassFilterCutoffSweep  = rfx.lpfCutoffSweepValue,
        .LowPassFilterResonance    = rfx.lpfResonanceValue,
        .HighPassFilterCutoff      = rfx.hpfCutoffValue,
        .HighPassFilterCutoffSweep = rfx.hpfCutoffSweepValue};

    auto dstGroup(io::magic::get_group(io::get_extension(dst)));
    if (dstGroup == "config") {
        data::config::object obj;
        Serialize(wave, obj["wave"]);

        if (!obj.save(dst)) {
            return print_error("error saving rfx config: " + dst);
        }

        std::cout << "done!\n";
        return 0;
    }

    if (dstGroup == "audio") {
        audio::sound_generator gen;
        if (!gen.create_buffer(wave).save(dst)) {
            return print_error("error saving rfx audio: " + dst);
        }

        std::cout << "done!\n";
        return 0;
    }

    return print_error("unsupported convert target format: " + dst);
}

auto convert_misc(std::shared_ptr<io::ifstream>& in, std::string const& src, std::string const& srcExt, std::string const& dst) -> i32
{
    if (srcExt == ".rfx") {
        return convert_rfx(in, src, dst);
    }
    return print_error("unsupported file: " + src);
}
