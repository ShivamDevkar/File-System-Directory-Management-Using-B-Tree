// ─────────────────────────────────────────────────────────────
// Filesystemmanager.cpp — Full implementation of all FS commands
// ─────────────────────────────────────────────────────────────

#include "Filesystemmanager.hpp"
#include "WALL_Logger.hpp"
#include "Serializer.hpp"
#include "Utils.hpp"
#include <queue>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <sstream>
#include <ctime>

using namespace std;

// ANSI colours (local use)
static const string R  = "\033[0m";
static const string RED= "\033[31m";
static const string GRN= "\033[32m";
static const string YEL= "\033[33m";
static const string BLU= "\033[34m";
static const string CYN= "\033[36m";
static const string BLD= "\033[1m";

// Global WAL logger
WALLogger logger;

// ─────────────────────────────────────────────
// PATH RESOLVER
// ─────────────────────────────────────────────
string FileSystemManager::resolvePath(const string& path) const {
    if (path.empty()) return currentDir;
    if (path == "/")  return "/root";
    if (path[0] == '/') return path;
    if (path == "..") {
        if (currentDir == "/root") return "/root";
        size_t pos = currentDir.find_last_of('/');
        return (pos <= 0) ? "/root" : currentDir.substr(0, pos);
    }
    return currentDir + "/" + path;
}

string FileSystemManager::buildPath(const string& name) const {
    return currentDir + "/" + name;
}

// ─────────────────────────────────────────────
// CONSTRUCTOR / DESTRUCTOR
// ─────────────────────────────────────────────
FileSystemManager::FileSystemManager(int order)
    : currentDir("/root"), treeOrder(order)
{
    Utils::ensureProjectDirectories();
    Serializer::loadState(directoryMap);

    if (directoryMap.empty()) {
        directoryMap["/root"] = new BPlusTree<string, FileMetadata>(treeOrder);
    }

    currentTree = directoryMap["/root"];
    logger.recoverIfNeeded();
    cout << "File system initialized. Root directory: /root\n";
}

FileSystemManager::~FileSystemManager() {
    for (auto& p : directoryMap) delete p.second;
}

// ─────────────────────────────────────────────
// NAVIGATION
// ─────────────────────────────────────────────
string FileSystemManager::getCurrentDir() const { return currentDir; }
void   FileSystemManager::setCurrentDir(const string& dir) { currentDir = dir; }

void FileSystemManager::pwd() {
    cout << currentDir << "\n";
}

void FileSystemManager::cd(const string& path) {
    if (path == "-") {
        if (dirHistory.empty()) {
            cout << RED << "No previous directory\n" << R;
            return;
        }
        string prev = dirHistory.top(); dirHistory.pop();
        if (directoryMap.count(prev)) {
            currentDir  = prev;
            currentTree = directoryMap[prev];
            cout << "Changed to: " << currentDir << "\n";
        }
        return;
    }

    string newPath = resolvePath(path);
    if (!directoryMap.count(newPath)) {
        cout << RED << "Error: no such directory '" << path << "'\n" << R;
        return;
    }
    dirHistory.push(currentDir);
    currentDir  = newPath;
    currentTree = directoryMap[newPath];
    cout << "Changed to: " << currentDir << "\n";
}

void FileSystemManager::back() { cd("-"); }

void FileSystemManager::history() {
    if (dirHistory.empty()) {
        cout << "No navigation history.\n";
        return;
    }
    stack<string> tmp = dirHistory;
    vector<string> hist;
    while (!tmp.empty()) { hist.push_back(tmp.top()); tmp.pop(); }
    cout << BLD << "Navigation history:\n" << R;
    for (int i = (int)hist.size()-1; i >= 0; i--)
        cout << "  " << hist[i] << "\n";
}

// ─────────────────────────────────────────────
// LISTING
// ─────────────────────────────────────────────
// Collect all leaf values into a vector
static vector<FileMetadata> collectEntries(BPlusTree<string,FileMetadata>* tree) {
    vector<FileMetadata> entries;
    auto node = tree->getRoot();
    if (!node) return entries;
    while (!node->isLeaf) node = node->children[0];
    while (node) {
        for (int i = 0; i < node->keyCount(); i++)
            entries.push_back(node->values[i]);
        node = node->next;
    }
    return entries;
}

