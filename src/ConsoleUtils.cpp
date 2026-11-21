#include "ConsoleUtils.hpp"
#include "ProcessUtils.hpp"
#include "Program.hpp"
#include "SetColor.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <regex>
#include <cstdlib>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <conio.h>
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace fis {

namespace {
#ifndef _WIN32
class RawTerminalGuard {
public:
    RawTerminalGuard() {
        if (isatty(STDIN_FILENO)) {
            tcgetattr(STDIN_FILENO, &orig_);
            termios raw = orig_;
            raw.c_lflag &= ~(ICANON | ECHO);
            raw.c_cc[VMIN] = 1;
            raw.c_cc[VTIME] = 0;
            tcsetattr(STDIN_FILENO, TCSANOW, &raw);
            active_ = true;
        }
    }
    ~RawTerminalGuard() {
        if (active_) {
            tcsetattr(STDIN_FILENO, TCSANOW, &orig_);
        }
    }
private:
    termios orig_{};
    bool active_ = false;
};

bool StdinReady(int timeoutMs) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    timeval tv{};
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;
    return select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) > 0;
}
#endif
} // namespace

void ClearScreen() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hOut, &csbi)) return;
    DWORD cellCount = csbi.dwSize.X * csbi.dwSize.Y;
    DWORD count;
    COORD homeCoords = {0, 0};
    FillConsoleOutputCharacterA(hOut, ' ', cellCount, homeCoords, &count);
    FillConsoleOutputAttribute(hOut, csbi.wAttributes, cellCount, homeCoords, &count);
    SetConsoleCursorPosition(hOut, homeCoords);
#else
    std::cout << "\x1b[2J\x1b[H" << std::flush;
#endif
}

void SetCursorPosition(int x, int y) {
    x = std::max(0, x);
    y = std::max(0, y);
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        COORD pos = {static_cast<SHORT>(x), static_cast<SHORT>(y)};
        SetConsoleCursorPosition(hOut, pos);
    }
#else
    std::cout << "\x1b[" << (y + 1) << ";" << (x + 1) << "H" << std::flush;
#endif
}

void SetCursorVisible(bool visible) {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        CONSOLE_CURSOR_INFO cursorInfo;
        if (GetConsoleCursorInfo(hOut, &cursorInfo)) {
            cursorInfo.bVisible = visible ? TRUE : FALSE;
            SetConsoleCursorInfo(hOut, &cursorInfo);
        }
    }
#else
    std::cout << (visible ? "\x1b[?25h" : "\x1b[?25l") << std::flush;
#endif
}

int GetCursorLeft() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (hOut != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(hOut, &csbi)) {
        return csbi.dwCursorPosition.X;
    }
    return 0;
#else
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) return 0;
    RawTerminalGuard guard;
    std::cout << "\x1b[6n" << std::flush;
    if (!StdinReady(50)) return 0;
    std::string resp;
    char ch;
    while (StdinReady(20) && read(STDIN_FILENO, &ch, 1) == 1) {
        resp += ch;
        if (ch == 'R') break;
    }
    size_t semi = resp.find(';');
    size_t rPos = resp.find('R');
    if (semi != std::string::npos && rPos != std::string::npos && rPos > semi) {
        try {
            return std::max(0, std::stoi(resp.substr(semi + 1, rPos - semi - 1)) - 1);
        } catch (...) {}
    }
    return 0;
#endif
}

int GetCursorTop() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (hOut != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(hOut, &csbi)) {
        return csbi.dwCursorPosition.Y;
    }
    return 0;
#else
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) return 0;
    RawTerminalGuard guard;
    std::cout << "\x1b[6n" << std::flush;
    if (!StdinReady(50)) return 0;
    std::string resp;
    char ch;
    while (StdinReady(20) && read(STDIN_FILENO, &ch, 1) == 1) {
        resp += ch;
        if (ch == 'R') break;
    }
    size_t bracket = resp.find('[');
    size_t semi = resp.find(';');
    if (bracket != std::string::npos && semi != std::string::npos && semi > bracket) {
        try {
            return std::max(0, std::stoi(resp.substr(bracket + 1, semi - bracket - 1)) - 1);
        } catch (...) {}
    }
    return 0;
#endif
}

