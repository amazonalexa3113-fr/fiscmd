#include "FisScript.hpp"
#include "ConsoleUtils.hpp"
#include "Program.hpp"
#include "SetColor.hpp"

#include <chrono>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <utility>

namespace fs = std::filesystem;

namespace fis {
namespace {

using Variables = std::unordered_map<std::string, std::string>;

std::string ParseVars(const std::string& text, const Variables& vars) {
    std::string result;
    for (size_t i = 0; i < text.size();) {
        if (text[i] != '{') {
            result += text[i++];
            continue;
        }
        const size_t close = text.find('}', i + 1);
        if (close == std::string::npos) {
            result.append(text, i, std::string::npos);
            break;
        }
        const std::string name = text.substr(i + 1, close - i - 1);
        const auto found = vars.find(name);
        if (found == vars.end()) result.append(text, i, close - i + 1);
        else result += found->second;
        i = close + 1;
    }
    return result;
}

void ShowFisScriptGuide() {
    SetColor(ConsoleColor::Cyan);
    std::cout << "=== FISSCRIPT (.fis) GUIDE ===\n\n";
    SetColor(ConsoleColor::Yellow);
    std::cout << "Supported statements:\n";
    std::cout << "  # comment              - ignored comment line\n";
    std::cout << "  var/set <var> = <value> - declare or assign a variable\n";
    std::cout << "  echo <text>            - print literal text\n";
    std::cout << "  print <text with {v}>  - print text with {var} interpolation\n";
    std::cout << "  input <var> [prompt]   - read user input into variable\n";
    std::cout << "  add / sub / mul / div <var> <num> - arithmetic on numeric variable\n";
    std::cout << "  rng <var> <min> <max>  - random integer in [min, max]\n";
    std::cout << "  sleep <ms>             - pause execution in milliseconds\n";
    std::cout << "  cls                    - clear screen\n";
    std::cout << "  cmd <fiscmd line>      - execute any fiscmd command\n";
    std::cout << "  if (a == b) { ... } else { ... } - conditional block\n";
    std::cout << "  if a == b then <stmt>  - single-line conditional\n";
    std::cout << "  repeat <count> ... end - repeat block N times\n";
    ResForegroundColor();
}

struct Node {
    enum class Kind { Command, If, Repeat } kind = Kind::Command;
    size_t line = 0;
    std::string text;
    std::vector<Node> body;
    std::vector<Node> alternative;
};

class ParseError : public std::runtime_error {
public:
    ParseError(size_t line, const std::string& message)
        : std::runtime_error("line " + std::to_string(line) + ": " + message) {}
};

class Parser {
public:
    explicit Parser(const std::vector<std::string>& source) : lines(source) {}

    std::vector<Node> Parse() {
        auto result = ParseSequence(false, false);
        if (index < lines.size()) throw ParseError(index + 1, "unexpected block terminator");
        return result;
    }

private:
    const std::vector<std::string>& lines;
    size_t index = 0;

    static bool IsCommentOrEmpty(const std::string& line) {
        return line.empty() || line.starts_with("#") || line.starts_with("//");
    }

    static bool StartsWord(const std::string& text, const std::string& word) {
        const std::string lower = ToLower(text);
        return lower == word || (lower.size() > word.size() && lower.compare(0, word.size(), word) == 0 &&
            std::isspace(static_cast<unsigned char>(lower[word.size()])));
    }

    std::string NextContent() {
        while (index < lines.size() && IsCommentOrEmpty(Trim(lines[index]))) ++index;
        return index < lines.size() ? Trim(lines[index]) : std::string();
    }

    std::vector<Node> ParseSequence(bool inBrace, bool inRepeat, std::string* closeSuffix = nullptr) {
        std::vector<Node> nodes;
        while (index < lines.size()) {
            std::string statement = Trim(lines[index]);
            const size_t lineNumber = index + 1;
            if (IsCommentOrEmpty(statement)) { ++index; continue; }

            if (statement[0] == '}') {
                if (!inBrace) throw ParseError(lineNumber, "unmatched closing brace");
                ++index;
                if (closeSuffix) *closeSuffix = Trim(statement.substr(1));
                return nodes;
            }
            if (ToLower(statement) == "end") {
                if (!inRepeat) throw ParseError(lineNumber, "unmatched 'end'");
                ++index;
                return nodes;
            }
            if (StartsWord(statement, "else"))
                throw ParseError(lineNumber, "'else' without a matching 'if'");

            if (StartsWord(statement, "if") && ToLower(statement).find(" then ") == std::string::npos) {
                nodes.push_back(ParseIf(statement, lineNumber));
                continue;
            }
            if (StartsWord(statement, "repeat")) {
                Node repeat;
                repeat.kind = Node::Kind::Repeat;
                repeat.line = lineNumber;
                repeat.text = Trim(statement.substr(6));
                ++index;
                repeat.body = ParseSequence(false, true);
                nodes.push_back(std::move(repeat));
                continue;
            }
            if (statement == "{")
                throw ParseError(lineNumber, "opening brace without a conditional");
            Node command;
            command.line = lineNumber;
            command.text = std::move(statement);
            nodes.push_back(std::move(command));
            ++index;
        }
        if (inBrace) throw ParseError(lines.size() ? lines.size() : 1, "missing closing brace");
        if (inRepeat) throw ParseError(lines.size() ? lines.size() : 1, "missing 'end' for repeat block");
        return nodes;
    }

