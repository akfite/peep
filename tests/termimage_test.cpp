#include <termimage/termimage.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace termimage;
namespace tid = termimage::detail;

// ---------------------------------------------------------------------------
// Options
// ---------------------------------------------------------------------------

TEST(Options, DefaultValues) {
    Options opts;
    EXPECT_TRUE(std::isnan(opts.clim_lo()));
    EXPECT_TRUE(std::isnan(opts.clim_hi()));
    EXPECT_EQ(opts.colormap(), Colormap::Gray);
    EXPECT_FALSE(opts.has_nan_color());
    EXPECT_FALSE(opts.has_neg_inf_color());
    EXPECT_FALSE(opts.has_pos_inf_color());
    EXPECT_EQ(opts.block_size(), 1);
    EXPECT_FALSE(opts.has_block_size());
    EXPECT_EQ(opts.layout(), Layout::RowMajor);
    EXPECT_EQ(&opts.ostream(), &std::cout);
    EXPECT_EQ(opts.crop_x(), 0u);
    EXPECT_EQ(opts.crop_y(), 0u);
    EXPECT_EQ(opts.crop_w(), 0u);
    EXPECT_EQ(opts.crop_h(), 0u);
    EXPECT_FALSE(opts.crop_is_centered());
    EXPECT_EQ(opts.crop_center_x(), 0u);
    EXPECT_EQ(opts.crop_center_y(), 0u);
    EXPECT_EQ(opts.fit(), Fit::Resample);
    EXPECT_EQ(opts.resampling(), Resampling::Bilinear);
    EXPECT_FALSE(opts.is_rgb());
    EXPECT_EQ(opts.rgb_layout(), RGBLayout::Interleaved);
    EXPECT_FALSE(opts.has_accessor());
    EXPECT_FALSE(opts.has_rgb_accessor());
    EXPECT_FALSE(opts.show_title());
    EXPECT_EQ(opts.title_text(), "");
    EXPECT_TRUE(opts.show_colorbar());
}

TEST(Options, ChainableSetters) {
    std::ostringstream oss;
    Options opts;
    Options& ref = opts.clim(1.0, 2.0)
                       .colormap("magma")
                       .block_size(3)
                       .layout(Layout::ColMajor)
                       .ostream(oss)
                       .crop(4, 5, 6, 7)
                       .fit(Fit::Trim)
                       .resampling(Resampling::Nearest)
                       .title("demo")
                       .colorbar();

    // Chaining must return the same object
    EXPECT_EQ(&ref, &opts);

    EXPECT_DOUBLE_EQ(opts.clim_lo(), 1.0);
    EXPECT_DOUBLE_EQ(opts.clim_hi(), 2.0);
    EXPECT_EQ(opts.colormap(), Colormap::Magma);
    EXPECT_EQ(opts.block_size(), 3);
    EXPECT_TRUE(opts.has_block_size());
    EXPECT_EQ(opts.layout(), Layout::ColMajor);
    EXPECT_EQ(&opts.ostream(), &oss);
    EXPECT_EQ(opts.crop_x(), 4u);
    EXPECT_EQ(opts.crop_y(), 5u);
    EXPECT_EQ(opts.crop_w(), 6u);
    EXPECT_EQ(opts.crop_h(), 7u);
    EXPECT_FALSE(opts.crop_is_centered());
    EXPECT_EQ(opts.fit(), Fit::Trim);
    EXPECT_EQ(opts.resampling(), Resampling::Nearest);
    EXPECT_TRUE(opts.show_title());
    EXPECT_EQ(opts.title_text(), "demo");
    EXPECT_TRUE(opts.show_colorbar());
    EXPECT_FALSE(opts.colorbar(false).show_colorbar());
}

TEST(Options, RgbAndAccessorSetters) {
    Options opts;
    opts.rgb(RGBLayout::Planar);
    EXPECT_TRUE(opts.is_rgb());
    EXPECT_EQ(opts.rgb_layout(), RGBLayout::Planar);

    opts.accessor([](size_t r, size_t c) {
        return static_cast<double>(r + c);
    });
    EXPECT_FALSE(opts.is_rgb());
    EXPECT_TRUE(opts.has_accessor());
    EXPECT_FALSE(opts.has_rgb_accessor());

    opts.rgb();
    EXPECT_TRUE(opts.is_rgb());
    EXPECT_FALSE(opts.has_accessor());
    EXPECT_FALSE(opts.has_rgb_accessor());

    opts.rgb_accessor([](size_t r, size_t c) {
        return Color{static_cast<std::uint8_t>(r),
                     static_cast<std::uint8_t>(c),
                     0};
    });
    EXPECT_TRUE(opts.is_rgb());
    EXPECT_FALSE(opts.has_accessor());
    EXPECT_TRUE(opts.has_rgb_accessor());

    opts.scalar();
    EXPECT_FALSE(opts.is_rgb());
    EXPECT_FALSE(opts.has_accessor());
    EXPECT_FALSE(opts.has_rgb_accessor());
}

TEST(Options, ClimIndividualSetters) {
    Options opts;
    opts.clim_lo(-5.0).clim_hi(10.0);
    EXPECT_DOUBLE_EQ(opts.clim_lo(), -5.0);
    EXPECT_DOUBLE_EQ(opts.clim_hi(), 10.0);
}

TEST(Options, SpecialValueColorSetters) {
    Options opts;
    opts.nan_color(1, 2, 3)
        .inf_colors(Color{4, 5, 6}, Color{7, 8, 9});

    EXPECT_TRUE(opts.has_nan_color());
    EXPECT_EQ(opts.nan_color(), (Color{1, 2, 3}));
    EXPECT_TRUE(opts.has_neg_inf_color());
    EXPECT_EQ(opts.neg_inf_color(), (Color{4, 5, 6}));
    EXPECT_TRUE(opts.has_pos_inf_color());
    EXPECT_EQ(opts.pos_inf_color(), (Color{7, 8, 9}));

    opts.clear_nan_color().clear_inf_colors();
    EXPECT_FALSE(opts.has_nan_color());
    EXPECT_FALSE(opts.has_neg_inf_color());
    EXPECT_FALSE(opts.has_pos_inf_color());
}

TEST(Options, SharedInfColorSetterSetsBothSigns) {
    Options opts;
    opts.inf_color(10, 20, 30);

    EXPECT_TRUE(opts.has_neg_inf_color());
    EXPECT_TRUE(opts.has_pos_inf_color());
    EXPECT_EQ(opts.neg_inf_color(), (Color{10, 20, 30}));
    EXPECT_EQ(opts.pos_inf_color(), (Color{10, 20, 30}));
}

TEST(Options, CropTwoArgSetsOpenEnd) {
    Options opts;
    opts.center_crop(1, 2, 3, 4);
    opts.crop(3, 7);
    EXPECT_EQ(opts.crop_x(), 3u);
    EXPECT_EQ(opts.crop_y(), 7u);
    EXPECT_EQ(opts.crop_w(), 0u);
    EXPECT_EQ(opts.crop_h(), 0u);
    EXPECT_FALSE(opts.crop_is_centered());
}

TEST(Options, CenterCropSetsCenterAndDimensions) {
    Options opts;
    Options& ref = opts.center_crop(4, 5, 6, 7);

    EXPECT_EQ(&ref, &opts);
    EXPECT_TRUE(opts.crop_is_centered());
    EXPECT_EQ(opts.crop_center_x(), 4u);
    EXPECT_EQ(opts.crop_center_y(), 5u);
    EXPECT_EQ(opts.crop_w(), 6u);
    EXPECT_EQ(opts.crop_h(), 7u);
}

// ---------------------------------------------------------------------------
// find_colormap
// ---------------------------------------------------------------------------

