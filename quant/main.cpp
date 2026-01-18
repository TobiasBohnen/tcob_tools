// Copyright (c) 2026 Tobias Bohnen
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include "../shared/argparse.hpp"

#include <tcob/tcob.hpp>

using namespace tcob;
namespace io = tcob::io;

auto print_error(std::string const& err) -> int
{
    std::cout << err;
    return 1;
}

auto main(int argc, char* argv[]) -> int
{
    argparse::ArgumentParser program("quant");
    program.add_argument("input");
    program.add_argument("output");
    program.add_argument("ncol")
        .scan<'i', i32>();

    auto pl {platform::HeadlessInit()};

    try {
        program.parse_args(argc, argv);
    } catch (std::exception const& err) {
        std::cout << err.what() << '\n';
        std::cout << program;
        return 1;
    }

    std::string const src {program.get("input")};
    std::string const dst {program.get("output")};
    auto const        ncol {program.get<i32>("ncol")};

    if (!io::is_file(src)) {
        return print_error("file not found: " + src);
    }

    auto in {std::make_shared<io::ifstream>(src)};
    auto sig {io::magic::get_signature(*in)};
    if (!sig) {
        return print_error("invalid file: " + src);
    }

    using namespace tcob::gfx;
    std::cout << std::format("converting image to {} colors: {} to {} \n", ncol, src, dst);

    in->seek(0, io::seek_dir::Begin);

    image img;
    if (!img.load(*in, sig->Extension)) {
        return print_error("error loading image: " + src);
    }

    auto const& info {img.info()};
    std::cout << std::format("source info: BPP: {}, Width: {}, Height: {} \n",
                             (info.Format == image::format::RGBA ? 4 : 3), info.Size.Width, info.Size.Height);

    octree_quantizer quant {ncol};
    auto const       newImg {quant(img)};
    std::cout << "quant done!\n";
    if (!newImg.save(dst)) {
        return print_error("error saving image: " + dst);
    }

    std::cout << "done!\n";
    return 0;
}
