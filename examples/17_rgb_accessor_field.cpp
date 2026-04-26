#include <peep/peep.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

struct Pixel {
    std::uint8_t red;
    std::uint8_t green;
    std::uint8_t blue;
};

static double clamp01(double v) {
    if (v < 0.0) return 0.0;
    if (v > 1.0) return 1.0;
    return v;
}

static std::uint8_t channel(double v) {
    return static_cast<std::uint8_t>(255.0 * clamp01(v) + 0.5);
}

int main() {
    const size_t rows = 36;
    const size_t cols = 72;
    std::vector<Pixel> field(rows * cols);

    for (size_t r = 0; r < rows; ++r) {
        const double y = -1.0 + 2.0 * static_cast<double>(r) / static_cast<double>(rows - 1);
        for (size_t c = 0; c < cols; ++c) {
            const double x = -1.0 + 2.0 * static_cast<double>(c) / static_cast<double>(cols - 1);
            const double u = -y + 0.35 * std::sin(5.0 * x);
            const double v = x + 0.35 * std::cos(5.0 * y);
            const double speed = std::sqrt(u * u + v * v);
            field[r * cols + c] = Pixel{
                channel(0.5 + 0.35 * u),
                channel(speed / 1.8),
                channel(0.5 + 0.35 * v)
            };
        }
    }

    peep::print(rows, cols, peep::Options()
        .rgb_accessor([&](size_t r, size_t c) {
            const Pixel& p = field[r * cols + c];
            return peep::Color{p.red, p.green, p.blue};
        })
        .title("RGB accessor over a vector-field pixel buffer"));
}
