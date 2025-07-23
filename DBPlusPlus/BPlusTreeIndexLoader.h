#pragma once
#include <string>
#include <vector>
#include <unordered_map>

struct BPTreePage {
    int      pageId;
    bool     isLeaf;
    int      numKeys;
    std::vector<std::string> keys;
    std::vector<int>         ptrs;
    int      nextLeaf;      // -1 si no aplica
};

struct BPTreeIndex {
    int order;              // ORDER del árbol
    int rootPageId;         // ROOT de la raíz
    std::unordered_map<int, BPTreePage> pages;

    // Devuelve nullptr si no existe
    const BPTreePage* getPage(int pageId) const {
        auto it = pages.find(pageId);
        return it != pages.end() ? &it->second : nullptr;
    }
};

BPTreeIndex loadBPlusTreeIndexTXT(const std::string& idxPath);