void FileSystemManager::ls(bool /*showHidden*/) {
    auto entries = collectEntries(currentTree);
    if (entries.empty()) { cout << "(empty)\n"; return; }
    for (auto& e : entries) {
        if (e.isDirectory())
            cout << BLU << BLD << e.fileName << "/" << R << "\n";
        else
            cout << e.fileName << "\n";
    }
}

void FileSystemManager::ll() {
    auto entries = collectEntries(currentTree);
    if (entries.empty()) { cout << "(empty)\n"; return; }

    cout << BLD
         << left  << setw(12) << "Type"
         << setw(30) << "Name"
         << setw(12) << "Size"
         << setw(12) << "Permissions"
         << "Modified\n" << R;
    Utils::printSeparator(80, '-');

    for (auto& e : entries) {
        string type = e.isDirectory() ? "dir" : "file";
        string name = e.fileName + (e.isDirectory() ? "/" : "");
        cout << left << setw(12) << type
             << setw(30) << name
             << setw(12) << e.getFormattedSize()
             << setw(12) << e.permissions
             << Utils::formatTimestamp(e.modifiedAt) << "\n";
    }
}

void FileSystemManager::lsSort(const string& by) {
    auto entries = collectEntries(currentTree);
    if (entries.empty()) { cout << "(empty)\n"; return; }

    if (by == "size") {
        sort(entries.begin(), entries.end(),
             [](const FileMetadata& a, const FileMetadata& b){ return a.fileSize < b.fileSize; });
    } else if (by == "time") {
        sort(entries.begin(), entries.end(),
             [](const FileMetadata& a, const FileMetadata& b){ return a.modifiedAt < b.modifiedAt; });
    } else {
        sort(entries.begin(), entries.end(),
             [](const FileMetadata& a, const FileMetadata& b){ return a.fileName < b.fileName; });
    }

    cout << BLD << "Sorted by: " << by << "\n" << R;
    for (auto& e : entries) {
        if (e.isDirectory()) cout << BLU << BLD << e.fileName << "/" << R << "\n";
        else cout << e.fileName << "\n";
    }
}

void FileSystemManager::tree(const string& /*path*/, int /*depth*/) {
    // BFS over all directories under currentDir
    queue<pair<string,int>> q;
    q.push(make_pair(currentDir, 0));

    while (!q.empty()) {
        pair<string,int> front = q.front(); q.pop();
        string dir = front.first;
        int depth = front.second;

        if (depth > 0) {
            string indent = Utils::repeatString("│   ", depth - 1) + "├── ";
            string dname  = Utils::getFileName(dir);
            cout << indent << BLU << BLD << dname << "/" << R << "\n";
        } else {
            cout << BLD << dir << R << "\n";
        }

        if (!directoryMap.count(dir)) continue;
        auto entries = collectEntries(directoryMap[dir]);

        for (auto& e : entries) {
            string indent = Utils::repeatString("│   ", depth) + "├── ";
            if (e.isDirectory()) {
                string childPath = dir + "/" + e.fileName;
                q.push(make_pair(childPath, depth + 1));
            } else {
                cout << indent << e.fileName << "\n";
            }
        }
    }
}

// ─────────────────────────────────────────────
// CREATE
// ─────────────────────────────────────────────
void FileSystemManager::mkdir(const string& name) {
    if (!Utils::isValidFileName(name)) {
        cout << RED << "Error: invalid directory name '" << name << "'\n" << R;
        return;
    }
    logger.logOperation("MKDIR", currentDir, name);

    FileMetadata ex;
    if (currentTree->search(name, ex)) {
        cout << YEL << "'" << name << "' already exists\n" << R;
        return;
    }

    string newPath = currentDir + "/" + name;
    directoryMap[newPath] = new BPlusTree<string, FileMetadata>(treeOrder);

    FileMetadata meta(name, EntryType::DIRECTORY, 0, "rwxr-xr-x", newPath);
    currentTree->insert(name, meta);
    logger.commit();
    cout << GRN << "Directory '" << name << "' created at " << newPath << "\n" << R;
}

