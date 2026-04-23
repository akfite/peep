#include "termimage.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

int main() {
    // 1. Horizontal gradient
    std::cout << "=== Horizontal gradient (gray) ===" << std::endl;
    {
        const int cols = 32;
        std::vector<double> grad(cols);
        for (int c = 0; c < cols; ++c)
            grad[c] = static_cast<double>(c) / (cols - 1);
        termimage::print(grad.data(), 1, cols,
            termimage::Options().block_size(2));
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
        termimage::print(gauss.data(), n, n,
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
        termimage::print(board.data(), rows, cols,
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

    return 0;
}
