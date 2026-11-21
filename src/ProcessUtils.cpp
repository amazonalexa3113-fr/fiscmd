#include "ProcessUtils.hpp"
#include "ConsoleUtils.hpp"
#include "Program.hpp"
#include "SetColor.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <tlhelp32.h>
#include <shellapi.h>
#else
#include <csignal>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace fis {

namespace {

struct ProcEntry {
    int pid = 0;
    std::string name;
};

std::vector<ProcEntry> GetRunningProcesses() {
    std::vector<ProcEntry> list;
#ifdef _WIN32
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 pe{};
        pe.dwSize = sizeof(pe);
        if (Process32First(hSnap, &pe)) {
            do {
                std::string exe = pe.szExeFile;
                if (EndsWithIgnoreCase(exe, ".exe")) {
                    exe = exe.substr(0, exe.size() - 4);
                }
                list.push_back({static_cast<int>(pe.th32ProcessID), exe});
            } while (Process32Next(hSnap, &pe));
        }
        CloseHandle(hSnap);
    }
#else
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator("/proc", ec)) {
        if (!entry.is_directory(ec)) continue;
        std::string fname = entry.path().filename().string();
        if (fname.empty() || !std::all_of(fname.begin(), fname.end(), ::isdigit)) continue;
        int pid = std::stoi(fname);
        std::ifstream commFile(entry.path() / "comm");
        std::string comm;
        if (std::getline(commFile, comm)) {
            list.push_back({pid, Trim(comm)});
        }
    }
#endif
    return list;
}

std::vector<std::string> SplitEnv(const char* val, char delim) {
    std::vector<std::string> out;
    if (!val) return out;
    std::stringstream ss(val);
    std::string item;
    while (std::getline(ss, item, delim)) {
        if (!item.empty()) out.push_back(item);
    }
    return out;
}

} // namespace

