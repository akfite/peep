#ifndef PEEP_PEEP_HPP
#define PEEP_PEEP_HPP

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include "peep/colormaps.hpp"
#include "peep/terminal.hpp"

namespace peep {

typedef std::array<std::uint8_t, 768> ColormapLut;

struct Color {
    std::uint8_t r, g, b;
    bool operator==(const Color& o) const { return r == o.r && g == o.g && b == o.b; }
    bool operator!=(const Color& o) const { return !(*this == o); }
};

typedef std::function<double(size_t, size_t)> ScalarAccessor;
typedef std::function<Color(size_t, size_t)> RGBAccessor;

enum class Layout { RowMajor, ColMajor };
enum class RGBLayout { Interleaved, Planar };
enum class Resampling { Nearest, Bilinear };
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
//   Off      - render at full size (may wrap or scroll).
//   Resample - downsample to fit the terminal.
//   Trim     - render the top-left portion that fits, discard the rest.
// Only engages when the output stream is a terminal (stdout/stderr + isatty).
// Otherwise behaves as Off, so to_string and piped output are unaffected.
enum class Fit { Off, Resample, Trim };

// Global default colormap used by Options() when no colormap is specified.
Colormap default_colormap();
void set_default_colormap(Colormap c);

struct Options {
private:
    enum class SourceMode {
        ScalarData,
        RGBData,
        ScalarAccessor,
        RGBAccessor
    };

    enum class CropMode {
        None,
        CornerToEnd,
        CornerWindow,
        Center
    };

public:
    Options();

    // Getters
    double clim_lo() const { return clim_lo_; }
    double clim_hi() const { return clim_hi_; }
    Colormap colormap() const { return colormap_; }
    const ColormapLut* custom_colormap() const {
        return custom_cmap_set_ ? &custom_cmap_ : nullptr;
    }
    bool has_custom_colormap() const { return custom_cmap_set_; }
    Color nan_color() const { return nan_color_; }
    bool has_nan_color() const { return nan_color_set_; }
    Color neg_inf_color() const { return neg_inf_color_; }
    bool has_neg_inf_color() const { return neg_inf_color_set_; }
    Color pos_inf_color() const { return pos_inf_color_; }
    bool has_pos_inf_color() const { return pos_inf_color_set_; }
    size_t block_size() const { return block_size_; }
    bool has_block_size() const { return block_size_set_; }
    Layout layout() const { return layout_; }
    std::ostream& ostream() const { return out_.get(); }
    size_t crop_x() const { return crop_x_; }
    size_t crop_y() const { return crop_y_; }
    size_t crop_w() const { return crop_w_; }
    size_t crop_h() const { return crop_h_; }
    bool crop_is_centered() const { return crop_mode_ == CropMode::Center; }
    bool crop_is_to_end() const { return crop_mode_ == CropMode::CornerToEnd; }
    bool crop_is_window() const { return crop_mode_ == CropMode::CornerWindow; }
    size_t crop_center_x() const { return crop_center_x_; }
    size_t crop_center_y() const { return crop_center_y_; }
    Fit fit() const { return fit_; }
    Resampling resampling() const { return resampling_; }
    bool is_rgb() const {
        return source_mode_ == SourceMode::RGBData
            || source_mode_ == SourceMode::RGBAccessor;
    }
    RGBLayout rgb_layout() const { return rgb_layout_; }
    bool has_accessor() const {
        return source_mode_ == SourceMode::ScalarAccessor;
    }
    bool has_rgb_accessor() const {
        return source_mode_ == SourceMode::RGBAccessor;
    }
    const ScalarAccessor& accessor() const { return scalar_accessor_; }
    const RGBAccessor& rgb_accessor() const { return rgb_accessor_; }
    bool show_title() const { return title_show_; }
    const std::string& title_text() const { return title_text_; }
    bool show_colorbar() const { return colorbar_; }

    // Chainable setters
    Options& clim(double lo, double hi) { clim_lo_ = lo; clim_hi_ = hi; return *this; }
    Options& clim_lo(double v) { clim_lo_ = v; return *this; }
    Options& clim_hi(double v) { clim_hi_ = v; return *this; }
    Options& colormap(Colormap c) { colormap_ = c; custom_cmap_set_ = false; return *this; }
    Options& colormap(const std::string& name);
    // Custom colormap: 256 RGB triplets (768 bytes), copied for safe ownership.
    Options& colormap(const ColormapLut& lut) {
        custom_cmap_ = lut;
        custom_cmap_set_ = true;
        return *this;
    }
    Options& colormap(const std::vector<std::uint8_t>& lut) {
        if (lut.size() != custom_cmap_.size()) {
            throw std::invalid_argument("peep custom colormap must contain exactly 768 bytes");
        }
        std::copy(lut.begin(), lut.end(), custom_cmap_.begin());
        custom_cmap_set_ = true;
        return *this;
    }
    Options& colormap(const std::uint8_t (&lut)[768]) {
        std::copy(lut, lut + custom_cmap_.size(), custom_cmap_.begin());
        custom_cmap_set_ = true;
        return *this;
    }
    Options& nan_color(Color c) { nan_color_ = c; nan_color_set_ = true; return *this; }
    Options& nan_color(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
        return nan_color(Color{r, g, b});
    }
    Options& clear_nan_color() { nan_color_set_ = false; return *this; }
    Options& inf_color(Color c) {
        neg_inf_color_ = c;
        pos_inf_color_ = c;
        neg_inf_color_set_ = true;
        pos_inf_color_set_ = true;
        return *this;
    }
    Options& inf_color(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
        return inf_color(Color{r, g, b});
    }
    Options& inf_colors(Color neg, Color pos) {
        neg_inf_color_ = neg;
        pos_inf_color_ = pos;
        neg_inf_color_set_ = true;
        pos_inf_color_set_ = true;
        return *this;
    }
    Options& neg_inf_color(Color c) {
        neg_inf_color_ = c;
        neg_inf_color_set_ = true;
        return *this;
    }
    Options& neg_inf_color(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
        return neg_inf_color(Color{r, g, b});
    }
    Options& pos_inf_color(Color c) {
        pos_inf_color_ = c;
        pos_inf_color_set_ = true;
        return *this;
    }
    Options& pos_inf_color(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
        return pos_inf_color(Color{r, g, b});
    }
    Options& clear_inf_colors() {
        neg_inf_color_set_ = false;
        pos_inf_color_set_ = false;
        return *this;
    }
    Options& block_size(size_t s) {
        block_size_ = (s > 0) ? s : 1;
        block_size_set_ = true;
        return *this;
    }
    Options& layout(Layout l) { layout_ = l; return *this; }
    Options& ostream(std::ostream& os) { out_ = std::ref(os); return *this; }

