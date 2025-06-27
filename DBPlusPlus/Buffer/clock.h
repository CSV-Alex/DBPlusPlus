#pragma once
#include <vector>
#include <unordered_map>
#include "replacementStrategy.h"
#include <iostream>
#include <stdexcept>

class Clock : public ReplacementStrategy {
public:
    Clock(int n);
    void newPage(int pageId) override;
    void pin(int pageId) override;
    void unpin(int pageId) override;
    void deletePage(int pageId) override;
    int victim() override;
    void touch(int pageId)   override;

    int getClockBit(int pageId) const {
        auto it = idx_.find(pageId);
        if (it == idx_.end()) return 0;
        return frames_[it->second].used ? 1 : 0;
    }

protected:
    struct Frame {
        int pageId; // ID of the page
        bool used;  // Used bit
        bool pinned = false;
        int pinCount;
    };
    std::vector<Frame>  frames_;
    std::unordered_map<int, int> idx_;
    int hand_; // index of actual page
    int findEmptySlot() const;
    int findSlotByPage(int pageId) const;
    int victimSlot();  // elige un slot libre y lo libera
};

Clock::Clock(int n)
    : frames_(n, Frame{ -1, false, false }), hand_(0)
{
}

void Clock::newPage(int pageId) {
    std::cout << "[DEBUG] Clock::newPage() llamado\n";
    // 1) Busca un slot vacío
    int slot = findEmptySlot();
    if (slot < 0) {
        // Buffer lleno, liberar un frame elegido por clock
        slot = victimSlot();
    }
    // Colocar la nueva página
    frames_[slot].pageId = pageId;
    frames_[slot].used = true;
    frames_[slot].pinned = false;
    idx_[pageId] = slot;
}


void Clock::pin(int pageId) {
    std::cout << "[DEBUG] Clock::pin() llamado\n";
    auto it = idx_.find(pageId);
    if (it == idx_.end()) return;
    Frame& f = frames_[it->second];
    f.pinned = true;
    f.pinCount++;
    f.used = true;
}


void Clock::unpin(int pageId) {
    std::cout << "[DEBUG] Clock::unpin() llamado\n";
    auto it = idx_.find(pageId);
    if (it == idx_.end()) return;
    Frame& f = frames_[it->second];
    //f.pinned = false;
    if (f.pinCount > 0) {
        f.pinCount--;           // NEW: decrementa contador
    }
    f.used = true;              // NEW: opcionalmente marca usado
}


void Clock::deletePage(int pageId) {
    std::cout << "[DEBUG] Clock::delete() llamado\n";
    auto it = idx_.find(pageId);
    if (it == idx_.end()) return;
    int slot = it->second;
    frames_[slot] = Frame{ -1, false, false };
    idx_.erase(it);
}

void Clock::touch(int pageId) {
    std::cout << "[DEBUG] Clock::touch() llamado\n";
    auto it = idx_.find(pageId);
    if (it == idx_.end()) return;
    frames_[it->second].used = true;
}

typedef ReplacementStrategy RS;
int Clock::victim() {
    std::cout << "[DEBUG] Clock::victim() llamado\n";
    const int n = static_cast<int>(frames_.size());
    for (int scanned = 0; scanned < 2 * n; ++scanned) {
        Frame& f = frames_[hand_];

        // 0) nunca expulsar si está 'pinned'
        if (f.pinned) {
            hand_ = (hand_ + 1) % n;
            continue;
        }

        // 1) si pinCount > 1, reducimos y pasamos al siguiente
        if (f.pageId >= 0 && f.pinCount > 1) {
            f.pinCount--;
            hand_ = (hand_ + 1) % n;
            continue;
        }

        // 2) primera pasada: pinCount≤1 y !used → expulsar
        if (f.pageId >= 0 && f.pinCount <= 1 && !f.used) {
            int victimId = f.pageId;
            idx_.erase(victimId);
            f = Frame{ -1, false, false, 0 };  // limpiar todo
            hand_ = (hand_ + 1) % n;
            return victimId;
        }

        // 3) segunda pasada: pinCount≤1 y used → limpiar used
        if (f.pageId >= 0 && f.pinCount <= 1 && f.used) {
            f.used = false;
        }

        hand_ = (hand_ + 1) % n;
    }
    std::cerr << "[ERROR] Clock::victim(): no hay frame elegible, continuando sin expulsar\n";
    return -1;
}

// Elige el índice de un frame libre usando victim()
int Clock::victimSlot() {
    // Llamamos a victim() para que libere un frame
    int oldPage = victim();
    // Como victim() ya eliminó el mapeo y vació el frame, buscamos un slot libre
    int slot = findEmptySlot();
    if (slot < 0) {
        throw std::runtime_error("Clock::victimSlot(): inconsistencia interna");
    }
    return slot;
}

// Busca slot vacío
int Clock::findEmptySlot() const {
    const int n = static_cast<int>(frames_.size());
    for (int i = 0; i < n; ++i) {
        if (frames_[i].pageId < 0) return i;
    }
    return -1;
}

int Clock::findSlotByPage(int pageId) const {
    auto it = idx_.find(pageId);
    if (it != idx_.end()) return it->second;
    // Búsqueda lineal como respaldo
    for (int i = 0, n = frames_.size(); i < n; ++i)
        if (frames_[i].pageId == pageId) return i;
    return -1;
}
