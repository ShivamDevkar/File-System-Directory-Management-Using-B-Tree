#pragma once

#include<vector>
using namespace std;

template <typename KeyType, typename ValueType>
class BPlusNode {
public:
//  FIELDS USED BY BOTH INTERNAL AND LEAF NODES
    bool isLeaf;
    BPlusNode* parent;
    vector<KeyType> keys;

    // FIELDS USED BY LEAF NODES ONLY
    // values[i] is the data paired with keys[i]
    // next points to the next leaf in the chain
    BPlusNode* next;
    vector<ValueType> values; // MetaData

    // FIELDS USED BY INTERNAL NODES ONLY
    // children[i] is the subtree between keys[i-1] and keys[i]
    // An internal node with N keys has exactly N+1 children
    // vector<BPlusNode<keyType, valueType>*> children;
    vector<BPlusNode*> children;

    // Constructor
     // Takes one argument — whether this node is a leaf or not
    // All pointers initialized to nullptr (safe default)
    BPlusNode(bool leafStatus = false)
        : isLeaf(leafStatus)
        , parent(nullptr)
        , next(nullptr)
    {}
    
    // Destructor
    // Recursively deletes all children when a node is deleted
    // This ensures no memory leaks when tree is destroyed
    // Only deletes children — not next (leaf chain managed
    // separately by BPlusTree destructor)
    ~BPlusNode(){
        for(auto child : children){
            delete child;
        }
    }

    // Returns how many keys this node currently holds
    // Used everywhere — during insert, delete, split, merge
    int keyCount() const {
        return (int)keys.size();
    }

    // Checks if this node is the root
    // A node is root if it has no parent
    bool isRoot() const {
        return parent == nullptr;
    }

    // Returns the index where a given key should be located
    // Uses linear search through keys left to right
    // Returns the index of the first key >= searchKey
    int findKeyIndex(const KeyType& searchKey) const {
        int i = 0;
        while(i < keys.size() && keys[i] < searchKey){
            i++;
        }
        return i;
    }

    // // Returns true if this node contains the exact key
    // // Used during search and delete operations
    // bool containsKey(const keyType& searchKey) const {
    //     for(const auto& key : keys){
    //         if(key == searchKey) return true;
    //     }
    //     return false;
    // }

    // Returns the index of the child pointer to follow
    // when searching for a given key in an internal node
    int findChildIndex(const KeyType& searchKey) const {
        int i = 0;
        while(i < (int)keys.size() && searchKey >= keys[i]){
            i++;
        }
        return i;
    }

    // Checks if this node is a sibling of another node
    // Two nodes are siblings if they share the same parent
    // Used during delete to find borrow/merge candidates
    bool isSiblingOf(BPlusNode<KeyType, ValueType>* other) const {
        return (parent != nullptr) && (parent == other->parent);
    }

};