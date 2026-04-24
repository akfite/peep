// Usage:
//
//   #include "termimage.h"
//
//   double data[] = {0, 1, 2, 3, 4, 5};
//   termimage::print(data, 2, 3);
//   termimage::print(data, 2, 3, termimage::Options().colormap("magma").block_size(3));
//

#ifndef TERMIMAGE_H
#define TERMIMAGE_H

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstddef>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "termimage_colormaps.h"
#include "termimage_terminal.h"

namespace termimage {

typedef std::array<std::uint8_t, 768> ColormapLut;

struct Color {
    std::uint8_t r, g, b;
    bool operator==(const Color& o) const { return r == o.r && g == o.g && b == o.b; }
    bool operator!=(const Color& o) const { return !(*this == o); }
};

enum class Layout { RowMajor, ColMajor };
enum class Colormap {
    Gray,
    Magma,
    Viridis,
    Plasma,
    Inferno,
    Cividis,
    Coolwarm,
    Gnuplot,
    Turbo
};

// What to do when the rendered image would exceed the terminal window.
//   Off      — render at full size (may wrap or scroll).
//   Resample — nearest-neighbor downsample to fit the terminal.
//   Trim     — render the top-left portion that fits, discard the rest.
// Only engages when the output stream is a terminal (stdout/stderr + isatty).
// Otherwise behaves as Off, so to_string and piped output are unaffected.
enum class Fit { Off, Resample, Trim };

namespace detail {

inline Colormap& default_colormap_ref() {
    static Colormap cmap = Colormap::Gray;
    return cmap;
}

inline std::string str_tolower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return out;
}

inline Colormap colormap_from_string(const std::string& name) {
    std::string lower = str_tolower(name);
    if (lower == "magma") return Colormap::Magma;
    if (lower == "viridis") return Colormap::Viridis;
    if (lower == "plasma") return Colormap::Plasma;
    if (lower == "inferno") return Colormap::Inferno;
    if (lower == "cividis") return Colormap::Cividis;
    if (lower == "coolwarm") return Colormap::Coolwarm;
    if (lower == "gnuplot") return Colormap::Gnuplot;
    if (lower == "turbo") return Colormap::Turbo;
    return Colormap::Gray;
}

} // namespace detail

// Global default colormap used by Options() when no colormap is specified.
inline Colormap default_colormap() { return detail::default_colormap_ref(); }
inline void set_default_colormap(Colormap c) { detail::default_colormap_ref() = c; }

struct Options {
    Options()
        : clim_lo_(NAN), clim_hi_(NAN), colormap_(default_colormap()),
        custom_cmap_(), has_custom_cmap_(false),
        nan_color_(), has_nan_color_(false),
        neg_inf_color_(), has_neg_inf_color_(false),
        pos_inf_color_(), has_pos_inf_color_(false),
        block_size_(1), layout_(Layout::RowMajor), out_(&std::cout),
        crop_r0_(0), crop_c0_(0), crop_h_(0), crop_w_(0),
        fit_(Fit::Resample), title_(false), title_text_() {}

    // Getters
    double clim_lo() const { return clim_lo_; }
    double clim_hi() const { return clim_hi_; }
    Colormap colormap() const { return colormap_; }
    const ColormapLut* custom_colormap() const {
        return has_custom_cmap_ ? &custom_cmap_ : nullptr;
    }
    bool has_custom_colormap() const { return has_custom_cmap_; }
    Color nan_color() const { return nan_color_; }
    bool has_nan_color() const { return has_nan_color_; }
    Color neg_inf_color() const { return neg_inf_color_; }
    bool has_neg_inf_color() const { return has_neg_inf_color_; }
    Color pos_inf_color() const { return pos_inf_color_; }
    bool has_pos_inf_color() const { return has_pos_inf_color_; }
    size_t block_size() const { return block_size_; }
    Layout layout() const { return layout_; }
    std::ostream& ostream() const { return *out_; }
    size_t crop_r0() const { return crop_r0_; }
    size_t crop_c0() const { return crop_c0_; }
    size_t crop_h() const { return crop_h_; }
    size_t crop_w() const { return crop_w_; }
    Fit fit() const { return fit_; }
    bool show_title() const { return title_; }
    const std::string& title_text() const { return title_text_; }

