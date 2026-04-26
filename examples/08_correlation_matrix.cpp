#include <peep/peep.hpp>

#include <cmath>
#include <cstddef>
#include <vector>

static double feature_value(size_t sample, size_t feature) {
    const double t = static_cast<double>(sample);
    const double seasonal = std::sin(0.07 * t);
    const double trend = (static_cast<double>(sample % 50) - 25.0) / 25.0;
    const double cycle = std::cos(0.13 * t + static_cast<double>(feature) * 0.2);

    if (feature < 4) return seasonal + 0.25 * cycle;
    if (feature < 7) return -0.65 * seasonal + 0.55 * trend + 0.15 * cycle;
    return 0.7 * trend - 0.2 * cycle;
}

int main() {
    const size_t samples = 180;
    const size_t features = 10;
    std::vector<double> data(samples * features);
    std::vector<double> mean(features, 0.0);
    std::vector<double> cov(features * features, 0.0);
    std::vector<double> corr(features * features, 0.0);

    for (size_t r = 0; r < samples; ++r) {
        for (size_t c = 0; c < features; ++c) {
            data[r * features + c] = feature_value(r, c);
            mean[c] += data[r * features + c];
        }
    }
    for (size_t c = 0; c < features; ++c) mean[c] /= static_cast<double>(samples);

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

    for (size_t i = 0; i < features; ++i) {
        for (size_t j = 0; j < features; ++j) {
            const double denom = std::sqrt(cov[i * features + i] * cov[j * features + j]);
            corr[i * features + j] = (denom > 0.0) ? cov[i * features + j] / denom : 0.0;
        }
    }

    peep::print(corr, features, features, peep::Options()
        .colormap("coolwarm")
        .clim(-1.0, 1.0)
        .block_size(3)
        .title("correlation matrix: seasonal vs trend features"));
}
