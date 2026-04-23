// termimage.h -- terminal matrix visualizer (header-only, C++11)
//
// Usage:
//   #include "termimage.h"
//
//   double data[] = {0, 1, 2, 3, 4, 5};
//   termimage::print(data, 2, 3);
//   termimage::print(data, 2, 3, termimage::Options().colormap("magma").block_size(2));
//

#ifndef TERMIMAGE_H
#define TERMIMAGE_H

#include <cmath>
#include <iostream>
#include <limits>
#include <string>

#include "termimage_colormaps.h"

namespace termimage {

// ---- Public types ----------------------------------------------------------

enum Layout { ROW_MAJOR, COL_MAJOR };

struct Options {
    double clim_lo_;
    double clim_hi_;
    std::string colormap_;
    int block_size_;
    Layout layout_;
    std::ostream* out_;
    int crop_r0_;
    int crop_c0_;
    int crop_h_;
    int crop_w_;

    Options()
        : clim_lo_(NAN), clim_hi_(NAN), colormap_("gray"),
        block_size_(1), layout_(ROW_MAJOR), out_(&std::cout),
        crop_r0_(0), crop_c0_(0), crop_h_(-1), crop_w_(-1) {}

    // Chainable setters
    Options& clim(double lo, double hi) { clim_lo_ = lo; clim_hi_ = hi; return *this; }
    Options& clim_lo(double v) { clim_lo_ = v; return *this; }
    Options& clim_hi(double v) { clim_hi_ = v; return *this; }
    Options& colormap(const std::string& name) { colormap_ = name; return *this; }
    Options& block_size(int s) { block_size_ = s; return *this; }
    Options& layout(Layout l) { layout_ = l; return *this; }
    Options& out(std::ostream& os) { out_ = &os; return *this; }

    // Crop: from (r0, c0) to end of matrix
    Options& crop(int r0, int c0) {
        crop_r0_ = r0; crop_c0_ = c0;
        crop_h_ = -1; crop_w_ = -1;
        return *this;
    }
    // Crop: from (r0, c0) with explicit size h x w
    Options& crop(int r0, int c0, int h, int w) {
        crop_r0_ = r0; crop_c0_ = c0;
        crop_h_ = h; crop_w_ = w;
        return *this;
    }
};

// ---- Public API ------------------------------------------------------------

template <typename T>
void print(const T* data, int rows, int cols, const Options& opts = Options());

// ---- Implementation (detail) -----------------------------------------------

namespace detail {

struct RGB { unsigned char r, g, b; };

inline const unsigned char* find_colormap(const std::string& name) {
    for (int i = 0; i < CMAP_COUNT; ++i) {
        if (name == CMAP_NAMES[i]) return CMAP_DATA[i];
    }
    return CMAP_GRAY;
}

inline RGB lookup(double normalized, const unsigned char* cmap) {
    int idx = static_cast<int>(normalized * 255.0 + 0.5);
    if (idx < 0) idx = 0;
    if (idx > 255) idx = 255;
    RGB c;
    c.r = cmap[idx * 3];
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

inline void emit_reset(std::ostream& os) { os << "\x1b[0m"; }

// U+2584 LOWER HALF BLOCK (UTF-8: E2 96 84)
inline void emit_lower_half(std::ostream& os) { os << "\xe2\x96\x84"; }

// U+2580 UPPER HALF BLOCK (UTF-8: E2 96 80)
inline void emit_upper_half(std::ostream& os) { os << "\xe2\x96\x80"; }

template <typename T>
void render(const T* data, int rows, int cols, const Options& opts) {
    if (rows <= 0 || cols <= 0) return;

    const unsigned char* cmap = find_colormap(opts.colormap_);
    std::ostream& os = *opts.out_;
    const int bs = opts.block_size_ > 0 ? opts.block_size_ : 1;
    const bool col_major = (opts.layout_ == COL_MAJOR);

    // Resolve crop window
    int r0 = opts.crop_r0_ < 0 ? 0 : opts.crop_r0_;
    int c0 = opts.crop_c0_ < 0 ? 0 : opts.crop_c0_;
    if (r0 >= rows || c0 >= cols) return;
    int vr = (opts.crop_h_ < 0) ? (rows - r0) : opts.crop_h_;
    int vc = (opts.crop_w_ < 0) ? (cols - c0) : opts.crop_w_;
    if (r0 + vr > rows) vr = rows - r0;
    if (c0 + vc > cols) vc = cols - c0;
    if (vr <= 0 || vc <= 0) return;

    // Element accessor (matrix coords relative to crop origin)
    auto get = [&](int mr, int mc) -> double {
        if (col_major)
            return static_cast<double>(data[(c0 + mc) * rows + (r0 + mr)]);
        else
            return static_cast<double>(data[(r0 + mr) * cols + (c0 + mc)]);
    };

    // Compute clim from visible region only
    double lo = opts.clim_lo_;
    double hi = opts.clim_hi_;
    bool auto_lo = std::isnan(lo);
    bool auto_hi = std::isnan(hi);

    if (auto_lo || auto_hi) {
        double flo = std::numeric_limits<double>::infinity();
        double fhi = -std::numeric_limits<double>::infinity();
        for (int mr = 0; mr < vr; ++mr) {
            for (int mc = 0; mc < vc; ++mc) {
                double v = get(mr, mc);
                if (std::isnan(v) || std::isinf(v)) continue;
                if (v < flo) flo = v;
                if (v > fhi) fhi = v;
            }
        }
        if (auto_lo) lo = flo;
        if (auto_hi) hi = fhi;
    }

    if (std::isinf(lo) || std::isinf(hi)) { lo = 0.0; hi = 1.0; }
    double range = (hi != lo) ? (hi - lo) : 1.0;

    auto normalize = [&](double v) -> double {
        if (std::isinf(v)) return (v > 0) ? 1.0 : 0.0;
        double n = (v - lo) / range;
        if (n < 0.0) n = 0.0;
        if (n > 1.0) n = 1.0;
        return n;
    };

    // Render in pixel space: each matrix element maps to a bs x bs block
    int prows = vr * bs;
    int pcols = vc * bs;

    for (int pr = 0; pr < prows; pr += 2) {
        bool has_bot = (pr + 1 < prows);

        for (int pc = 0; pc < pcols; ++pc) {
            int mr_top = pr / bs;
            int mc = pc / bs;
            double top_val = get(mr_top, mc);
            bool top_nan = std::isnan(top_val);

            double bot_val = NAN;
            bool bot_nan = true;
            if (has_bot) {
                int mr_bot = (pr + 1) / bs;
                bot_val = get(mr_bot, mc);
                bot_nan = std::isnan(bot_val);
            }

            if (top_nan && bot_nan) {
                emit_reset(os);
                os << ' ';
            } else if (top_nan) {
                emit_reset(os);
                emit_fg(os, lookup(normalize(bot_val), cmap));
                emit_lower_half(os);
            } else if (bot_nan) {
                emit_reset(os);
                emit_fg(os, lookup(normalize(top_val), cmap));
                emit_upper_half(os);
            } else {
                emit_bg(os, lookup(normalize(top_val), cmap));
                emit_fg(os, lookup(normalize(bot_val), cmap));
                emit_lower_half(os);
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
