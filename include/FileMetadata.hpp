#pragma once 

#include<string>
#include<ctime>
#include <sstream>
#include <iomanip>
using namespace std;

// Represents the type of an entry in the file system 
// Every entry is either a regular file or a directory
enum class EntryType {
    FILE,
    DIRECTORY
};

// Holds all metadata for a single file or directory
// This is the VALUE stored in every B+ Tree leaf node

struct FileMetadata {

    string    fileName;
    EntryType entryType;
    size_t    fileSize;
    string    permissions;
    time_t    createdAt;
    time_t    modifiedAt;
    string    absolutePath;

    // Default constructor
    // Called when you create FileMetadata with no arguments
    // FileMetadata meta;
    FileMetadata()
        : fileName("")
        , entryType(EntryType::FILE)
        , fileSize(0)
        , permissions("rw-r--r--")
        , createdAt(time(nullptr))
        , modifiedAt(time(nullptr))
        , absolutePath("")
    {}

    // Parameterized constructor
    // Called when you create FileMetadata with all details
    // FileMetadata meta("resume.pdf", EntryType::FILE, 2048, ...)
    FileMetadata(
        string    name,
        EntryType type,
        size_t    size,
        string    perms,
        string    path
    )
        : fileName(name)
        , entryType(type)
        , fileSize(size)
        , permissions(perms)
        , createdAt(time(nullptr))
        , modifiedAt(time(nullptr))
        , absolutePath(path)
    {}

    // Returns true if this entry is a directory
    // Used by cd and rm to check what they are dealing with
    bool isDirectory() const {
        return entryType == EntryType::DIRECTORY;
    }

    // Returns true if this entry is a regular file
    bool isFile() const {
        return entryType == EntryType::FILE;
    }

    // Returns a human readable string of the entry type
    // Used when printing ls output
    string getTypeString() const {
        return (entryType == EntryType::DIRECTORY) ? "dir" : "file";
    }

    // Returns file size in human readable format
    // 512      → "512 B"
    // 2048     → "2.00 KB"
    // 2097152  → "2.00 MB"
    string getFormattedSize() const {
        if (entryType == EntryType::DIRECTORY) return "-";
        if (fileSize < 1024)
            return to_string(fileSize) + " B";
        // else if (fileSize < 1024 * 1024)
        //     return to_string((double)fileSize / 1024.0) + " KB";
        // else
        //     return to_string((double)fileSize / (1024.0 * 1024.0)) + " MB";

        ostringstream oss;
    oss << fixed << setprecision(2);
    if (fileSize < 1024 * 1024) {
        oss << (static_cast<double>(fileSize) / 1024.0) << " KB";
    } else {
        oss << (static_cast<double>(fileSize) / (1024.0 * 1024.0)) << " MB";
    }
    return oss.str();
    }

    // Returns last modified time as a readable string
    // time_t stores seconds since Jan 1 1970
    // ctime() converts it to "Mon Jan 15 10:32:00 2024\n"
    // We trim the trailing newline with substr
    string getFormattedTime() const {
        string t = ctime(&modifiedAt);
        return t.substr(0, t.size() - 1);
    }
};