#pragma once

#include <string>
#include <stack>
#include <unordered_map>
#include <vector>
#include "BPlusTree.hpp"
#include "FileMetadata.hpp"

class FileSystemManager {

private:
    std::unordered_map<
        std::string,
        BPlusTree<std::string, FileMetadata>*
    > directoryMap;

    std::string resolvePath(const std::string& path) const;

    std::string currentDir;
    BPlusTree<std::string, FileMetadata>* currentTree;
    std::stack<std::string> dirHistory;
    int treeOrder;

    // HELPERS
    std::string buildPath(const std::string& name) const;

public:

    FileSystemManager(int order = 4);
    ~FileSystemManager();

    // NAVIGATION
    void cd(const std::string& path);
    void pwd();
    void back();

    // LISTING
    void ls(bool showHidden = false);   // ✅ ONLY ONE
    void ll();
    void lsSort(const std::string& by);
    void tree(const std::string& path = "", int depth = 0);

    // CREATE
    void mkdir(const std::string& name);
    void mkdirp(const std::string& path);
    void touch(const std::string& name, size_t size = 0, const std::string& perms = "rw-r--r--");

    // DELETE
    void rm(const std::string& name);
    void rmAll();

    // COPY / MOVE
    void mv(const std::string& name, const std::string& destDir);
    void cp(const std::string& name, const std::string& destDir);
    void rename(const std::string& oldName, const std::string& newName);

    // SEARCH
    void find(const std::string& targetName);
    void findExt(const std::string& ext);
    void search(const std::string& keyword);

    // FILE INFO
    void stat(const std::string& name);
    void fileCount();
    void du();

    // PERMISSIONS
    void chmod(const std::string& name, const std::string& perms);

    // SYSTEM
    void cls();
    void history();

    // DEBUG
    void displayCurrentTree();
    void benchmark(int n);

    // WAL
    void walLog();
    void walCount();
    void walUncommitted();

    // SAVE/LOAD
    void save();
    void load();

    // ACCESS
    std::unordered_map<std::string, BPlusTree<std::string, FileMetadata>*>& getDirectoryMap();
    std::string getCurrentDir() const;
    void setCurrentDir(const std::string& dir);
};
