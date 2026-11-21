#include "SetColor.hpp"
#include "Program.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace fis {

namespace {
#ifdef _WIN32
WORD ToWinColor(ConsoleColor fg, ConsoleColor bg) {
    return static_cast<WORD>(static_cast<int>(fg) | (static_cast<int>(bg) << 4));
}
#else
const char* ToAnsiFg(ConsoleColor c) {
    switch (c) {
        case ConsoleColor::Black:       return "\x1b[30m";
        case ConsoleColor::DarkRed:     return "\x1b[31m";
        case ConsoleColor::DarkGreen:   return "\x1b[32m";
        case ConsoleColor::DarkYellow:  return "\x1b[33m";
        case ConsoleColor::DarkBlue:    return "\x1b[34m";
        case ConsoleColor::DarkMagenta: return "\x1b[35m";
        case ConsoleColor::DarkCyan:    return "\x1b[36m";
        case ConsoleColor::Gray:        return "\x1b[37m";
        case ConsoleColor::DarkGray:    return "\x1b[90m";
        case ConsoleColor::Red:         return "\x1b[91m";
        case ConsoleColor::Green:       return "\x1b[92m";
        case ConsoleColor::Yellow:      return "\x1b[93m";
        case ConsoleColor::Blue:        return "\x1b[94m";
        case ConsoleColor::Magenta:     return "\x1b[95m";
        case ConsoleColor::Cyan:        return "\x1b[96m";
        case ConsoleColor::White:       return "\x1b[97m";
    }
    return "\x1b[37m";
}

const char* ToAnsiBg(ConsoleColor c) {
    switch (c) {
        case ConsoleColor::Black:       return "\x1b[40m";
        case ConsoleColor::DarkRed:     return "\x1b[41m";
        case ConsoleColor::DarkGreen:   return "\x1b[42m";
        case ConsoleColor::DarkYellow:  return "\x1b[43m";
        case ConsoleColor::DarkBlue:    return "\x1b[44m";
        case ConsoleColor::DarkMagenta: return "\x1b[45m";
        case ConsoleColor::DarkCyan:    return "\x1b[46m";
        case ConsoleColor::Gray:        return "\x1b[47m";
        case ConsoleColor::DarkGray:    return "\x1b[100m";
        case ConsoleColor::Red:         return "\x1b[101m";
        case ConsoleColor::Green:       return "\x1b[102m";
        case ConsoleColor::Yellow:      return "\x1b[103m";
        case ConsoleColor::Blue:        return "\x1b[104m";
        case ConsoleColor::Magenta:     return "\x1b[105m";
        case ConsoleColor::Cyan:        return "\x1b[106m";
        case ConsoleColor::White:       return "\x1b[107m";
    }
    return "\x1b[40m";
}
#endif

ConsoleColor activeFg = ConsoleColor::Gray;
ConsoleColor activeBg = ConsoleColor::Black;

void ApplyActiveColors() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        SetConsoleTextAttribute(hOut, ToWinColor(activeFg, activeBg));
    }
#else
    std::cout << ToAnsiFg(activeFg) << ToAnsiBg(activeBg) << std::flush;
#endif
}
} // namespace

void InitConsoleColors() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

void SetColor(ConsoleColor whatcolor, bool setforbackground) {
    if (nocolormode)
        return;

    if (setforbackground)
        activeBg = whatcolor;
    else
        activeFg = whatcolor;

    ApplyActiveColors();
}

void ResForegroundColor() {
    if (nocolormode) {
        ResetColor();
        return;
    }
    activeFg = currentFg;
    ApplyActiveColors();
}

void ResetColor() {
    activeFg = ConsoleColor::Gray;
    activeBg = ConsoleColor::Black;
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        SetConsoleTextAttribute(hOut, ToWinColor(activeFg, activeBg));
    }
#else
    std::cout << "\x1b[0m" << std::flush;
#endif
}

void TrueColorType(std::string_view whatoprint, int r, int g, int b) {
    if (nocolormode) {
        std::cout << whatoprint;
        return;
    }

    if (truecolor)
        std::cout << "\x1b[38;2;" << r << ";" << g << ";" << b << "m";

    std::cout << whatoprint;

    if (truecolor)
        std::cout << "\x1b[0m";
}

