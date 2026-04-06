#pragma once

#include <iostream>
#include <queue>
#include <vector>
#include <stdexcept>
#include "BPlusNode.hpp"

template <typename KeyType, typename ValueType>
class BPlusTree
{

private:
    BPlusNode<KeyType, ValueType> *root;
    int order;
    size_t maxKeys;
    int minKeys; // minimum keys every non-root node must maintain

public:
    // Disable copy operations to prevent double-free
    BPlusTree(const BPlusTree &) = delete;
    BPlusTree &operator=(const BPlusTree &) = delete;

    // Optionally enable move semantics
    BPlusTree(BPlusTree &&other) noexcept
        : root(other.root), order(other.order), maxKeys(other.maxKeys)
    {
        other.root = nullptr;
    }

    BPlusTree &operator=(BPlusTree &&other) noexcept
    {
        if (this != &other)
        {
            // Delete current tree (use proper cleanup)
            delete root;
            root = other.root;
            order = other.order;
            maxKeys = other.maxKeys;
            other.root = nullptr;
        }
        return *this;
    }

private:
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
        if (order < 3)
        {
            throw std::invalid_argument("B+ tree order must be >= 3");
        }
        this->order = order;
        maxKeys = static_cast<size_t>(order - 1);
        minKeys = ((order + 1) / 2) - 1; // ceil(order/2) - 1 in integer arithmetic
        root = new BPlusNode<KeyType, ValueType>(true);
    }

    ~BPlusTree()
    {
        // BPlusNode recursively deletes child pointers.
        delete root;
        root = nullptr;
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
            std::cout << "[ ";
            for (auto key : leaf->keys)
                std::cout << key << " ";

            std::cout << "] -> ";

            leaf = leaf->next;
        }

        std::cout << "NULL\n";
    }

