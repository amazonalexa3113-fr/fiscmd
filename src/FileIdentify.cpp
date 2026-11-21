#include "FileIdentify.hpp"
#include "ConsoleUtils.hpp"
#include "Program.hpp"
#include "SetColor.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>

namespace fs = std::filesystem;

namespace fis {

namespace {

bool StartsWithBytes(const std::vector<uint8_t>& data, std::initializer_list<uint8_t> sig) {
    if (data.size() < sig.size()) return false;
    size_t i = 0;
    for (uint8_t b : sig) {
        if (data[i++] != b) return false;
    }
    return true;
}

uint64_t ReadNum(const std::vector<uint8_t>& d, int64_t offset, int size, bool bigEndian) {
    if (offset < 0 || offset + size > static_cast<int64_t>(d.size()))
        return 0;

    uint64_t value = 0;
    for (int i = 0; i < size; i++) {
        int64_t index = bigEndian ? offset + i : offset + size - 1 - i;
        value = (value << 8) | d[static_cast<size_t>(index)];
    }
    return value;
}

bool HasAscii(const std::vector<uint8_t>& d, size_t offset, std::string_view text) {
    if (offset + text.size() > d.size()) return false;
    for (size_t i = 0; i < text.size(); i++) {
        if (d[offset + i] != static_cast<uint8_t>(text[i])) return false;
    }
    return true;
}

std::string BytesToHexLower(const uint8_t* ptr, size_t len) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; ++i) {
        oss << std::setw(2) << static_cast<int>(ptr[i]);
    }
    return oss.str();
}

std::vector<uint8_t> ReadRange(const std::string& path, int64_t offset, int count) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream || offset < 0 || count < 0) return {};
    stream.seekg(0, std::ios::end);
    int64_t len = stream.tellg();
    if (offset + count > len) return {};
    stream.seekg(offset, std::ios::beg);
    std::vector<uint8_t> buf(count);
    stream.read(reinterpret_cast<char*>(buf.data()), count);
    if (stream.gcount() != count) return {};
    return buf;
}

bool Bin(FileKind& kind, std::string mime, std::initializer_list<std::string> parts) {
    for (const auto& p : parts) kind.parts.push_back(p);
    kind.mime = std::move(mime);
    kind.charset = "binary";
    return true;
}

bool BinVec(FileKind& kind, std::string mime, std::vector<std::string> parts) {
    kind.parts.insert(kind.parts.end(), parts.begin(), parts.end());
    kind.mime = std::move(mime);
    kind.charset = "binary";
    return true;
}

bool DescribeElf(const std::vector<uint8_t>& d, const std::string& path, FileKind& kind) {
    bool is64 = d.size() > 4 && d[4] == 2;
    bool big = d.size() > 5 && d[5] == 2;
    int abi = d.size() > 7 ? d[7] : 0;

    uint16_t type = static_cast<uint16_t>(ReadNum(d, 16, 2, big));
    uint16_t machine = static_cast<uint16_t>(ReadNum(d, 18, 2, big));
    uint32_t version = static_cast<uint32_t>(ReadNum(d, 20, 4, big));

    int64_t phoff = static_cast<int64_t>(is64 ? ReadNum(d, 32, 8, big) : ReadNum(d, 28, 4, big));
    int64_t shoff = static_cast<int64_t>(is64 ? ReadNum(d, 40, 8, big) : ReadNum(d, 32, 4, big));
    int phentsize = static_cast<int>(ReadNum(d, is64 ? 54 : 42, 2, big));
    int phnum = static_cast<int>(ReadNum(d, is64 ? 56 : 44, 2, big));
    int shentsize = static_cast<int>(ReadNum(d, is64 ? 58 : 46, 2, big));
    int shnum = static_cast<int>(ReadNum(d, is64 ? 60 : 48, 2, big));

    std::string machineName;
    switch (machine) {
        case 2: machineName = "SPARC"; break;
        case 3: machineName = "Intel 80386"; break;
        case 4: machineName = "Motorola m68k"; break;
        case 8: machineName = "MIPS"; break;
        case 20: machineName = "PowerPC or cisco 4500"; break;
        case 21: machineName = "64-bit PowerPC or cisco 7500"; break;
        case 22: machineName = "IBM S/390"; break;
        case 40: machineName = "ARM"; break;
        case 42: machineName = "Renesas SH"; break;
        case 43: machineName = "SPARC V9"; break;
        case 50: machineName = "IA-64"; break;
        case 62: machineName = "x86-64"; break;
        case 183: machineName = "ARM aarch64"; break;
        case 243: machineName = "UCB RISC-V"; break;
        default: {
            std::ostringstream oss;
            oss << "*unknown arch 0x" << std::hex << machine << "*";
            machineName = oss.str();
            break;
        }
    }

    std::string abiName;
    switch (abi) {
        case 1: abiName = "HP-UX"; break;
        case 2: abiName = "NetBSD"; break;
        case 3: abiName = "GNU/Linux"; break;
        case 6: abiName = "Solaris"; break;
        case 9: abiName = "FreeBSD"; break;
        case 12: abiName = "OpenBSD"; break;
        default: abiName = "SYSV"; break;
    }

    std::string interpreter;
    bool hasInterp = false;
    bool hasDynamic = false;
    std::string buildId;
    std::string abiTag;

    for (int i = 0; i < phnum && i < 128; i++) {
        int64_t ph = phoff + static_cast<int64_t>(i) * phentsize;
        if (phentsize <= 0 || ph + phentsize > static_cast<int64_t>(d.size()))
            break;

        uint32_t pType = static_cast<uint32_t>(ReadNum(d, ph, 4, big));
        int64_t pOffset = static_cast<int64_t>(is64 ? ReadNum(d, ph + 8, 8, big) : ReadNum(d, ph + 4, 4, big));
        int64_t pFileSize = static_cast<int64_t>(is64 ? ReadNum(d, ph + 32, 8, big) : ReadNum(d, ph + 16, 4, big));

        if (pType == 2) {
            hasDynamic = true;
        } else if (pType == 3) {
            hasInterp = true;
            if (pOffset >= 0 && pOffset < static_cast<int64_t>(d.size())) {
                size_t start = static_cast<size_t>(pOffset);
                size_t end = start;
                while (end < d.size() && d[end] != 0) end++;
                interpreter.assign(reinterpret_cast<const char*>(d.data() + start), end - start);
            }
        } else if (pType == 4) {
            int64_t n = pOffset;
            int64_t noteEnd = std::min(pOffset + pFileSize, static_cast<int64_t>(d.size()));
            while (n >= 0 && n + 12 <= noteEnd) {
                int64_t nameSize = static_cast<int64_t>(ReadNum(d, n, 4, big));
                int64_t descSize = static_cast<int64_t>(ReadNum(d, n + 4, 4, big));
                uint32_t noteType = static_cast<uint32_t>(ReadNum(d, n + 8, 4, big));

                int64_t nameStart = n + 12;
                int64_t descStart = nameStart + ((nameSize + 3) & ~3LL);
                if (descStart + descSize > static_cast<int64_t>(d.size()))
                    break;

                bool gnu = nameSize == 4 && HasAscii(d, static_cast<size_t>(nameStart), "GNU");
                if (gnu && noteType == 3 && (descSize == 20 || descSize == 16)) {
                    std::string hex = BytesToHexLower(d.data() + descStart, static_cast<size_t>(descSize));
                    buildId = std::string("BuildID[") + (descSize == 20 ? "sha1" : "md5/uuid") + "]=" + hex;
                } else if (gnu && noteType == 1 && descSize >= 16 && ReadNum(d, descStart, 4, big) == 0) {
                    abiTag = "for GNU/Linux " +
                             std::to_string(ReadNum(d, descStart + 4, 4, big)) + "." +
                             std::to_string(ReadNum(d, descStart + 8, 4, big)) + "." +
                             std::to_string(ReadNum(d, descStart + 12, 4, big));
                }

                n = descStart + ((descSize + 3) & ~3LL);
            }
        }
    }

    bool symtab = false;
    bool sectionsOk = true;
    if (shoff > 0 && shentsize > 0 && shnum > 0 && shnum < 4096) {
        auto sections = ReadRange(path, shoff, shentsize * shnum);
        if (sections.empty()) {
            sectionsOk = false;
        } else {
            for (int i = 0; i < shnum; i++) {
                if (ReadNum(sections, static_cast<int64_t>(i) * shentsize + 4, 4, big) == 2) {
                    symtab = true;
                    break;
                }
            }
        }
    }

    std::string kindName;
    std::string mime;
    switch (type) {
        case 1: kindName = "relocatable"; mime = "application/x-object"; break;
        case 2: kindName = "executable"; mime = "application/x-executable"; break;
        case 3:
            kindName = hasInterp ? "pie executable" : "shared object";
            mime = hasInterp ? "application/x-pie-executable" : "application/x-sharedlib";
            break;
        case 4: kindName = "core file"; mime = "application/x-coredump"; break;
        default: kindName = "*unknown type " + std::to_string(type) + "*"; mime = "application/octet-stream"; break;
    }

    std::vector<std::string> parts;
    parts.push_back("ELF " + std::string(is64 ? "64" : "32") + "-bit " + (big ? "MSB" : "LSB") + " " + kindName);
    parts.push_back(machineName);
    parts.push_back("version " + std::to_string(version) + " (" + abiName + ")");

    if (type == 2 || type == 3) {
        parts.push_back((hasInterp || hasDynamic) ? "dynamically linked" : "statically linked");
        if (!interpreter.empty())
            parts.push_back("interpreter " + interpreter);
    }

    if (!buildId.empty()) parts.push_back(buildId);
    if (!abiTag.empty()) parts.push_back(abiTag);
    if (sectionsOk) parts.push_back(symtab ? "not stripped" : "stripped");

    return BinVec(kind, mime, std::move(parts));
}

