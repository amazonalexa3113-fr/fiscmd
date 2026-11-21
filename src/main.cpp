/*

><>$ echo HAVE A HAVE A LOOK ONE POUND FIS

tysm for using my open source fiscmd console app :D

ported to C++20 from the original .NET 6.0 C# console app
cross platform :D (works well + tested on windows + real linux)

its open source so u can edit/add anything here and give it a name as long as ur not copying me :skull:
especially editing the PrintPrompt() function, cuz thats where the ><>$ thing comes from

bye :D
*/

#include "ConsoleUtils.hpp"
#include "Program.hpp"
#include "SetColor.hpp"
#include "SystemCommands.hpp"

#include <csignal>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {

#ifdef _WIN32
BOOL WINAPI ConsoleCtrlHandler(DWORD ctrlType) {
    if (ctrlType == CTRL_C_EVENT) {
        return TRUE; // ignore Ctrl+C just like Console.CancelKeyPress += (s, e) => e.Cancel = true;
    }
    return FALSE;
}
#else
void SigIntHandler(int) {
    // ignore SIGINT in interactive shell
}
#endif

} // namespace

int main(int argc, char* argv[]) {
    using namespace fis;

    InitConsoleColors();
    InitProgramState();

    // preventing [CTRL] + C (SIGINT) termination
#ifdef _WIN32
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
#else
    std::signal(SIGINT, SigIntHandler);
#endif

#ifdef __linux__
    std::string sonwhat = "./fiscmd";
#else
    std::string sonwhat = "fiscmd.exe";
#endif

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

    if (!args.empty()) {
        std::vector<std::string> filtered;
        for (const auto& x : args) {
            if (EqualsIgnoreCase(x, "-nc") || EqualsIgnoreCase(x, "--no-color")) {
                nocolormode = true;
            } else if (EqualsIgnoreCase(x, "-tc") || EqualsIgnoreCase(x, "--true-color")) {
                truecolor = true;
            } else {
                filtered.push_back(x);
            }
        }
        args = std::move(filtered);

        if (!args.empty()) {
            std::string flag = ToLower(args[0]);
            if (flag == "-h" || flag == "--help" || flag == "-?") {
                SetColor(ConsoleColor::Cyan);
                std::cout << "fiscmd - a silly little console app :D\n\n";
                std::cout << "usage:\n";
                std::cout << sonwhat << "          open interactive shell\n";
                std::cout << sonwhat << " -h (or --help, maybe even -?) - show this help\n";
                std::cout << sonwhat << " -v (or --version) - show the current version of ts\n";
                std::cout << sonwhat << " -c (or --command) - execute any command fiscmd currently has\n";
                std::cout << sonwhat << " -nc (or --no-color) - toggle no color mode (add -c next to it to execute commands like normal)\n";
                std::cout << sonwhat << " -tc (or --true-color) - toggle true color mode (16 million color experience, add -c next to it to execute commands like normal)\n";
                std::cout << "honorable mention: \"" << sonwhat << " -q\" (or --quit) do nothing, in fact... this command is a ragebait :skull:\n";
                ResetColor();
                return 0;
            } else if (flag == "-c" || flag == "--command") {
                if (args.size() < 2) {
                    SetColor(ConsoleColor::Red);
                    std::cout << "where command\n";
                    ResetColor();
                    return 1;
                }
                std::vector<std::string> commandArgs(args.begin() + 1, args.end());
                std::string commandInput;
                for (size_t i = 0; i < commandArgs.size(); ++i) {
                    if (i > 0) commandInput += ' ';
                    commandInput += commandArgs[i];
                }
                execcmd(commandArgs, commandInput);
                ResetColor();
                return 0;
            } else if (flag == "-v" || flag == "--version") {
                ShowVersion();
                ResetColor();
                return 0;
            } else if (flag == "-a" || flag == "--alias") {
                SetColor(ConsoleColor::Red);
                std::cout << "uhm... that argument got removed from v1.9 beta what are you doing here\n";
                ResetColor();
                return 1;
            } else if (flag == "-q" || flag == "--quit" || flag == "--exit") {
                std::cout << "get ragebaited\n";
                return 0;
            } else {
                SetColor(ConsoleColor::Red);
                std::cout << "unknown argument: " << args[0] << "\n";
                std::cout << "use '" << sonwhat << " -h' for help\n";
                ResetColor();
                return 1;
            }
        }
    }

    SetCursorVisible(false);

    if (truecolor) {
        SetConsoleTitle("have a have a look one pound fi");
        AnimateFisCoolAsf();
        ResetColor();
    } else {
        SetConsoleTitle("have a have a look one pound fis ><>");
        SetColor(ConsoleColor::Cyan);
        PrintFisCoolAsf(true, 25, false);
        TypeWrite(" fiscmd initialized, welcome :D", 25, false);
        std::cout << '\n';
        ResetColor();
    }

    SetCursorVisible(true);

    while (true) {
        SetColor(currentFg);
        SetColor(currentBg, true);

        PrintPrompt();
        SetColor(currentFg);

        std::string input = ReadCommand();
        if (Trim(input).empty()) continue;

        history.push_back(input);
        historyIndex = static_cast<int>(history.size());

        std::string rawInput = Trim(input);
        std::vector<std::string> parts = ParseQuotedArgs(rawInput);

        if (execcmd(parts, rawInput))
            return 0;
    }
}