private:
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
            throw std::logic_error("Parent-child relationship is inconsistent");
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

    // Finds what index position 'child' occupies inside
    // its parent's children array.
    //
    // This is needed because to find a sibling you must first
    // know WHERE the current node sits relative to its parent.
    // Returns -1 only if something is deeply wrong with the tree
    // structure — should never happen in a valid B+ Tree.

    int getChildIndex(BPlusNode<KeyType, ValueType> *child, BPlusNode<KeyType, ValueType> *parent)
    {
        if (child->parent == nullptr)
        {
            return -1; // Root node has no parent
        }
        for (size_t i = 0; i < parent->children.size(); i++)
        {
            if (parent->children[i] == child)
            {
                return static_cast<int>(i);
            }
        }
        return -1; // Should never happen if tree structure is valid
    }

    // Returns the LEFT sibling of 'node' — the node immediately
    // to its left that shares the same parent.
    //
    // parentKeyIndex is an OUTPUT parameter (passed by reference).
    // It gets set to the index of the separator key in the parent
    // that sits BETWEEN the left sibling and this node.
    //
    // Why do we need the separator index?
    // Because borrowing and merging both require updating or
    // removing that separator key from the parent.
    //
    // Returns nullptr if node is the leftmost child (no left sibling).

    BPlusNode<KeyType, ValueType> *getLeftSibling(
        BPlusNode<KeyType, ValueType> *node,
        int &parentKeyIndex)
    {
        if (node->parent == nullptr)
            return nullptr;

        int childIdx = getChildIndex(node, node->parent); // If node is already the leftmost child, no left sibling exists
        if (childIdx <= 0)
            return nullptr;

        // The separator key between left sibling and this node
        // sits at index childIdx - 1 in the parent's keys array
        parentKeyIndex = childIdx - 1;
        return node->parent->children[childIdx - 1];
    }

    // Returns the RIGHT sibling of 'node'.
    //
    // parentKeyIndex gets set to the separator key index between
    // this node and the right sibling.
    //
    // Returns nullptr if node is the rightmost child.

    BPlusNode<KeyType, ValueType> *getRightSibling(
        BPlusNode<KeyType, ValueType> *node,
        int &parentKeyIndex)
    {
        if (node->parent == nullptr)
            return nullptr;

        int childIdx = getChildIndex(node, node->parent);
        int lastIdx = (int)node->parent->children.size() - 1;

        // If node is already the rightmost child, no right sibling exists
        if (childIdx >= lastIdx)
            return nullptr;

        // The separator key between this node and right sibling
        // sits at index childIdx in the parent's keys array
        parentKeyIndex = childIdx;
        return node->parent->children[childIdx + 1];
    }

    // REDISTRIBUTE by borrowing one entry from the LEFT sibling.
    //
    // For LEAF nodes — the logic is straightforward:
    //   Take the last key+value from left sibling and insert it
    //   at the front of the current node. Then update the parent
    //   separator to reflect the new leftmost key of this node.
    //
    // For INTERNAL nodes — it is a rotation through the parent:
    //   The parent separator comes DOWN into the front of this node.
    //   The left sibling's last child moves to be this node's first child.
    //   The left sibling's last key goes UP to replace the separator.
    //   This keeps the tree's ordering invariant intact.
    void borrowFromLeft(
        BPlusNode<KeyType, ValueType> *node,
        BPlusNode<KeyType, ValueType> *leftSibling,
        int parentKeyIndex)
    {
        if (node->isLeaf)
        {
            // Take the last key+value from left sibling
            KeyType borrowedKey = leftSibling->keys.back();
            ValueType borrowedValue = leftSibling->values.back();

            // Insert borrowed entry at the FRONT of current node
            node->keys.insert(node->keys.begin(), borrowedKey);
            node->values.insert(node->values.begin(), borrowedValue);

            // Remove the borrowed entry from left sibling
            leftSibling->keys.pop_back();
            leftSibling->values.pop_back();

            // Parent separator must now reflect the new first key of
            // this node, because the left boundary has shifted
            node->parent->keys[parentKeyIndex] = node->keys[0];
        }
        else
        {
            // Rotate: pull separator down into front of this node
            KeyType parentKey = node->parent->keys[parentKeyIndex];
            node->keys.insert(node->keys.begin(), parentKey);

            // Adopt left sibling's last child as this node's first child
            BPlusNode<KeyType, ValueType> *adoptedChild = leftSibling->children.back();
            node->children.insert(node->children.begin(), adoptedChild);
            adoptedChild->parent = node;

            // Push left sibling's last key up to replace the separator
            node->parent->keys[parentKeyIndex] = leftSibling->keys.back();

            // Remove the rotated key and child from left sibling
            leftSibling->keys.pop_back();
            leftSibling->children.pop_back();
        }
    }

    // REDISTRIBUTE by borrowing one entry from the RIGHT sibling.
    //
    // Mirror image of borrowFromLeft — but we take from the
    // FRONT of the right sibling and append to the END of this node.
    void borrowFromRight(
        BPlusNode<KeyType, ValueType> *node,
        BPlusNode<KeyType, ValueType> *rightSibling,
        int parentKeyIndex)
    {
        if (node->isLeaf)
        {
            // Take the first key+value from right sibling
            KeyType borrowedKey = rightSibling->keys.front();
            ValueType borrowedValue = rightSibling->values.front();

            // Append borrowed entry to the END of current node
            node->keys.push_back(borrowedKey);
            node->values.push_back(borrowedValue);

            // Remove the borrowed entry from front of right sibling
            rightSibling->keys.erase(rightSibling->keys.begin());
            rightSibling->values.erase(rightSibling->values.begin());

            // Parent separator now points to new first key of right sibling
            node->parent->keys[parentKeyIndex] = rightSibling->keys[0];
        }
        else
        {
            // Rotate: pull separator down into end of this node
            KeyType parentKey = node->parent->keys[parentKeyIndex];
            node->keys.push_back(parentKey);

            // Adopt right sibling's first child as this node's last child
            BPlusNode<KeyType, ValueType> *adoptedChild = rightSibling->children.front();
            node->children.push_back(adoptedChild);
            adoptedChild->parent = node;

            // Push right sibling's first key up to replace the separator
            node->parent->keys[parentKeyIndex] = rightSibling->keys.front();

            // Remove the rotated key and child from right sibling
            rightSibling->keys.erase(rightSibling->keys.begin());
            rightSibling->children.erase(rightSibling->children.begin());
        }
    }

    // ─────────────────────────────────────────────────────────────
    // MERGE — absorbs the RIGHT node completely into the LEFT node.
    // After this function, the right node no longer exists.
    //
    // We always merge right INTO left (never the other way around).
    // This means when the underflowed node has a left sibling,
    // the underflowed node IS the right and sibling is left.
    // When it only has a right sibling, node IS left and sibling
    // IS right.
    //
    // CRITICAL: right->children.clear() must be called before
    // delete right. If we skip this, right's destructor will try
    // to delete all its children — but those children now belong
    // to left. That would be a double-deletion and a crash.
    //
    // After merging, the parent loses a separator key and a child
    // pointer. This might cause the parent itself to underflow.
    // fixUnderflow() is called on the parent to handle this
    // cascade — this is the recursive chain we discussed earlier.
    // ─────────────────────────────────────────────────────────────
    void mergeNodes(
        BPlusNode<KeyType, ValueType> *left,
        BPlusNode<KeyType, ValueType> *right,
        int parentKeyIndex)
    {
        if (left->isLeaf)
        {
            // Copy all keys and values from right into left
            for (int i = 0; i < (int)right->keys.size(); i++)
            {
                left->keys.push_back(right->keys[i]);
                left->values.push_back(right->values[i]);
            }

            // Fix the leaf chain — left now points past right
            // to whatever right was pointing to
            left->next = right->next;
        }
        else
        {
            // For internal nodes, the parent separator key that was
            // dividing left and right must come DOWN into left.
            // The division no longer exists after merge, so the
            // separator is pulled down to maintain tree ordering.
            KeyType separatorKey = left->parent->keys[parentKeyIndex];
            left->keys.push_back(separatorKey);

            // Copy all keys from right into left
            for (auto &k : right->keys)
                left->keys.push_back(k);

            // Copy all children from right into left
            // and update each child's parent pointer to left
            for (auto &child : right->children)
            {
                left->children.push_back(child);
                child->parent = left;
            }
        }

        // Remove the separator key from parent
        left->parent->keys.erase(
            left->parent->keys.begin() + parentKeyIndex);

        // Remove right's pointer from parent's children array
        left->parent->children.erase(
            left->parent->children.begin() + parentKeyIndex + 1);

        // MUST clear right's children before deleting right,
        // otherwise right's destructor will delete children
        // that now belong to left — causing a double-free crash
        right->children.clear();
        delete right;

        // Parent lost a key — check if it now underflows too
        // This is the upward cascade that makes delete complex
        fixUnderflow(left->parent);
    }

    // ─────────────────────────────────────────────────────────────
    // COORDINATOR — examines the underflowed node and decides
    // which of the four responses to apply, in priority order:
    //
    //   1. Borrow from left  (if left sibling has keys to spare)
    //   2. Borrow from right (if right sibling has keys to spare)
    //   3. Merge with left   (if left exists but cannot spare)
    //   4. Merge with right  (fallback)
    //
    // The root is treated specially because it does not have a
    // minimum key requirement like other nodes. When the root
    // becomes an empty internal node after a merge below it,
    // its only remaining child becomes the new root and the
    // tree shrinks by one level.
    // ─────────────────────────────────────────────────────────────

    // COORDINATOR — examines the underflowed node and decides
    // which of the four responses to apply, in priority order:
    //
    //   1. Borrow from left  (if left sibling has keys to spare)
    //   2. Borrow from right (if right sibling has keys to spare)
    //   3. Merge with left   (if left exists but cannot spare)
    //   4. Merge with right  (fallback)
    //
    // The root is treated specially because it does not have a
    // minimum key requirement like other nodes. When the root
    // becomes an empty internal node after a merge below it,
    // its only remaining child becomes the new root and the
    // tree shrinks by one level.

    void fixUnderflow(BPlusNode<KeyType, ValueType> *node)
    {
        // Root gets special treatment — no minimum key requirement
        if (node->isRoot())
        {
            // An internal root with no keys means the tree just
            // merged everything into one level below — shrink tree
            if (!node->isLeaf && node->keys.empty())
            {
                BPlusNode<KeyType, ValueType> *newRoot = node->children[0];
                newRoot->parent = nullptr;
                root = newRoot;

                // Clear before delete to prevent destructor from
                // deleting the new root we just promoted
                node->children.clear();
                delete node;
            }
            // A leaf root with 0 keys means the tree is simply empty
            // That is a valid state — do nothing
            return;
        }

        // If this node still has enough keys, nothing to fix
        if ((int)node->keys.size() >= minKeys)
            return;

        // Discover siblings and their separator positions
        int leftParentKeyIndex = -1;
        int rightParentKeyIndex = -1;

        BPlusNode<KeyType, ValueType> *leftSibling =
            getLeftSibling(node, leftParentKeyIndex);
        BPlusNode<KeyType, ValueType> *rightSibling =
            getRightSibling(node, rightParentKeyIndex);

        // Priority 1: borrow from left if it has a key to spare
        if (leftSibling != nullptr &&
            (int)leftSibling->keys.size() > minKeys)
        {
            borrowFromLeft(node, leftSibling, leftParentKeyIndex);
        }
        // Priority 2: borrow from right if it has a key to spare
        else if (rightSibling != nullptr &&
                 (int)rightSibling->keys.size() > minKeys)
        {
            borrowFromRight(node, rightSibling, rightParentKeyIndex);
        }
        // Priority 3: merge with left sibling
        // Current node becomes the RIGHT in mergeNodes
        else if (leftSibling != nullptr)
        {
            mergeNodes(leftSibling, node, leftParentKeyIndex);
        }
        // Priority 4: merge with right sibling
        // Current node becomes the LEFT in mergeNodes
        else
        {
            mergeNodes(node, rightSibling, rightParentKeyIndex);
        }
    }

