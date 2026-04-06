// ─────────────────────────────────────────────────────────────
// FileSystemManager.cpp — Full Implementation
// All commands with line-by-line comments
// ─────────────────────────────────────────────────────────────

#include "FileSystemManager.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <ctime>

// ANSI colors used inside FileSystemManager output
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define CYAN    "\033[36m"
#define BOLD    "\033[1m"
#define RESET   "\033[0m"

// ─────────────────────────────────────────────────────────────
// CONSTRUCTOR & DESTRUCTOR
// ─────────────────────────────────────────────────────────────

FileSystemManager::FileSystemManager(int order)
    : currentDir("/root")
    , treeOrder(order)
{
    BPlusTree<std::string, FileMetadata>* rootTree =
        new BPlusTree<std::string, FileMetadata>(treeOrder);
    directoryMap["/root"] = rootTree;
    currentTree = rootTree;
    std::cout << GREEN << "File system initialized." << RESET
              << " Root directory: /root\n";
}

FileSystemManager::~FileSystemManager() {
    for (auto& pair : directoryMap) {
        delete pair.second;
        pair.second = nullptr;
    }
}

// ─────────────────────────────────────────────────────────────
// PRIVATE HELPERS
// ─────────────────────────────────────────────────────────────

std::string FileSystemManager::buildPath(const std::string& name) const {
    return currentDir + "/" + name;
}

// Collect all leaf entries from a tree into a vector
static std::vector<FileMetadata> getAllEntries(BPlusTree<std::string, FileMetadata>* tree) {
    std::vector<FileMetadata> entries;
    BPlusNode<std::string, FileMetadata>* leaf = tree->getRoot();
    if (!leaf) return entries;
    while (!leaf->isLeaf) leaf = leaf->children[0];
    while (leaf) {
        for (int i = 0; i < leaf->keyCount(); i++)
            entries.push_back(leaf->values[i]);
        leaf = leaf->next;
    }
    return entries;
}

void FileSystemManager::findInTree(const std::string& dirPath,
                                    const std::string& targetName,
                                    std::vector<std::string>& results) {
    if (!directoryMap.count(dirPath)) return;
    auto entries = getAllEntries(directoryMap[dirPath]);
    for (auto& meta : entries) {
        if (meta.fileName == targetName)
            results.push_back(meta.absolutePath);
        if (meta.isDirectory())
            findInTree(dirPath + "/" + meta.fileName, targetName, results);
    }
}

void FileSystemManager::findByExtInTree(const std::string& dirPath,
                                         const std::string& ext,
                                         std::vector<std::string>& results) {
    if (!directoryMap.count(dirPath)) return;
    auto entries = getAllEntries(directoryMap[dirPath]);
    for (auto& meta : entries) {
        if (meta.isFile()) {
            // Check if filename ends with the given extension
            std::string fname = meta.fileName;
            if (fname.size() >= ext.size() &&
                fname.substr(fname.size() - ext.size()) == ext)
                results.push_back(meta.absolutePath);
        }
        if (meta.isDirectory())
            findByExtInTree(dirPath + "/" + meta.fileName, ext, results);
    }
}

void FileSystemManager::searchKeywordInTree(const std::string& dirPath,
                                             const std::string& keyword,
                                             std::vector<std::string>& results) {
    if (!directoryMap.count(dirPath)) return;
    auto entries = getAllEntries(directoryMap[dirPath]);
    for (auto& meta : entries) {
        // Check if filename contains the keyword (case-sensitive)
        if (meta.fileName.find(keyword) != std::string::npos)
            results.push_back(meta.absolutePath);
        if (meta.isDirectory())
            searchKeywordInTree(dirPath + "/" + meta.fileName, keyword, results);
    }
}

void FileSystemManager::removeDirectoryRecursive(const std::string& dirPath) {
    if (!directoryMap.count(dirPath)) return;
    auto entries = getAllEntries(directoryMap[dirPath]);
    for (auto& meta : entries) {
        if (meta.isDirectory())
            removeDirectoryRecursive(dirPath + "/" + meta.fileName);
        directoryMap[dirPath]->remove(meta.fileName);
    }
    delete directoryMap[dirPath];
    directoryMap.erase(dirPath);
}

