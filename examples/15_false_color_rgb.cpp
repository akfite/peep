#include <peep/peep.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

static double clamp01(double v) {
    if (v < 0.0) return 0.0;
    if (v > 1.0) return 1.0;
    return v;
}

static std::uint8_t channel(double v) {
    return static_cast<std::uint8_t>(255.0 * clamp01(v) + 0.5);
}

int main() {
    const size_t rows = 42;
    const size_t cols = 84;
    std::vector<std::uint8_t> rgb(rows * cols * 3);

    for (size_t r = 0; r < rows; ++r) {
        const double y = -1.0 + 2.0 * static_cast<double>(r) / static_cast<double>(rows - 1);
        for (size_t c = 0; c < cols; ++c) {
            const double x = -1.0 + 2.0 * static_cast<double>(c) / static_cast<double>(cols - 1);
            const double vegetation = std::exp(-7.0 * ((x - 0.35) * (x - 0.35)
                                                     + (y + 0.25) * (y + 0.25)));
            const double elevation = 0.5 + 0.35 * x - 0.25 * y;
            const double moisture = 0.5 + 0.5 * std::sin(9.0 * (x * x + y * y));
            const size_t i = (r * cols + c) * 3;
            rgb[i + 0] = channel(vegetation);
            rgb[i + 1] = channel(elevation);
            rgb[i + 2] = channel(moisture);
        }
    }

    peep::print(rgb, rows, cols, peep::Options()
        .rgb()
        .title("false-color composite: vegetation/elevation/moisture"));
}