TEST(FindColormap, KnownColormapsByString) {
    EXPECT_EQ(&tid::find_colormap("gray"), &tid::cmap_gray());
    EXPECT_EQ(&tid::find_colormap("grey"), &tid::cmap_gray());
    EXPECT_EQ(&tid::find_colormap("magma"), &tid::CMAP_MAGMA);
    EXPECT_EQ(&tid::find_colormap("viridis"), &tid::CMAP_VIRIDIS);
    EXPECT_EQ(&tid::find_colormap("plasma"), &tid::CMAP_PLASMA);
    EXPECT_EQ(&tid::find_colormap("inferno"), &tid::CMAP_INFERNO);
    EXPECT_EQ(&tid::find_colormap("cividis"), &tid::CMAP_CIVIDIS);
    EXPECT_EQ(&tid::find_colormap("coolwarm"), &tid::CMAP_COOLWARM);
    EXPECT_EQ(&tid::find_colormap("gnuplot"), &tid::CMAP_GNUPLOT);
    EXPECT_EQ(&tid::find_colormap("turbo"), &tid::CMAP_TURBO);
}

TEST(FindColormap, KnownColormapsByEnum) {
    EXPECT_EQ(&tid::find_colormap(Colormap::Gray), &tid::cmap_gray());
    EXPECT_EQ(&tid::find_colormap(Colormap::Magma), &tid::CMAP_MAGMA);
    EXPECT_EQ(&tid::find_colormap(Colormap::Viridis), &tid::CMAP_VIRIDIS);
    EXPECT_EQ(&tid::find_colormap(Colormap::Plasma), &tid::CMAP_PLASMA);
    EXPECT_EQ(&tid::find_colormap(Colormap::Inferno), &tid::CMAP_INFERNO);
    EXPECT_EQ(&tid::find_colormap(Colormap::Cividis), &tid::CMAP_CIVIDIS);
    EXPECT_EQ(&tid::find_colormap(Colormap::Coolwarm), &tid::CMAP_COOLWARM);
    EXPECT_EQ(&tid::find_colormap(Colormap::Gnuplot), &tid::CMAP_GNUPLOT);
    EXPECT_EQ(&tid::find_colormap(Colormap::Turbo), &tid::CMAP_TURBO);
}

TEST(FindColormap, UnknownStringFallsBackToGray) {
    EXPECT_EQ(&tid::find_colormap("nonexistent"), &tid::cmap_gray());
    EXPECT_EQ(&tid::find_colormap(""), &tid::cmap_gray());
}

// ---------------------------------------------------------------------------
// lookup
// ---------------------------------------------------------------------------

TEST(Lookup, BoundsZeroAndOne) {
    // Gray colormap: index i -> (i, i, i)
    tid::RGB lo = tid::lookup(0.0, tid::cmap_gray());
    EXPECT_EQ(lo.r, 0);
    EXPECT_EQ(lo.g, 0);
    EXPECT_EQ(lo.b, 0);

    tid::RGB hi = tid::lookup(1.0, tid::cmap_gray());
    EXPECT_EQ(hi.r, 255);
    EXPECT_EQ(hi.g, 255);
    EXPECT_EQ(hi.b, 255);
}

TEST(Lookup, MidpointGray) {
    // 0.5 * 255 + 0.5 = 128.0 -> index 128
    tid::RGB mid = tid::lookup(0.5, tid::cmap_gray());
    EXPECT_EQ(mid.r, 128);
    EXPECT_EQ(mid.r, mid.g);
    EXPECT_EQ(mid.g, mid.b);
}

TEST(Lookup, ClampsBelowZero) {
    tid::RGB c = tid::lookup(-1.0, tid::cmap_gray());
    EXPECT_EQ(c.r, 0);
    EXPECT_EQ(c.g, 0);
    EXPECT_EQ(c.b, 0);
}

TEST(Lookup, ClampsAboveOne) {
    tid::RGB c = tid::lookup(2.0, tid::cmap_gray());
    EXPECT_EQ(c.r, 255);
    EXPECT_EQ(c.g, 255);
    EXPECT_EQ(c.b, 255);
}

TEST(Lookup, MagmaEndpoints) {
    // Magma index 0: (0x00, 0x00, 0x04)
    tid::RGB lo = tid::lookup(0.0, tid::CMAP_MAGMA);
    EXPECT_EQ(lo.r, 0x00);
    EXPECT_EQ(lo.g, 0x00);
    EXPECT_EQ(lo.b, 0x04);

    // Magma index 255 from matplotlib's magma colormap.
    tid::RGB hi = tid::lookup(1.0, tid::CMAP_MAGMA);
    EXPECT_EQ(hi.r, 0xfc);
    EXPECT_EQ(hi.g, 0xfd);
    EXPECT_EQ(hi.b, 0xbf);
}

// ---------------------------------------------------------------------------
// emit helpers
// ---------------------------------------------------------------------------

TEST(Emit, BackgroundEscape) {
    std::ostringstream oss;
    tid::RGB c = {10, 20, 30};
    tid::emit_bg(oss, c);
    EXPECT_EQ(oss.str(), "\x1b[48;2;10;20;30m");
}

TEST(Emit, ForegroundEscape) {
    std::ostringstream oss;
    tid::RGB c = {255, 128, 0};
    tid::emit_fg(oss, c);
    EXPECT_EQ(oss.str(), "\x1b[38;2;255;128;0m");
}

TEST(Emit, Reset) {
    std::ostringstream oss;
    tid::emit_reset(oss);
    EXPECT_EQ(oss.str(), "\x1b[0m");
}

TEST(Emit, LowerHalfBlock) {
    std::ostringstream oss;
    tid::emit_lower_half(oss);
    EXPECT_EQ(oss.str(), "\xe2\x96\x84");
}

TEST(Emit, UpperHalfBlock) {
    std::ostringstream oss;
    tid::emit_upper_half(oss);
    EXPECT_EQ(oss.str(), "\xe2\x96\x80");
}

// ---------------------------------------------------------------------------
// Helper: capture render output
// ---------------------------------------------------------------------------

static std::string render_to_string(const double* data, size_t rows, size_t cols,
                                     const Options& base = Options()) {
    std::ostringstream oss;
    Options opts = base;
    opts.ostream(oss).colorbar(false);
    termimage::print(data, rows, cols, opts);
    return oss.str();
}

static std::string capture_rgb_output(const std::uint8_t* data, size_t rows, size_t cols,
                                      RGBLayout rgb_layout = RGBLayout::Interleaved,
                                      const Options& base = Options()) {
    std::ostringstream oss;
    Options opts = base;
    opts.ostream(oss).rgb(rgb_layout);
    termimage::print(data, rows, cols, opts);
    return oss.str();
}

// ---------------------------------------------------------------------------
// Render – edge cases
// ---------------------------------------------------------------------------

TEST(Render, ZeroRowsProducesNothing) {
    double data[] = {1.0};
    EXPECT_EQ(render_to_string(data, 0, 1), "");
}

TEST(Render, ZeroColsProducesNothing) {
    double data[] = {1.0};
    EXPECT_EQ(render_to_string(data, 1, 0), "");
}

// ---------------------------------------------------------------------------
// Render – basic output structure
// ---------------------------------------------------------------------------

TEST(Render, SinglePixelProducesOneRow) {
    double data[] = {0.5};
    std::string out = render_to_string(data, 1, 1);
    // Should contain exactly one newline (one rendered row)
    int newlines = 0;
    for (char ch : out) if (ch == '\n') newlines++;
    EXPECT_EQ(newlines, 1);
}

TEST(Render, RowCountMatchesHalfPixelRows) {
    // 4 matrix rows, block_size=1 -> 4 pixel rows -> ceil(4/2) = 2 rendered lines
    double data[] = {0, 1, 2, 3, 4, 5, 6, 7};
    std::string out = render_to_string(data, 4, 2);
    int newlines = 0;
    for (char ch : out) if (ch == '\n') newlines++;
    EXPECT_EQ(newlines, 2);
}

TEST(Render, OddRowCountPadsCorrectly) {
    // 3 matrix rows, block_size=1 -> 3 pixel rows -> ceil(3/2) = 2 rendered lines
    double data[] = {0, 1, 2, 3, 4, 5};
    std::string out = render_to_string(data, 3, 2);
    int newlines = 0;
    for (char ch : out) if (ch == '\n') newlines++;
    EXPECT_EQ(newlines, 2);
}

// ---------------------------------------------------------------------------
// Render – uniform values
// ---------------------------------------------------------------------------

