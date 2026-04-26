#include <peep/peep.hpp>

#include <cmath>
#include <cstddef>
#include <vector>

static double sigmoid(double x) {
    return 1.0 / (1.0 + std::exp(-x));
}

int main() {
    const size_t observations = 24;
    const size_t neurons = 16;
    std::vector<double> activation(observations * neurons);

    for (size_t r = 0; r < observations; ++r) {
        const double x1 = std::sin(0.35 * static_cast<double>(r));
        const double x2 = std::cos(0.19 * static_cast<double>(r));
        for (size_t c = 0; c < neurons; ++c) {
            const double w1 = std::sin(0.4 * static_cast<double>(c));
            const double w2 = std::cos(0.27 * static_cast<double>(c));
            const double bias = (static_cast<double>(c) - 8.0) * 0.08;
            activation[r * neurons + c] = sigmoid(2.0 * (w1 * x1 + w2 * x2 + bias));
        }
    }

    peep::show(activation, observations, neurons, peep::Options()
        .colormap("viridis")
        .clim(0.0, 1.0)
        .title("sigmoid activations across observations and hidden units"));
}
