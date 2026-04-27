#include <peep/peep.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

static double clamp01(double v) {
    if (v < 0.0) return 0.0;
    if (v > 1.0) return 1.0;
    return v;
}

static std::uint8_t channel(double v) {
    return static_cast<std::uint8_t>(255.0 * clamp01(v) + 0.5);
}

class Image {
public:
    Image(size_t rows, size_t cols)
        : rows_(rows), cols_(cols), pixels_(rows * cols) {}

    size_t rows() const { return rows_; }
    size_t cols() const { return cols_; }

    void set_rgb(size_t r, size_t c, peep::Color color) {
        const double red = static_cast<double>(color.r) / 255.0;
        const double green = static_cast<double>(color.g) / 255.0;
        const double blue = static_cast<double>(color.b) / 255.0;

        YuvPixel& p = pixels_[index(r, c)];
        p.y = 0.299 * red + 0.587 * green + 0.114 * blue;
        p.u = 0.492 * (blue - p.y);
        p.v = 0.877 * (red - p.y);
    }

    peep::Color rgb(size_t r, size_t c) const {
        const YuvPixel& p = pixels_[index(r, c)];
        return peep::Color{
            channel(p.y + 1.140 * p.v),
            channel(p.y - 0.395 * p.u - 0.581 * p.v),
            channel(p.y + 2.032 * p.u)
        };
    }

private:
    struct YuvPixel {
        double y;
        double u;
        double v;
    };

    size_t index(size_t r, size_t c) const { return r * cols_ + c; }

    size_t rows_;
    size_t cols_;
    std::vector<YuvPixel> pixels_;
};

int main() {
    const size_t rows = 36;
    const size_t cols = 72;
    Image field(rows, cols);

    for (size_t r = 0; r < rows; ++r) {
        const double y = -1.0 + 2.0 * static_cast<double>(r) / static_cast<double>(rows - 1);
        for (size_t c = 0; c < cols; ++c) {
            const double x = -1.0 + 2.0 * static_cast<double>(c) / static_cast<double>(cols - 1);
            const double u = -y + 0.35 * std::sin(5.0 * x);
            const double v = x + 0.35 * std::cos(5.0 * y);
            const double speed = std::sqrt(u * u + v * v);
            field.set_rgb(r, c, peep::Color{
                channel(0.5 + 0.35 * u),
                channel(speed / 1.8),
                channel(0.5 + 0.35 * v)
            });
        }
    }

    peep::show(field.rows(), field.cols(), peep::Options()
        .rgb_accessor([&](size_t r, size_t c) {
            return field.rgb(r, c);
        })
        .title("YUV Image class via RGB accessor"));
}
