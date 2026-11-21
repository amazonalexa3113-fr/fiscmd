#include "SystemCommands.hpp"
#include "ConsoleUtils.hpp"
#include "Program.hpp"
#include "SetColor.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <thread>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>
#include <winhttp.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#endif

namespace fs = std::filesystem;

namespace fis {

namespace {

std::string FormatBytes(uint64_t bytes) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    if (bytes >= (1ULL << 40)) {
        oss << (bytes / static_cast<double>(1ULL << 40)) << " TiB";
    } else if (bytes >= (1ULL << 30)) {
        oss << (bytes / static_cast<double>(1ULL << 30)) << " GiB";
    } else if (bytes >= (1ULL << 20)) {
        oss << (bytes / static_cast<double>(1ULL << 20)) << " MiB";
    } else {
        oss << (bytes / 1024.0) << " KiB";
    }
    return oss.str();
}

bool IsWholeDisk(std::string_view diskName) {
    if (diskName.starts_with("sd") && diskName.size() == 3)
        return true;
    if (diskName.starts_with("nvme") && diskName.find('n') != std::string_view::npos && diskName.ends_with("n1"))
        return true;
    if (diskName.starts_with("mmcblk") && diskName.size() > 6 && std::isdigit(static_cast<unsigned char>(diskName[6])))
        return true;
    return false;
}

void DownloadFile(const std::string& url, const std::string& outputPath) {
#ifdef _WIN32
    std::wstring wUrl(url.begin(), url.end());
    URL_COMPONENTS uc{};
    uc.dwStructSize = sizeof(uc);
    wchar_t host[256]{}, path[2048]{};
    uc.lpszHostName = host;
    uc.dwHostNameLength = 256;
    uc.lpszUrlPath = path;
    uc.dwUrlPathLength = 2048;

    if (!WinHttpCrackUrl(wUrl.c_str(), 0, 0, &uc)) {
        SetColor(ConsoleColor::Red);
        std::cout << "download failed: invalid URL\n";
        ResForegroundColor();
        return;
    }

    HINTERNET hSession = WinHttpOpen(L"fiscmd/1.975", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    HINTERNET hConnect = hSession ? WinHttpConnect(hSession, host, uc.nPort, 0) : nullptr;
    DWORD flags = (uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = hConnect ? WinHttpOpenRequest(hConnect, L"GET", path, nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags) : nullptr;

    bool ok = false;
    if (hRequest && WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
        WinHttpReceiveResponse(hRequest, nullptr)) {
        std::ofstream out(outputPath, std::ios::binary);
        if (out) {
            DWORD bytesAvail = 0;
            while (WinHttpQueryDataAvailable(hRequest, &bytesAvail) && bytesAvail > 0) {
                std::vector<char> buf(bytesAvail);
                DWORD downloaded = 0;
                if (WinHttpReadData(hRequest, buf.data(), bytesAvail, &downloaded)) {
                    out.write(buf.data(), downloaded);
                }
            }
            ok = true;
        }
    }

    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);

    if (ok) {
        SetColor(ConsoleColor::Cyan);
        std::cout << "downloaded: " << outputPath << "\n";
    } else {
        SetColor(ConsoleColor::Red);
        std::cout << "download failed\n";
    }
    ResForegroundColor();
#else
    std::string cmd = "curl -fsSL \"" + url + "\" -o \"" + outputPath + "\"";
    int rc = std::system(cmd.c_str());
    if (rc == 0) {
        SetColor(ConsoleColor::Cyan);
        std::cout << "downloaded: " << outputPath << "\n";
    } else {
        SetColor(ConsoleColor::Red);
        std::cout << "download failed (curl exit code " << rc << ")\n";
    }
    ResForegroundColor();
#endif
}

} // namespace

void ShowUser() {
    SetColor(ConsoleColor::Cyan);

#ifdef _WIN32
    char username[256];
    DWORD usernameSize = sizeof(username);

    if (GetUserNameA(username, &usernameSize))
        std::cout << "ur " << username << "\n";
    else
        std::cout << "ur unknown\n";
#else
    const char* username = std::getenv("USER");
    std::cout << "ur " << (username ? username : "unknown") << "\n";
#endif

    std::cout << "u can still see ur name by using the command \"sysinfo\"\n";
    ResForegroundColor();
}

void ShowInfo() {
    SetColor(ConsoleColor::Cyan);
    std::cout << "tysm for using our console app :D\n\n";
    SetColor(ConsoleColor::Yellow);
    std::cout << "welcome to a console app i call \"fiscmd\" :D\n";
    SetColor(ConsoleColor::Cyan);
    std::cout << "in this very simple console app i made...\n";
    std::cout << "u can do uh...\n";
    SetColor(ConsoleColor::Yellow);
    std::cout << "pretty basic stuff ;-;\n\n";
    ResetColor();
    std::cout << "basically thats it\n";
    std::cout << ":ishowspeed-my-mom-is-kinda-homeless:\n\n";
    SetColor(ConsoleColor::Red);
    std::cout << "also forgr to mention ts screams for C++20 (originally .NET 6.0 LTS)\n";
    std::cout << "ye no if u do ctrl + c this would auto exits\n";
    std::cout << "(IF u have ctrl + shift + c copy method enabled in windows or ur on linux)\n\n";
    SetColor(ConsoleColor::Cyan);
    std::cout << "see ya! :D\n";
    ResForegroundColor();
}

void RunBeep(const std::vector<std::string>& parts) {
    int freq = 1000;
    int dur = 200;
    bool hasFreq = false;
    bool hasDur = false;
    bool useDefault = false;

    if (parts.size() == 1) {
        SetColor(ConsoleColor::Cyan);
        std::cout << "usage: beep [-freq hz] [-dur ms] (-default)\n";
        std::cout << "(-default) - optional switch, will plays a 1000 hz frequency for 200 millesec\n";
        std::cout << "[-freq hz] - targetted frequency ranged from 37 to 32767 hz\n";
        std::cout << "[-dur ms] - duration of the beep (counts in ms)\n";
        SetColor(ConsoleColor::Red);
        std::cout << "\nalso forgr to mention that these switches are basically useless on linux :sob:\n";
        ResForegroundColor();
        return;
    }

    for (size_t i = 1; i < parts.size(); i++) {
        std::string arg = ToLower(parts[i]);
        if (arg == "-freq") {
            if (i + 1 < parts.size()) {
                try {
                    freq = std::stoi(parts[i + 1]);
                    hasFreq = true;
                    i++;
                    continue;
                } catch (...) {}
            }
            SetColor(ConsoleColor::Cyan);
            std::cout << "usage: beep [-freq hz]\n";
            std::cout << "[-freq hz] - targetted frequency ranged from 37 to 32767 hz\n";
            ResForegroundColor();
            return;
        } else if (arg == "-dur") {
            if (i + 1 < parts.size()) {
                try {
                    dur = std::stoi(parts[i + 1]);
                    hasDur = true;
                    i++;
                    continue;
                } catch (...) {}
            }
            SetColor(ConsoleColor::Cyan);
            std::cout << "usage: beep [-freq hz] [-dur ms] (-default)\n";
            std::cout << "[-dur ms] - duration of the beep (counts in ms)\n";
            ResForegroundColor();
            return;
        } else if (arg == "-default") {
            useDefault = true;
        } else {
            SetColor(ConsoleColor::Red);
            std::cout << "fym by that vro :sob::pray:\n";
            ResForegroundColor();
            return;
        }
    }

    if (useDefault && (hasFreq || hasDur)) {
        SetColor(ConsoleColor::Red);
        std::cout << "fym by that vro :sob::pray:\n";
        ResForegroundColor();
        return;
    }

    if (useDefault) {
        freq = 1000;
        dur = 200;
    }

    if (freq < 37 || freq > 32767) {
        SetColor(ConsoleColor::Red);
        std::cout << "freq outta range bro (37 - 32767)\n";
        ResForegroundColor();
        return;
    }

    if (dur <= 0) {
        SetColor(ConsoleColor::Red);
        std::cout << "duration gotta be > 0 :sob::pray:\n";
        ResForegroundColor();
        return;
    }

#ifdef _WIN32
    Beep(static_cast<DWORD>(freq), static_cast<DWORD>(dur));
#else
    SetColor(ConsoleColor::Red);
    std::cout << "failed to beep: ur on linux\n";
#endif
    ResForegroundColor();
}

void DetectOS() {
#ifdef _WIN32
    SetColor(ConsoleColor::Cyan);
    std::cout << "ur currently on windows\n";
#elif defined(__linux__)
    SetColor(ConsoleColor::Cyan);
    std::cout << "ur currently on linux\n";
#else
    SetColor(ConsoleColor::Red);
    std::cout << "what os even is this bro\n";
#endif
    ResForegroundColor();
}

void WatchTime() {
    std::cout << "press any key to stop...\n\n";

    while (!KeyAvailable()) {
        std::time_t now = std::time(nullptr);
        std::tm tmBuf{};
#ifdef _WIN32
        localtime_s(&tmBuf, &now);
#else
        localtime_r(&now, &tmBuf);
#endif
        char buf[16];
        std::strftime(buf, sizeof(buf), "%H:%M:%S", &tmBuf);
        std::cout << "\r" << buf << std::flush;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    ReadKey(true);
    std::cout << '\n';
    ResForegroundColor();
}

void FakeCrash() {
    ClearScreen();
    SetColor(ConsoleColor::DarkBlue, true);
    SetColor(ConsoleColor::White);
    ClearScreen();

    std::cout << "A problem has been detected and Windows has been shut down to prevent damage\n";
    std::cout << "to your computer.\n\n";
    std::cout << "IRQL_NOT_LESS_OR_EQUAL\n\n";
    std::cout << "If this is the first time you've seen this Stop error screen,\n";
    std::cout << "restart your computer. If this screen appears again, follow\n";
    std::cout << "these steps:\n\n";
    std::cout << "Check to make sure any new hardware or software is properly installed.\n";
    std::cout << "If this is a new installation, ask your hardware or software manufacturer\n";
    std::cout << "for any Windows updates you might need.\n\n";
    std::cout << "If problems continue, disable or remove any newly installed hardware\n";
    std::cout << "or software. Disable BIOS memory options such as caching or shadowing.\n";
    std::cout << "If you need to use Safe Mode to remove or disable components, restart\n";
    std::cout << "your computer, press F8 to select Advanced Startup Options, and then\n";
    std::cout << "select Safe Mode.\n\n";
    std::cout << "Technical Information:\n\n";
    std::cout << "*** STOP: 0x0000000A (0xXXXXXXXX, 0xXXXXXXXX, 0xXXXXXXXX, 0xXXXXXXXX)\n" << std::flush;

    SetCursorVisible(false);
    ReadKey(true);
    SetCursorVisible(true);
    ResForegroundColor();
    SetColor(currentBg, true);
    ClearScreen();
}

void ShowHelp() {
    SetColor(ConsoleColor::Yellow);
    std::cout << "welcome to the help dialog1!1!11!1!!\n";
    std::cout << "today imma show u all apps + their command lines\n";
    std::cout << "that are available on this console app\n\n";
    std::cout << "full app name - their cmd line\n";
    std::cout << "about this simple console app - info / inf / i / aboutthisconsole / aboutthiscmd / abtthisconsole / abtthiscmd\n";
    std::cout << "requirement to run this program - requirement / req / rq / require / required\n";
    std::cout << "example dialog - example / ex / eg\n";
    std::cout << "random number generator - rng\n";
    std::cout << "simple calculator - calc / simplecalc\n";
    std::cout << "clear screen - cls\n";
    std::cout << "change colors - color <fg> [bg]\n";
    std::cout << "reset colors - color default\n";
    std::cout << "echo text - echo <text>\n";
    std::cout << "time - time / when / watsthetime\n";
    std::cout << "full time (date + time) - fulltime\n";
    std::cout << "beep sound - beep\n";
    std::cout << "live clock - watchtime\n";
    std::cout << "fake crash screen - crash\n";
    std::cout << "display ur current operating system - currentos / curos / os\n";
    SetColor(ConsoleColor::DarkYellow);
    std::cout << "(spoiler alert: may only checks the real os, wsl linux would still\n";
    std::cout << "be counted as windows)\n";
    SetColor(ConsoleColor::Yellow);
    std::cout << "view logs - log / update (theres still more in tab autocorrect)\n";
    std::cout << "exit app - exit (or press ctrl + c)\n";
    SetColor(ConsoleColor::DarkYellow);
    std::cout << "(alternatively u can press ctrl + c to exit\n";
    std::cout << "unless ur on windows without ctrl + shift + c copying method enabled)\n";
    SetColor(ConsoleColor::Yellow);
    std::cout << "show disk storage n stuff - diskparty\n";
    std::cout << "ping 1.1.1.1 and test network - netwatch\n";
    std::cout << "toggle on true color - toggletruecolor\n";
    std::cout << "activate non-color mode and annihilate all colors - togglenocolor\n";
    std::cout << "get \"man fiscmd\" support - man\n\n";

    SetColor(ConsoleColor::Red);
    std::cout << "stuff that has stuff to do with files (be careful)\n";
    SetColor(ConsoleColor::Yellow);
    std::cout << "browse through directory/folder - cd\n";
    std::cout << "remove directory/folder - rmdir (or \"rm -rf\" to delete both files that are in it\n";
    std::cout << "create directory/folder - mkdir\n";
    std::cout << "create/modify a text document (or any other type) file - touch\n";
    std::cout << "list out a directory/folder - ls / dir\n";
    std::cout << "read out a files content - cat\n";
    std::cout << "copy a source/file/folder to a destination - copy / cop / cope\n";
    SetColor(ConsoleColor::DarkRed);
    std::cout << "(i am NOT be adding the cp command :sob::pray:)\n";
    SetColor(ConsoleColor::Yellow);
    std::cout << "move a source/file/folder to a destination - move / mov / mv\n";
    std::cout << "run a file - run / open / launch\n";
    std::cout << "zip (or unzip) a file/folder - zip / unzip / (more in tab autocor)\n";
    std::cout << "rename a file/folder - rename / ren / rn (ye, no, rN not rM\n";
    std::cout << "view a files/sources has (sha-256) -  hash / sha256\n\n";
    SetColor(ConsoleColor::Cyan);

    std::cout << "commands that no one asked for\n";
    SetColor(ConsoleColor::Yellow);
    std::cout << "show current directory (><[current dir]>$ ) - toggleShowDir (or lowercase: toggleshowdir)\n";
    std::cout << "flip coin - coin / morecoins / flipcoin / flipacoin / headsntails / (more in tab autocorrect)\n";
    std::cout << "show the \"fiscmd initialized...\" message earlier - initializefis / initfis\n";
    SetColor(ConsoleColor::DarkYellow);
    std::cout << "honorable mention: entering \"initfis animate\" while having true color mode on will play a peak animation of the same line\n";
    SetColor(ConsoleColor::Yellow);
    std::cout << "\"exit\" command but translated ragebaitly - quit\n\n";

    std::cout << "NEW COMMAND - import / importcmd\nit is said that bro can actually import SPECIAL commands :0\n";
    SetColor(ConsoleColor::Cyan);
    std::cout << "how to use: import <specialcmd>\n";
    std::cout << "<specialcmd> - special commands needed to import before using\n";
    SetColor(ConsoleColor::Yellow);
    std::cout << "\nfor list of importable commands, type \"import\" and it'll shows the list :D\n";

    SetColor(ConsoleColor::Red);
    std::cout << "\nhonorable mention: tab autocorrect is obselette by the syntax highlight\n";
    SetColor(ConsoleColor::DarkCyan);
    std::cout << "the tab autocorrect code still remains in the source code\n";

    ResForegroundColor();
}

void RunRng() {
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, 100);
    int num = dist(rng);

    SetColor(ConsoleColor::Magenta);
    std::cout << "random number (0-100): " << num << "\n";
    ResForegroundColor();
}

void RunCalc() {
    try {
        std::string lineA, op, lineB;
        std::cout << "enter first number: " << std::flush;
        std::getline(std::cin, lineA);
        double a = std::stod(lineA);

        std::cout << "operator (+ - * / ^): " << std::flush;
        std::getline(std::cin, op);
        op = Trim(op);

        std::cout << "enter second number: " << std::flush;
        std::getline(std::cin, lineB);
        double b = std::stod(lineB);

        double result = 0.0;
        if (op == "+") result = a + b;
        else if (op == "-") result = a - b;
        else if (op == "*") result = a * b;
        else if (op == "/") {
            if (b == 0.0) throw std::runtime_error("div zero");
            result = a / b;
        } else if (op == "^" || op == "**") {
            result = std::pow(a, b);
        } else {
            SetColor(ConsoleColor::Red);
            std::cout << "invalid operator\n";
            ResForegroundColor();
            return;
        }

        SetColor(ConsoleColor::Blue);
        std::cout << "result: " << result << "\n";
        ResForegroundColor();
    } catch (...) {
        SetColor(ConsoleColor::Red);
        std::cout << "calc error\n";
        ResForegroundColor();
    }
}

void ShowExample() {
    SetColor(ConsoleColor::White);
    SetColor(ConsoleColor::DarkBlue, true);
    std::cout << "   EXAMPLE   \n";
    ResForegroundColor();
}

void ShowTime() {
    std::time_t now = std::time(nullptr);
    std::tm tmBuf{};
#ifdef _WIN32
    localtime_s(&tmBuf, &now);
#else
    localtime_r(&now, &tmBuf);
#endif
    char buf[16];
    std::strftime(buf, sizeof(buf), "%H:%M:%S", &tmBuf);
    SetColor(ConsoleColor::Cyan);
    std::cout << buf << "\n";
    ResForegroundColor();
}

void ShowFullTime() {
    std::time_t now = std::time(nullptr);
    std::tm tmBuf{};
#ifdef _WIN32
    localtime_s(&tmBuf, &now);
#else
    localtime_r(&now, &tmBuf);
#endif
    char buf[64];
    std::strftime(buf, sizeof(buf), "%A, %d %B %Y %H:%M:%S", &tmBuf);
    SetColor(ConsoleColor::Cyan);
    std::cout << buf << "\n";
    ResForegroundColor();
}

void DoEcho(std::string_view input) {
    std::vector<std::string> parts;
    size_t pos = 0;
    while (pos <= input.size()) {
        size_t next = input.find(' ', pos);
        if (next == std::string_view::npos) {
            parts.emplace_back(input.substr(pos));
            break;
        }
        parts.emplace_back(input.substr(pos, next - pos));
        pos = next + 1;
    }

    bool write = false;
    int delay = 25;
    size_t index = 1;

    auto ShowUsage = [](const char* flag = nullptr) {
        SetColor(ConsoleColor::Cyan);
        std::cout << "usage: echo [-write] [-delay ms] [text]\n";
        if (flag == nullptr) {
            std::cout << "[text] - the text needs to be inputted\n";
            std::cout << "[-write] - type the inputted text slowly instead of instanly prints them\n";
            std::cout << "[-delay ms] (requires [-write]) - delay before typing the next letter (counts in ms)\n";
            std::cout << "also [-delay ms] defaults at 25 ms\n";
        } else if (std::string_view(flag) == "-write") {
            std::cout << "[-write] - type the inputted text slowly instead of instanly prints them\n";
        } else if (std::string_view(flag) == "-delay") {
            std::cout << "[-delay ms] (requires [-write]) - delay before typing the next letter (counts in ms)\n";
            std::cout << "also [-delay ms] defaults at 25 ms\n";
        }
        ResForegroundColor();
    };

    if (parts.size() == 1) {
        ShowUsage();
        return;
    }

    while (index < parts.size() && parts[index].starts_with("-")) {
        std::string flag = ToLower(parts[index]);
        if (flag == "-write") {
            write = true;
            index++;
        } else if (flag == "-delay") {
            if (index + 1 < parts.size()) {
                try {
                    delay = std::stoi(parts[index + 1]);
                    index += 2;
                    continue;
                } catch (...) {}
            }
            ShowUsage("-delay");
            return;
        } else {
            index++;
        }
    }

    if (index >= parts.size()) {
        ShowUsage(write ? "-write" : nullptr);
        return;
    }

    std::string text;
    for (size_t i = index; i < parts.size(); ++i) {
        if (i > index) text += ' ';
        text += parts[i];
    }

    if (!write) {
        std::cout << text << '\n';
    } else {
        for (char c : text) {
            std::cout << c << std::flush;
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        }
        std::cout << '\n';
    }
}

void ShowHistory() {
    SetColor(ConsoleColor::Cyan);
    if (history.empty()) {
        SetColor(ConsoleColor::Red);
        std::cout << "no history yet :c\n";
        ResForegroundColor();
        return;
    }

    for (size_t i = 0; i < history.size(); i++) {
        std::cout << (i + 1) << ". " << history[i] << "\n";
    }
    ResForegroundColor();
}

void ClearHistory() {
    history.clear();
    historyIndex = -1;

    SetColor(ConsoleColor::DarkRed);
    std::cout << "ur entire history is nuked, no more undo\n";
    ResForegroundColor();
}

void ShowVersion() {
    SetColor(ConsoleColor::Yellow);
    std::cout << "fiscmd v2 final ><>\n\n";
    SetColor(ConsoleColor::DarkYellow);
    std::cout << "NVIDIA GEFORCE RTX 6770 FOUNDER ULTRAHYPERMAX DUO DUPLEX DELUXE MAX PRO PLUS GALAXY EDITION ULTRA\n";
    ResForegroundColor();
}

void SetTitle(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SetColor(ConsoleColor::Cyan);
        std::cout << "usage: title <text>\n";
        std::cout << "<text> is the new console title\n";
        SetColor(ConsoleColor::Red);
        std::cout << "whatever u do... DONT USE CURSED CHARACTERS FOR TS :skull:\n";
        ResForegroundColor();
        return;
    }

