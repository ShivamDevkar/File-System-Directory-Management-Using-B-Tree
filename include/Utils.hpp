#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// Utils.hpp  —  Declaration of all utility functions
// Implementation is in src/Utils.cpp
// ─────────────────────────────────────────────────────────────────────────────

#include <string>
#include <vector>
#include <ctime>

namespace Utils {

    // Directory management
    bool        ensureDirectory(const std::string& path);
    void        ensureProjectDirectories();

    // Path manipulation
    std::vector<std::string> splitPath(const std::string& path);
    std::string joinPath(const std::vector<std::string>& segments);
    std::string getParentPath(const std::string& path);
    std::string getFileName(const std::string& path);

    // Path utilities
    bool        isAbsolutePath(const std::string& path);
    std::string normalizePath(const std::string& path);
    std::string getExtension(const std::string& filename);

    // String helpers
    std::string trimWhitespace(const std::string& s);
    std::string toLower(const std::string& s);
    std::vector<std::string> splitTokens(const std::string& input);

    // Validation
    bool        isValidFileName(const std::string& name);
    bool        isValidPermissions(const std::string& perms);

    // Formatting
    std::string formatSize(size_t bytes);
    std::string formatTimestamp(time_t t);

    // Disk helpers
    bool        fileExists(const std::string& path);
    size_t      getFileSize(const std::string& path);

    // Console helpers
    void        printSeparator(int width = 50, char ch = '-');
    std::string repeatString(const std::string& s, int n);

} // namespace Utils