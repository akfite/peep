// Platform-specific terminal size query, kept out of the core header so
// termimage.h stays free of <windows.h>/<sys/ioctl.h>.
#ifndef TERMIMAGE_TERMINAL_H
#define TERMIMAGE_TERMINAL_H

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

namespace termimage {
namespace detail {

struct TerminalSize {
    size_t rows;
    size_t cols;
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
    ts.valid = true;
#else
    if (!isatty(fd)) return ts;
    struct winsize ws;
    if (ioctl(fd, TIOCGWINSZ, &ws) != 0) return ts;
    if (ws.ws_row == 0 || ws.ws_col == 0) return ts;
    ts.rows = static_cast<size_t>(ws.ws_row);
    ts.cols = static_cast<size_t>(ws.ws_col);
    ts.valid = true;
#endif

    return ts;
}

} // namespace detail
} // namespace termimage

#endif // TERMIMAGE_TERMINAL_H
