#include "FileCommands.hpp"
#include "ConsoleUtils.hpp"
#include "Program.hpp"
#include "SetColor.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <bcrypt.h>
#elif defined(FIS_HAS_OPENSSL)
#include <openssl/evp.h>
#endif

namespace fs = std::filesystem;

namespace fis {

namespace {

std::string JoinArgsFrom(const std::vector<std::string>& args, size_t start) {
    std::string out;
    for (size_t i = start; i < args.size(); ++i) {
        if (i > start) out += ' ';
        out += args[i];
    }
    return out;
}

uint64_t SafeLength(const fs::path& p) {
    std::error_code ec;
    auto sz = fs::file_size(p, ec);
    return ec ? 0 : sz;
}

std::time_t SafeLastWriteTime(const fs::path& p) {
    std::error_code ec;
    auto ftime = fs::last_write_time(p, ec);
    if (ec) return 0;
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
    return std::chrono::system_clock::to_time_t(sctp);
}

std::string FormatLsSize(uint64_t size) {
    const char* units[] = {"B", "K", "M", "G", "T"};
    double value = static_cast<double>(size);
    int unit = 0;
    while (value >= 1024.0 && unit < 4) {
        value /= 1024.0;
        unit++;
    }
    if (unit == 0) return std::to_string(size) + "B";
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(value >= 10.0 ? 0 : 1) << value << units[unit];
    return oss.str();
}

void HighlightKeyword(std::string_view text, std::string_view keyword) {
    std::string lowerText = ToLower(text);
    std::string lowerKey = ToLower(keyword);
    size_t start = 0;

    while (true) {
        size_t index = lowerText.find(lowerKey, start);
        if (index == std::string::npos) {
            SetColor(ConsoleColor::Yellow);
            std::cout << text.substr(start);
            break;
        }

        SetColor(ConsoleColor::Yellow);
        std::cout << text.substr(start, index - start);

        SetColor(ConsoleColor::Black);
        SetColor(ConsoleColor::Yellow, true);
        std::cout << text.substr(index, keyword.size());

        ResetColor();
        start = index + keyword.size();
    }
}

void PrintLsEntry(const fs::path& path, bool longFormat, bool human, bool classify, bool directoryOnly, const std::string& keyword) {
    std::error_code ec;
    bool isDir = fs::is_directory(path, ec);
    std::string name = directoryOnly ? path.string() : path.filename().string();
    uint64_t size = isDir ? 0 : SafeLength(path);
    std::time_t modified = SafeLastWriteTime(path);

    if (longFormat) {
        SetColor(ConsoleColor::DarkGray);
        std::cout << (isDir ? "drwxr-xr-x " : "-rw-r--r-- ");
        std::tm tmBuf{};
#ifdef _WIN32
        localtime_s(&tmBuf, &modified);
#else
        localtime_r(&modified, &tmBuf);
#endif
        char timeStr[32];
        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M", &tmBuf);
        std::string sizeStr = human ? FormatLsSize(size) : std::to_string(size);
        std::cout << timeStr << " " << std::right << std::setw(8) << sizeStr << " ";
    }

    if (!keyword.empty()) {
        HighlightKeyword(name, keyword);
    } else {
        SetColor(isDir ? ConsoleColor::Cyan : ConsoleColor::Yellow);
        std::cout << name;
    }

    if (classify && isDir)
        std::cout << "/";
    std::cout << '\n';
    ResForegroundColor();
}

void CopyDirectoryHelper(const fs::path& sourceDir, const fs::path& destDir, int& copied, int& skipped) {
    std::error_code ec;
    fs::create_directories(destDir, ec);
    if (ec) {
        skipped++;
        return;
    }

    for (const auto& entry : fs::directory_iterator(sourceDir, ec)) {
        if (entry.is_regular_file(ec)) {
            fs::path destFile = destDir / entry.path().filename();
            if (fs::weakly_canonical(entry.path(), ec) == fs::weakly_canonical(destFile, ec)) {
                skipped++;
                continue;
            }
            fs::copy_file(entry.path(), destFile, fs::copy_options::overwrite_existing, ec);
            if (ec) skipped++;
            else copied++;
        } else if (entry.is_directory(ec)) {
            CopyDirectoryHelper(entry.path(), destDir / entry.path().filename(), copied, skipped);
        }
    }
}

void MoveDirectoryHelper(const fs::path& sourceDir, const fs::path& destDir, int& moved, int& skipped) {
    std::error_code ec;
    fs::create_directories(destDir, ec);
    if (ec) {
        skipped++;
        return;
    }

    for (const auto& entry : fs::directory_iterator(sourceDir, ec)) {
        if (entry.is_regular_file(ec)) {
            fs::path destFile = destDir / entry.path().filename();
            if (fs::weakly_canonical(entry.path(), ec) == fs::weakly_canonical(destFile, ec)) {
                skipped++;
                continue;
            }
            fs::rename(entry.path(), destFile, ec);
            if (ec) {
                fs::copy_file(entry.path(), destFile, fs::copy_options::overwrite_existing, ec);
                if (!ec) {
                    fs::remove(entry.path(), ec);
                    moved++;
                } else {
                    skipped++;
                }
            } else {
                moved++;
            }
        } else if (entry.is_directory(ec)) {
            MoveDirectoryHelper(entry.path(), destDir / entry.path().filename(), moved, skipped);
            fs::remove_all(entry.path(), ec);
        }
    }

    fs::remove_all(sourceDir, ec);
}

void TreeHelper(const fs::path& path, const std::string& indent = "") {
    std::error_code ec;
    std::vector<fs::path> dirs;
    std::vector<fs::path> files;

    for (const auto& entry : fs::directory_iterator(path, ec)) {
        if (entry.is_directory(ec)) dirs.push_back(entry.path());
        else files.push_back(entry.path());
    }

    if (ec) {
        SetColor(ConsoleColor::Red);
        std::cout << indent << "[cannot read: " << ec.message() << "]\n";
        ResForegroundColor();
        return;
    }

    SetColor(ConsoleColor::Yellow);
    for (const auto& dir : dirs) {
        std::cout << indent << "├─ " << dir.filename().string() << "\n";
        TreeHelper(dir, indent + "│  ");
    }
    for (const auto& file : files) {
        std::cout << indent << "└─ " << file.filename().string() << "\n";
    }
    ResForegroundColor();
}

// Pure C++20 fallback SHA-256 & MD5 implementation when OpenSSL/BCrypt isn't linked
std::vector<uint8_t> ComputeSHA256Fallback(std::ifstream& in) {
    static constexpr uint32_t k[64] = {
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
    };
    uint32_t h[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    auto rotr = [](uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); };
    auto processBlock = [&](const uint8_t block[64]) {
        uint32_t w[64]{};
        for (int i = 0; i < 16; ++i) {
            w[i] = (uint32_t(block[i * 4]) << 24) | (uint32_t(block[i * 4 + 1]) << 16) |
                   (uint32_t(block[i * 4 + 2]) << 8) | uint32_t(block[i * 4 + 3]);
        }
        for (int i = 16; i < 64; ++i) {
            uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
        uint32_t e = h[4], f = h[5], g = h[6], hh = h[7];
        for (int i = 0; i < 64; ++i) {
            uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            uint32_t ch = (e & f) ^ ((~e) & g);
            uint32_t temp1 = hh + S1 + ch + k[i] + w[i];
            uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t temp2 = S0 + maj;
            hh = g; g = f; f = e; e = d + temp1;
            d = c; c = b; b = a; a = temp1 + temp2;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d;
        h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
    };

    uint8_t buf[64]{};
    size_t bufLen = 0;
    uint64_t totalBits = 0;

    while (in) {
        in.read(reinterpret_cast<char*>(buf + bufLen), 64 - bufLen);
        size_t got = static_cast<size_t>(in.gcount());
        bufLen += got;
        totalBits += got * 8;
        if (bufLen == 64) {
            processBlock(buf);
            bufLen = 0;
        }
    }

    buf[bufLen++] = 0x80;
    if (bufLen > 56) {
        while (bufLen < 64) buf[bufLen++] = 0;
        processBlock(buf);
        bufLen = 0;
    }
    while (bufLen < 56) buf[bufLen++] = 0;
    for (int i = 0; i < 8; ++i) {
        buf[56 + i] = static_cast<uint8_t>((totalBits >> ((7 - i) * 8)) & 0xFF);
    }
    processBlock(buf);

    std::vector<uint8_t> digest(32);
    for (int i = 0; i < 8; ++i) {
        digest[i * 4]     = static_cast<uint8_t>((h[i] >> 24) & 0xFF);
        digest[i * 4 + 1] = static_cast<uint8_t>((h[i] >> 16) & 0xFF);
        digest[i * 4 + 2] = static_cast<uint8_t>((h[i] >> 8) & 0xFF);
        digest[i * 4 + 3] = static_cast<uint8_t>(h[i] & 0xFF);
    }
    return digest;
}

std::vector<uint8_t> ComputeHashBytes(const fs::path& filePath, const std::string& algo) {
    std::ifstream in(filePath, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open file");

#ifdef _WIN32
    LPCWSTR algId = BCRYPT_SHA256_ALGORITHM;
    if (algo == "MD5") algId = BCRYPT_MD5_ALGORITHM;
    else if (algo == "SHA1") algId = BCRYPT_SHA1_ALGORITHM;
    else if (algo == "SHA384") algId = BCRYPT_SHA384_ALGORITHM;
    else if (algo == "SHA512") algId = BCRYPT_SHA512_ALGORITHM;

    BCRYPT_ALG_HANDLE hAlg = nullptr;
    BCRYPT_HASH_HANDLE hHash = nullptr;
    BCryptOpenAlgorithmProvider(&hAlg, algId, nullptr, 0);
    DWORD hashLen = 0, cbData = 0;
    BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, reinterpret_cast<PUCHAR>(&hashLen), sizeof(DWORD), &cbData, 0);
    BCryptCreateHash(hAlg, &hHash, nullptr, 0, nullptr, 0, 0);

    std::vector<char> chunk(65536);
    while (in) {
        in.read(chunk.data(), chunk.size());
        std::streamsize n = in.gcount();
        if (n > 0) BCryptHashData(hHash, reinterpret_cast<PUCHAR>(chunk.data()), static_cast<ULONG>(n), 0);
    }
    std::vector<uint8_t> out(hashLen);
    BCryptFinishHash(hHash, out.data(), hashLen, 0);
    BCryptDestroyHash(hHash);
    BCryptCloseAlgorithmProvider(hAlg, 0);
    return out;
#elif defined(FIS_HAS_OPENSSL)
    const EVP_MD* md = EVP_sha256();
    if (algo == "MD5") md = EVP_md5();
    else if (algo == "SHA1") md = EVP_sha1();
    else if (algo == "SHA384") md = EVP_sha384();
    else if (algo == "SHA512") md = EVP_sha512();

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, md, nullptr);
    std::vector<char> chunk(65536);
    while (in) {
        in.read(chunk.data(), chunk.size());
        std::streamsize n = in.gcount();
        if (n > 0) EVP_DigestUpdate(ctx, chunk.data(), static_cast<size_t>(n));
    }
    unsigned int len = 0;
    std::vector<uint8_t> out(EVP_MAX_MD_SIZE);
    EVP_DigestFinal_ex(ctx, out.data(), &len);
    EVP_MD_CTX_free(ctx);
    out.resize(len);
    return out;
#else
    if (algo != "SHA256") {
        throw std::runtime_error("compile with OpenSSL for MD5/SHA1/SHA384/SHA512 (SHA-256 built-in)");
    }
    return ComputeSHA256Fallback(in);
#endif
}

std::string BytesToHex(const std::vector<uint8_t>& bytes) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (uint8_t b : bytes) {
        oss << std::setw(2) << static_cast<int>(b);
    }
    return oss.str();
}

std::vector<uint8_t> HexToBytes(std::string_view hex) {
    if (hex.size() % 2 != 0) throw std::invalid_argument("hex length");
    std::vector<uint8_t> out;
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        std::string byteStr(hex.substr(i, 2));
        size_t idx = 0;
        int val = std::stoi(byteStr, &idx, 16);
        if (idx != 2) throw std::invalid_argument("hex char");
        out.push_back(static_cast<uint8_t>(val));
    }
    return out;
}

} // namespace

