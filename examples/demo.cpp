#include <termimage/termimage.hpp>

#include <cstddef>
#include <cstdint>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

int main() {
    // 1. Colormap gradients
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
            termimage::print(grad, 1, cols,
                termimage::Options().colormap(colormaps[i].colormap)
                                    .title(colormaps[i].name)
                                    .colorbar(false));
        }
    }
    std::cout << std::endl;

    // 2. RGB uint8 image. 2x4 color chart makes channel/order mistakes obvious.
    {
        const int rows = 16;
        const int cols = 32;
        std::vector<std::uint8_t> rgb(rows * cols * 3);

        const std::uint8_t colors[2][4][3] = {
            {
                {255, 0, 0},     // red, top-left
                {0, 255, 0},     // green
                {0, 0, 255},     // blue
                {255, 255, 255}  // white, top-right
            },
            {
                {0, 255, 255},   // cyan, bottom-left
                {255, 0, 255},   // magenta
                {255, 255, 0},   // yellow
                {0, 0, 0}        // black, bottom-right
            }
        };

        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                const int tile_r = r / (rows / 2);
                const int tile_c = c / (cols / 4);
                size_t i = static_cast<size_t>(r * cols + c) * 3;
                rgb[i + 0] = colors[tile_r][tile_c][0];
                rgb[i + 1] = colors[tile_r][tile_c][1];
                rgb[i + 2] = colors[tile_r][tile_c][2];
            }
        }
        termimage::print(rgb, rows, cols,
            termimage::Options()
                .rgb()
                .resampling(termimage::Resampling::Nearest)
                .title("rgb chart: top R G B W / bottom C M Y K"));
    }
    std::cout << std::endl;

    // 3. Square 2D Gaussian scaled by pi (magma, auto block size)
    {
        const int n = 25;
        const double pi = 3.14159265358979323846;
        std::vector<double> gauss(n * n);
        double center = (n - 1) / 2.0;
        double sigma = n / 5.0;
        for (int r = 0; r < n; ++r)
            for (int c = 0; c < n; ++c)
                gauss[r * n + c] = pi * std::exp(
                    -0.5 * ((c - center) * (c - center)
                          + (r - center) * (r - center))
                    / (sigma * sigma));
        termimage::print(gauss, n, n,
            termimage::Options().colormap("magma").title("gaussian x pi"));
    }
    std::cout << std::endl;

    // 4. Checkerboard with NaN holes (viridis)
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
            termimage::Options().colormap("viridis").title("checkerboard + NaN"));
    }
    std::cout << std::endl;

    // 5. 8x8 integer ramp, cropped to a 4x6 window starting at (2,1)
    {
        int mat[64];
        for (int i = 0; i < 64; ++i)
            mat[i] = i;
        termimage::print(mat, 8, 8,
            termimage::Options().crop(2, 1, 4, 6).title("cropped integers"));
    }
    std::cout << std::endl;

    // 6. Inf handling with explicit clim
    {
        double inf = std::numeric_limits<double>::infinity();
        double data[] = {
            -inf, 0.0, 0.25, 0.5,
            0.75, 1.0, inf, 0.5
        };
        termimage::print(data, 2, 4,
            termimage::Options().colormap("magma")
                                .clim(0.0, 1.0)
                                .title("inf handling"));
    }
    std::cout << std::endl;

    // 7. MATLAB peaks — oversized to demonstrate terminal auto-fit (resample).
    //    The source grid is much larger than any terminal; Fit::Resample
    //    (the default) downsamples it with bilinear interpolation.
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
            termimage::Options().colormap("viridis").title("peaks"));
    }

    return 0;
}
