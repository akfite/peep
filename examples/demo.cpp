#include "termimage.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

int main() {
    // --- 1. Horizontal gradient (1 x 32) ------------------------------------
    std::cout << "=== Horizontal gradient (gray) ===" << std::endl;
    {
        const int cols = 32;
        std::vector<double> grad(cols);
        for (int c = 0; c < cols; ++c)
            grad[c] = static_cast<double>(c) / (cols - 1);
        termimage::print(grad.data(), 1, cols);
    }
    std::cout << std::endl;

    // --- 2. 2D Gaussian (16 x 32, magma) ------------------------------------
    std::cout << "=== 2D Gaussian (magma) ===" << std::endl;
    {
        const int rows = 16, cols = 32;
        std::vector<double> gauss(rows * cols);
        const double cx = cols / 2.0, cy = rows / 2.0;
        const double sx = cols / 5.0, sy = rows / 5.0;
        for (int r = 0; r < rows; ++r)
            for (int c = 0; c < cols; ++c)
                gauss[r * cols + c] = std::exp(
                    -0.5 * ((c - cx) * (c - cx) / (sx * sx)
                          + (r - cy) * (r - cy) / (sy * sy)));

        termimage::Options opts;
        opts.colormap = "magma";
        termimage::print(gauss.data(), rows, cols, opts);
    }
    std::cout << std::endl;

    // --- 3. Checkerboard with NaN holes (10 x 20, viridis) ------------------
    std::cout << "=== Checkerboard + NaN holes (viridis) ===" << std::endl;
    {
        const int rows = 10, cols = 20;
        const double nan = std::numeric_limits<double>::quiet_NaN();
        std::vector<double> board(rows * cols);
        for (int r = 0; r < rows; ++r)
            for (int c = 0; c < cols; ++c) {
                if ((r + c) % 7 == 0)
                    board[r * cols + c] = nan;  // NaN → transparent
                else
                    board[r * cols + c] = static_cast<double>((r + c) % 5) / 4.0;
            }

        termimage::Options opts;
        opts.colormap = "viridis";
        termimage::print(board.data(), rows, cols, opts);
    }
    std::cout << std::endl;

    // --- 4. Integers, auto-scaled (8 x 8, gray) ----------------------------
    std::cout << "=== Integer matrix (gray) ===" << std::endl;
    {
        int mat[8][8];
        for (int r = 0; r < 8; ++r)
            for (int c = 0; c < 8; ++c)
                mat[r][c] = r * 8 + c;
        termimage::print(&mat[0][0], 8, 8);
    }
    std::cout << std::endl;

    // --- 5. +Inf and -Inf handling ------------------------------------------
    std::cout << "=== Inf handling (magma) ===" << std::endl;
    {
        const double inf = std::numeric_limits<double>::infinity();
        double data[] = {
            -inf,  0.0, 0.25, 0.5,
             0.75, 1.0,  inf, 0.5
        };
        termimage::Options opts;
        opts.colormap = "magma";
        opts.clim_lo = 0.0;
        opts.clim_hi = 1.0;
        termimage::print(data, 2, 4, opts);
    }

    return 0;
}
