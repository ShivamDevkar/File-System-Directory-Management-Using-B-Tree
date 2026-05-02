// ─────────────────────────────────────────────────────────────
// Utils.cpp — Utility / helper functions
// ─────────────────────────────────────────────────────────────

#include "Utils.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <cerrno>
#include <sys/stat.h>
#include <sstream>

#ifdef _WIN32
    #include <direct.h>
    #define MAKE_DIR(path) _mkdir(path)
#else
    #define MAKE_DIR(path) mkdir(path, 0755)
#endif

namespace Utils {

    // ── Directory management ────────────────────────────────

    bool ensureDirectory(const std::string& path) {
        int result = MAKE_DIR(path.c_str());
        if (result == 0 || errno == EEXIST) return true;
        std::cerr << "[Utils] WARNING: Could not create directory '"
                  << path << "': " << strerror(errno) << "\n";
        return false;
    }

    void ensureProjectDirectories() {
        ensureDirectory("logs");
        ensureDirectory("data");
        ensureDirectory("docs");
        ensureDirectory("tests");
    }

    // ── Path manipulation ───────────────────────────────────

    std::vector<std::string> splitPath(const std::string& path) {
        std::vector<std::string> segments;
        std::stringstream ss(path);
        std::string segment;
        while (std::getline(ss, segment, '/')) {
            if (!segment.empty()) segments.push_back(segment);
        }
        return segments;
    }

    std::string joinPath(const std::vector<std::string>& segments) {
        if (segments.empty()) return "/";
        std::string result;
        for (const auto& seg : segments) result += "/" + seg;
        return result;
    }

    std::string getParentPath(const std::string& path) {
        if (path == "/" || path.empty()) return "/";
        size_t lastSlash = path.rfind('/');
        if (lastSlash == 0) return "/root";
        return path.substr(0, lastSlash);
    }

    std::string getFileName(const std::string& path) {
        size_t lastSlash = path.rfind('/');
        if (lastSlash == std::string::npos) return path;
        return path.substr(lastSlash + 1);
    }

    // ── Path utilities ──────────────────────────────────────

    bool isAbsolutePath(const std::string& path) {
        return !path.empty() && path[0] == '/';
    }

    std::string normalizePath(const std::string& path) {
        if (path.empty()) return "/";
        std::vector<std::string> parts;
        std::stringstream ss(path);
        std::string segment;
        while (std::getline(ss, segment, '/')) {
            if (segment.empty() || segment == ".") continue;
            if (segment == "..") {
                if (!parts.empty()) parts.pop_back();
            } else {
                parts.push_back(segment);
            }
        }
        if (parts.empty()) return "/";
        std::string result;
        for (auto& p : parts) result += "/" + p;
        return result;
    }

    std::string getExtension(const std::string& filename) {
        size_t dot = filename.rfind('.');
        if (dot == std::string::npos || dot == 0) return "";
        return filename.substr(dot);
    }

    // ── String helpers ──────────────────────────────────────

    std::string trimWhitespace(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }

    std::string toLower(const std::string& s) {
        std::string result = s;
        std::transform(result.begin(), result.end(), result.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        return result;
    }

    std::vector<std::string> splitTokens(const std::string& input) {
        std::vector<std::string> tokens;
        std::stringstream ss(input);
        std::string token;
        while (ss >> token) tokens.push_back(token);
        return tokens;
    }

    // ── Validation ──────────────────────────────────────────

    bool isValidFileName(const std::string& name) {
        if (name.empty() || name == "." || name == ".." || name.size() > 255)
            return false;
        for (char c : name) {
            if (c == '/' || c == ' ' || c == '\t' || c == '\n') return false;
        }
        return true;
    }

    bool isValidPermissions(const std::string& perms) {
        if (perms.size() != 9) return false;
        for (int i = 0; i < 9; i++) {
            char expected;
            if      (i % 3 == 0) expected = 'r';
            else if (i % 3 == 1) expected = 'w';
            else                 expected = 'x';
            if (perms[i] != expected && perms[i] != '-') return false;
        }
        return true;
    }

    // ── Formatting ──────────────────────────────────────────

    std::string formatSize(size_t bytes) {
        if (bytes == 0) return "0 B";
        std::ostringstream oss;
        oss << std::fixed;
        if (bytes < 1024) {
            oss << bytes << " B";
        } else if (bytes < 1024ULL * 1024) {
            oss.precision(2);
            oss << (double)bytes / 1024.0 << " KB";
        } else if (bytes < 1024ULL * 1024 * 1024) {
            oss.precision(2);
            oss << (double)bytes / (1024.0 * 1024.0) << " MB";
        } else {
            oss.precision(2);
            oss << (double)bytes / (1024.0 * 1024.0 * 1024.0) << " GB";
        }
        return oss.str();
    }

    std::string formatTimestamp(time_t t) {
        std::string s = ctime(&t);
        if (!s.empty() && s.back() == '\n') s.pop_back();
        return s;
    }

    // ── Disk helpers ────────────────────────────────────────

    bool fileExists(const std::string& path) {
        struct stat buffer;
        return (stat(path.c_str(), &buffer) == 0);
    }

    size_t getFileSize(const std::string& path) {
        struct stat buffer;
        if (stat(path.c_str(), &buffer) != 0) return 0;
        return (size_t)buffer.st_size;
    }

    // ── Console helpers ─────────────────────────────────────

    void printSeparator(int width, char ch) {
        std::cout << std::string(width, ch) << "\n";
    }

    std::string repeatString(const std::string& s, int n) {
        std::string result;
        result.reserve(s.size() * n);
        for (int i = 0; i < n; i++) result += s;
        return result;
    }

} // namespace Utils