void FileSystemManager::treeHelper(const std::string& dirPath, int depth) {
    if (!directoryMap.count(dirPath)) return;
    auto entries = getAllEntries(directoryMap[dirPath]);
    for (size_t i = 0; i < entries.size(); i++) {
        auto& meta = entries[i];
        bool isLast = (i == entries.size() - 1);
        std::string indent = std::string(depth * 4, ' ');
        std::string branch = isLast ? "`-- " : "|-- ";
        if (meta.isDirectory()) {
            std::cout << indent << branch << BLUE << BOLD << meta.fileName << "/" << RESET << "\n";
            treeHelper(dirPath + "/" + meta.fileName, depth + 1);
        } else {
            std::cout << indent << branch << meta.fileName
                      << CYAN << " (" << meta.getFormattedSize() << ")" << RESET << "\n";
        }
    }
}

void FileSystemManager::duHelper(const std::string& dirPath, size_t& total, int& fcount, int& dcount) {
    if (!directoryMap.count(dirPath)) return;
    auto entries = getAllEntries(directoryMap[dirPath]);
    for (auto& meta : entries) {
        if (meta.isFile()) {
            total += meta.fileSize;
            fcount++;
        } else {
            dcount++;
            duHelper(dirPath + "/" + meta.fileName, total, fcount, dcount);
        }
    }
}

// ─────────────────────────────────────────────────────────────
// NAVIGATION COMMANDS
// ─────────────────────────────────────────────────────────────

void FileSystemManager::cd(const std::string& path) {
    if (path == "/") {
        dirHistory.push(currentDir);
        currentDir  = "/root";
        currentTree = directoryMap["/root"];
        std::cout << "Changed to: " << BOLD << currentDir << RESET << "\n";
        return;
    }
    if (path == "..") {
        if (currentDir == "/root") {
            std::cout << YELLOW << "Already at root directory\n" << RESET;
            return;
        }
        size_t lastSlash = currentDir.rfind('/');
        std::string parentDir = currentDir.substr(0, lastSlash);
        if (parentDir.empty()) parentDir = "/root";
        dirHistory.push(currentDir);
        currentDir  = parentDir;
        currentTree = directoryMap[currentDir];
        std::cout << "Changed to: " << BOLD << currentDir << RESET << "\n";
        return;
    }
    if (path == "-") {
        back();
        return;
    }
    std::string targetPath = buildPath(path);
    if (!directoryMap.count(targetPath)) {
        std::cout << RED << "Error: no such directory '" << path << "'\n" << RESET;
        return;
    }
    FileMetadata meta;
    if (currentTree->search(path, meta) && meta.isFile()) {
        std::cout << RED << "Error: '" << path << "' is a file, not a directory\n" << RESET;
        return;
    }
    dirHistory.push(currentDir);
    currentDir  = targetPath;
    currentTree = directoryMap[targetPath];
    std::cout << "Changed to: " << BOLD << currentDir << RESET << "\n";
}

void FileSystemManager::pwd() {
    std::cout << BOLD << currentDir << RESET << "\n";
}

void FileSystemManager::back() {
    // Goes back to the previous directory — like "cd -" in bash
    if (dirHistory.empty()) {
        std::cout << YELLOW << "No previous directory in history\n" << RESET;
        return;
    }
    std::string prev = dirHistory.top();
    dirHistory.pop();
    dirHistory.push(currentDir);   // so back() again returns here
    currentDir  = prev;
    currentTree = directoryMap[currentDir];
    std::cout << "Back to: " << BOLD << currentDir << RESET << "\n";
}

// ─────────────────────────────────────────────────────────────
// LISTING COMMANDS
// ─────────────────────────────────────────────────────────────

