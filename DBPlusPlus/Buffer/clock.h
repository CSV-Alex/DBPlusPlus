#pragma once
#include <list>
#include <unordered_map>
#include "replacementstrategy.h"

class Clock : public ReplacementStrategy {
public:
  Clock() : hand_(frames_.end()) {}

  // Called when a new page is brought into memory
  void newPage(int pageId) override {
    // Insert at end of list, with ref bit = 1 (just loaded)
    frames_.push_back({pageId, true});
    auto it = std::prev(frames_.end());
    page_map_[pageId] = it;

    if (hand_ == frames_.end())
      hand_ = it;  // first insertion
  }

  // “Pin” = mark as recently used
  void pin(int pageId) override {
    auto it = page_map_.find(pageId);
    if (it != page_map_.end())
      it->second->used = true;
  }

  // “Unpin” might clear usage — but typically you leave it
  // here we choose to clear, so it’s a candidate sooner:
  void unpin(int pageId) override {
    auto it = page_map_.find(pageId);
    if (it != page_map_.end())
      it->second->used = false;
  }

  // Remove a page entirely (e.g., when it’s freed)
  void deletePage(int pageId) override {
    auto it = page_map_.find(pageId);
    if (it == page_map_.end()) return;

    // If our hand is pointing at it, advance first
    if (hand_ == it->second) {
      ++hand_;
      if (hand_ == frames_.end()) hand_ = frames_.begin();
    }

    frames_.erase(it->second);
    page_map_.erase(it);
  }

  // Find a victim: advance hand until you hit used==false
  int victim() override {
    if (frames_.empty())
      return -1;  // or throw

    // Loop until we find a frame with used==false
    while (true) {
      if (hand_ == frames_.end())
        hand_ = frames_.begin();

      if (!hand_->used) {
        int pg = hand_->page;
        // Remove it from clock
        auto toErase = hand_++;
        if (hand_ == frames_.end()) hand_ = frames_.begin();
        frames_.erase(toErase);
        page_map_.erase(pg);
        return pg;
      } else {
        // give second chance
        hand_->used = false;
        ++hand_;
      }
    }
  }

private:
  struct Frame { int page; bool used; };

  std::list<Frame>                          frames_;
  std::unordered_map<int, typename std::list<Frame>::iterator> page_map_;
  typename std::list<Frame>::iterator       hand_;
};