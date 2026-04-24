#include "termimage.h"

#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <sstream>
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
    EXPECT_EQ(opts.colormap(), Colormap::Viridis);
    EXPECT_EQ(opts.block_size(), 1);
    EXPECT_EQ(opts.layout(), Layout::RowMajor);
    EXPECT_EQ(&opts.ostream(), &std::cout);
    EXPECT_EQ(opts.crop_r0(), 0);
    EXPECT_EQ(opts.crop_c0(), 0);
    EXPECT_EQ(opts.crop_h(), -1);
    EXPECT_EQ(opts.crop_w(), -1);
}

TEST(Options, ChainableSetters) {
    std::ostringstream oss;
    Options opts;
    Options& ref = opts.clim(1.0, 2.0)
                       .colormap("magma")
                       .block_size(3)
                       .layout(Layout::ColMajor)
                       .ostream(oss)
                       .crop(4, 5, 6, 7);

    // Chaining must return the same object
    EXPECT_EQ(&ref, &opts);

    EXPECT_DOUBLE_EQ(opts.clim_lo(), 1.0);
    EXPECT_DOUBLE_EQ(opts.clim_hi(), 2.0);
    EXPECT_EQ(opts.colormap(), Colormap::Magma);
    EXPECT_EQ(opts.block_size(), 3);
    EXPECT_EQ(opts.layout(), Layout::ColMajor);
    EXPECT_EQ(&opts.ostream(), &oss);
    EXPECT_EQ(opts.crop_r0(), 4);
    EXPECT_EQ(opts.crop_c0(), 5);
    EXPECT_EQ(opts.crop_h(), 6);
    EXPECT_EQ(opts.crop_w(), 7);
}

TEST(Options, ClimIndividualSetters) {
    Options opts;
    opts.clim_lo(-5.0).clim_hi(10.0);
    EXPECT_DOUBLE_EQ(opts.clim_lo(), -5.0);
    EXPECT_DOUBLE_EQ(opts.clim_hi(), 10.0);
}

TEST(Options, CropTwoArgSetsOpenEnd) {
    Options opts;
    opts.crop(3, 7);
    EXPECT_EQ(opts.crop_r0(), 3);
    EXPECT_EQ(opts.crop_c0(), 7);
    EXPECT_EQ(opts.crop_h(), -1);
    EXPECT_EQ(opts.crop_w(), -1);
}

// ---------------------------------------------------------------------------
// find_colormap
// ---------------------------------------------------------------------------

TEST(FindColormap, KnownColormapsByString) {
    EXPECT_EQ(find_colormap("gray"), CMAP_GRAY);
    EXPECT_EQ(find_colormap("magma"), CMAP_MAGMA);
    EXPECT_EQ(find_colormap("viridis"), CMAP_VIRIDIS);
}

TEST(FindColormap, KnownColormapsByEnum) {
    EXPECT_EQ(find_colormap(Colormap::Gray), CMAP_GRAY);
    EXPECT_EQ(find_colormap(Colormap::Magma), CMAP_MAGMA);
    EXPECT_EQ(find_colormap(Colormap::Viridis), CMAP_VIRIDIS);
}

TEST(FindColormap, UnknownStringFallsBackToGray) {
    EXPECT_EQ(find_colormap("nonexistent"), CMAP_GRAY);
    EXPECT_EQ(find_colormap(""), CMAP_GRAY);
}

// ---------------------------------------------------------------------------
// lookup
// ---------------------------------------------------------------------------

TEST(Lookup, BoundsZeroAndOne) {
    // Gray colormap: index i -> (i, i, i)
    RGB lo = lookup(0.0, CMAP_GRAY);
    EXPECT_EQ(lo.r, 0);
    EXPECT_EQ(lo.g, 0);
    EXPECT_EQ(lo.b, 0);

    RGB hi = lookup(1.0, CMAP_GRAY);
    EXPECT_EQ(hi.r, 255);
    EXPECT_EQ(hi.g, 255);
    EXPECT_EQ(hi.b, 255);
}

TEST(Lookup, MidpointGray) {
    // 0.5 * 255 + 0.5 = 128.0 -> index 128
    RGB mid = lookup(0.5, CMAP_GRAY);
    // Gray colormap entry 128: value ~0x81
    EXPECT_NEAR(mid.r, 0x81, 2);
    EXPECT_EQ(mid.r, mid.g);
    EXPECT_EQ(mid.g, mid.b);
}

TEST(Lookup, ClampsBelowZero) {
    RGB c = lookup(-1.0, CMAP_GRAY);
    EXPECT_EQ(c.r, 0);
    EXPECT_EQ(c.g, 0);
    EXPECT_EQ(c.b, 0);
}

TEST(Lookup, ClampsAboveOne) {
    RGB c = lookup(2.0, CMAP_GRAY);
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

    // Magma index 255: (0xfd, 0xfe, 0xc0)
    RGB hi = lookup(1.0, CMAP_MAGMA);
    EXPECT_EQ(hi.r, 0xfd);
    EXPECT_EQ(hi.g, 0xfe);
    EXPECT_EQ(hi.b, 0xc0);
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

static std::string render_to_string(const double* data, int rows, int cols,
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

TEST(Render, NegativeDimensionsProducesNothing) {
    double data[] = {1.0};
    EXPECT_EQ(render_to_string(data, -1, 5), "");
    EXPECT_EQ(render_to_string(data, 5, -1), "");
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
    // crop(1,1) with h=-1,w=-1 should show from (1,1) to end
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