    // Crop: from (x, y) to end of matrix.
    Options& crop(size_t x, size_t y) {
        crop_mode_ = CropMode::CornerToEnd;
        crop_x_ = x; crop_y_ = y;
        crop_h_ = 0; crop_w_ = 0;
        return *this;
    }
    // Crop: w x h window starting at (x, y).
    Options& crop(size_t x, size_t y, size_t w, size_t h) {
        crop_mode_ = CropMode::CornerWindow;
        crop_x_ = x; crop_y_ = y;
        crop_w_ = w; crop_h_ = h;
        return *this;
    }
    // Crop: w x h window centered on (center_x, center_y), padded outside the matrix.
    Options& center_crop(size_t center_x, size_t center_y, size_t w, size_t h) {
        crop_mode_ = CropMode::Center;
        crop_center_x_ = center_x;
        crop_center_y_ = center_y;
        crop_w_ = w;
        crop_h_ = h;
        crop_x_ = 0;
        crop_y_ = 0;
        return *this;
    }
    Options& fit(Fit f) { fit_ = f; return *this; }
    Options& resampling(Resampling r) { resampling_ = r; return *this; }
    Options& scalar() {
        source_mode_ = SourceMode::ScalarData;
        scalar_accessor_ = ScalarAccessor();
        rgb_accessor_ = RGBAccessor();
        return *this;
    }
    Options& rgb(RGBLayout layout = RGBLayout::Interleaved) {
        source_mode_ = SourceMode::RGBData;
        rgb_layout_ = layout;
        scalar_accessor_ = ScalarAccessor();
        rgb_accessor_ = RGBAccessor();
        return *this;
    }
    template <typename Accessor>
    Options& accessor(Accessor a) {
        source_mode_ = SourceMode::ScalarAccessor;
        scalar_accessor_ = ScalarAccessor(a);
        rgb_accessor_ = RGBAccessor();
        return *this;
    }
    template <typename Accessor>
    Options& rgb_accessor(Accessor a) {
        source_mode_ = SourceMode::RGBAccessor;
        rgb_accessor_ = RGBAccessor(a);
        scalar_accessor_ = ScalarAccessor();
        return *this;
    }
    Options& title(bool enabled = true) { title_show_ = enabled; return *this; }
    Options& title(const std::string& text) {
        title_show_ = true;
        title_text_ = text;
        return *this;
    }
    Options& title(const char* text) {
        title_show_ = true;
        title_text_ = text ? text : "";
        return *this;
    }
    Options& colorbar(bool enabled = true) {
        colorbar_ = enabled;
        return *this;
    }

private:
    double clim_lo_;
    double clim_hi_;
    Colormap colormap_;
    ColormapLut custom_cmap_;
    bool custom_cmap_set_;
    Color nan_color_;
    bool nan_color_set_;
    Color neg_inf_color_;
    bool neg_inf_color_set_;
    Color pos_inf_color_;
    bool pos_inf_color_set_;
    size_t block_size_;
    bool block_size_set_;
    Layout layout_;
    std::reference_wrapper<std::ostream> out_;
    size_t crop_x_;
    size_t crop_y_;
    size_t crop_h_;
    size_t crop_w_;
    CropMode crop_mode_;
    size_t crop_center_x_;
    size_t crop_center_y_;
    Fit fit_;
    Resampling resampling_;
    SourceMode source_mode_;
    RGBLayout rgb_layout_;
    ScalarAccessor scalar_accessor_;
    RGBAccessor rgb_accessor_;
    bool title_show_;
    std::string title_text_;
    bool colorbar_;
};

template <typename T>
void print(const T* data, size_t rows, size_t cols, const Options& opts = Options());
template <typename T>
void print(const std::vector<T>& data, size_t rows, size_t cols,
           const Options& opts = Options());
void print(size_t rows, size_t cols, const Options& opts);
template <typename T>
std::string to_string(const T* data, size_t rows, size_t cols,
                      const Options& opts = Options());
template <typename T>
std::string to_string(const std::vector<T>& data, size_t rows, size_t cols,
                      const Options& opts = Options());
std::string to_string(size_t rows, size_t cols, const Options& opts);

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

typedef Color RGB;

struct Pixel {
    RGB color;
    bool transparent;
};

// Computed grayscale colormap (avoids embedding trivial data)
inline ColormapLut make_gray_lut() {
    ColormapLut lut;
    for (size_t i = 0; i < 256; ++i) {
        std::uint8_t v = static_cast<std::uint8_t>(i);
        lut[i * 3 + 0] = v;
        lut[i * 3 + 1] = v;
        lut[i * 3 + 2] = v;
    }
    return lut;
}

inline const ColormapLut& cmap_gray() {
    static const ColormapLut lut = make_gray_lut();
    return lut;
}

struct ColormapEntry {
    Colormap id;
    const char* name;
    const ColormapLut* lut;
};

inline const ColormapEntry* colormap_entries(size_t& count) {
    static const ColormapEntry entries[] = {
        {Colormap::Gray,     "gray",     &cmap_gray()},
        {Colormap::Magma,    "magma",    &CMAP_MAGMA},
        {Colormap::Viridis,  "viridis",  &CMAP_VIRIDIS},
        {Colormap::Plasma,   "plasma",   &CMAP_PLASMA},
        {Colormap::Inferno,  "inferno",  &CMAP_INFERNO},
        {Colormap::Cividis,  "cividis",  &CMAP_CIVIDIS},
        {Colormap::Coolwarm, "coolwarm", &CMAP_COOLWARM},
        {Colormap::Gnuplot,  "gnuplot",  &CMAP_GNUPLOT},
        {Colormap::Turbo,    "turbo",    &CMAP_TURBO}
    };
    count = sizeof(entries) / sizeof(entries[0]);
    return entries;
}

inline Colormap colormap_from_string(const std::string& name) {
    std::string lower = str_tolower(name);
    if (lower == "grey") lower = "gray";

    size_t count = 0;
    const ColormapEntry* entries = colormap_entries(count);
    for (size_t i = 0; i < count; ++i) {
        if (lower == entries[i].name) return entries[i].id;
    }
    return Colormap::Gray;
}

inline const ColormapLut& find_colormap(Colormap cmap) {
    size_t count = 0;
    const ColormapEntry* entries = colormap_entries(count);
    for (size_t i = 0; i < count; ++i) {
        if (entries[i].id == cmap) return *entries[i].lut;
    }
    return cmap_gray();
}

inline const ColormapLut& find_colormap(const std::string& name) {
    return find_colormap(colormap_from_string(name));
}

inline const char* colormap_name(Colormap cmap) {
    size_t count = 0;
    const ColormapEntry* entries = colormap_entries(count);
    for (size_t i = 0; i < count; ++i) {
        if (entries[i].id == cmap) return entries[i].name;
    }
    return "gray";
}

// Resolve the effective colormap from Options (custom takes priority)
inline const ColormapLut& resolve_colormap(const Options& opts) {
    if (opts.custom_colormap()) return *opts.custom_colormap();
    return find_colormap(opts.colormap());
}

// Crop and fit planning shared by scalar and RGB renderers.
struct VisibleRegion {
    std::ptrdiff_t r0;
    std::ptrdiff_t c0;
    size_t rows;
    size_t cols;
    bool empty;
};

// Given the visible source size (vr, vc), block size, and a terminal size,
// decide output dimensions (out_r, out_c) and whether resampling is needed.
// Pure function of its inputs — the terminal query happens at the call site.
struct FitResolution {
    size_t out_r;
    size_t out_c;
    bool resample;
};

inline std::ptrdiff_t region_coord(size_t v) {
    const size_t max_coord =
        static_cast<size_t>((std::numeric_limits<std::ptrdiff_t>::max)());
    if (v > max_coord) return (std::numeric_limits<std::ptrdiff_t>::max)();
    return static_cast<std::ptrdiff_t>(v);
}

inline std::ptrdiff_t centered_region_start(size_t center, size_t extent) {
    return region_coord(center) - region_coord(extent / 2);
}

inline size_t region_extent_to_end(size_t start, size_t length) {
    return (start < length) ? (length - start) : 0;
}

inline size_t signed_magnitude(std::ptrdiff_t v) {
    return static_cast<size_t>(-(v + 1)) + 1;
}

inline bool resolve_source_index(std::ptrdiff_t start, size_t offset,
                                 size_t limit, size_t& index) {
    if (limit == 0) return false;

    if (start < 0) {
        const size_t pad = signed_magnitude(start);
        if (offset < pad) return false;
        index = offset - pad;
    } else {
        const size_t base = static_cast<size_t>(start);
        if (offset > (std::numeric_limits<size_t>::max)() - base) return false;
        index = base + offset;
    }

    return index < limit;
}

inline VisibleRegion resolve_visible_region(size_t rows, size_t cols,
                                            const Options& opts) {
    VisibleRegion r;
    r.r0 = 0;
    r.c0 = 0;
    r.rows = 0;
    r.cols = 0;
    r.empty = true;

    if (opts.crop_is_centered()) {
        r.r0 = centered_region_start(opts.crop_center_y(), opts.crop_h());
        r.c0 = centered_region_start(opts.crop_center_x(), opts.crop_w());
        r.rows = opts.crop_h();
        r.cols = opts.crop_w();
        r.empty = (r.rows == 0 || r.cols == 0);
        return r;
    }

    if (opts.crop_is_to_end()) {
        r.r0 = region_coord(opts.crop_y());
        r.c0 = region_coord(opts.crop_x());
        r.rows = region_extent_to_end(opts.crop_y(), rows);
        r.cols = region_extent_to_end(opts.crop_x(), cols);
        r.empty = (r.rows == 0 || r.cols == 0);
        return r;
    }

    if (opts.crop_is_window()) {
        r.r0 = region_coord(opts.crop_y());
        r.c0 = region_coord(opts.crop_x());
        r.rows = opts.crop_h();
        r.cols = opts.crop_w();
    } else {
        r.rows = rows;
        r.cols = cols;
    }
    r.empty = (r.rows == 0 || r.cols == 0);
    return r;
}

inline double half_block_pixel_aspect(TerminalSize ts) {
    if (!ts.valid || !ts.pixels_valid
        || ts.rows == 0 || ts.cols == 0
        || ts.width_px == 0 || ts.height_px == 0) {
        return 1.0;
    }

    const double cell_w = static_cast<double>(ts.width_px)
                          / static_cast<double>(ts.cols);
    const double cell_h = static_cast<double>(ts.height_px)
                          / static_cast<double>(ts.rows);
    const double aspect = (cell_h * 0.5) / cell_w;
    if (!std::isfinite(aspect) || aspect <= 0.0) return 1.0;
    return aspect;
}

inline size_t rounded_size(double v) {
    return (v > 1.0) ? static_cast<size_t>(v + 0.5) : 1;
}

inline size_t ceil_div(size_t n, size_t d) {
    return (d == 0) ? 1 : (n + d - 1) / d;
}

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
    const double pixel_aspect = half_block_pixel_aspect(ts);
    const bool correct_aspect = std::fabs(pixel_aspect - 1.0) > 0.01;
    if (pcols_needed <= max_pcols && prows_needed <= max_prows
        && (requested != Fit::Resample || !correct_aspect)) {
        return r;
    }

