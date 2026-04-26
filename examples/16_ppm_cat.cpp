#include <peep/peep.hpp>

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#ifndef PEEP_PPM_CAT_PATH
#define PEEP_PPM_CAT_PATH "assets/cat.ppm"
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

static std::int32_t next_int(std::istream& in) {
    return static_cast<std::int32_t>(std::stoi(next_token(in)));
}

static std::uint8_t scale_channel(std::int32_t v, std::int32_t max_value) {
    if (v < 0) v = 0;
    if (v > max_value) v = max_value;
    const std::int64_t scaled =
        static_cast<std::int64_t>(v) * 255 + max_value / 2;
    return static_cast<std::uint8_t>(scaled / max_value);
}

static Image load_ppm(const std::string& path) {
    std::ifstream in(path.c_str());
    if (!in) throw std::runtime_error("could not open " + path);

    if (next_token(in) != "P3") {
        throw std::runtime_error("only plain-text P3 PPM files are supported");
    }

    const std::int32_t width = next_int(in);
    const std::int32_t height = next_int(in);
    const std::int32_t max_value = next_int(in);
    if (width <= 0 || height <= 0 || max_value <= 0) {
        throw std::runtime_error("invalid PPM header");
    }

    Image img;
    img.width = static_cast<size_t>(width);
    img.height = static_cast<size_t>(height);
    img.rgb.resize(img.width * img.height * 3);

    for (size_t i = 0; i < img.rgb.size(); ++i) {
        img.rgb[i] = scale_channel(next_int(in), max_value);
    }
    return img;
}

int main(int argc, char** argv) {
    try {
        const std::string path = (argc > 1) ? argv[1] : PEEP_PPM_CAT_PATH;
        const Image image = load_ppm(path);
        peep::print(image.rgb, image.height, image.width, peep::Options()
            .rgb()
            .fit(peep::Fit::Off)
            .block_size(1)
            .title(path));
    } catch (const std::exception& e) {
        std::cerr << "16_ppm_cat: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