// mkdir (create folder)
void Mkdir(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SetColor(ConsoleColor::Cyan);
        std::cout << "usage: mkdir <folder>\n";
        SetColor(ConsoleColor::Red);
        std::cout << "i recommmend NOT adding a space in the name\n";
        ResForegroundColor();
        return;
    }

    fs::path path = fs::path(currentDir) / JoinArgsFrom(args, 1);
    std::error_code ec;

    if (!fs::exists(path, ec)) {
        fs::create_directories(path, ec);
        SetColor(ConsoleColor::Cyan);
        std::cout << "directory/folder created :D\n";
    } else {
        SetColor(ConsoleColor::Red);
        std::cout << "directory/folder already exists bro\n";
    }

    ResForegroundColor();
}

// rmdir (remove folder)
void Rmdir(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SetColor(ConsoleColor::Cyan);
        std::cout << "usage: rmdir <folder>\n";
        std::cout << "<folder> - the ";
        SetColor(ConsoleColor::Red);
        std::cout << "EMPTY FOLDER ";
        SetColor(ConsoleColor::Cyan);
        std::cout << "needed to delete\n";
        ResForegroundColor();
        return;
    }

    fs::path path = fs::path(currentDir) / JoinArgsFrom(args, 1);
    std::error_code ec;

    if (!fs::is_directory(path, ec)) {
        SetColor(ConsoleColor::Red);
        std::cout << "directory/folder not found or invalid :c\n";
        ResForegroundColor();
        return;
    }

    if (!fs::is_empty(path, ec)) {
        SetColor(ConsoleColor::Red);
        std::cout << "bro this directory/folder aint empty\n";
        std::cout << "use rm -rf if u REALLY mean it\n";
        ResForegroundColor();
        return;
    }

    if (fs::remove(path, ec)) {
        SetColor(ConsoleColor::Cyan);
        std::cout << "directory/folder removed :D\n";
    } else {
        SetColor(ConsoleColor::Red);
        std::cout << "failed to remove folder: " << ec.message() << "\n";
    }

    ResForegroundColor();
}

