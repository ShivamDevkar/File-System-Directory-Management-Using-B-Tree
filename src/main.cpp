// ─────────────────────────────────────────────────────────────
// main.cpp — Interactive CLI Shell (Full Version)
// ─────────────────────────────────────────────────────────────

#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include "FileSystemManager.hpp"

// ANSI Colors
const std::string RESET  = "\033[0m";
const std::string RED    = "\033[31m";
const std::string GREEN  = "\033[32m";
const std::string YELLOW = "\033[33m";
const std::string BLUE   = "\033[34m";
const std::string CYAN   = "\033[36m";
const std::string BOLD   = "\033[1m";

std::vector<std::string> splitTokens(const std::string& input) {
    std::vector<std::string> tokens;
    std::stringstream ss(input);
    std::string token;
    while (ss >> token) tokens.push_back(token);
    return tokens;
}

void printHelp() {
    std::cout << BOLD << CYAN
        << "\n+------------------------------------------+\n"
        << "|         FILE SYSTEM ALL COMMANDS       |\n"
        << "+------------------------------------------+\n" << RESET;

    std::cout << BOLD << YELLOW << "\n  NAVIGATION\n" << RESET;
    std::cout << "  cd <dir>            " << "Go into a directory\n";
    std::cout << "  cd ..               " << "Go up one level\n";
    std::cout << "  cd /                " << "Go to root\n";
    std::cout << "  cd -                " << "Go back to previous directory\n";
    std::cout << "  pwd                 " << "Print current directory path\n";
    std::cout << "  history             " << "Show navigation history\n";

    std::cout << BOLD << YELLOW << "\n  LISTING\n" << RESET;
    std::cout << "  ls                  " << "List files and folders\n";
    std::cout << "  ll                  " << "Detailed list with timestamps\n";
    std::cout << "  ls-sort <by>        " << "Sort list by: name / size / time\n";
    std::cout << "  tree                " << "Show full directory tree\n";

    std::cout << BOLD << YELLOW << "\n  CREATE\n" << RESET;
    std::cout << "  mkdir <name>        " << "Create a new directory\n";
    std::cout << "  mkdirp <a/b/c>      " << "Create nested directories at once\n";
    std::cout << "  touch <name> [size] " << "Create a file (size in bytes)\n";

    std::cout << BOLD << YELLOW << "\n  DELETE\n" << RESET;
    std::cout << "  rm <name>           " << "Delete file or directory\n";
    std::cout << "  rmall               " << "Delete everything in current dir\n";

    std::cout << BOLD << YELLOW << "\n  COPY / MOVE / RENAME\n" << RESET;
    std::cout << "  mv <name> <dest>    " << "Move file to another directory\n";
    std::cout << "  cp <name> <dest>    " << "Copy file to another directory\n";
    std::cout << "  rename <old> <new>  " << "Rename a file or directory\n";

    std::cout << BOLD << YELLOW << "\n  SEARCH\n" << RESET;
    std::cout << "  find <name>         " << "Find file/dir by exact name\n";
    std::cout << "  findext <.ext>      " << "Find all files with extension\n";
    std::cout << "  search <keyword>    " << "Find files whose name contains keyword\n";

    std::cout << BOLD << YELLOW << "\n  FILE INFO\n" << RESET;
    std::cout << "  stat <name>         " << "Show full metadata of a file\n";
    std::cout << "  count               " << "Count files and dirs here\n";
    std::cout << "  du                  " << "Total disk usage of current dir\n";

    std::cout << BOLD << YELLOW << "\n  PERMISSIONS\n" << RESET;
    std::cout << "  chmod <name> <perm> " << "Change permissions (e.g. rwxr-xr-x)\n";

    std::cout << BOLD << YELLOW << "\n  SYSTEM\n" << RESET;
    std::cout << "  cls                 " << "Clear the screen\n";
    std::cout << "  bptree              " << "Show B+ Tree structure (for project demo)\n";
    std::cout << "  bench <n>           " << "Benchmark: insert n files and measure speed\n";
    std::cout << "  help                " << "Show this help\n";
    std::cout << "  exit                " << "Exit\n";
    std::cout << "\n";
}

