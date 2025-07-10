#pragma once
#include <list>
#include <unordered_map>
#include <queue>
#include "replacementStrategy.h"

struct frame{
    int pageId; // ID of the page
    int pinCount = 0; // number of pins on the page
    bool pinStatus = false; // true if the page is pinned, false otherwise
    int lastAccess = 0; // last access time for LRU tracking
};

class LRU : public ReplacementStrategy {
public:
    explicit LRU(int capacity);
    void newPage(int pageId,bool) override;
    void touch(int pageId) override;
    void pin(int pageId,bool) override;
    void unpin(int pageId) override;
    void deletePage(int pageId) override;
    int getpos(int pageId);
    int victim() override;

private:
    int _capacity;
    vector<frame> frames_; // vector of frames
    std::list<int> lru; //tracks the order of pages in LRU fashion
    std::unordered_map<int, std::list<int>::iterator> pos; // maps pageId to its position in the LRU list
    std::queue<int> local_queue;
    int lru_tracker=0;
    bool full, locked;
};

LRU::LRU(int capacity) {
    _capacity = capacity;
    frames_.resize(capacity, frame{ -1, 0, false }); // initialize frames with invalid pageId
    pos.reserve(capacity); // reserve space for pageId positions
    local_queue = std::queue<int>(); // initialize the local queue
    full=false;
    locked=false;
}

void LRU::newPage(int pageId, bool pinned) {
    // 1) Find a free slot (pageId == -1) or pick a victim if full
    int idx = -1;
    for (int i = 0; i < _capacity; ++i) {
        if (frames_[i].pageId == -1) {
            idx = i;
            break;
        }
    }

    frame &f = frames_[idx];
    f.pageId    = pageId;
    f.pinStatus = 1;
    f.pinCount  = (pinned ? 1 : 0);

    ++lru_tracker;
    f.lastAccess = lru_tracker;

    // 4) lru insert
    lru.push_back(pageId);
    pos[pageId] = std::prev(lru.end());
}

void LRU::touch(int pageId) { //similar to pin, thou it's operation independent
    int idx = getpos(pageId);
    if (idx < 0) return;             // page not present

    // 1) advance the global clock
    ++lru_tracker;                  

    // 2) stamp the frame with the new time
    frames_[idx].lastAccess = lru_tracker;

    // 3) update its position in the LRU list
    //    (remove old position, then push to back)
    auto it = pos.find(pageId);
    if (it != pos.end()) {
        lru.erase(it->second);
    }
    lru.push_back(pageId);
    pos[pageId] = std::prev(lru.end());
}

void LRU::pin(int pageId, bool pinned) {
    int idx = getpos(pageId);

    if (idx < 0) return;

    frame &f = frames_[idx];

    ++f.pinCount;
    f.pinStatus = pinned;

    ++lru_tracker;

    f.lastAccess=lru_tracker;
    
    // remove from LRU list if it’s there
    auto it = pos.find(pageId);
    if (it != pos.end()) {
        lru.erase(it->second);
        pos.erase(it);
    }
    lru.push_back(pageId);
    pos[pageId] = std::prev(lru.end());

}

void LRU::unpin(int pageId) {
    int idx =getpos(pageId);

    frame &f =frames_[idx];

    f.pinCount =0;
    f.pinStatus = false;


    locked=false;
}

void LRU::deletePage(int pageId) {
    int idx =getpos(pageId);
    frames_[idx] = frame{-1,0, false, lru_tracker };    

    auto it = pos.find(pageId);
    if (it != pos.end()) {
        lru.erase(it->second);  
        pos.erase(it);
    }
}

int LRU::victim() {

    int n_locked=0;

    while(!locked){

        auto pageId = *(lru.begin());

        int victim_idx = getpos(pageId);
        //first pass
        frame &f = frames_[victim_idx];
        f.pinCount--;
        f.lastAccess= ++lru_tracker;
        // remove from LRU list if it’s there
        auto it = pos.find(f.pageId);
        if (it != pos.end()) {
            lru.erase(it->second);
            pos.erase(it);
        }
        lru.push_back(f.pageId);
        pos[f.pageId] = std::prev(lru.end());

        if(f.pinCount==0 && !f.pinStatus){
            return victim_idx;
        }

        n_locked++;
        if (n_locked >= _capacity) {
            locked = true; // si hemos recorrido todos los frames y están bloqueados, salimos del bucle
        }
        
    }

    /*
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
    */
    std::cerr << "[ERROR] no hay frame elegible\n";
    return -1;
}

int LRU::getpos(int pageId) {
    auto it = pos.find(pageId);
    if (it != pos.end()) {
        return std::distance(lru.begin(), it->second);
    }
    return -1;
}
