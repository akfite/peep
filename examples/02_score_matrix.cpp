#include <peep/peep.hpp>

#include <cmath>
#include <cstddef>
#include <vector>

static double logistic(double x) {
    return 1.0 / (1.0 + std::exp(-x));
}

int main() {
    const size_t rows = 8;
    const size_t cols = 8;
    std::vector<double> scores(rows * cols);

    for (size_t r = 0; r < rows; ++r) {
        for (size_t c = 0; c < cols; ++c) {
            const double feature = 0.8 * static_cast<double>(r)
                                 - 0.6 * static_cast<double>(c)
                                 + std::sin(static_cast<double>(r + c));
            scores[r * cols + c] = logistic((feature - 1.5) / 2.0);
        }
    }

    peep::show(scores, rows, cols, peep::Options()
        .colormap("magma")
        .clim(0.0, 1.0)
        .title("calibrated risk score matrix"));
}
