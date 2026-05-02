#pragma once
#include <fstream>
#include <string>
#include <vector>
#include <ctime>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdlib>   // for system()

class WALLogger {
private:
    std::string logPath;
    std::ofstream logFile;

public:

    // ✅ FIXED CONSTRUCTOR
    WALLogger(const std::string& path = "logs/wal.log")
        : logPath(path)
    {
        // ✅ Ensure logs folder exists (VERY IMPORTANT)
#ifdef _WIN32
        system("mkdir logs >nul 2>&1");
#else
        system("mkdir -p logs");
#endif

        logFile.open(logPath, std::ios::out | std::ios::app);

        if (!logFile.is_open()) {
            std::cerr << "[WALLogger] ERROR: Cannot open log file: "
                      << logPath << "\n";
        } else {
            std::cout << "[WALLogger] Log file ready: " << logPath << "\n";
        }
    }

    ~WALLogger() {
        if (logFile.is_open()) {
            logFile.flush();
            logFile.close();
        }
    }

    // ✅ Log operation
    void logOperation(const std::string& operation,
                      const std::string& directory,
                      const std::string& target)
    {
        if (!logFile.is_open()) return;

        time_t now = time(nullptr);

        logFile << now << " "
                << operation << " "
                << directory << " "
                << target << "\n";

        logFile.flush();
    }

    // ✅ Commit
    void commit() {
        if (!logFile.is_open()) return;

        time_t now = time(nullptr);
        logFile << now << " COMMIT\n";
        logFile.flush();
    }

    // ✅ Recovery check
    bool recoverIfNeeded() {

        if (logFile.is_open()) {
            logFile.flush();
            logFile.close();
        }

        std::ifstream reader(logPath);
        if (!reader.is_open()) {
            logFile.open(logPath, std::ios::out | std::ios::app);
            return false;
        }

        std::vector<std::string> lines;
        std::string line;

        while (std::getline(reader, line)) {
            if (!line.empty()) lines.push_back(line);
        }

        reader.close();
        logFile.open(logPath, std::ios::out | std::ios::app);

        if (lines.empty()) return false;

        std::string last = lines.back();

        if (last.find("COMMIT") == std::string::npos) {
            std::cout << "\n[WAL] ⚠ Crash detected\n";
            std::cout << "[WAL] Last operation incomplete:\n";
            std::cout << "      " << last << "\n\n";
            return true;
        }

        return false;
    }

    // ✅ Clear log
    void clearLog() {
        if (logFile.is_open()) logFile.close();

        std::ofstream wipe(logPath, std::ios::trunc);
        wipe.close();

        logFile.open(logPath, std::ios::out | std::ios::app);

        std::cout << "[WAL] Log cleared\n";
    }

    // ✅ Print log (for debugging)
    void printLog() {

        if (logFile.is_open()) {
            logFile.flush();
            logFile.close();
        }

        std::ifstream reader(logPath);

        if (!reader.is_open()) {
            std::cout << "[WAL] No log file found\n";
            logFile.open(logPath, std::ios::app);
            return;
        }

        std::cout << "\n=== WAL LOG ===\n";

        std::string line;
        while (std::getline(reader, line)) {
            std::cout << line << "\n";
        }

        std::cout << "===============\n\n";

        reader.close();
        logFile.open(logPath, std::ios::app);
    }

