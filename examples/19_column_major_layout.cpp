#include <peep/peep.hpp>

#include <cmath>
#include <cstddef>
#include <vector>

int main() {
    const size_t rows = 8;
    const size_t cols = 8;
    std::vector<double> gram(rows * cols);

    for (size_t r = 0; r < rows; ++r) {
        for (size_t c = 0; c < cols; ++c) {
            const double distance = static_cast<double>(r > c ? r - c : c - r);
            const double value = std::exp(-0.45 * distance)
                               + ((r == c) ? 0.25 : 0.0);
            gram[c * rows + r] = value; // column-major, like MATLAB or Fortran
        }
    }

    peep::show(gram, rows, cols, peep::Options()
        .layout(peep::Layout::ColMajor)
        .colormap("viridis")
        .block_size(4)
        .title("column-major Gram matrix"));
}
