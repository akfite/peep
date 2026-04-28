# Architecture

`peep` is a header-only C++11 library that turns small 2D arrays into
truecolor ANSI images. The implementation is deliberately modest: the public API
is a handful of overloads and one `Options` object, while the detail layer is a
linear rendering pipeline made of small, testable helpers.

This document is for new contributors who want to understand where a change
belongs and how pixels move through the system.

## Design Goals

- Keep the user-facing API tiny: include `<peep/peep.hpp>`, call `show()` or
  `to_string()`, and configure behavior with chainable `Options`.
- Stay header-only and C++11-compatible. The library uses inline functions,
  templates, function-local statics, and simple standard-library types.
- Treat terminal output as a best-effort debugging view, not a graphics stack.
  The code optimizes for predictable behavior, portability, and readable output.
- Share one rendering path for scalar data, RGB data, pointer input, vector
  input, and accessor input wherever possible.
- Keep platform-specific terminal details out of the public facade.

## Directory Map

```text
include/peep/
  peep.hpp                    public show()/to_string() facade
  options.hpp                 public enums, Color, Options, default colormap API
  colormaps.hpp               generated built-in LUT data

include/peep/detail/
  common.hpp                  shared RGB/Pixel types and scalar type checks
  colormap_registry.hpp       colormap names, lookup table registry, default map
  colormap.hpp                normalized value -> RGB color mapping
  terminal.hpp                terminal size queries and Windows console prep
  region.hpp                  crop/window resolution
  fit.hpp                     terminal fitting, auto block size, RenderPlan
  sampling.hpp                nearest/bilinear sampling helpers
  ansi.hpp                    truecolor ANSI and half-block body emission
  decorators.hpp             title and colorbar rendering
  render.hpp                 top-level render orchestration

examples/                     executable examples and README image sources
tests/                        GoogleTest suite and package smoke test
benchmarks/                   optional benchmark target
scripts/                      helper scripts for assets/generated data
```

The detail headers are included by `peep.hpp` in dependency order. Most detail
headers assume that public types from `options.hpp` are already available.

## Public Surface

The public facade in `peep.hpp` exposes three input styles:

- `show(data, rows, cols, opts)` for raw pointers.
- `show(vector, rows, cols, opts)` for `std::vector`.
- `show(rows, cols, opts)` for accessor-backed data.

`to_string()` mirrors those overloads, but writes to an internal
`std::ostringstream` and returns the ANSI text instead of sending it to the
configured stream.

The facade does only the work that is useful at the API boundary:

- scalar inputs must be `static_cast<double>`-able;
- vectors must have the expected number of elements;
- RGB vector input must contain `std::uint8_t` values;
- accessor options are only valid with the dimension-only overload.

Everything after validation is delegated to `detail::render_*`.

## Options Model

`Options` is intentionally a value object with chainable setters. It stores all
rendering choices: scalar color limits, colormap, special-value colors, layout,
stream, crop, fit mode, resampling mode, source mode, title, and colorbar.

Two private enums are important:

- `SourceMode` records whether the source is scalar data, RGB data, scalar
  accessor, or RGB accessor. Only one mode is active at a time.
- `CropMode` records which crop interpretation is active. Later crop calls
  replace earlier crop calls.

The design is "last setter wins" for mutually exclusive groups. That keeps
chained usage unsurprising:

```cpp
peep::Options()
    .rgb()
    .scalar()
    .colormap("magma");
```

The final source mode above is scalar. The colormap remains relevant because
scalar rendering uses colormaps and RGB rendering does not.

## Rendering Pipeline

At a high level, every call becomes:

```text
public API
  -> validate source shape and mode
  -> adapt input into source_at(row, col)
  -> build RenderPlan from crop + terminal budget + fit options
  -> convert visible source cells into detail::Pixel values
  -> optionally resample pixels to the planned output dimensions
  -> emit ANSI half-block rows
  -> append decorators such as title and colorbar
```

The main orchestrator is `include/peep/detail/render.hpp`.

### 1. Source Adapters

The renderer wants a callable shaped like:

```cpp
value_at(row, col)
```

Pointer and vector input become lambdas over the flat memory buffer. The lambda
honors `Layout::RowMajor` or `Layout::ColMajor`.

Accessor input is already a callable, so the renderer can use the accessor
stored in `Options` directly.

RGB data is similar, except the callable returns `RGB` (`peep::Color`) instead
of a scalar. Flat RGB buffers support two memory layouts:

- `RGBLayout::Interleaved`: `RGBRGBRGB...`
- `RGBLayout::Planar`: all red values, then green, then blue

### 2. Visible Region

