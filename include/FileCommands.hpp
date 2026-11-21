#pragma once

#include <string>
#include <vector>

namespace fis {

void Mkdir(const std::vector<std::string>& args);
void Rmdir(const std::vector<std::string>& args);
void Rm(const std::vector<std::string>& args);
void Cd(const std::vector<std::string>& args);
void ls(const std::vector<std::string>& args);
void Cat(const std::vector<std::string>& args);
void Copy(const std::vector<std::string>& args);
void Move(const std::vector<std::string>& args);
void TreeCommand(const std::vector<std::string>& args);
void Rename(const std::vector<std::string>& args);
void ZipFolder(const std::vector<std::string>& args);
void UnzipFile(const std::vector<std::string>& args);
void HashFile(const std::vector<std::string>& args);

} // namespace fis
