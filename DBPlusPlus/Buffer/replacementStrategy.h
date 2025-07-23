#pragma once

#include <vector>
#include <queue>
#include <unordered_map>
#include <string>

class ReplacementStrategy {
public:
    virtual ~ReplacementStrategy() = default;

    // Called when a new page is added to the buffer pool.
    virtual void newPage(int pageId,char op, bool pinned) =0;

    // Called when a page is pinned (accessed).
    virtual void pin(int pageId,char op, bool pinned) =0;

    // Called when a page is unpinned (no longer accessed).
    virtual void unpin(int pageId)=0 ;

    // Called when a page is deleted from the buffer pool.
    virtual void deletePage(int pageId)=0 ;

    // Returns the ID of the victim page to be replaced.
    virtual int victim() =0;

    virtual void touch(int pageId) = 0;

    virtual void Status() = 0;

    virtual void printEventsStatus() = 0;
    
    std::string printQueue(int idx, std::queue<char> local_queue) {
        std::string result;
        while(!local_queue.empty()) {
            result += local_queue.front();
            local_queue.pop();
        }
        return result;
    }
};