bool DescribePe(const std::vector<uint8_t>& d, FileKind& kind) {
    int64_t pe = static_cast<int64_t>(ReadNum(d, 0x3C, 4, false));

    if (pe < 64 || pe + 24 > static_cast<int64_t>(d.size()) ||
        !HasAscii(d, static_cast<size_t>(pe), "PE") ||
        d[static_cast<size_t>(pe + 2)] != 0 || d[static_cast<size_t>(pe + 3)] != 0) {
        return Bin(kind, "application/x-dosexec", {"MS-DOS executable"});
    }

    int machine = static_cast<int>(ReadNum(d, pe + 4, 2, false));
    int sections = static_cast<int>(ReadNum(d, pe + 6, 2, false));
    int characteristics = static_cast<int>(ReadNum(d, pe + 22, 2, false));

    int64_t opt = pe + 24;
    bool plus = ReadNum(d, opt, 2, false) == 0x20B;
    int subsystem = static_cast<int>(ReadNum(d, opt + 68, 2, false));

    int64_t dirCount = static_cast<int64_t>(ReadNum(d, opt + (plus ? 108 : 92), 4, false));
    int64_t clrRva = static_cast<int64_t>(ReadNum(d, opt + (plus ? 112 : 96) + 14 * 8, 4, false));
    bool dotnet = dirCount > 14 && clrRva != 0;

    std::string machineName;
    switch (machine) {
        case 0x14C: machineName = "Intel 80386"; break;
        case 0x1C0: machineName = "ARM"; break;
        case 0x1C4: machineName = "ARMv7 Thumb"; break;
        case 0x200: machineName = "Intel Itanium"; break;
        case 0x8664: machineName = "x86-64"; break;
        case 0xAA64: machineName = "Aarch64"; break;
        default: {
            std::ostringstream oss;
            oss << "unknown machine 0x" << std::hex << machine;
            machineName = oss.str();
            break;
        }
    }

    std::string subsystemName;
    switch (subsystem) {
        case 1: subsystemName = " (native)"; break;
        case 2: subsystemName = " (GUI)"; break;
        case 3: subsystemName = " (console)"; break;
        case 7: subsystemName = " (POSIX)"; break;
        case 9: subsystemName = " (Windows CE)"; break;
        case 10: subsystemName = " (EFI application)"; break;
        case 11: subsystemName = " (EFI boot service driver)"; break;
        case 12: subsystemName = " (EFI runtime driver)"; break;
        case 13: subsystemName = " (EFI ROM)"; break;
        case 14: subsystemName = " (Xbox)"; break;
        case 16: subsystemName = " (Windows boot application)"; break;
    }

    std::string dllName = (characteristics & 0x2000) != 0 ? " (DLL)" : "";
    std::string netName = dotnet ? " Mono/.Net assembly" : "";
    std::string peName = plus ? "PE32+" : "PE32";

    std::vector<std::string> parts;
    parts.push_back(peName + " executable" + dllName + subsystemName + " " + machineName + netName);
    parts.push_back("for MS Windows");
    if (sections > 0)
        parts.push_back(std::to_string(sections) + " sections");

    return BinVec(kind, "application/vnd.microsoft.portable-executable", std::move(parts));
}

bool DescribePng(const std::vector<uint8_t>& d, FileKind& kind) {
    if (d.size() < 29)
        return Bin(kind, "image/png", {"PNG image data"});

    int width = static_cast<int>(ReadNum(d, 16, 4, true));
    int height = static_cast<int>(ReadNum(d, 20, 4, true));
    int depth = d[24];
    int colorType = d[25];
    int interlace = d[28];

    std::string color;
    switch (colorType) {
        case 0: color = std::to_string(depth) + "-bit grayscale"; break;
        case 2: color = std::to_string(depth) + "-bit/color RGB"; break;
        case 3: color = std::to_string(depth) + "-bit colormap"; break;
        case 4: color = std::to_string(depth) + "-bit gray+alpha"; break;
        case 6: color = std::to_string(depth) + "-bit/color RGBA"; break;
        default: color = std::to_string(depth) + "-bit"; break;
    }

    return Bin(kind, "image/png", {
        "PNG image data",
        std::to_string(width) + " x " + std::to_string(height),
        color,
        interlace == 1 ? "interlaced" : "non-interlaced"
    });
}

