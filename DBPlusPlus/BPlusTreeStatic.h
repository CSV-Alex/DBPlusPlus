#pragma once
#include <vector>
#include <iostream>
#include <string>

// Insercion, eliminacion, modificacion y busqueda de llaves

class BPlusTree {
public:
    struct Node {
        bool isLeaf;
        std::vector<int> keys;
        std::vector<Node*> children;
        Node* parent;
        Node* next; // hojas
        Node(bool leaf)
            : isLeaf(leaf), parent(nullptr), next(nullptr) {
        }
    };

    explicit BPlusTree(int m);
    ~BPlusTree();

    bool insert(int key);
    void updateSeparators(Node* parent);
    bool remove(int key);
    bool modify(int oldKey, int newKey);
    bool search(int key) const;
    void print() const;
    void exportDot(const std::string& filename) const;

private:

    Node* root;
    int m;
    int minKeys;

    Node* findLeaf(int key) const;
    void splitLeaf(Node* leaf);
    void splitInternal(Node* node);
    void rebalance(Node* node);
    Node* getSibling(Node* node, int& idx, bool left) const;
};