// rm (yes, /r, /f, /rf and /fr commands too)
void Rm(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SetColor(ConsoleColor::Cyan);
        std::cout << "usage: rm <file> OR rm -r <folder> OR rm -f <file> OR rm -rf <folder>\n";
        std::cout << "-r - delete a entire folder with its contents and ye i mean it bro\n";
        std::cout << "-f - extrme force deletion yes\n";
        SetColor(ConsoleColor::Red);
        std::cout << "-rf and -fr combine both (be careful)\n";
        ResForegroundColor();
        return;
    }

    bool recursive = false;
    bool force = false;
    size_t pathStart = 1;

    while (pathStart < args.size()) {
        std::string arg = ToLower(args[pathStart]);
        if (arg == "-r") {
            recursive = true;
            pathStart++;
        } else if (arg == "-f") {
            force = true;
            pathStart++;
        } else if (arg == "-rf" || arg == "-fr") {
            recursive = true;
            force = true;
            pathStart++;
        } else {
            break;
        }
    }

    if (pathStart >= args.size()) {
        SetColor(ConsoleColor::Cyan);
        std::cout << "usage: rm <file>\n";
        std::cout << "       rm -r <folder>\n";
        std::cout << "       rm -f <file>\n";
        std::cout << "       rm -rf <folder>\n";
        ResForegroundColor();
        return;
    }

    std::string target = JoinArgsFrom(args, pathStart);
    fs::path rawTarget(target);
    std::error_code ec;
    fs::path targetPath = rawTarget.is_absolute()
        ? fs::weakly_canonical(rawTarget, ec)
        : fs::weakly_canonical(fs::path(currentDir) / rawTarget, ec);
    if (ec) targetPath = rawTarget;

    // nuclear rm -rf / (or C:) protection
    fs::path rootPath = targetPath.root_path();
    std::string tStr = targetPath.string();
    std::string rStr = rootPath.string();
    while (tStr.size() > 1 && (tStr.back() == '/' || tStr.back() == '\\')) tStr.pop_back();
    while (rStr.size() > 1 && (rStr.back() == '/' || rStr.back() == '\\')) rStr.pop_back();

    if (!rStr.empty() && EqualsIgnoreCase(tStr, rStr)) {
        SetColor(ConsoleColor::DarkRed);
        std::cout << "who are you, actually?\n";
        ResForegroundColor();
        return;
    }

    if (fs::is_directory(targetPath, ec)) {
        if (!recursive) {
            SetColor(ConsoleColor::Red);
            std::cout << "target is a directory/folder; use -r to delete it recursively\n";
            ResForegroundColor();
            return;
        }

        SetColor(ConsoleColor::Red);
        if (force)
            std::cout << "FORCE DELETE '" << target << "' AND ALL CONTENTS????? (y/n): " << std::flush;
        else
            std::cout << "PERMANENTLY DELETE '" << target << "' AND ALL CONTENTS????? (y/n): " << std::flush;

        std::string confirm;
        std::getline(std::cin, confirm);
        if (ToLower(Trim(confirm)) != "y") {
            SetColor(ConsoleColor::Cyan);
            std::cout << "oh cool at least u cancelled it :skull:\n";
            ResForegroundColor();
            return;
        }

        fs::remove_all(targetPath, ec);
        SetColor(ConsoleColor::Red);
        if (!ec) {
            std::cout << (force ? "directory/folder force-nuked, no more undo\n" : "directory/folder nuked, no more undo\n");
        } else {
            std::cout << "failed to delete directory/folder: " << ec.message() << "\n";
        }
        ResForegroundColor();
        return;
    }

    if (fs::is_regular_file(targetPath, ec)) {
        SetColor(ConsoleColor::Red);
        if (force)
            std::cout << "FORCE DELETE '" << target << "'????? (y/n): " << std::flush;
        else
            std::cout << "PERMANENTLY DELETE '" << target << "'????? (y/n): " << std::flush;

        std::string confirm;
        std::getline(std::cin, confirm);
        if (ToLower(Trim(confirm)) != "y") {
            SetColor(ConsoleColor::Cyan);
            std::cout << "oh cool at least u cancelled it :skull:\n";
            ResForegroundColor();
            return;
        }

        fs::remove(targetPath, ec);
        SetColor(ConsoleColor::Red);
        if (!ec) {
            std::cout << (force ? "file force-nuked, no more undo\n" : "file nuked, no more undo\n");
        } else {
            std::cout << "failed to delete file: " << ec.message() << "\n";
        }
        ResForegroundColor();
        return;
    }

    SetColor(ConsoleColor::Red);
    std::cout << "file/folder not found or invalid :c\n";
    ResForegroundColor();
}

