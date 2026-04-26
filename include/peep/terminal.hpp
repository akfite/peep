// Platform-specific terminal size query, kept out of the core header so
// peep.hpp stays free of <windows.h>/<sys/ioctl.h>.
#ifndef PEEP_TERMINAL_HPP
#define PEEP_TERMINAL_HPP

#include <cstddef>
#include <iostream>

#if defined(_WIN32)
    #include <io.h>
    #ifndef NOMINMAX
    #define NOMINMAX
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#else
    #include <sys/ioctl.h>
    #include <unistd.h>
#endif

namespace peep {
namespace detail {

struct TerminalSize {
    size_t rows;
    size_t cols;
    size_t width_px;
    size_t height_px;
    bool pixels_valid;
    bool valid;
};

// Map an ostream to a stdio file descriptor, if it is one of the three
// standard streams. Otherwise returns -1 (we have no way to ask "how wide is
// your ostringstream").
inline int stream_fd(const std::ostream& os) {
    if (&os == &std::cout) return 1;
    if (&os == &std::cerr) return 2;
    if (&os == &std::clog) return 2;
    return -1;
}

inline TerminalSize query_terminal_size(const std::ostream& os) {
    TerminalSize ts;
    ts.rows = 0;
    ts.cols = 0;
    ts.width_px = 0;
    ts.height_px = 0;
    ts.pixels_valid = false;
    ts.valid = false;

    const int fd = stream_fd(os);
    if (fd < 0) return ts;

#if defined(_WIN32)
    HANDLE h = reinterpret_cast<HANDLE>(_get_osfhandle(fd));
    if (h == reinterpret_cast<HANDLE>(-1) || h == nullptr) return ts;
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo(h, &info)) return ts;
    int cols = info.srWindow.Right - info.srWindow.Left + 1;
    int rows = info.srWindow.Bottom - info.srWindow.Top + 1;
    if (cols <= 0 || rows <= 0) return ts;
    ts.cols = static_cast<size_t>(cols);
    ts.rows = static_cast<size_t>(rows);
    CONSOLE_FONT_INFOEX font_info;
    font_info.cbSize = sizeof(font_info);
    if (GetCurrentConsoleFontEx(h, FALSE, &font_info)
        && font_info.dwFontSize.X > 0
        && font_info.dwFontSize.Y > 0) {
        ts.width_px = static_cast<size_t>(font_info.dwFontSize.X) * ts.cols;
        ts.height_px = static_cast<size_t>(font_info.dwFontSize.Y) * ts.rows;
        ts.pixels_valid = true;
    }
    ts.valid = true;
#else
    if (!isatty(fd)) return ts;
    struct winsize ws;
    if (ioctl(fd, TIOCGWINSZ, &ws) != 0) return ts;
    if (ws.ws_row == 0 || ws.ws_col == 0) return ts;
    ts.rows = static_cast<size_t>(ws.ws_row);
    ts.cols = static_cast<size_t>(ws.ws_col);
    if (ws.ws_xpixel > 0 && ws.ws_ypixel > 0) {
        ts.width_px = static_cast<size_t>(ws.ws_xpixel);
        ts.height_px = static_cast<size_t>(ws.ws_ypixel);
        ts.pixels_valid = true;
    }
    ts.valid = true;
#endif

    return ts;
}

} // namespace detail
} // namespace peep

#endif // PEEP_TERMINAL_HPP