bool DescribeJpeg(const std::vector<uint8_t>& d, FileKind& kind) {
    std::vector<std::string> parts = {"JPEG image data"};
    size_t i = 2;
    bool gotJfif = false;
    bool gotExif = false;

    while (i + 4 <= d.size()) {
        if (d[i] != 0xFF) break;
        int marker = d[i + 1];
        if (marker == 0xFF) { i++; continue; }
        if (marker == 0xD8 || marker == 0x01 || (marker >= 0xD0 && marker <= 0xD7)) {
            i += 2;
            continue;
        }

        int length = (d[i + 2] << 8) | d[i + 3];
        if (marker == 0xE0 && !gotJfif && HasAscii(d, i + 4, "JFIF") && i + 16 <= d.size()) {
            gotJfif = true;
            int units = d[i + 11];
            int xDensity = (d[i + 12] << 8) | d[i + 13];
            int yDensity = (d[i + 14] << 8) | d[i + 15];

            char verBuf[32];
            std::snprintf(verBuf, sizeof(verBuf), "JFIF standard %d.%02d", d[i + 9], d[i + 10]);
            parts.emplace_back(verBuf);
            parts.emplace_back(units == 1 ? "resolution (DPI)" : units == 2 ? "resolution (DPCM)" : "aspect ratio");
            parts.emplace_back("density " + std::to_string(xDensity) + "x" + std::to_string(yDensity));
            parts.emplace_back("segment length " + std::to_string(length));
        } else if (marker == 0xE1 && !gotExif && !gotJfif && HasAscii(d, i + 4, "Exif") && i + 18 <= d.size()) {
            gotExif = true;
            size_t tiff = i + 10;
            bool tiffBig = d[tiff] == 0x4D;
            int64_t ifd = static_cast<int64_t>(ReadNum(d, tiff + 4, 4, tiffBig));
            int entries = (ifd >= 8 && tiff + ifd + 2 <= d.size())
                ? static_cast<int>(ReadNum(d, tiff + ifd, 2, tiffBig)) : 0;

            parts.emplace_back(std::string("Exif standard: [TIFF image data, ") +
                               (tiffBig ? "big-endian" : "little-endian") +
                               ", direntries=" + std::to_string(entries) + "]");
        } else if (marker >= 0xC0 && marker <= 0xCF && marker != 0xC4 && marker != 0xC8 && marker != 0xCC && i + 10 <= d.size()) {
            std::string coding;
            switch (marker) {
                case 0xC0: coding = "baseline"; break;
                case 0xC1: coding = "extended sequential"; break;
                case 0xC2: coding = "progressive"; break;
                default: coding = "lossless"; break;
            }
            int height = (d[i + 5] << 8) | d[i + 6];
            int width = (d[i + 7] << 8) | d[i + 8];

            parts.push_back(coding);
            parts.push_back("precision " + std::to_string(d[i + 4]));
            parts.push_back(std::to_string(width) + "x" + std::to_string(height));
            parts.push_back("components " + std::to_string(d[i + 9]));
            break;
        } else if (marker == 0xDA) {
            break;
        }

        i += 2 + static_cast<size_t>(length);
    }

    return BinVec(kind, "image/jpeg", std::move(parts));
}

bool DescribeZip(const std::vector<uint8_t>& d, FileKind& kind) {
    if (StartsWithBytes(d, {0x50, 0x4B, 0x05, 0x06}))
        return Bin(kind, "application/zip", {"Zip archive", "with no entries"});

    int need = static_cast<int>(ReadNum(d, 4, 2, false));
    int method = static_cast<int>(ReadNum(d, 8, 2, false));
    size_t nameLength = static_cast<size_t>(ReadNum(d, 26, 2, false));
    size_t extraLength = static_cast<size_t>(ReadNum(d, 28, 2, false));

    std::string name;
    if (30 + nameLength <= d.size()) {
        name.assign(reinterpret_cast<const char*>(d.data() + 30), nameLength);
    }

    if (name == "mimetype") {
        size_t start = 30 + nameLength + extraLength;
        std::string mimetype;
        if (start < d.size()) {
            mimetype.assign(reinterpret_cast<const char*>(d.data() + start), std::min<size_t>(80, d.size() - start));
        }
        size_t pk = mimetype.find("PK");
        if (pk != std::string::npos) mimetype = mimetype.substr(0, pk);

        if (mimetype == "application/epub+zip")
            return Bin(kind, mimetype, {"EPUB document"});
        if (mimetype == "application/vnd.oasis.opendocument.text")
            return Bin(kind, mimetype, {"OpenDocument Text"});
        if (mimetype == "application/vnd.oasis.opendocument.spreadsheet")
            return Bin(kind, mimetype, {"OpenDocument Spreadsheet"});
        if (mimetype == "application/vnd.oasis.opendocument.presentation")
            return Bin(kind, mimetype, {"OpenDocument Presentation"});
    }

    if (name.starts_with("META-INF/"))
        return Bin(kind, "application/java-archive", {"Java archive data (JAR)"});

    if (name == "[Content_Types].xml" || name == "_rels/.rels" || name.starts_with("docProps/") ||
        name.starts_with("word/") || name.starts_with("xl/") || name.starts_with("ppt/")) {
        std::string head(reinterpret_cast<const char*>(d.data()), std::min<size_t>(d.size(), 8192));
        if (head.find("word/") != std::string::npos)
            return Bin(kind, "application/vnd.openxmlformats-officedocument.wordprocessingml.document", {"Microsoft Word 2007+"});
        if (head.find("xl/") != std::string::npos)
            return Bin(kind, "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet", {"Microsoft Excel 2007+"});
        if (head.find("ppt/") != std::string::npos)
            return Bin(kind, "application/vnd.openxmlformats-officedocument.presentationml.presentation", {"Microsoft PowerPoint 2007+"});
        return Bin(kind, "application/octet-stream", {"Microsoft OOXML"});
    }

    std::string methodName;
    switch (method) {
        case 0: methodName = "store"; break;
        case 1: methodName = "shrink"; break;
        case 6: methodName = "implode"; break;
        case 8: methodName = "deflate"; break;
        case 9: methodName = "deflate64"; break;
        case 12: methodName = "bzip2"; break;
        case 14: methodName = "LZMA"; break;
        case 93: methodName = "Zstandard"; break;
        case 95: methodName = "XZ"; break;
        case 98: methodName = "PPMd"; break;
        default: methodName = "unknown " + std::to_string(method); break;
    }

    return Bin(kind, "application/zip", {
        "Zip archive data",
        "at least v" + std::to_string(need / 10) + "." + std::to_string(need % 10) + " to extract",
        "compression method=" + methodName
    });
}