public:
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
        std::vector<ValueType> result;

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

    // When the first key of a leaf is deleted, any internal node
    // that was using that key as a separator now holds a stale
    // value. This function walks upward from the leaf's parent
    // and replaces the old key with the new first key of the leaf.
    //
    // Why only the first key?
    // In B+ Trees, internal separators are always COPIES of the
    // first key of some leaf node. Only first-key deletions can
    // make a separator stale — deleting any other position in the
    // leaf does not affect any separator above.

    void updateParentKey(
        BPlusNode<KeyType, ValueType> *leaf,
        const KeyType &oldKey,
        const KeyType &newKey)
    {
        BPlusNode<KeyType, ValueType> *current = leaf->parent;

        while (current != nullptr)
        {
            for (int i = 0; i < (int)current->keys.size(); i++)
            {
                if (current->keys[i] == oldKey)
                {
                    current->keys[i] = newKey;
                    return;
                }
            }
            current = current->parent;
        }
    }

    //  remove() — the only function your teammates call.
    //
    // The full deletion sequence is:
    //   1. Navigate to the correct leaf using findLeaf()
    //   2. Search the leaf linearly for the exact key
    //   3. If not found → return false immediately
    //   4. Erase key+value from the leaf at the found index
    //   5. If the deleted key was at index 0 AND the leaf still
    //      has keys AND the leaf is not the root:
    //      → a parent separator may now be stale → update it
    //   6. Call fixUnderflow() on the leaf to handle any
    //      minimum-key violation and its upward cascade
    //   7. Return true

    bool remove(const KeyType &key)
    {
        BPlusNode<KeyType, ValueType> *leaf = findLeaf(key);

        // Search leaf linearly for the exact key
        int keyIndex = -1;
        for (int i = 0; i < (int)leaf->keys.size(); i++)
        {
            if (leaf->keys[i] == key)
            {
                keyIndex = i;
                break;
            }
        }

        // Key does not exist in the tree at all
        if (keyIndex == -1)
            return false;

        // Erase the key and its paired value from the leaf
        leaf->keys.erase(leaf->keys.begin() + keyIndex);
        leaf->values.erase(leaf->values.begin() + keyIndex);

        // If we deleted the first key (index 0) of this leaf,
        // AND the leaf still has at least one key remaining,
        // AND this leaf is not the root (roots have no separators above),
        // then there is possibly a stale separator in a parent node.
        // Update it now before fixUnderflow reshapes the tree.
        if (keyIndex == 0 && !leaf->keys.empty() && !leaf->isRoot())
        {
            updateParentKey(leaf, key, leaf->keys[0]);
        }

        // Check and repair any underflow — may cascade upward
        fixUnderflow(leaf);

        return true;
    }

    // Returns the root node of this tree
    // Used by FileSystemManager to walk leaf nodes for ls() and find()
    BPlusNode<KeyType, ValueType>* getRoot() { return root; }

    // Returns the order of this tree
    int getOrder() const { return order; }
};