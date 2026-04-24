#include <termimage/termimage.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using Clock = std::chrono::high_resolution_clock;

enum class OutputMode { Buffer, Terminal };

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

static std::vector<std::uint8_t> make_rgb_blocks(size_t rows, size_t cols) {
    std::vector<std::uint8_t> data(rows * cols * 3);
    const std::uint8_t colors[8][3] = {
        {255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 255, 255},
        {0, 255, 255}, {255, 0, 255}, {255, 255, 0}, {0, 0, 0}
    };

    const size_t tile_h = (rows >= 2) ? (rows / 2) : 1;
    const size_t tile_w = (cols >= 4) ? (cols / 4) : 1;
    for (size_t r = 0; r < rows; ++r) {
        for (size_t c = 0; c < cols; ++c) {
            size_t tile_r = std::min<size_t>(r / tile_h, 1);
            size_t tile_c = std::min<size_t>(c / tile_w, 3);
            const std::uint8_t* color = colors[tile_r * 4 + tile_c];
            size_t i = (r * cols + c) * 3;
            data[i + 0] = color[0];
            data[i + 1] = color[1];
            data[i + 2] = color[2];
        }
    }
    return data;
}

static std::vector<std::uint8_t> make_rgb_noisy(size_t rows, size_t cols) {
    std::vector<std::uint8_t> data(rows * cols * 3);
    std::uint32_t x = 0x12345678u;
    for (size_t i = 0; i < data.size(); ++i) {
        x = x * 1664525u + 1013904223u;
        data[i] = static_cast<std::uint8_t>(x >> 24);
    }
    return data;
}

static std::vector<std::uint8_t> interleaved_to_planar(
    const std::vector<std::uint8_t>& interleaved, size_t rows, size_t cols) {
    std::vector<std::uint8_t> planar(interleaved.size());
    const size_t plane = rows * cols;
    for (size_t i = 0; i < plane; ++i) {
        planar[i] = interleaved[i * 3 + 0];
        planar[plane + i] = interleaved[i * 3 + 1];
        planar[2 * plane + i] = interleaved[i * 3 + 2];
    }
    return planar;
}

static BenchResult run_bench(const std::string& name,
                              const double* data, size_t rows, size_t cols,
                              const termimage::Options& base_opts,
                              int iterations,
                              OutputMode mode = OutputMode::Buffer) {
    // Warmup
    {
        std::ostringstream sink;
        termimage::Options opts = base_opts;
        if (mode == OutputMode::Buffer) opts.ostream(sink);
        else opts.ostream(std::cout);
        termimage::print(data, rows, cols, opts);
    }

    // Measure
    std::ostringstream sink;
    termimage::Options opts = base_opts;
    if (mode == OutputMode::Buffer) opts.ostream(sink);
    else opts.ostream(std::cout);

    auto t0 = Clock::now();
    for (int i = 0; i < iterations; ++i) {
        if (mode == OutputMode::Buffer) {
            sink.str("");
            sink.clear();
        } else {
            std::cout << "\x1b[H\x1b[J";
        }
        termimage::print(data, rows, cols, opts);
        if (mode == OutputMode::Terminal) std::cout.flush();
    }
    auto t1 = Clock::now();

    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    BenchResult r;
    r.name = name;
    r.rows = rows;
    r.cols = cols;
    r.iterations = iterations;
    r.total_ms = ms;
    r.output_bytes = (mode == OutputMode::Buffer) ? sink.str().size() : 0;
    return r;
}

