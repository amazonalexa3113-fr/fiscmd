#include "Program.hpp"
#include "ConsoleUtils.hpp"
#include "FileCommands.hpp"
#include "FileIdentify.hpp"
#include "FisDraw.hpp"
#include "FisScript.hpp"
#include "ProcessUtils.hpp"
#include "SetColor.hpp"
#include "Snake.hpp"
#include "SystemCommands.hpp"
#include "TouchEditor.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <pwd.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace fis {

const std::vector<std::string> commands = {
    "exit", "quit", "info", "inf", "i", "aboutthiscmd",
    "aboutthisconsole", "abtthiscmd", "abtthisconsole",
    "help", "helpmepls", "/?", "?",
    "rng", "calc", "simplecalc",
    "example", "ex", "eg",
    "cls", "color",
    "time", "fulltime", "when", "watsthetime",
    "beep", "watchtime", "crash", "requirement",
    "req", "rq", "require", "required", "currentos",
    "curos", "os", "cd", "mkdir", "rm", "rmdir",
    "toggleShowDir", "toggleshowdir", "touch", "ls",
    "dir", "cat", "copy", "cop", "cope", "move", "mov", "mv",
    "run", "open", "launch", "history", "hist", "his", "clearhistory",
    "clearhist", "clearhis", "clshistory", "clshist", "clshis",
    "showversion", "version", "showver", "ver", "settitle", "title",
    "printfish", "printfis", "prntfish", "prntfis", "prnfis",
    "prnfish", "flipcoin", "flipacoin", "coin", "morecoins", "headandtail",
    "headsandtails", "headsntails", "headntail", "tree",
    "systeminfo", "systeminf", "systemi", "sysinfo", "sysinf", "sysi",
    "memoryinfo", "memoryinf", "memoryi", "meminfo", "meminf",
    "memi", "benchmark", "benchm", "bmark",
    "zip", "zipfolder", "zipfol",
    "unzip", "unzipfile", "unzipfolder", "unzipfol",
    "rename", "ren", "rn (ye, no not rM)",
    "hash", "whoami", "showupdates", "update", "showupdate", "updates",
    "showlog", "showlogs", "logs", "log", "echo", "import",
    "top", "fistop", "taskmgr",
    "killproc", "fiskill", "kill",
    "sudo",
    "fissnake", "snake",
    "fisscript", "script", "scriptfile", "scr",
    "fisdraw", "draw", "fispaint",
    "netwatch",
    "diskparty", "diskusages",
    "fistars", "fisstar", "stars", "star",
    "alias",
    "initfis", "initializefis",
    "toggletruecolor", "togglenocolor",
    "download", "down", "dl",
    "file"
};

void InitProgramState() {
    std::error_code ec;
    currentDir = fs::current_path(ec).string();
    if (currentDir.empty()) currentDir = ".";
}

std::string GetCurrentUserName() {
#ifdef _WIN32
    char buf[256];
    DWORD size = sizeof(buf);
    if (GetUserNameA(buf, &size)) return std::string(buf);
    if (const char* u = std::getenv("USERNAME")) return u;
#else
    if (const char* u = std::getenv("USER")) return u;
    if (passwd* pw = getpwuid(geteuid())) return pw->pw_name;
#endif
    return "user";
}

std::string GetMachineName() {
#ifdef _WIN32
    char buf[256];
    DWORD size = sizeof(buf);
    if (GetComputerNameA(buf, &size)) return std::string(buf);
    if (const char* c = std::getenv("COMPUTERNAME")) return c;
#else
    char buf[256];
    if (gethostname(buf, sizeof(buf)) == 0) return std::string(buf);
#endif
    return "localhost";
}

void toggleShowDir() {
    showDir = !showDir;
}

bool WarnNotImported(bool isImported) {
    if (isImported) return false;
    SetColor(ConsoleColor::Red);
    std::cout << "u haven't imported this command yet bro (use 'import') :sob:\n";
    ResForegroundColor();
    return true;
}

