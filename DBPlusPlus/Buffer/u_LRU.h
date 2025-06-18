#pragma once
#include <list>
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>

//#include "ReplacementStrategy.h"

class LRUReplacer: public ReplacementStrategy {
    public:
        void newPage(int pageId) { touch(pageId); }
        void pin(int pageId) { touch(pageId); }
        void unpin(int pageId) { touch(pageId); }
        void deletePage(int pageId) {
            auto it = pos.find(pageId);
            if (it != pos.end()) {
                lru.erase(it->second);
                pos.erase(it);
            }
        }
        int victim() {
            if (lru.empty()) return -1;
            return lru.front();
        }
    private:
        std::list<int> lru;
        std::unordered_map<int, std::list<int>::iterator> pos;
        void touch(int pageId) {
            auto it = pos.find(pageId);
            if (it != pos.end()) {
                lru.erase(it->second);
                pos.erase(it);
            }
            lru.push_back(pageId);
            pos[pageId] = std::prev(lru.end());
        }
};