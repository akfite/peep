#ifndef PEEP_DETAIL_COMMON_HPP
#define PEEP_DETAIL_COMMON_HPP

#include <type_traits>
#include <utility>

namespace peep {
namespace detail {

typedef Color RGB;

template <typename T>
class is_static_castable_to_double {
private:
    template <typename U>
    static auto test(int) -> decltype(static_cast<double>(std::declval<const U&>()),
                                      std::true_type());

    template <typename>
    static std::false_type test(...);

public:
    static const bool value = decltype(test<T>(0))::value;
};

template <typename T>
inline void require_scalar_castable_to_double() {
    static_assert(is_static_castable_to_double<T>::value,
                  "peep scalar data type T must support static_cast<double>(value)");
}

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
