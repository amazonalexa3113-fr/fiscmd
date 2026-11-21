#include "Snake.hpp"
#include "ConsoleUtils.hpp"
#include "SetColor.hpp"

#include <algorithm>
#include <chrono>
#include <deque>
#include <iostream>
#include <random>
#include <thread>
#include <utility>

namespace fis {

void Snake(const std::vector<std::string>& args) {
    int width = 30;
    int height = 16;
    int speedMs = 110;

    if (args.size() > 1) {
        std::string mode = ToLower(args[1]);
        if (mode == "-help" || mode == "--help" || mode == "-?") {
            SetColor(ConsoleColor::Cyan);
            std::cout << "usage: fissnake [-easy | -normal | -hard]\n";
            std::cout << "controls: WASD or Arrow Keys to steer, ESC or Q to quit\n";
            ResForegroundColor();
            return;
        } else if (mode == "-easy") {
            speedMs = 150;
        } else if (mode == "-hard") {
            speedMs = 65;
        }
    }

    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> distX(1, width - 2);
    std::uniform_int_distribution<int> distY(1, height - 2);

    std::deque<std::pair<int, int>> snake = {
        {width / 2, height / 2},
        {width / 2 - 1, height / 2},
        {width / 2 - 2, height / 2}
    };

    int dirX = 1;
    int dirY = 0;
    int score = 0;

    auto spawnFood = [&]() -> std::pair<int, int> {
        while (true) {
            int fx = distX(rng);
            int fy = distY(rng);
            bool onSnake = false;
            for (const auto& seg : snake) {
                if (seg.first == fx && seg.second == fy) {
                    onSnake = true;
                    break;
                }
            }
            if (!onSnake) return {fx, fy};
        }
    };

    auto food = spawnFood();
    ClearScreen();
    SetCursorVisible(false);

    bool gameOver = false;

    while (!gameOver) {
        if (KeyAvailable()) {
            KeyInfo k = ReadKey(true);
            char c = static_cast<char>(std::tolower(static_cast<unsigned char>(k.keyChar)));
            if (k.key == Key::Escape || c == 'q') {
                break;
            } else if ((k.key == Key::UpArrow || c == 'w') && dirY != 1) {
                dirX = 0; dirY = -1;
            } else if ((k.key == Key::DownArrow || c == 's') && dirY != -1) {
                dirX = 0; dirY = 1;
            } else if ((k.key == Key::LeftArrow || c == 'a') && dirX != 1) {
                dirX = -1; dirY = 0;
            } else if ((k.key == Key::RightArrow || c == 'd') && dirX != -1) {
                dirX = 1; dirY = 0;
            }
        }

        int nextX = snake.front().first + dirX;
        int nextY = snake.front().second + dirY;

        if (nextX <= 0 || nextX >= width - 1 || nextY <= 0 || nextY >= height - 1) {
            gameOver = true;
            break;
        }

        for (const auto& seg : snake) {
            if (seg.first == nextX && seg.second == nextY) {
                gameOver = true;
                break;
            }
        }
        if (gameOver) break;

        snake.push_front({nextX, nextY});
        if (nextX == food.first && nextY == food.second) {
            score += 10;
            food = spawnFood();
        } else {
            snake.pop_back();
        }

        SetCursorPosition(0, 0);
        SetColor(ConsoleColor::Cyan);
        std::cout << "><> FISSNAKE | Score: " << score << " | WASD/Arrows, Q/ESC quit\n";

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                if (y == 0 || y == height - 1 || x == 0 || x == width - 1) {
                    SetColor(ConsoleColor::DarkCyan);
                    std::cout << '#';
                } else if (x == food.first && y == food.second) {
                    SetColor(ConsoleColor::Red);
                    std::cout << '@';
                } else if (x == snake.front().first && y == snake.front().second) {
                    SetColor(ConsoleColor::Yellow);
                    std::cout << 'O';
                } else {
                    bool isBody = false;
                    for (size_t i = 1; i < snake.size(); ++i) {
                        if (snake[i].first == x && snake[i].second == y) {
                            isBody = true;
                            break;
                        }
                    }
                    if (isBody) {
                        SetColor(ConsoleColor::Green);
                        std::cout << 'o';
                    } else {
                        std::cout << ' ';
                    }
                }
            }
            std::cout << '\n';
        }
        std::cout << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(speedMs));
    }

    SetCursorVisible(true);
    ClearScreen();
    SetColor(ConsoleColor::Red);
    std::cout << (gameOver ? "skill issue, game over :skull:\n" : "exited fissnake :D\n");
    SetColor(ConsoleColor::Cyan);
    std::cout << "final score: " << score << "\n";
    ResForegroundColor();
}

} // namespace fis