// cd
void Cd(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SetColor(ConsoleColor::Cyan);
        std::cout << "current directory/folder: " << currentDir << "\n";
        ResForegroundColor();
        return;
    }

    std::string target = JoinArgsFrom(args, 1);
    fs::path newPath;
    std::error_code ec;

    if (target == "..") {
        fs::path parent = fs::path(currentDir).parent_path();
        newPath = parent.empty() ? fs::path(currentDir) : parent;
    } else if (fs::path(target).is_absolute()) {
        newPath = fs::path(target);
    } else {
        newPath = fs::path(currentDir) / target;
    }

    fs::path canonical = fs::weakly_canonical(newPath, ec);
    if (!ec) newPath = canonical;

    if (fs::is_directory(newPath, ec)) {
        currentDir = newPath.string();
    } else {
        SetColor(ConsoleColor::Red);
        std::cout << "ts was barely even a directory/folder\n";
        ResForegroundColor();
    }
}

void ls(const std::vector<std::string>& args) {
    bool all = false, almostAll = false, longFormat = false, human = false;
    bool recursive = false, reverse = false, byTime = false, bySize = false;
    bool directoryOnly = false, classify = false;
    std::vector<std::string> paths;
    std::string keyword;

    for (size_t i = 1; i < args.size(); i++) {
        const std::string& arg = args[i];
        if (arg == "--") {
            for (size_t j = i + 1; j < args.size(); ++j) paths.push_back(args[j]);
            break;
        }

        if (arg.starts_with("--")) {
            if (arg == "--all") all = true;
            else if (arg == "--almost-all") almostAll = true;
            else if (arg == "--long") longFormat = true;
            else if (arg == "--human-readable") human = true;
            else if (arg == "--recursive") recursive = true;
            else if (arg == "--reverse") reverse = true;
            else if (arg == "--time") byTime = true;
            else if (arg == "--size") bySize = true;
            else if (arg == "--directory") directoryOnly = true;
            else if (arg == "--classify") classify = true;
            else if (arg == "--find" || arg == "--keyword") {
                if (i + 1 >= args.size()) {
                    SetColor(ConsoleColor::Red);
                    std::cout << "where keyword\n";
                    ResForegroundColor();
                    return;
                }
                keyword = args[++i];
            } else if (arg == "--help") {
                SetColor(ConsoleColor::Cyan);
                std::cout << "usage: ls [-aAhlRrtSdF1] [path ...]\n";
                std::cout << "-a (or -A)  include hidden entries / exclude . and ..\n";
                std::cout << "-l - long listing with size and modified time\n";
                std::cout << "-h - human-readable sizes (use with -l)\n";
                std::cout << "-R - recursive listing\n";
                std::cout << "-r - reverse sort; -t sort by modified time; -S sort by size\n";
                std::cout << "-d - list directory itself; -F append / to directories\n";
                std::cout << "-1 - one entry per line\n";
                std::cout << "-f (or -k) - find a file in current directory\n";
                ResForegroundColor();
                return;
            } else {
                SetColor(ConsoleColor::Red);
                std::cout << "'" << arg << "' is barely even an option\n";
                ResForegroundColor();
                return;
            }
            continue;
        }

        if (arg.starts_with("-") && arg.size() > 1) {
            for (size_t k = 1; k < arg.size(); ++k) {
                char flag = arg[k];
                switch (flag) {
                    case 'a': all = true; break;
                    case 'A': almostAll = true; break;
                    case 'l': longFormat = true; break;
                    case 'h': human = true; break;
                    case 'R': recursive = true; break;
                    case 'r': reverse = true; break;
                    case 't': byTime = true; break;
                    case 'S': bySize = true; break;
                    case 'd': directoryOnly = true; break;
                    case 'F': classify = true; break;
                    case '1': break;
                    case 'f':
                    case 'k':
                        if (i + 1 >= args.size()) {
                            SetColor(ConsoleColor::Red);
                            std::cout << "where keyword\n";
                            ResForegroundColor();
                            return;
                        }
                        keyword = args[++i];
                        break;
                    default:
                        SetColor(ConsoleColor::Red);
                        std::cout << "the fuck you mean '" << flag << "'\n";
                        ResForegroundColor();
                        return;
                }
            }
        } else {
            paths.push_back(arg);
        }
    }

    if (paths.empty()) paths.push_back(".");

    std::error_code ec;
    for (const auto& input : paths) {
        fs::path raw(input);
        fs::path target = raw.is_absolute()
            ? fs::weakly_canonical(raw, ec)
            : fs::weakly_canonical(fs::path(currentDir) / raw, ec);
        if (ec) target = raw;

        if (!fs::exists(target, ec)) {
            SetColor(ConsoleColor::Red);
            std::cout << "'" << input << "' is barely even a file\n";
            ResForegroundColor();
            continue;
        }

        if (paths.size() > 1 || recursive) {
            SetColor(ConsoleColor::Cyan);
            std::cout << target.string() << ":\n";
        }

        if (fs::is_regular_file(target, ec) || directoryOnly) {
            PrintLsEntry(target, longFormat, human, classify, directoryOnly && fs::is_directory(target, ec), keyword);
            continue;
        }

        std::vector<fs::path> listing;
        for (const auto& entry : fs::directory_iterator(target, ec)) {
            std::string fname = entry.path().filename().string();
            if (!all && !almostAll && !fname.empty() && fname[0] == '.') continue;
            listing.push_back(entry.path());
        }

        if (byTime) {
            std::sort(listing.begin(), listing.end(), [](const fs::path& a, const fs::path& b) {
                return SafeLastWriteTime(a) > SafeLastWriteTime(b);
            });
        } else if (bySize) {
            std::sort(listing.begin(), listing.end(), [](const fs::path& a, const fs::path& b) {
                return SafeLength(a) > SafeLength(b);
            });
        } else {
            std::sort(listing.begin(), listing.end(), [](const fs::path& a, const fs::path& b) {
                return ToLower(a.filename().string()) < ToLower(b.filename().string());
            });
        }

        if (reverse) {
            std::reverse(listing.begin(), listing.end());
        }

        for (const auto& entry : listing) {
            PrintLsEntry(entry, longFormat, human, classify, false, keyword);
        }

        if (recursive) {
            for (const auto& dir : listing) {
                if (fs::is_directory(dir, ec)) {
                    std::cout << '\n';
                    SetColor(ConsoleColor::Cyan);
                    std::cout << dir.string() << ":\n";
                    ResForegroundColor();
                    std::string flags = "-l";
                    if (human) flags += 'h';
                    if (all) flags += 'a';
                    ls({"ls", flags, dir.string()});
                }
            }
        }
    }

    ResForegroundColor();
}

