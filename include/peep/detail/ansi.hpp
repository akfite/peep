#ifndef PEEP_DETAIL_ANSI_HPP
#define PEEP_DETAIL_ANSI_HPP

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <sstream>
#include <vector>

namespace peep {
namespace detail {

inline void emit_bg(std::ostream& os, RGB c) {
    os << "\x1b[48;2;"
        << static_cast<std::uint16_t>(c.r) << ';'
        << static_cast<std::uint16_t>(c.g) << ';'
        << static_cast<std::uint16_t>(c.b) << 'm';
}

inline void emit_fg(std::ostream& os, RGB c) {
    os << "\x1b[38;2;"
        << static_cast<std::uint16_t>(c.r) << ';'
        << static_cast<std::uint16_t>(c.g) << ';'
        << static_cast<std::uint16_t>(c.b) << 'm';
}

inline void emit_reset(std::ostream& os) { os << "\x1b[0m"; }

// U+2584 LOWER HALF BLOCK (UTF-8: E2 96 84)
inline void emit_lower_half(std::ostream& os) { os << "\xe2\x96\x84"; }

// U+2580 UPPER HALF BLOCK (UTF-8: E2 96 80)
inline void emit_upper_half(std::ostream& os) { os << "\xe2\x96\x80"; }

class AnsiHalfBlockWriter {
public:
    explicit AnsiHalfBlockWriter(std::ostringstream& buf)
        : buf_(buf),
          cur_bg_(RGB{0, 0, 0}),
          cur_fg_(RGB{0, 0, 0}),
          bg_set_(false),
          fg_set_(false) {}

    void emit_pixel(Pixel top, Pixel bot) {
        if (top.transparent && bot.transparent) {
            reset_if_needed();
            buf_ << ' ';
        } else if (top.transparent) {
            reset_if_needed();
            set_fg(bot.color);
            emit_lower_half(buf_);
        } else if (bot.transparent) {
            reset_if_needed();
            set_fg(top.color);
            emit_upper_half(buf_);
        } else {
            set_bg(top.color);
            set_fg(bot.color);
            emit_lower_half(buf_);
        }
    }

    void end_row() {
        reset_if_needed();
        buf_ << '\n';
        bg_set_ = false;
        fg_set_ = false;
    }

private:
    void reset_if_needed() {
        if (bg_set_ || fg_set_) {
            emit_reset(buf_);
            bg_set_ = false;
            fg_set_ = false;
        }
    }

    void set_bg(RGB c) {
        if (!bg_set_ || cur_bg_ != c) {
            emit_bg(buf_, c);
            cur_bg_ = c;
            bg_set_ = true;
        }
    }

    void set_fg(RGB c) {
        if (!fg_set_ || cur_fg_ != c) {
            emit_fg(buf_, c);
            cur_fg_ = c;
            fg_set_ = true;
        }
    }

    std::ostringstream& buf_;
    RGB cur_bg_;
    RGB cur_fg_;
    bool bg_set_;
    bool fg_set_;
};

template <typename PixelAt>
inline void fill_pixel_row(std::vector<Pixel>& row, size_t logical_row,
                           size_t out_c, PixelAt pixel_at) {
    row.resize(out_c);
    for (size_t c = 0; c < out_c; ++c) {
        row[c] = pixel_at(logical_row, c);
    }
}

template <typename PixelAt>
inline void emit_pixel_body(std::ostringstream& buf, size_t out_r, size_t out_c,
                            size_t bs, PixelAt pixel_at) {
    const size_t prows = out_r * bs;
    const size_t pcols = out_c * bs;

    AnsiHalfBlockWriter writer(buf);
    std::vector<Pixel> top_row;
    std::vector<Pixel> bot_row;

    for (size_t pr = 0; pr < prows; pr += 2) {
        const bool has_bot = (pr + 1 < prows);
        const size_t mr_top = pr / bs;
        fill_pixel_row(top_row, mr_top, out_c, pixel_at);

        if (has_bot) {
            const size_t mr_bot = (pr + 1) / bs;
            if (mr_bot == mr_top) {
                bot_row = top_row;
            } else {
                fill_pixel_row(bot_row, mr_bot, out_c, pixel_at);
            }
        }

        for (size_t pc = 0; pc < pcols; ++pc) {
            const size_t mc = pc / bs;
            const Pixel bot = has_bot ? bot_row[mc] : transparent_pixel();
            writer.emit_pixel(top_row[mc], bot);
        }

        writer.end_row();
    }
}

} // namespace detail
} // namespace peep

#endif // PEEP_DETAIL_ANSI_HPP