    std::string title;
    for (size_t i = 1; i < args.size(); ++i) {
        if (i > 1) title += ' ';
        title += args[i];
    }
    #ifdef _WIN32
    SetConsoleTitleA(title.c_str());
    #else
    std::cout << "\033]0;" << title << "\007";
    #endif

    SetColor(ConsoleColor::Cyan);
    std::cout << "title changed :D\n";
    ResForegroundColor();
}

void PrintFis() {
    ResForegroundColor();
    std::cout << "><> look de fis :0\n";
    ResForegroundColor();
}

void FlipCoin() {
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, 1);

    SetColor(ConsoleColor::Cyan);
    std::cout << (dist(rng) == 0 ? "heads\n" : "tails\n");
    ResForegroundColor();
}

void SysInfo() {
    SetColor(ConsoleColor::Yellow);
    std::cout << "machine: " << GetMachineName() << "\n";
#ifdef _WIN32
    char username[256];
    DWORD usernameSize = sizeof(username);

    if (GetUserNameA(username, &usernameSize))
        std::cout << "user: " << username << "\n";
    else
        std::cout << "user: unknown\n";
#else
    const char* username = std::getenv("USER");
    std::cout << "user: " << (username ? username : "unknown") << "\n";
#endif
#ifdef _WIN32
    std::cout << "os: Microsoft Windows\n";
#elif defined(__linux__)
    std::cout << "os: Linux\n";
#else
    std::cout << "os: Unix-like\n";
#endif
    std::cout << "64 bit os yes or no: " << (sizeof(void*) == 8 ? "yes\n" : "nah\n");
    std::cout << "runtime: C++20 native\n";
    ResForegroundColor();
}

