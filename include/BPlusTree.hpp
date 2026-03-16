#pragma once

#include <iostream>
#include <queue>
#include <vector>
#include <stdexcept>
#include "BPlusNode.hpp"
using namespace std;

template <typename KeyType, typename ValueType>
class BPlusTree
{

private:
    BPlusNode<KeyType, ValueType> *root;
    int order;
    size_t maxKeys;

    BPlusNode<KeyType, ValueType> *findLeaf(const KeyType &key)
    {

        BPlusNode<KeyType, ValueType> *current = root;

        while (!current->isLeaf)
        {

            int index = current->findChildIndex(key);
            current = current->children[index];
        }

        return current;
    }

public:
    BPlusTree(int order = 4)
    {
        if(order < 3){
            throw invalid_argument("B+ tree order must be >= 3");
        }
        this->order = order;
        maxKeys = static_cast<size_t>(order - 1);
        root = new BPlusNode<KeyType, ValueType>(true);
    }

    ~BPlusTree()
    {
        delete root;
    }

    bool search(const KeyType &key, ValueType &result)
    {
        BPlusNode<KeyType, ValueType> *leaf = findLeaf(key);

        for (size_t i = 0; i < leaf->keys.size(); i++)
        {
            if (leaf->keys[i] == key)
            {
                result = leaf->values[i];
                return true;
            }
        }
        return false;
    }

    void insert(const KeyType &key, const ValueType &value)
    {
        BPlusNode<KeyType, ValueType> *leaf = findLeaf(key);

        int index = leaf->findKeyIndex(key);

        // If key already exists, update its value instead of inserting duplicate key
        if (index < (int)leaf->keys.size() && leaf->keys[index] == key)
        {
            leaf->values[index] = value;
            return;
        }

        leaf->keys.insert(leaf->keys.begin() + index, key);
        leaf->values.insert(leaf->values.begin() + index, value);

        if (leaf->keys.size() > maxKeys)
        {
            splitLeaf(leaf);
        }
    }

    void displayLeaf()
    {
        BPlusNode<KeyType, ValueType> *leaf = root;

        while (!leaf->isLeaf)
            leaf = leaf->children[0];

        while (leaf != nullptr)
        {
            cout << "[ ";
            for (auto key : leaf->keys)
                cout << key << " ";

            cout << "] -> ";

            leaf = leaf->next;
        }

        cout << "NULL\n";
    }

    void splitInternal(BPlusNode<KeyType, ValueType> *node)
    {
        BPlusNode<KeyType, ValueType> *newInternal = new BPlusNode<KeyType, ValueType>(false);

        int totalKeys = (int)node->keys.size();
        int midIndex = totalKeys / 2;
        KeyType promoteKey = node->keys[midIndex];

        // Move keys
        for (int i = midIndex + 1; i < totalKeys; i++)
        {
            newInternal->keys.push_back(node->keys[i]);
        }

        // Move children
        for (int i = midIndex + 1; i < (int)node->children.size(); i++)
        {
            newInternal->children.push_back(node->children[i]);
            node->children[i]->parent = newInternal;
        }

        // Resize keys and children
        node->keys.resize(midIndex);
        node->children.resize(midIndex + 1);

        newInternal->parent = node->parent;

        insertInParent(node, promoteKey, newInternal);
    }

    void insertInParent(BPlusNode<KeyType, ValueType> *left, const KeyType &key, BPlusNode<KeyType, ValueType> *right)
    {
        // Case 1: if left node was root
        if (left->isRoot())
        {
            BPlusNode<KeyType, ValueType> *newRoot = new BPlusNode<KeyType, ValueType>(false);
            newRoot->keys.push_back(key);
            newRoot->children.push_back(left);
            newRoot->children.push_back(right);

            left->parent = newRoot;
            right->parent = newRoot;

            root = newRoot;

            return;
        }

        // Case 2: normal insertion into parent
        // BPlusNode<KeyType, ValueType>* parent = left->parent;

        // int index = parent->findChildIndex(key);
        // parent->keys.insert(parent->keys.begin() + index, key);
        // parent->children.insert(parent->children.begin() + index + 1, right);

        // right->parent = parent;
        BPlusNode<KeyType, ValueType> *parent = left->parent;

        int leftChildIndex = -1;
        for (size_t i = 0; i < parent->children.size(); i++)
        {
            if (parent->children[i] == left)
            {
                leftChildIndex = static_cast<int>(i);
                break;
            }
        }

        if (leftChildIndex == -1)
        {
            throw logic_error("Parent-child relationship is inconsistent");
        }

        parent->keys.insert(parent->keys.begin() + leftChildIndex, key);
        parent->children.insert(parent->children.begin() + leftChildIndex + 1, right);

        right->parent = parent;

        // Case 3: parent overflow
        if (parent->keys.size() > maxKeys)
        {
            splitInternal(parent);
        }
    }

    void splitLeaf(BPlusNode<KeyType, ValueType> *leaf)
    {
        // Create a new leaf
        BPlusNode<KeyType, ValueType> *newLeaf = new BPlusNode<KeyType, ValueType>(true);
        int totalKeys = (int)leaf->keys.size();
        int splitIndex = (totalKeys + 1) / 2;

        // Move second half from splitIndex in newLeaf
        for (int i = splitIndex; i < totalKeys; i++)
        {
            newLeaf->keys.push_back(leaf->keys[i]);
            newLeaf->values.push_back(leaf->values[i]);
        }

        // Remove moved keys from original leaf
        leaf->keys.resize(splitIndex);
        leaf->values.resize(splitIndex);

        // Maintain leaf chain
        newLeaf->next = leaf->next;
        leaf->next = newLeaf;
        newLeaf->parent = leaf->parent;

        // Promote first key of new leaf
        KeyType promoteKey = newLeaf->keys[0];

        insertInParent(leaf, promoteKey, newLeaf);
    }

    void displayTree()
    {
        if (root == nullptr)
        {
            std::cout << "Tree is empty\n";
            return;
        }

        std::queue<BPlusNode<KeyType, ValueType> *> q;
        q.push(root);

        int level = 0;

        while (!q.empty())
        {
            int size = (int)q.size();

            std::cout << "Level " << level << " : ";

            for (int i = 0; i < size; i++)
            {
                BPlusNode<KeyType, ValueType> *node = q.front();
                q.pop();

                std::cout << "[ ";

                for (auto key : node->keys)
                    std::cout << key << " ";

                std::cout << "] ";

                if (!node->isLeaf)
                {
                    for (auto child : node->children)
                        q.push(child);
                }
            }

            std::cout << std::endl;

            level++;
        }
    }

    vector<ValueType> rangeSearch(const KeyType &startKey, const KeyType &endKey)
    {
        vector<ValueType> result;

        BPlusNode<KeyType, ValueType> *leaf = findLeaf(startKey);

        while (leaf != nullptr)
        {
            for (size_t i = 0; i < leaf->keys.size(); i++)
            {
                KeyType key = leaf->keys[i];

                if (key >= startKey && key <= endKey)
                {
                    result.push_back(leaf->values[i]);
                }

                if (key > endKey)
                    return result;
            }

            leaf = leaf->next;
        }

        return result;
    }
};
