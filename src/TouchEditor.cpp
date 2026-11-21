#include "TouchEditor.hpp"
#include "ConsoleUtils.hpp"
#include "Program.hpp"
#include "SetColor.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_set>

namespace fs = std::filesystem;

namespace fis {

namespace {

std::unordered_set<std::string> SplitSet(std::string_view words) {
    std::unordered_set<std::string> set;
    std::istringstream iss{std::string(words)};
    std::string w;
    while (iss >> w) set.insert(w);
    return set;
}

bool ContainsExt(std::string_view ext, std::initializer_list<std::string_view> list) {
    for (auto item : list) {
        if (ext == item) return true;
    }
    return false;
}

} // namespace

void Touch(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SetColor(ConsoleColor::Cyan);
        std::cout << "usage: touch <filename>\n";
        std::cout << "<filename> - name of the file needed to edit\n\n";
        SetColor(ConsoleColor::Red);
        std::cout << "anonnoyingly flashes for every arrow key u press\n";
        SetColor(ConsoleColor::Cyan);
        std::cout << "i hope ur all fine with ur suffering :sob::pray:\n";
        SetColor(ConsoleColor::DarkCyan);
        std::cout << "\nhonorable mention: supports syntax highlighting for most valid file extension today now :D\n";
        ResForegroundColor();
        return;
    }

    std::string filename;
    for (size_t i = 1; i < args.size(); ++i) {
        if (i > 1) filename += ' ';
        filename += args[i];
    }
    filename = Trim(filename);

    if (filename.empty()) {
        SetColor(ConsoleColor::Cyan);
        std::cout << "usage: touch <filename>\n";
        ResForegroundColor();
        return;
    }

    std::error_code ec;
    fs::path path = fs::weakly_canonical(fs::path(currentDir) / filename, ec);
    if (ec) path = fs::path(currentDir) / filename;

