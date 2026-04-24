#include "termimage.h"

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
using namespace termimage::detail;

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
    EXPECT_EQ(opts.layout(), Layout::RowMajor);
    EXPECT_EQ(&opts.ostream(), &std::cout);
    EXPECT_EQ(opts.crop_r0(), 0);
    EXPECT_EQ(opts.crop_c0(), 0);
    EXPECT_EQ(opts.crop_h(), 0u);
    EXPECT_EQ(opts.crop_w(), 0u);
    EXPECT_EQ(opts.fit(), Fit::Resample);
    EXPECT_FALSE(opts.show_title());
    EXPECT_EQ(opts.title_text(), "");
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
                       .title("demo");

    // Chaining must return the same object
    EXPECT_EQ(&ref, &opts);

    EXPECT_DOUBLE_EQ(opts.clim_lo(), 1.0);
    EXPECT_DOUBLE_EQ(opts.clim_hi(), 2.0);
    EXPECT_EQ(opts.colormap(), Colormap::Magma);
    EXPECT_EQ(opts.block_size(), 3);
    EXPECT_EQ(opts.layout(), Layout::ColMajor);
    EXPECT_EQ(&opts.ostream(), &oss);
    EXPECT_EQ(opts.crop_r0(), 4u);
    EXPECT_EQ(opts.crop_c0(), 5u);
    EXPECT_EQ(opts.crop_h(), 6u);
    EXPECT_EQ(opts.crop_w(), 7u);
    EXPECT_EQ(opts.fit(), Fit::Trim);
    EXPECT_TRUE(opts.show_title());
    EXPECT_EQ(opts.title_text(), "demo");
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
    opts.crop(3, 7);
    EXPECT_EQ(opts.crop_r0(), 3u);
    EXPECT_EQ(opts.crop_c0(), 7u);
    EXPECT_EQ(opts.crop_h(), 0u);
    EXPECT_EQ(opts.crop_w(), 0u);
}

// ---------------------------------------------------------------------------
// find_colormap
// ---------------------------------------------------------------------------

TEST(FindColormap, KnownColormapsByString) {
    EXPECT_EQ(&find_colormap("gray"), &cmap_gray());
    EXPECT_EQ(&find_colormap("grey"), &cmap_gray());
    EXPECT_EQ(&find_colormap("magma"), &CMAP_MAGMA);
    EXPECT_EQ(&find_colormap("viridis"), &CMAP_VIRIDIS);
    EXPECT_EQ(&find_colormap("plasma"), &CMAP_PLASMA);
    EXPECT_EQ(&find_colormap("inferno"), &CMAP_INFERNO);
    EXPECT_EQ(&find_colormap("cividis"), &CMAP_CIVIDIS);
    EXPECT_EQ(&find_colormap("coolwarm"), &CMAP_COOLWARM);
    EXPECT_EQ(&find_colormap("gnuplot"), &CMAP_GNUPLOT);
    EXPECT_EQ(&find_colormap("turbo"), &CMAP_TURBO);
}

TEST(FindColormap, KnownColormapsByEnum) {
    EXPECT_EQ(&find_colormap(Colormap::Gray), &cmap_gray());
    EXPECT_EQ(&find_colormap(Colormap::Magma), &CMAP_MAGMA);
    EXPECT_EQ(&find_colormap(Colormap::Viridis), &CMAP_VIRIDIS);
    EXPECT_EQ(&find_colormap(Colormap::Plasma), &CMAP_PLASMA);
    EXPECT_EQ(&find_colormap(Colormap::Inferno), &CMAP_INFERNO);
    EXPECT_EQ(&find_colormap(Colormap::Cividis), &CMAP_CIVIDIS);
    EXPECT_EQ(&find_colormap(Colormap::Coolwarm), &CMAP_COOLWARM);
    EXPECT_EQ(&find_colormap(Colormap::Gnuplot), &CMAP_GNUPLOT);
    EXPECT_EQ(&find_colormap(Colormap::Turbo), &CMAP_TURBO);
}

TEST(FindColormap, UnknownStringFallsBackToGray) {
    EXPECT_EQ(&find_colormap("nonexistent"), &cmap_gray());
    EXPECT_EQ(&find_colormap(""), &cmap_gray());
}

