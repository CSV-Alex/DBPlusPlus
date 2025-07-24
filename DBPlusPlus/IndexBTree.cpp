#define _CRT_SECURE_NO_WARNINGS

#include "QueryBlocks.h"               // loadRelationHeader
#include "IndexBTree.h"      // loadBPlusTreeIndexTXT, BPTreeIndex, BPTreePage
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>

// Helpers para trim y parseo
static std::string trimIndex(const std::string& s) {
    size_t i = 0, j = s.size();
    while (i < j && isspace((unsigned char)s[i])) ++i;
    while (j > i && isspace((unsigned char)s[j - 1])) --j;
    return s.substr(i, j - i);
}

static std::vector<std::string> splitIndex(const std::string& s, char d) {
    std::vector<std::string> out;
    std::istringstream ss(s);
    std::string item;
    while (std::getline(ss, item, d))
        out.push_back(trimIndex(item));
    return out;
}

TreeIndex loadBPlusTreeIndexTXT(const std::string& idxPath) {
    std::ifstream in(idxPath);
    if (!in) throw std::runtime_error("No se pudo abrir " + idxPath);

    TreeIndex idx;
    std::string line;
    BPTreePageLoader curPage;
    bool inPage = false;

    while (std::getline(in, line)) {
        line = trimIndex(line);
        if (line.empty() || line[0] == '#') continue;

        auto kv = splitIndex(line, '=');
        if (kv.size() == 2) {
            auto& key = kv[0];
            auto& val = kv[1];

            if (key == "ORDER") {
                idx.order = std::stoi(val);
            }
            else if (key == "ROOT") {
                idx.rootPageId = std::stoi(val);
            }
            else if (key == "PAGE_ID") {
                // si ya estábamos dentro de un page, guardamos la anterior
                if (inPage) {
                    idx.pages[curPage.pageId] = std::move(curPage);
                    curPage = BPTreePageLoader{};
                }
                inPage = true;
                curPage.pageId = std::stoi(val);
            }
            else if (key == "IS_LEAF") {
                curPage.isLeaf = (val != "0");
            }
            else if (key == "NUM_KEYS") {
                curPage.numKeys = std::stoi(val);
            }
            else if (key == "KEYS") {
                curPage.keys = splitIndex(val, ',');
            }
            else if (key == "PTRS") {
                auto parts = splitIndex(val, ',');
                curPage.ptrs.clear();
                for (auto& p : parts)
                    curPage.ptrs.push_back(std::stoi(p));
            }
            else if (key == "NEXT_LEAF") {
                curPage.nextLeaf = std::stoi(val);
            }
        }
    }
    // guardamos la última página leída
    if (inPage) idx.pages[curPage.pageId] = std::move(curPage);
    return idx;
}