void FileSystemManager::mkdirp(const string& path) {
    auto parts = Utils::splitPath(path);
    string cur = currentDir;

    for (auto& part : parts) {
        string newPath = cur + "/" + part;
        if (!directoryMap.count(newPath)) {
            directoryMap[newPath] = new BPlusTree<string, FileMetadata>(treeOrder);
            FileMetadata meta(part, EntryType::DIRECTORY, 0, "rwxr-xr-x", newPath);
            directoryMap[cur]->insert(part, meta);
            cout << GRN << "Created: " << newPath << "\n" << R;
        }
        cur = newPath;
    }
}

void FileSystemManager::touch(const string& name, size_t size, const string& perms) {
    if (!Utils::isValidFileName(name)) {
        cout << RED << "Error: invalid file name '" << name << "'\n" << R;
        return;
    }
    logger.logOperation("INSERT", currentDir, name);

    FileMetadata ex;
    if (currentTree->search(name, ex)) {
        ex.modifiedAt = time(nullptr);
        currentTree->insert(name, ex);
        logger.commit();
        cout << YEL << "Updated timestamp: " << name << "\n" << R;
        return;
    }

    string filePath = buildPath(name);
    FileMetadata meta(name, EntryType::FILE, size, perms, filePath);
    currentTree->insert(name, meta);
    logger.commit();
    cout << GRN << "File '" << name << "' created";
    if (size > 0) cout << " (" << Utils::formatSize(size) << ")";
    cout << "\n" << R;
}

// ─────────────────────────────────────────────
// DELETE
// ─────────────────────────────────────────────
void FileSystemManager::rm(const string& name) {
    FileMetadata meta;
    if (!currentTree->search(name, meta)) {
        cout << RED << "Error: '" << name << "' not found\n" << R;
        return;
    }
    logger.logOperation("DELETE", currentDir, name);

    if (meta.isDirectory()) {
        string dirPath = currentDir + "/" + name;
        if (directoryMap.count(dirPath)) {
            // Recursively remove children
            auto children = collectEntries(directoryMap[dirPath]);
            for (auto& c : children) {
                if (c.isDirectory()) directoryMap[dirPath]->remove(c.fileName);
            }
            delete directoryMap[dirPath];
            directoryMap.erase(dirPath);
        }
    }
    currentTree->remove(name);
    logger.commit();
    cout << GRN << "Deleted '" << name << "'\n" << R;
}

void FileSystemManager::rmAll() {
    auto entries = collectEntries(currentTree);
    if (entries.empty()) { cout << "(already empty)\n"; return; }
    for (auto& e : entries) rm(e.fileName);
    cout << GRN << "Deleted all entries in " << currentDir << "\n" << R;
}

// ─────────────────────────────────────────────
// COPY / MOVE / RENAME
// ─────────────────────────────────────────────
void FileSystemManager::mv(const string& name, const string& dest) {
    FileMetadata meta;
    if (!currentTree->search(name, meta)) {
        cout << RED << "Error: '" << name << "' not found\n" << R;
        return;
    }
    string destPath = resolvePath(dest);
    if (!directoryMap.count(destPath)) {
        cout << RED << "Error: destination '" << dest << "' does not exist\n" << R;
        return;
    }
    meta.absolutePath = destPath + "/" + name;
    directoryMap[destPath]->insert(name, meta);
    currentTree->remove(name);
    cout << GRN << "Moved '" << name << "' → " << destPath << "\n" << R;
}

