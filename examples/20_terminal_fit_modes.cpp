#include <peep/peep.hpp>

#include <cmath>
#include <cstddef>
#include <iostream>
#include <vector>

int main() {
    const size_t rows = 96;
    const size_t cols = 192;
    std::vector<double> surface(rows * cols);

    for (size_t r = 0; r < rows; ++r) {
        const double y = -3.0 + 6.0 * static_cast<double>(r) / static_cast<double>(rows - 1);
        for (size_t c = 0; c < cols; ++c) {
            const double x = -6.0 + 12.0 * static_cast<double>(c) / static_cast<double>(cols - 1);
            surface[r * cols + c] = std::sin(x) * std::cos(y)
                                  + 0.25 * std::sin(3.0 * x + y);
        }
    }

    peep::show(surface, rows, cols, peep::Options()
        .colormap("turbo")
        .fit(peep::Fit::Resample)
        .title("Fit::Resample for a large analytic surface"));

    std::cout << '\n';

    peep::show(surface, rows, cols, peep::Options()
        .colormap("turbo")
        .fit(peep::Fit::Trim)
        .title("Fit::Trim for the same large surface"));

    std::cout << '\n';

    peep::show(surface, rows, cols, peep::Options()
        .colormap("turbo")
        .fit(peep::Fit::Off)
        .crop(64, 32, 64, 32)
        .title("Fit::Off on a safe exact-size crop"));
}
