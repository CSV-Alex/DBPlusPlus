#pragma once
#include <vector>
#include <unordered_map>
#include "replacementstrategy.h"

class Clock : public ReplacementStrategy {
public:
  Clock(int n) ;
  void newPage(int pageId) override;
  void pin(int pageId) override;
  void unpin(int pageId) override;
  void deletePage(int pageId) override;
  int victim() override;

private:
  struct Frame {
    int pageId; // ID of the page
    bool used;  // Used bit
  };
  std::vector<Frame>  frames_;
  int hand_; // index of actual page
  void touch(int pageId);
};

Clock::Clock(int n): hand_(0) {
    frames_.resize(n);
    for (int i = 0; i < n; ++i) {
        frames_[i].pageId = -1;
        frames_[i].used = false; 
    }
  }

void Clock::newPage(int pageId) {
    std::cout << "[DEBUG] Clock::newPage() llamado\n";
    touch(pageId);
}

void Clock::pin(int pageId) {
    std::cout << "[DEBUG] Clock::pin() llamado\n";
    touch(pageId);
}
void Clock::unpin(int pageId) {
    std::cout << "[DEBUG] Clock::unpin() llamado\n";
    for (int i = 0; i < frames_.size(); ++i) {
        if (frames_[i].pageId == pageId) {
            frames_[i].used = false; // Mark as not used
            return;
        }
    }
}
void Clock::deletePage(int pageId) {
    std::cout << "[DEBUG] Clock::delete() llamado\n";
    for (int i = 0; i < frames_.size(); ++i) {
        if (frames_[i].pageId == pageId) {
            frames_[i].pageId = -1; // Clear the frame
            frames_[i].used = false; // Clear used bit
            return;
        }
    }
}
void Clock::touch(int pageId) {
    std::cout << "[DEBUG] Clock::touch() llamado\n";
    for (int i = 0; i < frames_.size(); ++i) {
        if (frames_[i].pageId == pageId) {
            frames_[i].used = true; // Mark as used
            return;
        }
    }
    // If not found, add a new page
    frames_[hand_].pageId = pageId;
    frames_[hand_].used = true; // Mark as used
    hand_ = (hand_ + 1) % frames_.size(); // Move hand to next frame
}
int Clock::victim() {
    std::cout << "[DEBUG] Clock::victim() llamado\n";
    while (true) {
        if (frames_[hand_].used) {
            frames_[hand_].used = false; // Clear used bit
        } else {
            int victimPageId = frames_[hand_].pageId;
            frames_[hand_].pageId = -1; // Clear the frame
            hand_ = (hand_ + 1) % frames_.size(); // Move hand to next frame
            return victimPageId; // Return the victim page ID
        }
        hand_ = (hand_ + 1) % frames_.size(); // Move hand to next frame
    }
}
    