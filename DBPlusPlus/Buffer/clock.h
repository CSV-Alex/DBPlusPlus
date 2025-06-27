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
    //f.used = true;
}


void Clock::unpin(int pageId) {
    std::cout << "[DEBUG] Clock::unpin() llamado\n";
    auto it = idx_.find(pageId);
    if (it == idx_.end()) return;
    Frame& f = frames_[it->second];
    f.pinned = false;
    //f.used = true;
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

        // 1ª pasada: expulsar si nunca fue usado y no está pineado
        if (f.pageId >= 0 && !f.pinned && !f.used) {
            int oldId = f.pageId;
            f = Frame{ -1, false, false };          // MODIFICADO: limpiar frame :contentReference[oaicite:0]{index=0}
            idx_.erase(oldId);                      // MODIFICADO: quitar del índice
            hand_ = (hand_ + 1) % n;                // MODIFICADO: avanzar la mano
            return oldId;                           // MODIFICADO: devolver víctima
        }
        // 2ª pasada: si estaba marcado como usado, limpiar bit y darle otra vuelta
        if (f.pageId >= 0 && !f.pinned && f.used) {
            f.used = false;                        // MODIFICADO: limpiar bit used
        }

        hand_ = (hand_ + 1) % n;
    }
    throw std::runtime_error("Clock::victim(): no hay frame elegible");  // inalterado
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