TEST(Render, UniformValueProducesValidOutput) {
    // All values the same => range collapses to 1.0, normalized = 0
    double data[] = {5.0, 5.0, 5.0, 5.0};
    std::string out = render_to_string(data, 2, 2);
    EXPECT_FALSE(out.empty());
    // Should contain escape sequences and a newline
    EXPECT_NE(out.find("\x1b["), std::string::npos);
}

// ---------------------------------------------------------------------------
// Render – NaN handling
// ---------------------------------------------------------------------------

TEST(Render, AllNaNProducesSpaces) {
    double nan = std::numeric_limits<double>::quiet_NaN();
    double data[] = {nan, nan, nan, nan};
    std::string out = render_to_string(data, 2, 2);
    // Each NaN cell should produce a space character
    int spaces = 0;
    for (char ch : out) if (ch == ' ') spaces++;
    // 2 cols per rendered row, 1 rendered row (2 pixel rows packed)
    EXPECT_GE(spaces, 2);
}

TEST(Render, MixedNaNUsesHalfBlocks) {
    double nan = std::numeric_limits<double>::quiet_NaN();
    // Top row NaN, bottom row value -> should use lower-half block
    double data[] = {nan, nan, 0.5, 0.5};
    std::string out = render_to_string(data, 2, 2);
    // Lower half block U+2584 = "\xe2\x96\x84"
    EXPECT_NE(out.find("\xe2\x96\x84"), std::string::npos);
}

TEST(Render, TopValueBottomNaNUsesUpperHalfBlock) {
    double nan = std::numeric_limits<double>::quiet_NaN();
    // Top row value, bottom row NaN -> should use upper-half block
    double data[] = {0.5, 0.5, nan, nan};
    std::string out = render_to_string(data, 2, 2);
    // Upper half block U+2580 = "\xe2\x96\x80"
    EXPECT_NE(out.find("\xe2\x96\x80"), std::string::npos);
}

TEST(Render, ConfiguredNaNColorRendersAsColoredPixels) {
    double nan = std::numeric_limits<double>::quiet_NaN();
    double data[] = {nan, nan, nan, nan};
    std::string out = render_to_string(data, 2, 2,
        Options().nan_color(9, 8, 7));

    EXPECT_NE(out.find("9;8;7"), std::string::npos);
    EXPECT_NE(out.find("\xe2\x96\x84"), std::string::npos);
}

// ---------------------------------------------------------------------------
// Render – Infinity handling
// ---------------------------------------------------------------------------

TEST(Render, InfNormalizesToBoundary) {
    double inf = std::numeric_limits<double>::infinity();
    // With clim(0,1), +inf should map to 1.0 and -inf to 0.0
    double data[] = {-inf, inf};
    Options opts;
    opts.clim(0.0, 1.0);
    std::string out = render_to_string(data, 1, 2, opts);
    EXPECT_FALSE(out.empty());
    EXPECT_NE(out.find("\x1b["), std::string::npos);
}

TEST(Render, AllInfFallsBackToDefaultRange) {
    double inf = std::numeric_limits<double>::infinity();
    double data[] = {inf, inf, inf, inf};
    std::string out = render_to_string(data, 2, 2);
    // auto-clim on all-inf data falls back to lo=0, hi=1
    EXPECT_FALSE(out.empty());
}

// ---------------------------------------------------------------------------
// Render – manual clim
// ---------------------------------------------------------------------------

TEST(Render, ManualClimClampsOutput) {
    // Values outside clim should be clamped
    double data[] = {-10.0, 0.5, 20.0, 0.5};
    Options opts;
    opts.clim(0.0, 1.0).colormap("gray");
    std::string out = render_to_string(data, 2, 2, opts);
    EXPECT_FALSE(out.empty());
}

TEST(Render, PartialAutoClimLo) {
    // Only set clim_hi, let lo be auto-computed
    double data[] = {2.0, 4.0, 6.0, 8.0};
    Options opts;
    opts.clim_hi(10.0);
    std::string out = render_to_string(data, 2, 2, opts);
    EXPECT_FALSE(out.empty());
}

TEST(Render, PartialAutoClimHi) {
    // Only set clim_lo, let hi be auto-computed
    double data[] = {2.0, 4.0, 6.0, 8.0};
    Options opts;
    opts.clim_lo(0.0);
    std::string out = render_to_string(data, 2, 2, opts);
    EXPECT_FALSE(out.empty());
}

// ---------------------------------------------------------------------------
// Render – column-major layout
// ---------------------------------------------------------------------------

TEST(Render, ColMajorDifferentFromRowMajor) {
    // Non-symmetric data: output should differ between layouts
    double data[] = {0.0, 1.0, 0.5, 0.25};
    std::string row_out = render_to_string(data, 2, 2, Options().layout(Layout::RowMajor));
    std::string col_out = render_to_string(data, 2, 2, Options().layout(Layout::ColMajor));
    // For non-symmetric data the outputs should differ
    EXPECT_NE(row_out, col_out);
}

TEST(Render, ColMajorSymmetricMatchesRowMajor) {
    // Symmetric 2x2 matrix: row-major and col-major should be identical
    //   | a  b |      col-major stores [a, c, b, d]
    //   | c  d |      but if a=d and b=c, both layouts yield the same image
    double data[] = {0.0, 1.0, 1.0, 0.0};
    // Row-major: row0=[0,1] row1=[1,0]
    // Col-major: col0=[0,1] col1=[1,0] => row0=[0,1] row1=[1,0]  same!
    std::string row_out = render_to_string(data, 2, 2, Options().layout(Layout::RowMajor));
    std::string col_out = render_to_string(data, 2, 2, Options().layout(Layout::ColMajor));
    EXPECT_EQ(row_out, col_out);
}

// ---------------------------------------------------------------------------
// Render – block_size
// ---------------------------------------------------------------------------

TEST(Render, BlockSizeScalesOutputColumns) {
    double data[] = {0.0, 1.0};
    // block_size=1: 2 pixel cols
    std::string bs1 = render_to_string(data, 1, 2, Options().block_size(1));
    // block_size=3: 6 pixel cols -> 3x as many color runs
    std::string bs3 = render_to_string(data, 1, 2, Options().block_size(3));
    EXPECT_GT(bs3.size(), bs1.size());
}

TEST(Render, BlockSizeScalesOutputRows) {
    double data[] = {0.0, 0.5, 1.0, 0.25};
    // block_size=1: 2 pixel rows -> 1 rendered line
    std::string bs1 = render_to_string(data, 2, 2, Options().block_size(1));
    int nl1 = 0;
    for (char ch : bs1) if (ch == '\n') nl1++;
    // block_size=2: 4 pixel rows -> 2 rendered lines
    std::string bs2 = render_to_string(data, 2, 2, Options().block_size(2));
    int nl2 = 0;
    for (char ch : bs2) if (ch == '\n') nl2++;
    EXPECT_EQ(nl1, 1);
    EXPECT_EQ(nl2, 2);
}

TEST(Render, BlockSizeZeroTreatedAsOne) {
    double data[] = {0.0, 1.0, 0.5, 0.25};
    std::string bs0 = render_to_string(data, 2, 2, Options().block_size(0));
    std::string bs1 = render_to_string(data, 2, 2, Options().block_size(1));
    EXPECT_EQ(bs0, bs1);
}

// ---------------------------------------------------------------------------
// Render – crop
// ---------------------------------------------------------------------------

TEST(Render, CropReducesOutput) {
    // 4x4 matrix, crop to 2x2 subregion
    double data[16];
    for (int i = 0; i < 16; i++) data[i] = static_cast<double>(i);
    std::string full = render_to_string(data, 4, 4);
    std::string cropped = render_to_string(data, 4, 4, Options().crop(1, 1, 2, 2));
    EXPECT_GT(full.size(), cropped.size());
}

TEST(Render, CropUsesXywhOrder) {
    double data[20];
    for (int i = 0; i < 20; i++) data[i] = static_cast<double>(i);

    double expected[] = {7.0, 8.0, 12.0, 13.0};
    std::string cropped = render_to_string(data, 4, 5, Options().crop(2, 1, 2, 2));
    std::string direct = render_to_string(expected, 2, 2);

    EXPECT_EQ(cropped, direct);
}