void Cat(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SetColor(ConsoleColor::Cyan);
        std::cout << "usage: cat <filename>\n";
        std::cout << "<filename> - the file needed to echo out\n";
        ResForegroundColor();
        return;
    }

    fs::path path = fs::path(currentDir) / JoinArgsFrom(args, 1);
    std::error_code ec;

    if (fs::is_regular_file(path, ec)) {
        std::ifstream file(path);
        if (file) {
            SetColor(ConsoleColor::Yellow);
            std::cout << file.rdbuf() << '\n';
        } else {
            SetColor(ConsoleColor::Red);
            std::cout << "failed to read the file\n";
        }
    } else {
        SetColor(ConsoleColor::Red);
        std::cout << "file not found/invalid :c\n";
    }

    ResForegroundColor();
}

void Copy(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        SetColor(ConsoleColor::Cyan);
        std::cout << "usage: copy/cop/cope [-r] <source/file> <destination>\n";
        std::cout << "[-r] - copy the entire folder, including its (sub)folders\n";
        std::cout << "<source/file> - the source/file needed to copy\n";
        std::cout << "<destination> - the destination needed to indicate where should the copied source/file go\n";
        ResForegroundColor();
        return;
    }

    bool recursive = false;
    std::vector<std::string> filtered;
    for (const auto& a : args) {
        if (a == "-r") recursive = true;
        else filtered.push_back(a);
    }

    if (filtered.size() < 3) {
        SetColor(ConsoleColor::Cyan);
        std::cout << "usage: copy/cop/cope [-r] <source> <destination>\n";
        ResForegroundColor();
        return;
    }

    std::error_code ec;
    fs::path sourcePath = fs::weakly_canonical(fs::path(currentDir) / filtered[1], ec);
    fs::path destPath = fs::weakly_canonical(fs::path(currentDir) / filtered[2], ec);

    if (sourcePath == destPath) {
        SetColor(ConsoleColor::Yellow);
        std::cout << "source and destination are the same :skull:\n";
        ResForegroundColor();
        return;
    }

    if (fs::is_regular_file(sourcePath, ec)) {
        if (fs::is_directory(destPath, ec))
            destPath /= sourcePath.filename();

        fs::copy_file(sourcePath, destPath, fs::copy_options::overwrite_existing, ec);
        if (!ec) {
            SetColor(ConsoleColor::Cyan);
            std::cout << "1 file copied :D\n";
        } else {
            SetColor(ConsoleColor::Red);
            std::cout << "copy failed: " << ec.message() << "\n";
        }
        ResForegroundColor();
        return;
    }

    if (fs::is_directory(sourcePath, ec)) {
        if (!recursive) {
            SetColor(ConsoleColor::Red);
            std::cout << "use -r to copy directories\n";
            ResForegroundColor();
            return;
        }
        int copied = 0, skipped = 0;
        CopyDirectoryHelper(sourcePath, destPath, copied, skipped);
        SetColor(ConsoleColor::Cyan);
        std::cout << copied << " files copied, " << skipped << " skipped :D\n";
        ResForegroundColor();
        return;
    }

    SetColor(ConsoleColor::Red);
    std::cout << "source not found :c\n";
    ResForegroundColor();
}

