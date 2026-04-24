#include "termimage.h"

#include <cstddef>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

int main() {
    // 1. Colormap gradients
    std::cout << "=== Colormap gradients ===" << std::endl;
    {
        struct ColormapDemo {
            const char* name;
            termimage::Colormap colormap;
        };

        const ColormapDemo colormaps[] = {
            {"gray", termimage::Colormap::Gray},
            {"viridis", termimage::Colormap::Viridis},
            {"plasma", termimage::Colormap::Plasma},
            {"inferno", termimage::Colormap::Inferno},
            {"magma", termimage::Colormap::Magma},
            {"cividis", termimage::Colormap::Cividis},
            {"coolwarm", termimage::Colormap::Coolwarm},
            {"gnuplot", termimage::Colormap::Gnuplot},
            {"turbo", termimage::Colormap::Turbo},
        };

        const int cols = 64;
        std::vector<double> grad(cols);
        for (int c = 0; c < cols; ++c)
            grad[c] = static_cast<double>(c) / (cols - 1);

        for (size_t i = 0; i < sizeof(colormaps) / sizeof(colormaps[0]); ++i) {
            std::cout << colormaps[i].name << std::endl;
            termimage::print(grad, 1, cols,
                termimage::Options().colormap(colormaps[i].colormap));
        }
    }
    std::cout << std::endl;

    // 2. Square 2D Gaussian (magma, block_size=2)
    std::cout << "=== 2D Gaussian (magma, block_size=2) ===" << std::endl;
    {
        const int n = 24;
        std::vector<double> gauss(n * n);
        double center = n / 2.0;
        double sigma = n / 5.0;
        for (int r = 0; r < n; ++r)
            for (int c = 0; c < n; ++c)
                gauss[r * n + c] = std::exp(
                    -0.5 * ((c - center) * (c - center)
                          + (r - center) * (r - center))
                    / (sigma * sigma));
        termimage::print(gauss, n, n,
            termimage::Options().colormap("magma").block_size(2));
    }
    std::cout << std::endl;

    // 3. Checkerboard with NaN holes (viridis)
    std::cout << "=== Checkerboard + NaN (viridis) ===" << std::endl;
    {
        const int rows = 10, cols = 20;
        double nan = std::numeric_limits<double>::quiet_NaN();
        std::vector<double> board(rows * cols);
        for (int r = 0; r < rows; ++r)
            for (int c = 0; c < cols; ++c) {
                if ((r + c) % 7 == 0)
                    board[r * cols + c] = nan;
                else
                    board[r * cols + c] = static_cast<double>((r + c) % 5) / 4.0;
            }
        termimage::print(board, rows, cols,
            termimage::Options().colormap("viridis"));
    }
    std::cout << std::endl;

    // 4. 8x8 integer ramp, cropped to a 4x6 window starting at (2,1)
    std::cout << "=== 8x8 integers, cropped to (2,1) 4x6 (gray, block_size=3) ===" << std::endl;
    {
        int mat[64];
        for (int i = 0; i < 64; ++i)
            mat[i] = i;
        termimage::print(mat, 8, 8,
            termimage::Options().crop(2, 1, 4, 6).block_size(3));
    }
    std::cout << std::endl;

    // 5. Inf handling with explicit clim
    std::cout << "=== Inf handling (magma, block_size=3) ===" << std::endl;
    {
        double inf = std::numeric_limits<double>::infinity();
        double data[] = {
            -inf, 0.0, 0.25, 0.5,
            0.75, 1.0, inf, 0.5
        };
        termimage::print(data, 2, 4,
            termimage::Options().colormap("magma").clim(0.0, 1.0).block_size(3));
    }
    std::cout << std::endl;

    // 6. MATLAB peaks — oversized to demonstrate terminal auto-fit (resample).
    //    The source grid is much larger than any terminal; Fit::Resample
    //    (the default) nearest-neighbor downsamples it to fit the window.
    std::cout << "=== MATLAB peaks (viridis, 1000x1000 resampled to fit) ===" << std::endl;
    {
        const int n = 1000;
        std::vector<double> peaks(n * n);
        for (int r = 0; r < n; ++r) {
            double y = -3.0 + 6.0 * r / (n - 1);
            for (int c = 0; c < n; ++c) {
                double x = -3.0 + 6.0 * c / (n - 1);
                peaks[r * n + c] =
                    3.0 * (1 - x) * (1 - x)
                        * std::exp(-x * x - (y + 1) * (y + 1))
                    - 10.0 * (x / 5.0 - x * x * x - y * y * y * y * y)
                        * std::exp(-x * x - y * y)
                    - 1.0 / 3.0
                        * std::exp(-(x + 1) * (x + 1) - y * y);
            }
        }
        termimage::print(peaks, n, n,
            termimage::Options().colormap("viridis"));
    }

    return 0;
}
