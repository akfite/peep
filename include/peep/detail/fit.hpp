#ifndef PEEP_DETAIL_FIT_HPP
#define PEEP_DETAIL_FIT_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <ostream>

namespace peep {
namespace detail {

// Given the visible source size (vr, vc), block size, and terminal budget,
// decide output dimensions (out_r, out_c) and whether resampling is needed.
struct FitResolution {
    size_t out_r;
    size_t out_c;
    bool resample;
};

// Terminal space available to the image body after title/colorbar decorators.
// Pixel aspect is still computed from the original terminal geometry.
struct ViewportBudget {
    TerminalSize terminal;
    size_t rows;
    size_t cols;
    bool valid;
};

inline size_t terminal_rows_reserved_for_decorators(const Options& opts) {
    size_t rows = 0;
    if (opts.show_title()) ++rows;
    if (!opts.is_rgb() && opts.show_colorbar()) ++rows;
    return rows;
}

inline ViewportBudget make_viewport_budget(TerminalSize ts, size_t reserved_rows) {
    ViewportBudget b;
    b.terminal = ts;
    b.rows = 0;
    b.cols = 0;
    b.valid = false;

    if (!ts.valid || ts.cols == 0 || ts.rows <= reserved_rows) return b;

    b.rows = ts.rows - reserved_rows;
    b.cols = ts.cols;
    b.valid = true;
    return b;
}

inline ViewportBudget make_viewport_budget(TerminalSize ts, const Options& opts) {
    return make_viewport_budget(ts, terminal_rows_reserved_for_decorators(opts));
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

inline double half_block_pixel_aspect(ViewportBudget budget) {
    return half_block_pixel_aspect(budget.terminal);
}

inline size_t rounded_size(double v) {
    return (v > 1.0) ? static_cast<size_t>(v + 0.5) : 1;
}

inline size_t ceil_div(size_t n, size_t d) {
    return (d == 0) ? 1 : (n + d - 1) / d;
}

inline FitResolution resolve_fit(size_t vr, size_t vc, size_t bs,
                                 Fit requested, ViewportBudget budget) {
    FitResolution r;
    r.out_r = vr;
    r.out_c = vc;
    r.resample = false;
    if (requested == Fit::Off || !budget.valid) return r;

    const size_t max_pcols = budget.cols;
    const size_t max_prows = budget.rows * 2; // 2 pixel rows per half-block cell
    const size_t pcols_needed = vc * bs;
    const size_t prows_needed = vr * bs;
    const double pixel_aspect = half_block_pixel_aspect(budget);
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

inline FitResolution resolve_fit(size_t vr, size_t vc, size_t bs,
                                 Fit requested, TerminalSize ts) {
    return resolve_fit(vr, vc, bs, requested, make_viewport_budget(ts, 0));
}

inline size_t resolve_effective_block_size(size_t vr, size_t vc,
                                           const Options& opts,
                                           ViewportBudget budget) {
    const size_t requested = opts.block_size();
    if (opts.has_block_size() || !budget.valid || vr == 0 || vc == 0) {
        return requested;
    }

    const size_t max_pcols = budget.cols;
    const size_t max_prows = budget.rows * 2;
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

inline size_t resolve_effective_block_size(size_t vr, size_t vc,
                                           const Options& opts,
                                           TerminalSize ts) {
    return resolve_effective_block_size(vr, vc, opts,
                                        make_viewport_budget(ts, opts));
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
    ViewportBudget budget = make_viewport_budget(ts, opts);
    p.block_size = resolve_effective_block_size(p.visible.rows, p.visible.cols,
                                                opts, budget);
    FitResolution fr = resolve_fit(p.visible.rows, p.visible.cols, p.block_size,
                                   opts.fit(), budget);
    p.out_r = fr.out_r;
    p.out_c = fr.out_c;
    p.resample = fr.resample;
    return p;
}

} // namespace detail
} // namespace peep

#endif // PEEP_DETAIL_FIT_HPP
