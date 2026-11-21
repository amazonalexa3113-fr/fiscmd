#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace fis {

enum class Key {
    Unknown,
    Enter,
    Escape,
    Backspace,
    Delete,
    Tab,
    UpArrow,
    DownArrow,
    LeftArrow,
    RightArrow,
    Home,
    End,
    PageUp,
    PageDown,
    Spacebar,
    Char
};

struct KeyInfo {
    Key key = Key::Unknown;
    char keyChar = '\0';
    bool ctrl = false;
    bool alt = false;
    bool shift = false;
};

struct FileGhostResult {
    int start = 0;
    std::string replacement;
    std::string ghost;
};

// smth RedrawLine() would need
inline int inputStartTop = 0;

// Cross-platform terminal helpers
void ClearScreen();
void SetCursorPosition(int x, int y);
void SetCursorVisible(bool visible);
int GetCursorLeft();
int GetCursorTop();
int GetWindowWidth();
int GetWindowHeight();
int GetBufferWidth();
void SetConsoleTitle(std::string_view title);

bool KeyAvailable();
KeyInfo ReadKey(bool intercept = true);

// Interactive command prompt & line reader
std::string ReturnPrompt();
void PrintPrompt();
std::string ReadCommand();

std::optional<FileGhostResult> GetFileGhost(std::string_view input);

void TabComplete(
    std::string& buffer,
    int& cursor,
    std::string& tabPrefix,
    std::string& tabCurrentSelection,
    std::vector<std::string>& tabMatches,
    int& tabIndex);

void RedrawLine(const std::string& buffer, int cursor, bool showGhost = true);

// String & quote helpers
std::vector<std::string> ParseQuotedArgs(std::string_view input);
bool EqualsIgnoreCase(std::string_view a, std::string_view b);
bool StartsWithIgnoreCase(std::string_view str, std::string_view prefix);
bool EndsWithIgnoreCase(std::string_view str, std::string_view suffix);
std::string ToLower(std::string_view s);
std::string ToUpper(std::string_view s);
std::string Trim(std::string_view s);

} // namespace fis
