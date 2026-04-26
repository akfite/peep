#include <peep/peep.hpp>

#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

struct PixelPair {
    size_t x;
    size_t y;
};

int main() {
    const size_t rows = 12;
    const size_t cols = 24;
    std::vector<double> instrument(rows * cols);
    const double nan = std::numeric_limits<double>::quiet_NaN();

    for (size_t r = 0; r < rows; ++r) {
        for (size_t c = 0; c < cols; ++c) {
            const double x = static_cast<double>(c) / static_cast<double>(cols - 1);
            const double y = static_cast<double>(r) / static_cast<double>(rows - 1);
            const double drift = 0.25 * x + 0.15 * y;
            const double peak = std::exp(-35.0 * ((x - 0.62) * (x - 0.62)
                                                + (y - 0.42) * (y - 0.42)));
            instrument[r * cols + c] = 0.25 + drift + 0.55 * peak;
        }
    }

    // Missing pixels are listed as (x, y), where x is column and y is row.
    const PixelPair missing_pixels[] = {
        {4, 2},
        {12, 5},
        {19, 8},
        {7, 10}
    };

    for (size_t i = 0; i < sizeof(missing_pixels) / sizeof(missing_pixels[0]); ++i) {
        const PixelPair p = missing_pixels[i];
        instrument[p.y * cols + p.x] = nan;
    }

    peep::show(instrument, rows, cols, peep::Options()
        .colormap("gray")
        .clim(0.0, 1.0)
        .nan_color(255, 20, 147)
        .title("NaNs at (x,y): (4,2), (12,5), (19,8), (7,10)"));
}
