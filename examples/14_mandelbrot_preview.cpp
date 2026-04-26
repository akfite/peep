#include <peep/peep.hpp>

#include <cstddef>
#include <vector>

int main() {
    const size_t rows = 48;
    const size_t cols = 96;
    const size_t max_iter = 80;
    std::vector<double> iter(rows * cols);

    for (size_t r = 0; r < rows; ++r) {
        const double ci = -1.15 + 2.3 * static_cast<double>(r) / static_cast<double>(rows - 1);
        for (size_t c = 0; c < cols; ++c) {
            const double cr = -2.2 + 3.2 * static_cast<double>(c) / static_cast<double>(cols - 1);
            double zr = 0.0;
            double zi = 0.0;
            size_t k = 0;
            while (k < max_iter && zr * zr + zi * zi <= 4.0) {
                const double next_r = zr * zr - zi * zi + cr;
                zi = 2.0 * zr * zi + ci;
                zr = next_r;
                ++k;
            }
            iter[r * cols + c] = static_cast<double>(k);
        }
    }

    peep::show(iter, rows, cols, peep::Options()
        .colormap("turbo")
        .clim(0.0, static_cast<double>(max_iter))
        .title("Mandelbrot escape iterations"));
}
