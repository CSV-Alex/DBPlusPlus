#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>

struct BPTreePageLoader {
    int      pageId;
    bool     isLeaf;
    int      numKeys;
    std::vector<std::string> keys;
    std::vector<int>         ptrs;
    int      nextLeaf;      // -1 si no aplica
};

struct TreeIndex {
    int order;
    int rootPageId;
    std::unordered_map<int, BPTreePageLoader> pages;

    // Devuelve nullptr si no existe
    const BPTreePageLoader* getPageIndex(int pageId) const {
        auto it = pages.find(pageId);
        return it != pages.end() ? &it->second : nullptr;
    }
};

TreeIndex loadBPlusTreeIndexTXT(const std::string& idxPath);