// ---------------------------------------------------------------------------
// lookup
// ---------------------------------------------------------------------------

TEST(Lookup, BoundsZeroAndOne) {
    // Gray colormap: index i -> (i, i, i)
    RGB lo = lookup(0.0, cmap_gray());
    EXPECT_EQ(lo.r, 0);
    EXPECT_EQ(lo.g, 0);
    EXPECT_EQ(lo.b, 0);

    RGB hi = lookup(1.0, cmap_gray());
    EXPECT_EQ(hi.r, 255);
    EXPECT_EQ(hi.g, 255);
    EXPECT_EQ(hi.b, 255);
}

TEST(Lookup, MidpointGray) {
    // 0.5 * 255 + 0.5 = 128.0 -> index 128
    RGB mid = lookup(0.5, cmap_gray());
    EXPECT_EQ(mid.r, 128);
    EXPECT_EQ(mid.r, mid.g);
    EXPECT_EQ(mid.g, mid.b);
}

TEST(Lookup, ClampsBelowZero) {
    RGB c = lookup(-1.0, cmap_gray());
    EXPECT_EQ(c.r, 0);
    EXPECT_EQ(c.g, 0);
    EXPECT_EQ(c.b, 0);
}

TEST(Lookup, ClampsAboveOne) {
    RGB c = lookup(2.0, cmap_gray());
    EXPECT_EQ(c.r, 255);
    EXPECT_EQ(c.g, 255);
    EXPECT_EQ(c.b, 255);
}

TEST(Lookup, MagmaEndpoints) {
    // Magma index 0: (0x00, 0x00, 0x04)
    RGB lo = lookup(0.0, CMAP_MAGMA);
    EXPECT_EQ(lo.r, 0x00);
    EXPECT_EQ(lo.g, 0x00);
    EXPECT_EQ(lo.b, 0x04);

    // Magma index 255 from matplotlib's magma colormap.
    RGB hi = lookup(1.0, CMAP_MAGMA);
    EXPECT_EQ(hi.r, 0xfc);
    EXPECT_EQ(hi.g, 0xfd);
    EXPECT_EQ(hi.b, 0xbf);
}

// ---------------------------------------------------------------------------
// emit helpers
// ---------------------------------------------------------------------------

TEST(Emit, BackgroundEscape) {
    std::ostringstream oss;
    RGB c = {10, 20, 30};
    emit_bg(oss, c);
    EXPECT_EQ(oss.str(), "\x1b[48;2;10;20;30m");
}

TEST(Emit, ForegroundEscape) {
    std::ostringstream oss;
    RGB c = {255, 128, 0};
    emit_fg(oss, c);
    EXPECT_EQ(oss.str(), "\x1b[38;2;255;128;0m");
}

TEST(Emit, Reset) {
    std::ostringstream oss;
    emit_reset(oss);
    EXPECT_EQ(oss.str(), "\x1b[0m");
}

TEST(Emit, LowerHalfBlock) {
    std::ostringstream oss;
    emit_lower_half(oss);
    EXPECT_EQ(oss.str(), "\xe2\x96\x84");
}

TEST(Emit, UpperHalfBlock) {
    std::ostringstream oss;
    emit_upper_half(oss);
    EXPECT_EQ(oss.str(), "\xe2\x96\x80");
}

// ---------------------------------------------------------------------------
// Helper: capture render output
// ---------------------------------------------------------------------------