    if (requested == Fit::Resample) {
        // Aspect-preserving: pick a single uniform scale factor from the tighter
        // axis and apply to both. When the terminal reports pixel dimensions,
        // compensate for terminals whose half-block pixels are not quite square.
        const double width_scale =
            static_cast<double>(max_pcols) / static_cast<double>(pcols_needed);
        const double height_scale =
            (static_cast<double>(max_prows) * pixel_aspect)
            / static_cast<double>(prows_needed);
        double scale = std::min(width_scale, height_scale);
        if (scale > 1.0) scale = 1.0;

        const size_t cap_c = (max_pcols / bs) ? (max_pcols / bs) : 1;
        const size_t cap_r = (max_prows / bs) ? (max_prows / bs) : 1;
        r.out_c = rounded_size(static_cast<double>(vc) * scale);
        r.out_r = rounded_size(static_cast<double>(vr) * scale / pixel_aspect);
        if (r.out_c > cap_c) r.out_c = cap_c;
        if (r.out_r > cap_r) r.out_r = cap_r;
        r.resample = (r.out_r != vr || r.out_c != vc);
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

inline size_t terminal_rows_reserved_for_chrome(const Options& opts) {
    size_t rows = 0;
    if (opts.show_title()) ++rows;
    if (!opts.is_rgb() && opts.show_colorbar()) ++rows;
    return rows;
}

inline size_t resolve_effective_block_size(size_t vr, size_t vc,
                                           const Options& opts,
                                           TerminalSize ts) {
    const size_t requested = opts.block_size();
    if (opts.has_block_size() || !ts.valid || vr == 0 || vc == 0) {
        return requested;
    }

    const size_t reserved_rows = terminal_rows_reserved_for_chrome(opts);
    if (ts.rows <= reserved_rows || ts.cols == 0) return requested;

    const size_t max_pcols = ts.cols;
    const size_t max_prows = (ts.rows - reserved_rows) * 2;
    if (vc > max_pcols || vr > max_prows) return requested;

    const size_t max_by_cols = max_pcols / vc;
    const size_t max_by_rows = max_prows / vr;
    size_t max_bs = std::min(max_by_cols, max_by_rows);
    if (max_bs <= requested) return requested;

    const size_t kTargetPixelCols = 48;
    const size_t kTargetPixelRows = 32;
    const size_t kMaxAutoBlockSize = 8;

    const size_t target_pcols = std::min(max_pcols, kTargetPixelCols);
    const size_t target_prows = std::min(max_prows, kTargetPixelRows);
    size_t auto_bs = std::min(ceil_div(target_pcols, vc),
                              ceil_div(target_prows, vr));
    if (auto_bs < requested) auto_bs = requested;
    if (auto_bs > max_bs) auto_bs = max_bs;
    if (auto_bs > kMaxAutoBlockSize) auto_bs = kMaxAutoBlockSize;
    return auto_bs;
}

struct RenderPlan {
    VisibleRegion visible;
    size_t out_r;
    size_t out_c;
    size_t block_size;
    bool resample;
    bool empty;
};

inline RenderPlan make_render_plan(size_t rows, size_t cols,
                                   const Options& opts, std::ostream& os) {
    RenderPlan p;
    p.visible = resolve_visible_region(rows, cols, opts);
    p.out_r = 0;
    p.out_c = 0;
    p.block_size = opts.block_size();
    p.resample = false;
    p.empty = p.visible.empty;
    if (p.empty) return p;

    // Terminal fit only engages when the output is a real terminal;
    // ostringstream / pipes / files return an invalid size and we fall through
    // to full-size rendering (Fit::Off semantics).
    TerminalSize ts = query_terminal_size(os);
    p.block_size = resolve_effective_block_size(p.visible.rows, p.visible.cols,
                                                opts, ts);
    FitResolution fr = resolve_fit(p.visible.rows, p.visible.cols, p.block_size,
                                   opts.fit(), ts);
    p.out_r = fr.out_r;
    p.out_c = fr.out_c;
    p.resample = fr.resample;
    return p;
}

inline RGB lookup(double normalized, const ColormapLut& cmap) {
    if (std::isnan(normalized)) normalized = 0.0;

    const double scaled = normalized * 255.0 + 0.5;
    size_t idx = 0;
    if (scaled >= 255.0) {
        idx = 255;
    } else if (scaled > 0.0) {
        idx = static_cast<size_t>(scaled);
    }

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

inline Pixel transparent_pixel() {
    Pixel p;
    p.color = RGB{0, 0, 0};
    p.transparent = true;
    return p;
}

inline Pixel opaque_pixel(RGB c) {
    Pixel p;
    p.color = c;
    p.transparent = false;
    return p;
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

template <typename PixelAt>
inline void emit_pixel_body(std::ostringstream& buf, size_t out_r, size_t out_c,
                            size_t bs, PixelAt pixel_at) {
    size_t prows = out_r * bs;
    size_t pcols = out_c * bs;

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
            Pixel top = pixel_at(mr_top, mc);
            Pixel bot = transparent_pixel();
            if (has_bot) {
                size_t mr_bot = (pr + 1) / bs;
                bot = pixel_at(mr_bot, mc);
            }

            if (top.transparent && bot.transparent) {
                do_reset();
                buf << ' ';
            } else if (top.transparent) {
                do_reset();
                set_fg(bot.color);
                emit_lower_half(buf);
            } else if (bot.transparent) {
                do_reset();
                set_fg(top.color);
                emit_upper_half(buf);
            } else {
                set_bg(top.color);
                set_fg(bot.color);
                emit_lower_half(buf);
            }
        }

        do_reset();
        buf << '\n';
        bg_set = false;
        fg_set = false;
    }
}

inline std::string format_colorbar_value(double v) {
    std::ostringstream ss;
    ss << v;
    return ss.str();
}

inline void emit_colorbar(std::ostringstream& buf, const ColormapLut& cmap,
                          double lo, double hi, size_t image_cols) {
    if (image_cols == 0) return;

    const std::string lo_label = format_colorbar_value(lo);
    const std::string hi_label = format_colorbar_value(hi);
    size_t bar_width = image_cols;
    const size_t label_width = lo_label.size() + hi_label.size() + 2;
    if (image_cols > label_width) {
        bar_width = image_cols - label_width;
    } else {
        bar_width = 1;
    }

    buf << lo_label << ' ';
    for (size_t i = 0; i < bar_width; ++i) {
        const double t = (bar_width > 1)
            ? static_cast<double>(i) / static_cast<double>(bar_width - 1)
            : 0.5;
        emit_bg(buf, lookup(t, cmap));
        buf << ' ';
    }
    emit_reset(buf);
    buf << ' ' << hi_label << '\n';
}

inline std::string sanitize_title_text(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        out.push_back((c < 0x20 || c == 0x7f) ? ' ' : s[i]);
    }
    return out;
}

inline void append_title_summary(std::ostringstream& ss, const Options& opts,
                                 size_t rows, size_t cols,
                                 std::ptrdiff_t r0, std::ptrdiff_t c0,
                                 size_t vr, size_t vc,
                                 size_t out_r, size_t out_c,
                                 bool resample) {
    const std::string& label = opts.title_text();
    ss << (label.empty() ? "peep" : sanitize_title_text(label)) << ": ";
    ss << "data=" << rows << 'x' << cols;

    const bool cropped = (r0 != 0 || c0 != 0 || vr != rows || vc != cols);
    if (cropped) {
        ss << " crop=(" << c0 << ',' << r0 << ' ' << vc << 'x' << vr << ')';
    }

    if (out_r != vr || out_c != vc) {
        ss << " rendered as " << out_r << 'x' << out_c;
        ss << (resample ? " resampled" : " trimmed");
    }
}

inline std::string render_title(const Options& opts, size_t rows, size_t cols,
                                std::ptrdiff_t r0, std::ptrdiff_t c0,
                                size_t vr, size_t vc,
                                size_t out_r, size_t out_c,
                                bool resample, size_t block_size = 0) {
    std::ostringstream ss;
    append_title_summary(ss, opts, rows, cols, r0, c0, vr, vc,
                         out_r, out_c, resample);

    if (block_size == 0) block_size = opts.block_size();
    ss << " cmap=" << (opts.has_custom_colormap() ? "custom" : colormap_name(opts.colormap()));
    if (opts.layout() == Layout::ColMajor) ss << " layout=col-major";
    if (block_size != 1) ss << " block=" << block_size;
    return ss.str();
}

inline const char* rgb_layout_name(RGBLayout layout) {
    return (layout == RGBLayout::Planar) ? "planar" : "interleaved";
}

inline std::string render_rgb_title(const Options& opts, const char* rgb_source,
                                    size_t rows, size_t cols,
                                    std::ptrdiff_t r0, std::ptrdiff_t c0,
                                    size_t vr, size_t vc,
                                    size_t out_r, size_t out_c,
                                    bool resample, size_t block_size = 0) {
    std::ostringstream ss;
    append_title_summary(ss, opts, rows, cols, r0, c0, vr, vc,
                         out_r, out_c, resample);

    if (block_size == 0) block_size = opts.block_size();
    ss << " rgb=" << rgb_source;
    if (opts.layout() == Layout::ColMajor) ss << " layout=col-major";
    if (block_size != 1) ss << " block=" << block_size;
    return ss.str();
}

template <typename Get>
inline void apply_auto_clim(size_t rows, size_t cols, Get get,
                            double& lo, double& hi,
                            bool auto_lo, bool auto_hi) {
    if (!auto_lo && !auto_hi) return;

    double flo = std::numeric_limits<double>::infinity();
    double fhi = -std::numeric_limits<double>::infinity();
    for (size_t r = 0; r < rows; ++r) {
        for (size_t c = 0; c < cols; ++c) {
            double v = get(r, c);
            if (std::isnan(v) || std::isinf(v)) continue;
            if (v < flo) flo = v;
            if (v > fhi) fhi = v;
        }
    }
    if (auto_lo) lo = flo;
    if (auto_hi) hi = fhi;
}

inline double resample_coord(size_t out_i, size_t out_n, size_t in_n) {
    if (in_n <= 1 || out_n == 0) return 0.0;
    // Center-aligned mapping keeps downsampled pixels representative of the
    // source area they cover instead of always sampling from the upper-left.
    double coord = ((static_cast<double>(out_i) + 0.5)
                    * static_cast<double>(in_n)
                    / static_cast<double>(out_n)) - 0.5;
    const double max_coord = static_cast<double>(in_n - 1);
    if (coord < 0.0) coord = 0.0;
    if (coord > max_coord) coord = max_coord;
    return coord;
}

template <typename Get>
inline double sample_nearest(size_t mr, size_t mc, size_t out_r, size_t out_c,
                             size_t src_r, size_t src_c, Get get) {
    size_t sr = (mr * src_r) / out_r;
    size_t sc = (mc * src_c) / out_c;
    return get(sr, sc);
}

template <typename Get>
inline double sample_bilinear(size_t mr, size_t mc, size_t out_r, size_t out_c,
                              size_t src_r, size_t src_c, Get get) {
    const double r = resample_coord(mr, out_r, src_r);
    const double c = resample_coord(mc, out_c, src_c);
    const size_t r0 = static_cast<size_t>(std::floor(r));
    const size_t c0 = static_cast<size_t>(std::floor(c));
    const size_t r1 = std::min(r0 + 1, src_r - 1);
    const size_t c1 = std::min(c0 + 1, src_c - 1);
    const double wr = r - static_cast<double>(r0);
    const double wc = c - static_cast<double>(c0);

    const double v00 = get(r0, c0);
    const double v01 = get(r0, c1);
    const double v10 = get(r1, c0);
    const double v11 = get(r1, c1);
    const double w00 = (1.0 - wr) * (1.0 - wc);
    const double w01 = (1.0 - wr) * wc;
    const double w10 = wr * (1.0 - wc);
    const double w11 = wr * wc;

    // Preserve NaN/Inf semantics rather than blending or spreading them.
    if ((w00 > 0.0 && !std::isfinite(v00))
        || (w01 > 0.0 && !std::isfinite(v01))
        || (w10 > 0.0 && !std::isfinite(v10))
        || (w11 > 0.0 && !std::isfinite(v11))) {
        return sample_nearest(mr, mc, out_r, out_c, src_r, src_c, get);
    }

    return v00 * w00 + v01 * w01 + v10 * w10 + v11 * w11;
}

template <typename Get>
inline double sample_resampled(size_t mr, size_t mc, size_t out_r, size_t out_c,
                               size_t src_r, size_t src_c, Get get,
                               Resampling resampling) {
    if (resampling == Resampling::Nearest) {
        return sample_nearest(mr, mc, out_r, out_c, src_r, src_c, get);
    }
    return sample_bilinear(mr, mc, out_r, out_c, src_r, src_c, get);
}

inline std::uint8_t clamp_channel(double v) {
    if (v <= 0.0) return 0;
    if (v >= 255.0) return 255;
    return static_cast<std::uint8_t>(v + 0.5);
}

inline RGB blend_rgb(RGB c00, RGB c01, RGB c10, RGB c11,
                     double w00, double w01, double w10, double w11) {
    RGB c;
    c.r = clamp_channel(c00.r * w00 + c01.r * w01 + c10.r * w10 + c11.r * w11);
    c.g = clamp_channel(c00.g * w00 + c01.g * w01 + c10.g * w10 + c11.g * w11);
    c.b = clamp_channel(c00.b * w00 + c01.b * w01 + c10.b * w10 + c11.b * w11);
    return c;
}

template <typename Get>
inline RGB sample_rgb_nearest(size_t mr, size_t mc, size_t out_r, size_t out_c,
                              size_t src_r, size_t src_c, Get get) {
    size_t sr = (mr * src_r) / out_r;
    size_t sc = (mc * src_c) / out_c;
    return get(sr, sc);
}

template <typename Get>
inline RGB sample_rgb_bilinear(size_t mr, size_t mc, size_t out_r, size_t out_c,
                               size_t src_r, size_t src_c, Get get) {
    const double r = resample_coord(mr, out_r, src_r);
    const double c = resample_coord(mc, out_c, src_c);
    const size_t r0 = static_cast<size_t>(std::floor(r));
    const size_t c0 = static_cast<size_t>(std::floor(c));
    const size_t r1 = std::min(r0 + 1, src_r - 1);
    const size_t c1 = std::min(c0 + 1, src_c - 1);
    const double wr = r - static_cast<double>(r0);
    const double wc = c - static_cast<double>(c0);

    const double w00 = (1.0 - wr) * (1.0 - wc);
    const double w01 = (1.0 - wr) * wc;
    const double w10 = wr * (1.0 - wc);
    const double w11 = wr * wc;
    return blend_rgb(get(r0, c0), get(r0, c1), get(r1, c0), get(r1, c1),
                     w00, w01, w10, w11);
}

template <typename Get>
inline RGB sample_rgb_resampled(size_t mr, size_t mc, size_t out_r, size_t out_c,
                                size_t src_r, size_t src_c, Get get,
                                Resampling resampling) {
    if (resampling == Resampling::Nearest) {
        return sample_rgb_nearest(mr, mc, out_r, out_c, src_r, src_c, get);
    }
    return sample_rgb_bilinear(mr, mc, out_r, out_c, src_r, src_c, get);
}

template <typename GetSource>
void render_scalar_source(size_t rows, size_t cols, GetSource get_source_abs,
                          const Options& opts) {
    const ColormapLut& cmap = resolve_colormap(opts);
    std::ostream& os = opts.ostream();

    RenderPlan plan = make_render_plan(rows, cols, opts, os);
    if (plan.empty) return;
    const size_t bs = plan.block_size;
    const std::ptrdiff_t r0 = plan.visible.r0;
    const std::ptrdiff_t c0 = plan.visible.c0;
    const size_t vr = plan.visible.rows;
    const size_t vc = plan.visible.cols;
    const size_t out_r = plan.out_r;
    const size_t out_c = plan.out_c;
    const bool resample = plan.resample;

    auto get_source = [&](size_t sr, size_t sc) -> double {
        size_t source_r = 0;
        size_t source_c = 0;
        if (!resolve_source_index(r0, sr, rows, source_r)
            || !resolve_source_index(c0, sc, cols, source_c)) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        return static_cast<double>(get_source_abs(source_r, source_c));
    };
    auto get = [&](size_t mr, size_t mc) -> double {
        if (!resample) return get_source(mr, mc);
        return sample_resampled(mr, mc, out_r, out_c, vr, vc,
                                get_source, opts.resampling());
    };

    // Compute clim from visible region only
    double lo = opts.clim_lo();
    double hi = opts.clim_hi();
    bool auto_lo = std::isnan(lo);
    bool auto_hi = std::isnan(hi);

    apply_auto_clim(vr, vc, get_source, lo, hi, auto_lo, auto_hi);

    if (std::isinf(lo) || std::isinf(hi)) { lo = 0.0; hi = 1.0; }
    double range = (hi != lo) ? (hi - lo) : 1.0;

    auto normalize = [&](double v) -> double {
        if (std::isinf(v)) return (v > 0) ? 1.0 : 0.0;
        double n = (v - lo) / range;
        if (n < 0.0) n = 0.0;
        if (n > 1.0) n = 1.0;
        return n;
    };

    // Buffer all output, then flush once
    std::ostringstream buf;
    if (opts.show_title()) {
        buf << render_title(opts, rows, cols, r0, c0, vr, vc,
                            out_r, out_c, resample, bs)
            << '\n';
    }

    auto pixel_at = [&](size_t mr, size_t mc) -> Pixel {
        double v = get(mr, mc);
        if (std::isnan(v) && !opts.has_nan_color()) return transparent_pixel();
        return opaque_pixel(color_for_value(v, cmap, opts, normalize(v)));
    };

    emit_pixel_body(buf, out_r, out_c, bs, pixel_at);
    if (opts.show_colorbar()) {
        emit_colorbar(buf, cmap, lo, hi, out_c * bs);
    }

    os << buf.str();
}

template <typename T>
void render(const T* data, size_t rows, size_t cols, const Options& opts) {
    if (!data) return;
    const bool col_major = (opts.layout() == Layout::ColMajor);

    auto get_source = [&](size_t r, size_t c) -> double {
        if (col_major)
            return static_cast<double>(data[c * rows + r]);
        return static_cast<double>(data[r * cols + c]);
    };

    render_scalar_source(rows, cols, get_source, opts);
}

template <typename GetSource>
void render_rgb_source(size_t rows, size_t cols, GetSource get_source_abs,
                       const char* rgb_source, const Options& opts) {
    std::ostream& os = opts.ostream();

    RenderPlan plan = make_render_plan(rows, cols, opts, os);
    if (plan.empty) return;
    const size_t bs = plan.block_size;
    const std::ptrdiff_t r0 = plan.visible.r0;
    const std::ptrdiff_t c0 = plan.visible.c0;
    const size_t vr = plan.visible.rows;
    const size_t vc = plan.visible.cols;
    const size_t out_r = plan.out_r;
    const size_t out_c = plan.out_c;
    const bool resample = plan.resample;

    auto get_source = [&](size_t sr, size_t sc) -> RGB {
        size_t source_r = 0;
        size_t source_c = 0;
        if (!resolve_source_index(r0, sr, rows, source_r)
            || !resolve_source_index(c0, sc, cols, source_c)) {
            return RGB{0, 0, 0};
        }
        return get_source_abs(source_r, source_c);
    };

    auto get = [&](size_t mr, size_t mc) -> RGB {
        if (!resample) return get_source(mr, mc);
        return sample_rgb_resampled(mr, mc, out_r, out_c, vr, vc,
                                    get_source, opts.resampling());
    };

    std::ostringstream buf;
    if (opts.show_title()) {
        buf << render_rgb_title(opts, rgb_source, rows, cols,
                                r0, c0, vr, vc, out_r, out_c, resample, bs)
            << '\n';
    }

    auto pixel_at = [&](size_t mr, size_t mc) -> Pixel {
        return opaque_pixel(get(mr, mc));
    };
    emit_pixel_body(buf, out_r, out_c, bs, pixel_at);

    os << buf.str();
}

inline void render_rgb(const std::uint8_t* data, size_t rows, size_t cols,
                       RGBLayout rgb_layout, const Options& opts) {
    if (!data) return;
    const bool col_major = (opts.layout() == Layout::ColMajor);
    const size_t plane = rows * cols;

    auto spatial_index = [&](size_t r, size_t c) -> size_t {
        return col_major ? (c * rows + r) : (r * cols + c);
    };

    auto get_source = [&](size_t r, size_t c) -> RGB {
        const size_t i = spatial_index(r, c);
        if (rgb_layout == RGBLayout::Planar) {
            return RGB{data[i], data[plane + i], data[2 * plane + i]};
        }
        const size_t j = i * 3;
        return RGB{data[j], data[j + 1], data[j + 2]};
    };

    render_rgb_source(rows, cols, get_source, rgb_layout_name(rgb_layout), opts);
}

inline void validate_vector_size(size_t size, size_t rows, size_t cols) {
    if (rows == 0 || cols == 0) return;
    if (cols != 0 && rows > (std::numeric_limits<size_t>::max() / cols)) {
        throw std::invalid_argument("peep vector dimensions overflow size_t");
    }
    const size_t expected = rows * cols;
    if (size != expected) {
        throw std::invalid_argument("peep vector size must equal rows * cols");
    }
}

inline void validate_rgb_vector_size(size_t size, size_t rows, size_t cols) {
    if (rows == 0 || cols == 0) return;
    if (cols != 0 && rows > (std::numeric_limits<size_t>::max() / cols)) {
        throw std::invalid_argument("peep RGB vector dimensions overflow size_t");
    }
    const size_t pixels = rows * cols;
    if (pixels > (std::numeric_limits<size_t>::max() / 3)) {
        throw std::invalid_argument("peep RGB vector dimensions overflow size_t");
    }
    const size_t expected = pixels * 3;
    if (size != expected) {
        throw std::invalid_argument("peep RGB vector size must equal rows * cols * 3");
    }
}

inline void render_from_options(size_t rows, size_t cols, const Options& opts) {
    if (opts.is_rgb()) {
        if (!opts.has_rgb_accessor()) {
            throw std::invalid_argument("peep RGB accessor is required for print(rows, cols, opts)");
        }
        render_rgb_source(rows, cols, opts.rgb_accessor(), "accessor", opts);
        return;
    }
    if (!opts.has_accessor()) {
        throw std::invalid_argument("peep accessor is required for print(rows, cols, opts)");
    }
    render_scalar_source(rows, cols, opts.accessor(), opts);
}

inline std::string render_options_to_string(size_t rows, size_t cols, const Options& opts) {
    std::ostringstream oss;
    Options local = opts;
    local.ostream(oss);
    render_from_options(rows, cols, local);
    return oss.str();
}

template <typename T>
inline void render_data_as_rgb(const T*, size_t, size_t, const Options&, std::false_type) {
    throw std::invalid_argument("peep RGB input data must be std::uint8_t");
}

inline void render_data_as_rgb(const std::uint8_t* data, size_t rows, size_t cols,
                               const Options& opts, std::true_type) {
    render_rgb(data, rows, cols, opts.rgb_layout(), opts);
}

template <typename T>
inline void render_data(const T* data, size_t rows, size_t cols, const Options& opts) {
    if (opts.has_accessor() || opts.has_rgb_accessor()) {
        throw std::invalid_argument("peep accessor options require print(rows, cols, opts)");
    }
    if (opts.is_rgb()) {
        render_data_as_rgb(data, rows, cols, opts, typename std::is_same<T, std::uint8_t>::type());
    } else {
        render(data, rows, cols, opts);
    }
}

template <typename T>
inline std::string render_data_to_string(const T* data, size_t rows, size_t cols,
                                         const Options& opts) {
    std::ostringstream oss;
    Options local = opts;
    local.ostream(oss);
    render_data(data, rows, cols, local);
    return oss.str();
}

template <typename T>
inline void validate_data_vector_size(size_t size, size_t rows, size_t cols,
                                      const Options& opts) {
    if (opts.has_accessor() || opts.has_rgb_accessor()) {
        throw std::invalid_argument("peep accessor options require print(rows, cols, opts)");
    }
    if (opts.is_rgb()) {
        if (!std::is_same<T, std::uint8_t>::value) {
            throw std::invalid_argument("peep RGB vector input must contain std::uint8_t values");
        }
        validate_rgb_vector_size(size, rows, cols);
    } else {
        validate_vector_size(size, rows, cols);
    }
}

} // namespace detail