Cropping is resolved in `detail/region.hpp`. The renderer works in a visible
coordinate space even when the crop reaches outside the original image.

`VisibleRegion` stores:

- `r0`, `c0`: the visible window start in source coordinates;
- `rows`, `cols`: visible size;
- `empty`: whether there is anything to render.

Centered crops may have negative starts. `resolve_source_index()` converts a
visible coordinate back to a source coordinate and reports whether the source
coordinate exists.

Out-of-bounds behavior is source-specific:

- scalar paths treat missing source cells as NaN;
- RGB paths treat missing source cells as black.

### 3. Render Plan

`detail/fit.hpp` turns visible region size and terminal information into a
`RenderPlan`:

```cpp
struct RenderPlan {
    VisibleRegion visible;
    size_t out_r;
    size_t out_c;
    size_t block_size;
    bool resample;
    bool empty;
};
```

The plan answers: "How many logical rows and columns should be emitted, should
we resample, and how large should each logical pixel be?"

Terminal fitting only engages for real terminal streams. For strings, files,
pipes, and non-standard streams, terminal size is invalid and the behavior falls
back to full-size output.

The fitting policy is:

- `Fit::Off`: render the full visible region.
- `Fit::Trim`: keep identity sampling and emit the top-left area that fits.
- `Fit::Resample`: shrink uniformly to fit, optionally correcting for terminal
  half-block pixel aspect when pixel dimensions are available.

If the user did not set `block_size()`, small images may be automatically
upsampled to make them easier to see. Explicit `block_size()` disables that
automatic upsampling.

### 4. Scalar Color Mapping

Scalar rendering happens in `render_scalar_source()`.

The scalar path first builds `visible_value_at(row, col)`, which applies crop
coordinates and returns NaN for missing cells. Then it resolves color limits:

- `clim(lo, hi)` uses explicit limits;
- if either limit is NaN, that side is auto-scanned from finite values in the
  visible region;
- all-NaN or all-infinite visible regions fall back to `[0, 1]`;
- normalized values clamp to `[0, 1]`.

`detail/colormap.hpp` maps the normalized value through the selected LUT.
Special values are handled before ordinary lookup:

- NaN with no configured `nan_color()` becomes a transparent pixel.
- NaN with `nan_color()` becomes an opaque override color.
- infinities use configured infinity colors when present.
- otherwise `-inf` maps to the low endpoint and `+inf` to the high endpoint.

Non-finite opaque pixels are marked `force_nearest` so bilinear sampling does
not smear NaNs or infinities into neighboring pixels.

### 5. RGB Color Mapping

RGB rendering skips scalar color limits, colormaps, and colorbars. The source
already returns final colors.

The RGB path wraps each source color in an opaque `Pixel`. Missing cells from
out-of-bounds crops become opaque black. Resampling can still use nearest or
bilinear sampling; bilinear RGB sampling blends each channel.

### 6. Sampling

`detail/sampling.hpp` contains small sampling helpers for scalar, RGB, and
`Pixel` callables. The main render path uses `sample_pixel_resampled()` because
by that point scalar and RGB inputs have both been converted into `Pixel`.

Nearest sampling uses integer coordinate mapping.

Bilinear sampling uses center-aligned coordinates:

```text
output cell center -> source coordinate -> four source neighbors -> blend
```

The `Pixel` bilinear path falls back to nearest if any contributing pixel is
transparent or marked `force_nearest`. This preserves important debugging
signals: transparent NaN holes stay holes, and explicitly colored non-finite
values stay localized.

### 7. ANSI Emission

`detail/ansi.hpp` emits terminal image rows using Unicode half-block glyphs:

- U+2584 LOWER HALF BLOCK stores the top pixel as background and bottom pixel
  as foreground.
- U+2580 UPPER HALF BLOCK is used when only the top pixel is present.
- A space is used when both pixels are transparent.

Two source pixel rows become one terminal row. That gives twice the vertical
resolution of ordinary space characters while staying simple and portable.

`AnsiHalfBlockWriter` caches the current foreground and background colors and
only emits ANSI color changes when needed. It resets at row boundaries so color
state does not leak into subsequent terminal text.

`block_size` is applied here by repeating logical pixels into a larger pixel
grid before packing rows into half-block cells.

### 8. Decorators

`detail/decorators.hpp` owns the optional title and scalar colorbar.

The title is a concise summary of the render:

- original data shape;
- crop window when active;
- rendered size when resampled or trimmed;
- colormap or RGB source;
- non-default layout and block size.

Control characters in title text are replaced with spaces.

The colorbar renders below scalar images only. It uses the same colormap and the
resolved scalar limits, so auto-clim output and fixed-clim output are labeled
with the actual values used for the image.

