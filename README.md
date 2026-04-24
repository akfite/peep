# termimage

A tiny header-only C++11 library for debugging and visualizing 2D numeric arrays directly in the terminal.  Similar to MATLAB's `imagesc` or matplotlib's `imshow`, but directly from your C++ application into the terminal.

```cpp
termimage::print(data_ptr, n_rows, n_cols);
termimage::print(data_vector, n_rows, n_cols);
termimage::print_rgb(rgb_vector, n_rows, n_cols);
std::string rendered = termimage::rgb_to_string(rgb_vector, n_rows, n_cols);
```

## Highlights

- `termimage::print(data, rows, cols)` for raw pointers or `std::vector` data that can be cast to `double`
- `termimage::print_rgb(data, rows, cols)` for `uint8_t` RGB images
- **Header-only**: `#include "termimage.h"`
- **Minimal requirements**: C++11 compiler and a modern terminal (with full color support)
- **Color mapping** with support for `gray`, `viridis`, `plasma`, `inferno`, `magma`, `cividis`, `coolwarm`, `gnuplot`, and `turbo`
- **Automatic or manual scaling**: computes `clim` from data, or you can set it yourself
- **Terminal auto-fit**: oversized images are resampled (or trimmed) to fit the window
- **Crop**: render a subregion without copying data
- **Upscale**: block upscale small matrices for visibility
- **Row-major & column-major** memory layout support
- **RGB image support** — interleaved `RGBRGB...` or planar `RRR...GGG...BBB...` `uint8_t` data
- **NaN & Inf aware** — NaN cells render as transparent gaps and ±Inf clamps to the color range by default; both are color-configurable
- **Any numeric type** — templated over `int`, `float`, `double`, etc.

## Quick Start

```cpp
#include "termimage.h"
#include <cstdint>
#include <vector>

double data[] = {0, 1, 2, 3, 4, 5};
termimage::print(data, 2, 3); // print with 2 rows, 3 cols

std::vector<double> vec = {0, 1, 2, 3, 4, 5};
termimage::print(vec, 2, 3);

std::vector<std::uint8_t> rgb = {
    255, 0, 0,   0, 255, 0,
    0, 0, 255,   255, 255, 255
};
termimage::print_rgb(rgb, 2, 2);
```

## Building

```bash
cmake -S . -B build
cmake --build build
```

This builds the demo (but not unit tests) by default. To configure:

```bash
cmake -S . -B build -DTERMIMAGE_BUILD_EXAMPLES=OFF
cmake -S . -B build -DTERMIMAGE_BUILD_TESTS=OFF
```

### Running the Demo

```bash
./build/demo
```

### Running Tests

