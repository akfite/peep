#include <peep/peep.hpp>

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#ifndef PEEP_PPM_DEMO_PATH
#define PEEP_PPM_DEMO_PATH "assets/cat.ppm"
#endif

struct Image {
    size_t width;
    size_t height;
    std::vector<std::uint8_t> rgb;
};

static std::string next_token(std::istream& in) {
    std::string token;
    while (in >> token) {
        if (!token.empty() && token[0] == '#') {
            std::string rest;
            std::getline(in, rest);
            continue;
        }
        return token;
    }
    throw std::runtime_error("unexpected end of PPM file");
}

static int next_int(std::istream& in) {
    return std::atoi(next_token(in).c_str());
}

static std::uint8_t scale_channel(int v, int max_value) {
    if (v < 0) v = 0;
    if (v > max_value) v = max_value;
    const long scaled = (static_cast<long>(v) * 255L + max_value / 2) / max_value;
    return static_cast<std::uint8_t>(scaled);
}

static Image load_ascii_ppm(const std::string& path) {
    std::ifstream in(path.c_str());
    if (!in) throw std::runtime_error("could not open " + path);

    if (next_token(in) != "P3") {
        throw std::runtime_error("expected an ASCII PPM (P3) file");
    }

    const int width = next_int(in);
    const int height = next_int(in);
    const int max_value = next_int(in);
    if (width <= 0 || height <= 0 || max_value <= 0) {
        throw std::runtime_error("invalid PPM header");
    }

    Image image;
    image.width = static_cast<size_t>(width);
    image.height = static_cast<size_t>(height);
    if (image.height > (std::numeric_limits<size_t>::max)() / image.width) {
        throw std::runtime_error("PPM dimensions are too large");
    }
    const size_t pixels = image.width * image.height;
    if (pixels > (std::numeric_limits<size_t>::max)() / 3) {
        throw std::runtime_error("PPM dimensions are too large");
    }

    image.rgb.reserve(pixels * 3);

    for (size_t i = 0; i < pixels * 3; ++i) {
        image.rgb.push_back(scale_channel(next_int(in), max_value));
    }

    return image;
}

int main(int argc, char** argv) {
    const std::string path = (argc > 1) ? argv[1] : PEEP_PPM_DEMO_PATH;

    try {
        Image image = load_ascii_ppm(path);
        peep::print(image.rgb, image.height, image.width,
            peep::Options()
                .rgb()
                .fit(peep::Fit::Off)
                .block_size(1)
                .title(path));
    } catch (const std::exception& e) {
        std::cerr << "ppm_demo: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
