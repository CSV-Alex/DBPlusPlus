#pragma once
#include "Buffer/BufferPool.h"
#include "QueryBlocks.h"               // loadRelationHeader
#include "PageWithRecords.h"           // getRawData() + filterRecords(...)
#include "BPlusTreeIndexLoader.h"      // loadBPlusTreeIndexTXT, BPTreeIndex, BPTreePage
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
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

void executeBPlusTreeQuery(
    BufferPool* pBufPool,
    const std::string& basePath,
    const std::string& from,
    const std::string& field,
    const std::string& op,
    const std::string& val
) {
    const int INDEX_OFFSET = 10000;
    try {
        // 1) Ruta dinámica al archivo de índice
        std::string idxFile = basePath + from + "_" + field + "_bptree.txt";
        std::cout << "Cargando B+Tree desde: " << idxFile << "\n";

        // 2) Carga del índice en memoria
        BPTreeIndex bpt = loadBPlusTreeIndexTXT(idxFile);

        // 3) Navegar desde la raíz, pinneando índice
        const BPTreePage* node = bpt.getPage(bpt.rootPageId);
        if (!node) throw std::runtime_error("Raíz no encontrada en índice");

        // Pin root index page con offset
        pBufPool->getPage(node->pageId + INDEX_OFFSET, 'R', true);

        // Descender hasta la hoja
        while (!node->isLeaf) {
            size_t i = 0;
            // encontrar rama correcta
            while (i < (size_t)node->numKeys && val >= node->keys[i]) {
                ++i;
            }
            int childPid = node->ptrs[i];

            // Unpin current index page
            pBufPool->unpinPage(node->pageId + INDEX_OFFSET);
            // Pin next index page
            pBufPool->getPage(childPid + INDEX_OFFSET, 'R', true);

            node = bpt.getPage(childPid);
            if (!node) throw std::runtime_error(
                "Nodo interno faltante, PAGE_ID=" + std::to_string(childPid)
            );
        }

        // 4) Ahora node es hoja, ya está pinneada
        std::vector<int> dataBlocks = node->ptrs;
        std::cout << "Leaf PAGE_ID=" << node->pageId
            << " → bloques: ";
        for (int b : dataBlocks) std::cout << b << " ";
        std::cout << "\n\n";

        // 5) Cargar header de la tabla para filtrar
        auto headers = loadRelationHeader(basePath + from + ".txt");

        // 6) Leer y filtrar cada bloque
        for (int blk : dataBlocks) {
            Page* raw = pBufPool->getPage(blk, 'R', true);
            auto page = dynamic_cast<PageWithRecords*>(raw);
            if (!page) {
                std::cerr << "ERROR: bloque " << blk
                    << " no es PageWithRecords\n";
                pBufPool->unpinPage(blk);
                continue;
            }

            auto rows = page->filterRecords(
                basePath + from + ".txt",
                field, op, val
            );
            std::cout << "Registros en bloque " << blk << ":\n";
            for (auto& r : rows) {
                for (size_t j = 0; j < r.size(); ++j) {
                    std::cout << r[j]
                        << (j + 1 < r.size() ? " | " : "\n");
                }
            }
            std::cout << "\n";

            pBufPool->unpinPage(blk);
        }

        // Unpin leaf index page
        pBufPool->unpinPage(node->pageId + INDEX_OFFSET);
    }
    catch (const std::exception& e) {
        std::cerr << "ERROR B+Tree: " << e.what() << "\n";
    }
}