    Node ParseIf(const std::string& header, size_t lineNumber) {
        Node node;
        node.kind = Node::Kind::If;
        node.line = lineNumber;

        std::string tail = Trim(header.substr(2));
        if (!tail.empty() && tail.back() == '{') {
            tail = Trim(tail.substr(0, tail.size() - 1));
            ++index;
        }
        else {
            ++index;
            const std::string open = NextContent();
            if (open != "{") throw ParseError(index < lines.size() ? index + 1 : lineNumber, "expected '{' after conditional");
            ++index;
        }
        node.text = Trim(tail);
        if (node.text.size() >= 2 && node.text.front() == '(' && node.text.back() == ')')
            node.text = Trim(node.text.substr(1, node.text.size() - 2));
        if (node.text.empty()) throw ParseError(lineNumber, "empty condition");

        std::string closeSuffix;
        node.body = ParseSequence(true, false, &closeSuffix);
        std::string possibleElse = closeSuffix;
        if (possibleElse.empty()) {
            possibleElse = NextContent();
            if (!possibleElse.empty() && index < lines.size() && StartsWord(possibleElse, "else")) ++index;
            else possibleElse.clear();
        }
        if (!possibleElse.empty()) {
            if (!StartsWord(possibleElse, "else"))
                throw ParseError(index ? index : lineNumber, "unexpected text after closing brace");
            std::string afterElse = Trim(possibleElse.substr(4));
            if (afterElse.empty()) {
                const std::string open = NextContent();
                if (open != "{") throw ParseError(index < lines.size() ? index + 1 : lineNumber, "expected '{' after else");
                ++index;
            } else if (afterElse != "{") {
                throw ParseError(index ? index : lineNumber, "expected '{' after else");
            }
            node.alternative = ParseSequence(true, false);
        }
        return node;
    }
};

bool EvaluateCondition(std::string condition, const Variables& vars, size_t line) {
    condition = ParseVars(Trim(condition), vars);
    const size_t eq = condition.find("==");
    const size_t neq = condition.find("!=");
    const size_t op = eq != std::string::npos ? eq : neq;
    if (op == std::string::npos || (eq != std::string::npos && neq != std::string::npos))
        throw ParseError(line, "condition must contain exactly one '==' or '!=' operator");
    std::string lhs = Trim(condition.substr(0, op));
    std::string rhs = Trim(condition.substr(op + 2));
    if (lhs.empty() || rhs.empty()) throw ParseError(line, "condition has a missing value");
    if (const auto found = vars.find(lhs); found != vars.end()) lhs = found->second;
    if (const auto found = vars.find(rhs); found != vars.end()) rhs = found->second;
    return eq != std::string::npos ? lhs == rhs : lhs != rhs;
}

void ReportError(const std::string& message) {
    SetColor(ConsoleColor::Red);
    std::cout << "fisscript: " << message << '\n';
    ResForegroundColor();
}

void ExecuteCommand(const Node& node, Variables& vars, std::mt19937& rng) {
    const std::string& stmt = node.text;
    const size_t sp = stmt.find_first_of(" \t");
    const std::string op = ToLower(sp == std::string::npos ? stmt : stmt.substr(0, sp));
    const std::string rest = sp == std::string::npos ? "" : Trim(stmt.substr(sp + 1));

    if (op == "echo" || op == "print") {
        SetColor(ConsoleColor::Cyan);
        std::cout << (op == "echo" ? rest : ParseVars(rest, vars)) << '\n';
        ResForegroundColor();
    } else if (op == "var" || op == "set") {
        const size_t eq = rest.find('=');
        if (eq == std::string::npos) throw ParseError(node.line, "expected '<name> = <value>'");
        const std::string name = Trim(rest.substr(0, eq));
        if (name.empty() || name.find_first_of(" \t{}") != std::string::npos)
            throw ParseError(node.line, "invalid variable name");
        std::string value = ParseVars(Trim(rest.substr(eq + 1)), vars);
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
            value = value.substr(1, value.size() - 2);
        vars[name] = std::move(value);
    } else if (op == "input") {
        const size_t split = rest.find_first_of(" \t");
        const std::string name = split == std::string::npos ? rest : Trim(rest.substr(0, split));
        if (name.empty()) throw ParseError(node.line, "input requires a variable name");
        const std::string prompt = split == std::string::npos ? "" : ParseVars(Trim(rest.substr(split + 1)), vars);
        if (!prompt.empty()) std::cout << prompt << std::flush;
        std::string value;
        std::getline(std::cin, value);
        vars[name] = std::move(value);
    } else if (op == "add" || op == "sub" || op == "mul" || op == "div") {
        std::istringstream input(rest);
        std::string name, number;
        if (!(input >> name >> number)) throw ParseError(node.line, "arithmetic command requires a variable and number");
        try {
            double current = vars.count(name) ? std::stod(vars.at(name)) : 0.0;
            const double operand = std::stod(ParseVars(number, vars));
            if (op == "add") current += operand;
            else if (op == "sub") current -= operand;
            else if (op == "mul") current *= operand;
            else if (operand != 0.0) current /= operand;
            else throw ParseError(node.line, "division by zero");
            vars[name] = std::floor(current) == current
                ? std::to_string(static_cast<long long>(current)) : std::to_string(current);
        } catch (const ParseError&) { throw; }
        catch (...) { throw ParseError(node.line, "invalid numeric value in arithmetic command"); }
    } else if (op == "rng") {
        std::istringstream input(rest);
        std::string name;
        int minValue = 0, maxValue = 100;
        if (!(input >> name >> minValue >> maxValue)) throw ParseError(node.line, "rng requires a variable and two bounds");
        if (minValue > maxValue) std::swap(minValue, maxValue);
        vars[name] = std::to_string(std::uniform_int_distribution<int>(minValue, maxValue)(rng));
    } else if (op == "sleep") {
        try {
            const int ms = std::stoi(ParseVars(rest, vars));
            if (ms > 0) std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        } catch (...) { throw ParseError(node.line, "sleep requires a valid millisecond value"); }
    } else if (op == "cls") {
        ClearScreen();
    } else if (op == "cmd") {
        const std::string expanded = ParseVars(rest, vars);
        const auto parsed = ParseQuotedArgs(expanded);
        execcmd(parsed, expanded);
    } else if (stmt.ends_with("++") || stmt.ends_with("--")) {
        const std::string name = Trim(stmt.substr(0, stmt.size() - 2));
        if (name.empty()) throw ParseError(node.line, "increment/decrement requires a variable name");
        try {
            const double current = vars.count(name) ? std::stod(vars.at(name)) : 0.0;
            const double updated = current + (stmt.ends_with("++") ? 1.0 : -1.0);
            vars[name] = std::floor(updated) == updated
                ? std::to_string(static_cast<long long>(updated)) : std::to_string(updated);
        } catch (...) { throw ParseError(node.line, "increment/decrement requires a numeric variable"); }
    } else if (op == "if") {
        const std::string lower = ToLower(rest);
        const size_t thenPos = lower.find(" then ");
        if (thenPos == std::string::npos) throw ParseError(node.line, "malformed single-line if; expected 'then'");
        if (EvaluateCondition(rest.substr(0, thenPos), vars, node.line)) {
            Node action;
            action.line = node.line;
            action.text = Trim(rest.substr(thenPos + 6));
            if (action.text.empty()) throw ParseError(node.line, "single-line if requires a statement");
            ExecuteCommand(action, vars, rng);
        }
    } else {
        throw ParseError(node.line, "unsupported command '" + op + "'");
    }
}

void Execute(const std::vector<Node>& nodes, Variables& vars, std::mt19937& rng) {
    for (const Node& node : nodes) {
        if (node.kind == Node::Kind::Command) ExecuteCommand(node, vars, rng);
        else if (node.kind == Node::Kind::If) {
            if (!node.alternative.empty() && node.alternative.front().kind == Node::Kind::Command &&
                node.alternative.front().text.starts_with("if ") && node.body.empty()) {
                ExecuteCommand(node.alternative.front(), vars, rng);
                continue;
            }
            Execute(EvaluateCondition(node.text, vars, node.line) ? node.body : node.alternative, vars, rng);
        } else {
            int count = 0;
            try { count = std::stoi(ParseVars(node.text, vars)); }
            catch (...) { throw ParseError(node.line, "repeat requires a valid count"); }
            for (int i = 0; i < count; ++i) Execute(node.body, vars, rng);
        }
    }
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
    const fs::path scriptPath = fs::path(filename).is_absolute()
        ? fs::path(filename) : (fs::path(currentDir) / filename);
    std::error_code ec;
    if (!fs::is_regular_file(scriptPath, ec)) {
        SetColor(ConsoleColor::Red);
        std::cout << "fisscript: script file not found :c\n";
        ResForegroundColor();
        return;
    }
    std::ifstream input(scriptPath);
    if (!input) { ReportError("could not open script file"); return; }
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(std::move(line));
    }

    try {
        const auto program = Parser(lines).Parse();
        Variables vars;
        static std::mt19937 rng(std::random_device{}());
        Execute(program, vars, rng);
    } catch (const ParseError& error) {
        ReportError(error.what());
    } catch (const std::exception& error) {
        ReportError(error.what());
    }
    ResForegroundColor();
}

} // namespace fis
