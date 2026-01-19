// Copyright (c) 2026 Tobias Bohnen
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include "../shared/argparse.hpp"

#include <tcob/tcob.hpp>

using namespace tcob;
namespace io = tcob::io;
using namespace tcob::gfx;

auto print_error(std::string const& err) -> int
{
    std::cout << err;
    return 1;
}

auto main(int argc, char* argv[]) -> int
{
    argparse::ArgumentParser program("quant");
    // Positional arguments
    program.add_argument("input")
        .help("input image file path")
        .metavar("INPUT");

    program.add_argument("output")
        .help("output image file path")
        .metavar("OUTPUT");

    // Optional arguments with flags
    program.add_argument("-c", "--colors")
        .help("number of colors in output palette")
        .default_value(256)
        .scan<'i', i32>()
        .metavar("N");

    program.add_argument("-q", "--quantizer")
        .help("quantization algorithm")
        .default_value("neuquant")
        .choices("neuquant", "octree")
        .metavar("ALGO");

    program.add_argument("-d", "--dithering")
        .help("dithering algorithm")
        .default_value("none")
        .choices("none", "floyd-steinberg", "ordered", "atkinson")
        .metavar("ALGO");

    auto pl {platform::HeadlessInit()};

    try {
        program.parse_args(argc, argv);
    } catch (std::exception const& err) {
        std::cout << err.what() << '\n';
        std::cout << program;
        return 1;
    }

    std::string const input {program.get<std::string>("input")};
    std::string const output {program.get<std::string>("output")};
    i32 const         colors {program.get<i32>("--colors")};
    std::string const quantizer {program.get<std::string>("--quantizer")};
    std::string const dithering {program.get<std::string>("--dithering")};

    if (!io::is_file(input)) { return print_error("file not found: " + input); }

    auto in {std::make_shared<io::ifstream>(input)};
    auto sig {io::magic::get_signature(*in)};
    if (!sig) { return print_error("invalid file: " + input); }
    std::cout << std::format("converting image to {} colors: {} to {} \n", colors, input, output);

    in->seek(0, io::seek_dir::Begin);

    image img;
    if (!img.load(*in, sig->Extension)) { return print_error("error loading image: " + input); }

    auto const& info {img.info()};
    std::cout << std::format("source info: BPP: {}, Width: {}, Height: {} \n", (info.Format == image::format::RGBA ? 4 : 3), info.Size.Width, info.Size.Height);
    std::cout << std::format("Old color count:{}\n", img.count_colors());

    stopwatch sw {stopwatch::StartNew()};

    auto const doQuant {[&]<typename T>(T&& quant) { // NOLINT
        image newImg;

        if (dithering == "ordered") {
            newImg = ordered_dither {T::GetPalette(img, colors)}(img);
        } else if (dithering == "atkinson") {
            newImg = atkinson_dither {T::GetPalette(img, colors)}(img);
        } else if (dithering == "floyd-steinberg") {
            newImg = floyd_steinberg_dither {T::GetPalette(img, colors)}(img);
        } else {
            newImg = quant(img);
        }

        std::cout << std::format("New color count:{}\nUsed palette:\n", newImg.count_colors());
        auto const ms {sw.elapsed_milliseconds()};

        if (newImg.save(output)) {
            std::cout << std::format("done in {}ms!\n", ms);
            return 0;
        }
        return print_error("error saving image: " + output);
    }};

    if (quantizer == "neuquant") { return doQuant(neuquant {colors}); }
    return doQuant(octree_quantizer {colors});
}