TEST(Render, CropOutOfBoundsPadsWithNaN) {
    double data[] = {1.0, 2.0, 3.0, 4.0};
    double nan = std::numeric_limits<double>::quiet_NaN();
    double expected[] = {nan};

    std::string out = render_to_string(data, 2, 2, Options().crop(10, 10, 1, 1));
    std::string direct = render_to_string(expected, 1, 1);

    EXPECT_EQ(out, direct);
}

TEST(Render, CropPadsPastMatrixEdge) {
    double nan = std::numeric_limits<double>::quiet_NaN();
    double data[4] = {0.0, 1.0, 0.5, 0.75};
    double expected[] = {
        0.0, 1.0, nan, nan,
        0.5, 0.75, nan, nan,
        nan, nan, nan, nan
    };

    std::string out = render_to_string(data, 2, 2, Options().crop(0, 0, 4, 3));
    std::string direct = render_to_string(expected, 3, 4);

    EXPECT_EQ(out, direct);
}

TEST(Render, CropTwoArgShowsFromOriginToEnd) {
    double data[16];
    for (int i = 0; i < 16; i++) data[i] = static_cast<double>(i);
    // crop(1,1) with h=0,w=0 should show from (1,1) to end
    std::string out = render_to_string(data, 4, 4, Options().crop(1, 1));
    EXPECT_FALSE(out.empty());
    // Should be smaller than full output (3x3 vs 4x4)
    std::string full = render_to_string(data, 4, 4);
    EXPECT_GT(full.size(), out.size());
}

TEST(Render, CenterCropMatchesEquivalentExplicitCrop) {
    double data[30];
    for (int i = 0; i < 30; i++) data[i] = static_cast<double>(i);

    std::string centered = render_to_string(data, 5, 6,
        Options().center_crop(2, 3, 3, 4));
    std::string explicit_crop = render_to_string(data, 5, 6,
        Options().crop(1, 1, 3, 4));

    EXPECT_EQ(centered, explicit_crop);
}

TEST(Render, CenterCropPadsAtMatrixEdge) {
    double nan = std::numeric_limits<double>::quiet_NaN();
    double data[25];
    for (int i = 0; i < 25; i++) data[i] = static_cast<double>(i);
    double expected[] = {
        nan, nan, nan,
        nan, 0.0, 1.0,
        nan, 5.0, 6.0
    };

    std::string centered = render_to_string(data, 5, 5,
        Options().center_crop(0, 0, 3, 3));
    std::string direct = render_to_string(expected, 3, 3);

    EXPECT_EQ(centered, direct);
}

TEST(Render, CenterCropOutOfBoundsPadsWithNaN) {
    double nan = std::numeric_limits<double>::quiet_NaN();
    double data[] = {1.0, 2.0, 3.0, 4.0};
    double expected[] = {
        nan, nan, nan,
        nan, nan, nan,
        nan, nan, nan
    };

    std::string out = render_to_string(data, 2, 2,
        Options().center_crop(10, 10, 3, 3));
    std::string direct = render_to_string(expected, 3, 3);

    EXPECT_EQ(out, direct);
}

TEST(Render, CenterCropWorksForRgbInput) {
    std::uint8_t data[4 * 4 * 3];
    for (int i = 0; i < 4 * 4 * 3; i++) data[i] = static_cast<std::uint8_t>(i);

    std::string centered = capture_rgb_output(data, 4, 4, RGBLayout::Interleaved,
        Options().center_crop(2, 2, 2, 2));
    std::string explicit_crop = capture_rgb_output(data, 4, 4, RGBLayout::Interleaved,
        Options().crop(1, 1, 2, 2));

    EXPECT_EQ(centered, explicit_crop);
}

TEST(Render, CropPadsRgbInputWithBlack) {
    const std::uint8_t data[] = {255, 0, 0};
    const std::uint8_t expected[] = {
        255, 0, 0,
        0, 0, 0
    };

    std::string cropped = capture_rgb_output(data, 1, 1, RGBLayout::Interleaved,
        Options().crop(0, 0, 2, 1));
    std::string direct = capture_rgb_output(expected, 1, 2);

    EXPECT_EQ(cropped, direct);
}

// ---------------------------------------------------------------------------
// Render – integer data type
// ---------------------------------------------------------------------------

TEST(Render, IntegerDataType) {
    int data[] = {0, 64, 128, 255};
    std::ostringstream oss;
    termimage::print(data, 2, 2, Options().ostream(oss));
    std::string out = oss.str();
    EXPECT_FALSE(out.empty());
    EXPECT_NE(out.find("\x1b["), std::string::npos);
}

TEST(Render, ConfiguredInfColorsOverrideColormapEndpoints) {
    double inf = std::numeric_limits<double>::infinity();
    double data[] = {-inf, inf};
    std::string out = render_to_string(data, 1, 2,
        Options().clim(0.0, 1.0)
                 .inf_colors(Color{1, 2, 3}, Color{4, 5, 6}));

    EXPECT_NE(out.find("1;2;3"), std::string::npos);
    EXPECT_NE(out.find("4;5;6"), std::string::npos);
}

TEST(Render, FloatDataType) {
    float data[] = {0.0f, 0.25f, 0.75f, 1.0f};
    std::ostringstream oss;
    termimage::print(data, 2, 2, Options().ostream(oss));
    std::string out = oss.str();
    EXPECT_FALSE(out.empty());
}

// ---------------------------------------------------------------------------
// RGB render
// ---------------------------------------------------------------------------

TEST(RenderRgb, InterleavedUsesRgbTripletsDirectly) {
    const std::uint8_t data[] = {
        255, 0, 0,
        0, 0, 255
    };

    std::string out = capture_rgb_output(data, 2, 1);

    EXPECT_EQ(out,
        "\x1b[48;2;255;0;0m"
        "\x1b[38;2;0;0;255m"
        "\xe2\x96\x84"
        "\x1b[0m\n");
}

TEST(RenderRgb, PlanarUsesSeparateColorPlanes) {
    const std::uint8_t data[] = {
        255, 0,   // R plane
        0, 0,     // G plane
        0, 255    // B plane
    };

    EXPECT_EQ(capture_rgb_output(data, 2, 1, RGBLayout::Planar),
        "\x1b[48;2;255;0;0m"
        "\x1b[38;2;0;0;255m"
        "\xe2\x96\x84"
        "\x1b[0m\n");
}

TEST(RenderRgb, OddRowsUseUpperHalfBlock) {
    const std::uint8_t data[] = {9, 8, 7};

    std::string out = capture_rgb_output(data, 1, 1);

    EXPECT_EQ(out,
        "\x1b[38;2;9;8;7m"
        "\xe2\x96\x80"
        "\x1b[0m\n");
}

TEST(RenderRgb, HonorsColumnMajorSpatialLayout) {
    const std::uint8_t data[] = {
        255, 0, 0,
        0, 255, 0,
        0, 0, 255,
        255, 255, 255
    };

    std::string row_out = capture_rgb_output(data, 2, 2, RGBLayout::Interleaved,
        Options().layout(Layout::RowMajor));
    std::string col_out = capture_rgb_output(data, 2, 2, RGBLayout::Interleaved,
        Options().layout(Layout::ColMajor));

    EXPECT_NE(row_out, col_out);
}

TEST(RenderRgb, VectorInputMatchesPointerInput) {
    std::vector<std::uint8_t> data = {
        255, 0, 0,
        0, 0, 255
    };
    std::ostringstream oss;
    termimage::print(data, 2, 1, Options().ostream(oss).rgb());

    EXPECT_EQ(oss.str(), capture_rgb_output(data.data(), 2, 1));
}

TEST(RenderRgb, VectorInputRejectsMismatchedDimensions) {
    std::vector<std::uint8_t> data(5);
    EXPECT_THROW(termimage::print(data, 2, 1, Options().rgb()), std::invalid_argument);
}

TEST(RenderAccessor, ScalarAccessorMatchesPointerInput) {
    std::vector<double> data = {0.0, 1.0, 2.0, 3.0};

    Options opts;
    opts.colormap("gray")
        .accessor([&](size_t r, size_t c) {
            return data[r * 2 + c];
        });

    EXPECT_EQ(to_string(2, 2, opts),
              to_string(data, 2, 2, Options().colormap("gray")));
}

