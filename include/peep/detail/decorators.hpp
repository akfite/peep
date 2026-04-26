#ifndef PEEP_DETAIL_DECORATORS_HPP
#define PEEP_DETAIL_DECORATORS_HPP

#include <cstddef>
#include <sstream>
#include <string>

namespace peep {
namespace detail {

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

inline std::string rgb_layout_name(RGBLayout layout) {
    return (layout == RGBLayout::Planar) ? "planar" : "interleaved";
}

inline std::string render_rgb_title(const Options& opts, const std::string& rgb_source,
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

} // namespace detail
} // namespace peep

#endif // PEEP_DETAIL_DECORATORS_HPP
