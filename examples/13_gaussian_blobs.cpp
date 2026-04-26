#include <peep/peep.hpp>

#include <cmath>
#include <cstddef>
#include <vector>

static double gaussian(double x, double y, double mx, double my,
                       double sx, double sy, double rho) {
    const double zx = (x - mx) / sx;
    const double zy = (y - my) / sy;
    const double q = (zx * zx - 2.0 * rho * zx * zy + zy * zy)
                   / (2.0 * (1.0 - rho * rho));
    return std::exp(-q) / (sx * sy * std::sqrt(1.0 - rho * rho));
}

int main() {
    const size_t rows = 60;
    const size_t cols = 100;
    std::vector<double> density(rows * cols);

    for (size_t r = 0; r < rows; ++r) {
        const double y = -3.0 + 6.0 * static_cast<double>(r) / static_cast<double>(rows - 1);
        for (size_t c = 0; c < cols; ++c) {
            const double x = -4.0 + 8.0 * static_cast<double>(c) / static_cast<double>(cols - 1);
            density[r * cols + c] =
                0.45 * gaussian(x, y, -1.8, -0.9, 0.8, 0.5, 0.45)
              + 0.35 * gaussian(x, y, 1.5, 0.6, 1.0, 0.7, -0.25)
              + 0.20 * gaussian(x, y, 0.2, 1.8, 0.5, 0.4, 0.0);
        }
    }

    peep::print(density, rows, cols, peep::Options()
        .colormap("turbo")
        .title("Gaussian mixture density estimate"));
}