TEST(RenderAccessor, RgbAccessorMatchesRgbPointerInput) {
    const std::uint8_t rgb[] = {
        255, 0, 0,
        0, 0, 255
    };

    Options opts;
    opts.rgb_accessor([&](size_t r, size_t) {
        const size_t i = r * 3;
        return Color{rgb[i + 0], rgb[i + 1], rgb[i + 2]};
    });

    EXPECT_EQ(to_string(2, 1, opts),
              to_string(rgb, 2, 1, Options().rgb()));
}

TEST(RenderAccessor, MissingAccessorThrows) {
    EXPECT_THROW(print(2, 2, Options()), std::invalid_argument);
    EXPECT_THROW(print(2, 2, Options().rgb()), std::invalid_argument);
    EXPECT_THROW(to_string(2, 2, Options()), std::invalid_argument);
}

TEST(RenderAccessor, DataInputRejectsAccessorOptions) {
    double scalar_data[] = {1.0};
    std::vector<double> scalar_vec = {1.0};
    const std::uint8_t rgb_data[] = {255, 0, 0};

    auto scalar = [](size_t, size_t) {
        return 1.0;
    };
    auto rgb = [](size_t, size_t) {
        return Color{255, 0, 0};
    };

    EXPECT_THROW(print(scalar_data, 1, 1, Options().accessor(scalar)), std::invalid_argument);
    EXPECT_THROW(to_string(scalar_vec, 1, 1, Options().accessor(scalar)), std::invalid_argument);
    EXPECT_THROW(print(rgb_data, 1, 1, Options().rgb_accessor(rgb)), std::invalid_argument);
}

// ---------------------------------------------------------------------------
// Render – colormap selection
// ---------------------------------------------------------------------------

TEST(Render, DifferentColormapsProduceDifferentOutput) {
    double data[] = {0.0, 0.5, 1.0, 0.25};
    std::string gray_out = render_to_string(data, 2, 2, Options().colormap("gray"));
    std::string magma_out = render_to_string(data, 2, 2, Options().colormap("magma"));
    std::string viridis_out = render_to_string(data, 2, 2, Options().colormap("viridis"));
    EXPECT_NE(gray_out, magma_out);
    EXPECT_NE(gray_out, viridis_out);
    EXPECT_NE(magma_out, viridis_out);
}

TEST(Render, EnumColormapMatchesStringColormap) {
    double data[] = {0.0, 0.5, 1.0, 0.25};
    EXPECT_EQ(render_to_string(data, 2, 2, Options().colormap("gray")),
        render_to_string(data, 2, 2, Options().colormap(Colormap::Gray)));
    EXPECT_EQ(render_to_string(data, 2, 2, Options().colormap("magma")),
        render_to_string(data, 2, 2, Options().colormap(Colormap::Magma)));
    EXPECT_EQ(render_to_string(data, 2, 2, Options().colormap("viridis")),
        render_to_string(data, 2, 2, Options().colormap(Colormap::Viridis)));
    EXPECT_EQ(render_to_string(data, 2, 2, Options().colormap("plasma")),
        render_to_string(data, 2, 2, Options().colormap(Colormap::Plasma)));
    EXPECT_EQ(render_to_string(data, 2, 2, Options().colormap("inferno")),
        render_to_string(data, 2, 2, Options().colormap(Colormap::Inferno)));
    EXPECT_EQ(render_to_string(data, 2, 2, Options().colormap("cividis")),
        render_to_string(data, 2, 2, Options().colormap(Colormap::Cividis)));
    EXPECT_EQ(render_to_string(data, 2, 2, Options().colormap("coolwarm")),
        render_to_string(data, 2, 2, Options().colormap(Colormap::Coolwarm)));
    EXPECT_EQ(render_to_string(data, 2, 2, Options().colormap("gnuplot")),
        render_to_string(data, 2, 2, Options().colormap(Colormap::Gnuplot)));
    EXPECT_EQ(render_to_string(data, 2, 2, Options().colormap("turbo")),
        render_to_string(data, 2, 2, Options().colormap(Colormap::Turbo)));
}

TEST(Render, UnknownColormapUsesGray) {
    double data[] = {0.0, 0.5, 1.0, 0.25};
    std::string gray_out = render_to_string(data, 2, 2, Options().colormap(Colormap::Gray));
    std::string unknown_out = render_to_string(data, 2, 2, Options().colormap("doesnotexist"));
    EXPECT_EQ(gray_out, unknown_out);
}

// ---------------------------------------------------------------------------
// Render – determinism
// ---------------------------------------------------------------------------

TEST(Render, SameInputProducesSameOutput) {
    double data[] = {0.0, 0.25, 0.5, 0.75, 1.0, 0.1};
    std::string a = render_to_string(data, 2, 3);
    std::string b = render_to_string(data, 2, 3);
    EXPECT_EQ(a, b);
}

// ---------------------------------------------------------------------------
// Render – reset always present at end of each line
// ---------------------------------------------------------------------------

TEST(Render, EachLineEndsWithResetThenNewline) {
    double data[] = {0.0, 1.0, 0.5, 0.25};
    std::string out = render_to_string(data, 2, 2);
    std::string reset = "\x1b[0m";
    // Every line should end with \x1b[0m\n
    std::string suffix = reset + "\n";

    size_t pos = 0;
    int lines = 0;
    while ((pos = out.find('\n', pos)) != std::string::npos) {
        // Check that the reset precedes this newline
        ASSERT_GE(pos, suffix.size() - 1);
        std::string ending = out.substr(pos - reset.size(), reset.size() + 1);
        EXPECT_EQ(ending, suffix) << "Line " << lines << " missing trailing reset";
        pos++;
        lines++;
    }
    EXPECT_GT(lines, 0);
}

// ---------------------------------------------------------------------------
// Render – large matrix smoke test
// ---------------------------------------------------------------------------

TEST(Render, LargeMatrixDoesNotCrash) {
    const int rows = 200, cols = 300;
    std::vector<double> data(rows * cols);
    for (int i = 0; i < rows * cols; i++)
        data[i] = std::sin(static_cast<double>(i) * 0.01);
    std::string out = render_to_string(data.data(), rows, cols,
        Options().colormap("viridis").block_size(1));
    EXPECT_FALSE(out.empty());
    int newlines = 0;
    for (char ch : out) if (ch == '\n') newlines++;
    EXPECT_EQ(newlines, (rows + 1) / 2);  // ceil(200/2) = 100
}

// ---------------------------------------------------------------------------
// Computed gray colormap correctness
// ---------------------------------------------------------------------------

TEST(CmapGray, AllEntriesCorrect) {
    const ColormapLut& g = tid::cmap_gray();
    for (int i = 0; i < 256; ++i) {
        EXPECT_EQ(g[i * 3 + 0], static_cast<std::uint8_t>(i));
        EXPECT_EQ(g[i * 3 + 1], static_cast<std::uint8_t>(i));
        EXPECT_EQ(g[i * 3 + 2], static_cast<std::uint8_t>(i));
    }
}

TEST(CmapGray, StablePointer) {
    // Repeated calls should return the same singleton object.
    EXPECT_EQ(&tid::cmap_gray(), &tid::cmap_gray());
}

// ---------------------------------------------------------------------------
// Custom colormap
// ---------------------------------------------------------------------------

TEST(CustomColormap, OverridesEnumColormap) {
    // Build a trivial all-red LUT
    ColormapLut red_lut;
    for (int i = 0; i < 256; ++i) {
        red_lut[i * 3 + 0] = 255;
        red_lut[i * 3 + 1] = 0;
        red_lut[i * 3 + 2] = 0;
    }

    double data[] = {0.0, 0.5, 1.0, 0.25};
    std::string custom_out = render_to_string(data, 2, 2,
        Options().colormap(red_lut));
    std::string viridis_out = render_to_string(data, 2, 2,
        Options().colormap(Colormap::Viridis));
    EXPECT_NE(custom_out, viridis_out);
    // Custom red LUT should produce "255;0;0" in escape sequences
    EXPECT_NE(custom_out.find("255;0;0"), std::string::npos);
}

