#pragma once

#include <fstream>
#include <string>
#include <vector>
#include <queue>
#include <unordered_map>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <cstdint>
#include <sys/stat.h>

#include "BPlusTree.hpp"
#include "FileMetadata.hpp"

class Serializer {
public:

    // ───────────────── SAVE ─────────────────
    static bool saveState(
        const std::unordered_map<std::string,
              BPlusTree<std::string, FileMetadata>*>& dirMap,
        const std::string& filePath = "data/filesystem.dat")
    {
        // NEW: Backup old save file before overwriting
        backupSaveFile(filePath);

        std::ofstream out(filePath, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            std::cerr << "[Serializer] ERROR: Cannot open file: " << filePath << "\n";
            return false;
        }

        int numDirs = (int)dirMap.size();
        out.write((char*)&numDirs, sizeof(int));

        for (const auto& pair : dirMap) {
            const std::string& dirPath = pair.first;
            auto tree = pair.second;

            writeString(out, dirPath);

            int order = tree->getOrder();
            out.write((char*)&order, sizeof(int));

            std::vector<BPlusNode<std::string, FileMetadata>*> nodeList;
            std::unordered_map<BPlusNode<std::string, FileMetadata>*, int> indexMap;

            auto root = tree->getRoot();

            if (root) {
                std::queue<BPlusNode<std::string, FileMetadata>*> q;
                q.push(root);

                while (!q.empty()) {
                    auto node = q.front(); q.pop();

                    if (indexMap.count(node)) continue;

                    indexMap[node] = nodeList.size();
                    nodeList.push_back(node);

                    if (!node->isLeaf) {
                        for (auto child : node->children) {
                            if (child) q.push(child);
                        }
                    }
                }
            }

            int nodeCount = (int)nodeList.size();
            out.write((char*)&nodeCount, sizeof(int));

            for (auto node : nodeList) {
                writeNode(out, node, indexMap);
            }
        }

        out.close();
        std::cout << "[Serializer] Saved successfully\n";
        return true;
    }

    // ───────────────── LOAD ─────────────────
    static bool loadState(
        std::unordered_map<std::string,
              BPlusTree<std::string, FileMetadata>*>& dirMap,
        const std::string& filePath = "data/filesystem.dat")
    {
        std::ifstream in(filePath, std::ios::binary);
        if (!in.is_open()) {
            std::cout << "[Serializer] No save file found.\n";
            return false;
        }

        for (auto& p : dirMap) delete p.second;
        dirMap.clear();

        int numDirs = 0;
        in.read((char*)&numDirs, sizeof(int));

        for (int d = 0; d < numDirs; d++) {

            std::string dirPath = readString(in);

            int order;
            in.read((char*)&order, sizeof(int));

            int nodeCount;
            in.read((char*)&nodeCount, sizeof(int));

            std::vector<BPlusNode<std::string, FileMetadata>*> nodes(nodeCount);
            std::vector<std::vector<int>> children(nodeCount);
            std::vector<int> nextIdx(nodeCount, -1);

            for (int i = 0; i < nodeCount; i++) {

                char isLeafByte;
                in.read(&isLeafByte, sizeof(char));

                bool isLeaf = (isLeafByte == 1);
                auto node = new BPlusNode<std::string, FileMetadata>(isLeaf);
                nodes[i] = node;

                int keyCount;
                in.read((char*)&keyCount, sizeof(int));

                for (int k = 0; k < keyCount; k++) {
                    node->keys.push_back(readString(in));

                    if (isLeaf) {
                        node->values.push_back(readMetadata(in));
                    }
                }

                int childCount;
                in.read((char*)&childCount, sizeof(int));

                for (int c = 0; c < childCount; c++) {
                    int idx;
                    in.read((char*)&idx, sizeof(int));
                    children[i].push_back(idx);
                }

                int nxt;
                in.read((char*)&nxt, sizeof(int));
                nextIdx[i] = nxt;
            }

            // reconnect pointers
            for (int i = 0; i < nodeCount; i++) {
                auto node = nodes[i];

                for (int idx : children[i]) {
                    if (idx >= 0 && idx < nodeCount) {
                        node->children.push_back(nodes[idx]);
                        nodes[idx]->parent = node;
                    }
                }

                if (nextIdx[i] >= 0 && nextIdx[i] < nodeCount) {
                    node->next = nodes[nextIdx[i]];
                }
            }

            auto tree = buildTreeFromNodes(nodes, order);
            dirMap[dirPath] = tree;
        }

        in.close();
        std::cout << "[Serializer] Loaded successfully\n";
        return true;
    }