    // Chainable setters
    Options& clim(double lo, double hi) { clim_lo_ = lo; clim_hi_ = hi; return *this; }
    Options& clim_lo(double v) { clim_lo_ = v; return *this; }
    Options& clim_hi(double v) { clim_hi_ = v; return *this; }
    Options& colormap(Colormap c) { colormap_ = c; has_custom_cmap_ = false; return *this; }
    Options& colormap(const std::string& name) {
        has_custom_cmap_ = false;
        colormap_ = detail::colormap_from_string(name);
        return *this;
    }
    // Custom colormap: 256 RGB triplets (768 bytes), copied for safe ownership.
    Options& colormap(const ColormapLut& lut) {
        custom_cmap_ = lut;
        has_custom_cmap_ = true;
        return *this;
    }
    Options& colormap(const std::vector<std::uint8_t>& lut) {
        if (lut.size() != custom_cmap_.size()) {
            throw std::invalid_argument("termimage custom colormap must contain exactly 768 bytes");
        }
        std::copy(lut.begin(), lut.end(), custom_cmap_.begin());
        has_custom_cmap_ = true;
        return *this;
    }
    Options& colormap(const std::uint8_t (&lut)[768]) {
        std::copy(lut, lut + custom_cmap_.size(), custom_cmap_.begin());
        has_custom_cmap_ = true;
        return *this;
    }
    Options& nan_color(Color c) { nan_color_ = c; has_nan_color_ = true; return *this; }
    Options& nan_color(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
        return nan_color(Color{r, g, b});
    }
    Options& clear_nan_color() { has_nan_color_ = false; return *this; }
    Options& inf_color(Color c) {
        neg_inf_color_ = c;
        pos_inf_color_ = c;
        has_neg_inf_color_ = true;
        has_pos_inf_color_ = true;
        return *this;
    }
    Options& inf_color(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
        return inf_color(Color{r, g, b});
    }
    Options& inf_colors(Color neg, Color pos) {
        neg_inf_color_ = neg;
        pos_inf_color_ = pos;
        has_neg_inf_color_ = true;
        has_pos_inf_color_ = true;
        return *this;
    }
    Options& neg_inf_color(Color c) {
        neg_inf_color_ = c;
        has_neg_inf_color_ = true;
        return *this;
    }
    Options& neg_inf_color(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
        return neg_inf_color(Color{r, g, b});
    }
    Options& pos_inf_color(Color c) {
        pos_inf_color_ = c;
        has_pos_inf_color_ = true;
        return *this;
    }
    Options& pos_inf_color(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
        return pos_inf_color(Color{r, g, b});
    }
    Options& clear_inf_colors() {
        has_neg_inf_color_ = false;
        has_pos_inf_color_ = false;
        return *this;
    }
    Options& block_size(size_t s) { block_size_ = (s > 0) ? s : 1; return *this; }
    Options& layout(Layout l) { layout_ = l; return *this; }
    Options& ostream(std::ostream& os) { out_ = &os; return *this; }

    // Crop: from (r0, c0) to end of matrix
    Options& crop(size_t r0, size_t c0) {
        crop_r0_ = r0; crop_c0_ = c0;
        crop_h_ = 0; crop_w_ = 0;
        return *this;
    }
    // Crop: from (r0, c0) with explicit size h x w
    Options& crop(size_t r0, size_t c0, size_t h, size_t w) {
        crop_r0_ = r0; crop_c0_ = c0;
        crop_h_ = h; crop_w_ = w;
        return *this;
    }
    Options& fit(Fit f) { fit_ = f; return *this; }
    Options& title(bool enabled = true) { title_ = enabled; return *this; }
    Options& title(const std::string& text) {
        title_ = true;
        title_text_ = text;
        return *this;
    }
    Options& title(const char* text) {
        title_ = true;
        title_text_ = text ? text : "";
        return *this;
    }

private:
    double clim_lo_;
    double clim_hi_;
    Colormap colormap_;
    ColormapLut custom_cmap_;
    bool has_custom_cmap_;
    Color nan_color_;
    bool has_nan_color_;
    Color neg_inf_color_;
    bool has_neg_inf_color_;
    Color pos_inf_color_;
    bool has_pos_inf_color_;
    size_t block_size_;
    Layout layout_;
    std::ostream* out_;
    size_t crop_r0_;
    size_t crop_c0_;
    size_t crop_h_;
    size_t crop_w_;
    Fit fit_;
    bool title_;
    std::string title_text_;
};

// public API
template <typename T>
void print(const T* data, size_t rows, size_t cols, const Options& opts = Options());
template <typename T>
void print(const std::vector<T>& data, size_t rows, size_t cols,
           const Options& opts = Options());