int RunShellCommand(std::string_view command) {
    if (Trim(command).empty()) {
        SetColor(ConsoleColor::Red);
        std::cout << "where command bro\n";
        ResForegroundColor();
        return 1;
    }

    SetColor(ConsoleColor::Cyan);
    std::cout << "press ctrl+c to fuck the program\n";
    ResetColor();

    std::string cmdStr(command);

#ifdef _WIN32
    std::string comspec = "cmd.exe";
    if (const char* envCom = std::getenv("COMSPEC")) {
        comspec = envCom;
    }
    std::string fullCmd = "\"" + comspec + "\" /d /s /c \"" + cmdStr + "\"";

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    std::vector<char> mutableCmd(fullCmd.begin(), fullCmd.end());
    mutableCmd.push_back('\0');

    if (!CreateProcessA(
            nullptr,
            mutableCmd.data(),
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            currentDir.c_str(),
            &si,
            &pi)) {
        SetColor(ConsoleColor::Red);
        std::cout << "failed to start the shell :c\n";
        ResForegroundColor();
        return 1;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return static_cast<int>(exitCode);
#else
    pid_t pid = fork();
    if (pid < 0) {
        SetColor(ConsoleColor::Red);
        std::cout << "failed to start the shell :c\n";
        ResForegroundColor();
        return 1;
    }
    if (pid == 0) {
        std::error_code ec;
        fs::current_path(currentDir, ec);
        signal(SIGINT, SIG_DFL);
        execl("/bin/bash", "bash", "-c", cmdStr.c_str(), static_cast<char*>(nullptr));
        _exit(127);
    }

    int status = 0;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    return 1;
#endif
}

std::optional<std::string> FindExecutable(std::string_view command) {
    if (Trim(command).empty())
        return std::nullopt;

    std::string cmd(command);
    bool hasPath = (cmd.find('/') != std::string::npos) || (cmd.find('\\') != std::string::npos);
    fs::path cmdPath(cmd);
    std::error_code ec;

    if (hasPath || cmdPath.is_absolute()) {
        fs::path path = cmdPath.is_absolute() ? cmdPath : (fs::path(currentDir) / cmdPath);
        if (fs::is_regular_file(path, ec)) {
            return fs::weakly_canonical(path, ec).string();
        }
        return std::nullopt;
    }

    fs::path local = fs::path(currentDir) / cmd;
    if (fs::is_regular_file(local, ec)) {
        return fs::weakly_canonical(local, ec).string();
    }

#ifdef _WIN32
    constexpr char pathSep = ';';
    auto exts = SplitEnv(std::getenv("PATHEXT"), ';');
    if (exts.empty()) exts = {".EXE", ".CMD", ".BAT", ".COM"};
#else
    constexpr char pathSep = ':';
#endif

    auto dirs = SplitEnv(std::getenv("PATH"), pathSep);
    for (const auto& dir : dirs) {
        fs::path candidate = fs::path(dir) / cmd;
        if (fs::is_regular_file(candidate, ec)) {
#ifndef _WIN32
            if (access(candidate.c_str(), X_OK) == 0)
#endif
                return candidate.string();
        }

#ifdef _WIN32
        for (const auto& ext : exts) {
            fs::path withExt = candidate.string() + ext;
            if (fs::is_regular_file(withExt, ec)) {
                return withExt.string();
            }
        }
#endif
    }

    return std::nullopt;
}

std::optional<std::string> GetExecutableGhost(std::string_view prefix) {
    if (Trim(prefix).empty())
        return std::nullopt;

#ifdef _WIN32
    constexpr char pathSep = ';';
    auto exts = SplitEnv(std::getenv("PATHEXT"), ';');
    if (exts.empty()) exts = {".EXE", ".CMD", ".BAT", ".COM"};
#else
    constexpr char pathSep = ':';
#endif

    auto dirs = SplitEnv(std::getenv("PATH"), pathSep);
    std::error_code ec;

    for (const auto& dir : dirs) {
        if (!fs::is_directory(dir, ec)) continue;

        std::vector<std::string> matches;
        for (const auto& entry : fs::directory_iterator(dir, ec)) {
            if (!entry.is_regular_file(ec)) continue;
            std::string name = entry.path().filename().string();

#ifdef _WIN32
            for (const auto& ext : exts) {
                if (EndsWithIgnoreCase(name, ext)) {
                    name = name.substr(0, name.size() - ext.size());
                    break;
                }
            }
#endif
            if (StartsWithIgnoreCase(name, prefix) && name.size() > prefix.size()) {
                matches.push_back(name);
            }
        }

        if (!matches.empty()) {
            std::sort(matches.begin(), matches.end(), [](const std::string& a, const std::string& b) {
                return ToLower(a) < ToLower(b);
            });
            return matches.front().substr(prefix.size());
        }
    }

    return std::nullopt;
}

bool IsLaunchableProgram(std::string_view command) {
    return FindExecutable(command).has_value();
}

bool LaunchProgram(const std::vector<std::string>& args) {
    if (args.empty()) return false;
    auto executable = FindExecutable(args[0]);
    if (!executable.has_value())
        return false;

    ResetColor();
#ifdef _WIN32
    std::ostringstream cmdLine;
    cmdLine << "\"" << *executable << "\"";
    for (size_t i = 1; i < args.size(); ++i) {
        cmdLine << " \"" << args[i] << "\"";
    }
    std::string cmdStr = cmdLine.str();
    std::vector<char> mutableCmd(cmdStr.begin(), cmdStr.end());
    mutableCmd.push_back('\0');

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    if (!CreateProcessA(
            executable->c_str(),
            mutableCmd.data(),
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            currentDir.c_str(),
            &si,
            &pi)) {
        SetColor(ConsoleColor::Red);
        std::cout << "failed to launch executable\n";
        ResForegroundColor();
        return true;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    ResForegroundColor();
    return true;
#else
    pid_t pid = fork();
    if (pid < 0) {
        SetColor(ConsoleColor::Red);
        std::cout << "failed to launch executable\n";
        ResForegroundColor();
        return true;
    }
    if (pid == 0) {
        std::error_code ec;
        fs::current_path(currentDir, ec);
        signal(SIGINT, SIG_DFL);
        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(executable->c_str()));
        for (size_t i = 1; i < args.size(); ++i) {
            argv.push_back(const_cast<char*>(args[i].c_str()));
        }
        argv.push_back(nullptr);
        execv(executable->c_str(), argv.data());
        _exit(127);
    }
    int status = 0;
    waitpid(pid, &status, 0);
    ResForegroundColor();
    return true;
#endif
}

bool DetectRoot() {
#ifdef _WIN32
    BOOL isAdmin = FALSE;
    PSID adminGroup = nullptr;
    SID_IDENTIFIER_AUTHORITY ntAuth = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuth, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(nullptr, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin == TRUE;
#else
    return geteuid() == 0;
#endif
}

void RunCommand(const std::vector<std::string>& args, std::string_view rawInput) {
    if (args.size() < 2) {
        SetColor(ConsoleColor::DarkRed);
        std::cout << "it can't be...\n";
        SetColor(ConsoleColor::Red);
        std::cout << "YOU STILL USE ME?????\n\n";
        SetColor(ConsoleColor::Cyan);
        std::cout << "usage: run/open/launch [args] <command>\n";
        std::cout << "u can replace [args] with:\n";
        std::cout << "-website or -web - launches webpage/website instead of files\n";
        std::cout << "-sudo or -su - launches as ";
#ifdef _WIN32
        std::cout << "an administrator\n";
#else
        std::cout << "root\n";
#endif
        std::cout << "or leave empty to run a shell command normally\n";
        ResForegroundColor();
        return;
    }

    bool useSudo = EqualsIgnoreCase(args[1], "-sudo") || EqualsIgnoreCase(args[1], "-su");
    size_t targetIndex = useSudo ? 2 : 1;

    if (args.size() <= targetIndex) {
        SetColor(ConsoleColor::Red);
        std::cout << "u forgot the command bro\n";
        ResForegroundColor();
        return;
    }

    // WEBSITE MODE
    if (EqualsIgnoreCase(args[targetIndex], "-website") || EqualsIgnoreCase(args[targetIndex], "-web")) {
        if (args.size() <= targetIndex + 1) {
            SetColor(ConsoleColor::Red);
            std::cout << "u forgot the website link bro\n";
            ResForegroundColor();
            return;
        }

        std::string website = args[targetIndex + 1];
        if (!StartsWithIgnoreCase(website, "http://") && !StartsWithIgnoreCase(website, "https://")) {
            SetColor(ConsoleColor::Cyan);
            std::cout << "u forgot the https:// thing brah\n";
            std::cout << "press enter or y to auto add\n";

            std::string confirm;
            std::getline(std::cin, confirm);
            confirm = ToLower(Trim(confirm));
            if (confirm == "y" || confirm.empty()) {
                website = "https://" + website;
            }
        }

#ifdef _WIN32
        ShellExecuteA(nullptr, "open", website.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
        std::string openCmd = "xdg-open \"" + website + "\" >/dev/null 2>&1 &";
        int rc = std::system(openCmd.c_str());
        (void)rc;
#endif
        SetColor(ConsoleColor::Cyan);
        std::cout << "website opened successfully :D\n";
        ResForegroundColor();
        return;
    }

    size_t commandStart = 0;
    while (commandStart < rawInput.size() && !std::isspace(static_cast<unsigned char>(rawInput[commandStart])))
        commandStart++;
    while (commandStart < rawInput.size() && std::isspace(static_cast<unsigned char>(rawInput[commandStart])))
        commandStart++;

    if (useSudo) {
        while (commandStart < rawInput.size() && !std::isspace(static_cast<unsigned char>(rawInput[commandStart])))
            commandStart++;
        while (commandStart < rawInput.size() && std::isspace(static_cast<unsigned char>(rawInput[commandStart])))
            commandStart++;
    }

    if (commandStart >= rawInput.size()) {
        SetColor(ConsoleColor::Red);
        std::cout << "where's the command bro\n";
        ResForegroundColor();
        return;
    }

    std::string shellCommand = Trim(rawInput.substr(commandStart));
    if (shellCommand.empty()) {
        SetColor(ConsoleColor::Red);
        std::cout << "where's the command bro\n";
        ResForegroundColor();
        return;
    }

    if (useSudo) {
        SetColor(ConsoleColor::Cyan);
        std::cout << "sudo/administrator mode has been used, press ctrl+c to fuck the program\n";
        ResetColor();
#ifdef _WIN32
        const char* comspec = std::getenv("COMSPEC");
        std::string cmdExe = comspec ? comspec : "cmd.exe";
        std::string param = "/c " + shellCommand;
        SHELLEXECUTEINFOA sei{};
        sei.cbSize = sizeof(sei);
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;
        sei.lpVerb = "runas";
        sei.lpFile = cmdExe.c_str();
        sei.lpParameters = param.c_str();
        sei.nShow = SW_SHOWNORMAL;
        if (ShellExecuteExA(&sei) && sei.hProcess) {
            WaitForSingleObject(sei.hProcess, INFINITE);
            CloseHandle(sei.hProcess);
        }
#else
        pid_t pid = fork();
        if (pid == 0) {
            execlp("sudo", "sudo", "sh", "-c", shellCommand.c_str(), static_cast<char*>(nullptr));
            _exit(127);
        } else if (pid > 0) {
            int st = 0;
            waitpid(pid, &st, 0);
        }
#endif
    } else {
        RunShellCommand(shellCommand);
    }

    ResForegroundColor();
}

void Taskmgr(const std::vector<std::string>& args) {
    if (args.size() > 1 && (ToLower(args[1]) == "-watch" || ToLower(args[1]) == "-live")) {
        std::string keyword = (args.size() > 2) ? ToLower(args[2]) : "";

        SetColor(ConsoleColor::Cyan);
        if (!keyword.empty()) {
            std::cout << "watching for '" << keyword << "' processes...\n";
        } else {
            std::cout << "watching ALL processes...\n";
        }
        std::cout << "press any key to stop watching\n\n";
        ResForegroundColor();

        while (!KeyAvailable()) {
            ClearScreen();
            auto procs = GetRunningProcesses();
            std::vector<ProcEntry> filtered;
            for (const auto& p : procs) {
                if (keyword.empty() || ToLower(p.name).find(keyword) != std::string::npos) {
                    filtered.push_back(p);
                }
            }
            std::sort(filtered.begin(), filtered.end(), [](const ProcEntry& a, const ProcEntry& b) {
                return ToLower(a.name) < ToLower(b.name);
            });

            SetColor(ConsoleColor::Cyan);
            if (!keyword.empty()) {
                std::cout << "live process watcher for '" << keyword << "'\n";
            } else {
                std::cout << "live process watcher\n";
            }
            std::cout << "press any key to exit\n";
            std::cout << "\nname                 pid\n";
            std::cout << "------------------------\n";

            SetColor(ConsoleColor::Yellow);
            if (filtered.empty()) {
                SetColor(ConsoleColor::Red);
                std::cout << "no matching processes found :sob::pray:\n";
            } else {
                for (const auto& proc : filtered) {
                    std::cout << std::left << std::setw(20) << proc.name << " " << proc.pid << "\n";
                }
            }
            std::cout << std::flush;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        ReadKey(true);
        SetColor(ConsoleColor::Cyan);
        std::cout << "\nstopped watching process :D\n";
        ResForegroundColor();
        return;
    }

    auto processes = GetRunningProcesses();

    if (args.size() > 1 && ToLower(args[1]) == "-search") {
        if (args.size() < 3) {
            SetColor(ConsoleColor::Red);
            std::cout << "u forgr the keyword bro\n";
            ResForegroundColor();
            return;
        }
        std::string keyword = ToLower(args[2]);
        std::vector<ProcEntry> filtered;
        for (const auto& p : processes) {
            if (ToLower(p.name).find(keyword) != std::string::npos) {
                filtered.push_back(p);
            }
        }
        processes = std::move(filtered);
    } else if (args.size() > 1 && ToLower(args[1]) == "-searchpid") {
        if (args.size() < 3) {
            SetColor(ConsoleColor::Red);
            std::cout << "u forgr the pid bro\n";
            ResForegroundColor();
            return;
        }
        int targetPid = -1;
        try {
            targetPid = std::stoi(args[2]);
        } catch (...) {
            SetColor(ConsoleColor::Red);
            std::cout << "ts was barely even a pid\n";
            ResForegroundColor();
            return;
        }
        std::vector<ProcEntry> filtered;
        for (const auto& p : processes) {
            if (p.pid == targetPid) filtered.push_back(p);
        }
        processes = std::move(filtered);
    }

    std::sort(processes.begin(), processes.end(), [](const ProcEntry& a, const ProcEntry& b) {
        return ToLower(a.name) < ToLower(b.name);
    });

    SetColor(ConsoleColor::Cyan);
    std::cout << "name                 pid\n";
    std::cout << "------------------------\n";

    SetColor(ConsoleColor::Yellow);
    for (const auto& proc : processes) {
        std::cout << std::left << std::setw(20) << proc.name << " " << proc.pid << "\n";
    }
    ResForegroundColor();
}

void KillProc(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SetColor(ConsoleColor::Cyan);
        std::cout << "usage: fiskill <pid> (-search [keyword])\n";
        std::cout << "<pid> - the exact process' pid to terminate\n";
        std::cout << "(-search [keyword]) - optional switch to search the process' keyword before terminating it\n";
        SetColor(ConsoleColor::Red);
        std::cout << "\nbe careful when using this...\n";
        ResForegroundColor();
        return;
    }

    auto terminateByPid = [](int pid) {
        auto procs = GetRunningProcesses();
        std::string pname = "unknown";
        for (const auto& p : procs) {
            if (p.pid == pid) {
                pname = p.name;
                break;
            }
        }

        SetColor(ConsoleColor::Red);
        std::cout << "TERMINATE '" << pname << "' FR????? (y/n): " << std::flush;

        std::string confirm;
        std::getline(std::cin, confirm);
        if (ToLower(Trim(confirm)) != "y") {
            SetColor(ConsoleColor::Cyan);
            std::cout << "oh cool u cancelled it :skull:\n";
            ResForegroundColor();
            return;
        }

#ifdef _WIN32
        HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, static_cast<DWORD>(pid));
        if (!hProc || !TerminateProcess(hProc, 1)) {
            if (hProc) CloseHandle(hProc);
            SetColor(ConsoleColor::Red);
            std::cout << "failed to terminate the process\n";
            ResForegroundColor();
            return;
        }
        CloseHandle(hProc);
#else
        if (kill(pid, SIGKILL) != 0) {
            SetColor(ConsoleColor::Red);
            std::cout << "failed to terminate the process\n";
            ResForegroundColor();
            return;
        }
#endif
        SetColor(ConsoleColor::DarkRed);
        std::cout << "process sent to the shadow realm\n";
        ResForegroundColor();
    };

    if (ToLower(args[1]) == "-search") {
        if (args.size() < 3) {
            SetColor(ConsoleColor::Red);
            std::cout << "u forgr the keyword bro\n";
            ResForegroundColor();
            return;
        }
        std::string keyword = ToLower(args[2]);
        SetColor(ConsoleColor::Yellow);
        for (const auto& p : GetRunningProcesses()) {
            if (ToLower(p.name).find(keyword) != std::string::npos) {
                std::cout << p.name << " (PID: " << p.pid << ")\n";
            }
        }

        SetColor(ConsoleColor::Cyan);
        std::cout << "\nenter pid to terminate: " << std::flush;
        std::string pidInput;
        std::getline(std::cin, pidInput);
        try {
            int pid = std::stoi(Trim(pidInput));
            terminateByPid(pid);
        } catch (...) {
            SetColor(ConsoleColor::Red);
            std::cout << "ts was barely even a pid\n";
            ResForegroundColor();
        }
        return;
    }

    try {
        int pid = std::stoi(args[1]);
        terminateByPid(pid);
    } catch (...) {
        SetColor(ConsoleColor::Red);
        std::cout << "ts was barely even a pid\n";
        ResForegroundColor();
    }
}

} // namespace fis
