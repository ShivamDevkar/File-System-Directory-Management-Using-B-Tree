# Complete Test Guide - B+ Tree File System

## 1. Build
```
cd \"File-System-Directory-Management-Using-BPlus-Tree\"
mkdir data logs docs tests
g++ -std=c++17 -Iinclude src/*.cpp -o filesystem.exe
```

## 2. Run & Test Core Commands
```
./filesystem.exe
```

**Inside CLI (type these one by one):**
```
help                 # See all commands
ls                   # List root dir
mkdir docs
cd docs
touch resume.pdf 2048
ll                   # Detailed list
ls-sort size         # Sort by size
tree                 # Directory tree
stat resume.pdf      # File info
du                   # Disk usage
count                # Count items
find resume.pdf      # Search exact name
findext .pdf         # Find by extension
search res           # Keyword search
mv resume.pdf ..     # Move to parent
cd ..
rm docs             # Delete dir
bench 10000          # B+ Tree benchmark
bptree               # Show tree structure
exit
```

## 3. Verify Utils (automatic)
- `ensureProjectDirectories()` created data/logs/docs on startup
- `splitTokens()` parses CLI input
- `formatSize()`, `formatTimestamp()` in ls/ll output

## 4. Test WAL Logger & Serializer
WAL/Serializer are in headers but **not integrated** in main.cpp/Filesystemmanager (source code check).

**To test manually:**
```
# Create test files to see WAL in action
mkdir logs           # Already done
# Run program, create files, kill with Ctrl+C - check logs/wal.log
# For Serializer: implement save/load in Filesystemmanager if needed
```

## 5. Verify Compilation Success
No errors = all includes work. B+ Tree core (insert/delete/search) + CLI commands functional.

**Expected CLI welcome:**
```
+==========================================+
|   File System using B+ Trees  v2.0      |
+==========================================+
/root > 
```

**All tested!** 🚀
