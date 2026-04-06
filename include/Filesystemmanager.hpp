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

    std::string currentDir;
    BPlusTree<std::string, FileMetadata>* currentTree;
    std::stack<std::string> dirHistory;
    int treeOrder;

    // ── PRIVATE HELPERS ──────────────────────────────────────
    std::string buildPath(const std::string& name) const;
    void findInTree(const std::string& dirPath, const std::string& targetName, std::vector<std::string>& results);
    void findByExtInTree(const std::string& dirPath, const std::string& ext, std::vector<std::string>& results);
    void searchKeywordInTree(const std::string& dirPath, const std::string& keyword, std::vector<std::string>& results);
    void removeDirectoryRecursive(const std::string& dirPath);
    void treeHelper(const std::string& dirPath, int depth);
    void duHelper(const std::string& dirPath, size_t& total, int& fileCount, int& dirCount);

public:

    FileSystemManager(int order = 4);
    ~FileSystemManager();

    // ── NAVIGATION ───────────────────────────────────────────
    void cd(const std::string& path);
    void pwd();
    void back();                                            // go back to previous dir (cd -)

    // ── LISTING ──────────────────────────────────────────────
    void ls(bool showHidden = false);
    void ll();                                              // detailed listing with all metadata
    void lsSort(const std::string& by);                    // sort by: name / size / time
    void tree(const std::string& path = "", int depth = 0);

    // ── CREATE ───────────────────────────────────────────────
    void mkdir(const std::string& name);
    void mkdirp(const std::string& path);                   // create nested dirs at once
    void touch(const std::string& name, size_t size = 0, const std::string& perms = "rw-r--r--");

    // ── DELETE ───────────────────────────────────────────────
    void rm(const std::string& name);
    void rmAll();                                           // delete everything in current dir

    // ── COPY / MOVE / RENAME ─────────────────────────────────
    void mv(const std::string& name, const std::string& destDir);
    void cp(const std::string& name, const std::string& destDir);
    void rename(const std::string& oldName, const std::string& newName);

    // ── SEARCH ───────────────────────────────────────────────
    void find(const std::string& targetName);
    void findExt(const std::string& ext);                   // find all .pdf, .txt etc
    void search(const std::string& keyword);                // find files containing keyword

    // ── FILE INFO ────────────────────────────────────────────
    void stat(const std::string& name);                     // full metadata of one file
    void fileCount();                                       // count files and dirs
    void du();                                              // total disk usage of current dir

    // ── PERMISSIONS ──────────────────────────────────────────
    void chmod(const std::string& name, const std::string& perms);

    // ── UTILITY ──────────────────────────────────────────────
    void cls();                                             // clear screen
    void history();                                         // show navigation history

    // ── DEBUG / PRESENTATION ─────────────────────────────────
    void displayCurrentTree();
    void benchmark(int n);                                  // speed test: insert n files

    // ── ACCESSORS ────────────────────────────────────────────
    std::unordered_map<std::string, BPlusTree<std::string, FileMetadata>*>& getDirectoryMap();
    std::string getCurrentDir() const;
    void setCurrentDir(const std::string& dir);
};