#pragma once
#include <list>
#include <unordered_map>


class LRUReplacer {
public:
    void newPage(int pageId) { touch(pageId); }
    void pin(int pageId) { touch(pageId); }
    void unpin(int pageId) { touch(pageId); }
    void deletePage(int pageId);
    int victim()   ;
private:
    std::list<int> lru;
    std::unordered_map<int, std::list<int>::iterator> pos;
    void touch(int pageId);
};

void LRUReplacer::deletePage(int pageId) {
    auto it = pos.find(pageId);
    if (it != pos.end()) {
        lru.erase(it->second);
        pos.erase(it);
    }
}
int LRUReplacer::victim() {
    if (lru.empty()) return -1;
    return lru.front();
}

void LRUReplacer::touch(int pageId) {
    auto it = pos.find(pageId);
    if (it != pos.end()) {
        lru.erase(it->second);
        pos.erase(it);
    }
    lru.push_back(pageId);
    pos[pageId] = std::prev(lru.end());
}