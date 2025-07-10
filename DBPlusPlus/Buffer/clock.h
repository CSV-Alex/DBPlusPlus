#pragma once
#include <vector>
#include <queue>
#include <unordered_map>
#include "replacementStrategy.h"
#include <iostream>
#include <iomanip>
#include <stdexcept>

class Clock : public ReplacementStrategy {
public:
    Clock(int n);
    void newPage(int pageId,char op, bool) override;
    void pin(int pageId,char op, bool) override;
    void unpin(int pageId) override;
    void deletePage(int pageId) override;
    int victim() override;
    void touch(int pageId)   override;
    void setPermanentPin(int pageId, bool value);

    void Status() override;

    int getClockBit(int pageId) const {
        auto it = idx_.find(pageId);
        if (it == idx_.end()) return 0;
        return frames_[it->second].REF_bit ? 1 : 0;
    }

protected:
    struct Frame {
        int pageId; // ID of the page
        int pinCount = 0;
        bool pin_status = false;
        bool REF_bit = false; // used bit
    };

    int capacity_;
    std::vector<Frame>  frames_;
    std::unordered_map<int, int> idx_;
    int hand_; // index of actual page
    bool full, locked; //full tracks wether buffer is full, locked tracks if the no possible victim in buffer is unpinned

    std::vector<std::queue<char>> local_queues;

    int findEmptySlot() const;
    int findSlotByPage(int pageId) const;
    int victimSlot();  // elige un slot libre y lo libera
};

Clock::Clock(int n){
    capacity_=n;
    frames_.resize(n, Frame{ -1, 0, false, false });
    hand_ = 0; 
    full = false; 
    locked = false; 
    local_queues.resize(n); // inicializa las colas locales para cada frame
}

void Clock::newPage(int pageId, char op,  bool pinned) {
    std::cout << "[DEBUG] Clock::newPage() llamado\n";
    /*// 1) Busca un slot vacío
    int slot = findEmptySlot();
    if (slot < 0) {
        // Buffer lleno, liberar un frame elegido por clock
        slot = victimSlot();
    }
    */
    int slot; 
    if(!full){
        slot=hand_;
        hand_=(++hand_)%frames_.size(); //avanzamos el puntero
        if(hand_==0) full=true; // si llegamos al final, marcamos como lleno
    }
    // Colocar la nueva página
    frames_[slot].pageId = pageId;
    frames_[slot].pinCount = 1; // contador de pins
    frames_[slot].pin_status = pinned; // marca como pineado
    frames_[slot].REF_bit = true;
    idx_[pageId] = slot;

    local_queues[slot].push(op); // almacenar la operación en la cola local

}


void Clock::pin(int pageId, char op, bool pinned) {
    std::cout << "[DEBUG] Clock::pin() llamado\n";
    auto it = idx_.find(pageId);
    if (it == idx_.end()) return; //not found
    Frame& f = frames_[it->second];
    f.pinCount++;
    f.pin_status = pinned; // marca como pineado

    local_queues[it->second].push(op); // almacenar la operación en la cola local

    f.REF_bit = true;


}
void Clock::setPermanentPin(int pageId, bool value) {
    std::cout << "[DEBUG] Clock::setPermanentPin() llamado\n";
    auto it = idx_.find(pageId);
    if (it == idx_.end()) return; //not found
    frames_[it->second].pin_status = value; // marca como pineado permanentemente
    local_queues[it->second].push(value ? 'P' : 'U'); // almacenar la operación en la cola local
}   
void Clock::unpin(int pageId) { //only method to get out of locked
    std::cout << "[DEBUG] Clock::unpin() llamado\n";
    auto it = idx_.find(pageId);
    if (it == idx_.end()) return;
    Frame& f = frames_[it->second];

    f.pinCount=0;
    f.pin_status=false;
    f.REF_bit=0;

    while(!local_queues[it->second].empty()) {
        local_queues[it->second].pop(); // clear the local queue
    }
    
    locked=false;
}