bool DescribeGzip(const std::vector<uint8_t>& d, const std::string& path, FileKind& kind) {
    std::vector<std::string> parts = {"gzip compressed data"};
    int flags = d.size() > 3 ? d[3] : 0;
    int64_t mtime = static_cast<int64_t>(ReadNum(d, 4, 4, false));
    int xfl = d.size() > 8 ? d[8] : 0;
    int os = d.size() > 9 ? d[9] : 255;

    size_t pos = 10;
    if ((flags & 0x04) != 0) {
        pos += 2 + static_cast<size_t>(ReadNum(d, pos, 2, false));
    }

    if ((flags & 0x08) != 0 && pos < d.size()) {
        size_t end = pos;
        while (end < d.size() && d[end] != 0) end++;
        if (end < d.size()) {
            std::string fname(reinterpret_cast<const char*>(d.data() + pos), end - pos);
            parts.push_back("was \"" + fname + "\"");
        }
    }

    if (mtime != 0) {
        std::time_t tt = static_cast<std::time_t>(mtime);
        std::tm tmBuf{};
#ifdef _WIN32
        localtime_s(&tmBuf, &tt);
#else
        localtime_r(&tt, &tmBuf);
#endif
        char buf[64];
        std::strftime(buf, sizeof(buf), "last modified: %a %b %e %H:%M:%S %Y", &tmBuf);
        parts.emplace_back(buf);
    }

    if (xfl == 2) parts.emplace_back("max compression");
    else if (xfl == 4) parts.emplace_back("max speed");

    switch (os) {
        case 0: parts.emplace_back("from FAT filesystem (MS-DOS, OS/2, NT)"); break;
        case 3: parts.emplace_back("from Unix"); break;
        case 7: parts.emplace_back("from Macintosh"); break;
        case 11: parts.emplace_back("from NTFS filesystem (NT)"); break;
    }

    std::ifstream stream(path, std::ios::binary);
    if (stream) {
        stream.seekg(0, std::ios::end);
        int64_t len = stream.tellg();
        if (len >= 4) {
            stream.seekg(-4, std::ios::end);
            std::vector<uint8_t> tail(4);
            stream.read(reinterpret_cast<char*>(tail.data()), 4);
            if (stream.gcount() == 4) {
                parts.push_back("original size modulo 2^32 " + std::to_string(ReadNum(tail, 0, 4, false)));
            }
        }
    }

    return BinVec(kind, "application/gzip", std::move(parts));
}

bool DescribeSqlite(const std::vector<uint8_t>& d, FileKind& kind) {
    std::vector<std::string> parts = {"SQLite 3.x database"};
    int64_t applicationId = static_cast<int64_t>(ReadNum(d, 68, 4, true));
    int64_t userVersion = static_cast<int64_t>(ReadNum(d, 60, 4, true));

    if (applicationId != 0) parts.push_back("application id " + std::to_string(applicationId));
    if (userVersion != 0) parts.push_back("user version " + std::to_string(userVersion));

    std::string encoding;
    switch (ReadNum(d, 56, 4, true)) {
        case 1: encoding = "UTF-8"; break;
        case 2: encoding = "UTF-16 little-endian"; break;
        case 3: encoding = "UTF-16 big-endian"; break;
        default: encoding = "unknown encoding"; break;
    }

    std::ostringstream cookieOss;
    cookieOss << "cookie 0x" << std::hex << ReadNum(d, 48, 4, true);

    parts.push_back("last written using SQLite version " + std::to_string(ReadNum(d, 96, 4, true)));
    parts.push_back("file counter " + std::to_string(ReadNum(d, 24, 4, true)));
    parts.push_back("database pages " + std::to_string(ReadNum(d, 28, 4, true)));
    parts.push_back(cookieOss.str());
    parts.push_back("schema " + std::to_string(ReadNum(d, 44, 4, true)));
    parts.push_back(encoding);
    parts.push_back("version-valid-for " + std::to_string(ReadNum(d, 92, 4, true)));

    return BinVec(kind, "application/vnd.sqlite3", std::move(parts));
}

bool DescribeOgg(const std::vector<uint8_t>& d, FileKind& kind) {
    std::vector<std::string> parts = {"Ogg data"};
    if (d.size() >= 58 && d[28] == 1 && HasAscii(d, 29, "vorbis")) {
        int channels = d[39];
        int64_t rate = static_cast<int64_t>(ReadNum(d, 40, 4, false));
        int nominal = static_cast<int>(ReadNum(d, 48, 4, false));
        std::string layout = channels == 1 ? "mono" : channels == 2 ? "stereo" : std::to_string(channels) + " channels";
        parts.emplace_back("Vorbis audio");
        parts.push_back(layout);
        parts.push_back(std::to_string(rate) + " Hz");
        if (nominal > 0) parts.push_back("~" + std::to_string(nominal) + " bps");
    } else if (HasAscii(d, 28, "OpusHead")) {
        parts.emplace_back("Opus audio");
    }
    return BinVec(kind, "audio/ogg", std::move(parts));
}

bool DescribeFlac(const std::vector<uint8_t>& d, FileKind& kind) {
    std::vector<std::string> parts = {"FLAC audio bitstream data"};
    if (d.size() >= 26) {
        uint64_t sampleRate = (static_cast<uint64_t>(d[18]) << 12) | (static_cast<uint64_t>(d[19]) << 4) | (d[20] >> 4);
        int channels = ((d[20] >> 1) & 7) + 1;
        int bits = (((d[20] & 1) << 4) | (d[21] >> 4)) + 1;
        uint64_t samples = (static_cast<uint64_t>(d[21] & 0xF) << 32) | ReadNum(d, 22, 4, true);

        std::string layout = channels == 1 ? "mono" : channels == 2 ? "stereo" : std::to_string(channels) + " channels";
        std::ostringstream khz;
        khz << (sampleRate / 1000.0) << " kHz";

        parts.push_back(std::to_string(bits) + " bit");
        parts.push_back(layout);
        parts.push_back(khz.str());
        parts.push_back(std::to_string(samples) + " samples");
    }
    return BinVec(kind, "audio/flac", std::move(parts));
}

