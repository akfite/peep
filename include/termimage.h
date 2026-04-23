// termimage.h -- terminal matrix visualizer (header-only, C++11)
//
// Usage:
//   #include "termimage.h"
//
//   double data[] = {0, 1, 2, 3, 4, 5};
//   termimage::print(data, 2, 3);  // 2 rows x 3 cols
//
// See termimage::Options for colormap, clim, cell_width, layout, etc.

#ifndef TERMIMAGE_H
#define TERMIMAGE_H

#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <string>

#include "termimage_colormaps.h"

namespace termimage {

// ---- Public types ----------------------------------------------------------

enum Layout { ROW_MAJOR, COL_MAJOR };

struct Options {
    double clim_lo;        // color-axis lower bound (NAN = auto)
    double clim_hi;        // color-axis upper bound (NAN = auto)
    std::string colormap;  // "gray", "magma", "viridis"
    int cell_width;        // terminal columns per matrix element
    Layout layout;         // memory layout of the data pointer
    std::ostream* out;     // output stream

    Options()
        : clim_lo(NAN)
        , clim_hi(NAN)
        , colormap("gray")
        , cell_width(1)
        , layout(ROW_MAJOR)
        , out(&std::cout) {}
};

// ---- Public API ------------------------------------------------------------

/// Print a 2D numeric array to the terminal as a colorized image.
/// @param data   Pointer to contiguous numeric data (any type castable to double).
/// @param rows   Number of rows in the matrix.
/// @param cols   Number of columns in the matrix.
/// @param opts   Display options (colormap, clim, cell_width, layout, ...).
template <typename T>
void print(const T* data, int rows, int cols, const Options& opts = Options());

// ---- Implementation (detail) -----------------------------------------------

namespace detail {

struct RGB { unsigned char r, g, b; };

inline const unsigned char* find_colormap(const std::string& name) {
    for (int i = 0; i < CMAP_COUNT; ++i) {
        if (name == CMAP_NAMES[i]) return CMAP_DATA[i];
    }
    return CMAP_GRAY;  // fallback
}

inline RGB lookup(double normalized, const unsigned char* cmap) {
    int idx = static_cast<int>(normalized * 255.0 + 0.5);  // round
    if (idx < 0) idx = 0;
    if (idx > 255) idx = 255;
    RGB c;
    c.r = cmap[idx * 3 + 0];
    c.g = cmap[idx * 3 + 1];
    c.b = cmap[idx * 3 + 2];
    return c;
}

inline void emit_bg(std::ostream& os, RGB c) {
    os << "\x1b[48;2;"
       << static_cast<int>(c.r) << ';'
       << static_cast<int>(c.g) << ';'
       << static_cast<int>(c.b) << 'm';
}

inline void emit_fg(std::ostream& os, RGB c) {
    os << "\x1b[38;2;"
       << static_cast<int>(c.r) << ';'
       << static_cast<int>(c.g) << ';'
       << static_cast<int>(c.b) << 'm';
}

inline void emit_reset(std::ostream& os) {
    os << "\x1b[0m";
}

// U+2584 LOWER HALF BLOCK  (UTF-8: E2 96 84)
inline void emit_lower_half(std::ostream& os) {
    os << "\xe2\x96\x84";
}

// U+2580 UPPER HALF BLOCK  (UTF-8: E2 96 80)
inline void emit_upper_half(std::ostream& os) {
    os << "\xe2\x96\x80";
}

/// Core rendering implementation.
template <typename T>
void render(const T* data, int rows, int cols, const Options& opts) {
    if (rows <= 0 || cols <= 0) return;

    const unsigned char* cmap = find_colormap(opts.colormap);
    std::ostream& os = *opts.out;
    const int cw = opts.cell_width > 0 ? opts.cell_width : 1;

    // --- Element accessor (handles layout) ----------------------------------
    // ROW_MAJOR: data[row * cols + col]
    // COL_MAJOR: data[col * rows + row]
    const bool col_major = (opts.layout == COL_MAJOR);

    // --- Compute clim -------------------------------------------------------
    double lo = opts.clim_lo;
    double hi = opts.clim_hi;

    const bool auto_lo = std::isnan(lo);
    const bool auto_hi = std::isnan(hi);

    if (auto_lo || auto_hi) {
        double found_lo =  std::numeric_limits<double>::infinity();
        double found_hi = -std::numeric_limits<double>::infinity();
        const int n = rows * cols;
        for (int i = 0; i < n; ++i) {
            double v = static_cast<double>(data[i]);
            if (std::isnan(v) || std::isinf(v)) continue;
            if (v < found_lo) found_lo = v;
            if (v > found_hi) found_hi = v;
        }
        if (auto_lo) lo = found_lo;
        if (auto_hi) hi = found_hi;
    }

    // Guard: if lo/hi are still infinite (all NaN data) or equal, pick sane defaults
    if (std::isinf(lo) || std::isinf(hi)) { lo = 0.0; hi = 1.0; }
    const double range = (hi != lo) ? (hi - lo) : 1.0;

    // --- Render rows in pairs -----------------------------------------------
    for (int r = 0; r < rows; r += 2) {
        const bool has_bot = (r + 1 < rows);

        for (int c = 0; c < cols; ++c) {
            // Fetch top value
            const double top_raw = col_major
                ? static_cast<double>(data[c * rows + r])
                : static_cast<double>(data[r * cols + c]);
            const bool top_nan = std::isnan(top_raw);

            // Fetch bottom value (NaN if no bottom row)
            double bot_raw = NAN;
            if (has_bot) {
                bot_raw = col_major
                    ? static_cast<double>(data[c * rows + (r + 1)])
                    : static_cast<double>(data[(r + 1) * cols + c]);
            }
            const bool bot_nan = std::isnan(bot_raw);

            // Normalize (±Inf → clim extremes)
            auto normalize = [&](double v) -> double {
                if (std::isinf(v)) return (v > 0) ? 1.0 : 0.0;
                double n = (v - lo) / range;
                if (n < 0.0) n = 0.0;
                if (n > 1.0) n = 1.0;
                return n;
            };

            if (top_nan && bot_nan) {
                // Both transparent: emit plain spaces
                emit_reset(os);
                for (int w = 0; w < cw; ++w) os << ' ';
            } else if (top_nan) {
                // Top transparent, bottom colored → ▄ with default bg
                emit_reset(os);
                emit_fg(os, lookup(normalize(bot_raw), cmap));
                for (int w = 0; w < cw; ++w) emit_lower_half(os);
            } else if (bot_nan) {
                // Top colored, bottom transparent → ▀ with default bg
                emit_reset(os);
                emit_fg(os, lookup(normalize(top_raw), cmap));
                for (int w = 0; w < cw; ++w) emit_upper_half(os);
            } else {
                // Both colored → bg=top, fg=bottom, ▄
                emit_bg(os, lookup(normalize(top_raw), cmap));
                emit_fg(os, lookup(normalize(bot_raw), cmap));
                for (int w = 0; w < cw; ++w) emit_lower_half(os);
            }
        }

        emit_reset(os);
        os << '\n';
    }
}

} // namespace detail

// ---- Template definition ---------------------------------------------------

template <typename T>
void print(const T* data, int rows, int cols, const Options& opts) {
    detail::render(data, rows, cols, opts);
}

} // namespace termimage

#endif // TERMIMAGE_H