void Move(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        SetColor(ConsoleColor::Cyan);
        std::cout << "usage: move/mov/mv [-r] <source/file> <destination>\n";
        std::cout << "[-r] - move the entire folder, including its (sub)folders\n";
        std::cout << "<source/file> - the source/file needed to move\n";
        std::cout << "<destination> - the destination needed to indicate where should the source/file move to\n";
        ResForegroundColor();
        return;
    }

    bool recursive = false;
    std::vector<std::string> filtered;
    for (const auto& a : args) {
        if (a == "-r") recursive = true;
        else filtered.push_back(a);
    }

    if (filtered.size() < 3) {
        SetColor(ConsoleColor::Cyan);
        std::cout << "usage: move/mov/mv [-r] <source/file> <destination>\n";
        ResForegroundColor();
        return;
    }

    std::error_code ec;
    fs::path sourcePath = fs::weakly_canonical(fs::path(currentDir) / filtered[1], ec);
    fs::path destPath = fs::weakly_canonical(fs::path(currentDir) / filtered[2], ec);

    if (sourcePath == destPath) {
        SetColor(ConsoleColor::Red);
        std::cout << "source and destination are the same :skull:\n";
        ResForegroundColor();
        return;
    }

    if (fs::is_regular_file(sourcePath, ec)) {
        if (fs::is_directory(destPath, ec))
            destPath /= sourcePath.filename();

        fs::rename(sourcePath, destPath, ec);
        if (ec) {
            fs::copy_file(sourcePath, destPath, fs::copy_options::overwrite_existing, ec);
            if (!ec) fs::remove(sourcePath, ec);
        }

        if (!ec) {
            SetColor(ConsoleColor::Cyan);
            std::cout << "1 file moved :D\n";
        } else {
            SetColor(ConsoleColor::Red);
            std::cout << "move failed: " << ec.message() << "\n";
        }
        ResForegroundColor();
        return;
    }

    if (fs::is_directory(sourcePath, ec)) {
        if (!recursive) {
            SetColor(ConsoleColor::Red);
            std::cout << "use -r to move directories\n";
            ResForegroundColor();
            return;
        }
        int moved = 0, skipped = 0;
        MoveDirectoryHelper(sourcePath, destPath, moved, skipped);
        SetColor(ConsoleColor::Cyan);
        std::cout << moved << " items moved, " << skipped << " skipped :D\n";
        ResForegroundColor();
        return;
    }

    SetColor(ConsoleColor::Red);
    std::cout << "source not found :c\n";
    ResForegroundColor();
}