void MemInfo() {
    uint64_t mem = 0;
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc{};
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        mem = pmc.WorkingSetSize;
    }
#elif defined(__linux__)
    std::ifstream statm("/proc/self/statm");
    uint64_t sizePages = 0, residentPages = 0;
    if (statm >> sizePages >> residentPages) {
        mem = residentPages * 4096ULL;
    }
#endif
    SetColor(ConsoleColor::Yellow);
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "working set: " << mem << " bytes\n";
    std::cout << "working set: " << (mem / 1024.0) << " KiB\n";
    std::cout << "working set: " << (mem / (1024.0 * 1024.0)) << " MiB\n";
    ResForegroundColor();
}

void Benchmark() {
    auto start = std::chrono::steady_clock::now();
    volatile int sink = 0;
    for (int i = 0; i < 1000000; i++) {
        sink = i * i;
    }
    auto end = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    SetColor(ConsoleColor::Cyan);
    std::cout << "benchmark done in " << ms << " ms\n";
    ResForegroundColor();
}

void ShowUpdate() {
    SetColor(ConsoleColor::Yellow);
    TypeWrite("v2 beta logs (press any key for each next log ok):\n");
    ReadKey(true);
    TypeWrite("- \"initfis animate\" wouldn't show the gradient on windows (or on linux without true color mode on) since cmd.exe doesn't support true color");
    ReadKey(true);
    TypeWrite("- added a -match flag to the \"hash\" command :D");
    ReadKey(true);
    TypeWrite("- NEW IMPORTABLE COMMAND: download");
    ReadKey(true);
    TypeWrite("- made rm rejects the infamous -rf / on linux (and C: for windows)");
    ReadKey(true);
    TypeWrite("- obliterated the requirement label off the version shower (the \"ver\" command, yk?)");
    ReadKey(true);
    TypeWrite("- fixed the prompt showing the same color as the syntax hightligting color from v1.8 beta or v1.9 beta");
    ReadKey(true);
    TypeWrite("- shortend the \"animate\" switch from command \"initfis\" (\"initfis animate\") to \"anim\"");
    ReadKey(true);
    TypeWrite("- added syntax highlighting to the \"touch\" command");
    ReadKey(true);
    TypeWrite("- also fixed the syntax hightligting having stroke when no color mode is on");
    ReadKey(true);
    TypeWrite("- because i don't wanna waste ur storage for a file that stores alias, the --alias (or simply -a) got removed from v1.9/8");
    ReadKey(true);
    TypeWrite("- implemented cmd.exe (or bash shell) into the \"launch\" command so that it doesn't just execute any file and that's it");
    TypeWrite("cuz i mean... \"launch /usr/bin/firefox example.com\" barely even do shii besides opening up firefox in new tab and not the example.com");
    TypeWrite("and yes -sudo (or -su) from \"launch\" command is also present");
    ReadKey(true);
    TypeWrite("- the -hex switch from the \"touch\" command is now obselette\ndue to complicated errors that mr.chatgpt couldn't solve");
    ReadKey(true);
    TypeWrite("- highlights filename now");
    ReadKey(true);
    TypeWrite("- i implemented something... u literally can run any file, without using the \"launch\" command");
    TypeWrite("that command is useless now since i gave yall the muscle memory u guys ACTUALLY need,");
    TypeWrite("the command still exists, its usage text roasts you for being a caveman since v1 final update");
    ReadKey(true);
    TypeWrite("- [HUGE] gemini helped tranform this ENTIRE project in C++ (modern C++)");
    TypeWrite("- thats it lmao");
    ReadKey(true);

    SetColor(ConsoleColor::Blue);
    TypeWrite("\nfor more logs, https://discord.gg/C4g2RgYr2g");
    TypeWrite("visit the same website now?? (y/N)");
    if (showDir) {
        SetColor(ConsoleColor::Cyan);
        std::cout << "><[answer default: no]> " << std::flush;
    } else {
        ResForegroundColor();
    }

    std::string confirm;
    std::getline(std::cin, confirm);
    if (ToLower(Trim(confirm)) == "y") {
#ifdef _WIN32
        ShellExecuteA(nullptr, "open", "https://discord.gg/C4g2RgYr2g", nullptr, nullptr, SW_SHOWNORMAL);
#else
        int rc = std::system("xdg-open \"https://discord.gg/C4g2RgYr2g\" >/dev/null 2>&1 &");
        (void)rc;
#endif
        SetColor(ConsoleColor::Cyan);
        TypeWrite("we basically launched the browser for u :D");
        ReadKey(true);
    }

    SetColor(ConsoleColor::Cyan);
    TypeWrite("\ntysm for using my console app :D");
    SetColor(ConsoleColor::DarkCyan);
    TypeWrite("press any key to return");
    ReadKey(true);
    ResForegroundColor();
}

