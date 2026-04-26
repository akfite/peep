#ifndef PEEP_DETAIL_COMMON_HPP
#define PEEP_DETAIL_COMMON_HPP

namespace peep {
namespace detail {

typedef Color RGB;

struct Pixel {
    RGB color;
    bool transparent;
    bool force_nearest;
};

inline Pixel transparent_pixel() {
    Pixel p;
    p.color = RGB{0, 0, 0};
    p.transparent = true;
    p.force_nearest = false;
    return p;
}

inline Pixel opaque_pixel(RGB c, bool force_nearest = false) {
    Pixel p;
    p.color = c;
    p.transparent = false;
    p.force_nearest = force_nearest;
    return p;
}

} // namespace detail
} // namespace peep

#endif // PEEP_DETAIL_COMMON_HPP