bool DescribeRiff(const std::vector<uint8_t>& d, FileKind& kind) {
    std::string format(reinterpret_cast<const char*>(d.data() + 8), 4);
    if (format == "WAVE") {
        std::vector<std::string> parts = {"RIFF (little-endian) data", "WAVE audio"};
        if (d.size() >= 36 && HasAscii(d, 12, "fmt ")) {
            int tag = static_cast<int>(ReadNum(d, 20, 2, false));
            int channels = static_cast<int>(ReadNum(d, 22, 2, false));
            int64_t rate = static_cast<int64_t>(ReadNum(d, 24, 4, false));
            int bits = static_cast<int>(ReadNum(d, 34, 2, false));

            std::string tagName = tag == 1 ? "Microsoft PCM" : tag == 3 ? "IEEE Float" : tag == 0x55 ? "MPEG Layer 3" : "format " + std::to_string(tag);
            std::string layout = channels == 1 ? "mono" : channels == 2 ? "stereo" : std::to_string(channels) + " channels";
            parts.push_back(tagName);
            if (bits > 0) parts.push_back(std::to_string(bits) + " bit");
            parts.push_back(layout + " " + std::to_string(rate) + " Hz");
        }
        return BinVec(kind, "audio/x-wav", std::move(parts));
    }
    if (format == "WEBP")
        return Bin(kind, "image/webp", {"RIFF (little-endian) data", "Web/P image"});
    if (format == "AVI ")
        return Bin(kind, "video/x-msvideo", {"RIFF (little-endian) data", "AVI"});
    return Bin(kind, "application/octet-stream", {"RIFF (little-endian) data"});
}

bool DetectBinaryType(const std::vector<uint8_t>& data, const std::string& path, FileKind& kind) {
    if (StartsWithBytes(data, {0x7F, 0x45, 0x4C, 0x46}))
        return DescribeElf(data, path, kind);

    if (StartsWithBytes(data, {0x4D, 0x5A}) && data.size() >= 64)
        return DescribePe(data, kind);

    if (StartsWithBytes(data, {0x00, 0x61, 0x73, 0x6D}) && data.size() >= 8) {
        std::ostringstream oss;
        oss << "WebAssembly (wasm) binary module version 0x" << std::hex << ReadNum(data, 4, 4, false) << " (MVP)";
        return Bin(kind, "application/wasm", {oss.str()});
    }

    if (StartsWithBytes(data, {0xCA, 0xFE, 0xBA, 0xBE}) && data.size() >= 8) {
        int javaMinor = static_cast<int>(ReadNum(data, 4, 2, true));
        int javaMajor = static_cast<int>(ReadNum(data, 6, 2, true));
        if (javaMajor >= 45 && javaMajor < 100) {
            return Bin(kind, "application/x-java-applet", {
                "compiled Java class data",
                "version " + std::to_string(javaMajor) + "." + std::to_string(javaMinor) + " (Java 1." + std::to_string(javaMajor - 44) + ")"
            });
        }
    }

    if (StartsWithBytes(data, {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A}))
        return DescribePng(data, kind);

    if (StartsWithBytes(data, {0xFF, 0xD8, 0xFF}))
        return DescribeJpeg(data, kind);

    if (StartsWithBytes(data, {0x47, 0x49, 0x46, 0x38}) && data.size() >= 10 &&
        (data[4] == '7' || data[4] == '9') && data[5] == 'a') {
        std::string gifVersion(reinterpret_cast<const char*>(data.data() + 4), 2);
        int gifWidth = static_cast<int>(ReadNum(data, 6, 2, false));
        int gifHeight = static_cast<int>(ReadNum(data, 8, 2, false));
        return Bin(kind, "image/gif", {"GIF image data", "version " + gifVersion, std::to_string(gifWidth) + " x " + std::to_string(gifHeight)});
    }

    if (StartsWithBytes(data, {0x42, 0x4D}) && data.size() >= 54 && ReadNum(data, 14, 4, false) == 40) {
        int bmpWidth = static_cast<int>(ReadNum(data, 18, 4, false));
        int bmpHeight = static_cast<int>(ReadNum(data, 22, 4, false));
        int bmpBits = static_cast<int>(ReadNum(data, 28, 2, false));
        return Bin(kind, "image/bmp", {
            "PC bitmap",
            "Windows 3.x format",
            std::to_string(bmpWidth) + " x " + std::to_string(bmpHeight) + " x " + std::to_string(bmpBits),
            "image size " + std::to_string(ReadNum(data, 34, 4, false)),
            "resolution " + std::to_string(ReadNum(data, 38, 4, false)) + " x " + std::to_string(ReadNum(data, 42, 4, false)) + " px/m",
            "cbSize " + std::to_string(ReadNum(data, 2, 4, false)),
            "bits offset " + std::to_string(ReadNum(data, 10, 4, false))
        });
    }

    if ((StartsWithBytes(data, {0x49, 0x49, 0x2A, 0x00}) || StartsWithBytes(data, {0x4D, 0x4D, 0x00, 0x2A})) && data.size() >= 8) {
        bool tiffBig = data[0] == 0x4D;
        int64_t ifd = static_cast<int64_t>(ReadNum(data, 4, 4, tiffBig));
        int entries = (ifd >= 8 && ifd + 2 <= static_cast<int64_t>(data.size()))
            ? static_cast<int>(ReadNum(data, ifd, 2, tiffBig)) : 0;
        return Bin(kind, "image/tiff", {"TIFF image data", tiffBig ? "big-endian" : "little-endian", "direntries=" + std::to_string(entries)});
    }

    if (StartsWithBytes(data, {0x25, 0x50, 0x44, 0x46, 0x2D})) {
        std::string pdfVersion(reinterpret_cast<const char*>(data.data() + 5), std::min<size_t>(3, data.size() - 5));
        return Bin(kind, "application/pdf", {"PDF document", "version " + pdfVersion});
    }

    if (StartsWithBytes(data, {0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1}))
        return Bin(kind, "application/CDFV2", {"Composite Document File V2 Document"});

    if (StartsWithBytes(data, {0x50, 0x4B, 0x03, 0x04}) || (StartsWithBytes(data, {0x50, 0x4B, 0x05, 0x06}) && data.size() == 22))
        return DescribeZip(data, kind);

    if (StartsWithBytes(data, {0x52, 0x61, 0x72, 0x21, 0x1A, 0x07})) {
        std::string rarVersion = (data.size() > 7 && data[6] == 1 && data[7] == 0) ? "v5" : "v4";
        return Bin(kind, "application/vnd.rar", {"RAR archive data", rarVersion});
    }

    if (StartsWithBytes(data, {0x37, 0x7A, 0xBC, 0xAF, 0x27, 0x1C}) && data.size() >= 8)
        return Bin(kind, "application/x-7z-compressed", {"7-zip archive data", "version " + std::to_string(data[6]) + "." + std::to_string(data[7])});

    if (StartsWithBytes(data, {0x1F, 0x8B}))
        return DescribeGzip(data, path, kind);

    if (StartsWithBytes(data, {0x42, 0x5A, 0x68}) && data.size() > 3 && data[3] >= '1' && data[3] <= '9') {
        std::string blk = std::string(1, static_cast<char>(data[3])) + "00k";
        return Bin(kind, "application/x-bzip2", {"bzip2 compressed data", "block size = " + blk});
    }

    if (StartsWithBytes(data, {0xFD, 0x37, 0x7A, 0x58, 0x5A, 0x00}) && data.size() >= 8) {
        std::string xzCheck;
        switch (data[7]) {
            case 0x00: xzCheck = "None"; break;
            case 0x01: xzCheck = "CRC32"; break;
            case 0x04: xzCheck = "CRC64"; break;
            case 0x0A: xzCheck = "SHA-256"; break;
            default: xzCheck = "unknown"; break;
        }
        return Bin(kind, "application/x-xz", {"XZ compressed data", "checksum " + xzCheck});
    }

    if (StartsWithBytes(data, {0x28, 0xB5, 0x2F, 0xFD}))
        return Bin(kind, "application/zstd", {"Zstandard compressed data (v0.8+)", "Dictionary ID: None"});

    if (data.size() >= 265 && HasAscii(data, 257, "ustar")) {
        std::string tarKind = data[262] == 0x20 ? "POSIX tar archive (GNU)" : "POSIX tar archive";
        return Bin(kind, "application/x-tar", {tarKind});
    }

    if (StartsWithBytes(data, {0x53, 0x51, 0x4C, 0x69, 0x74, 0x65, 0x20, 0x66, 0x6F, 0x72, 0x6D, 0x61, 0x74, 0x20, 0x33, 0x00}) && data.size() >= 100)
        return DescribeSqlite(data, kind);

    if (StartsWithBytes(data, {0x49, 0x44, 0x33}) && data.size() >= 10 && data[3] < 0xFF && data[4] < 0xFF)
        return Bin(kind, "audio/mpeg", {"Audio file with ID3 version 2." + std::to_string(data[3]) + "." + std::to_string(data[4])});

    if (StartsWithBytes(data, {0x4F, 0x67, 0x67, 0x53}))
        return DescribeOgg(data, kind);

    if (StartsWithBytes(data, {0x66, 0x4C, 0x61, 0x43}))
        return DescribeFlac(data, kind);

    if (StartsWithBytes(data, {0x52, 0x49, 0x46, 0x46}) && data.size() >= 12)
        return DescribeRiff(data, kind);

    if (data.size() >= 12 && HasAscii(data, 4, "ftyp")) {
        std::string brand(reinterpret_cast<const char*>(data.data() + 8), 4);
        if (brand == "mp41") return Bin(kind, "video/mp4", {"ISO Media", "MP4 v1 [ISO 14496-1:ch13]"});
        if (brand == "mp42") return Bin(kind, "video/mp4", {"ISO Media", "MP4 v2 [ISO 14496-14]"});
        if (brand == "isom") return Bin(kind, "video/mp4", {"ISO Media", "MP4 Base Media v1 [ISO 14496-12:2003]"});
        if (brand == "M4A ") return Bin(kind, "audio/x-m4a", {"ISO Media", "Apple iTunes ALAC/AAC-LC (.M4A) Audio"});
        if (brand == "M4V ") return Bin(kind, "video/x-m4v", {"ISO Media", "Apple iTunes Video (.M4V) Video"});
        if (brand == "qt  ") return Bin(kind, "video/quicktime", {"ISO Media", "Apple QuickTime movie"});
        if (brand == "avif") return Bin(kind, "image/avif", {"ISO Media", "AVIF Image"});
        return Bin(kind, "application/octet-stream", {"ISO Media"});
    }

    return false;
}