void FileSystemManager::ls(bool showHidden) {
    std::cout << "\n" << BOLD << "Contents of " << currentDir << ":\n" << RESET;
    std::cout << std::string(52, '-') << "\n";
    std::cout << std::left
              << std::setw(6)  << "Type"
              << std::setw(26) << "Name"
              << std::setw(10) << "Size"
              << "Permissions\n";
    std::cout << std::string(52, '-') << "\n";

    BPlusNode<std::string, FileMetadata>* leaf = currentTree->getRoot();
    if (!leaf) { std::cout << "(empty)\n"; return; }
    while (!leaf->isLeaf) leaf = leaf->children[0];

    int count = 0;
    while (leaf) {
        for (int i = 0; i < leaf->keyCount(); i++) {
            auto& meta = leaf->values[i];
            if (!showHidden && !meta.fileName.empty() && meta.fileName[0] == '.') continue;
            // display variable unused — kept for future color use
            (void)meta; // suppress warning
            std::cout << std::left
                      << std::setw(6)  << meta.getTypeString()
                      << std::setw(26) << (meta.isDirectory() ? meta.fileName + "/" : meta.fileName)
                      << std::setw(10) << meta.getFormattedSize()
                      << meta.permissions << "\n";
            count++;
        }
        leaf = leaf->next;
    }
    std::cout << std::string(52, '-') << "\n";
    std::cout << count << " item(s)\n\n";
}

void FileSystemManager::ll() {
    // Detailed listing — shows all metadata including timestamps
    std::cout << "\n" << BOLD << "Detailed listing of " << currentDir << ":\n" << RESET;
    std::cout << std::string(90, '-') << "\n";
    std::cout << std::left
              << std::setw(6)  << "Type"
              << std::setw(22) << "Name"
              << std::setw(10) << "Size"
              << std::setw(12) << "Perms"
              << "Last Modified\n";
    std::cout << std::string(90, '-') << "\n";

    auto entries = getAllEntries(currentTree);
    if (entries.empty()) { std::cout << "(empty)\n"; return; }

    for (auto& meta : entries) {
        std::cout << std::left
                  << std::setw(6)  << meta.getTypeString()
                  << std::setw(22) << (meta.isDirectory() ? meta.fileName + "/" : meta.fileName)
                  << std::setw(10) << meta.getFormattedSize()
                  << std::setw(12) << meta.permissions
                  << meta.getFormattedTime() << "\n";
    }
    std::cout << std::string(90, '-') << "\n";
    std::cout << entries.size() << " item(s)\n\n";
}

void FileSystemManager::lsSort(const std::string& by) {
    // List current directory sorted by name / size / time
    auto entries = getAllEntries(currentTree);
    if (entries.empty()) { std::cout << "(empty directory)\n"; return; }

    if (by == "size") {
        // Sort by file size ascending
        std::sort(entries.begin(), entries.end(),
            [](const FileMetadata& a, const FileMetadata& b) {
                return a.fileSize < b.fileSize;
            });
        std::cout << CYAN << "Sorted by size (smallest first):\n" << RESET;
    } else if (by == "time") {
        // Sort by modification time — most recent first
        std::sort(entries.begin(), entries.end(),
            [](const FileMetadata& a, const FileMetadata& b) {
                return a.modifiedAt > b.modifiedAt;
            });
        std::cout << CYAN << "Sorted by time (newest first):\n" << RESET;
    } else {
        // Default: sort by name alphabetically
        std::sort(entries.begin(), entries.end(),
            [](const FileMetadata& a, const FileMetadata& b) {
                return a.fileName < b.fileName;
            });
        std::cout << CYAN << "Sorted by name (A-Z):\n" << RESET;
    }

    std::cout << std::string(50, '-') << "\n";
    for (auto& meta : entries) {
        std::cout << std::left
                  << std::setw(6)  << meta.getTypeString()
                  << std::setw(26) << (meta.isDirectory() ? meta.fileName + "/" : meta.fileName)
                  << std::setw(10) << meta.getFormattedSize()
                  << meta.permissions << "\n";
    }
    std::cout << std::string(50, '-') << "\n";
}

void FileSystemManager::tree(const std::string& path, int depth) {
    std::string startPath = path.empty() ? currentDir : path;
    std::cout << BOLD << startPath << RESET << "\n";
    treeHelper(startPath, 0);
    std::cout << "\n";
}

// ─────────────────────────────────────────────────────────────
// CREATE COMMANDS
// ─────────────────────────────────────────────────────────────

