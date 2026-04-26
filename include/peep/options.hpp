#ifndef PEEP_OPTIONS_HPP
#define PEEP_OPTIONS_HPP

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "peep/colormaps.hpp"

namespace peep {

typedef std::array<std::uint8_t, 768> ColormapLut;

struct Color {
    std::uint8_t r, g, b;
    bool operator==(const Color& o) const { return r == o.r && g == o.g && b == o.b; }
    bool operator!=(const Color& o) const { return !(*this == o); }
};

typedef std::function<double(size_t, size_t)> ScalarAccessor;
typedef std::function<Color(size_t, size_t)> RGBAccessor;

enum class Layout { RowMajor, ColMajor };
enum class RGBLayout { Interleaved, Planar };
enum class Resampling { Nearest, Bilinear };
enum class Colormap {
    Gray,
    Magma,
    Viridis,
    Plasma,
    Inferno,
    Cividis,
    Coolwarm,
    Gnuplot,
    Turbo
};

// What to do when the rendered image would exceed the terminal window.
//   Off      - render at full size (may wrap or scroll).
//   Resample - downsample to fit the terminal.
//   Trim     - render the top-left portion that fits, discard the rest.
// Only engages when the output stream is a terminal (stdout/stderr + isatty).
// Otherwise behaves as Off, so to_string and piped output are unaffected.
enum class Fit { Off, Resample, Trim };

// Global default colormap used by Options() when no colormap is specified.
Colormap default_colormap();
void set_default_colormap(Colormap c);

struct Options {
private:
    enum class SourceMode {
        ScalarData,
        RGBData,
        ScalarAccessor,
        RGBAccessor
    };

    enum class CropMode {
        None,
        CornerToEnd,
        CornerWindow,
        Center
    };

public:
    Options();

    // Getters
    double clim_lo() const { return clim_lo_; }
    double clim_hi() const { return clim_hi_; }
    Colormap colormap() const { return colormap_; }
    const ColormapLut* custom_colormap() const {
        return custom_cmap_set_ ? &custom_cmap_ : nullptr;
    }
    bool has_custom_colormap() const { return custom_cmap_set_; }
    Color nan_color() const { return nan_color_; }
    bool has_nan_color() const { return nan_color_set_; }
    Color neg_inf_color() const { return neg_inf_color_; }
    bool has_neg_inf_color() const { return neg_inf_color_set_; }
    Color pos_inf_color() const { return pos_inf_color_; }
    bool has_pos_inf_color() const { return pos_inf_color_set_; }
    size_t block_size() const { return block_size_; }
    bool has_block_size() const { return block_size_set_; }
    Layout layout() const { return layout_; }
    std::ostream& ostream() const { return out_.get(); }
    size_t crop_x() const { return crop_x_; }
    size_t crop_y() const { return crop_y_; }
    size_t crop_w() const { return crop_w_; }
    size_t crop_h() const { return crop_h_; }
    bool crop_is_centered() const { return crop_mode_ == CropMode::Center; }
    bool crop_is_to_end() const { return crop_mode_ == CropMode::CornerToEnd; }
    bool crop_is_window() const { return crop_mode_ == CropMode::CornerWindow; }
    size_t crop_center_x() const { return crop_center_x_; }
    size_t crop_center_y() const { return crop_center_y_; }
    Fit fit() const { return fit_; }
    Resampling resampling() const { return resampling_; }
    bool is_rgb() const {
        return source_mode_ == SourceMode::RGBData
            || source_mode_ == SourceMode::RGBAccessor;
    }
    RGBLayout rgb_layout() const { return rgb_layout_; }
    bool has_accessor() const {
        return source_mode_ == SourceMode::ScalarAccessor;
    }
    bool has_rgb_accessor() const {
        return source_mode_ == SourceMode::RGBAccessor;
    }
    const ScalarAccessor& accessor() const { return scalar_accessor_; }
    const RGBAccessor& rgb_accessor() const { return rgb_accessor_; }
    bool show_title() const { return title_show_; }
    const std::string& title_text() const { return title_text_; }
    bool show_colorbar() const { return colorbar_; }

