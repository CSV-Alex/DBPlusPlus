#pragma once
#include <vector>
#include <iostream>

// Insercion, eliminacion, modificacion y busqueda de llaves

class BPlusTree {
public:
    explicit BPlusTree(int m);
    ~BPlusTree();

    bool insert(int key);
    bool remove(int key);
    bool modify(int oldKey, int newKey);
    bool search(int key) const;
    void print() const;

private:
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

    Node* root;
    int m;
    int minKeys;

    Node* findLeaf(int key) const;
    void splitLeaf(Node* leaf);
    void splitInternal(Node* node);
    void rebalance(Node* node);
    Node* getSibling(Node* node, int& idx, bool left) const;
};