# peep

`peep` is a small header-only C++11 library for showing data as color images directly in the terminal.  It is intended to be an easy-to-use debugging and visualization tool, not a real graphics stack.

```cpp
#include <peep/peep.hpp>

int main() {
    int data[] = { 1, 2, 3, 4, 5, 6 };
    peep::show(data, 2, 3); // as 2x3 matrix
}
```

If your terminal supports 24-bit color, you will see a matrix displayed as an image using ANSI half-block characters:

![peep rendering a small matrix in the terminal](assets/readme_intro_demo.svg)

## Features

- Show any 2D numeric data or RGB image
- Includes built-in colormaps from [matplotlib](https://matplotlib.org/): `gray`, `viridis`, `plasma`, `inferno`, `magma`, `cividis`, `coolwarm`, `gnuplot`, `turbo` (or load your own)
- Render a colorbar and configure color limits
- Support for row-major and column-major memory layouts
- Show your own custom types by defining an accessor function
- Crop out and display only a part of your buffer
- NaN and infinity handling
- Automatic block upsampling of small arrays
- Can resample arrays too large to render in the terminal
- `to_string()` when you want the ANSI output but do not want to write it yet
- All options are configurable

## Examples

### Colormaps

[`examples/03_colormap_gallery.cpp`](examples/03_colormap_gallery.cpp) renders the same linear ramp with each built-in colormap.

```cpp
peep::show(values, rows, cols, peep::Options()
    .colormap("viridis")
    .clim(0.0, 1.0));
```

![Expected terminal output from examples/03_colormap_gallery.cpp](assets/readme_03_colormap_gallery.svg)

Scalar images default to `gray` if no colormap is specified. RGB rendering paths ignore colormaps.

### Visualize a 2D function ("peaks")

[`examples/12_matlab_peaks.cpp`](examples/12_matlab_peaks.cpp) renders a 2D function as an image with the `viridis` colormap.

```cpp
peep::show(surface, n, n, peep::Options()
    .colormap("viridis")
    .title("MATLAB peaks surface"));
```

![Expected terminal output from examples/12_matlab_peaks.cpp](assets/readme_12_matlab_peaks.svg)

### Highlight NaNs

[`examples/04_highlight_nans.cpp`](examples/04_highlight_nans.cpp) shows how to pinpoint the location of NaNs in an array.

```cpp
peep::show(instrument, rows, cols, peep::Options()
    .colormap("gray")
    .clim(0.0, 1.0)
    .nan_color(255, 20, 147)); // hot pink
```

![Expected terminal output from examples/04_highlight_nans.cpp](assets/readme_04_highlight_nans.svg)

Infinities can be handled the same way with `inf_colors()`. See [`examples/05_outliers_and_infinities.cpp`](examples/05_outliers_and_infinities.cpp).

### Covariance Matrix

[`examples/07_covariance_coolwarm.cpp`](examples/07_covariance_coolwarm.cpp) uses a diverging colormap and explicit color limits for a covariance matrix.

```cpp
peep::show(cov, features, features, peep::Options()
    .colormap("coolwarm")
    .clim(-1.2, 1.2)
    .block_size(4)
    .title("covariance matrix"));
```

![Expected terminal output from examples/07_covariance_coolwarm.cpp](assets/readme_07_covariance_coolwarm.svg)

Use fixed limits when comparing several arrays. Otherwise `peep` scans finite values and scales each image independently.

### RGB buffers

[`examples/16_ppm_cat.cpp`](examples/16_ppm_cat.cpp) loads a plain-text PPM cat image and passes the RGB bytes directly to `peep`, which interprets it as interleaved RGB by default:

```cpp
peep::show(image.rgb, image.height, image.width, peep::Options()
    .rgb() // peep::RGBLayout::Interleaved
    .block_size(1));
```

![Expected terminal output from examples/16_ppm_cat.cpp](assets/readme_16_ppm_cat.svg)

RGB input is plain `std::uint8_t` data. Interleaved layout means `RGBRGBRGB...`, and planar layout is available with `rgb(peep::RGBLayout::Planar)`.

### Displaying custom types

If your data is not stored as a flat buffer, pass dimensions directly and provide an accessor in `Options`. Scalar accessors return a value that `peep` maps through the selected colormap:

```cpp
peep::show(rows, cols, peep::Options()
    .accessor([&](size_t r, size_t c) {
        return image.value_at(r, c);
    })
    .colormap("turbo"));
```

RGB accessors return a `peep::Color` directly. [`examples/17_rgb_accessor_field.cpp`](examples/17_rgb_accessor_field.cpp) uses this path with a custom `Image` class that stores pixels as YUV and converts them to RGB on demand.

```cpp
peep::show(image.rows(), image.cols(), peep::Options()
    .rgb_accessor([&](size_t r, size_t c) {
        return image.rgb(r, c);
    }));
```

![Expected terminal output from examples/17_rgb_accessor_field.cpp](assets/readme_17_rgb_accessor_field.svg)

## Installation

`peep` is header-only, so direct use is just:

```cpp
#include <peep/peep.hpp>
```

With CMake, install it and link the exported interface target:

```bash
cmake --install build --prefix /path/to/install
```

```cmake
find_package(peep CONFIG REQUIRED)
target_link_libraries(my_target PRIVATE peep::peep)
```

By default, a top-level build compiles the examples but not tests or benchmarks:

```bash
cmake -S . -B build
cmake --build build
```

## Requirements

- C++11
- A terminal with 24-bit truecolor support

## License

MIT
