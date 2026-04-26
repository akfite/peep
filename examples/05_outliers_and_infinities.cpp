#include <peep/peep.hpp>

#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

int main() {
    const size_t rows = 10;
    const size_t cols = 20;
    std::vector<double> rate(rows * cols);
    const double inf = std::numeric_limits<double>::infinity();

    for (size_t r = 0; r < rows; ++r) {
        for (size_t c = 0; c < cols; ++c) {
            const double hour = static_cast<double>(c);
            const double sensor = static_cast<double>(r);
            rate[r * cols + c] = 52.0
                + 14.0 * std::sin(hour * 0.38)
                + 6.0 * std::cos(sensor * 0.7);
        }
    }

    // Selected pixels are listed as (x, y), where x is column and y is row.
    rate[2 * cols + 5] = -inf;   // (5, 2): detector underflow
    rate[4 * cols + 16] = inf;   // (16, 4): saturated spike
    rate[8 * cols + 11] = 135.0; // (11, 8): finite outlier clipped by clim

    peep::print(rate, rows, cols, peep::Options()
        .colormap("cividis")
        .clim(25.0, 85.0)
        .inf_colors(peep::Color{0, 80, 255}, peep::Color{255, 40, 20})
        .title("special pixels: (5,2)=-inf, (16,4)=+inf, (11,8)=135"));
}
