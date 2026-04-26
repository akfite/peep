#ifndef PEEP_DETAIL_REGION_HPP
#define PEEP_DETAIL_REGION_HPP

#include <cstddef>
#include <limits>

namespace peep {
namespace detail {

// Crop and fit planning shared by scalar and RGB renderers.
struct VisibleRegion {
    std::ptrdiff_t r0;
    std::ptrdiff_t c0;
    size_t rows;
    size_t cols;
    bool empty;
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

} // namespace detail
} // namespace peep

#endif // PEEP_DETAIL_REGION_HPP