void FileSystemManager::mkdir(const std::string& name) {
    if (name.empty()) { std::cout << RED << "Error: name cannot be empty\n" << RESET; return; }
    FileMetadata ex;
    if (currentTree->search(name, ex)) {
        std::cout << RED << "Error: '" << name << "' already exists\n" << RESET; return;
    }
    std::string newPath = buildPath(name);
    directoryMap[newPath] = new BPlusTree<std::string, FileMetadata>(treeOrder);
    FileMetadata dirMeta(name, EntryType::DIRECTORY, 0, "rwxr-xr-x", newPath);
    currentTree->insert(name, dirMeta);
    std::cout << GREEN << "Directory '" << name << "' created" << RESET << " at " << newPath << "\n";
}

void FileSystemManager::mkdirp(const std::string& path) {
    // Creates nested directories in one command
    // Example: mkdirp projects/cpp/ds2
    // Creates: /root/projects, /root/projects/cpp, /root/projects/cpp/ds2

    if (path.empty()) { std::cout << RED << "Error: path cannot be empty\n" << RESET; return; }

    // Split path by '/'
    std::vector<std::string> parts;
    std::stringstream ss(path);
    std::string part;
    while (std::getline(ss, part, '/')) {
        if (!part.empty()) parts.push_back(part);
    }

    // Save where we are now — we'll come back here after creating dirs
    std::string savedDir  = currentDir;
    BPlusTree<std::string, FileMetadata>* savedTree = currentTree;

    for (auto& p : parts) {
        FileMetadata ex;
        if (!currentTree->search(p, ex)) {
            // Directory doesn't exist yet — create it
            std::string newPath = buildPath(p);
            directoryMap[newPath] = new BPlusTree<std::string, FileMetadata>(treeOrder);
            FileMetadata dirMeta(p, EntryType::DIRECTORY, 0, "rwxr-xr-x", newPath);
            currentTree->insert(p, dirMeta);
            std::cout << GREEN << "  Created: " << RESET << newPath << "\n";
        }
        // Move into this directory to create the next level
        std::string nextPath = buildPath(p);
        currentDir  = nextPath;
        currentTree = directoryMap[nextPath];
    }

    // Restore original location
    currentDir  = savedDir;
    currentTree = savedTree;

    std::cout << GREEN << "All directories created successfully\n" << RESET;
}

void FileSystemManager::touch(const std::string& name, size_t size, const std::string& perms) {
    if (name.empty()) { std::cout << RED << "Error: name cannot be empty\n" << RESET; return; }
    FileMetadata ex;
    if (currentTree->search(name, ex)) {
        // touch on existing file just updates modification time (like Linux)
        ex.modifiedAt = time(nullptr);
        currentTree->insert(name, ex);
        std::cout << YELLOW << "Updated timestamp of '" << name << "'\n" << RESET;
        return;
    }
    std::string filePath = buildPath(name);
    FileMetadata fileMeta(name, EntryType::FILE, size, perms, filePath);
    currentTree->insert(name, fileMeta);
    std::cout << GREEN << "File '" << name << "' created" << RESET
              << " (" << fileMeta.getFormattedSize() << ")\n";
}

// ─────────────────────────────────────────────────────────────
// DELETE COMMANDS
// ─────────────────────────────────────────────────────────────

void FileSystemManager::rm(const std::string& name) {
    FileMetadata meta;
    if (!currentTree->search(name, meta)) {
        std::cout << RED << "Error: '" << name << "' not found\n" << RESET; return;
    }
    if (meta.isFile()) {
        currentTree->remove(name);
        std::cout << RED << "Deleted file: " << RESET << name << "\n";
    } else {
        std::string dirPath = buildPath(name);
        removeDirectoryRecursive(dirPath);
        currentTree->remove(name);
        std::cout << RED << "Deleted directory: " << RESET << name << " (and all contents)\n";
    }
}

void FileSystemManager::rmAll() {
    // Deletes EVERYTHING in the current directory
    // Asks for confirmation first (safety check)
    auto entries = getAllEntries(currentTree);
    if (entries.empty()) { std::cout << YELLOW << "Directory is already empty\n" << RESET; return; }

    std::cout << YELLOW << "WARNING: This will delete " << entries.size()
              << " items in " << currentDir << "\n";
    std::cout << "Type 'yes' to confirm: " << RESET;
    std::string confirm;
    std::getline(std::cin, confirm);
    if (confirm != "yes") { std::cout << "Cancelled.\n"; return; }

    for (auto& meta : entries) {
        if (meta.isDirectory()) {
            removeDirectoryRecursive(buildPath(meta.fileName));
        }
        currentTree->remove(meta.fileName);
    }
    std::cout << GREEN << "Directory cleared.\n" << RESET;
}