bool IsValidUtf8(const uint8_t* data, size_t len, bool complete) {
    size_t i = 0;
    while (i < len) {
        uint8_t c = data[i];
        size_t seqLen = 0;
        if (c <= 0x7F) seqLen = 1;
        else if ((c & 0xE0) == 0xC0) { if (c < 0xC2) return false; seqLen = 2; }
        else if ((c & 0xF0) == 0xE0) seqLen = 3;
        else if ((c & 0xF8) == 0xF0) { if (c > 0xF4) return false; seqLen = 4; }
        else return false;

        if (i + seqLen > len) {
            return !complete; // partial sequence at buffer boundary is acceptable
        }
        for (size_t j = 1; j < seqLen; ++j) {
            if ((data[i + j] & 0xC0) != 0x80) return false;
        }
        i += seqLen;
    }
    return true;
}

struct TextLanguage {
    std::string language;
    std::string mime = "text/plain";
    bool executable = false;
};

TextLanguage DetectTextLanguage(const std::string& content) {
    size_t trimStart = 0;
    while (trimStart < content.size() && std::isspace(static_cast<unsigned char>(content[trimStart]))) trimStart++;
    std::string trimmed = content.substr(trimStart);

    // shebang scripts
    if (content.starts_with("#!")) {
        size_t eol = content.find_first_of("\r\n");
        std::string firstLine = Trim(eol == std::string::npos ? content.substr(2) : content.substr(2, eol - 2));
        std::istringstream iss(firstLine);
        std::vector<std::string> tokens;
        std::string tok;
        while (iss >> tok) tokens.push_back(tok);

        if (!tokens.empty()) {
            std::string interpreter = fs::path(tokens[0]).filename().string();
            if (interpreter == "env") {
                interpreter.clear();
                for (size_t i = 1; i < tokens.size(); i++) {
                    if (tokens[i].starts_with("-") || tokens[i].find('=') != std::string::npos) continue;
                    interpreter = fs::path(tokens[i]).filename().string();
                    break;
                }
            }

            if (interpreter == "bash") return {"Bourne-Again shell script", "text/x-shellscript", true};
            if (interpreter == "sh" || interpreter == "dash" || interpreter == "ash") return {"POSIX shell script", "text/x-shellscript", true};
            if (interpreter == "zsh") return {"Paul Falstad's zsh script", "text/x-shellscript", true};
            if (interpreter == "ksh") return {"Korn shell script", "text/x-shellscript", true};
            if (interpreter.starts_with("python")) return {"Python script", "text/x-script.python", true};
            if (interpreter.starts_with("perl")) return {"Perl script", "text/x-perl", true};
            if (interpreter.starts_with("ruby")) return {"Ruby script", "text/x-ruby", true};
            if (interpreter.starts_with("php")) return {"PHP script", "text/x-php", true};
            if (interpreter == "node" || interpreter == "nodejs") return {"Node.js script", "text/javascript", true};
            if (interpreter.starts_with("lua")) return {"Lua script", "text/x-lua", true};
        }

        return {"a " + firstLine + " script", "text/plain", true};
    }

    if (trimmed.starts_with("<?xml")) {
        std::string head = trimmed.substr(0, 4096);
        if (head.find("<svg") != std::string::npos)
            return {"SVG Scalable Vector Graphics image", "image/svg+xml", false};

        std::regex verRx(R"(version\s*=\s*["']([0-9.]+)["'])");
        std::smatch m;
        std::string xmlVersion = std::regex_search(head, m, verRx) ? m[1].str() : "1.0";
        return {"XML " + xmlVersion + " document", "text/xml", false};
    }

    if (trimmed.starts_with("<svg"))
        return {"SVG Scalable Vector Graphics image", "image/svg+xml", false};

    if (StartsWithIgnoreCase(trimmed, "<!DOCTYPE html") || StartsWithIgnoreCase(trimmed, "<html"))
        return {"HTML document", "text/html", false};

    if (trimmed.starts_with("<?php"))
        return {"PHP script", "text/x-php", false};

    std::regex cLikeRx(R"(^[ \t]*#[ \t]*(include|ifdef|ifndef)\b|^[ \t]*#[ \t]*define[ \t]+\w+)");
    std::regex cppLikeRx(R"(^[ \t]*(namespace[ \t]+\w|template[ \t]*<|using[ \t]+namespace[ \t]|class[ \t]+\w+[ \t]*(\{|:[ \t]*(public|private|protected)\b)))");

    if (std::regex_search(content, cppLikeRx))
        return {"C++ source", "text/x-c++", false};

    if (std::regex_search(content, cLikeRx))
        return {"C source", "text/x-c", false};

    std::regex pyRx(R"(^(from[ \t]+[\w.]+[ \t]+import[ \t]+\S|import[ \t]+[\w.]+|def[ \t]+\w+\(.*\)[ \t]*(->.*)?:[ \t]*\r?$))");
    if (std::regex_search(content, pyRx))
        return {"Python script", "text/x-script.python", true};

    return {"", "text/plain", false};
}