TEST(CustomColormap, EnumSetterClearsCustom) {
    ColormapLut lut = {};
    Options opts;
    opts.colormap(lut);
    EXPECT_NE(opts.custom_colormap(), nullptr);
    EXPECT_TRUE(opts.has_custom_colormap());
    opts.colormap(Colormap::Viridis);
    EXPECT_EQ(opts.custom_colormap(), nullptr);
    EXPECT_FALSE(opts.has_custom_colormap());
}

TEST(CustomColormap, StringSetterClearsCustom) {
    ColormapLut lut = {};
    Options opts;
    opts.colormap(lut);
    EXPECT_NE(opts.custom_colormap(), nullptr);
    opts.colormap("magma");
    EXPECT_EQ(opts.custom_colormap(), nullptr);
}

TEST(CustomColormap, DefaultIsNull) {
    Options opts;
    EXPECT_EQ(opts.custom_colormap(), nullptr);
    EXPECT_FALSE(opts.has_custom_colormap());
}

TEST(CustomColormap, CopiesArrayInput) {
    ColormapLut lut = {};
    lut[0] = 1;

    Options opts;
    opts.colormap(lut);
    lut[0] = 99;

    ASSERT_NE(opts.custom_colormap(), nullptr);
    EXPECT_EQ((*opts.custom_colormap())[0], 1);
}

TEST(CustomColormap, AcceptsVectorInput) {
    std::vector<std::uint8_t> lut(768, 0);
    lut[0] = 10;
    lut[1] = 20;
    lut[2] = 30;

    Options opts;
    opts.colormap(lut);

    ASSERT_NE(opts.custom_colormap(), nullptr);
    EXPECT_EQ((*opts.custom_colormap())[0], 10);
    EXPECT_EQ((*opts.custom_colormap())[1], 20);
    EXPECT_EQ((*opts.custom_colormap())[2], 30);
}

TEST(CustomColormap, RejectsWrongSizedVectorInput) {
    Options opts;
    std::vector<std::uint8_t> lut(767, 0);
    EXPECT_THROW(opts.colormap(lut), std::invalid_argument);
}

// ---------------------------------------------------------------------------
// Global default colormap
// ---------------------------------------------------------------------------

TEST(DefaultColormap, InitiallyGray) {
    EXPECT_EQ(default_colormap(), Colormap::Gray);
}

TEST(DefaultColormap, SetAndRestore) {
    Colormap original = default_colormap();
    set_default_colormap(Colormap::Magma);
    EXPECT_EQ(default_colormap(), Colormap::Magma);

    // New Options should pick up the global default
    Options opts;
    EXPECT_EQ(opts.colormap(), Colormap::Magma);

    // Restore
    set_default_colormap(original);
    EXPECT_EQ(default_colormap(), original);
}

TEST(DefaultColormap, OptionsOverrideStillWorks) {
    set_default_colormap(Colormap::Viridis);
    Options opts;
    opts.colormap(Colormap::Gray);
    EXPECT_EQ(opts.colormap(), Colormap::Gray);
    // Restore
    set_default_colormap(Colormap::Gray);
}

// ---------------------------------------------------------------------------
// Fit — terminal auto-sizing
// ---------------------------------------------------------------------------

namespace {
tid::TerminalSize ts(size_t rows, size_t cols, bool valid = true,
                size_t width_px = 0, size_t height_px = 0) {
    tid::TerminalSize t;
    t.rows = rows;
    t.cols = cols;
    t.width_px = width_px;
    t.height_px = height_px;
    t.pixels_valid = width_px > 0 && height_px > 0;
    t.valid = valid;
    return t;
}
} // namespace

TEST(Fit, InvalidTerminalIsNoop) {
    // With no usable terminal size, all modes are identity.
    for (Fit m : {Fit::Off, Fit::Resample, Fit::Trim}) {
        tid::FitResolution r = tid::resolve_fit(100, 200, 1, m, ts(0, 0, false));
        EXPECT_EQ(r.out_r, 100u);
        EXPECT_EQ(r.out_c, 200u);
        EXPECT_FALSE(r.resample);
    }
}

TEST(Fit, OffIgnoresTerminalSize) {
    // Even with a too-small terminal, Off leaves dimensions alone.
    tid::FitResolution r = tid::resolve_fit(100, 200, 1, Fit::Off, ts(10, 20));
    EXPECT_EQ(r.out_r, 100u);
    EXPECT_EQ(r.out_c, 200u);
    EXPECT_FALSE(r.resample);
}

TEST(Fit, FitsExactlyIsNoop) {
    // vr*bs = 20 pixel rows = 10 cells, vc*bs = 40 cells → fits exactly.
    tid::FitResolution r = tid::resolve_fit(20, 40, 1, Fit::Resample, ts(10, 40));
    EXPECT_EQ(r.out_r, 20u);
    EXPECT_EQ(r.out_c, 40u);
    EXPECT_FALSE(r.resample);
}

TEST(Fit, ResampleMatchingAspectScalesUniformly) {
    // Source 100×200 (vc:vr = 2:1), terminal pixel cap 40×20 (also 2:1).
    // Uniform scale of 1/5 on both axes → out_c=40, out_r=20.
    tid::FitResolution r = tid::resolve_fit(100, 200, 1, Fit::Resample, ts(10, 40));
    EXPECT_EQ(r.out_r, 20u);
    EXPECT_EQ(r.out_c, 40u);
    EXPECT_TRUE(r.resample);
}

TEST(Fit, ResamplePreservesAspectWhenOnlyWidthOverruns) {
    // Source 10×200 (vc:vr = 20:1). Height alone fits (10 ≤ 2*50=100), but
    // width overruns (200 > 40). Aspect-preserving resample shrinks BOTH
    // axes by the width scale factor (40/200 = 1/5) → out_c=40, out_r=2.
    // Without the aspect fix, the old behavior would leave out_r=10 and
    // stretch the image vertically by 5x.
    tid::FitResolution r = tid::resolve_fit(10, 200, 1, Fit::Resample, ts(50, 40));
    EXPECT_EQ(r.out_c, 40u);
    EXPECT_EQ(r.out_r, 2u);
    EXPECT_EQ(r.out_c / r.out_r, 20u);  // aspect preserved
    EXPECT_TRUE(r.resample);
}

TEST(Fit, ResamplePreservesAspectWhenOnlyHeightOverruns) {
    // Mirror case: source 200×10 (tall & narrow, vc:vr = 1:20). Width fits
    // (10 ≤ 40), height overruns (200 > 2*10=20). Height scale = 20/200 = 1/10;
    // applied to both → out_r=20, out_c=1.
    tid::FitResolution r = tid::resolve_fit(200, 10, 1, Fit::Resample, ts(10, 40));
    EXPECT_EQ(r.out_r, 20u);
    EXPECT_EQ(r.out_c, 1u);
    EXPECT_EQ(r.out_r / r.out_c, 20u);  // aspect preserved
    EXPECT_TRUE(r.resample);
}

TEST(Fit, ResamplePreservesSquareAspect) {
    // Square source (100×100). Terminal 20 cells × 20 cells → 20 pixel cols,
    // 40 pixel rows. Width is tighter. Uniform scale → out_c=20, out_r=20.
    tid::FitResolution r = tid::resolve_fit(100, 100, 1, Fit::Resample, ts(20, 20));
    EXPECT_EQ(r.out_c, 20u);
    EXPECT_EQ(r.out_r, 20u);  // square stays square
    EXPECT_TRUE(r.resample);
}

TEST(Fit, ResampleUsesDetectedTerminalPixelAspect) {
    // 100 cols in 1000px -> 10px-wide cells.
    // 100 rows in 2200px -> 22px-tall cells.
    // Half-block pixels are therefore 11/10 as tall as they are wide, so a
    // square source should render with fewer output rows.
    tid::FitResolution r = tid::resolve_fit(100, 100, 1, Fit::Resample,
                                  ts(100, 100, true, 1000, 2200));
    EXPECT_EQ(r.out_c, 100u);
    EXPECT_EQ(r.out_r, 91u);
    EXPECT_TRUE(r.resample);
}