    // Chainable setters
    Options& clim(double lo, double hi) { clim_lo_ = lo; clim_hi_ = hi; return *this; }
    Options& clim_lo(double v) { clim_lo_ = v; return *this; }
    Options& clim_hi(double v) { clim_hi_ = v; return *this; }
    Options& colormap(Colormap c) { colormap_ = c; custom_cmap_set_ = false; return *this; }
    Options& colormap(const std::string& name);
    // Custom colormap: 256 RGB triplets (768 bytes), copied for safe ownership.
    Options& colormap(const ColormapLut& lut) {
        custom_cmap_ = lut;
        custom_cmap_set_ = true;
        return *this;
    }
    Options& colormap(const std::vector<std::uint8_t>& lut) {
        if (lut.size() != custom_cmap_.size()) {
            throw std::invalid_argument("peep custom colormap must contain exactly 768 bytes");
        }
        std::copy(lut.begin(), lut.end(), custom_cmap_.begin());
        custom_cmap_set_ = true;
        return *this;
    }
    Options& colormap(const std::uint8_t (&lut)[768]) {
        std::copy(lut, lut + custom_cmap_.size(), custom_cmap_.begin());
        custom_cmap_set_ = true;
        return *this;
    }
    Options& nan_color(Color c) { nan_color_ = c; nan_color_set_ = true; return *this; }
    Options& nan_color(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
        return nan_color(Color{r, g, b});
    }
    Options& clear_nan_color() { nan_color_set_ = false; return *this; }
    Options& inf_color(Color c) {
        neg_inf_color_ = c;
        pos_inf_color_ = c;
        neg_inf_color_set_ = true;
        pos_inf_color_set_ = true;
        return *this;
    }
    Options& inf_color(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
        return inf_color(Color{r, g, b});
    }
    Options& inf_colors(Color neg, Color pos) {
        neg_inf_color_ = neg;
        pos_inf_color_ = pos;
        neg_inf_color_set_ = true;
        pos_inf_color_set_ = true;
        return *this;
    }
    Options& neg_inf_color(Color c) {
        neg_inf_color_ = c;
        neg_inf_color_set_ = true;
        return *this;
    }
    Options& neg_inf_color(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
        return neg_inf_color(Color{r, g, b});
    }
    Options& pos_inf_color(Color c) {
        pos_inf_color_ = c;
        pos_inf_color_set_ = true;
        return *this;
    }
    Options& pos_inf_color(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
        return pos_inf_color(Color{r, g, b});
    }
    Options& clear_inf_colors() {
        neg_inf_color_set_ = false;
        pos_inf_color_set_ = false;
        return *this;
    }
    Options& block_size(size_t s) {
        block_size_ = (s > 0) ? s : 1;
        block_size_set_ = true;
        return *this;
    }
    Options& layout(Layout l) { layout_ = l; return *this; }
    Options& ostream(std::ostream& os) { out_ = std::ref(os); return *this; }

