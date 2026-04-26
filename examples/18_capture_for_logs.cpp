#include <peep/peep.hpp>

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

int main() {
    const size_t rows = 6;
    const size_t cols = 10;
    std::vector<double> leverage(rows * cols);

    for (size_t r = 0; r < rows; ++r) {
        for (size_t c = 0; c < cols; ++c) {
            const double row_weight = static_cast<double>(r + 1) / static_cast<double>(rows);
            const double col_weight = static_cast<double>(c + 1) / static_cast<double>(cols);
            leverage[r * cols + c] = row_weight * col_weight;
        }
    }

    const std::string ansi = peep::to_string(leverage, rows, cols, peep::Options()
        .colormap("cividis")
        .clim(0.0, 1.0)
        .title("captured leverage scores"));

    std::cout << "captured " << ansi.size() << " bytes of ANSI image output\n";
    std::cout << ansi;
}