void ImportCmd(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SetColor(ConsoleColor::Cyan);
        std::cout << "available importable commands:\n";
        SetColor(ConsoleColor::Yellow);
        std::cout << "- fissnake (snake)\n";
        std::cout << "- fisscript (script)\n";
        std::cout << "- fisdraw (draw)\n";
        std::cout << "- fistars (stars)\n";
        std::cout << "- download (dl)\n";
        std::cout << "- all (import everything at once)\n";
        ResForegroundColor();
        return;
    }

    std::string target = ToLower(args[1]);
    if (target == "fissnake" || target == "snake") {
        importedFissnake = true;
        SetColor(ConsoleColor::Cyan);
        std::cout << "imported fissnake :D\n";
    } else if (target == "fisscript" || target == "script" || target == "scr") {
        importedFisscript = true;
        SetColor(ConsoleColor::Cyan);
        std::cout << "imported fisscript :D\n";
    } else if (target == "fisdraw" || target == "draw" || target == "fispaint") {
        importedFisdraw = true;
        SetColor(ConsoleColor::Cyan);
        std::cout << "imported fisdraw :D\n";
    } else if (target == "fistars" || target == "fisstar" || target == "stars" || target == "star") {
        importedStars = true;
        SetColor(ConsoleColor::Cyan);
        std::cout << "imported fistars :D\n";
    } else if (target == "download" || target == "down" || target == "dl") {
        importedDown = true;
        SetColor(ConsoleColor::Cyan);
        std::cout << "imported download :D\n";
    } else if (target == "all") {
        importedFissnake = true;
        importedFisscript = true;
        importedFisdraw = true;
        importedStars = true;
        importedDown = true;
        SetColor(ConsoleColor::Cyan);
        std::cout << "imported ALL special commands :D\n";
    } else {
        SetColor(ConsoleColor::Red);
        std::cout << "ts is barely even an importable command\n";
    }
    ResForegroundColor();
}