// ─────────────────────────────────────────────────────────────
// COPY / MOVE / RENAME
// ─────────────────────────────────────────────────────────────

void FileSystemManager::mv(const std::string& name, const std::string& destDir) {
    FileMetadata meta;
    if (!currentTree->search(name, meta)) {
        std::cout << RED << "Error: '" << name << "' not found\n" << RESET; return;
    }
    if (meta.isDirectory()) {
        std::cout << RED << "Error: mv does not support moving directories\n" << RESET; return;
    }
    if (!directoryMap.count(destDir)) {
        std::cout << RED << "Error: destination '" << destDir << "' does not exist\n" << RESET; return;
    }
    FileMetadata check;
    if (directoryMap[destDir]->search(name, check)) {
        std::cout << RED << "Error: '" << name << "' already exists in " << destDir << "\n" << RESET; return;
    }
    meta.absolutePath = destDir + "/" + name;
    meta.modifiedAt   = time(nullptr);
    directoryMap[destDir]->insert(name, meta);
    currentTree->remove(name);
    std::cout << GREEN << "Moved '" << name << "'" << RESET << " to " << destDir << "\n";
}

void FileSystemManager::cp(const std::string& name, const std::string& destDir) {
    // Copy a file to another directory (keeps original in place)
    FileMetadata meta;
    if (!currentTree->search(name, meta)) {
        std::cout << RED << "Error: '" << name << "' not found\n" << RESET; return;
    }
    if (meta.isDirectory()) {
        std::cout << RED << "Error: cp does not support copying directories\n" << RESET; return;
    }
    if (!directoryMap.count(destDir)) {
        std::cout << RED << "Error: destination '" << destDir << "' does not exist\n" << RESET; return;
    }
    FileMetadata check;
    if (directoryMap[destDir]->search(name, check)) {
        std::cout << RED << "Error: '" << name << "' already exists in " << destDir << "\n" << RESET; return;
    }
    // Create a copy with updated path and time
    FileMetadata copyMeta = meta;
    copyMeta.absolutePath = destDir + "/" + name;
    copyMeta.createdAt    = time(nullptr);
    copyMeta.modifiedAt   = time(nullptr);
    directoryMap[destDir]->insert(name, copyMeta);
    std::cout << GREEN << "Copied '" << name << "'" << RESET << " to " << destDir << "\n";
}

void FileSystemManager::rename(const std::string& oldName, const std::string& newName) {
    if (newName.empty()) { std::cout << RED << "Error: new name cannot be empty\n" << RESET; return; }
    FileMetadata meta;
    if (!currentTree->search(oldName, meta)) {
        std::cout << RED << "Error: '" << oldName << "' not found\n" << RESET; return;
    }
    FileMetadata check;
    if (currentTree->search(newName, check)) {
        std::cout << RED << "Error: '" << newName << "' already exists\n" << RESET; return;
    }
    if (meta.isDirectory()) {
        std::string oldPath = buildPath(oldName);
        std::string newPath = buildPath(newName);
        if (directoryMap.count(oldPath)) {
            directoryMap[newPath] = directoryMap[oldPath];
            directoryMap.erase(oldPath);
        }
    }
    meta.fileName     = newName;
    meta.absolutePath = buildPath(newName);
    meta.modifiedAt   = time(nullptr);
    currentTree->remove(oldName);
    currentTree->insert(newName, meta);
    std::cout << GREEN << "Renamed '" << oldName << "' -> '" << newName << "'\n" << RESET;
}

// ─────────────────────────────────────────────────────────────
// SEARCH COMMANDS
// ─────────────────────────────────────────────────────────────

