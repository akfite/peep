#include <peep/peep.hpp>

#include <cmath>
#include <cstddef>
#include <iostream>
#include <vector>

static double gaussian(double x, double y, double cx, double cy, double sigma) {
    const double dx = x - cx;
    const double dy = y - cy;
    return std::exp(-(dx * dx + dy * dy) / (2.0 * sigma * sigma));
}

int main() {
    const size_t rows = 64;
    const size_t cols = 96;
    std::vector<double> residual(rows * cols);

    for (size_t r = 0; r < rows; ++r) {
        for (size_t c = 0; c < cols; ++c) {
            const double x = static_cast<double>(c);
            const double y = static_cast<double>(r);
            residual[r * cols + c] =
                0.35 * std::sin(x * 0.16)
              + 0.25 * std::cos(y * 0.21)
              + 2.8 * gaussian(x, y, 66.0, 31.0, 6.0)
              - 2.1 * gaussian(x, y, 24.0, 48.0, 7.5);
        }
    }

    peep::show(residual, rows, cols, peep::Options()
        .colormap("coolwarm")
        .clim(-3.0, 3.0)
        .title("full residual field"));

    std::cout << '\n';

    peep::show(residual, rows, cols, peep::Options()
        .colormap("coolwarm")
        .clim(-3.0, 3.0)
        .crop(52, 20, 32, 24)
        .title("crop around positive anomaly"));

    std::cout << '\n';

    peep::show(residual, rows, cols, peep::Options()
        .colormap("coolwarm")
        .clim(-3.0, 3.0)
        .center_crop(24, 48, 28, 20)
        .title("center crop around negative anomaly"));
}
