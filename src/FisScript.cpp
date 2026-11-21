#include "FisScript.hpp"
#include "ConsoleUtils.hpp"
#include "Program.hpp"
#include "SetColor.hpp"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <thread>
#include <unordered_map>

namespace fs = std::filesystem;

namespace fis {

namespace {

// helper for FisScript() (for the print command)
std::string ParseVars(std::string text, const std::unordered_map<std::string, std::string>& vars) {
    for (const auto& [k, v] : vars) {
        std::string needle = "{" + k + "}";
        size_t pos = 0;
        while ((pos = text.find(needle, pos)) != std::string::npos) {
            text.replace(pos, needle.size(), v);
            pos += v.size();
        }
    }
    return text;
}

void ShowFisScriptGuide() {
    SetColor(ConsoleColor::Cyan);
    std::cout << "=== FISSCRIPT (.fis) GUIDE ===\n\n";
    SetColor(ConsoleColor::Yellow);
    std::cout << "Supported statements:\n";
    std::cout << "  # comment              - ignored comment line\n";
    std::cout << "  set <var> = <value>    - assign string or number variable\n";
    std::cout << "  print <text with {v}>  - print text with {var} interpolation\n";
    std::cout << "  input <var> [prompt]   - read user input into variable\n";
    std::cout << "  add / sub / mul / div <var> <num> - arithmetic on numeric variable\n";
    std::cout << "  rng <var> <min> <max>  - random integer in [min, max]\n";
    std::cout << "  sleep <ms>             - pause execution in milliseconds\n";
    std::cout << "  cls                    - clear screen\n";
    std::cout << "  cmd <fiscmd line>      - execute any fiscmd command\n";
    std::cout << "  if <v1> == <v2> then <stmt> - conditional single-line statement\n";
    std::cout << "  repeat <count> ... end - repeat block N times\n";
    ResForegroundColor();
}

} // namespace

void FisScript(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SetColor(ConsoleColor::Cyan);
        std::cout << "usage: fisscript <file.fis> OR fisscript -guide\n";
        std::cout << "<file.fis> - script file to run\n";
        std::cout << "-guide - learn how to write fisscript\n";
        ResForegroundColor();
        return;
    }

    if (EqualsIgnoreCase(args[1], "-guide") || EqualsIgnoreCase(args[1], "--guide")) {
        ShowFisScriptGuide();
        return;
    }

    std::string filename;
    for (size_t i = 1; i < args.size(); ++i) {
        if (i > 1) filename += ' ';
        filename += args[i];
    }

    fs::path scriptPath = fs::path(filename).is_absolute()
        ? fs::path(filename)
        : (fs::path(currentDir) / filename);

    std::error_code ec;
    if (!fs::is_regular_file(scriptPath, ec)) {
        SetColor(ConsoleColor::Red);
        std::cout << "fisscript: script file not found :c\n";
        ResForegroundColor();
        return;
    }