bool AnalyzeText(const std::vector<uint8_t>& data, int64_t fileLength, FileKind& kind) {
    bool complete = static_cast<int64_t>(data.size()) >= fileLength;
    std::string content;
    std::string charset = "us-ascii";
    std::vector<std::string> charsetParts;

    if (StartsWithBytes(data, {0xFF, 0xFE, 0x00, 0x00})) {
        charsetParts.emplace_back("Unicode text");
        charsetParts.emplace_back("UTF-32, little-endian text");
        charset = "utf-32le";
        for (size_t i = 4; i + 3 < data.size(); i += 4) {
            uint32_t cp = data[i] | (data[i + 1] << 8) | (data[i + 2] << 16) | (data[i + 3] << 24);
            content.push_back(cp < 128 ? static_cast<char>(cp) : '?');
        }
    } else if (StartsWithBytes(data, {0x00, 0x00, 0xFE, 0xFF})) {
        charsetParts.emplace_back("Unicode text");
        charsetParts.emplace_back("UTF-32, big-endian text");
        charset = "utf-32be";
        for (size_t i = 4; i + 3 < data.size(); i += 4) {
            uint32_t cp = (data[i] << 24) | (data[i + 1] << 16) | (data[i + 2] << 8) | data[i + 3];
            content.push_back(cp < 128 ? static_cast<char>(cp) : '?');
        }
    } else if (StartsWithBytes(data, {0xEF, 0xBB, 0xBF})) {
        if (!IsValidUtf8(data.data() + 3, data.size() - 3, complete)) return false;
        content.assign(reinterpret_cast<const char*>(data.data() + 3), data.size() - 3);
        charsetParts.emplace_back("Unicode text");
        charsetParts.emplace_back("UTF-8 (with BOM) text");
        charset = "utf-8";
    } else if (StartsWithBytes(data, {0xFF, 0xFE})) {
        charsetParts.emplace_back("Unicode text");
        charsetParts.emplace_back("UTF-16, little-endian text");
        charset = "utf-16le";
        for (size_t i = 2; i + 1 < data.size(); i += 2) {
            uint16_t ch = data[i] | (data[i + 1] << 8);
            content.push_back(ch < 128 ? static_cast<char>(ch) : '?');
        }
    } else if (StartsWithBytes(data, {0xFE, 0xFF})) {
        charsetParts.emplace_back("Unicode text");
        charsetParts.emplace_back("UTF-16, big-endian text");
        charset = "utf-16be";
        for (size_t i = 2; i + 1 < data.size(); i += 2) {
            uint16_t ch = (data[i] << 8) | data[i + 1];
            content.push_back(ch < 128 ? static_cast<char>(ch) : '?');
        }
    } else {
        if (IsValidUtf8(data.data(), data.size(), complete)) {
            content.assign(reinterpret_cast<const char*>(data.data()), data.size());
            bool hasHigh = std::any_of(data.begin(), data.end(), [](uint8_t b) { return b >= 0x80; });
            if (hasHigh) {
                charsetParts.emplace_back("Unicode text");
                charsetParts.emplace_back("UTF-8 text");
                charset = "utf-8";
            } else {
                charsetParts.emplace_back("ASCII text");
                charset = "us-ascii";
            }
        } else {
            content.assign(reinterpret_cast<const char*>(data.data()), data.size());
            bool hasC1 = std::any_of(data.begin(), data.end(), [](uint8_t b) { return b >= 0x80 && b <= 0x9F; });
            if (hasC1) {
                charsetParts.emplace_back("Non-ISO extended-ASCII text");
                charset = "unknown-8bit";
            } else {
                charsetParts.emplace_back("ISO-8859 text");
                charset = "iso-8859-1";
            }
        }
    }

    for (unsigned char c : content) {
        if (c < 32 && c != 7 && c != 8 && c != 9 && c != 10 && c != 11 && c != 12 && c != 13 && c != 27)
            return false;
        if (c == 127)
            return false;
    }

    std::string trimmed = Trim(content);
    if (complete && trimmed.size() > 1 &&
        ((trimmed.front() == '{' && trimmed.back() == '}') ||
         (trimmed.front() == '[' && trimmed.back() == ']'))) {
        kind.parts.emplace_back("JSON text data");
        kind.mime = "application/json";
        kind.charset = charset;
        return true;
    }

    int crlf = 0, cr = 0, lf = 0, lineLength = 0, longest = 0;
    bool hasEscapes = false, hasBackspace = false;

    for (size_t i = 0; i < content.size(); i++) {
        char c = content[i];
        if (c == '\r') {
            if (i + 1 < content.size() && content[i + 1] == '\n') {
                crlf++;
                i++;
            } else {
                cr++;
            }
            longest = std::max(longest, lineLength);
            lineLength = 0;
            continue;
        }
        if (c == '\n') {
            lf++;
            longest = std::max(longest, lineLength);
            lineLength = 0;
            continue;
        }
        if (c == 27) hasEscapes = true;
        if (c == 8) hasBackspace = true;
        lineLength++;
    }
    longest = std::max(longest, lineLength);

    auto language = DetectTextLanguage(content);
    if (!language.language.empty())
        kind.parts.push_back(language.language);

    if (language.executable && !charsetParts.empty())
        charsetParts.back() += " executable";

    kind.parts.insert(kind.parts.end(), charsetParts.begin(), charsetParts.end());

    if (longest > 300)
        kind.parts.push_back("with very long lines (" + std::to_string(longest) + ")");

    if (!(crlf == 0 && cr == 0 && lf > 0)) {
        if (crlf == 0 && cr == 0 && lf == 0) {
            kind.parts.emplace_back("with no line terminators");
        } else {
            std::vector<std::string> terminators;
            if (crlf > 0) terminators.emplace_back("CRLF");
            if (cr > 0) terminators.emplace_back("CR");
            if (lf > 0) terminators.emplace_back("LF");
            std::string joined;
            for (size_t i = 0; i < terminators.size(); ++i) {
                if (i > 0) joined += ", ";
                joined += terminators[i];
            }
            kind.parts.push_back("with " + joined + " line terminators");
        }
    }

    if (hasEscapes) kind.parts.emplace_back("with escape sequences");
    if (hasBackspace) kind.parts.emplace_back("with overstriking");

    kind.mime = language.mime;
    kind.charset = charset;
    return true;
}

} // namespace