void FileSystemManager::cp(const string& name, const string& dest) {
    FileMetadata meta;
    if (!currentTree->search(name, meta)) {
        cout << RED << "Error: '" << name << "' not found\n" << R;
        return;
    }
    string destPath = resolvePath(dest);
    if (!directoryMap.count(destPath)) {
        cout << RED << "Error: destination '" << dest << "' does not exist\n" << R;
        return;
    }
    meta.absolutePath = destPath + "/" + name;
    meta.createdAt = time(nullptr);
    directoryMap[destPath]->insert(name, meta);
    cout << GRN << "Copied '" << name << "' → " << destPath << "\n" << R;
}

void FileSystemManager::rename(const string& oldName, const string& newName) {
    if (!Utils::isValidFileName(newName)) {
        cout << RED << "Error: invalid name '" << newName << "'\n" << R;
        return;
    }
    FileMetadata meta;
    if (!currentTree->search(oldName, meta)) {
        cout << RED << "Error: '" << oldName << "' not found\n" << R;
        return;
    }
    currentTree->remove(oldName);
    meta.fileName    = newName;
    meta.absolutePath = currentDir + "/" + newName;
    meta.modifiedAt  = time(nullptr);
    currentTree->insert(newName, meta);
    cout << GRN << "Renamed '" << oldName << "' → '" << newName << "'\n" << R;
}

// ─────────────────────────────────────────────
// SEARCH
// ─────────────────────────────────────────────
void FileSystemManager::find(const string& targetName) {
    bool found = false;
    for (auto& p : directoryMap) {
        string dir = p.first;
        auto tree = p.second;
        FileMetadata meta;
        if (tree->search(targetName, meta)) {
            cout << GRN << "Found: " << meta.absolutePath << R
                 << "  [" << meta.getTypeString() << "]\n";
            found = true;
        }
    }
    if (!found) cout << YEL << "Not found: " << targetName << "\n" << R;
}

void FileSystemManager::findExt(const string& ext) {
    bool found = false;
    for (auto& p : directoryMap) {
        string dir = p.first;
        auto tree = p.second;
        auto entries = collectEntries(tree);
        for (auto& e : entries) {
            if (e.isFile() && Utils::getExtension(e.fileName) == ext) {
                cout << GRN << e.absolutePath << R << "\n";
                found = true;
            }
        }
    }
    if (!found) cout << YEL << "No files with extension '" << ext << "' found\n" << R;
}

void FileSystemManager::search(const string& keyword) {
    string kw = Utils::toLower(keyword);
    bool found = false;
    for (auto& p : directoryMap) {
        string dir = p.first;
        auto tree = p.second;
        auto entries = collectEntries(tree);
        for (auto& e : entries) {
            if (Utils::toLower(e.fileName).find(kw) != string::npos) {
                cout << GRN << e.absolutePath << R
                     << "  [" << e.getTypeString() << "]\n";
                found = true;
            }
        }
    }
    if (!found) cout << YEL << "No matches for keyword '" << keyword << "'\n" << R;
}

// ─────────────────────────────────────────────
// FILE INFO
// ─────────────────────────────────────────────
void FileSystemManager::stat(const string& name) {
    FileMetadata meta;
    if (!currentTree->search(name, meta)) {
        cout << RED << "Error: '" << name << "' not found\n" << R;
        return;
    }
    Utils::printSeparator(50, '-');
    cout << BLD << "Name        : " << R << meta.fileName << "\n";
    cout << BLD << "Type        : " << R << meta.getTypeString() << "\n";
    cout << BLD << "Size        : " << R << meta.getFormattedSize() << "\n";
    cout << BLD << "Permissions : " << R << meta.permissions << "\n";
    cout << BLD << "Path        : " << R << meta.absolutePath << "\n";
    cout << BLD << "Created     : " << R << Utils::formatTimestamp(meta.createdAt)  << "\n";
    cout << BLD << "Modified    : " << R << Utils::formatTimestamp(meta.modifiedAt) << "\n";
    Utils::printSeparator(50, '-');
}

void FileSystemManager::fileCount() {
    auto entries = collectEntries(currentTree);
    int files = 0, dirs = 0;
    for (auto& e : entries) {
        if (e.isFile()) files++;
        else            dirs++;
    }
    cout << BLD << "Files: " << R << files
         << "  " << BLD << "Dirs: " << R << dirs
         << "  " << BLD << "Total: " << R << (files + dirs) << "\n";
}

