#include <peep/peep.hpp>

#include <cstddef>
#include <vector>

int main() {
    const size_t classes = 4;
    const size_t samples = 240;
    std::vector<int> confusion(classes * classes, 0);

    for (size_t i = 0; i < samples; ++i) {
        const size_t truth = (i * 7 + i / 13) % classes;
        size_t pred = truth;

        if (i % 11 == 0) pred = (truth + 1) % classes;
        if (i % 29 == 0) pred = (truth + 2) % classes;
        if (truth == 3 && i % 5 == 0) pred = 2;

        ++confusion[truth * classes + pred];
    }

    peep::show(confusion, classes, classes, peep::Options()
        .colormap("magma")
        .block_size(5)
        .title("confusion matrix: rows=true, cols=predicted"));
}