int GetWindowWidth() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (hOut != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(hOut, &csbi)) {
        return std::max(1, csbi.srWindow.Right - csbi.srWindow.Left + 1);
    }
    return 80;
#else
    winsize w{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0) {
        return w.ws_col;
    }
    return 80;
#endif
}

int GetWindowHeight() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (hOut != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(hOut, &csbi)) {
        return std::max(1, csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
    }
    return 25;
#else
    winsize w{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_row > 0) {
        return w.ws_row;
    }
    return 25;
#endif
}

int GetBufferWidth() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (hOut != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(hOut, &csbi)) {
        return std::max(1, static_cast<int>(csbi.dwSize.X));
    }
    return 80;
#else
    return GetWindowWidth();
#endif
}

void SetConsoleTitle(std::string_view title) {
#ifdef _WIN32
    SetConsoleTitleA(std::string(title).c_str());
#else
    std::cout << "\x1b]0;" << title << "\x07" << std::flush;
#endif
}

bool KeyAvailable() {
#ifdef _WIN32
    return _kbhit() != 0;
#else
    if (!isatty(STDIN_FILENO)) return false;
    RawTerminalGuard guard;
    return StdinReady(0);
#endif
}

KeyInfo ReadKey(bool intercept) {
    KeyInfo info;
#ifdef _WIN32
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    INPUT_RECORD record;
    DWORD eventsRead;
    while (ReadConsoleInputA(hIn, &record, 1, &eventsRead)) {
        if (record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown) {
            const auto& ke = record.Event.KeyEvent;
            DWORD ctrlState = ke.dwControlKeyState;
            info.ctrl = (ctrlState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0;
            info.alt = (ctrlState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) != 0;
            info.shift = (ctrlState & SHIFT_PRESSED) != 0;
            info.keyChar = ke.uChar.AsciiChar;

            switch (ke.wVirtualKeyCode) {
                case VK_RETURN: info.key = Key::Enter; break;
                case VK_ESCAPE: info.key = Key::Escape; break;
                case VK_BACK:   info.key = Key::Backspace; break;
                case VK_DELETE: info.key = Key::Delete; break;
                case VK_TAB:    info.key = Key::Tab; break;
                case VK_UP:     info.key = Key::UpArrow; break;
                case VK_DOWN:   info.key = Key::DownArrow; break;
                case VK_LEFT:   info.key = Key::LeftArrow; break;
                case VK_RIGHT:  info.key = Key::RightArrow; break;
                case VK_HOME:   info.key = Key::Home; break;
                case VK_END:    info.key = Key::End; break;
                case VK_PRIOR:  info.key = Key::PageUp; break;
                case VK_NEXT:   info.key = Key::PageDown; break;
                case VK_SPACE:  info.key = Key::Spacebar; break;
                default:
                    if (info.keyChar != 0) {
                        info.key = Key::Char;
                    } else {
                        continue; // modifier-only key press
                    }
                    break;
            }
            if (!intercept && info.keyChar >= 32) {
                std::cout << info.keyChar << std::flush;
            }
            return info;
        }
    }
    return info;
#else
    RawTerminalGuard guard;
    unsigned char ch = 0;
    if (read(STDIN_FILENO, &ch, 1) <= 0) {
        return info;
    }

    if (ch == '\r' || ch == '\n') {
        info.key = Key::Enter;
        info.keyChar = '\r';
    } else if (ch == 127 || ch == 8) {
        info.key = Key::Backspace;
        info.keyChar = '\b';
    } else if (ch == '\t') {
        info.key = Key::Tab;
        info.keyChar = '\t';
    } else if (ch == 12) { // Ctrl+L
        info.key = Key::Char;
        info.keyChar = 'l';
        info.ctrl = true;
    } else if (ch == 19) { // Ctrl+S
        info.key = Key::Char;
        info.keyChar = 's';
        info.ctrl = true;
    } else if (ch == 24) { // Ctrl+X
        info.key = Key::Char;
        info.keyChar = 'x';
        info.ctrl = true;
    } else if (ch == 27) { // ESC or escape sequence
        if (!StdinReady(25)) {
            info.key = Key::Escape;
            info.keyChar = 27;
        } else {
            unsigned char seq1 = 0;
            if (read(STDIN_FILENO, &seq1, 1) == 1 && seq1 == '[') {
                std::string seq;
                unsigned char c = 0;
                while (StdinReady(25) && read(STDIN_FILENO, &c, 1) == 1) {
                    seq += static_cast<char>(c);
                    if ((c >= 'A' && c <= 'Z') || c == '~') break;
                }
                if (seq == "A") info.key = Key::UpArrow;
                else if (seq == "B") info.key = Key::DownArrow;
                else if (seq == "C") info.key = Key::RightArrow;
                else if (seq == "D") info.key = Key::LeftArrow;
                else if (seq == "1;5C") { info.key = Key::RightArrow; info.ctrl = true; }
                else if (seq == "1;5D") { info.key = Key::LeftArrow; info.ctrl = true; }
                else if (seq == "H" || seq == "1~" || seq == "7~") info.key = Key::Home;
                else if (seq == "F" || seq == "4~" || seq == "8~") info.key = Key::End;
                else if (seq == "3~") info.key = Key::Delete;
                else if (seq == "5~") info.key = Key::PageUp;
                else if (seq == "6~") info.key = Key::PageDown;
            } else {
                info.key = Key::Escape;
            }
        }
    } else if (ch == ' ') {
        info.key = Key::Spacebar;
        info.keyChar = ' ';
    } else if (ch >= 1 && ch <= 26) {
        info.key = Key::Char;
        info.keyChar = static_cast<char>('a' + ch - 1);
        info.ctrl = true;
    } else {
        info.key = Key::Char;
        info.keyChar = static_cast<char>(ch);
    }

    if (!intercept && info.keyChar >= 32) {
        std::cout << info.keyChar << std::flush;
    }
    return info;
#endif
}

std::string ReturnPrompt() {
    std::error_code ec;
    if (!fs::exists(currentDir, ec) || !showDir)
        return "><>$ ";

    std::string normalized = fs::weakly_canonical(currentDir, ec).string();
    if (ec) normalized = currentDir;

    while (normalized.size() > 1 && (normalized.back() == '\\' || normalized.back() == '/')) {
#ifdef _WIN32
        if (normalized.size() == 3 && normalized[1] == ':') break;
#endif
        normalized.pop_back();
    }

#ifdef _WIN32
    if (normalized.size() == 3 && normalized[1] == ':' && normalized[2] == '\\')
        normalized.pop_back();
#endif

    std::string user = "unknown";
    
    #ifdef _WIN32
    char username[256];
    DWORD usernameSize = sizeof(username);
    
    if (GetUserNameA(username, &usernameSize))
        user = username;
    #else
    const char* username = std::getenv("USER");
    
    if (username)
        user = username;
    #endif
    
    if (EqualsIgnoreCase(normalized, "C:") || normalized == "/")
        return "><[main os partition]>$ ";
    if (EqualsIgnoreCase(normalized, "C:\\Windows"))
        return "><[important windows folder]>$ ";
    if (EqualsIgnoreCase(normalized, "C:\\Windows\\System32"))
        return "><[important admin folder]>$ ";
    if (EqualsIgnoreCase(normalized, "/root"))
        return "><[important root folder]>$ ";
    if (EqualsIgnoreCase(normalized, "C:\\Users\\" + user) || EqualsIgnoreCase(normalized, "/home/" + user))
        return "><[ur user folder, " + user + "]>$ ";
    if (EqualsIgnoreCase(normalized, "C:\\Users") || EqualsIgnoreCase(normalized, "/home"))
        return "><[user folder]>$ ";

    return "><[" + normalized + "]>$ ";
}

void PrintPrompt() {
    std::cout << ReturnPrompt() << std::flush;
}

std::string ReadCommand() {
    if (!nocolormode) {
        SetColor(currentFg);
        SetColor(currentBg, true);
    }

    SetCursorVisible(true);

    std::string buffer;
    int cursor = 0;

    std::string tabPrefix;
    std::string tabCurrentSelection;
    std::vector<std::string> tabMatches;
    int tabIndex = -1;

    inputStartTop = GetCursorTop();

    if (!nocolormode) {
        SetColor(currentFg);
        SetColor(currentBg, true);
    }

    while (true) {
        KeyInfo key = ReadKey(true);

        // enter
        if (key.key == Key::Enter) {
            std::string result = buffer;
            cursor = static_cast<int>(buffer.size());

            // redraw without ghost text before submitting
            RedrawLine(buffer, cursor, false);

            ResForegroundColor();
            SetColor(currentBg, true);

            std::cout << '\n';
            inputStartTop = GetCursorTop();
            return result;
        }
        // arrow keys
        else if (key.key == Key::LeftArrow) {
            if (cursor > 0) {
                cursor--;
                RedrawLine(buffer, cursor);
            }
        } else if (key.key == Key::RightArrow) {
            std::string current = buffer;

            // accept a filesystem ghost suggestion first
            if (cursor == static_cast<int>(buffer.size()) && !nocolormode) {
                auto fileGhost = GetFileGhost(current);
                if (fileGhost.has_value() && !fileGhost->ghost.empty()) {
                    buffer.erase(fileGhost->start);
                    buffer.append(fileGhost->replacement);

                    cursor = static_cast<int>(buffer.size());
                    tabPrefix.clear();
                    tabMatches.clear();
                    tabIndex = -1;
                    tabCurrentSelection.clear();
                    RedrawLine(buffer, cursor);
                    continue;
                }
            }

            // existing command autocomplete
            std::string match;
            for (const auto& c : commands) {
                if (StartsWithIgnoreCase(c, current) && c != current) {
                    match = c;
                    break;
                }
            }

            if (cursor == static_cast<int>(buffer.size()) && !match.empty()) {
                buffer = match;
                cursor = static_cast<int>(buffer.size());
                tabCurrentSelection.clear();
                RedrawLine(buffer, cursor);
            } else if (cursor < static_cast<int>(buffer.size())) {
                cursor++;
                tabCurrentSelection.clear();
                RedrawLine(buffer, cursor);
            }
        }
        // backspace (to delete a character)
        else if (key.key == Key::Backspace) {
            if (cursor > 0) {
                tabPrefix.clear();
                tabMatches.clear();
                tabIndex = -1;

                buffer.erase(cursor - 1, 1);
                cursor--;
                RedrawLine(buffer, cursor);
            }
        }
        // same thing but with [DELETE] key
        else if (key.key == Key::Delete) {
            if (cursor < static_cast<int>(buffer.size())) {
                tabPrefix.clear();
                tabCurrentSelection.clear();
                tabMatches.clear();
                tabIndex = -1;

                buffer.erase(cursor, 1);
                RedrawLine(buffer, cursor);
            }
        }
        // home and end keys
        else if (key.key == Key::Home) {
            cursor = 0;
            RedrawLine(buffer, cursor);
        } else if (key.key == Key::End) {
            cursor = static_cast<int>(buffer.size());
            RedrawLine(buffer, cursor);
        }
        // show last command upon pressing the up/down arrow key
        else if (key.key == Key::UpArrow) {
            if (!history.empty() && historyIndex > 0) {
                historyIndex--;
                buffer = history[historyIndex];
                cursor = static_cast<int>(buffer.size());
                RedrawLine(buffer, cursor);
            }
        } else if (key.key == Key::DownArrow) {
            if (historyIndex < static_cast<int>(history.size()) - 1) {
                historyIndex++;
                buffer = history[historyIndex];
            } else {
                historyIndex = static_cast<int>(history.size());
                buffer.clear();
            }
            cursor = static_cast<int>(buffer.size());
            RedrawLine(buffer, cursor);
        }
        // [TAB] directory/file autocomplete
        else if (key.key == Key::Tab) {
            TabComplete(buffer, cursor, tabPrefix, tabCurrentSelection, tabMatches, tabIndex);
        }
        // [CTRL] + [L] cls logic
        else if (key.ctrl && (key.keyChar == 'l' || key.keyChar == 'L')) {
            ClearScreen();
            SetCursorPosition(0, 0);
            inputStartTop = 0;
            RedrawLine(buffer, cursor);
        }
        // printable characters
        else if (!key.ctrl && key.keyChar >= 32 && key.keyChar != 127) {
            tabPrefix.clear();
            tabCurrentSelection.clear();
            tabMatches.clear();
            tabIndex = -1;

            buffer.insert(buffer.begin() + cursor, key.keyChar);
            cursor++;
            RedrawLine(buffer, cursor);
        }
    }
}

std::optional<FileGhostResult> GetFileGhost(std::string_view input) {
    if (input.empty())
        return std::nullopt;

    // don't suggest filesystem entries in command position
    size_t spacePos = input.find(' ');
    if (spacePos == std::string_view::npos)
        return std::nullopt;

    int start = static_cast<int>(input.rfind(' ')) + 1;
    std::string typed(input.substr(start));

    if (typed.empty())
        return std::nullopt;

    bool quoted = typed.front() == '"';
    std::string search = quoted ? typed.substr(1) : typed;

    fs::path searchPath(search);
    fs::path combined = searchPath.is_absolute() ? searchPath : (fs::path(currentDir) / searchPath);

    std::error_code ec;
    const char sepChar = fs::path::preferred_separator;
    std::string separator(1, sepChar);

    if (!search.empty() &&
        fs::is_directory(combined, ec) &&
        search.back() != '/' &&
        search.back() != '\\') {
        return FileGhostResult{start, (quoted ? "\"" : "") + search + separator, separator};
    }

    fs::path directory;
    std::string prefix;

    if (fs::is_directory(combined, ec)) {
        directory = combined;
        prefix = "";
    } else {
        directory = combined.parent_path();
        if (directory.empty()) directory = currentDir;
        prefix = combined.filename().string();
    }

    if (!fs::is_directory(directory, ec))
        return std::nullopt;

    std::vector<std::string> candidates;
    for (const auto& entry : fs::directory_iterator(directory, ec)) {
        std::string name = entry.path().filename().string();
        if (StartsWithIgnoreCase(name, prefix) && !EqualsIgnoreCase(name, prefix)) {
            candidates.push_back(name);
        }
    }

    if (candidates.empty())
        return std::nullopt;

    std::sort(candidates.begin(), candidates.end(), [](const std::string& a, const std::string& b) {
        return ToLower(a) < ToLower(b);
    });

    const std::string& match = candidates.front();
    size_t keepLen = search.size() > prefix.size() ? search.size() - prefix.size() : 0;
    std::string typedDirectory = search.substr(0, keepLen);
    std::string candidate = typedDirectory + match;

    fs::path fullCand = fs::path(candidate).is_absolute()
        ? fs::path(candidate)
        : (fs::path(currentDir) / candidate);

    bool isDirectory = fs::is_directory(fullCand, ec);
    if (isDirectory)
        candidate += sepChar;

    std::string replacement;
    if (candidate.find(' ') != std::string::npos) {
        std::string trimmedCand = candidate;
        while (!trimmedCand.empty() && (trimmedCand.back() == '/' || trimmedCand.back() == '\\')) {
            trimmedCand.pop_back();
        }
        replacement = "\"" + trimmedCand + "\"";
    } else {
        replacement = candidate;
    }

    if (quoted) {
        replacement = "\"" + candidate + (isDirectory ? "" : "\"");
    }

    std::string ghost = candidate.substr(std::min(search.size(), candidate.size()));
    return FileGhostResult{start, replacement, ghost};
}

void TabComplete(
    std::string& buffer,
    int& cursor,
    std::string& tabPrefix,
    std::string& tabCurrentSelection,
    std::vector<std::string>& tabMatches,
    int& tabIndex) {
    std::string beforeCursor = buffer.substr(0, cursor);
    size_t lastSpace = beforeCursor.rfind(' ');
    int argStart = (lastSpace == std::string::npos) ? 0 : static_cast<int>(lastSpace) + 1;
    std::string typed = beforeCursor.substr(argStart);

    if (typed.empty())
        return;

    bool cycling = !tabMatches.empty() && tabIndex >= 0 && typed == tabCurrentSelection;

    if (!cycling) {
        tabPrefix = typed;
        tabCurrentSelection.clear();
        tabIndex = -1;
        tabMatches.clear();

        std::string searchPathStr = typed;
        if (!searchPathStr.empty() && searchPathStr.front() == '"') searchPathStr.erase(0, 1);
        if (!searchPathStr.empty() && searchPathStr.back() == '"') searchPathStr.pop_back();

        fs::path searchPath(searchPathStr);
        fs::path directory;
        std::string prefix;
        std::error_code ec;

        if (searchPath.is_absolute()) {
            directory = searchPath.parent_path();
            if (directory.empty()) directory = currentDir;
            prefix = searchPath.filename().string();
        } else {
            fs::path combined = fs::path(currentDir) / searchPath;
            if (fs::is_directory(combined, ec) &&
                (searchPathStr.empty() || searchPathStr.back() == '/' || searchPathStr.back() == '\\')) {
                directory = combined;
                prefix = "";
            } else {
                directory = combined.parent_path();
                if (directory.empty()) directory = currentDir;
                prefix = combined.filename().string();
            }
        }

        if (!fs::is_directory(directory, ec))
            return;

        for (const auto& entry : fs::directory_iterator(directory, ec)) {
            std::string name = entry.path().filename().string();
            if (StartsWithIgnoreCase(name, prefix)) {
                tabMatches.push_back(entry.path().string());
            }
        }

        std::sort(tabMatches.begin(), tabMatches.end(), [](const std::string& a, const std::string& b) {
            return ToLower(fs::path(a).filename().string()) < ToLower(fs::path(b).filename().string());
        });
    }

    if (tabMatches.empty())
        return;

    tabIndex = (tabIndex + 1) % static_cast<int>(tabMatches.size());

    fs::path selected(tabMatches[tabIndex]);
    std::string selectedName = selected.filename().string();

    std::string typedWithoutQuotes = tabPrefix;
    if (!typedWithoutQuotes.empty() && typedWithoutQuotes.front() == '"') typedWithoutQuotes.erase(0, 1);
    if (!typedWithoutQuotes.empty() && typedWithoutQuotes.back() == '"') typedWithoutQuotes.pop_back();

    fs::path typedDir = fs::path(typedWithoutQuotes).parent_path();
    std::string replacement = typedDir.empty()
        ? selectedName
        : (typedDir / selectedName).string();

    std::error_code ec;
    if (fs::is_directory(selected, ec))
        replacement += fs::path::preferred_separator;

    if (!tabPrefix.empty() && tabPrefix.front() == '"') {
        while (!replacement.empty() && (replacement.back() == '/' || replacement.back() == '\\')) {
            replacement.pop_back();
        }
        replacement = "\"" + replacement + "\"";
    } else if (replacement.find(' ') != std::string::npos) {
        replacement = "\"" + replacement + "\"";
    }

    buffer.erase(argStart, cursor - argStart);
    buffer.insert(argStart, replacement);

    cursor = argStart + static_cast<int>(replacement.size());
    tabCurrentSelection = replacement;

    RedrawLine(buffer, cursor);
}

void RedrawLine(const std::string& buffer, int cursor, bool showGhost) {
    auto ResCol = []() {
        ResForegroundColor();
        SetColor(currentBg, true);
    };

    ResCol();

    int top = inputStartTop;
    int bufWidth = std::max(2, GetBufferWidth());

    std::string prompt = ReturnPrompt();

    // calculate how many rows the input occupies
    int totalLength = static_cast<int>(prompt.size() + buffer.size());
    int linesUsed = std::max(1, (totalLength + bufWidth - 1) / bufWidth);

    // clear all affected lines
    for (int i = 0; i < linesUsed; i++) {
        SetCursorPosition(0, top + i);
        ResCol();
        std::cout << std::string(bufWidth - 1, ' ');
    }

    // return to original line
    SetCursorPosition(0, top);

    // render prompt
    ResCol();
    std::cout << prompt;
    ResCol();

    // split command + args
    size_t firstSpace = buffer.find(' ');
    std::string cmd = (firstSpace == std::string::npos) ? buffer : buffer.substr(0, firstSpace);
    std::string args = (firstSpace == std::string::npos) ? "" : buffer.substr(firstSpace);

    bool exact = false;
    for (const auto& c : commands) {
        if (EqualsIgnoreCase(c, cmd)) {
            exact = true;
            break;
        }
    }

    bool launchable = IsLaunchableProgram(cmd);

    std::optional<std::string> executableGhost =
        (cursor == static_cast<int>(buffer.size()) && firstSpace == std::string::npos)
            ? GetExecutableGhost(cmd)
            : std::nullopt;

    bool partial = false;
    if (!launchable && !cmd.empty()) {
        for (const auto& c : commands) {
            if (StartsWithIgnoreCase(c, cmd)) {
                partial = true;
                break;
            }
        }
    }

    // command highlighting
    if (!cmd.empty()) {
        if (!nocolormode) {
            if (exact || launchable || executableGhost.has_value())
                SetColor(ConsoleColor::White);
            else if (partial)
                SetColor(ConsoleColor::Yellow);
            else
                SetColor(ConsoleColor::Red);
        }

        std::cout << cmd;
        ResCol();
    }

    // arguments with highlighting
    if (!args.empty()) {
        size_t pos = 0;
        while (pos < args.size()) {
            size_t nextSpace = args.find(' ', pos);
            std::string arg = (nextSpace == std::string::npos)
                ? args.substr(pos)
                : args.substr(pos, nextSpace - pos);

            if (!arg.empty()) {
                std::cout << ' ';

                if (arg.starts_with("-") || arg.starts_with("--") || arg.starts_with("/")) {
                    SetColor(ConsoleColor::DarkCyan);
                } else if (arg.size() >= 2 && arg.front() == '"' && arg.back() == '"') {
                    SetColor(ConsoleColor::DarkYellow);
                } else if (arg.find('\\') != std::string::npos ||
                           arg.find('/') != std::string::npos ||
                           arg.find(':') != std::string::npos) {
                    SetColor(ConsoleColor::Blue);
                } else {
                    SetColor(currentFg);
                }

                std::cout << arg;
                ResCol();
            }

            if (nextSpace == std::string::npos) break;
            pos = nextSpace + 1;
        }
    }

    // ghost autocomplete
    if (showGhost) {
        auto fileGhost = GetFileGhost(buffer);

        if (!nocolormode &&
            cursor == static_cast<int>(buffer.size()) &&
            fileGhost.has_value() &&
            !fileGhost->ghost.empty()) {
            if (fileGhost->start == 0) {
                SetCursorPosition(0, top);
                ResCol();
                std::cout << prompt;

                SetColor((exact || launchable) ? ConsoleColor::White : ConsoleColor::Yellow);
                std::cout << buffer;

                SetColor(ConsoleColor::DarkGray);
                std::cout << fileGhost->ghost;
            } else {
                SetCursorPosition(static_cast<int>(prompt.size() + buffer.size()), top);
                SetColor(ConsoleColor::DarkGray);
                std::cout << fileGhost->ghost;
            }
        } else if (!nocolormode &&
                   cursor == static_cast<int>(buffer.size()) &&
                   executableGhost.has_value()) {
            SetColor(ConsoleColor::DarkGray);
            std::cout << *executableGhost;
        } else if (!nocolormode && partial && !exact && !launchable && !cmd.empty()) {
            for (const auto& c : commands) {
                if (StartsWithIgnoreCase(c, cmd)) {
                    SetColor(ConsoleColor::DarkGray);
                    std::cout << c.substr(cmd.size());
                    break;
                }
            }
        }
    }

    ResCol();

    // restore cursor
    int absolute = static_cast<int>(prompt.size()) + cursor;
    int newLeft = absolute % bufWidth;
    int newTop = top + (absolute / bufWidth);

    SetCursorPosition(newLeft, newTop);
    ResCol();
    std::cout << std::flush;
}

std::vector<std::string> ParseQuotedArgs(std::string_view input) {
    std::vector<std::string> result;
    std::string current;
    bool inQuotes = false;
    bool hasToken = false;

    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];
        if (c == '"') {
            inQuotes = !inQuotes;
            hasToken = true;
        } else if (!inQuotes && std::isspace(static_cast<unsigned char>(c))) {
            if (hasToken) {
                result.push_back(current);
                current.clear();
                hasToken = false;
            }
        } else {
            current.push_back(c);
            hasToken = true;
        }
    }

    if (hasToken) {
        result.push_back(current);
    }

    return result;
}

bool EqualsIgnoreCase(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

bool StartsWithIgnoreCase(std::string_view str, std::string_view prefix) {
    if (prefix.size() > str.size()) return false;
    return EqualsIgnoreCase(str.substr(0, prefix.size()), prefix);
}

bool EndsWithIgnoreCase(std::string_view str, std::string_view suffix) {
    if (suffix.size() > str.size()) return false;
    return EqualsIgnoreCase(str.substr(str.size() - suffix.size()), suffix);
}

std::string ToLower(std::string_view s) {
    std::string out(s);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return out;
}

std::string ToUpper(std::string_view s) {
    std::string out(s);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return out;
}

std::string Trim(std::string_view s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
        start++;
    }
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
        end--;
    }
    return std::string(s.substr(start, end - start));
}

} // namespace fis