void Clock::deletePage(int pageId) {
    std::cout << "[DEBUG] Clock::delete() llamado\n";
    auto it = idx_.find(pageId);
    if (it == idx_.end()) return;
    int slot = it->second;

    while(!local_queues[slot].empty()) {
        local_queues[slot].pop(); // clear the local queue
    }

    frames_[slot] = Frame{ -1,0, false, false };
    idx_.erase(it);

}

void Clock::touch(int pageId) {
    std::cout << "[DEBUG] Clock::touch() llamado\n";
    auto it = idx_.find(pageId);
    if (it == idx_.end()) return;
    frames_[it->second].REF_bit = true;
}

int Clock::victim() {
    
    //std::cout << "[DEBUG] Clock::victim() llamado\n";
    
    const int n = static_cast<int>(frames_.size());

    int n_locked = 0; // contador de frames bloqueados

    while(!locked){  //doesn't use n_blocked reset, in order to favor some iterations
        Frame& f = frames_[hand_];

        // 1) Fase de consumo de pins temporales
        if (f.pageId >= 0 && f.pinCount > 0) {
            f.pinCount--;
            local_queues[hand_].pop(); // eliminar la operación de la cola local
            hand_ = (hand_ + 1) % n;
            continue;
        }

        // 2) Fase de segunda oportunidad: limpiar REF_bit
        if (f.pageId >= 0 && f.pinCount == 0 && f.REF_bit) {
            f.REF_bit = false;
            hand_ = (hand_ + 1) % n;
            continue;
        }

        // 3) Fase de expulsión: sin pins ni REF_bit
        if (f.pageId >= 0 && !f.REF_bit && !f.pin_status) {
            int victimId = f.pageId;
            idx_.erase(victimId);
            // reset completo del frame
            f = Frame{ -1, false, false };
            hand_ = (hand_ + 1) % n;
            return victimId; // devuelve el ID de la página expulsada
        }

        // Si llegamos aquí, significa que el frame está bloqueado, acumulativo, no exactament econsecutivo.
        n_locked++;
        if (n_locked >= n) {
            locked = true; // si hemos recorrido todos los frames y están bloqueados, salimos del bucle
        }
        
        // avanza el puntero
        hand_ = (hand_ + 1) % n;
    }

    std::cerr << "[ERROR] Clock::victim(): no hay frame elegible\n";
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


void Clock::Status() {
    std::cout << "|"<<std::setw(10)<<"Frame"
              <<"|"<<std::setw(10)<<"PageID"
              <<"|"<<std::setw(10)<<"OpType"
              <<"|"<<std::setw(10)<<"Dirty"
              <<"|"<<std::setw(10)<<"Pincount"
              <<"|"<<std::setw(15)<<"Pin Status"
              <<"|"<<std::setw(10)<<"Clock"
              <<"|"<<std::setw(15)<<"Queue"
              <<"|"<<std::endl;
    for (int idx=0;frames_.size(); ++idx) {
        Frame &f = frames_[idx];
            std::cout << "| " << std::setw(5) << idx;
            if(f.pageId != -1) {
                std::cout<< " | " << std::setw(10) << f.pageId
                << " | " << std::setw(10) << local_queues[idx].front() // Assuming the front of the local queue is the operation type
                << " | " << std::setw(10) << (local_queues[idx].front()=='W' ? "1" : "0")               
                << " | " << std::setw(10) << f.pinCount
                << " | " << std::setw(15) << f.pin_status
                << " | " << std::setw(10) << f.REF_bit
                << " | " << std::setw(15) << printQueue(idx, local_queues[idx])
                << " |\n";
            }
            else {
                std::cout<< " | " << std::setw(10) << "-1"
                << " | " << std::setw(10) << "-"
                << " | " << std::setw(10) << "-"            
                << " | " << std::setw(10) << "-"
                << " | " << std::setw(15) << "-"
                << " | " << std::setw(10) << "0"
                << " | " << std::setw(15) << "[Empty]"
                << " |\n";
            }
        }
}