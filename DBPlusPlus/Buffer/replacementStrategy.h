#pragma once

class ReplacementStrategy {
public:
    virtual ~ReplacementStrategy() = default;

    // Called when a new page is added to the buffer pool.
    virtual void newPage(int pageId, bool pinned) =0;

    // Called when a page is pinned (accessed).
    virtual void pin(int pageId, bool pinned) =0;

    // Called when a page is unpinned (no longer accessed).
    virtual void unpin(int pageId)=0 ;

    // Called when a page is deleted from the buffer pool.
    virtual void deletePage(int pageId)=0 ;

    // Returns the ID of the victim page to be replaced.
    virtual int victim() =0;

    virtual void touch(int pageId) = 0;
};