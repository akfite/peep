#include <peep/peep.hpp>

#include <cmath>
#include <cstddef>
#include <vector>

int main() {
    const size_t rows = 48;
    const size_t cols = 80;
    std::vector<double> residual(rows * cols);

    for (size_t r = 0; r < rows; ++r) {
        for (size_t c = 0; c < cols; ++c) {
            const double x = -1.0 + 2.0 * static_cast<double>(c) / static_cast<double>(cols - 1);
            const double y = -1.0 + 2.0 * static_cast<double>(r) / static_cast<double>(rows - 1);
            const double observed = 1.5 * x - 0.7 * y
                                  + 0.35 * std::sin(8.0 * x) * std::cos(6.0 * y);
            const double linear_model = 1.5 * x - 0.7 * y;
            residual[r * cols + c] = observed - linear_model;
        }
    }

    peep::print(residual, rows, cols, peep::Options()
        .colormap("coolwarm")
        .clim(-0.4, 0.4)
        .title("model residuals after fitting only the linear trend"));
}