    std::vector<std::string> lines;
    if (fs::exists(path, ec)) {
        std::ifstream in(path);
        std::string line;
        while (std::getline(in, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            lines.push_back(line);
        }
    }
    if (lines.empty()) lines.emplace_back("");

    int cursorX = 0;
    int cursorY = 0;
    int scrollOffset = 0;
    bool dirty = false;

    std::string extension = ToLower(path.extension().string());

    bool pythonLike = ContainsExt(extension, {
        ".py", ".pyw", ".sh", ".bash", ".fish",
        ".yml", ".yaml", ".toml", ".r", ".rb",
        ".pl", ".pm", ".conf", ".ini", ".properties"
    });

    bool hashComments = pythonLike || ContainsExt(extension, {".ps1", ".psm1", ".psd1"});

    bool slashComments = ContainsExt(extension, {
        ".cs", ".java", ".js", ".jsx", ".ts", ".tsx",
        ".c", ".h", ".cpp", ".hpp", ".cc", ".hh",
        ".go", ".rs", ".swift", ".kt", ".kts",
        ".dart", ".scala", ".php", ".css", ".scss",
        ".less", ".jsonc", ".gradle", ".groovy",
        ".sql", ".lua", ".m", ".mm"
    });

    bool htmlComments = ContainsExt(extension, {".html", ".htm", ".xml", ".svg", ".xhtml"});
    bool sqlComments = (extension == ".sql");

    std::unordered_set<std::string> keywords;
    if (ContainsExt(extension, {".cs", ".java", ".c", ".h", ".cpp", ".hpp", ".cc", ".hh", ".kt", ".kts", ".scala", ".swift"})) {
        keywords = SplitSet(
            "abstract as async await base bool break byte case "
            "catch char checked class const continue decimal default "
            "delegate do double else enum event explicit extern false "
            "finally fixed float for foreach goto if implicit in int "
            "interface internal is lock long namespace new null object "
            "operator out override params private protected public "
            "readonly record ref return sbyte sealed short sizeof stackalloc "
            "static string struct switch this throw true try typeof uint "
            "ulong unchecked unsafe ushort using var virtual void volatile "
            "while get set init required yield package import extends "
            "implements super throws final native synchronized transient"
        );
    } else if (ContainsExt(extension, {".py", ".pyw"})) {
        keywords = SplitSet(
            "and as assert async await break class continue def del elif "
            "else except finally for from global if import in is lambda "
            "nonlocal not or pass raise return try while with yield "
            "False None True self"
        );
    } else if (ContainsExt(extension, {".js", ".jsx", ".ts", ".tsx", ".mjs", ".cjs"})) {
        keywords = SplitSet(
            "async await break case catch class const continue debugger "
            "default delete do else export extends false finally for "
            "from function if import in instanceof let new null of return "
            "static super switch this throw true try typeof undefined "
            "var void while with yield interface type enum implements "
            "private protected public readonly abstract declare"
        );
    } else if (extension == ".json" || extension == ".jsonc") {
        keywords = {"true", "false", "null"};
    } else if (extension == ".sql") {
        keywords = SplitSet(
            "ADD ALL ALTER AND AS ASC BEGIN BETWEEN BY CASE CAST CHECK "
            "COLUMN COMMIT CONSTRAINT CREATE CROSS DATABASE DEFAULT DELETE "
            "DESC DISTINCT DROP ELSE END EXCEPT EXISTS FOREIGN FROM FULL "
            "GROUP HAVING IN INDEX INNER INSERT INTO IS JOIN KEY LEFT LIKE "
            "LIMIT NOT NULL OFFSET ON OR ORDER OUTER PRIMARY REFERENCES "
            "RIGHT ROLLBACK SELECT SET TABLE THEN TOP TRANSACTION UNION "
            "UNIQUE UPDATE VALUES VIEW WHEN WHERE WITH"
        );
    } else if (ContainsExt(extension, {".html", ".htm", ".xml", ".svg"})) {
        keywords = SplitSet(
            "html head body title meta link script style div span p a "
            "img input button form label main section article header "
            "footer nav ul ol li table tr td th h1 h2 h3 h4 h5 h6 "
            "doctype xmlns svg path rect circle"
        );
    } else if (ContainsExt(extension, {".css", ".scss", ".less"})) {
        keywords = SplitSet(
            "important inherit initial unset revert none auto block "
            "inline flex grid absolute relative fixed sticky hidden "
            "solid dashed dotted transparent currentColor"
        );
    } else if (extension == ".md" || extension == ".markdown") {
        keywords = {"#", "##", "###", "####", "#####", "######"};
    } else if (ContainsExt(extension, {".sh", ".bash", ".fish"})) {
        keywords = SplitSet(
            "if then else elif fi for while do done case esac in "
            "function return exit export local readonly unset source "
            "set echo printf read cd pwd eval exec test"
        );
    }

    auto WriteHighlightedLine = [&](std::string_view text) {
        size_t i = 0;
        while (i < text.size()) {
            bool isComment = false;

            if (htmlComments && text.substr(i).starts_with("<!--")) {
                size_t end = text.find("-->", i);
                size_t count = (end == std::string_view::npos) ? (text.size() - i) : (end + 3 - i);
                SetColor(ConsoleColor::DarkGreen);
                std::cout << text.substr(i, count);
                ResForegroundColor();
                i += count;
                continue;
            }

            if (slashComments &&
                (text.substr(i).starts_with("//") || (sqlComments && text.substr(i).starts_with("--")))) {
                isComment = true;
            }

            if (hashComments && text[i] == '#') {
                isComment = true;
            }

            if (isComment) {
                SetColor(ConsoleColor::DarkGreen);
                std::cout << text.substr(i);
                ResForegroundColor();
                break;
            }

            if (text[i] == '"' || text[i] == '\'' ||
                (text[i] == '`' && ContainsExt(extension, {".js", ".jsx", ".ts", ".tsx", ".mjs", ".cjs"}))) {
                char quote = text[i];
                size_t start = i++;
                bool escaped = false;
                while (i < text.size()) {
                    char c = text[i++];
                    if (escaped) { escaped = false; continue; }
                    if (c == '\\') { escaped = true; continue; }
                    if (c == quote) break;
                }
                SetColor(ConsoleColor::Yellow);
                std::cout << text.substr(start, i - start);
                ResForegroundColor();
                continue;
            }

            if (std::isdigit(static_cast<unsigned char>(text[i])) &&
                (i == 0 || (!std::isalnum(static_cast<unsigned char>(text[i - 1])) && text[i - 1] != '_'))) {
                size_t start = i++;
                while (i < text.size() &&
                       (std::isalnum(static_cast<unsigned char>(text[i])) ||
                        text[i] == '.' || text[i] == '_' || text[i] == '+' || text[i] == '-')) {
                    if ((text[i] == '+' || text[i] == '-') && text[i - 1] != 'e' && text[i - 1] != 'E')
                        break;
                    i++;
                }
                SetColor(ConsoleColor::Magenta);
                std::cout << text.substr(start, i - start);
                ResForegroundColor();
                continue;
            }

            if (std::isalpha(static_cast<unsigned char>(text[i])) || text[i] == '_' || text[i] == '$') {
                size_t start = i++;
                while (i < text.size() &&
                       (std::isalnum(static_cast<unsigned char>(text[i])) || text[i] == '_' || text[i] == '$')) {
                    i++;
                }
                std::string word(text.substr(start, i - start));
                if (keywords.count(word) || (extension == ".sql" && keywords.count(ToUpper(word)))) {
                    SetColor(ConsoleColor::Cyan);
                    std::cout << word;
                    ResForegroundColor();
                } else if (word == "TODO" || word == "FIXME" || word == "NOTE" || word == "HACK") {
                    SetColor(ConsoleColor::Red);
                    std::cout << word;
                    ResForegroundColor();
                } else {
                    std::cout << word;
                }
                continue;
            }

            if ((extension == ".md" || extension == ".markdown") &&
                text[i] == '#' && (i == 0 || text[i - 1] == ' ')) {
                size_t start = i;
                while (i < text.size() && text[i] == '#') i++;
                SetColor(ConsoleColor::Cyan);
                std::cout << text.substr(start, i - start);
                ResForegroundColor();
                continue;
            }

            std::cout << text[i];
            i++;
        }
    };

    auto SaveFile = [&]() {
        std::ofstream out(path);
        for (size_t idx = 0; idx < lines.size(); ++idx) {
            out << lines[idx];
            if (idx + 1 < lines.size()) out << '\n';
        }
        dirty = false;
    };

    ClearScreen();

    while (true) {
        if (lines.empty()) lines.emplace_back("");

        int winW = GetWindowWidth();
        int winH = GetWindowHeight();
        int visibleHeight = std::max(1, winH - 2);

        cursorY = std::clamp(cursorY, 0, static_cast<int>(lines.size()) - 1);
        cursorX = std::clamp(cursorX, 0, static_cast<int>(lines[cursorY].size()));

        if (cursorY < scrollOffset) scrollOffset = cursorY;
        if (cursorY >= scrollOffset + visibleHeight) scrollOffset = cursorY - visibleHeight + 1;

        SetCursorPosition(0, 0);

        for (int i = 0; i < visibleHeight; i++) {
            int lineIndex = i + scrollOffset;
            SetCursorPosition(0, i);

            std::string_view text = (lineIndex < static_cast<int>(lines.size()))
                ? std::string_view(lines[lineIndex])
                : std::string_view("");

            int maxWidth = std::max(0, winW - 1);
            std::string_view visibleText = (static_cast<int>(text.size()) > maxWidth)
                ? text.substr(0, maxWidth)
                : text;

            WriteHighlightedLine(visibleText);

            int padding = maxWidth - static_cast<int>(visibleText.size());
            if (padding > 0) std::cout << std::string(padding, ' ');
        }

        // status bar
        SetCursorPosition(0, winH - 2);
        SetColor(ConsoleColor::DarkGray);
        std::string status = " TOUCH | " + std::string(dirty ? "modified" : "saved") +
                             " | ESC quit | Ctrl+S save | Ctrl+X save & quit | line " +
                             std::to_string(cursorY + 1) + "/" + std::to_string(lines.size());
        if (static_cast<int>(status.size()) > winW) status.resize(winW);
        status.resize(winW, ' ');
        std::cout << status;
        ResForegroundColor();

        SetCursorPosition(0, winH - 1);
        SetColor(ConsoleColor::DarkGray);
        std::string fnameBar = " " + path.filename().string();
        if (static_cast<int>(fnameBar.size()) > winW) fnameBar.resize(winW);
        fnameBar.resize(winW, ' ');
        std::cout << fnameBar;
        ResForegroundColor();

        int drawY = cursorY - scrollOffset;
        int drawX = std::min(cursorX, std::max(0, winW - 1));
        SetCursorPosition(drawX, drawY);
        std::cout << std::flush;

        KeyInfo key = ReadKey(true);

        if (key.key == Key::Escape) {
            ClearScreen();
            std::cout << (dirty ? "exited without saving :skull:\n" : "exited :D\n");
            ResForegroundColor();
            return;
        }

        if (key.ctrl && (key.keyChar == 's' || key.keyChar == 'S')) {
            SaveFile();
            SetCursorPosition(0, winH - 1);
            SetColor(ConsoleColor::Cyan);
            std::string savedMsg = "[saved with Ctrl+S :D]";
            if (static_cast<int>(savedMsg.size()) > winW) savedMsg.resize(winW);
            savedMsg.resize(winW, ' ');
            std::cout << savedMsg << std::flush;
            ResForegroundColor();
            continue;
        }

        if (key.ctrl && (key.keyChar == 'x' || key.keyChar == 'X')) {
            SaveFile();
            ClearScreen();
            SetColor(ConsoleColor::Cyan);
            std::cout << "saved & exited :D\n";
            ResForegroundColor();
            return;
        }

        if (key.key == Key::Home) { cursorX = 0; continue; }
        if (key.key == Key::End) { cursorX = static_cast<int>(lines[cursorY].size()); continue; }

        if (key.key == Key::PageUp) {
            cursorY = std::max(0, cursorY - visibleHeight);
            cursorX = std::min(cursorX, static_cast<int>(lines[cursorY].size()));
            continue;
        }
        if (key.key == Key::PageDown) {
            cursorY = std::min(static_cast<int>(lines.size()) - 1, cursorY + visibleHeight);
            cursorX = std::min(cursorX, static_cast<int>(lines[cursorY].size()));
            continue;
        }

        if (key.key == Key::LeftArrow && key.ctrl) {
            if (cursorX > 0) {
                while (cursorX > 0 && std::isspace(static_cast<unsigned char>(lines[cursorY][cursorX - 1]))) cursorX--;
                while (cursorX > 0 && (std::isalnum(static_cast<unsigned char>(lines[cursorY][cursorX - 1])) || lines[cursorY][cursorX - 1] == '_')) cursorX--;
                if (cursorX > 0 && !std::isspace(static_cast<unsigned char>(lines[cursorY][cursorX - 1]))) cursorX--;
            } else if (cursorY > 0) {
                cursorY--;
                cursorX = static_cast<int>(lines[cursorY].size());
            }
            continue;
        }

        if (key.key == Key::RightArrow && key.ctrl) {
            const std::string& line = lines[cursorY];
            int len = static_cast<int>(line.size());
            if (cursorX < len) {
                while (cursorX < len && (std::isalnum(static_cast<unsigned char>(line[cursorX])) || line[cursorX] == '_')) cursorX++;
                while (cursorX < len && std::isspace(static_cast<unsigned char>(line[cursorX]))) cursorX++;
                if (cursorX < len && !std::isalnum(static_cast<unsigned char>(line[cursorX])) && line[cursorX] != '_') cursorX++;
            } else if (cursorY < static_cast<int>(lines.size()) - 1) {
                cursorY++;
                cursorX = 0;
            }
            continue;
        }

        if (key.key == Key::Enter) {
            if (Trim(lines[cursorY]) == ":wq") {
                lines.erase(lines.begin() + cursorY);
                if (lines.empty()) lines.emplace_back("");
                SaveFile();
                ClearScreen();
                SetColor(ConsoleColor::Cyan);
                std::cout << "saved & exited :D\n";
                ResForegroundColor();
                return;
            }
            std::string left = lines[cursorY].substr(0, cursorX);
            std::string right = lines[cursorY].substr(cursorX);
            lines[cursorY] = left;
            lines.insert(lines.begin() + cursorY + 1, right);
            cursorY++;
            cursorX = 0;
            dirty = true;
            continue;
        }

        if (key.key == Key::Backspace) {
            if (cursorX > 0) {
                lines[cursorY].erase(cursorX - 1, 1);
                cursorX--;
                dirty = true;
            } else if (cursorY > 0) {
                int prevLen = static_cast<int>(lines[cursorY - 1].size());
                lines[cursorY - 1] += lines[cursorY];
                lines.erase(lines.begin() + cursorY);
                cursorY--;
                cursorX = prevLen;
                dirty = true;
            }
            continue;
        }

        if (key.key == Key::Delete) {
            if (cursorX < static_cast<int>(lines[cursorY].size())) {
                lines[cursorY].erase(cursorX, 1);
                dirty = true;
            } else if (cursorY < static_cast<int>(lines.size()) - 1) {
                lines[cursorY] += lines[cursorY + 1];
                lines.erase(lines.begin() + cursorY + 1);
                dirty = true;
            }
            continue;
        }

        if (key.key == Key::LeftArrow) {
            if (cursorX > 0) cursorX--;
            else if (cursorY > 0) { cursorY--; cursorX = static_cast<int>(lines[cursorY].size()); }
            continue;
        }

        if (key.key == Key::RightArrow) {
            if (cursorX < static_cast<int>(lines[cursorY].size())) cursorX++;
            else if (cursorY < static_cast<int>(lines.size()) - 1) { cursorY++; cursorX = 0; }
            continue;
        }

        if (key.key == Key::UpArrow) {
            if (cursorY > 0) {
                cursorY--;
                cursorX = std::min(cursorX, static_cast<int>(lines[cursorY].size()));
            }
            continue;
        }

        if (key.key == Key::DownArrow) {
            if (cursorY < static_cast<int>(lines.size()) - 1) {
                cursorY++;
                cursorX = std::min(cursorX, static_cast<int>(lines[cursorY].size()));
            }
            continue;
        }

        if (!key.ctrl && key.keyChar >= 32 && key.keyChar != 127) {
            lines[cursorY].insert(lines[cursorY].begin() + cursorX, key.keyChar);
            cursorX++;
            dirty = true;
        }
    }
}

} // namespace fis