void TypeWrite(std::string_view input, int delay, bool newline) {
    for (char c : input) {
        std::cout << c << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    }
    if (newline) {
        std::cout << '\n';
    }
}

void PrintFisCoolAsf(bool type, int delay, bool newline) {
    if (type) {
        SetColor(ConsoleColor::DarkBlue);
        std::cout << ">" << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        SetColor(ConsoleColor::DarkCyan);
        std::cout << "<" << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        SetColor(ConsoleColor::Cyan);
        std::cout << ">" << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));

        if (newline) {
            std::cout << '\n';
        }
    } else {
        SetColor(ConsoleColor::Cyan);
        std::cout << ">";
        SetColor(ConsoleColor::DarkCyan);
        std::cout << "<";
        SetColor(ConsoleColor::Cyan);
        std::cout << ">";
    }
}

void AnimateFisCoolAsf(bool newline) {
    const std::string text = "><> fiscmd initialized, welcome :D";

    // if colors are disabled, don't do ANSI animation.
    if (nocolormode) {
        std::cout << text;
        if (newline)
            std::cout << '\n';
        return;
    }

#ifdef _WIN32
    constexpr bool isWindows = true;
#else
    constexpr bool isWindows = false;
#endif

    const int frames = 18;
    const double pi = 3.14159265358979323846;

    // clear the current line before every frame
    for (int frame = 0; frame <= frames; frame++) {
        double progress = static_cast<double>(frame) / frames;

        std::cout << "\r\x1b[2K";

        for (size_t i = 0; i < text.size(); i++) {
            double position = static_cast<double>(i) / (text.size() - 1);

            // smooth white reveal gradient
            double distance = progress - position;
            double brightness = std::clamp((distance + 0.15) / 0.30, 0.0, 1.0);

            int r = static_cast<int>(255 * brightness);
            int g = static_cast<int>(255 * brightness);
            int b = static_cast<int>(255 * brightness);

            if (truecolor && !isWindows) {
                std::cout << "\x1b[38;2;" << r << ";" << g << ";" << b << "m";
            } else {
                // approximate the RGB brightness with console colors
                if (brightness < 0.25)
                    SetColor(ConsoleColor::Black);
                else if (brightness < 0.5)
                    SetColor(ConsoleColor::DarkGray);
                else if (brightness < 0.75)
                    SetColor(ConsoleColor::Gray);
                else
                    SetColor(ConsoleColor::White);
            }

            std::cout << text[i];
        }

        // reset ANSI color
        if (truecolor) {
            std::cout << "\x1b[0m";
        } else {
            ResForegroundColor();
        }
        std::cout << std::flush;

        std::this_thread::sleep_for(std::chrono::milliseconds(12));
    }

    // slow cyan -> dark cyan -> blue sweep
    const int colorFrames = 60;
    const int colorFrameDelay = 1000 / colorFrames;

    for (int frame = 0; frame <= colorFrames; frame++) {
        double linearProgress = static_cast<double>(frame) / colorFrames;

        // sine out easing
        double progress = std::sin(linearProgress * pi / 2.0);

        std::cout << "\r\x1b[2K";

        for (size_t i = 0; i < text.size(); i++) {
            double position = static_cast<double>(i) / (text.size() - 1);

            // calculate the position inside the cyan -> dark cyan -> blue gradient
            double sweep = progress - position;

            // smooth the edge of the color sweep
            double blend = std::clamp((sweep + 0.12) / 0.24, 0.0, 1.0);

            int targetR = 0;
            int targetG = 0;
            int targetB = 0;

            if (position < 0.5) {
                // cyan -> darker cyan
                double p = position / 0.5;
                targetR = 0;
                targetG = static_cast<int>(255 - (75 * p));
                targetB = static_cast<int>(255 - (35 * p));
            } else {
                // darker cyan -> blue
                double p = (position - 0.5) / 0.5;
                targetR = 0;
                targetG = static_cast<int>(180 - (100 * p));
                targetB = static_cast<int>(220 + (35 * p));
            }

            // blend white into the gradient
            int r = static_cast<int>(255 + ((targetR - 255) * blend));
            int g = static_cast<int>(255 + ((targetG - 255) * blend));
            int b = static_cast<int>(255 + ((targetB - 255) * blend));

            if (truecolor && !isWindows) {
                std::cout << "\x1b[38;2;" << r << ";" << g << ";" << b << "m";
            } else {
                if (blend < 0.25)
                    SetColor(ConsoleColor::White);
                else if (position < 0.5)
                    SetColor(ConsoleColor::Cyan);
                else if (position < 0.75)
                    SetColor(ConsoleColor::DarkCyan);
                else
                    SetColor(ConsoleColor::Blue);
            }

            std::cout << text[i];
        }

        if (truecolor) {
            std::cout << "\x1b[0m";
        } else {
            ResForegroundColor();
        }
        std::cout << std::flush;

        std::this_thread::sleep_for(std::chrono::milliseconds(colorFrameDelay));
    }

    if (newline)
        std::cout << '\n';
}

