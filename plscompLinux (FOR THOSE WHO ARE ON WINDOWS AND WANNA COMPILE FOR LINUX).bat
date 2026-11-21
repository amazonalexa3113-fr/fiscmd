@echo off
wsl bash -lc "cd '%CD%' && g++ -std=c++20 -Wall -Wextra -Wpedantic -Iinclude src/*.cpp -o fiscmd"
