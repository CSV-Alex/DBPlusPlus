#pragma once
#include <list>
#include <unordered_map>
#include <queue>
#include "replacementStrategy.h"

class LRU : public ReplacementStrategy {
public:
    explicit LRU(int capacity);
    void newPage(int pageId) override;
    void touch(int pageId) override;
    void pin(int pageId) override;
    void unpin(int pageId) override;
    void deletePage(int pageId) override;
    int getpos(int pageId);
    int victim() override;

private:
    int _capacity;
    std::list<int> lru;
    std::unordered_map<int, std::list<int>::iterator> pos;
    std::unordered_map<int, int> pinCount;
    std::unordered_map<int, bool> pinStatus;
    std::queue<int> local_queue;
};

LRU::LRU(int capacity) : _capacity(capacity) {}

void LRU::newPage(int pageId) {
    if ((int)lru.size() == _capacity) {
        int old = victim();
        if (old != -1) deletePage(old);
    }
    if (!pos.count(pageId)) {
        lru.push_back(pageId);
        pos[pageId] = std::prev(lru.end());
    }
    pinCount[pageId] = 1;
    pinStatus[pageId] = false;
    local_queue.push(pageId);
}

void LRU::touch(int pageId) {
    auto it = pos.find(pageId);
    if (it != pos.end()) {
        lru.erase(it->second);
        pos.erase(it);
    }
    lru.push_back(pageId);
    pos[pageId] = std::prev(lru.end());
    local_queue.push(pageId);
}

void LRU::pin(int pageId) {
    pinCount[pageId]++;
    pinStatus[pageId] = true;
    local_queue.push(pageId);
    auto it = pos.find(pageId);
    if (it != pos.end()) {
        lru.erase(it->second);
        pos.erase(it);
    }
}

void LRU::unpin(int pageId) {
    auto itCount = pinCount.find(pageId);
    if (itCount != pinCount.end() && itCount->second > 0) {
        itCount->second--;
    }
    pinStatus[pageId] = false;
    local_queue.push(pageId);
    if (!pos.count(pageId)) {
        lru.push_back(pageId);
        pos[pageId] = std::prev(lru.end());
    }
}

void LRU::deletePage(int pageId) {
    auto it = pos.find(pageId);
    if (it != pos.end()) {
        lru.erase(it->second);
        pos.erase(it);
    }
    pinCount.erase(pageId);
    pinStatus.erase(pageId);
}

int LRU::victim() {
    while (!local_queue.empty()) {
        int pid = local_queue.front();
        local_queue.pop();
        auto itCount = pinCount.find(pid);
        if (itCount == pinCount.end()) continue;
        int pc = itCount->second;
        if (pc > 1) {
            itCount->second = pc - 1;
            auto it = pos.find(pid);
            if (it != pos.end()) {
                lru.erase(it->second);
                pos.erase(it);
            }
            lru.push_back(pid);
            pos[pid] = std::prev(lru.end());
        } else {
            bool ps = pinStatus[pid];
            if (pc <= 1 && ps == false) {
                auto it = pos.find(pid);
                if (it != pos.end()) {
                    lru.erase(it->second);
                    pos.erase(it);
                }
                pinCount.erase(pid);
                pinStatus.erase(pid);
                return pid;
            }
        }
    }
    return -1;
}

int LRU::getpos(int pageId) {
    auto it = pos.find(pageId);
    if (it != pos.end()) {
        return std::distance(lru.begin(), it->second);
    }
    return -1;
}
