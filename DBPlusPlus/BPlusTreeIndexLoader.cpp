#include "BPlusTreeIndexLoader.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

// Helpers para trim y parseo
static std::string trim(const std::string& s) {
    size_t i = 0, j = s.size();
    while (i < j && isspace((unsigned char)s[i])) ++i;
    while (j > i && isspace((unsigned char)s[j - 1])) --j;
    return s.substr(i, j - i);
}

static std::vector<std::string> split(const std::string& s, char d) {
    std::vector<std::string> out;
    std::istringstream ss(s);
    std::string item;
    while (std::getline(ss, item, d))
        out.push_back(trim(item));
    return out;
}

BPTreeIndex loadBPlusTreeIndexTXT(const std::string& idxPath) {
    std::ifstream in(idxPath);
    if (!in) throw std::runtime_error("No se pudo abrir " + idxPath);

    BPTreeIndex idx;
    std::string line;
    BPTreePage curPage;
    bool inPage = false;

    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        auto kv = split(line, '=');
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
                    curPage = BPTreePage{};
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
                curPage.keys = split(val, ',');
            }
            else if (key == "PTRS") {
                auto parts = split(val, ',');
                curPage.ptrs.clear();
                for (auto& p : parts)
                    curPage.ptrs.push_back(std::stoi(p));
            }
            else if (key == "NEXT_LEAF") {
                curPage.nextLeaf = std::stoi(val);
            }
            // cualquier otro campo lo ignoramos
        }
    }
    // guardamos la última página leída
    if (inPage) idx.pages[curPage.pageId] = std::move(curPage);
    return idx;
}
