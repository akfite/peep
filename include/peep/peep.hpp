#ifndef PEEP_PEEP_HPP
#define PEEP_PEEP_HPP

#include <cstddef>
#include <string>
#include <vector>

#include "peep/options.hpp"

// Rendering implementation. These stay in detail headers so this public
// umbrella remains a small set of user-facing entrypoints.
#include "peep/detail/common.hpp"
#include "peep/detail/colormap.hpp"
#include "peep/detail/terminal.hpp"
#include "peep/detail/region.hpp"
#include "peep/detail/fit.hpp"
#include "peep/detail/sampling.hpp"
#include "peep/detail/ansi.hpp"
#include "peep/detail/decorators.hpp"
#include "peep/detail/render.hpp"

namespace peep {

// numeric pointer
template <typename T>
void print(const T* data,
           size_t rows,
           size_t cols,
           const Options& opts = Options()) {
    detail::require_scalar_castable_to_double<T>();
    detail::render_data(data, rows, cols, opts);
}

// std::vector support
template <typename T>
void print(const std::vector<T>& data,
           size_t rows,
           size_t cols,
           const Options& opts = Options()) {
    detail::require_scalar_castable_to_double<T>();
    detail::validate_data_vector_size<T>(data.size(), rows, cols, opts);
    detail::render_data(data.data(), rows, cols, opts);
}

// custom accessor-backed rendering requires Options so the caller
// can provide either a scalar accessor or an RGB accessor
inline void print(size_t rows, size_t cols, const Options& opts) {
    detail::render_from_options(rows, cols, opts);
}

template <typename T>
std::string to_string(const T* data,
                      size_t rows,
                      size_t cols,
                      const Options& opts = Options()) {
    detail::require_scalar_castable_to_double<T>();
    return detail::render_data_to_string(data, rows, cols, opts);
}

template <typename T>
std::string to_string(const std::vector<T>& data,
                      size_t rows,
                      size_t cols,
                      const Options& opts = Options()) {
    detail::require_scalar_castable_to_double<T>();
    detail::validate_data_vector_size<T>(data.size(), rows, cols, opts);
    return detail::render_data_to_string(data.data(), rows, cols, opts);
}

// Accessor-backed string rendering mirrors print(rows, cols, opts).
inline std::string to_string(size_t rows, size_t cols, const Options& opts) {
    return detail::render_options_to_string(rows, cols, opts);
}

} // namespace peep

#endif // PEEP_PEEP_HPP
