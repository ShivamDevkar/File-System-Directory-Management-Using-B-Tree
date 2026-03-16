#include <iostream>
#include "BPlusTree.hpp"
using namespace std;

// g++ src/main.cpp -Iinclude -o main
// after this next commmand is .\main  or ./main (check which works)

int main()
{
    BPlusTree<int, int> tree;
    tree.insert(10, 100);
    tree.insert(20, 200);
    tree.insert(5, 50);

    tree.displayLeaf();
    return 0;
}