bool execcmd(const std::vector<std::string>& commandArgs, std::string_view rawInput) {
    if (commandArgs.empty())
        return false;

    auto aliasIt = aliases.find(commandArgs[0]);
    if (aliasIt != aliases.end()) {
        std::string expandedCommand = aliasIt->second;
        for (size_t i = 1; i < commandArgs.size(); ++i) {
            expandedCommand += " " + commandArgs[i];
        }
        auto expandedArgs = ParseQuotedArgs(expandedCommand);
        return execcmd(expandedArgs, expandedCommand);
    }

    std::string cmd = ToLower(commandArgs[0]);

    if (cmd == "exit" || cmd == "quit") {
        SetColor(ConsoleColor::Cyan);
        std::cout << "tysm for using our console app :D\n";
        SetColor(ConsoleColor::White);
        std::cout << "any key to exit sir ;-;\n" << std::flush;
        ReadKey(true);
        ResetColor();
        return true;
    } else if (cmd == "info" || cmd == "inf" || cmd == "i" || cmd == "about" || cmd == "abt" ||
               cmd == "aboutthiscmd" || cmd == "aboutthisconsole" || cmd == "abtthisconsole" || cmd == "abtthiscmd") {
        ShowInfo();
    } else if (cmd == "help" || cmd == "helpmepls" || cmd == "/?" || cmd == "?") {
        ShowHelp();
    } else if (cmd == "rng") {
        RunRng();
    } else if (cmd == "calc" || cmd == "simplecalc") {
        RunCalc();
    } else if (cmd == "example" || cmd == "ex" || cmd == "eg") {
        ShowExample();
    } else if (cmd == "clear" || cmd == "cls") {
        ClearScreen();
        inputStartTop = 0;
    } else if (cmd == "time" || cmd == "when" || cmd == "watsthetime") {
        ShowTime();
    } else if (cmd == "fulltime") {
        ShowFullTime();
    } else if (cmd == "beep") {
        RunBeep(commandArgs);
    } else if (cmd == "watchtime") {
        WatchTime();
    } else if (cmd == "crash") {
        FakeCrash();
    } else if (cmd == "requirement" || cmd == "req" || cmd == "rq" || cmd == "require" || cmd == "required") {
        SetColor(ConsoleColor::DarkMagenta);
        std::cout << "C++20 compiler & standard library (ported from .NET 6.0 LTS)\n";
        ResForegroundColor();
    } else if (cmd == "currentos" || cmd == "curos" || cmd == "os") {
        DetectOS();
    } else if (cmd == "copy" || cmd == "cop" || cmd == "cope") {
        Copy(commandArgs);
    } else if (cmd == "cp") {
        SetColor(ConsoleColor::Red);
        std::cout << "what command are u tryna enter :pray::wilted-rose:\n";
        ResForegroundColor();
    } else if (cmd == "mkdir") {
        Mkdir(commandArgs);
    } else if (cmd == "rmdir") {
        Rmdir(commandArgs);
    } else if (cmd == "rm") {
        Rm(commandArgs);
    } else if (cmd == "cd") {
        Cd(commandArgs);
    } else if (cmd == "toggleshowdir") {
        toggleShowDir();
    } else if (cmd == "touch") {
        Touch(commandArgs);
    } else if (cmd == "ls" || cmd == "dir") {
        ls(commandArgs);
    } else if (cmd == "cat") {
        Cat(commandArgs);
    } else if (cmd == "move" || cmd == "mov" || cmd == "mv") {
        Move(commandArgs);
    } else if (cmd == "run" || cmd == "open" || cmd == "launch") {
        RunCommand(commandArgs, rawInput);
    } else if (cmd == "toggletruecolor") {
        if (nocolormode) {
            std::cout << "why would u enter this command when u asked for no color\n";
            return false;
        }
#ifdef _WIN32
        SetColor(ConsoleColor::Red);
        std::cout << "NO SHIT UR cmd.exe DO NOT HAVE TRUE COLOR SUPPORT GNG\n";
        ResForegroundColor();
        return false;
#else
        truecolor = !truecolor;
        SetColor(ConsoleColor::Cyan);
        if (truecolor) {
            std::cout << "true color mode is on :D\n";
            std::cout << "experience the ultra 16 million colors\n";
        } else {
            std::cout << "true color mode is off :D\n";
        }
        ResForegroundColor();
#endif
    } else if (cmd == "togglenocolor") {
        nocolormode = !nocolormode;
        if (nocolormode) {
            ResetColor();
            std::cout << "no color mode is on :D\n";
            std::cout << "colors have been fucking evicted\n";
        } else {
            SetColor(ConsoleColor::Cyan);
            std::cout << "no color mode is off :D\n";
            ResForegroundColor();
        }
    } else if (cmd == "history" || cmd == "his" || cmd == "hist") {
        ShowHistory();
    } else if (cmd == "clearhistory" || cmd == "clshistory" || cmd == "clearhis" ||
               cmd == "clshis" || cmd == "clearhist" || cmd == "clshist") {
        ClearHistory();
    } else if (cmd == "showversion" || cmd == "showver" || cmd == "version" || cmd == "ver" || cmd == "v") {
        ShowVersion();
    } else if (cmd == "settitle" || cmd == "title") {
        SetTitle(commandArgs);
#ifdef __linux__
        SetColor(ConsoleColor::Yellow);
        std::cout << "honorable mention: giving ur terminal a custom title wouldn't work on some certain terminals if ur on linux, especially Konsole\n";
        ResForegroundColor();
#endif
    } else if (cmd == "printfis" || cmd == "printfish" || cmd == "prnfish" ||
               cmd == "prnfis" || cmd == "prntfish" || cmd == "prntfis") {
        PrintFis();
    } else if (cmd == "flipcoin" || cmd == "flipacoin" || cmd == "coin" || cmd == "morecoins" ||
               cmd == "morecoin" || cmd == "headntail" || cmd == "headsntails" ||
               cmd == "headandtail" || cmd == "headsandtails") {
        FlipCoin();
    } else if (cmd == "tree") {
        TreeCommand(commandArgs);
    } else if (cmd == "systeminfo" || cmd == "systeminf" || cmd == "systemi" ||
               cmd == "sysinfo" || cmd == "sysinf" || cmd == "sysi") {
        SysInfo();
    } else if (cmd == "memoryinfo" || cmd == "memoryinf" || cmd == "memoryi" ||
               cmd == "meminfo" || cmd == "meminf" || cmd == "memi") {
        MemInfo();
    } else if (cmd == "benchmark" || cmd == "bmark" || cmd == "benchm") {
        Benchmark();
    } else if (cmd == "zip" || cmd == "zipfolder" || cmd == "zipfol") {
        ZipFolder(commandArgs);
    } else if (cmd == "unzip" || cmd == "unzipfile" || cmd == "unzipfolder" || cmd == "unzipfol") {
        UnzipFile(commandArgs);
    } else if (cmd == "rename" || cmd == "ren" || cmd == "rn") {
        Rename(commandArgs);
    } else if (cmd == "hash" || cmd == "sha256") {
        HashFile(commandArgs);
    } else if (cmd == "whoami" || cmd == "%userprofile%") {
        ShowUser();
    } else if (cmd == "echo") {
        ResetColor();
        DoEcho(rawInput);
        ResForegroundColor();
    } else if (cmd == "showupdates" || cmd == "showupdate" || cmd == "showlogs" || cmd == "showlog" ||
               cmd == "updates" || cmd == "update" || cmd == "logs" || cmd == "log") {
        ShowUpdate();
    } else if (cmd == "initializefis" || cmd == "initfis") {
        if (commandArgs.size() > 1 && (EqualsIgnoreCase(commandArgs[1], "animate") || EqualsIgnoreCase(commandArgs[1], "anim"))) {
            AnimateFisCoolAsf();
        } else {
            PrintFisCoolAsf(true, 25, false);
            TypeWrite(" fiscmd initialized, welcome :D", 25, false);
            ResForegroundColor();
            std::cout << '\n';
        }
    } else if (cmd == "netwatch") {
        NetWatch();
    } else if (cmd == "diskparty") {
        DiskParty();
    } else if (cmd == "alias" || cmd == "aliases") {
        if (commandArgs.size() == 1) {
            if (aliases.empty()) {
                SetColor(ConsoleColor::Red);
                std::cout << "no aliases\n";
                ResForegroundColor();
                return false;
            }
            for (const auto& [k, v] : aliases) {
                SetColor(ConsoleColor::Cyan);
                std::cout << k << " >> " << v << ", gotcha\n";
            }
            ResForegroundColor();
            return false;
        }
        if (commandArgs.size() == 2) {
            const std::string& aliasName = commandArgs[1];
            auto it = aliases.find(aliasName);
            if (it != aliases.end()) {
                SetColor(ConsoleColor::Cyan);
                std::cout << aliasName << " >> " << it->second << ", gotcha\n";
            } else {
                SetColor(ConsoleColor::Red);
                std::cout << "alias '" << aliasName << "' not found\n";
            }
            ResForegroundColor();
            return false;
        }
        std::string newAliasName = commandArgs[1];
        std::string newAliasCommand;
        for (size_t i = 2; i < commandArgs.size(); ++i) {
            if (i > 2) newAliasCommand += ' ';
            newAliasCommand += commandArgs[i];
        }
        aliases[newAliasName] = newAliasCommand;
        std::cout << "alias created: " << newAliasName << " -> " << newAliasCommand << "\n";
    } else if (cmd == "file") {
        FileInfoCommand(commandArgs);
    } else if (cmd == "importcmd" || cmd == "import") {
        ImportCmd(commandArgs);
    } else if (cmd == "fissnake" || cmd == "snake") {
        if (!WarnNotImported(importedFissnake)) Snake(commandArgs);
    } else if (cmd == "fisscript" || cmd == "scriptfile" || cmd == "script" || cmd == "scr") {
        if (!WarnNotImported(importedFisscript)) FisScript(commandArgs);
    } else if (cmd == "fisdraw" || cmd == "draw" || cmd == "fispaint") {
        if (!WarnNotImported(importedFisdraw)) FisDraw(commandArgs);
    } else if (cmd == "fisstar" || cmd == "fisstars" || cmd == "star" || cmd == "stars") {
        if (!WarnNotImported(importedStars)) Stars();
    } else if (cmd == "download" || cmd == "down" || cmd == "dl") {
        if (!WarnNotImported(importedDown)) DownCommand(commandArgs);
    } else if (cmd == "top" || cmd == "fistop" || cmd == "taskmgr") {
        Taskmgr(commandArgs);
    } else if (cmd == "kill" || cmd == "fiskill" || cmd == "killproc") {
        KillProc(commandArgs);
    } else if (cmd == "#") {
        // ragebait command
    } else {
        if (commandArgs[0] == "color") {
            SetColorTerminal(rawInput);
        } else if (!LaunchProgram(commandArgs)) {
            SetColor(ConsoleColor::Red);
            std::cout << "ts is barely even a command\n";
            ResForegroundColor();
        }
    }

    return false;
}

} // namespace fis
