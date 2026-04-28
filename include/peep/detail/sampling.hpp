#ifndef PEEP_DETAIL_SAMPLING_HPP
#define PEEP_DETAIL_SAMPLING_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace peep {
namespace detail {

template <typename ScalarAt>
inline void apply_auto_clim(size_t rows, size_t cols, ScalarAt scalar_at,
                            double& lo, double& hi,
                            bool auto_lo, bool auto_hi) {
    if (!auto_lo && !auto_hi) return;

    double flo = std::numeric_limits<double>::infinity();
    double fhi = -std::numeric_limits<double>::infinity();
    for (size_t r = 0; r < rows; ++r) {
        for (size_t c = 0; c < cols; ++c) {
            double v = scalar_at(r, c);
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

template <typename ScalarAt>
inline double sample_nearest(size_t mr, size_t mc, size_t out_r, size_t out_c,
                             size_t src_r, size_t src_c, ScalarAt scalar_at) {
    size_t sr = (mr * src_r) / out_r;
    size_t sc = (mc * src_c) / out_c;
    return scalar_at(sr, sc);
}

template <typename ScalarAt>
inline double sample_bilinear(size_t mr, size_t mc, size_t out_r, size_t out_c,
                              size_t src_r, size_t src_c, ScalarAt scalar_at) {
    const double r = resample_coord(mr, out_r, src_r);
    const double c = resample_coord(mc, out_c, src_c);
    const size_t r0 = static_cast<size_t>(std::floor(r));
    const size_t c0 = static_cast<size_t>(std::floor(c));
    const size_t r1 = std::min(r0 + 1, src_r - 1);
    const size_t c1 = std::min(c0 + 1, src_c - 1);
    const double wr = r - static_cast<double>(r0);
    const double wc = c - static_cast<double>(c0);

    const double v00 = scalar_at(r0, c0);
    const double v01 = scalar_at(r0, c1);
    const double v10 = scalar_at(r1, c0);
    const double v11 = scalar_at(r1, c1);
    const double w00 = (1.0 - wr) * (1.0 - wc);
    const double w01 = (1.0 - wr) * wc;
    const double w10 = wr * (1.0 - wc);
    const double w11 = wr * wc;

    // Preserve NaN/Inf semantics rather than blending or spreading them.
    if ((w00 > 0.0 && !std::isfinite(v00))
        || (w01 > 0.0 && !std::isfinite(v01))
        || (w10 > 0.0 && !std::isfinite(v10))
        || (w11 > 0.0 && !std::isfinite(v11))) {
        return sample_nearest(mr, mc, out_r, out_c, src_r, src_c, scalar_at);
    }

    return v00 * w00 + v01 * w01 + v10 * w10 + v11 * w11;
}

template <typename ScalarAt>
inline double sample_resampled(size_t mr, size_t mc, size_t out_r, size_t out_c,
                               size_t src_r, size_t src_c, ScalarAt scalar_at,
                               Resampling resampling) {
    if (resampling == Resampling::Nearest) {
        return sample_nearest(mr, mc, out_r, out_c, src_r, src_c, scalar_at);
    }
    return sample_bilinear(mr, mc, out_r, out_c, src_r, src_c, scalar_at);
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

template <typename RgbAt>
inline RGB sample_rgb_nearest(size_t mr, size_t mc, size_t out_r, size_t out_c,
                              size_t src_r, size_t src_c, RgbAt rgb_at) {
    size_t sr = (mr * src_r) / out_r;
    size_t sc = (mc * src_c) / out_c;
    return rgb_at(sr, sc);
}

template <typename RgbAt>
inline RGB sample_rgb_bilinear(size_t mr, size_t mc, size_t out_r, size_t out_c,
                               size_t src_r, size_t src_c, RgbAt rgb_at) {
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
    return blend_rgb(rgb_at(r0, c0), rgb_at(r0, c1),
                     rgb_at(r1, c0), rgb_at(r1, c1),
                     w00, w01, w10, w11);
}

template <typename RgbAt>
inline RGB sample_rgb_resampled(size_t mr, size_t mc, size_t out_r, size_t out_c,
                                size_t src_r, size_t src_c, RgbAt rgb_at,
                                Resampling resampling) {
    if (resampling == Resampling::Nearest) {
        return sample_rgb_nearest(mr, mc, out_r, out_c, src_r, src_c, rgb_at);
    }
    return sample_rgb_bilinear(mr, mc, out_r, out_c, src_r, src_c, rgb_at);
}

template <typename PixelAt>
inline Pixel sample_pixel_nearest(size_t mr, size_t mc, size_t out_r, size_t out_c,
                                  size_t src_r, size_t src_c, PixelAt pixel_at) {
    size_t sr = (mr * src_r) / out_r;
    size_t sc = (mc * src_c) / out_c;
    return pixel_at(sr, sc);
}

template <typename PixelAt>
inline Pixel sample_pixel_bilinear(size_t mr, size_t mc, size_t out_r, size_t out_c,
                                   size_t src_r, size_t src_c, PixelAt pixel_at) {
    const double r = resample_coord(mr, out_r, src_r);
    const double c = resample_coord(mc, out_c, src_c);
    const size_t r0 = static_cast<size_t>(std::floor(r));
    const size_t c0 = static_cast<size_t>(std::floor(c));
    const size_t r1 = std::min(r0 + 1, src_r - 1);
    const size_t c1 = std::min(c0 + 1, src_c - 1);
    const double wr = r - static_cast<double>(r0);
    const double wc = c - static_cast<double>(c0);

    const Pixel p00 = pixel_at(r0, c0);
    const Pixel p01 = pixel_at(r0, c1);
    const Pixel p10 = pixel_at(r1, c0);
    const Pixel p11 = pixel_at(r1, c1);
    const double w00 = (1.0 - wr) * (1.0 - wc);
    const double w01 = (1.0 - wr) * wc;
    const double w10 = wr * (1.0 - wc);
    const double w11 = wr * wc;

    if ((w00 > 0.0 && (p00.transparent || p00.force_nearest))
        || (w01 > 0.0 && (p01.transparent || p01.force_nearest))
        || (w10 > 0.0 && (p10.transparent || p10.force_nearest))
        || (w11 > 0.0 && (p11.transparent || p11.force_nearest))) {
        return sample_pixel_nearest(mr, mc, out_r, out_c, src_r, src_c, pixel_at);
    }

    return opaque_pixel(blend_rgb(p00.color, p01.color, p10.color, p11.color,
                                  w00, w01, w10, w11));
}

template <typename PixelAt>
inline Pixel sample_pixel_resampled(size_t mr, size_t mc, size_t out_r, size_t out_c,
                                    size_t src_r, size_t src_c, PixelAt pixel_at,
                                    Resampling resampling) {
    if (resampling == Resampling::Nearest) {
        return sample_pixel_nearest(mr, mc, out_r, out_c, src_r, src_c, pixel_at);
    }
    return sample_pixel_bilinear(mr, mc, out_r, out_c, src_r, src_c, pixel_at);
}

} // namespace detail
} // namespace peep

#endif // PEEP_DETAIL_SAMPLING_HPP
