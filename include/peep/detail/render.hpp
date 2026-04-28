#ifndef PEEP_DETAIL_RENDER_HPP
#define PEEP_DETAIL_RENDER_HPP

#include <cstddef>
#include <cstdint>
#include <cmath>
#include <limits>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace peep {
namespace detail {

enum class RenderTitleKind {
    Scalar,
    RGB
};

struct RenderDecorators {
    RenderTitleKind title_kind;
    std::string rgb_source;
    const ColormapLut* colorbar_cmap;
    double colorbar_lo;
    double colorbar_hi;
    bool show_colorbar;
};

template <typename PixelSource>
void render_pixel_source(size_t rows, size_t cols, const RenderPlan& plan,
                         PixelSource source_pixel_at,
                         const RenderDecorators& decorators,
                         const Options& opts) {
    if (plan.empty) return;

    std::ostream& os = opts.ostream();
    const size_t bs = plan.block_size;
    const std::ptrdiff_t r0 = plan.visible.r0;
    const std::ptrdiff_t c0 = plan.visible.c0;
    const size_t vr = plan.visible.rows;
    const size_t vc = plan.visible.cols;
    const size_t out_r = plan.out_r;
    const size_t out_c = plan.out_c;
    const bool resample = plan.resample;

    auto output_pixel_at = [&](size_t mr, size_t mc) -> Pixel {
        if (!resample) return source_pixel_at(mr, mc);
        return sample_pixel_resampled(mr, mc, out_r, out_c, vr, vc,
                                      source_pixel_at, opts.resampling());
    };

    std::ostringstream buf;
    if (opts.show_title()) {
        if (decorators.title_kind == RenderTitleKind::Scalar) {
            buf << render_title(opts, rows, cols, r0, c0, vr, vc,
                                out_r, out_c, resample, bs)
                << '\n';
        } else {
            buf << render_rgb_title(opts, decorators.rgb_source, rows, cols,
                                    r0, c0, vr, vc, out_r, out_c, resample, bs)
                << '\n';
        }
    }

    emit_pixel_body(buf, out_r, out_c, bs, output_pixel_at);

    if (decorators.show_colorbar && decorators.colorbar_cmap) {
        emit_colorbar(buf, *decorators.colorbar_cmap, decorators.colorbar_lo,
                      decorators.colorbar_hi, out_c * bs);
    }

    os << buf.str();
}

template <typename ScalarSource>
void render_scalar_source(size_t rows, size_t cols, ScalarSource source_value_at,
                          const Options& opts) {
    const ColormapLut& cmap = resolve_colormap(opts);
    std::ostream& os = opts.ostream();
    prepare_terminal_output(os);

    RenderPlan plan = make_render_plan(rows, cols, opts, os);
    if (plan.empty) return;
    const std::ptrdiff_t r0 = plan.visible.r0;
    const std::ptrdiff_t c0 = plan.visible.c0;
    const size_t vr = plan.visible.rows;
    const size_t vc = plan.visible.cols;

    auto visible_value_at = [&](size_t sr, size_t sc) -> double {
        size_t source_r = 0;
        size_t source_c = 0;
        if (!resolve_source_index(r0, sr, rows, source_r)
            || !resolve_source_index(c0, sc, cols, source_c)) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        return static_cast<double>(source_value_at(source_r, source_c));
    };

    // Compute clim from visible region only.
    double lo = opts.clim_lo();
    double hi = opts.clim_hi();
    bool auto_lo = std::isnan(lo);
    bool auto_hi = std::isnan(hi);

    apply_auto_clim(vr, vc, visible_value_at, lo, hi, auto_lo, auto_hi);

    if (std::isinf(lo) || std::isinf(hi)) { lo = 0.0; hi = 1.0; }
    double range = (hi != lo) ? (hi - lo) : 1.0;

    auto normalize = [&](double v) -> double {
        if (std::isinf(v)) return (v > 0) ? 1.0 : 0.0;
        double n = (v - lo) / range;
        if (n < 0.0) n = 0.0;
        if (n > 1.0) n = 1.0;
        return n;
    };

    auto visible_pixel_at = [&](size_t sr, size_t sc) -> Pixel {
        double v = visible_value_at(sr, sc);
        if (std::isnan(v) && !opts.has_nan_color()) return transparent_pixel();
        return opaque_pixel(color_for_value(v, cmap, opts, normalize(v)),
                            !std::isfinite(v));
    };

    RenderDecorators decorators;
    decorators.title_kind = RenderTitleKind::Scalar;
    decorators.rgb_source.clear();
    decorators.colorbar_cmap = &cmap;
    decorators.colorbar_lo = lo;
    decorators.colorbar_hi = hi;
    decorators.show_colorbar = opts.show_colorbar();
    render_pixel_source(rows, cols, plan, visible_pixel_at, decorators, opts);
}

template <typename T>
void render(const T* data, size_t rows, size_t cols, const Options& opts) {
    if (!data) return;
    const bool col_major = (opts.layout() == Layout::ColMajor);

    auto scalar_at = [&](size_t r, size_t c) -> double {
        if (col_major)
            return static_cast<double>(data[c * rows + r]);
        return static_cast<double>(data[r * cols + c]);
    };

    render_scalar_source(rows, cols, scalar_at, opts);
}

template <typename RgbSource>
void render_rgb_source(size_t rows, size_t cols, RgbSource source_rgb_at,
                       const std::string& rgb_source, const Options& opts) {
    std::ostream& os = opts.ostream();
    prepare_terminal_output(os);

    RenderPlan plan = make_render_plan(rows, cols, opts, os);
    if (plan.empty) return;
    const std::ptrdiff_t r0 = plan.visible.r0;
    const std::ptrdiff_t c0 = plan.visible.c0;

    auto visible_pixel_at = [&](size_t sr, size_t sc) -> Pixel {
        size_t source_r = 0;
        size_t source_c = 0;
        if (!resolve_source_index(r0, sr, rows, source_r)
            || !resolve_source_index(c0, sc, cols, source_c)) {
            return opaque_pixel(RGB{0, 0, 0});
        }
        return opaque_pixel(source_rgb_at(source_r, source_c));
    };

    RenderDecorators decorators;
    decorators.title_kind = RenderTitleKind::RGB;
    decorators.rgb_source = rgb_source;
    decorators.colorbar_cmap = nullptr;
    decorators.colorbar_lo = 0.0;
    decorators.colorbar_hi = 0.0;
    decorators.show_colorbar = false;
    render_pixel_source(rows, cols, plan, visible_pixel_at, decorators, opts);
}

inline void render_rgb(const std::uint8_t* data, size_t rows, size_t cols,
                       RGBLayout rgb_layout, const Options& opts) {
    if (!data) return;
    const bool col_major = (opts.layout() == Layout::ColMajor);
    const size_t plane = rows * cols;

    auto spatial_index = [&](size_t r, size_t c) -> size_t {
        return col_major ? (c * rows + r) : (r * cols + c);
    };

    auto rgb_at = [&](size_t r, size_t c) -> RGB {
        const size_t i = spatial_index(r, c);
        if (rgb_layout == RGBLayout::Planar) {
            return RGB{data[i], data[plane + i], data[2 * plane + i]};
        }
        const size_t j = i * 3;
        return RGB{data[j], data[j + 1], data[j + 2]};
    };

    render_rgb_source(rows, cols, rgb_at, rgb_layout_name(rgb_layout), opts);
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
            throw std::invalid_argument("peep RGB accessor is required for show(rows, cols, opts)");
        }
        render_rgb_source(rows, cols, opts.rgb_accessor(), std::string("accessor"), opts);
        return;
    }
    if (!opts.has_accessor()) {
        throw std::invalid_argument("peep accessor is required for show(rows, cols, opts)");
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
        throw std::invalid_argument("peep accessor options require show(rows, cols, opts)");
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
        throw std::invalid_argument("peep accessor options require show(rows, cols, opts)");
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
} // namespace peep

#endif // PEEP_DETAIL_RENDER_HPP