void NetWatch() {
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, 1);

    std::cout << "press any key to stop...\n\n";

    while (!KeyAvailable()) {
        bool success = false;
        long ms = 0;

#ifdef _WIN32
        HANDLE hIcmp = IcmpCreateFile();
        if (hIcmp != INVALID_HANDLE_VALUE) {
            char sendData[32] = "fiscmd_ping";
            DWORD replySize = sizeof(ICMP_ECHO_REPLY) + sizeof(sendData);
            std::vector<char> replyBuf(replySize);
            IPAddr destIp = 0x01010101; // 1.1.1.1
            DWORD dwRet = IcmpSendEcho(hIcmp, destIp, sendData, sizeof(sendData), nullptr, replyBuf.data(), replySize, 1000);
            if (dwRet != 0) {
                auto* reply = reinterpret_cast<ICMP_ECHO_REPLY*>(replyBuf.data());
                if (reply->Status == IP_SUCCESS) {
                    success = true;
                    ms = static_cast<long>(reply->RoundTripTime);
                }
            }
            IcmpCloseHandle(hIcmp);
        }
#else
        auto t0 = std::chrono::steady_clock::now();
        int rc = std::system("ping -c 1 -W 1 1.1.1.1 >/dev/null 2>&1");
        auto t1 = std::chrono::steady_clock::now();
        if (rc == 0) {
            success = true;
            ms = static_cast<long>(std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());
        }
#endif

        ClearScreen();
        if (success) {
            SetColor(ConsoleColor::Cyan);
            std::cout << "PING: " << ms << "ms\n";
            SetColor(ConsoleColor::Green);
            std::cout << "GRAPH: ";
            for (int i = 0; i < 20; i++) {
                std::cout << (dist(rng) == 0 ? "-" : "_");
            }
            std::cout << '\n';
        } else {
            SetColor(ConsoleColor::Red);
            std::cout << "bros internet died mid pinging :sob:\n";
        }
        std::cout << std::flush;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    ReadKey(true);
    ResForegroundColor();
}