void FileInfoCommand(const std::vector<std::string>& args) {
    bool brief = false;
    bool mime = false;
    bool mimeTypeOnly = false;
    bool mimeEncodingOnly = false;
    bool follow = false;
    std::vector<std::string> targets;

    auto ShowUsage = []() {
        SetColor(ConsoleColor::Cyan);
        std::cout << "usage: file [-bhiL] [--mime-type] [--mime-encoding] <file>...\n";
        std::cout << "-b (or --brief) - dont show the filename, just the result\n";
        std::cout << "-i (or --mime) - show the mime type + charset instead of a description\n";
        std::cout << "--mime-type - only show the mime type\n";
        std::cout << "--mime-encoding - only show the charset\n";
        std::cout << "-L (or --dereference) - follow symlinks\n";
        std::cout << "-h (or --no-dereference) - dont follow symlinks (default)\n";
        std::cout << "<file> - the file(s) needed to identify (use quotes if the name has spaces)\n";
        ResForegroundColor();
    };

    for (size_t i = 1; i < args.size(); i++) {
        const std::string& arg = args[i];
        if (arg == "--") {
            for (size_t j = i + 1; j < args.size(); ++j) targets.push_back(args[j]);
            break;
        }

        if (arg.starts_with("--")) {
            if (arg == "--brief") brief = true;
            else if (arg == "--mime") mime = true;
            else if (arg == "--mime-type") { mime = true; mimeTypeOnly = true; }
            else if (arg == "--mime-encoding") { mime = true; mimeEncodingOnly = true; }
            else if (arg == "--dereference") follow = true;
            else if (arg == "--no-dereference") follow = false;
            else if (arg == "--help") { ShowUsage(); return; }
            else {
                SetColor(ConsoleColor::Red);
                std::cout << "file: unrecognized option '" << arg << "'\n";
                std::cout << "for more info pls seek help thru \"file --help\"\n";
                ResForegroundColor();
                return;
            }
            continue;
        }

        if (arg.starts_with("-") && arg.size() > 1) {
            for (size_t k = 1; k < arg.size(); ++k) {
                char flag = arg[k];
                switch (flag) {
                    case 'b': brief = true; break;
                    case 'i': mime = true; break;
                    case 'L': follow = true; break;
                    case 'h': follow = false; break;
                    case '?': ShowUsage(); return;
                    default:
                        SetColor(ConsoleColor::Red);
                        std::cout << "file: invalid option -- '" << flag << "'\n";
                        std::cout << "for more info pls seek help thru \"file --help\"\n";
                        ResForegroundColor();
                        return;
                }
            }
            continue;
        }

        targets.push_back(arg);
    }

    if (targets.empty()) {
        ShowUsage();
        return;
    }

    for (const auto& target : targets) {
        FileKind kind = IdentifyFile(target, follow);
        std::vector<std::string> lines;

        if (kind.error || !mime) {
            lines = kind.parts;
        } else if (mimeTypeOnly) {
            lines.push_back(kind.mime);
        } else if (mimeEncodingOnly) {
            lines.push_back(kind.charset);
        } else {
            lines.push_back(kind.mime);
            lines.push_back("charset=" + kind.charset);
        }

        PrintFileTree(brief ? nullptr : &target, lines, kind.error);
    }

    ResForegroundColor();
}

void PrintFileTree(const std::string* name, const std::vector<std::string>& lines, bool error) {
    ConsoleColor textColor = error ? ConsoleColor::Red : ConsoleColor::Yellow;

    if (name != nullptr) {
        SetColor(ConsoleColor::Cyan);
        std::cout << "┌─ " << *name << "\n";
    }

    for (size_t i = 0; i < lines.size(); i++) {
        std::string branch;
        if (name == nullptr && lines.size() == 1)
            branch = "── ";
        else if (name == nullptr && i == 0)
            branch = "┌─ ";
        else if (i == lines.size() - 1)
            branch = "└─ ";
        else
            branch = "│  ";

        SetColor(ConsoleColor::Cyan);
        std::cout << branch;
        SetColor(textColor);
        std::cout << lines[i] << "\n";
    }

    ResForegroundColor();
}

FileKind IdentifyFile(const std::string& target, bool follow) {
    FileKind kind;
    std::error_code ec;
    fs::path raw(target);
    fs::path path = raw.is_absolute() ? raw : (fs::path(currentDir) / raw);

    if (fs::is_symlink(path, ec) && !follow) {
        fs::path linkTarget = fs::read_symlink(path, ec);
        bool broken = !fs::exists(path, ec);
        kind.parts.push_back(std::string(broken ? "broken symbolic link to " : "symbolic link to ") + linkTarget.string());
        kind.mime = "inode/symlink";
        return kind;
    }

    if (fs::is_directory(path, ec)) {
        kind.parts.emplace_back("directory");
        kind.mime = "inode/directory";
        return kind;
    }

    if (!fs::is_regular_file(path, ec)) {
        kind.error = true;
        kind.parts.push_back("tf u mean `" + target + "'");
        return kind;
    }

    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        kind.error = true;
        kind.parts.push_back("cannot open `" + target + "' (Permission denied)");
        return kind;
    }

    stream.seekg(0, std::ios::end);
    int64_t fileLength = stream.tellg();
    stream.seekg(0, std::ios::beg);

    if (fileLength == 0) {
        kind.parts.emplace_back("empty");
        kind.mime = "inode/x-empty";
        return kind;
    }

    size_t toRead = static_cast<size_t>(std::min<int64_t>(fileLength, 1048576LL));
    std::vector<uint8_t> data(toRead);
    stream.read(reinterpret_cast<char*>(data.data()), toRead);
    data.resize(static_cast<size_t>(stream.gcount()));

    if (DetectBinaryType(data, path.string(), kind))
        return kind;

    if (AnalyzeText(data, fileLength, kind))
        return kind;

    if (data.size() <= 1) {
        kind.parts.emplace_back("very short file (no magic)");
        return kind;
    }

    kind.parts.emplace_back("data");
    return kind;
}

} // namespace fis