void FileSystemManager::find(const std::string& targetName) {
    std::cout << CYAN << "Searching for '" << targetName << "'..." << RESET << "\n";
    std::vector<std::string> results;
    findInTree("/root", targetName, results);
    if (results.empty()) {
        std::cout << YELLOW << "Not found: '" << targetName << "'\n" << RESET;
    } else {
        std::cout << GREEN << "Found " << results.size() << " result(s):\n" << RESET;
        for (auto& r : results) std::cout << "  " << r << "\n";
    }
}

void FileSystemManager::findExt(const std::string& ext) {
    // Find all files with a given extension across the whole filesystem
    // Usage: findext .pdf   or  findext pdf
    std::string searchExt = ext;
    if (!searchExt.empty() && searchExt[0] != '.') searchExt = "." + searchExt;

    std::cout << CYAN << "Searching for '*" << searchExt << "' files..." << RESET << "\n";
    std::vector<std::string> results;
    findByExtInTree("/root", searchExt, results);

    if (results.empty()) {
        std::cout << YELLOW << "No files found with extension '" << searchExt << "'\n" << RESET;
    } else {
        std::cout << GREEN << "Found " << results.size() << " file(s):\n" << RESET;
        for (auto& r : results) std::cout << "  " << r << "\n";
    }
}

void FileSystemManager::search(const std::string& keyword) {
    // Find all files/dirs whose name contains the keyword
    std::cout << CYAN << "Searching for files containing '" << keyword << "'..." << RESET << "\n";
    std::vector<std::string> results;
    searchKeywordInTree("/root", keyword, results);

    if (results.empty()) {
        std::cout << YELLOW << "Nothing found containing '" << keyword << "'\n" << RESET;
    } else {
        std::cout << GREEN << "Found " << results.size() << " result(s):\n" << RESET;
        for (auto& r : results) std::cout << "  " << r << "\n";
    }
}

// ─────────────────────────────────────────────────────────────
// FILE INFO COMMANDS
// ─────────────────────────────────────────────────────────────

void FileSystemManager::stat(const std::string& name) {
    // Show complete metadata for a single file or directory
    FileMetadata meta;
    if (!currentTree->search(name, meta)) {
        std::cout << RED << "Error: '" << name << "' not found\n" << RESET; return;
    }
    std::cout << "\n" << BOLD << "--- File Info ---\n" << RESET;
    std::cout << std::left << std::setw(18) << "Name:"         << meta.fileName << "\n";
    std::cout << std::left << std::setw(18) << "Type:"         << meta.getTypeString() << "\n";
    std::cout << std::left << std::setw(18) << "Size:"         << meta.getFormattedSize() << "\n";
    std::cout << std::left << std::setw(18) << "Permissions:"  << meta.permissions << "\n";
    std::cout << std::left << std::setw(18) << "Absolute path:" << meta.absolutePath << "\n";
    std::cout << std::left << std::setw(18) << "Created:"      << meta.getFormattedTime() << "\n";
    std::cout << std::left << std::setw(18) << "Modified:"     << meta.getFormattedTime() << "\n";
    std::cout << "\n";
}

void FileSystemManager::fileCount() {
    // Count files and directories in current directory only (not recursive)
    auto entries = getAllEntries(currentTree);
    int files = 0, dirs = 0;
    for (auto& e : entries) {
        if (e.isFile()) files++;
        else dirs++;
    }
    std::cout << CYAN << "In " << currentDir << ":\n" << RESET;
    std::cout << "  Files       : " << files << "\n";
    std::cout << "  Directories : " << dirs  << "\n";
    std::cout << "  Total       : " << (files + dirs) << "\n";
}

void FileSystemManager::du() {
    // Disk usage — total size of current directory including all subdirs
    size_t total = 0;
    int fcount = 0, dcount = 0;
    duHelper(currentDir, total, fcount, dcount);

    // Convert bytes to readable
    std::string sizeStr;
    if (total < 1024)
        sizeStr = std::to_string(total) + " B";
    else if (total < 1024*1024) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << (double)total/1024.0 << " KB";
        sizeStr = oss.str();
    } else {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << (double)total/(1024.0*1024.0) << " MB";
        sizeStr = oss.str();
    }

    std::cout << CYAN << "Disk usage for " << currentDir << ":\n" << RESET;
    std::cout << "  Total size  : " << BOLD << sizeStr << RESET << "\n";
    std::cout << "  Files       : " << fcount << "\n";
    std::cout << "  Directories : " << dcount << "\n";
}