template <typename T>
std::string to_string(const T* data, size_t rows, size_t cols,
                      const Options& opts = Options());
template <typename T>
std::string to_string(const std::vector<T>& data, size_t rows, size_t cols,
                      const Options& opts = Options());

// internal API
namespace detail {

typedef Color RGB;

// Computed grayscale colormap (avoids embedding trivial data)
inline const ColormapLut& cmap_gray() {
    static ColormapLut lut; // 256 * 3
    static bool init = false;
    if (!init) {
        for (int i = 0; i < 256; ++i) {
            std::uint8_t v = static_cast<std::uint8_t>(i);
            lut[i * 3 + 0] = v;
            lut[i * 3 + 1] = v;
            lut[i * 3 + 2] = v;
        }
        init = true;
    }
    return lut;
}

inline const ColormapLut& find_colormap(const std::string& name) {
    std::string lower = str_tolower(name);
    if (lower == "gray" || lower == "grey") return cmap_gray();
    for (int i = 0; i < CMAP_COUNT; ++i) {
        if (lower == CMAP_NAMES[i]) return *CMAP_DATA[i];
    }
    return cmap_gray();
}

inline const ColormapLut& find_colormap(Colormap cmap) {
    switch (cmap) {
        case Colormap::Magma:   return CMAP_MAGMA;
        case Colormap::Viridis: return CMAP_VIRIDIS;
        case Colormap::Plasma:  return CMAP_PLASMA;
        case Colormap::Inferno: return CMAP_INFERNO;
        case Colormap::Cividis: return CMAP_CIVIDIS;
        case Colormap::Coolwarm: return CMAP_COOLWARM;
        case Colormap::Gnuplot: return CMAP_GNUPLOT;
        case Colormap::Turbo:   return CMAP_TURBO;
        default:                return cmap_gray();
    }
}

inline const char* colormap_name(Colormap cmap) {
    switch (cmap) {
        case Colormap::Magma:    return "magma";
        case Colormap::Viridis:  return "viridis";
        case Colormap::Plasma:   return "plasma";
        case Colormap::Inferno:  return "inferno";
        case Colormap::Cividis:  return "cividis";
        case Colormap::Coolwarm: return "coolwarm";
        case Colormap::Gnuplot:  return "gnuplot";
        case Colormap::Turbo:    return "turbo";
        default:                 return "gray";
    }
}

// Resolve the effective colormap from Options (custom takes priority)
inline const ColormapLut& resolve_colormap(const Options& opts) {
    if (opts.custom_colormap()) return *opts.custom_colormap();
    return find_colormap(opts.colormap());
}

// Given the visible source size (vr, vc), block size, and a terminal size,
// decide output dimensions (out_r, out_c) and whether resampling is needed.
// Pure function of its inputs — the terminal query happens at the call site.
struct FitResolution {
    size_t out_r;
    size_t out_c;
    bool resample;
};

inline FitResolution resolve_fit(size_t vr, size_t vc, size_t bs,
                                 Fit requested, TerminalSize ts) {
    FitResolution r;
    r.out_r = vr;
    r.out_c = vc;
    r.resample = false;
    if (requested == Fit::Off || !ts.valid) return r;

    const size_t max_pcols = ts.cols;
    const size_t max_prows = ts.rows * 2; // 2 pixel rows per half-block cell
    const size_t pcols_needed = vc * bs;
    const size_t prows_needed = vr * bs;
    if (pcols_needed <= max_pcols && prows_needed <= max_prows) return r;

    if (requested == Fit::Resample) {
        // Aspect-preserving: pick a single uniform scale factor from the tighter
        // axis and apply to both, so vc:vr is preserved in pixels (and therefore
        // on screen, since a half-block pixel is square in a 1:2 terminal cell).
        //
        // width-limited ⟺ max_pcols/pcols_needed < max_prows/prows_needed
        //              ⟺ max_pcols*prows_needed < max_prows*pcols_needed
        const bool width_limited =
            max_pcols * prows_needed < max_prows * pcols_needed;
        size_t out_pcols, out_prows;
        if (width_limited) {
            out_pcols = max_pcols;
            out_prows = (prows_needed * max_pcols) / pcols_needed;
        } else {
            out_prows = max_prows;
            out_pcols = (pcols_needed * max_prows) / prows_needed;
        }
        r.out_c = (out_pcols / bs) ? (out_pcols / bs) : 1;
        r.out_r = (out_prows / bs) ? (out_prows / bs) : 1;
        r.resample = true;
    } else { // Fit::Trim
        // Identity sampling: no stretching possible, so we fill the terminal
        // with as much of the top-left of the source as fits.
        const size_t cap_c = (max_pcols / bs) ? (max_pcols / bs) : 1;
        const size_t cap_r = (max_prows / bs) ? (max_prows / bs) : 1;
        if (cap_c < r.out_c) r.out_c = cap_c;
        if (cap_r < r.out_r) r.out_r = cap_r;
    }
    return r;
}

inline RGB lookup(double normalized, const ColormapLut& cmap) {
    int idx = static_cast<int>(normalized * 255.0 + 0.5);
    if (idx < 0) idx = 0;
    if (idx > 255) idx = 255;
    RGB c;
    c.r = cmap[idx * 3];
    c.g = cmap[idx * 3 + 1];
    c.b = cmap[idx * 3 + 2];
    return c;
}

inline RGB color_for_value(double v, const ColormapLut& cmap, const Options& opts,
                           double normalized) {
    if (std::isnan(v) && opts.has_nan_color()) return opts.nan_color();
    if (std::isinf(v)) {
        if (v < 0.0 && opts.has_neg_inf_color()) return opts.neg_inf_color();
        if (v > 0.0 && opts.has_pos_inf_color()) return opts.pos_inf_color();
    }
    return lookup(normalized, cmap);
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

inline std::string render_title(const Options& opts, size_t rows, size_t cols,
                                size_t r0, size_t c0, size_t vr, size_t vc,
                                size_t out_r, size_t out_c,
                                bool resample) {
    std::ostringstream ss;
    const std::string& label = opts.title_text();
    ss << (label.empty() ? "termimage" : label) << ": ";
    ss << "data=" << rows << 'x' << cols;

    const bool cropped = (r0 != 0 || c0 != 0 || vr != rows || vc != cols);
    if (cropped) {
        ss << " crop=(" << r0 << ',' << c0 << ' ' << vr << 'x' << vc << ')';
    }

    if (out_r != vr || out_c != vc) {
        ss << " display=" << out_r << 'x' << out_c;
        ss << (resample ? " resampled" : " trimmed");
    }

    ss << " cmap=" << (opts.has_custom_colormap() ? "custom" : colormap_name(opts.colormap()));
    if (opts.layout() == Layout::ColMajor) ss << " layout=col-major";
    if (opts.block_size() != 1) ss << " block=" << opts.block_size();
    return ss.str();
}

template <typename T>
void render(const T* data, size_t rows, size_t cols, const Options& opts) {
    if (!data || rows == 0 || cols == 0) return;

    const ColormapLut& cmap = resolve_colormap(opts);
    std::ostream& os = opts.ostream();
    const size_t bs = opts.block_size();
    const bool col_major = (opts.layout() == Layout::ColMajor);

    // Resolve crop window
    size_t r0 = opts.crop_r0();
    size_t c0 = opts.crop_c0();
    if (r0 >= rows || c0 >= cols) return;
    size_t vr = (opts.crop_h() == 0) ? (rows - r0) : opts.crop_h();
    size_t vc = (opts.crop_w() == 0) ? (cols - c0) : opts.crop_w();
    if (r0 + vr > rows) vr = rows - r0;
    if (c0 + vc > cols) vc = cols - c0;
    if (vr == 0 || vc == 0) return;

    // Resolve terminal-fit. Only engages when the output is a real terminal;
    // ostringstream / pipes / files return an invalid size and we fall through
    // to full-size rendering (Fit::Off semantics).
    FitResolution fr = resolve_fit(vr, vc, bs, opts.fit(),
                                   query_terminal_size(os));
    const size_t out_r = fr.out_r;
    const size_t out_c = fr.out_c;
    const bool resample = fr.resample;

    // Element accessor (output-matrix coords, maps to source via resample or identity).
    // Bresenham-style index scaling: when out == v, (mr*vr)/out_r == mr (identity), so
    // resample=true with no shrink is a no-op. trim uses identity sampling directly.
    auto get = [&](size_t mr, size_t mc) -> double {
        size_t sr = resample ? (mr * vr) / out_r : mr;
        size_t sc = resample ? (mc * vc) / out_c : mc;
        if (col_major)
            return static_cast<double>(data[(c0 + sc) * rows + (r0 + sr)]);
        else
            return static_cast<double>(data[(r0 + sr) * cols + (c0 + sc)]);
    };

    // Compute clim from visible region only
    double lo = opts.clim_lo();
    double hi = opts.clim_hi();
    bool auto_lo = std::isnan(lo);
    bool auto_hi = std::isnan(hi);

    if (auto_lo || auto_hi) {
        double flo = std::numeric_limits<double>::infinity();
        double fhi = -std::numeric_limits<double>::infinity();
        for (size_t mr = 0; mr < out_r; ++mr) {
            for (size_t mc = 0; mc < out_c; ++mc) {
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

    // Render in pixel space: each output matrix element maps to a bs x bs block
    size_t prows = out_r * bs;
    size_t pcols = out_c * bs;

    // Buffer all output, then flush once
    std::ostringstream buf;
    if (opts.show_title()) {
        buf << render_title(opts, rows, cols, r0, c0, vr, vc, out_r, out_c, resample)
            << '\n';
    }

    // Track current terminal color state to skip redundant escapes
    RGB cur_bg = {0, 0, 0}, cur_fg = {0, 0, 0};
    bool bg_set = false, fg_set = false;

    auto do_reset = [&]() {
        if (bg_set || fg_set) {
            emit_reset(buf);
            bg_set = false;
            fg_set = false;
        }
    };

    auto set_bg = [&](RGB c) {
        if (!bg_set || cur_bg != c) {
            emit_bg(buf, c);
            cur_bg = c;
            bg_set = true;
        }
    };

    auto set_fg = [&](RGB c) {
        if (!fg_set || cur_fg != c) {
            emit_fg(buf, c);
            cur_fg = c;
            fg_set = true;
        }
    };

    for (size_t pr = 0; pr < prows; pr += 2) {
        bool has_bot = (pr + 1 < prows);

        for (size_t pc = 0; pc < pcols; ++pc) {
            size_t mr_top = pr / bs;
            size_t mc = pc / bs;
            double top_val = get(mr_top, mc);
            bool top_transparent = std::isnan(top_val) && !opts.has_nan_color();

            double bot_val = NAN;
            bool bot_transparent = true;
            if (has_bot) {
                size_t mr_bot = (pr + 1) / bs;
                bot_val = get(mr_bot, mc);
                bot_transparent = std::isnan(bot_val) && !opts.has_nan_color();
            }

            if (top_transparent && bot_transparent) {
                do_reset();
                buf << ' ';
            } else if (top_transparent) {
                do_reset();
                set_fg(color_for_value(bot_val, cmap, opts, normalize(bot_val)));
                emit_lower_half(buf);
            } else if (bot_transparent) {
                do_reset();
                set_fg(color_for_value(top_val, cmap, opts, normalize(top_val)));
                emit_upper_half(buf);
            } else {
                set_bg(color_for_value(top_val, cmap, opts, normalize(top_val)));
                set_fg(color_for_value(bot_val, cmap, opts, normalize(bot_val)));
                emit_lower_half(buf);
            }
        }

        do_reset();
        buf << '\n';
        // Reset tracking at line boundaries (reset was just emitted)
        bg_set = false;
        fg_set = false;
    }

    os << buf.str();
}

inline void validate_vector_size(size_t size, size_t rows, size_t cols) {
    if (rows == 0 || cols == 0) return;
    if (cols != 0 && rows > (std::numeric_limits<size_t>::max() / cols)) {
        throw std::invalid_argument("termimage vector dimensions overflow size_t");
    }
    const size_t expected = rows * cols;
    if (size != expected) {
        throw std::invalid_argument("termimage vector size must equal rows * cols");
    }
}

template <typename T>
std::string render_to_string(const T* data, size_t rows, size_t cols, const Options& opts = Options()) {
    std::ostringstream oss;
    Options local = opts;
    local.ostream(oss);
    render(data, rows, cols, local);
    return oss.str();
}

} // namespace detail

// template definitions
template <typename T>
void print(const T* data, size_t rows, size_t cols, const Options& opts) {
    detail::render(data, rows, cols, opts);
}

template <typename T>
void print(const std::vector<T>& data, size_t rows, size_t cols, const Options& opts) {
    detail::validate_vector_size(data.size(), rows, cols);
    detail::render(data.data(), rows, cols, opts);
}

template <typename T>
std::string to_string(const T* data, size_t rows, size_t cols, const Options& opts) {
    return detail::render_to_string(data, rows, cols, opts);
}

template <typename T>
std::string to_string(const std::vector<T>& data, size_t rows, size_t cols, const Options& opts) {
    detail::validate_vector_size(data.size(), rows, cols);
    return detail::render_to_string(data.data(), rows, cols, opts);
}

} // namespace termimage

#endif // TERMIMAGE_H