    // ✅ Print log in formatted table (used by 'wallog' command)
    void printLogFormatted() {
        if (logFile.is_open()) { logFile.flush(); logFile.close(); }

        std::ifstream reader(logPath);
        if (!reader.is_open()) {
            std::cout << "[WAL] No log file found at: " << logPath << "\n";
            logFile.open(logPath, std::ios::app);
            return;
        }

        std::vector<std::string> lines;
        std::string line;
        while (std::getline(reader, line))
            if (!line.empty()) lines.push_back(line);
        reader.close();
        logFile.open(logPath, std::ios::app);

        if (lines.empty()) {
            std::cout << "[WAL] Log is empty.\n";
            return;
        }

        std::cout << "\n";
        std::cout << "+------------+------------------+---------------------+------------------+\n";
        std::cout << "| Timestamp  | Operation        | Directory           | Target           |\n";
        std::cout << "+------------+------------------+---------------------+------------------+\n";

        for (auto& l : lines) {
            std::istringstream ss(l);
            std::string ts, op, dir, target;
            ss >> ts >> op;
            if (op == "COMMIT") {
                std::cout << "| " << std::left << std::setw(10) << ts
                          << " | " << std::setw(16) << "--- COMMIT ---"
                          << " |                     |                  |\n";
            } else {
                ss >> dir >> target;
                std::cout << "| " << std::left << std::setw(10) << ts
                          << " | " << std::setw(16) << op
                          << " | " << std::setw(19) << dir
                          << " | " << std::setw(16) << target << " |\n";
            }
        }
        std::cout << "+------------+------------------+---------------------+------------------+\n\n";
    }

    // ✅ Count total operations logged (used by 'walcount' command)
    void countOperations() {
        if (logFile.is_open()) { logFile.flush(); logFile.close(); }

        std::ifstream reader(logPath);
        if (!reader.is_open()) {
            std::cout << "[WAL] No log file found.\n";
            logFile.open(logPath, std::ios::app);
            return;
        }

        int total = 0, commits = 0, inserts = 0, deletes = 0,
            mkdirs = 0, rmdirs = 0, renames = 0, chmods = 0, others = 0;

        std::string line;
        while (std::getline(reader, line)) {
            if (line.empty()) continue;
            total++;
            std::istringstream ss(line);
            std::string ts, op;
            ss >> ts >> op;
            if      (op == "COMMIT") commits++;
            else if (op == "INSERT") inserts++;
            else if (op == "DELETE") deletes++;
            else if (op == "MKDIR")  mkdirs++;
            else if (op == "RMDIR")  rmdirs++;
            else if (op == "RENAME") renames++;
            else if (op == "CHMOD")  chmods++;
            else                     others++;
        }
        reader.close();
        logFile.open(logPath, std::ios::app);

        std::cout << "\n[WAL] Operation Count:\n";
        std::cout << "  Total lines   : " << total   << "\n";
        std::cout << "  COMMITs       : " << commits << "\n";
        std::cout << "  INSERTs       : " << inserts << "\n";
        std::cout << "  DELETEs       : " << deletes << "\n";
        std::cout << "  MKDIRs        : " << mkdirs  << "\n";
        std::cout << "  RMDIRs        : " << rmdirs  << "\n";
        std::cout << "  RENAMEs       : " << renames << "\n";
        std::cout << "  CHMODs        : " << chmods  << "\n";
        std::cout << "  Other         : " << others  << "\n";
        std::cout << "  Operations    : " << (total - commits) << "\n\n";
    }

    // ✅ Print uncommitted operations (used by 'waluncommitted' command)
    void printUncommitted() {
        if (logFile.is_open()) { logFile.flush(); logFile.close(); }

        std::ifstream reader(logPath);
        if (!reader.is_open()) {
            std::cout << "[WAL] No log file found.\n";
            logFile.open(logPath, std::ios::app);
            return;
        }

        std::vector<std::string> lines;
        std::string line;
        while (std::getline(reader, line))
            if (!line.empty()) lines.push_back(line);
        reader.close();
        logFile.open(logPath, std::ios::app);

        std::vector<std::string> uncommitted;
        for (auto& l : lines) {
            std::istringstream ss(l);
            std::string ts, op;
            ss >> ts >> op;
            if (op != "COMMIT") {
                uncommitted.push_back(l);
            } else {
                // COMMIT covers the last pending operation
                if (!uncommitted.empty()) uncommitted.pop_back();
            }
        }

        if (uncommitted.empty()) {
            std::cout << "[WAL] ✅ No uncommitted operations. Log is clean.\n";
        } else {
            std::cout << "\n[WAL] ⚠ Uncommitted operations (" << uncommitted.size() << "):\n";
            for (auto& u : uncommitted)
                std::cout << "  " << u << "\n";
            std::cout << "\n";
        }
    }
};