void TreeCommand(const std::vector<std::string>& args) {
    fs::path target = currentDir;

    SetColor(ConsoleColor::Cyan);
    std::cout << "hollup, creating tree for the directory: " << currentDir << "\n\n";

    if (args.size() >= 2)
        target = fs::path(currentDir) / args[1];

    std::error_code ec;
    if (!fs::is_directory(target, ec)) {
        SetColor(ConsoleColor::Red);
        std::cout << "folder not found or invalid :c\n";
        ResForegroundColor();
        return;
    }

    SetColor(ConsoleColor::Cyan);
    std::cout << target.filename().string() << "\n";
    TreeHelper(target);
    ResForegroundColor();
}

void Rename(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        SetColor(ConsoleColor::Cyan);
        std::cout << "usage: rename <old> <new>\n";
        std::cout << "<old> is source file/folder\n";
        std::cout << "<new> is new name\n";
        ResForegroundColor();
        return;
    }

    fs::path oldPath = fs::path(currentDir) / args[1];
    fs::path newPath = fs::path(currentDir) / args[2];
    std::error_code ec;

    if (!fs::exists(oldPath, ec)) {
        SetColor(ConsoleColor::Red);
        std::cout << "source not found :c\n";
        ResForegroundColor();
        return;
    }

    fs::rename(oldPath, newPath, ec);
    if (!ec) {
        SetColor(ConsoleColor::Cyan);
        std::cout << "renamed :D\n";
    } else {
        SetColor(ConsoleColor::Red);
        std::cout << "failed to rename the file/source: " << ec.message() << "\n";
    }
    ResForegroundColor();
}

void ZipFolder(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SetColor(ConsoleColor::Cyan);
        std::cout << "usage: zip <folder>\n";
        std::cout << "<folder> is the folder needed to compress/zip\n";
        ResForegroundColor();
        return;
    }

    fs::path folder = fs::path(currentDir) / args[1];
    std::string zip = folder.string() + ".zip";

    std::error_code ec;
    if (!fs::is_directory(folder, ec)) {
        SetColor(ConsoleColor::Red);
        std::cout << "zip failed: folder not found\n";
        ResForegroundColor();
        return;
    }

#ifdef _WIN32
    std::string cmd = "powershell -NoProfile -Command \"Compress-Archive -Path '" +
                      folder.string() + "\\*' -DestinationPath '" + zip + "' -Force\"";
#else
    std::string cmd = "cd \"" + folder.parent_path().string() + "\" && zip -rq \"" +
                      zip + "\" \"" + folder.filename().string() + "\"";
#endif
    int rc = std::system(cmd.c_str());
    if (rc == 0) {
        SetColor(ConsoleColor::Cyan);
        std::cout << "zipped :D\n";
    } else {
        SetColor(ConsoleColor::Red);
        std::cout << "zip failed (exit code " << rc << ")\n";
    }
    ResForegroundColor();
}

void UnzipFile(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SetColor(ConsoleColor::Cyan);
        std::cout << "usage: unzip <file.zip>\n";
        std::cout << "<file.zip> is the archive to extract\n";
        ResForegroundColor();
        return;
    }

    fs::path zip = fs::path(currentDir) / args[1];
    std::string outDir = zip.string() + "_unzipped";

    std::error_code ec;
    if (!fs::is_regular_file(zip, ec)) {
        SetColor(ConsoleColor::Red);
        std::cout << "unzip failed: archive not found\n";
        ResForegroundColor();
        return;
    }