    std::ifstream in(scriptPath);
    std::vector<std::string> rawLines;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        rawLines.push_back(line);
    }

    std::unordered_map<std::string, std::string> vars;
    static std::mt19937 rng(std::random_device{}());

    auto execLine = [&](auto& self, const std::string& rawStmt) -> void {
        std::string stmt = Trim(rawStmt);
        if (stmt.empty() || stmt.starts_with("#") || stmt.starts_with("//"))
            return;

        size_t sp = stmt.find(' ');
        std::string op = ToLower(sp == std::string::npos ? stmt : stmt.substr(0, sp));
        std::string rest = (sp == std::string::npos) ? "" : Trim(stmt.substr(sp + 1));

        if (op == "print") {
            SetColor(ConsoleColor::Cyan);
            std::cout << ParseVars(rest, vars) << '\n';
            ResForegroundColor();
        } else if (op == "set") {
            size_t eq = rest.find('=');
            if (eq != std::string::npos) {
                std::string varName = Trim(rest.substr(0, eq));
                std::string val = ParseVars(Trim(rest.substr(eq + 1)), vars);
                if (val.size() >= 2 && val.front() == '"' && val.back() == '"') {
                    val = val.substr(1, val.size() - 2);
                }
                vars[varName] = val;
            }
        } else if (op == "input") {
            size_t sp2 = rest.find(' ');
            std::string varName = (sp2 == std::string::npos) ? rest : Trim(rest.substr(0, sp2));
            std::string prompt = (sp2 == std::string::npos) ? "" : ParseVars(Trim(rest.substr(sp2 + 1)), vars);
            if (!prompt.empty()) std::cout << prompt << std::flush;
            std::string userIn;
            std::getline(std::cin, userIn);
            vars[varName] = userIn;
        } else if (op == "add" || op == "sub" || op == "mul" || op == "div") {
            std::istringstream iss(rest);
            std::string varName, numStr;
            if (iss >> varName >> numStr) {
                try {
                    double cur = vars.count(varName) ? std::stod(vars[varName]) : 0.0;
                    double operand = std::stod(ParseVars(numStr, vars));
                    if (op == "add") cur += operand;
                    else if (op == "sub") cur -= operand;
                    else if (op == "mul") cur *= operand;
                    else if (op == "div" && operand != 0.0) cur /= operand;

                    if (std::floor(cur) == cur)
                        vars[varName] = std::to_string(static_cast<long long>(cur));
                    else
                        vars[varName] = std::to_string(cur);
                } catch (...) {}
            }
        } else if (op == "rng") {
            std::istringstream iss(rest);
            std::string varName;
            int minV = 0, maxV = 100;
            if (iss >> varName >> minV >> maxV) {
                if (minV > maxV) std::swap(minV, maxV);
                std::uniform_int_distribution<int> dist(minV, maxV);
                vars[varName] = std::to_string(dist(rng));
            }
        } else if (op == "sleep") {
            try {
                int ms = std::stoi(ParseVars(rest, vars));
                if (ms > 0) std::this_thread::sleep_for(std::chrono::milliseconds(ms));
            } catch (...) {}
        } else if (op == "cls") {
            ClearScreen();
        } else if (op == "cmd") {
            std::string expanded = ParseVars(rest, vars);
            auto parsed = ParseQuotedArgs(expanded);
            execcmd(parsed, expanded);
        } else if (op == "if") {
            size_t thenPos = ToLower(rest).find(" then ");
            if (thenPos != std::string::npos) {
                std::string cond = ParseVars(Trim(rest.substr(0, thenPos)), vars);
                std::string action = Trim(rest.substr(thenPos + 6));
                size_t eqPos = cond.find("==");
                size_t neqPos = cond.find("!=");
                if (eqPos != std::string::npos) {
                    std::string lhs = Trim(cond.substr(0, eqPos));
                    std::string rhs = Trim(cond.substr(eqPos + 2));
                    if (lhs == rhs) self(self, action);
                } else if (neqPos != std::string::npos) {
                    std::string lhs = Trim(cond.substr(0, neqPos));
                    std::string rhs = Trim(cond.substr(neqPos + 2));
                    if (lhs != rhs) self(self, action);
                }
            }
        }
    };

    for (size_t i = 0; i < rawLines.size(); ++i) {
        std::string trimmed = Trim(rawLines[i]);
        if (StartsWithIgnoreCase(trimmed, "repeat ")) {
            int count = 1;
            try {
                count = std::stoi(Trim(trimmed.substr(7)));
            } catch (...) {}

            std::vector<std::string> block;
            size_t j = i + 1;
            while (j < rawLines.size() && !EqualsIgnoreCase(Trim(rawLines[j]), "end")) {
                block.push_back(rawLines[j]);
                j++;
            }
            for (int rep = 0; rep < count; ++rep) {
                for (const auto& bLine : block) {
                    execLine(execLine, bLine);
                }
            }
            i = j;
        } else {
            execLine(execLine, trimmed);
        }
    }

    ResForegroundColor();
}

} // namespace fis