int main() {

    // Welcome banner
    std::cout << BOLD << GREEN
        << "\n+==========================================+\n"
        << "|   File System using B+ Trees  v2.0      |\n"
        << "|   DS-II Course Project                   |\n"
        << "+==========================================+\n" << RESET;
    std::cout << "Type " << YELLOW << "help" << RESET << " to see all commands.\n\n";

    FileSystemManager fs(4);
    std::string inputLine;

    while (true) {

        // Prompt: shows current directory
        std::cout << BOLD << BLUE << fs.getCurrentDir()
                  << RESET << GREEN << " > " << RESET;

        if (!std::getline(std::cin, inputLine)) break;
        if (inputLine.empty()) continue;

        std::vector<std::string> tokens = splitTokens(inputLine);
        if (tokens.empty()) continue;

        std::string cmd = tokens[0];

        // ── NAVIGATION ────────────────────────────────────────
        if (cmd == "cd") {
            if (tokens.size() < 2) std::cout << RED << "Usage: cd <directory>\n" << RESET;
            else fs.cd(tokens[1]);
        }
        else if (cmd == "pwd")     { fs.pwd(); }
        else if (cmd == "history") { fs.history(); }

        // ── LISTING ───────────────────────────────────────────
        else if (cmd == "ls")      { fs.ls(); }
        else if (cmd == "ll")      { fs.ll(); }
        else if (cmd == "ls-sort") {
            std::string by = (tokens.size() >= 2) ? tokens[1] : "name";
            fs.lsSort(by);
        }
        else if (cmd == "tree")    { fs.tree(); }

        // ── CREATE ────────────────────────────────────────────
        else if (cmd == "mkdir") {
            if (tokens.size() < 2) std::cout << RED << "Usage: mkdir <name>\n" << RESET;
            else fs.mkdir(tokens[1]);
        }
        else if (cmd == "mkdirp") {
            if (tokens.size() < 2) std::cout << RED << "Usage: mkdirp <path/to/dir>\n" << RESET;
            else fs.mkdirp(tokens[1]);
        }
        else if (cmd == "touch") {
            if (tokens.size() < 2) std::cout << RED << "Usage: touch <name> [size]\n" << RESET;
            else {
                size_t size = 0;
                if (tokens.size() >= 3) size = (size_t)std::stoul(tokens[2]);
                fs.touch(tokens[1], size);
            }
        }

        // ── DELETE ────────────────────────────────────────────
        else if (cmd == "rm") {
            if (tokens.size() < 2) std::cout << RED << "Usage: rm <name>\n" << RESET;
            else fs.rm(tokens[1]);
        }
        else if (cmd == "rmall") { fs.rmAll(); }

        // ── COPY / MOVE / RENAME ──────────────────────────────
        else if (cmd == "mv") {
            if (tokens.size() < 3) {
                std::cout << RED << "Usage: mv <filename> <dest_path>\n" << RESET;
                std::cout << RED << "Example: mv resume.pdf /root/documents\n" << RESET;
            } else fs.mv(tokens[1], tokens[2]);
        }
        else if (cmd == "cp") {
            if (tokens.size() < 3) {
                std::cout << RED << "Usage: cp <filename> <dest_path>\n" << RESET;
                std::cout << RED << "Example: cp notes.txt /root/backup\n" << RESET;
            } else fs.cp(tokens[1], tokens[2]);
        }
        else if (cmd == "rename") {
            if (tokens.size() < 3) std::cout << RED << "Usage: rename <old> <new>\n" << RESET;
            else fs.rename(tokens[1], tokens[2]);
        }

        // ── SEARCH ────────────────────────────────────────────
        else if (cmd == "find") {
            if (tokens.size() < 2) std::cout << RED << "Usage: find <name>\n" << RESET;
            else fs.find(tokens[1]);
        }
        else if (cmd == "findext") {
            if (tokens.size() < 2) std::cout << RED << "Usage: findext <.ext>  e.g. findext .pdf\n" << RESET;
            else fs.findExt(tokens[1]);
        }
        else if (cmd == "search") {
            if (tokens.size() < 2) std::cout << RED << "Usage: search <keyword>\n" << RESET;
            else fs.search(tokens[1]);
        }

        // ── FILE INFO ─────────────────────────────────────────
        else if (cmd == "stat") {
            if (tokens.size() < 2) std::cout << RED << "Usage: stat <name>\n" << RESET;
            else fs.stat(tokens[1]);
        }
        else if (cmd == "count")   { fs.fileCount(); }
        else if (cmd == "du")      { fs.du(); }

        // ── PERMISSIONS ───────────────────────────────────────
        else if (cmd == "chmod") {
            if (tokens.size() < 3) std::cout << RED << "Usage: chmod <name> <permissions>\n" << RESET;
            else fs.chmod(tokens[1], tokens[2]);
        }

        // ── SYSTEM ────────────────────────────────────────────
        else if (cmd == "cls" || cmd == "clear") { fs.cls(); }
        else if (cmd == "bptree")  { fs.displayCurrentTree(); }
        else if (cmd == "bench") {
            int n = 1000;
            if (tokens.size() >= 2) n = std::stoi(tokens[1]);
            fs.benchmark(n);
        }
        else if (cmd == "help")    { printHelp(); }
        else if (cmd == "exit" || cmd == "quit") {
            std::cout << GREEN << "Goodbye!\n" << RESET;
            break;
        }
        else {
            std::cout << RED << "Unknown command: '" << cmd << "'\n" << RESET;
            std::cout << "Type " << YELLOW << "help" << RESET << " to see all commands.\n";
        }
    }

    return 0;
}