void DiskParty() {
#ifdef __linux__
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator("/sys/block", ec)) {
        std::string diskName = entry.path().filename().string();
        if (!IsWholeDisk(diskName))
            continue;

        std::string device = "/dev/" + diskName;
        std::ifstream sizeFile(entry.path() / "size");
        uint64_t sectors = 0;
        if (sizeFile >> sectors) {
            uint64_t totalBytes = sectors * 512ULL;
            SetColor(ConsoleColor::Yellow);
            std::cout << device << " [" << FormatBytes(totalBytes) << "]\n";
        } else {
            SetColor(ConsoleColor::Red);
            std::cout << "failed to read " << device << "\n";
        }
    }
#elif defined(_WIN32)
    DWORD mask = GetLogicalDrives();
    for (char letter = 'A'; letter <= 'Z'; ++letter) {
        if (mask & (1 << (letter - 'A'))) {
            std::string root = std::string(1, letter) + ":\\";
            ULARGE_INTEGER totalBytes{};
            if (GetDiskFreeSpaceExA(root.c_str(), nullptr, &totalBytes, nullptr)) {
                SetColor(ConsoleColor::Yellow);
                std::cout << root << " [" << FormatBytes(totalBytes.QuadPart) << "]\n";
            }
        }
    }
#else
    SetColor(ConsoleColor::Red);
    std::cout << "diskparty is unsupported on this operating system.\n";
