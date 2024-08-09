// Copyright (c) 2024 Tobias Bohnen
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include "../shared/argparse.hpp"
#include "common.hpp"

void static list_formats()
{
    std::cout <<
        R"(
Supported file formats:

# image

- gif (read)
- pcx (read, write)
- png (read, write)
- pnm (read)
- qoi (read, write)
- tga (read, write)
- webp (read, write)
- bmp (read, write)
- bsi (read, write)

# config 

- ini (read, write)
- json (read, write)
- xml (read, write)
- yaml (read, write)
- bsbd (read, write)

# audio

- wav (read, write)
- flac (read)
- mp3 (read)
- vorbis (read, write)
- opus (read, write)
- midi (read)
- it (read)
- mod (read)
- s3m (read)
- xm (read)
- bsa (read, write)

# misc 

- rFXGen (.rfx) -> config, audio
- Bitmap Font Generator (.fnt) -> config

)";
}

auto main(int argc, char* argv[]) -> int
{
    io::magic::add_signature({{{0, {'r', 'F', 'X', ' '}}}, ".rfx", "misc"});
    io::magic::add_signature({{{0, {'B', 'M', 'F'}}}, ".fnt", "misc"});

    argparse::ArgumentParser program("cia_conv");
    program.add_argument("input");
    program.add_argument("output");
    program.add_argument("-l", "--list-formats")
        .help("list supported formats and exits")
        .flag()
        .action([&](auto const&) {
            list_formats();
            std::exit(0);
        });
    program.add_argument("--magic")
        .help("prints the detected input file format extension and exits")
        .default_value("")
        .nargs(1)
        .action([&](std::string const& file) {
            if (io::is_file(file)) {
                io::ifstream stream {file};
                std::cout << io::magic::get_extension(stream) << "\n";
            } else {
                print_error("file not found: " + file);
            }
            std::exit(0);
        });

#if defined(TCOB_ENABLE_ADDON_AUDIO_TINYSOUNDFONT)
    program.add_argument("-sf", "--sound-font")
        .help("SoundFont file for midi")
        .default_value("")
        .nargs(1);
#endif

    auto pl {platform::HeadlessInit(argv[0])};

    try {
        program.parse_args(argc, argv);
    } catch (std::exception const& err) {
        std::cout << err.what() << '\n';
        std::cout << program;
        return 1;
    }

    std::string const src {program.get("input")};
    std::string const dst {program.get("output")};
#if defined(TCOB_ENABLE_ADDON_AUDIO_TINYSOUNDFONT)
    std::string const ctx {program.get("-sf")};
#else
    std::string const ctx {""};
#endif

    if (!io::is_file(src)) {
        return print_error("file not found: " + src);
    }

    auto in {std::make_shared<io::ifstream>(src)};
    if (auto sig {io::magic::get_signature(*in)}) {
        if (sig->Group == "audio") {
            return convert_audio(in, src, sig->Extension, dst, ctx);
        }
        if (sig->Group == "image") {
            return convert_image(in, src, sig->Extension, dst);
        }
        if (sig->Group == "misc") {
            return convert_misc(in, src, sig->Extension, dst);
        }
    }

    return convert_config(in, src, io::get_extension(src), dst);
}