TEST(Fit, PixelAspectFallsBackToSquareWhenPixelsAreUnavailable) {
    tid::FitResolution r = tid::resolve_fit(100, 100, 1, Fit::Resample, ts(100, 100));
    EXPECT_EQ(r.out_c, 100u);
    EXPECT_EQ(r.out_r, 100u);
    EXPECT_FALSE(r.resample);
}

TEST(Fit, TrimFillsTerminalWithoutAspectConstraint) {
    // Trim is identity-sampled (no stretching possible), so it fills the
    // terminal with whatever top-left region fits — no aspect adjustment.
    tid::FitResolution r = tid::resolve_fit(100, 200, 1, Fit::Trim, ts(10, 40));
    EXPECT_EQ(r.out_r, 20u);
    EXPECT_EQ(r.out_c, 40u);
    EXPECT_FALSE(r.resample);
}

TEST(Fit, TrimDoesNotShrinkAxisThatFits) {
    // Source 10×200 fits vertically as-is; Trim should leave out_r=10 and
    // only clamp width. Resample with the same inputs would shrink height
    // for aspect — this test guards the divergence between the two modes.
    tid::FitResolution r = tid::resolve_fit(10, 200, 1, Fit::Trim, ts(50, 40));
    EXPECT_EQ(r.out_r, 10u);   // unchanged
    EXPECT_EQ(r.out_c, 40u);   // capped
    EXPECT_FALSE(r.resample);
}

TEST(Fit, BlockSizeScalesCaps) {
    // With bs=4, each output cell takes 4 pixel cols. Terminal 40 cols → 10 cells.
    // 100×200 source: width-limited scale → out_pcols=40, out_prows=40*100/200=20.
    // Divided by bs=4: out_c=10, out_r=5 (aspect preserved, was previously 10×50).
    tid::FitResolution r = tid::resolve_fit(100, 200, 4, Fit::Resample, ts(100, 40));
    EXPECT_EQ(r.out_c, 10u);
    EXPECT_EQ(r.out_r, 5u);
    EXPECT_TRUE(r.resample);
}

TEST(Fit, BlockSizeLargerThanTerminalClampsToOne) {
    // bs=100, terminal only 40 cols wide → can't even render one cell, clamp to 1.
    tid::FitResolution r = tid::resolve_fit(100, 200, 100, Fit::Resample, ts(100, 40));
    EXPECT_EQ(r.out_c, 1u);
    EXPECT_TRUE(r.resample);
}

TEST(Fit, AutoBlockSizeInvalidTerminalIsNoop) {
    Options opts;
    EXPECT_EQ(tid::resolve_effective_block_size(8, 8, opts, ts(0, 0, false)), 1u);
}

TEST(Fit, AutoBlockSizeRespectsExplicitBlockSize) {
    Options opts;
    opts.block_size(1);
    EXPECT_EQ(tid::resolve_effective_block_size(8, 8, opts, ts(24, 80)), 1u);

    opts.block_size(3);
    EXPECT_EQ(tid::resolve_effective_block_size(8, 8, opts, ts(24, 80)), 3u);
}

TEST(Fit, AutoBlockSizeUpscalesSmallImage) {
    Options opts;
    EXPECT_EQ(tid::resolve_effective_block_size(8, 8, opts, ts(24, 80)), 4u);
}

TEST(Fit, AutoBlockSizeNeverExceedsDisplay) {
    Options opts;
    size_t bs = tid::resolve_effective_block_size(5, 7, opts, ts(6, 20));

    EXPECT_GT(bs, 1u);
    EXPECT_LE(7u * bs, 20u);
    // One terminal row is reserved for the default scalar colorbar.
    EXPECT_LE(5u * bs, (6u - 1u) * 2u);

    tid::FitResolution r = tid::resolve_fit(5, 7, bs, Fit::Resample, ts(6, 20));
    EXPECT_EQ(r.out_r, 5u);
    EXPECT_EQ(r.out_c, 7u);
    EXPECT_FALSE(r.resample);
}

TEST(Fit, AutoBlockSizeLeavesAlreadyTooLargeImageAtOne) {
    Options opts;
    EXPECT_EQ(tid::resolve_effective_block_size(100, 100, opts, ts(10, 40)), 1u);
}

TEST(Fit, AutoBlockSizeIgnoresAspectOnlyResample) {
    // Half-block pixels are not square here, so Fit::Resample will adjust
    // output rows. That should not suppress auto-enlargement for a small image.
    Options opts;
    EXPECT_EQ(tid::resolve_effective_block_size(10, 10, opts,
        ts(50, 50, true, 500, 1100)), 4u);
}

TEST(Fit, AutoBlockSizeUpscalesCenterCropWithTerminalPixels) {
    Options opts;
    opts.title().center_crop(4, 4, 6, 4);

    EXPECT_EQ(tid::resolve_effective_block_size(4, 6, opts,
        ts(24, 80, true, 640, 480)), 8u);
}

TEST(Clim, AutoClimScansWholeVisibleSourceRegion) {
    double values[] = {
        0.0, 100.0, 1.0,
        2.0, 3.0, 4.0
    };
    double lo = std::numeric_limits<double>::quiet_NaN();
    double hi = std::numeric_limits<double>::quiet_NaN();

    tid::apply_auto_clim(2, 3,
        [&](size_t r, size_t c) { return values[r * 3 + c]; },
        lo, hi, true, true);

    EXPECT_DOUBLE_EQ(lo, 0.0);
    EXPECT_DOUBLE_EQ(hi, 100.0);
}

TEST(Resampling, NearestUsesIntegerSourceCells) {
    double values[] = {
        0.0,  1.0,  2.0,  3.0,
        4.0,  5.0,  6.0,  7.0,
        8.0,  9.0, 10.0, 11.0,
       12.0, 13.0, 14.0, 15.0
    };

    double v = tid::sample_resampled(1, 1, 2, 2, 4, 4,
        [&](size_t r, size_t c) { return values[r * 4 + c]; },
        Resampling::Nearest);

    EXPECT_DOUBLE_EQ(v, 10.0);
}

TEST(Resampling, BilinearAveragesCenteredSourcePatch) {
    double values[] = {
        0.0, 10.0,
        20.0, 30.0
    };

    double v = tid::sample_resampled(0, 0, 1, 1, 2, 2,
        [&](size_t r, size_t c) { return values[r * 2 + c]; },
        Resampling::Bilinear);

    EXPECT_DOUBLE_EQ(v, 15.0);
}

TEST(Resampling, BilinearFallsBackToNearestAroundNonFiniteValues) {
    double values[] = {
        0.0, std::numeric_limits<double>::infinity(),
        20.0, 30.0
    };

    double v = tid::sample_resampled(0, 0, 1, 1, 2, 2,
        [&](size_t r, size_t c) { return values[r * 2 + c]; },
        Resampling::Bilinear);

    EXPECT_DOUBLE_EQ(v, 0.0);
}

TEST(Resampling, RgbBilinearInterpolatesEachChannel) {
    tid::RGB values[] = {
        tid::RGB{0, 0, 0},       tid::RGB{100, 0, 0},
        tid::RGB{0, 100, 0},     tid::RGB{0, 0, 100}
    };

    tid::RGB c = tid::sample_rgb_resampled(0, 0, 1, 1, 2, 2,
        [&](size_t r, size_t col) { return values[r * 2 + col]; },
        Resampling::Bilinear);

    EXPECT_EQ(c, (tid::RGB{25, 25, 25}));
}

TEST(Fit, OstringstreamRendersIdenticallyAcrossModes) {
    // ostringstream is not a tty, so Resample / Trim / Off must produce the
    // same bytes. This guards against accidentally activating fit on non-tty
    // streams (which would silently mutate piped output).
    std::vector<double> data(50 * 80);
    for (size_t i = 0; i < data.size(); ++i) data[i] = static_cast<double>(i);

    std::string off      = render_to_string(data.data(), 50, 80, Options().fit(Fit::Off));
    std::string resample = render_to_string(data.data(), 50, 80, Options().fit(Fit::Resample));
    std::string trim     = render_to_string(data.data(), 50, 80, Options().fit(Fit::Trim));

    EXPECT_EQ(off, resample);
    EXPECT_EQ(off, trim);
}

