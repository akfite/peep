#include <peep/peep.hpp>

#include <cstddef>
#include <string>
#include <vector>

int main() {
    struct ColormapDemo {
        std::string name;
        peep::Colormap colormap;
    };

    const ColormapDemo colormaps[] = {
        {"gray", peep::Colormap::Gray},
        {"viridis", peep::Colormap::Viridis},
        {"plasma", peep::Colormap::Plasma},
        {"inferno", peep::Colormap::Inferno},
        {"magma", peep::Colormap::Magma},
        {"cividis", peep::Colormap::Cividis},
        {"coolwarm", peep::Colormap::Coolwarm},
        {"gnuplot", peep::Colormap::Gnuplot},
        {"turbo", peep::Colormap::Turbo},
    };

    const size_t cols = 72;
    std::vector<double> quantiles(cols);
    for (size_t c = 0; c < cols; ++c) {
        quantiles[c] = static_cast<double>(c) / static_cast<double>(cols - 1);
    }

    for (size_t i = 0; i < sizeof(colormaps) / sizeof(colormaps[0]); ++i) {
        peep::show(quantiles, 1, cols, peep::Options()
            .colormap(colormaps[i].colormap)
            .clim(0.0, 1.0)
            .title(colormaps[i].name)
            .colorbar(false));
    }
}
