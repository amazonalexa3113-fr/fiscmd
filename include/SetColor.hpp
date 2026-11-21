#pragma once

#include <string>
#include <string_view>

namespace fis {

enum class ConsoleColor : int {
    Black = 0,
    DarkBlue = 1,
    DarkGreen = 2,
    DarkCyan = 3,
    DarkRed = 4,
    DarkMagenta = 5,
    DarkYellow = 6,
    Gray = 7,
    DarkGray = 8,
    Blue = 9,
    Green = 10,
    Cyan = 11,
    Red = 12,
    Magenta = 13,
    Yellow = 14,
    White = 15
};

// true color mode yes or no
inline bool truecolor = false;

// accept color mode or not
inline bool nocolormode = false;

// current fore/background
inline ConsoleColor currentFg = ConsoleColor::Green;
inline ConsoleColor currentBg = ConsoleColor::Black;

// Initialize Windows Virtual Terminal Processing if needed
void InitConsoleColors();

// hohoho... what's that?
// if setforbackground is true, SetColor() will set the color of the background instead of foreground
void SetColor(ConsoleColor whatcolor, bool setforbackground = false);

// only applies to the ><>$ thingy (not whole stuff)
void ResForegroundColor();

// Reset terminal colors completely
void ResetColor();

// peak 16 million color true color mode
void TrueColorType(std::string_view whatoprint, int r, int g, int b);

// useless typewriting function only used in ShowUpdate() command :sob::pray:
void TypeWrite(std::string_view input, int delay = 10, bool newline = true);

// "useless" print the fish ascii
void PrintFisCoolAsf(bool type, int delay = 10, bool newline = true);

// truecolor animated fish welcome banner
void AnimateFisCoolAsf(bool newline = true);

// renamed due to another one "SetColor" exists
void SetColorTerminal(std::string_view input);

} // namespace fis
