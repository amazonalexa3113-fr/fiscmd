#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace fis {

// static state
inline bool showDir = true;
inline std::string currentDir;

// history (for autocomplete)
inline std::vector<std::string> history;
inline int historyIndex = -1;

// importable commands
inline bool importedFissnake = false;
inline bool importedFisscript = false;
inline bool importedFisdraw = false;
inline bool importedStars = false;
inline bool importedDown = false;

// for storing alias history purposes
inline std::unordered_map<std::string, std::string> aliases;

// commands for tab autocomplete
extern const std::vector<std::string> commands;

void InitProgramState();
std::string GetCurrentUserName();
std::string GetMachineName();

void toggleShowDir();
bool WarnNotImported(bool isImported);
void ImportCmd(const std::vector<std::string>& args);

// slop everything any switch() has into here
bool execcmd(const std::vector<std::string>& commandArgs, std::string_view rawInput);

} // namespace fis
