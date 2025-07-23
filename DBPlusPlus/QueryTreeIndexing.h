// File: BPlusTree.h
#pragma once
#include <vector>
#include <string>
#include <iostream>

class BPlusTree {
public:
    struct Node {
        bool isLeaf;
        std::vector<int> keys;
        std::vector<std::vector<int>> blocks;
        std::vector<Node*> children;
        Node* parent;
        Node* next;
        Node(bool leaf)
            : isLeaf(leaf), parent(nullptr), next(nullptr) {
        }
    };

    explicit BPlusTree(int degree);
    ~BPlusTree();

    bool insertKey(int key, int blk);
    bool removeKey(int key);
    bool modifyKey(int oldKey, int newKey);
    bool search(int key) const;
    void printTree() const;
    void exportDot(const std::string& filename) const;
    void updateSeparators(Node* parent);
    void dumpToTxt(const std::string& filename) const;

private:
    size_t m;
    size_t minKeys;
    Node* root;

    Node* findLeaf(int key) const;
    void splitLeaf(Node* leaf);
    void splitInternal(Node* node);
    void rebalance(Node* node);
    Node* getSibling(Node* node, int& idx, bool left) const;
};

// Clase para indexación con B+Tree
class BPlusTreeIndex : public BPlusTree {
public:
    BPlusTreeIndex(const std::string& catalogPath,
        const std::string& blocksDir,
        const std::string& tableTxt,
        const std::string& relName,
        const std::string& indexField,
        int degree);

    void buildIndex();
    std::string getBlockPath(int blk) const;
    std::vector<std::string> split(const std::string& s, char delimiter);

private:
    int _fieldIndex;
    std::string _catalogPath;
    std::string _blocksDir;
    std::string _tableTxt;
    std::string _relName;
    std::string _indexField;
};