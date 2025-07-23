// File: BPlusTree.h
#pragma once
#include <vector>
#include <string>
#include <iostream>

// Plantilla de B+ Tree para claves de tipo genérico
template<typename Key>
class BPlusTree {
public:
    struct Node {
        bool isLeaf;
        std::vector<Key> keys;
        std::vector<std::vector<int>> blocksPorKey;
        std::vector<Node*> children;
        Node* parent;
        Node* next;
        Node(bool leaf)
            : isLeaf(leaf), parent(nullptr), next(nullptr) {
        }
    };

    explicit BPlusTree(size_t degree);
    ~BPlusTree();

    bool insertKey(const Key& key);
    bool removeKey(const Key& key);
    bool modifyKey(const Key& oldKey, const Key& newKey);
    bool search(const Key& key) const;
    void print() const;
    void exportDot(const std::string& filename) const;
    void updateSeparators(Node* parent);

private:
    size_t m;
    size_t minKeys;
    Node* root;

    Node* findLeaf(const Key& key) const;
    void splitLeaf(Node* leaf);
    void splitInternal(Node* node);
    void rebalance(Node* node);
    Node* getSibling(Node* node, int& idx, bool left) const;
};