inline Colormap default_colormap() { return detail::default_colormap_ref(); }
inline void set_default_colormap(Colormap c) { detail::default_colormap_ref() = c; }

inline Options::Options()
    : clim_lo_(std::numeric_limits<double>::quiet_NaN()),
      clim_hi_(std::numeric_limits<double>::quiet_NaN()),
      colormap_(default_colormap()),
      custom_cmap_(),
      custom_cmap_set_(false),
      nan_color_(),
      nan_color_set_(false),
      neg_inf_color_(),
      neg_inf_color_set_(false),
      pos_inf_color_(),
      pos_inf_color_set_(false),
      block_size_(1),
      block_size_set_(false),
      layout_(Layout::RowMajor),
      out_(std::cout),
      crop_x_(0),
      crop_y_(0),
      crop_h_(0),
      crop_w_(0),
      crop_mode_(CropMode::None),
      crop_center_x_(0),
      crop_center_y_(0),
      fit_(Fit::Resample),
      resampling_(Resampling::Bilinear),
      source_mode_(SourceMode::ScalarData),
      rgb_layout_(RGBLayout::Interleaved),
      scalar_accessor_(),
      rgb_accessor_(),
      title_show_(false),
      title_text_(),
      colorbar_(true) {}

inline Options& Options::colormap(const std::string& name) {
    custom_cmap_set_ = false;
    colormap_ = detail::colormap_from_string(name);
    return *this;
}

template <typename T>
void print(const T* data, size_t rows, size_t cols, const Options& opts) {
    detail::render_data(data, rows, cols, opts);
}

template <typename T>
void print(const std::vector<T>& data, size_t rows, size_t cols, const Options& opts) {
    detail::validate_data_vector_size<T>(data.size(), rows, cols, opts);
    detail::render_data(data.data(), rows, cols, opts);
}

inline void print(size_t rows, size_t cols, const Options& opts) {
    detail::render_from_options(rows, cols, opts);
}

template <typename T>
std::string to_string(const T* data, size_t rows, size_t cols, const Options& opts) {
    return detail::render_data_to_string(data, rows, cols, opts);
}

template <typename T>
std::string to_string(const std::vector<T>& data, size_t rows, size_t cols, const Options& opts) {
    detail::validate_data_vector_size<T>(data.size(), rows, cols, opts);
    return detail::render_data_to_string(data.data(), rows, cols, opts);
}

inline std::string to_string(size_t rows, size_t cols, const Options& opts) {
    return detail::render_options_to_string(rows, cols, opts);
}

} // namespace peep

#endif // PEEP_PEEP_HPP