    // ───────────────── NEW: EXPORT TO TEXT FILE ─────────────────
    // Saves entire file system as human-readable text
    static void exportToText(
        const std::unordered_map<std::string,
              BPlusTree<std::string, FileMetadata>*>& dirMap,
        const std::string& outPath = "docs/filesystem_export.txt")
    {
        std::ofstream out(outPath);
        if (!out.is_open()) {
            std::cerr << "[Serializer] ERROR: Cannot create export file\n";
            return;
        }

        time_t now = time(nullptr);
        out << "============================================\n";
        out << "  FILE SYSTEM EXPORT\n";
        out << "  Generated: " << ctime(&now);
        out << "  Total Directories: " << dirMap.size() << "\n";
        out << "============================================\n\n";

        for (const auto& pair : dirMap) {
            out << "DIRECTORY: " << pair.first << "\n";
            out << std::string(40, '-') << "\n";

            auto tree = pair.second;
            auto leaf = tree->getRoot();

            if (!leaf) {
                out << "  (empty)\n\n";
                continue;
            }

            while (!leaf->isLeaf) leaf = leaf->children[0];

            int fileCount = 0;
            int dirCount  = 0;

            while (leaf) {
                for (int i = 0; i < (int)leaf->keys.size(); i++) {
                    auto& m = leaf->values[i];

                    std::string type = (m.entryType == EntryType::DIRECTORY)
                                       ? "[DIR] " : "[FILE]";

                    if (m.entryType == EntryType::DIRECTORY) dirCount++;
                    else fileCount++;

                    out << "  " << type << " "
                        << std::left << std::setw(25) << m.fileName
                        << "  " << std::setw(10) << m.getFormattedSize()
                        << "  " << m.permissions << "\n";
                }
                leaf = leaf->next;
            }

            out << std::string(40, '-') << "\n";
            out << "  Files: " << fileCount
                << "  Directories: " << dirCount << "\n\n";
        }

        out.close();
        std::cout << "[Serializer] Exported to: " << outPath << "\n";
    }

    // ───────────────── NEW: SHOW SAVE FILE INFO ─────────────────
    static void showSaveInfo(
        const std::string& filePath = "data/filesystem.dat")
    {
        struct stat st;
        if (stat(filePath.c_str(), &st) != 0) {
            std::cout << "[Serializer] No save file found at: " << filePath << "\n";
            return;
        }

        double sizeKB = st.st_size / 1024.0;
        time_t modTime = st.st_mtime;

        std::cout << "\n[Serializer] Save File Info:\n";
        std::cout << "  Path      : " << filePath << "\n";
        std::cout << "  Size      : " << std::fixed << std::setprecision(2)
                  << sizeKB << " KB (" << st.st_size << " bytes)\n";
        std::cout << "  Last Saved: " << ctime(&modTime);
    }

    // ───────────────── NEW: VERIFY SAVE FILE ─────────────────
    // Checks if save file is valid and not corrupted
    static bool verifySaveFile(
        const std::string& filePath = "data/filesystem.dat")
    {
        std::ifstream in(filePath, std::ios::binary);
        if (!in.is_open()) {
            std::cout << "[Serializer] ❌ Save file not found\n";
            return false;
        }

        int numDirs = 0;
        in.read((char*)&numDirs, sizeof(int));

        if (in.fail() || numDirs < 0 || numDirs > 10000) {
            std::cout << "[Serializer] ❌ Save file is corrupted\n";
            in.close();
            return false;
        }

        in.close();
        std::cout << "[Serializer] ✅ Save file is valid ("
                  << numDirs << " directories)\n";
        return true;
    }

    // ───────────────── EXISTING: EXPORT REPORT ─────────────────
    static void exportReport(
        const std::unordered_map<std::string,
              BPlusTree<std::string, FileMetadata>*>& dirMap,
        const std::string& outPath = "docs/report.txt")
    {
        std::ofstream out(outPath);
        if (!out.is_open()) return;

        out << "FILE SYSTEM REPORT\n\n";

        for (const auto& pair : dirMap) {
            out << "DIR: " << pair.first << "\n";

            auto tree = pair.second;
            auto leaf = tree->getRoot();

            if (!leaf) continue;

            while (!leaf->isLeaf) leaf = leaf->children[0];

            while (leaf) {
                for (int i = 0; i < (int)leaf->keys.size(); i++) {
                    auto& m = leaf->values[i];
                    out << m.fileName << "  "
                        << m.getTypeString() << "  "
                        << m.getFormattedSize() << "\n";
                }
                leaf = leaf->next;
            }

            out << "\n";
        }

        std::cout << "[Serializer] Report saved to: " << outPath << "\n";
    }

private:

