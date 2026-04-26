#ifndef PEEP_DETAIL_COLORMAP_REGISTRY_HPP
#define PEEP_DETAIL_COLORMAP_REGISTRY_HPP

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <string>

namespace peep {
namespace detail {

inline Colormap& default_colormap_ref() {
    static Colormap cmap = Colormap::Gray;
    return cmap;
}

inline std::string str_tolower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return out;
}

// Computed grayscale colormap (avoids embedding trivial data)
inline ColormapLut make_gray_lut() {
    ColormapLut lut;
    for (size_t i = 0; i < 256; ++i) {
        std::uint8_t v = static_cast<std::uint8_t>(i);
        lut[i * 3 + 0] = v;
        lut[i * 3 + 1] = v;
        lut[i * 3 + 2] = v;
    }
    return lut;
}

inline const ColormapLut& cmap_gray() {
    static const ColormapLut lut = make_gray_lut();
    return lut;
}

struct ColormapEntry {
    Colormap id;
    std::string name;
    const ColormapLut* lut;
};

inline const ColormapEntry* colormap_entries(size_t& count) {
    static const ColormapEntry entries[] = {
        {Colormap::Gray,     "gray",     &cmap_gray()},
        {Colormap::Magma,    "magma",    &CMAP_MAGMA},
        {Colormap::Viridis,  "viridis",  &CMAP_VIRIDIS},
        {Colormap::Plasma,   "plasma",   &CMAP_PLASMA},
        {Colormap::Inferno,  "inferno",  &CMAP_INFERNO},
        {Colormap::Cividis,  "cividis",  &CMAP_CIVIDIS},
        {Colormap::Coolwarm, "coolwarm", &CMAP_COOLWARM},
        {Colormap::Gnuplot,  "gnuplot",  &CMAP_GNUPLOT},
        {Colormap::Turbo,    "turbo",    &CMAP_TURBO}
    };
    count = sizeof(entries) / sizeof(entries[0]);
    return entries;
}

inline Colormap colormap_from_string(const std::string& name) {
    std::string lower = str_tolower(name);
    if (lower == "grey") lower = "gray";

    size_t count = 0;
    const ColormapEntry* entries = colormap_entries(count);
    for (size_t i = 0; i < count; ++i) {
        if (lower == entries[i].name) return entries[i].id;
    }
    return Colormap::Gray;
}

inline const ColormapLut& find_colormap(Colormap cmap) {
    size_t count = 0;
    const ColormapEntry* entries = colormap_entries(count);
    for (size_t i = 0; i < count; ++i) {
        if (entries[i].id == cmap) return *entries[i].lut;
    }
    return cmap_gray();
}

inline const ColormapLut& find_colormap(const std::string& name) {
    return find_colormap(colormap_from_string(name));
}

inline std::string colormap_name(Colormap cmap) {
    size_t count = 0;
    const ColormapEntry* entries = colormap_entries(count);
    for (size_t i = 0; i < count; ++i) {
        if (entries[i].id == cmap) return entries[i].name;
    }
    return "gray";
}

} // namespace detail
} // namespace peep

#endif // PEEP_DETAIL_COLORMAP_REGISTRY_HPP
