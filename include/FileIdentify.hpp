#pragma once

#include <string>
#include <vector>

namespace fis {

// everything the file command figured out about one file
struct FileKind {
    std::vector<std::string> parts;
    std::string mime = "application/octet-stream";
    std::string charset = "binary";
    bool error = false;
};

void FileInfoCommand(const std::vector<std::string>& args);
void PrintFileTree(const std::string* name, const std::vector<std::string>& lines, bool error);
FileKind IdentifyFile(const std::string& target, bool follow);

} // namespace fis
