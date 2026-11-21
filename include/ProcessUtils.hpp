#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace fis {

// run foreground process helper for RunCommand()
int RunShellCommand(std::string_view command);

// even more helper functions for running command yes winos + linux cross-platform
std::optional<std::string> FindExecutable(std::string_view command);
std::optional<std::string> GetExecutableGhost(std::string_view prefix);
bool IsLaunchableProgram(std::string_view command);
bool LaunchProgram(const std::vector<std::string>& args);

// totally administrator/root detector
bool DetectRoot();

// run/open/launch command
void RunCommand(const std::vector<std::string>& args, std::string_view rawInput);

// fistop & fiskill
void Taskmgr(const std::vector<std::string>& args);
void KillProc(const std::vector<std::string>& args);

} // namespace fis
