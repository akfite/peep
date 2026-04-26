#ifndef PEEP_DETAIL_COLORMAP_HPP
#define PEEP_DETAIL_COLORMAP_HPP

#include <cmath>

#include "peep/detail/colormap_registry.hpp"

namespace peep {
namespace detail {

// Resolve the effective colormap from Options (custom takes priority).
inline const ColormapLut& resolve_colormap(const Options& opts) {
    if (opts.custom_colormap()) return *opts.custom_colormap();
    return find_colormap(opts.colormap());
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

} // namespace detail
} // namespace peep

#endif // PEEP_DETAIL_COLORMAP_HPP