void SetColorTerminal(std::string_view input) {
    if (nocolormode) {
        std::cout << "non-color mode is on, turn it off first >:c\n";
        return;
    }

    std::istringstream iss{std::string(input)};
    std::vector<std::string> parts;
    std::string token;
    while (iss >> token) {
        parts.push_back(token);
    }

    if (parts.size() == 1) {
        SetColor(ConsoleColor::Cyan);
        std::cout << "usage: color <fg> [bg] or color default\n\n";
        std::cout << "<fg> is foreground\n";
        std::cout << "[bg] is background\n\n";
        std::cout << "both <fg> and [bg] can be between 0 and 15\n";

        SetColor(ConsoleColor::Red);
        std::cout << "\nuse it wisely, u do NOT wanna be blinded by the texts and\n";
        std::cout << "the foreground color being the same\n";

        SetColor(ConsoleColor::Yellow);
        std::cout << "\nalso i recommend entering \"cls\" after setting the color\n";

        SetColor(ConsoleColor::Red);
        std::cout << "(because background color may NOT apply the whole thing without cls)\n";
        std::cout << "(also foreground color wouldnt be applying to everything, just the prompt u type bru)\n";

        ResForegroundColor();
        return;
    }

    std::string arg1Lower = parts[1];
    std::transform(arg1Lower.begin(), arg1Lower.end(), arg1Lower.begin(), ::tolower);

    if (parts.size() > 1 && arg1Lower == "default") {
        currentFg = ConsoleColor::Gray;
        currentBg = ConsoleColor::Black;
        ResForegroundColor();
        return;
    }

    // validate fg first (no committing yet)
    int fg = -1;
    try {
        size_t idx = 0;
        fg = std::stoi(parts[1], &idx);
        if (idx != parts[1].size() || fg < 0 || fg > 15) {
            throw std::invalid_argument("range");
        }
    } catch (...) {
        SetColor(ConsoleColor::Red);
        std::cout << "invalid color :c\n";
        ResForegroundColor();
        return;
    }

    ConsoleColor newFg = static_cast<ConsoleColor>(fg);
    ConsoleColor newBg = currentBg;

    // optional bg
    if (parts.size() > 2) {
        int bg = -1;
        try {
            size_t idx = 0;
            bg = std::stoi(parts[2], &idx);
            if (idx != parts[2].size() || bg < 0 || bg > 15) {
                throw std::invalid_argument("range");
            }
        } catch (...) {
            SetColor(ConsoleColor::Red);
            std::cout << "invalid background color :c\n";
            ResForegroundColor();
            return;
        }
        newBg = static_cast<ConsoleColor>(bg);
    }

    // same color detection BEFORE applying anything
    if (newFg == newBg) {
        SetColor(ConsoleColor::Red);
        std::cout << "dawg u set both foreground and background same :skull:\n";
        std::cout << "still setting color?? (y/n)\n";

        if (showDir) {
            SetColor(ConsoleColor::Cyan);
            std::cout << "><[answer]> " << std::flush;
        } else {
            ResForegroundColor();
        }

        std::string confirm;
        std::getline(std::cin, confirm);
        std::transform(confirm.begin(), confirm.end(), confirm.begin(), ::tolower);

        if (confirm != "y") {
            std::cout << "uh ok good :sob::thumbs-up:\n";
            ResForegroundColor();
            return;
        }

        std::cout << "good luck\n";
    }

    // commit changes AFTER checks
    currentFg = newFg;
    currentBg = newBg;

    ResForegroundColor();
}

} // namespace fis