// ---------------------------------------------------------------------------
// Optional title
// ---------------------------------------------------------------------------

TEST(Title, RenderPrependsConciseSummaryWhenEnabled) {
    double data[] = {0.0, 1.0, 2.0, 3.0};
    std::string out = to_string(data, 2, 2, Options().title());

    EXPECT_EQ(out.find("termimage: data=2x2 cmap=gray\n"), 0u);
}

TEST(Title, CustomLabelAndNonDefaultOptionsAppearInSummary) {
    double data[] = {0.0, 1.0, 2.0, 3.0};
    std::string out = to_string(data, 2, 2,
        Options().title("frame 7")
                 .colormap("turbo")
                 .layout(Layout::ColMajor)
                 .block_size(2));

    EXPECT_EQ(out.find("frame 7: data=2x2 cmap=turbo layout=col-major block=2\n"), 0u);
}

TEST(Title, ControlCharactersAreSanitized) {
    double data[] = {1.0};
    std::string label = std::string("bad") + '\x1b' + "[31m\nnext";
    std::string out = to_string(data, 1, 1, Options().title(label));

    EXPECT_EQ(out.find("bad [31m next: data=1x1 cmap=gray\n"), 0u);
}

TEST(Title, SummaryIncludesCrop) {
    Options opts;
    opts.title().crop(1, 2, 3, 4).colormap("viridis");

    EXPECT_EQ(tid::render_title(opts, 10, 20, 2, 1, 4, 3, 4, 3, false),
        "termimage: data=10x20 crop=(1,2 3x4) cmap=viridis");
}

TEST(Title, SummaryIncludesRenderedSizeWhenResampledOrTrimmed) {
    Options resampled;
    resampled.title().colormap("magma");
    EXPECT_EQ(tid::render_title(resampled, 100, 200, 0, 0, 100, 200, 20, 40, true),
        "termimage: data=100x200 rendered as 20x40 resampled cmap=magma");

    Options trimmed;
    trimmed.title().fit(Fit::Trim);
    EXPECT_EQ(tid::render_title(trimmed, 100, 200, 0, 0, 100, 200, 20, 40, false),
        "termimage: data=100x200 rendered as 20x40 trimmed cmap=gray");
}

TEST(Title, RgbSummaryIncludesRgbLayout) {
    Options opts;
    opts.title("rgb frame").layout(Layout::ColMajor).block_size(2);

    EXPECT_EQ(tid::render_rgb_title(opts, "planar",
            10, 20, 2, 1, 4, 3, 4, 3, false),
        "rgb frame: data=10x20 crop=(1,2 3x4) rgb=planar layout=col-major block=2");
}

// ---------------------------------------------------------------------------
// Colorbar
// ---------------------------------------------------------------------------

TEST(Colorbar, AppendsScaleBelowScalarImage) {
    double data[] = {0.0, 1.0};
    std::string out = to_string(data, 1, 2,
        Options().colormap("gray").clim(-2.5, 7.5));

    int newlines = 0;
    for (char ch : out) if (ch == '\n') newlines++;
    EXPECT_EQ(newlines, 2);
    EXPECT_NE(out.find("\n-2.5 "), std::string::npos);
    EXPECT_NE(out.find(" 7.5\n"), std::string::npos);
    EXPECT_NE(out.find("\x1b[48;2;128;128;128m"), std::string::npos);
}

TEST(Colorbar, CanBeDisabled) {
    double data[] = {0.0, 1.0};
    std::string out = to_string(data, 1, 2,
        Options().colormap("gray").colorbar(false));

    int newlines = 0;
    for (char ch : out) if (ch == '\n') newlines++;
    EXPECT_EQ(newlines, 1);
    EXPECT_EQ(out.find("\n0 "), std::string::npos);
}

TEST(Colorbar, UsesAutoClimLabels) {
    double data[] = {3.0, 0.0};
    std::string out = to_string(data, 1, 2,
        Options().colormap("gray"));

    EXPECT_NE(out.find("\n0 "), std::string::npos);
    EXPECT_NE(out.find(" 3\n"), std::string::npos);
}

TEST(Colorbar, IgnoredForRgbImage) {
    const std::uint8_t data[] = {255, 0, 0};

    EXPECT_EQ(to_string(data, 1, 1, Options().rgb()),
              to_string(data, 1, 1, Options().rgb().colorbar(false)));
}

// ---------------------------------------------------------------------------
// to_string convenience function
// ---------------------------------------------------------------------------

TEST(ToString, MatchesPrintOutput) {
    double data[] = {0.0, 0.5, 1.0, 0.25};
    // to_string should produce the same output as print-to-ostringstream
    std::string via_tostring = to_string(data, 2, 2,
        Options().colormap("magma"));
    std::ostringstream oss;
    print(data, 2, 2, Options().ostream(oss).colormap("magma"));
    std::string via_print = oss.str();
    EXPECT_EQ(via_tostring, via_print);
}

TEST(ToString, EmptyOnZeroDimensions) {
    double data[] = {1.0};
    EXPECT_EQ(to_string(data, 0, 1), "");
    EXPECT_EQ(to_string(data, 1, 0), "");
}

TEST(ToString, DoesNotAffectOriginalOstream) {
    // Passing opts with a custom ostream — to_string should NOT write to it
    std::ostringstream oss;
    Options opts;
    opts.colormap("gray").ostream(oss);
    double data[] = {0.0, 1.0};
    std::string result = to_string(data, 1, 2, opts);
    EXPECT_FALSE(result.empty());
    // The original ostream should be untouched
    EXPECT_EQ(oss.str(), "");
}

TEST(ToStringRgb, MatchesPrintOutput) {
    const std::uint8_t data[] = {
        255, 0, 0,
        0, 0, 255
    };
    std::string via_tostring = to_string(data, 2, 1, Options().rgb());
    std::string via_print = capture_rgb_output(data, 2, 1);

    EXPECT_EQ(via_tostring, via_print);
}

TEST(ToStringRgb, SupportsPlanarLayout) {
    const std::uint8_t data[] = {
        255, 0,
        0, 0,
        0, 255
    };

    EXPECT_EQ(to_string(data, 2, 1, Options().rgb(RGBLayout::Planar)),
              capture_rgb_output(data, 2, 1, RGBLayout::Planar));
}

TEST(ToStringRgb, DoesNotAffectOriginalOstream) {
    std::ostringstream oss;
    Options opts;
    opts.ostream(oss);
    const std::uint8_t data[] = {255, 0, 0};

    std::string result = to_string(data, 1, 1, opts.rgb());

    EXPECT_FALSE(result.empty());
    EXPECT_EQ(oss.str(), "");
}

TEST(VectorApi, PrintMatchesPointerInput) {
    std::vector<double> data = {0.0, 0.5, 1.0, 0.25};
    std::ostringstream oss;
    print(data, 2, 2, Options().ostream(oss).colormap("magma"));

    EXPECT_EQ(oss.str(), to_string(data.data(), 2, 2,
        Options().colormap("magma")));
}

TEST(VectorApi, ToStringMatchesPointerInput) {
    std::vector<int> data = {0, 1, 2, 3};

    EXPECT_EQ(to_string(data, 2, 2, Options().colormap("viridis")),
        to_string(data.data(), 2, 2, Options().colormap("viridis")));
}

TEST(VectorApi, RejectsMismatchedDimensions) {
    std::vector<double> data = {0.0, 1.0, 2.0};

    EXPECT_THROW(print(data, 2, 2), std::invalid_argument);
    EXPECT_THROW(to_string(data, 2, 2), std::invalid_argument);
}

TEST(VectorApi, RgbVectorToStringMatchesPointerInput) {
    std::vector<std::uint8_t> data = {
        255, 0, 0,
        0, 0, 255
    };

    EXPECT_EQ(to_string(data, 2, 1, Options().rgb()),
              to_string(data.data(), 2, 1, Options().rgb()));
}

TEST(VectorApi, RgbVectorToStringRejectsMismatchedDimensions) {
    std::vector<std::uint8_t> data(5);

    EXPECT_THROW(to_string(data, 2, 1, Options().rgb()), std::invalid_argument);
}
