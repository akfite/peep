#include "termimage.h"

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using Clock = std::chrono::high_resolution_clock;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

struct BenchResult {
    std::string name;
    size_t rows;
    size_t cols;
    int iterations;
    double total_ms;
    size_t output_bytes;

    double avg_ms() const { return total_ms / iterations; }
    double throughput_mpix() const {
        // million source pixels per second
        double pixels = static_cast<double>(rows) * cols;
        return (pixels * iterations) / (total_ms * 1e3);
    }
};

static std::vector<double> make_gradient(size_t rows, size_t cols) {
    std::vector<double> data(rows * cols);
    for (size_t i = 0; i < data.size(); ++i)
        data[i] = static_cast<double>(i) / static_cast<double>(data.size() - 1);
    return data;
}

static std::vector<double> make_gaussian(size_t n) {
    std::vector<double> data(n * n);
    double center = n / 2.0;
    double sigma = n / 5.0;
    for (size_t r = 0; r < n; ++r)
        for (size_t c = 0; c < n; ++c)
            data[r * n + c] = std::exp(
                -0.5 * ((c - center) * (c - center)
                       + (r - center) * (r - center))
                / (sigma * sigma));
    return data;
}

static std::vector<double> make_noisy(size_t rows, size_t cols) {
    // Pseudo-random data where every pixel differs (worst case for dedup)
    std::vector<double> data(rows * cols);
    double x = 0.123456789;
    for (size_t i = 0; i < data.size(); ++i) {
        x = std::fmod(x * 1597.0 + 51749.0, 244944.0);
        data[i] = x / 244944.0;
    }
    return data;
}

static BenchResult run_bench(const std::string& name,
                              const double* data, size_t rows, size_t cols,
                              const termimage::Options& base_opts,
                              int iterations) {
    // Warmup
    {
        std::ostringstream sink;
        termimage::Options opts = base_opts;
        opts.ostream(sink);
        termimage::print(data, rows, cols, opts);
    }

    // Measure
    std::ostringstream sink;
    termimage::Options opts = base_opts;
    opts.ostream(sink);

    auto t0 = Clock::now();
    for (int i = 0; i < iterations; ++i) {
        sink.str("");
        sink.clear();
        termimage::print(data, rows, cols, opts);
    }
    auto t1 = Clock::now();

    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    BenchResult r;
    r.name = name;
    r.rows = rows;
    r.cols = cols;
    r.iterations = iterations;
    r.total_ms = ms;
    r.output_bytes = sink.str().size();
    return r;
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main() {
    std::vector<BenchResult> results;

    // --- Small matrix, block_size=1 (baseline) ---
    {
        auto data = make_gradient(8, 16);
        results.push_back(run_bench(
            "small_gradient_bs1", data.data(), 8, 16,
            termimage::Options().colormap("viridis").block_size(1),
            5000));
    }

    // --- Small matrix, block_size=4 (high redundancy) ---
    {
        auto data = make_gradient(8, 16);
        results.push_back(run_bench(
            "small_gradient_bs4", data.data(), 8, 16,
            termimage::Options().colormap("viridis").block_size(4),
            2000));
    }

    // --- Medium gaussian, block_size=1 ---
    {
        auto data = make_gaussian(64);
        results.push_back(run_bench(
            "medium_gaussian_bs1", data.data(), 64, 64,
            termimage::Options().colormap("magma").block_size(1),
            500));
    }

    // --- Medium gaussian, block_size=2 ---
    {
        auto data = make_gaussian(64);
        results.push_back(run_bench(
            "medium_gaussian_bs2", data.data(), 64, 64,
            termimage::Options().colormap("magma").block_size(2),
            200));
    }

    // --- Large matrix, block_size=1 ---
    {
        auto data = make_gradient(200, 300);
        results.push_back(run_bench(
            "large_gradient_bs1", data.data(), 200, 300,
            termimage::Options().colormap("viridis").block_size(1),
            50));
    }

    // --- Large noisy (worst case: every pixel unique) ---
    {
        auto data = make_noisy(200, 300);
        results.push_back(run_bench(
            "large_noisy_bs1", data.data(), 200, 300,
            termimage::Options().colormap("viridis").block_size(1),
            50));
    }

    // --- Large with block_size=3 (high redundancy) ---
    {
        auto data = make_gradient(200, 300);
        results.push_back(run_bench(
            "large_gradient_bs3", data.data(), 200, 300,
            termimage::Options().colormap("gray").block_size(3),
            20));
    }

    // --- Large with crop ---
    {
        auto data = make_gradient(200, 300);
        results.push_back(run_bench(
            "large_gradient_crop", data.data(), 200, 300,
            termimage::Options().colormap("viridis").crop(50, 50, 100, 200),
            100));
    }

    // --- Print results ---
    std::cout << std::left
              << std::setw(26) << "benchmark"
              << std::right
              << std::setw(10) << "size"
              << std::setw(8)  << "iters"
              << std::setw(12) << "avg (ms)"
              << std::setw(14) << "Mpix/s"
              << std::setw(14) << "out (KB)"
              << '\n';
    std::cout << std::string(84, '-') << '\n';

    for (const auto& r : results) {
        std::string dims = std::to_string(r.rows) + "x" + std::to_string(r.cols);
        std::cout << std::left
                  << std::setw(26) << r.name
                  << std::right
                  << std::setw(10) << dims
                  << std::setw(8)  << r.iterations
                  << std::setw(12) << std::fixed << std::setprecision(3) << r.avg_ms()
                  << std::setw(14) << std::fixed << std::setprecision(2) << r.throughput_mpix()
                  << std::setw(14) << std::fixed << std::setprecision(1) << (r.output_bytes / 1024.0)
                  << '\n';
    }

    return 0;
}
