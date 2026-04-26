#include <peep/peep.hpp>

#include <cmath>
#include <cstddef>
#include <vector>

static double sample_value(size_t row, size_t feature) {
    const double t = static_cast<double>(row);
    const double z1 = std::sin(0.09 * t);
    const double z2 = std::cos(0.05 * t + 0.4);
    const double z3 = std::sin(0.17 * t + 1.2);

    switch (feature) {
        case 0: return 1.2 * z1 + 0.2 * z2;
        case 1: return 0.9 * z1 - 0.3 * z2;
        case 2: return -0.8 * z1 + 0.6 * z3;
        case 3: return 1.1 * z2 + 0.2 * z3;
        case 4: return -0.7 * z2 + 0.4 * z1;
        case 5: return 0.9 * z3;
        default: return 0.5 * z1 - 0.5 * z3;
    }
}

int main() {
    const size_t samples = 160;
    const size_t features = 7;
    std::vector<double> data(samples * features);
    std::vector<double> mean(features, 0.0);
    std::vector<double> cov(features * features, 0.0);

    for (size_t r = 0; r < samples; ++r) {
        for (size_t c = 0; c < features; ++c) {
            data[r * features + c] = sample_value(r, c);
            mean[c] += data[r * features + c];
        }
    }

    for (size_t c = 0; c < features; ++c) {
        mean[c] /= static_cast<double>(samples);
    }

    for (size_t i = 0; i < features; ++i) {
        for (size_t j = 0; j < features; ++j) {
            double accum = 0.0;
            for (size_t r = 0; r < samples; ++r) {
                accum += (data[r * features + i] - mean[i])
                       * (data[r * features + j] - mean[j]);
            }
            cov[i * features + j] = accum / static_cast<double>(samples - 1);
        }
    }

    peep::print(cov, features, features, peep::Options()
        .colormap("coolwarm")
        .clim(-1.2, 1.2)
        .block_size(4)
        .title("covariance matrix from synthetic latent factors"));
}