    // ───────────────── NEW: AUTO BACKUP ─────────────────
    static void backupSaveFile(const std::string& filePath) {
        std::ifstream src(filePath, std::ios::binary);
        if (!src.is_open()) return; // no file to backup yet

        std::string backupPath = filePath + ".bak";
        std::ofstream dst(backupPath, std::ios::binary | std::ios::trunc);

        dst << src.rdbuf();

        src.close();
        dst.close();
        std::cout << "[Serializer] Backup created: " << backupPath << "\n";
    }

    // ───────── STRING ─────────
    static void writeString(std::ofstream& out, const std::string& s) {
        int len = (int)s.size();
        out.write((char*)&len, sizeof(int));
        out.write(s.c_str(), len);
    }

    static std::string readString(std::ifstream& in) {
        int len;
        in.read((char*)&len, sizeof(int));

        if (len < 0 || len > 10000) return "";

        std::string s(len, '\0');
        in.read(&s[0], len);
        return s;
    }

    // ───────── METADATA ─────────
    static void writeMetadata(std::ofstream& out, const FileMetadata& m) {
        writeString(out, m.fileName);

        int type = (m.entryType == EntryType::DIRECTORY) ? 1 : 0;
        out.write((char*)&type, sizeof(int));

        uint64_t size = m.fileSize;
        out.write((char*)&size, sizeof(uint64_t));

        writeString(out, m.permissions);

        int64_t c   = m.createdAt;
        int64_t mod = m.modifiedAt;
        out.write((char*)&c,   sizeof(int64_t));
        out.write((char*)&mod, sizeof(int64_t));

        writeString(out, m.absolutePath);
    }

    static FileMetadata readMetadata(std::ifstream& in) {
        FileMetadata m;

        m.fileName = readString(in);

        int type;
        in.read((char*)&type, sizeof(int));
        m.entryType = (type == 1) ? EntryType::DIRECTORY : EntryType::FILE;

        uint64_t size;
        in.read((char*)&size, sizeof(uint64_t));
        m.fileSize = size;

        m.permissions = readString(in);

        int64_t c, mod;
        in.read((char*)&c,   sizeof(int64_t));
        in.read((char*)&mod, sizeof(int64_t));

        m.createdAt  = c;
        m.modifiedAt = mod;

        m.absolutePath = readString(in);

        return m;
    }

    // ───────── NODE ─────────
    static void writeNode(
        std::ofstream& out,
        BPlusNode<std::string, FileMetadata>* node,
        std::unordered_map<BPlusNode<std::string, FileMetadata>*, int>& indexMap)
    {
        char isLeaf = node->isLeaf ? 1 : 0;
        out.write(&isLeaf, sizeof(char));

        int keyCount = node->keys.size();
        out.write((char*)&keyCount, sizeof(int));

        for (int i = 0; i < keyCount; i++) {
            writeString(out, node->keys[i]);
            if (node->isLeaf) writeMetadata(out, node->values[i]);
        }

        int childCount = node->children.size();
        out.write((char*)&childCount, sizeof(int));

        for (auto c : node->children) {
            int idx = indexMap.count(c) ? indexMap[c] : -1;
            out.write((char*)&idx, sizeof(int));
        }

        int nxt = (node->next && indexMap.count(node->next))
                  ? indexMap[node->next] : -1;
        out.write((char*)&nxt, sizeof(int));
    }

    // ───────── BUILD TREE ─────────
    static BPlusTree<std::string, FileMetadata>* buildTreeFromNodes(
        std::vector<BPlusNode<std::string, FileMetadata>*>& nodes,
        int order)
    {
        auto tree = new BPlusTree<std::string, FileMetadata>(order);

        if (nodes.empty()) return tree;

        auto leaf = nodes[0];
        while (!leaf->isLeaf) leaf = leaf->children[0];

        while (leaf) {
            for (int i = 0; i < (int)leaf->keys.size(); i++) {
                tree->insert(leaf->keys[i], leaf->values[i]);
            }
            leaf = leaf->next;
        }

        for (auto n : nodes) {
            n->children.clear();
            n->next = nullptr;
        }

        for (auto n : nodes) delete n;
        nodes.clear();

        return tree;
    }
};