#endif
    ResForegroundColor();
}

void Stars() {
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> spawnDist(0, 89);

    int width = GetWindowWidth();
    int height = GetWindowHeight();

    const char fadeChars[] = {'.', '+', '*', '0', '*', '+', '.'};
    constexpr int numFrames = 7;

    SetCursorVisible(false);
    std::vector<std::vector<int>> stars(width, std::vector<int>(height, 0));

    while (!KeyAvailable()) {
        SetCursorPosition(0, 0);

        for (int y = 0; y < height - 1; y++) {
            for (int x = 0; x < width; x++) {
                if (stars[x][y] == 0 && spawnDist(rng) == 0) {
                    stars[x][y] = 1;
                }

                int frame = stars[x][y];
                if (frame > 0) {
                    SetColor(currentFg);
                    SetColor(currentBg, true);
                    std::cout << fadeChars[frame - 1];

                    frame++;
                    if (frame > numFrames)
                        frame = 0;
                    stars[x][y] = frame;
                } else {
                    std::cout << ' ';
                }
            }
            std::cout << '\n';
        }
        std::cout << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(80));
    }

    ReadKey(true);
    SetCursorVisible(true);
    SetColor(currentFg);
    SetColor(currentBg, true);
    ClearScreen();
}

void DownCommand(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SetColor(ConsoleColor::Cyan);
        std::cout << "usage: download/dl <url> [filename] [destination]\n";
        std::cout << "<url> - the download link that u need to fill in\n";
        std::cout << "[filename] - optional flag used to set custom filename instead\n";
        std::cout << "[destination] - optional flag used to download the file to a destinaton instead\n";
        ResForegroundColor();
        return;
    }

    std::string url = args[1];
    std::string fileName;

    if (args.size() >= 3) {
        fileName = args[2];
    } else {
        size_t slash = url.rfind('/');
        std::string tail = (slash == std::string::npos) ? "" : url.substr(slash + 1);
        size_t q = tail.find('?');
        if (q != std::string::npos) tail = tail.substr(0, q);
        fileName = tail;
    }

    if (Trim(fileName).empty())
        fileName = "download";

    std::string destination = currentDir;
    if (args.size() >= 4) {
        destination.clear();
        for (size_t i = 3; i < args.size(); ++i) {
            if (i > 3) destination += ' ';
            destination += args[i];
        }
    }

    fs::path outputPath = fs::path(destination) / fileName;
    DownloadFile(url, outputPath.string());
}

} // namespace fis