static BenchResult run_rgb_bench(const std::string& name,
                                 const std::uint8_t* data, size_t rows, size_t cols,
                                 termimage::RGBLayout rgb_layout,
                                 const termimage::Options& base_opts,
                                 int iterations,
                                 OutputMode mode = OutputMode::Buffer) {
    // Warmup
    {
        std::ostringstream sink;
        termimage::Options opts = base_opts;
        opts.rgb(rgb_layout);
        if (mode == OutputMode::Buffer) opts.ostream(sink);
        else opts.ostream(std::cout);
        termimage::print(data, rows, cols, opts);
    }

    // Measure
    std::ostringstream sink;
    termimage::Options opts = base_opts;
    opts.rgb(rgb_layout);
    if (mode == OutputMode::Buffer) opts.ostream(sink);
    else opts.ostream(std::cout);

    auto t0 = Clock::now();
    for (int i = 0; i < iterations; ++i) {
        if (mode == OutputMode::Buffer) {
            sink.str("");
            sink.clear();
        } else {
            std::cout << "\x1b[H\x1b[J";
        }
        termimage::print(data, rows, cols, opts);
        if (mode == OutputMode::Terminal) std::cout.flush();
    }
    auto t1 = Clock::now();

    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    BenchResult r;
    r.name = name;
    r.rows = rows;
    r.cols = cols;
    r.iterations = iterations;
    r.total_ms = ms;
    r.output_bytes = (mode == OutputMode::Buffer) ? sink.str().size() : 0;
    return r;
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main(int argc, char** argv) {
    OutputMode mode = OutputMode::Buffer;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--terminal") == 0) {
            mode = OutputMode::Terminal;
        } else if (std::strcmp(argv[i], "--help") == 0
                   || std::strcmp(argv[i], "-h") == 0) {
            std::cout << "usage: bench [--terminal]\n\n"
                      << "default: render to an ostringstream for stable renderer metrics\n"
                      << "--terminal: write frames to stdout for noisy end-to-end terminal metrics\n";
            return 0;
        } else {
            std::cerr << "unknown argument: " << argv[i] << '\n'
                      << "usage: bench [--terminal]\n";
            return 1;
        }
    }

    auto iters = [&](int n) {
        return (mode == OutputMode::Terminal) ? std::min(n, 3) : n;
    };

    if (mode == OutputMode::Terminal) {
        std::cerr << "terminal mode: writing real frames to stdout; timings include terminal I/O "
                     "and are expected to be noisy\n";
        std::cout << "\x1b[2J\x1b[H";
    }

    std::vector<BenchResult> results;

    // --- Small matrix, block_size=1 (baseline) ---
    {
        auto data = make_gradient(8, 16);
        results.push_back(run_bench(
            "small_gradient_bs1", data.data(), 8, 16,
            termimage::Options().colormap("viridis").block_size(1),
            iters(5000), mode));
    }

    // --- Small matrix, block_size=4 (high redundancy) ---
    {
        auto data = make_gradient(8, 16);
        results.push_back(run_bench(
            "small_gradient_bs4", data.data(), 8, 16,
            termimage::Options().colormap("viridis").block_size(4),
            iters(2000), mode));
    }

    // --- Medium gaussian, block_size=1 ---
    {
        auto data = make_gaussian(64);
        results.push_back(run_bench(
            "medium_gaussian_bs1", data.data(), 64, 64,
            termimage::Options().colormap("magma").block_size(1),
            iters(500), mode));
    }

    // --- Medium gaussian, block_size=2 ---
    {
        auto data = make_gaussian(64);
        results.push_back(run_bench(
            "medium_gaussian_bs2", data.data(), 64, 64,
            termimage::Options().colormap("magma").block_size(2),
            iters(200), mode));
    }

    // --- Large matrix, block_size=1 ---
    {
        auto data = make_gradient(200, 300);
        results.push_back(run_bench(
            "large_gradient_bs1", data.data(), 200, 300,
            termimage::Options().colormap("viridis").block_size(1),
            iters(50), mode));
    }

    // --- Large noisy (worst case: every pixel unique) ---
    {
        auto data = make_noisy(200, 300);
        results.push_back(run_bench(
            "large_noisy_bs1", data.data(), 200, 300,
            termimage::Options().colormap("viridis").block_size(1),
            iters(50), mode));
    }

    // --- Large with block_size=3 (high redundancy) ---
    {
        auto data = make_gradient(200, 300);
        results.push_back(run_bench(
            "large_gradient_bs3", data.data(), 200, 300,
            termimage::Options().colormap("gray").block_size(3),
            iters(20), mode));
    }

    // --- Large with crop ---
    {
        auto data = make_gradient(200, 300);
        results.push_back(run_bench(
            "large_gradient_crop", data.data(), 200, 300,
            termimage::Options().colormap("viridis").crop(50, 50, 200, 100),
            iters(100), mode));
    }

    // --- RGB interleaved, low color churn ---
    {
        auto data = make_rgb_blocks(64, 128);
        results.push_back(run_rgb_bench(
            "rgb_blocks_interleaved", data.data(), 64, 128,
            termimage::RGBLayout::Interleaved,
            termimage::Options().block_size(1),
            iters(500), mode));
    }

    // --- RGB planar, same image as above ---
    {
        auto interleaved = make_rgb_blocks(64, 128);
        auto data = interleaved_to_planar(interleaved, 64, 128);
        results.push_back(run_rgb_bench(
            "rgb_blocks_planar", data.data(), 64, 128,
            termimage::RGBLayout::Planar,
            termimage::Options().block_size(1),
            iters(500), mode));
    }

    // --- RGB interleaved, high color churn ---
    {
        auto data = make_rgb_noisy(200, 300);
        results.push_back(run_rgb_bench(
            "rgb_noisy_interleaved", data.data(), 200, 300,
            termimage::RGBLayout::Interleaved,
            termimage::Options().block_size(1),
            iters(50), mode));
    }

    // --- RGB cropped ---
    {
        auto data = make_rgb_blocks(200, 300);
        results.push_back(run_rgb_bench(
            "rgb_blocks_crop", data.data(), 200, 300,
            termimage::RGBLayout::Interleaved,
            termimage::Options().crop(50, 50, 200, 100),
            iters(100), mode));
    }

    // --- Print results ---
    if (mode == OutputMode::Terminal) {
        std::cout << "\x1b[0m\x1b[2J\x1b[H";
    }
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
        std::string out_kb = (mode == OutputMode::Buffer)
            ? std::to_string(r.output_bytes / 1024.0)
            : "-";
        std::cout << std::left
                  << std::setw(26) << r.name
                  << std::right
                  << std::setw(10) << dims
                  << std::setw(8)  << r.iterations
                  << std::setw(12) << std::fixed << std::setprecision(3) << r.avg_ms()
                  << std::setw(14) << std::fixed << std::setprecision(2) << r.throughput_mpix()
                  << std::setw(14);
        if (mode == OutputMode::Buffer)
            std::cout << std::fixed << std::setprecision(1) << (r.output_bytes / 1024.0);
        else
            std::cout << out_kb;
        std::cout << '\n';
    }

    return 0;
}