## Terminal Handling

`detail/terminal.hpp` hides platform-specific code.

On POSIX-like systems, terminal size comes from `isatty()` and `ioctl()`.

On Windows, the code maps standard streams to console handles and uses console
APIs for dimensions. Before rendering to a real console, `prepare_terminal_output()`
enables virtual-terminal processing and sets the console output code page to
UTF-8. That matters because the image body contains UTF-8 half-block glyphs and
ANSI truecolor escape sequences.

The terminal helpers only recognize `std::cout`, `std::cerr`, and `std::clog`.
Other streams deliberately report no terminal size. This keeps `to_string()`,
file output, and test streams deterministic.

## Colormaps

Built-in colormap bytes live in `colormaps.hpp`, generated by the helper script.
`detail/colormap_registry.hpp` binds public enum values and string names to
those LUTs.

Important decisions:

- `gray` is generated at runtime instead of stored as static data.
- `grey` is accepted as an alias for `gray`.
- unknown string names fall back to `gray`.
- custom LUTs are copied into `Options` so callers do not need to preserve the
  original buffer lifetime.
- selecting a named or enum colormap clears any previous custom colormap.

The global default colormap is a function-local static exposed through
`default_colormap()` and `set_default_colormap()`. New `Options` instances copy
the current default at construction time.

## Header-Only Constraints

Because `peep` is header-only:

- functions are defined inline or templated;
- no separately compiled source file owns global state;
- generated data must be safe to include from many translation units;
- public headers should avoid pulling platform headers into the user-visible
  facade.

`peep.hpp` includes the detail headers in dependency order so users can include
just one file. Keeping implementation state in `detail` preserves a small public
namespace while still making the implementation testable.

## Testing Strategy

The unit tests focus on behavior rather than visual snapshots:

- option defaults and setter ordering;
- scalar and RGB rendering paths;
- colormap lookup and custom colormaps;
- NaN and infinity handling;
- row-major and column-major layout;
- crop and center crop behavior;
- terminal fitting and auto block size logic;
- sampling kernels and special-value fallback;
- title, colorbar, `to_string()`, vector API, and package smoke tests.

Most rendering tests assert ANSI substrings, line counts, reset behavior, or
equivalence between API paths. This is intentional: full terminal screenshots
would be more fragile than the library warrants.

CI builds examples, tests, benchmarks, installation, and package consumption on
Linux, macOS, and Windows. Coverage runs separately on Linux.

## Adding Features

Use this checklist when deciding where a change belongs:

1. Is it a new user-facing setting? Add it to `Options`, document the default,
   and test setter ordering if it interacts with another setting.
2. Does it change source interpretation? Start in `render_data()`,
   `render_scalar_source()`, `render_rgb_source()`, or the flat-buffer adapters.
3. Does it change crop/window behavior? Start in `region.hpp`.
4. Does it change terminal sizing or overflow behavior? Start in `fit.hpp` and
   add focused tests that use synthetic `TerminalSize` or `ViewportBudget`.
5. Does it change value-to-color behavior? Start in `colormap.hpp` or
   `sampling.hpp`, depending on whether the change happens before or during
   resampling.
6. Does it change emitted ANSI text? Start in `ansi.hpp` or `decorators.hpp`,
   then add tests around reset boundaries and output shape.
7. Does it touch actual terminal APIs? Keep the platform code in
   `terminal.hpp`, and make non-terminal streams deterministic.

Prefer small helpers with explicit coordinate spaces. Names like
`source_value_at`, `visible_value_at`, `visible_pixel_at`, and
`output_pixel_at` are used to show where a callable lives in the pipeline.

## Common Pitfalls

- Do not assume terminal fitting happens for `to_string()`. It does not, because
  `to_string()` writes to an internal stream with no terminal size.
- Do not blend non-finite scalar values during bilinear sampling. They carry
  debugging information and should remain localized.
- Do not let ANSI color state leak. Every emitted row should reset before the
  newline.
- Do not put platform-specific includes in `peep.hpp`.
- Do not use C++17 conveniences; this project targets C++11.
- Do not treat `rows` and `cols` as interchangeable. Public crop APIs use
  `(x, y, w, h)`, while most internal loops use `(row, col)`.

## A Tiny Mental Model

Think of the renderer as a series of coordinate transformations:

```text
original data coordinates
  -> visible crop coordinates
  -> rendered logical pixel coordinates
  -> block-expanded pixel coordinates
  -> terminal half-block character cells
```

Most bugs become easier to place once you know which coordinate space you are
in. The code tries to make that visible in names: `source_*`, `visible_*`,
`out_*`, and `pixel_*` refer to different stages of the same image as it moves
toward the terminal.