static std::string render_to_string(const double* data, size_t rows, size_t cols,
                                     const Options& base = Options()) {
    std::ostringstream oss;
    Options opts = base;
    opts.ostream(oss);
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

TEST(Render, CropOutOfBoundsProducesNothing) {
    double data[] = {1.0, 2.0, 3.0, 4.0};
    // crop start beyond matrix dimensions
    std::string out = render_to_string(data, 2, 2, Options().crop(10, 10, 1, 1));
    EXPECT_EQ(out, "");
}

TEST(Render, CropClampedToMatrixEdge) {
    // crop window extends past matrix; should be clamped
    double data[4] = {0.0, 1.0, 0.5, 0.75};
    std::string out = render_to_string(data, 2, 2, Options().crop(0, 0, 100, 100));
    std::string full = render_to_string(data, 2, 2);
    // Clamped crop should match the full matrix output
    EXPECT_EQ(out, full);
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
    const ColormapLut& g = cmap_gray();
    for (int i = 0; i < 256; ++i) {
        EXPECT_EQ(g[i * 3 + 0], static_cast<std::uint8_t>(i));
        EXPECT_EQ(g[i * 3 + 1], static_cast<std::uint8_t>(i));
        EXPECT_EQ(g[i * 3 + 2], static_cast<std::uint8_t>(i));
    }
}

TEST(CmapGray, StablePointer) {
    // Repeated calls should return the same singleton object.
    EXPECT_EQ(&cmap_gray(), &cmap_gray());
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
TerminalSize ts(size_t rows, size_t cols, bool valid = true) {
    TerminalSize t;
    t.rows = rows;
    t.cols = cols;
    t.valid = valid;
    return t;
}
} // namespace

TEST(Fit, InvalidTerminalIsNoop) {
    // With no usable terminal size, all modes are identity.
    for (Fit m : {Fit::Off, Fit::Resample, Fit::Trim}) {
        FitResolution r = resolve_fit(100, 200, 1, m, ts(0, 0, false));
        EXPECT_EQ(r.out_r, 100u);
        EXPECT_EQ(r.out_c, 200u);
        EXPECT_FALSE(r.resample);
    }
}

TEST(Fit, OffIgnoresTerminalSize) {
    // Even with a too-small terminal, Off leaves dimensions alone.
    FitResolution r = resolve_fit(100, 200, 1, Fit::Off, ts(10, 20));
    EXPECT_EQ(r.out_r, 100u);
    EXPECT_EQ(r.out_c, 200u);
    EXPECT_FALSE(r.resample);
}

TEST(Fit, FitsExactlyIsNoop) {
    // vr*bs = 20 pixel rows = 10 cells, vc*bs = 40 cells → fits exactly.
    FitResolution r = resolve_fit(20, 40, 1, Fit::Resample, ts(10, 40));
    EXPECT_EQ(r.out_r, 20u);
    EXPECT_EQ(r.out_c, 40u);
    EXPECT_FALSE(r.resample);
}

TEST(Fit, ResampleMatchingAspectScalesUniformly) {
    // Source 100×200 (vc:vr = 2:1), terminal pixel cap 40×20 (also 2:1).
    // Uniform scale of 1/5 on both axes → out_c=40, out_r=20.
    FitResolution r = resolve_fit(100, 200, 1, Fit::Resample, ts(10, 40));
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
    FitResolution r = resolve_fit(10, 200, 1, Fit::Resample, ts(50, 40));
    EXPECT_EQ(r.out_c, 40u);
    EXPECT_EQ(r.out_r, 2u);
    EXPECT_EQ(r.out_c / r.out_r, 20u);  // aspect preserved
    EXPECT_TRUE(r.resample);
}

TEST(Fit, ResamplePreservesAspectWhenOnlyHeightOverruns) {
    // Mirror case: source 200×10 (tall & narrow, vc:vr = 1:20). Width fits
    // (10 ≤ 40), height overruns (200 > 2*10=20). Height scale = 20/200 = 1/10;
    // applied to both → out_r=20, out_c=1.
    FitResolution r = resolve_fit(200, 10, 1, Fit::Resample, ts(10, 40));
    EXPECT_EQ(r.out_r, 20u);
    EXPECT_EQ(r.out_c, 1u);
    EXPECT_EQ(r.out_r / r.out_c, 20u);  // aspect preserved
    EXPECT_TRUE(r.resample);
}

TEST(Fit, ResamplePreservesSquareAspect) {
    // Square source (100×100). Terminal 20 cells × 20 cells → 20 pixel cols,
    // 40 pixel rows. Width is tighter. Uniform scale → out_c=20, out_r=20.
    FitResolution r = resolve_fit(100, 100, 1, Fit::Resample, ts(20, 20));
    EXPECT_EQ(r.out_c, 20u);
    EXPECT_EQ(r.out_r, 20u);  // square stays square
    EXPECT_TRUE(r.resample);
}

TEST(Fit, TrimFillsTerminalWithoutAspectConstraint) {
    // Trim is identity-sampled (no stretching possible), so it fills the
    // terminal with whatever top-left region fits — no aspect adjustment.
    FitResolution r = resolve_fit(100, 200, 1, Fit::Trim, ts(10, 40));
    EXPECT_EQ(r.out_r, 20u);
    EXPECT_EQ(r.out_c, 40u);
    EXPECT_FALSE(r.resample);
}

TEST(Fit, TrimDoesNotShrinkAxisThatFits) {
    // Source 10×200 fits vertically as-is; Trim should leave out_r=10 and
    // only clamp width. Resample with the same inputs would shrink height
    // for aspect — this test guards the divergence between the two modes.
    FitResolution r = resolve_fit(10, 200, 1, Fit::Trim, ts(50, 40));
    EXPECT_EQ(r.out_r, 10u);   // unchanged
    EXPECT_EQ(r.out_c, 40u);   // capped
    EXPECT_FALSE(r.resample);
}

TEST(Fit, BlockSizeScalesCaps) {
    // With bs=4, each output cell takes 4 pixel cols. Terminal 40 cols → 10 cells.
    // 100×200 source: width-limited scale → out_pcols=40, out_prows=40*100/200=20.
    // Divided by bs=4: out_c=10, out_r=5 (aspect preserved, was previously 10×50).
    FitResolution r = resolve_fit(100, 200, 4, Fit::Resample, ts(100, 40));
    EXPECT_EQ(r.out_c, 10u);
    EXPECT_EQ(r.out_r, 5u);
    EXPECT_TRUE(r.resample);
}

TEST(Fit, BlockSizeLargerThanTerminalClampsToOne) {
    // bs=100, terminal only 40 cols wide → can't even render one cell, clamp to 1.
    FitResolution r = resolve_fit(100, 200, 100, Fit::Resample, ts(100, 40));
    EXPECT_EQ(r.out_c, 1u);
    EXPECT_TRUE(r.resample);
}

TEST(Clim, AutoClimScansWholeVisibleSourceRegion) {
    double values[] = {
        0.0, 100.0, 1.0,
        2.0, 3.0, 4.0
    };
    double lo = NAN;
    double hi = NAN;

    apply_auto_clim(2, 3,
        [&](size_t r, size_t c) { return values[r * 3 + c]; },
        lo, hi, true, true);

    EXPECT_DOUBLE_EQ(lo, 0.0);
    EXPECT_DOUBLE_EQ(hi, 100.0);
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

    EXPECT_EQ(render_title(opts, 10, 20, 1, 2, 3, 4, 3, 4, false),
        "termimage: data=10x20 crop=(1,2 3x4) cmap=viridis");
}

TEST(Title, SummaryIncludesDisplayedSizeWhenResampledOrTrimmed) {
    Options resampled;
    resampled.title().colormap("magma");
    EXPECT_EQ(render_title(resampled, 100, 200, 0, 0, 100, 200, 20, 40, true),
        "termimage: data=100x200 display=20x40 resampled cmap=magma");

    Options trimmed;
    trimmed.title().fit(Fit::Trim);
    EXPECT_EQ(render_title(trimmed, 100, 200, 0, 0, 100, 200, 20, 40, false),
        "termimage: data=100x200 display=20x40 trimmed cmap=gray");
}

// ---------------------------------------------------------------------------
// to_string convenience function
// ---------------------------------------------------------------------------

TEST(ToString, MatchesPrintOutput) {
    double data[] = {0.0, 0.5, 1.0, 0.25};
    // to_string should produce the same output as print-to-ostringstream
    std::string via_tostring = to_string(data, 2, 2,
        Options().colormap("magma"));
    std::string via_print = render_to_string(data, 2, 2,
        Options().colormap("magma"));
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

TEST(VectorApi, PrintMatchesPointerInput) {
    std::vector<double> data = {0.0, 0.5, 1.0, 0.25};
    std::ostringstream oss;
    print(data, 2, 2, Options().ostream(oss).colormap("magma"));

    EXPECT_EQ(oss.str(), render_to_string(data.data(), 2, 2,
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
