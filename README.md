# termimage

A tiny header-only C++11 library for debugging and visualizing 2D numeric arrays directly in the terminal.  Similar to MATLAB's `imagesc` or matplotlib's `imshow`, but directly from your C++ application into the terminal.

```cpp
termimage::print(data_ptr, n_rows, n_cols);
```

## Highlights

- `termimage::print(*data, rows, cols)` any data that can be cast to `double`
- **Header-only**: `#include "termimage.h"`
- **Minimal requirements**: C++11 compiler and a modern terminal (with full color support)
- **Color mapping** with support for `gray`, `magma`, `viridis` (from [matplotlib](https://github.com/matplotlib/matplotlib))
- **Automatic or manual scaling**: computes `clim` from data, or you can set it yourself
- **Terminal auto-fit**: oversized images are resampled (or trimmed) to fit the window
- **Crop**: render a subregion without copying data
- **Upscale**: block upscale small matrices for visibility
- **Row-major & column-major** memory layout support
- **NaN & Inf aware** — NaN cells render as transparent gaps; ±Inf clamps to the color range
- **Any numeric type** — templated over `int`, `float`, `double`, etc.

## Quick Start

```cpp
#include "termimage.h"

double data[] = {0, 1, 2, 3, 4, 5};
termimage::print(data, 2, 3); // print with 2 rows, 3 cols
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
void print(const T* data, int rows, int cols,
           const Options& opts = Options());
```

Renders a `rows × cols` matrix pointed to by `data` as a colored image on the terminal.

### `termimage::Options`

All setters are chainable and return `Options&`.

| Setter | Getter | Description |
|---|---|---|
| `.clim(lo, hi)` | `.clim_lo()` / `.clim_hi()` | Color limits. Default: auto from data. |
| `.colormap(Colormap)` | `.colormap()` | Set by enum (`Colormap::Gray`, `Magma`, `Viridis`). |
| `.colormap("name")` | | Set by string (`"gray"`, `"magma"`, `"viridis"`). |
| `.block_size(n)` | `.block_size()` | Pixel scale factor per matrix element. Default: `1`. |
| `.layout(Layout)` | `.layout()` | `Layout::RowMajor` (default) or `Layout::ColMajor`. |
| `.ostream(os)` | `.ostream()` | Output stream. Default: `std::cout`. |
| `.crop(r0, c0)` | `.crop_r0()` / `.crop_c0()` | Crop from `(r0, c0)` to end of matrix. |
| `.crop(r0, c0, h, w)` | `.crop_h()` / `.crop_w()` | Crop a `h × w` subregion starting at `(r0, c0)`. |
| `.fit(Fit)` | `.fit()` | Behavior when the image exceeds the terminal. Default: `Fit::Resample`. |

### Enums

```cpp
enum class Layout   { RowMajor, ColMajor };
enum class Colormap { Gray, Magma, Viridis };
enum class Fit      { Off, Resample, Trim };
```

`Fit::Resample` (default) nearest-neighbor downsamples oversized images to fit,
using a single uniform scale factor so the source aspect ratio is preserved —
a square matrix stays square, a `10 × 200` waveform stays wide and thin.
`Fit::Trim` renders the top-left portion that fits and discards the rest
(identity-sampled, so the rendered pixels are 1:1 with the source).
`Fit::Off` always renders at full size, letting the terminal wrap or scroll.

Fit only engages when the output stream is a real terminal (stdout/stderr +
`isatty`). When rendering to an `ostringstream`, file, or pipe, fit behaves as
`Off`, so `to_string()` and redirected output are byte-identical regardless of
mode.

## Requirements

- C++11 compiler
- A terminal with 24-bit truecolor support

## License

MIT