// ─────────────────────────────────────────────────────────────
// PERMISSIONS
// ─────────────────────────────────────────────────────────────

void FileSystemManager::chmod(const std::string& name, const std::string& perms) {
    FileMetadata meta;
    if (!currentTree->search(name, meta)) {
        std::cout << RED << "Error: '" << name << "' not found\n" << RESET; return;
    }
    std::string oldPerms = meta.permissions;
    meta.permissions = perms;
    meta.modifiedAt  = time(nullptr);
    currentTree->insert(name, meta);   // insert() updates if key exists
    std::cout << GREEN << "Permissions changed for '" << name << "': "
              << RESET << oldPerms << " -> " << BOLD << perms << RESET << "\n";
}

// ─────────────────────────────────────────────────────────────
// UTILITY
// ─────────────────────────────────────────────────────────────

void FileSystemManager::cls() {
    // Clear the terminal screen
    std::cout << "\033[2J\033[H";
}

void FileSystemManager::history() {
    // Show the navigation history stack
    if (dirHistory.empty()) {
        std::cout << YELLOW << "Navigation history is empty\n" << RESET; return;
    }
    // Copy stack to vector so we can print it (stack has no iterator)
    std::stack<std::string> temp = dirHistory;
    std::vector<std::string> hist;
    while (!temp.empty()) {
        hist.push_back(temp.top());
        temp.pop();
    }
    std::cout << CYAN << "Navigation History (most recent first):\n" << RESET;
    for (size_t i = 0; i < hist.size(); i++) {
        std::cout << "  " << (i+1) << ". " << hist[i] << "\n";
    }
}

// ─────────────────────────────────────────────────────────────
// DEBUG / PRESENTATION
// ─────────────────────────────────────────────────────────────

void FileSystemManager::displayCurrentTree() {
    std::cout << BOLD << CYAN
              << "\nB+ Tree structure for: " << currentDir << "\n"
              << RESET;
    currentTree->displayTree();
    std::cout << "\nLeaf chain: ";
    currentTree->displayLeaf();
}

void FileSystemManager::benchmark(int n) {
    // Insert n dummy files and measure how long it takes
    // Great for demonstrating O(log n) performance in presentation
    std::cout << CYAN << "Benchmark: inserting " << n << " files...\n" << RESET;

    BPlusTree<std::string, FileMetadata> testTree(4);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < n; i++) {
        std::string name = "bench_file_" + std::to_string(i) + ".txt";
        FileMetadata m(name, EntryType::FILE, (size_t)i * 100, "rw-r--r--", "/bench/" + name);
        testTree.insert(name, m);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << BOLD << GREEN << "Results:\n" << RESET;
    std::cout << "  Files inserted : " << n    << "\n";
    std::cout << "  Total time     : " << ms   << " ms\n";
    if (ms > 0)
        std::cout << "  Avg per insert : " << (double)ms / n << " ms\n";
    else
        std::cout << "  Avg per insert : < 1 ms (very fast!)\n";

    // Now test search speed
    std::string searchTarget = "bench_file_" + std::to_string(n/2) + ".txt";
    auto s2 = std::chrono::high_resolution_clock::now();
    FileMetadata result;
    bool found = testTree.search(searchTarget, result);
    auto e2 = std::chrono::high_resolution_clock::now();
    auto us  = std::chrono::duration_cast<std::chrono::microseconds>(e2 - s2).count();
    std::cout << "  Search '" << searchTarget << "': "
              << (found ? "FOUND" : "NOT FOUND")
              << " in " << us << " microseconds\n";
}

// ─────────────────────────────────────────────────────────────
// ACCESSORS
// ─────────────────────────────────────────────────────────────

std::unordered_map<std::string, BPlusTree<std::string, FileMetadata>*>&
FileSystemManager::getDirectoryMap() { return directoryMap; }

std::string FileSystemManager::getCurrentDir() const { return currentDir; }

void FileSystemManager::setCurrentDir(const std::string& dir) {
    if (directoryMap.count(dir)) {
        currentDir  = dir;
        currentTree = directoryMap[dir];
    }
}