#ifdef _WIN32
    std::string cmd = "powershell -NoProfile -Command \"Expand-Archive -Path '" +
                      zip.string() + "' -DestinationPath '" + outDir + "' -Force\"";
#else
    std::string cmd = "unzip -oq \"" + zip.string() + "\" -d \"" + outDir + "\"";
#endif
    int rc = std::system(cmd.c_str());
    if (rc == 0) {
        SetColor(ConsoleColor::Cyan);
        std::cout << "unzipped :D\n";
    } else {
        SetColor(ConsoleColor::Red);
        std::cout << "unzip failed (exit code " << rc << ")\n";
    }
    ResForegroundColor();
}

void HashFile(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        SetColor(ConsoleColor::Cyan);
        std::cout << "usage: hash -<algorithm> [-match] <filename> [hash]\n";
        std::cout << "-md5 - use MD5\n";
        std::cout << "-sha <bits> - use SHA-1, SHA-256, SHA-384, or SHA-512\n";
        std::cout << "-match - compare the calculated hash against a provided hash\n";
        std::cout << "<filename> - file to hash\n";
        std::cout << "[hash] - expected hash when using -match\n";
        ResForegroundColor();
        return;
    }

    std::string algorithm;
    bool match = false;
    std::string filename;
    std::string expectedHash;

    for (size_t i = 1; i < args.size(); i++) {
        const std::string& arg = args[i];

        if (EqualsIgnoreCase(arg, "-md5")) {
            algorithm = "MD5";
        } else if (EqualsIgnoreCase(arg, "-sha")) {
            if (i + 1 >= args.size()) {
                SetColor(ConsoleColor::Red);
                std::cout << "hash: -sha requires a hash size\n";
                ResForegroundColor();
                return;
            }
            const std::string& bits = args[++i];
            if (bits == "1") algorithm = "SHA1";
            else if (bits == "256") algorithm = "SHA256";
            else if (bits == "384") algorithm = "SHA384";
            else if (bits == "512") algorithm = "SHA512";
            else {
                SetColor(ConsoleColor::Red);
                std::cout << "hash: unsupported SHA size: " << bits << "\n";
                ResForegroundColor();
                return;
            }
        } else if (EqualsIgnoreCase(arg, "-match")) {
            match = true;
        } else if (filename.empty()) {
            filename = arg;
        } else if (match && expectedHash.empty()) {
            expectedHash = arg;
        } else {
            SetColor(ConsoleColor::Red);
            std::cout << "hash: unexpected argument: " << arg << "\n";
            ResForegroundColor();
            return;
        }
    }

    if (algorithm.empty()) {
        SetColor(ConsoleColor::Red);
        std::cout << "hash: no hashing algorithm specified\n";
        ResForegroundColor();
        return;
    }
    if (filename.empty()) {
        SetColor(ConsoleColor::Red);
        std::cout << "hash: no filename specified\n";
        ResForegroundColor();
        return;
    }
    if (match && expectedHash.empty()) {
        SetColor(ConsoleColor::Red);
        std::cout << "hash: -match requires an expected hash\n";
        ResForegroundColor();
        return;
    }

    fs::path fpath(filename);
    fs::path filePath = fpath.is_absolute() ? fpath : (fs::path(currentDir) / fpath);

    std::error_code ec;
    if (!fs::exists(filePath, ec)) {
        SetColor(ConsoleColor::Red);
        std::cout << "hash: file not found: " << filePath.string() << "\n";
        ResForegroundColor();
        return;
    }

    try {
        auto hashBytes = ComputeHashBytes(filePath, algorithm);
        std::string actualHash = BytesToHex(hashBytes);

        if (match) {
            auto expectedBytes = HexToBytes(expectedHash);
            bool equal = (expectedBytes.size() == hashBytes.size());
            volatile uint8_t diff = 0;
            if (equal) {
                for (size_t i = 0; i < hashBytes.size(); ++i) {
                    diff |= (hashBytes[i] ^ expectedBytes[i]);
                }
                equal = (diff == 0);
            }

            if (equal) {
                SetColor(ConsoleColor::Cyan);
                std::cout << "hash match :D\n";
            } else {
                SetColor(ConsoleColor::Red);
                std::cout << "hash mismatch\n";
                SetColor(ConsoleColor::Cyan);
                std::cout << "expected: " << expectedHash << "\n";
                std::cout << "actual:   " << actualHash << "\n";
            }
        } else {
            SetColor(ConsoleColor::Cyan);
            std::cout << algorithm << ": " << actualHash << "\n";
        }
    } catch (const std::invalid_argument&) {
        SetColor(ConsoleColor::Red);
        std::cout << "hash: expected hash must be valid hexadecimal\n";
    } catch (const std::exception& ex) {
        SetColor(ConsoleColor::Red);
        std::cout << "hash failed: " << ex.what() << "\n";
    }

    ResForegroundColor();
}

} // namespace fis