Tests use [GoogleTest](https://github.com/google/googletest) (fetched automatically via CMake `FetchContent`):

```bash
ctest --test-dir build
```

## API Reference

### `termimage::print`

```cpp
template <typename T>
void print(const T* data, size_t rows, size_t cols,
           const Options& opts = Options());

template <typename T>
void print(const std::vector<T>& data, size_t rows, size_t cols,
           const Options& opts = Options());
```

Renders a `rows × cols` matrix as a colored image on the terminal. Pointer
inputs are intentionally simple and work with custom containers; their
dimensions are trusted. The vector overload validates that
`data.size() == rows * cols`.

### `termimage::print_rgb`

```cpp
void print_rgb(const std::uint8_t* data, size_t rows, size_t cols,
               const Options& opts = Options());

void print_rgb(const std::uint8_t* data, size_t rows, size_t cols,
               RGBLayout rgb_layout, const Options& opts = Options());

void print_rgb(const std::vector<std::uint8_t>& data, size_t rows, size_t cols,
               const Options& opts = Options());

void print_rgb(const std::vector<std::uint8_t>& data, size_t rows, size_t cols,
               RGBLayout rgb_layout, const Options& opts = Options());
```

Renders a `rows × cols` RGB image from `uint8_t` data. The default
`RGBLayout::Interleaved` expects `RGBRGB...`; `RGBLayout::Planar` expects all
red samples first, then green, then blue. Pointer dimensions are trusted. The
vector overload validates that `data.size() == rows * cols * 3`.

`print_rgb` uses the layout, crop, fit, resampling, block size, title, and
ostream options. Scalar-only options such as `clim`, `colormap`, and special
NaN/Inf colors do not apply.

### `termimage::rgb_to_string`

```cpp
std::string rgb_to_string(const std::uint8_t* data, size_t rows, size_t cols,
                          const Options& opts = Options());

std::string rgb_to_string(const std::uint8_t* data, size_t rows, size_t cols,
                          RGBLayout rgb_layout, const Options& opts = Options());

std::string rgb_to_string(const std::vector<std::uint8_t>& data,
                          size_t rows, size_t cols,
                          const Options& opts = Options());

std::string rgb_to_string(const std::vector<std::uint8_t>& data,
                          size_t rows, size_t cols,
                          RGBLayout rgb_layout,
                          const Options& opts = Options());
```

Returns the same ANSI-rendered RGB output as `print_rgb` without writing to the
configured stream.

### `termimage::Options`

All setters are chainable and return `Options&`.

| Setter | Getter | Description |
|---|---|---|
| `.clim(lo, hi)` | `.clim_lo()` / `.clim_hi()` | Color limits. Default: auto from data. |
| `.colormap(Colormap)` | `.colormap()` | Set by enum (`Colormap::Gray`, `Magma`, `Viridis`, `Plasma`, `Inferno`, `Cividis`, `Coolwarm`, `Gnuplot`, `Turbo`). |
| `.colormap("name")` | | Set by string (`"gray"`, `"viridis"`, `"plasma"`, `"inferno"`, `"magma"`, `"cividis"`, `"coolwarm"`, `"gnuplot"`, `"turbo"`). |
| `.colormap(ColormapLut)` | `.custom_colormap()` | Set a custom 256-entry RGB LUT copied from `std::array<std::uint8_t, 768>`. |
| `.colormap(vector<uint8_t>)` | `.custom_colormap()` | Set a custom LUT from a vector with exactly 768 bytes. |
| `.nan_color(Color)` | `.nan_color()` / `.has_nan_color()` | Render NaN values with a fixed RGB color instead of transparent gaps. |
| `.inf_color(Color)` / `.inf_colors(neg, pos)` | `.neg_inf_color()` / `.pos_inf_color()` | Render infinities with fixed RGB colors instead of colormap endpoints. |
| `.block_size(n)` | `.block_size()` | Pixel scale factor per matrix element. Default: `1`. |
| `.layout(Layout)` | `.layout()` | `Layout::RowMajor` (default) or `Layout::ColMajor`. |
| `.ostream(os)` | `.ostream()` | Output stream. Default: `std::cout`. |
| `.crop(r0, c0)` | `.crop_r0()` / `.crop_c0()` | Crop from `(r0, c0)` to end of matrix. |
| `.crop(r0, c0, h, w)` | `.crop_h()` / `.crop_w()` | Crop a `h × w` subregion starting at `(r0, c0)`. |
| `.fit(Fit)` | `.fit()` | Behavior when the image exceeds the terminal. Default: `Fit::Resample`. |
| `.resampling(Resampling)` | `.resampling()` | Strategy used by `Fit::Resample`: `Resampling::Bilinear` (default) or `Resampling::Nearest`. |
| `.title()` / `.title("label")` | `.show_title()` / `.title_text()` | Prepend a concise info line with data size, crop, display size when resized, colormap, and non-default layout/block size. Default: off. |

### Enums

```cpp
enum class Layout   { RowMajor, ColMajor };
enum class RGBLayout { Interleaved, Planar };
enum class Resampling { Nearest, Bilinear };
enum class Colormap {
    Gray, Magma, Viridis, Plasma, Inferno, Cividis, Coolwarm, Gnuplot, Turbo
};
enum class Fit      { Off, Resample, Trim };
```

`Fit::Resample` (default) downsamples oversized images to fit,
using a single uniform scale factor so the source aspect ratio is preserved —
a square matrix stays square, a `10 × 200` waveform stays wide and thin.
The resampling strategy defaults to bilinear interpolation; use
`.resampling(Resampling::Nearest)` for nearest-neighbor sampling.
`Fit::Trim` renders the top-left portion that fits and discards the rest
(identity-sampled, so the rendered pixels are 1:1 with the source).
`Fit::Off` always renders at full size, letting the terminal wrap or scroll.

Fit only engages when the output stream is a real terminal (stdout/stderr +
`isatty`). When rendering to an `ostringstream`, file, or pipe, fit behaves as
`Off`, so `to_string()` and redirected output are byte-identical regardless of
mode.

When the terminal reports both cell and pixel dimensions, `Fit::Resample`
also compensates for terminals whose half-block pixels are not physically
square. Terminals that do not expose pixel dimensions use the standard `1:2`
cell assumption.

## Requirements

- C++11 compiler
- A terminal with 24-bit truecolor support
- `matplotlib` only when regenerating `include/termimage_colormaps.h` with
  `scripts/generate_colormaps.py`

## License

MIT