    // Crop: from (x, y) to end of matrix.
    Options& crop(size_t x, size_t y) {
        crop_mode_ = CropMode::CornerToEnd;
        crop_x_ = x; crop_y_ = y;
        crop_h_ = 0; crop_w_ = 0;
        return *this;
    }
    // Crop: w x h window starting at (x, y).
    Options& crop(size_t x, size_t y, size_t w, size_t h) {
        crop_mode_ = CropMode::CornerWindow;
        crop_x_ = x; crop_y_ = y;
        crop_w_ = w; crop_h_ = h;
        return *this;
    }
    // Crop: w x h window centered on (center_x, center_y), padded outside the matrix.
    Options& center_crop(size_t center_x, size_t center_y, size_t w, size_t h) {
        crop_mode_ = CropMode::Center;
        crop_center_x_ = center_x;
        crop_center_y_ = center_y;
        crop_w_ = w;
        crop_h_ = h;
        crop_x_ = 0;
        crop_y_ = 0;
        return *this;
    }
    Options& fit(Fit f) { fit_ = f; return *this; }
    Options& resampling(Resampling r) { resampling_ = r; return *this; }
    Options& scalar() {
        source_mode_ = SourceMode::ScalarData;
        scalar_accessor_ = ScalarAccessor();
        rgb_accessor_ = RGBAccessor();
        return *this;
    }
    Options& rgb(RGBLayout layout = RGBLayout::Interleaved) {
        source_mode_ = SourceMode::RGBData;
        rgb_layout_ = layout;
        scalar_accessor_ = ScalarAccessor();
        rgb_accessor_ = RGBAccessor();
        return *this;
    }
    template <typename Accessor>
    Options& accessor(Accessor a) {
        source_mode_ = SourceMode::ScalarAccessor;
        scalar_accessor_ = ScalarAccessor(a);
        rgb_accessor_ = RGBAccessor();
        return *this;
    }
    template <typename Accessor>
    Options& rgb_accessor(Accessor a) {
        source_mode_ = SourceMode::RGBAccessor;
        rgb_accessor_ = RGBAccessor(a);
        scalar_accessor_ = ScalarAccessor();
        return *this;
    }
    Options& title(bool enabled = true) { title_show_ = enabled; return *this; }
    Options& title(const std::string& text) {
        title_show_ = true;
        title_text_ = text;
        return *this;
    }
    Options& title(const char* text) {
        title_show_ = true;
        title_text_ = text ? text : "";
        return *this;
    }
    Options& colorbar(bool enabled = true) {
        colorbar_ = enabled;
        return *this;
    }

private:
    double clim_lo_;
    double clim_hi_;
    Colormap colormap_;
    ColormapLut custom_cmap_;
    bool custom_cmap_set_;
    Color nan_color_;
    bool nan_color_set_;
    Color neg_inf_color_;
    bool neg_inf_color_set_;
    Color pos_inf_color_;
    bool pos_inf_color_set_;
    size_t block_size_;
    bool block_size_set_;
    Layout layout_;
    std::reference_wrapper<std::ostream> out_;
    size_t crop_x_;
    size_t crop_y_;
    size_t crop_h_;
    size_t crop_w_;
    CropMode crop_mode_;
    size_t crop_center_x_;
    size_t crop_center_y_;
    Fit fit_;
    Resampling resampling_;
    SourceMode source_mode_;
    RGBLayout rgb_layout_;
    ScalarAccessor scalar_accessor_;
    RGBAccessor rgb_accessor_;
    bool title_show_;
    std::string title_text_;
    bool colorbar_;
};

} // namespace peep

#include "peep/detail/colormap_registry.hpp"

namespace peep {

inline Colormap default_colormap() { return detail::default_colormap_ref(); }
inline void set_default_colormap(Colormap c) { detail::default_colormap_ref() = c; }

inline Options::Options()
    : clim_lo_(std::numeric_limits<double>::quiet_NaN()),
      clim_hi_(std::numeric_limits<double>::quiet_NaN()),
      colormap_(default_colormap()),
      custom_cmap_(),
      custom_cmap_set_(false),
      nan_color_(),
      nan_color_set_(false),
      neg_inf_color_(),
      neg_inf_color_set_(false),
      pos_inf_color_(),
      pos_inf_color_set_(false),
      block_size_(1),
      block_size_set_(false),
      layout_(Layout::RowMajor),
      out_(std::cout),
      crop_x_(0),
      crop_y_(0),
      crop_h_(0),
      crop_w_(0),
      crop_mode_(CropMode::None),
      crop_center_x_(0),
      crop_center_y_(0),
      fit_(Fit::Resample),
      resampling_(Resampling::Bilinear),
      source_mode_(SourceMode::ScalarData),
      rgb_layout_(RGBLayout::Interleaved),
      scalar_accessor_(),
      rgb_accessor_(),
      title_show_(false),
      title_text_(),
      colorbar_(true) {}

inline Options& Options::colormap(const std::string& name) {
    custom_cmap_set_ = false;
    colormap_ = detail::colormap_from_string(name);
    return *this;
}

} // namespace peep

#endif // PEEP_OPTIONS_HPP