void FileSystemManager::du() {
    size_t total = 0;
    auto entries = collectEntries(currentTree);
    for (auto& e : entries) total += e.fileSize;
    cout << BLD << "Disk usage for " << currentDir << ": " << R
         << Utils::formatSize(total) << "\n";
}

// ─────────────────────────────────────────────
// PERMISSIONS
// ─────────────────────────────────────────────
void FileSystemManager::chmod(const string& name, const string& perms) {
    if (!Utils::isValidPermissions(perms)) {
        cout << RED << "Error: invalid permission string '" << perms
             << "' (expected 9 chars like rwxr-xr-x)\n" << R;
        return;
    }
    FileMetadata meta;
    if (!currentTree->search(name, meta)) {
        cout << RED << "Error: '" << name << "' not found\n" << R;
        return;
    }
    meta.permissions = perms;
    meta.modifiedAt  = time(nullptr);
    currentTree->insert(name, meta);
    cout << GRN << "Permissions of '" << name << "' set to " << perms << "\n" << R;
}

// ─────────────────────────────────────────────
// SAVE / LOAD
// ─────────────────────────────────────────────
void FileSystemManager::save() {
    Serializer::saveState(directoryMap);
    logger.clearLog();
    cout << GRN << "Saved successfully\n" << R;
}

void FileSystemManager::load() {
    Serializer::loadState(directoryMap);
    if (directoryMap.count(currentDir))
        currentTree = directoryMap[currentDir];
    else {
        currentDir  = "/root";
        currentTree = directoryMap["/root"];
    }
    cout << GRN << "Loaded successfully\n" << R;
}

// ─────────────────────────────────────────────
// CLEAR SCREEN
// ─────────────────────────────────────────────
void FileSystemManager::cls() {
#ifdef _WIN32
    (void)system("cls");
#else
    (void)system("clear");
#endif
}

// ─────────────────────────────────────────────
// DISPLAY B+ TREE (BFS level order)
// ─────────────────────────────────────────────
void FileSystemManager::displayCurrentTree() {
    cout << "\n--- B+ TREE STRUCTURE ---\n";
    auto root = currentTree->getRoot();
    if (!root) { cout << "(empty)\n"; return; }

    queue<BPlusNode<string, FileMetadata>*> q;
    q.push(root);

    while (!q.empty()) {
        int sz = (int)q.size();
        while (sz--) {
            auto node = q.front(); q.pop();
            cout << "[ ";
            for (auto& k : node->keys) cout << k << " ";
            cout << "] ";
            if (!node->isLeaf)
                for (auto child : node->children) q.push(child);
        }
        cout << "\n";
    }
}

// ─────────────────────────────────────────────
// BENCHMARK
// ─────────────────────────────────────────────
void FileSystemManager::benchmark(int n) {
    cout << "Running benchmark for " << n << " files...\n";
    auto start = chrono::high_resolution_clock::now();
    for (int i = 0; i < n; i++) {
        string name = "bench_" + to_string(i) + ".tmp";
        touch(name, (size_t)(i * 100));
    }
    auto end  = chrono::high_resolution_clock::now();
    double ms = chrono::duration<double, milli>(end - start).count();
    cout << GRN << "Inserted " << n << " files in "
         << fixed << setprecision(3) << ms << " ms\n" << R;
}

// ─────────────────────────────────────────────
// ACCESS
// ─────────────────────────────────────────────
unordered_map<string, BPlusTree<string, FileMetadata>*>&
FileSystemManager::getDirectoryMap() {
    return directoryMap;
}

// ─────────────────────────────────────────────
// WAL COMMANDS — routed through the single global logger
// ─────────────────────────────────────────────
void FileSystemManager::walLog() {
    logger.printLogFormatted();
}

void FileSystemManager::walCount() {
    logger.countOperations();
}

void FileSystemManager::walUncommitted() {
    logger.printUncommitted();
}
