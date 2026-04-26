#include <peep/peep.hpp>

#include <cmath>
#include <cstddef>
#include <vector>

static double peaks(double x, double y) {
    return 3.0 * (1.0 - x) * (1.0 - x)
             * std::exp(-x * x - (y + 1.0) * (y + 1.0))
         - 10.0 * (x / 5.0 - x * x * x - y * y * y * y * y)
             * std::exp(-x * x - y * y)
         - (1.0 / 3.0) * std::exp(-(x + 1.0) * (x + 1.0) - y * y);
}

int main() {
    const size_t n = 180;
    std::vector<double> surface(n * n);

    for (size_t r = 0; r < n; ++r) {
        const double y = -3.0 + 6.0 * static_cast<double>(r) / static_cast<double>(n - 1);
        for (size_t c = 0; c < n; ++c) {
            const double x = -3.0 + 6.0 * static_cast<double>(c) / static_cast<double>(n - 1);
            surface[r * n + c] = peaks(x, y);
        }
    }

    peep::print(surface, n, n, peep::Options()
        .colormap("viridis")
        .title("MATLAB peaks surface"));
}
