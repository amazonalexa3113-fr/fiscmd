#include "FisDraw.hpp"
#include "ConsoleUtils.hpp"
#include "Program.hpp"
#include "SetColor.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

namespace fis {

void FisDraw(const std::vector<std::string>& args) {
    constexpr int canvasW = 48;
    constexpr int canvasH = 18;

    using Grid = std::vector<std::string>;
    Grid canvas(canvasH, std::string(canvasW, ' '));
    std::vector<Grid> undoStack;
    std::vector<Grid> redoStack;

    fs::path filePath;
    if (args.size() > 1) {
        filePath = fs::path(args[1]).is_absolute()
            ? fs::path(args[1])
            : (fs::path(currentDir) / args[1]);
        std::error_code ec;
        if (fs::is_regular_file(filePath, ec)) {
            std::ifstream in(filePath);
            std::string line;
            int row = 0;
            while (row < canvasH && std::getline(in, line)) {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                for (int col = 0; col < std::min<int>(canvasW, static_cast<int>(line.size())); ++col) {
                    canvas[row][col] = line[col];
                }
                row++;
            }
        }
    }

    int cx = canvasW / 2;
    int cy = canvasH / 2;
    char brush = '#';

    auto pushUndo = [&]() {
        undoStack.push_back(canvas);
        if (undoStack.size() > 50) undoStack.erase(undoStack.begin());
        redoStack.clear();
    };

    ClearScreen();
    SetCursorVisible(false);

    while (true) {
        SetCursorPosition(0, 0);
        SetColor(ConsoleColor::Cyan);
        std::cout << "><> FISDRAW | Brush: [" << brush << "] | Pos: (" << cx << "," << cy << ")\n";
        std::cout << "Arrows: Move | Space: Draw | E: Erase | B: Brush | Z: Undo | Y: Redo | S: Save | Q/ESC: Quit\n";

        SetColor(ConsoleColor::DarkCyan);
        std::cout << '+' << std::string(canvasW, '-') << "+\n";
        for (int y = 0; y < canvasH; ++y) {
            SetColor(ConsoleColor::DarkCyan);
            std::cout << '|';
            for (int x = 0; x < canvasW; ++x) {
                if (x == cx && y == cy) {
                    SetColor(ConsoleColor::Black);
                    SetColor(ConsoleColor::Yellow, true);
                    std::cout << (canvas[y][x] == ' ' ? '+' : canvas[y][x]);
                    ResetColor();
                } else {
                    SetColor(ConsoleColor::Yellow);
                    std::cout << canvas[y][x];
                }
            }
            SetColor(ConsoleColor::DarkCyan);
            std::cout << "|\n";
        }
        std::cout << '+' << std::string(canvasW, '-') << "+\n" << std::flush;

        KeyInfo k = ReadKey(true);
        char c = static_cast<char>(std::tolower(static_cast<unsigned char>(k.keyChar)));

        if (k.key == Key::Escape || c == 'q') {
            break;
        } else if (k.key == Key::UpArrow && cy > 0) {
            cy--;
        } else if (k.key == Key::DownArrow && cy < canvasH - 1) {
            cy++;
        } else if (k.key == Key::LeftArrow && cx > 0) {
            cx--;
        } else if (k.key == Key::RightArrow && cx < canvasW - 1) {
            cx++;
        } else if (k.key == Key::Spacebar || k.key == Key::Enter) {
            pushUndo();
            canvas[cy][cx] = brush;
        } else if (c == 'e') {
            pushUndo();
            canvas[cy][cx] = ' ';
        } else if (c == 'c') {
            pushUndo();
            canvas.assign(canvasH, std::string(canvasW, ' '));
        } else if (c == 'z') {
            if (!undoStack.empty()) {
                redoStack.push_back(canvas);
                canvas = undoStack.back();
                undoStack.pop_back();
            }
        } else if (c == 'y') {
            if (!redoStack.empty()) {
                undoStack.push_back(canvas);
                canvas = redoStack.back();
                redoStack.pop_back();
            }
        } else if (c == 'b') {
            SetCursorPosition(0, canvasH + 4);
            SetColor(ConsoleColor::Cyan);
            std::cout << "Press new brush character: " << std::flush;
            KeyInfo bKey = ReadKey(true);
            if (bKey.keyChar >= 32 && bKey.keyChar != 127) {
                brush = bKey.keyChar;
            }
            ClearScreen();
        } else if (c == 's') {
            if (filePath.empty()) {
                filePath = fs::path(currentDir) / "fisdraw_art.txt";
            }
            std::ofstream out(filePath);
            for (const auto& row : canvas) {
                out << row << '\n';
            }
            SetCursorPosition(0, canvasH + 4);
            SetColor(ConsoleColor::Cyan);
            std::cout << "Saved to " << filePath.string() << " :D   " << std::flush;
        }
    }

    SetCursorVisible(true);
    ClearScreen();
    ResForegroundColor();
}

} // namespace fis
