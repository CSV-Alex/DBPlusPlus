#pragma once

#include <iostream>
#include <vector>
#include <list>
#include <unordered_map>
#include <limits>
#include <iomanip>
#include <string>

struct Frame { //PIN STATUS in table
    char   page         = '-';
    bool   dirty        = false;
    int    opType       = 0;    // 0 lectura, 1 escritura
    int    pinCount     = 0;
    int    pinStatus = 0; //pin status
    int    clock = 0; //replaces pin status

};

class BufferPool{
    private:
        int _capacity;
        float _hitRate[2];
        std::vector<Frame> _frames;
        int hand_ = 0; // index of actual page
    public:
        BufferPool(int);
        bool access(char p, int op, int pinStatus);
        void printFrames();
        void printStats();
};

BufferPool::BufferPool(int capacity): _capacity(capacity){
    _hitRate[0] = 0.0f;
    _hitRate[1] = 0.0f;
    _frames.resize(_capacity);
}

bool BufferPool::access(char p, int op, int pinStatus) {
    if(pinStatus== -48){
        pinStatus = 0; //si es un char, lo convierto a int
    }
    for (size_t i = 0; i < _frames.size(); ++i) {
        if (_frames[i].page == '-') { //espacio libre

            _frames[i].page = p;
            _frames[i].opType = op;
            _frames[i].pinCount++;
            _frames[i].pinStatus = pinStatus;
            _frames[i].clock = 1;
            if (op == 1) _frames[i].dirty = true; //escritura
            _hitRate[1]++;

            return true;
        }
        if(_frames[i].page == p) { //ya esta en el pool
            _frames[i].opType = op;
            _frames[i].pinCount++;
            if (op == 1) _frames[i].dirty = true; //escritura
            _frames[i].pinStatus = pinStatus;
            _hitRate[0]++;
            _hitRate[1]++;

            return true; //acceso exitoso
        }
    }

    int victim = -1;
    while(victim == -1) {
        if (_frames[hand_].pinCount > 0 && _frames[hand_].clock==1) { //si la pagina esta en uso, no se puede reemplazar
            _frames[hand_].pinCount --;
            if(_frames[hand_].pinCount == 0) { //si no hay mas pines, se puede reemplazar
                _frames[hand_].clock = 0; //marca el bit de reloj como 0
            }
            hand_ = (hand_ + 1) % _capacity; //avanza al siguiente frame 
        } 
        else if (_frames[hand_].clock == 1 && _frames[hand_].pinCount==0) { //si el bit de reloj es 1, se reemplaza
            _frames[hand_].clock = 0; //marca el bit de reloj como 0
            hand_ = (hand_ + 1) % _capacity; //avanza al siguiente frame
        }
        else if (_frames[hand_].clock == 0 && _frames[hand_].pinCount==0 && _frames[hand_].pinStatus==0) { //si el bit de reloj es 0 y no hay pines, se reemplaza
            victim = hand_; //se guarda el frame a reemplazar
        } 
        else {
            std::cout << "Error: No se pudo encontrar un frame para reemplazar" << std::endl;
            return false; //error al encontrar un frame para reemplazar  
        }
        hand_ = (hand_ + 1) % _capacity; //avanza al siguiente frame
    }

    if (victim < 0) {
        std::cout << "Todas las paginas estan en uso, se debe escribir una para reemplazar" << std::endl;
        return false; //no se pudo reemplazar
    }
    if(_frames[victim].dirty) { //si la pagina es sucia, se escribe
        std::cout << "Se escribe y descarta: " << _frames[victim].page << std::endl;
    }

    //reemplazo

    _frames[victim].page = p;
    _frames[victim].opType = op;
    _frames[victim].pinCount = 1;
    _frames[victim].clock = pinStatus;
    if (op == 1) _frames[victim].dirty = true; //escritura
    if (op == 0) _frames[victim].dirty = false; //escritura
    _hitRate[1]++;

    return true; 
}

void BufferPool::printFrames() {
    std::cout   <<"|"<<std::setw(10)<< "Frame"<<"|"<<std::setw(10)<< "PageID"
                <<"|"<<std::setw(10)<< "Dirty" <<"|"<<std::setw(10)<< "pinCount"
                <<"|"<<std::setw(10)<< "OpType" <<"|"<<std::setw(15)<< "Pin Status"
                <<"|"<<std::setw(10)<< "Clock" <<"|" << std::endl;
    for (size_t i = 0; i < _frames.size(); ++i) {
        const Frame& f = _frames[i];
        std::cout   <<"|"<<std::setw(10)<< i<<"|"<<std::setw(10)<< f.page
                <<"|"<<std::setw(10)<< f.dirty <<"|"<<std::setw(10)<< f.pinCount
                <<"|"<<std::setw(10)<< f.opType <<"|"<<std::setw(15)<< f.pinStatus
                <<"|"<<std::setw(10)<<f.clock<<"|"<< std::endl;
    }
}

void BufferPool::printStats() {//HitRATE
    double rate = _hitRate[0]/_hitRate[1] * 100.0;
    std::cout <<"Number of Hits:    " << _hitRate[0]   <<
                " Total Calls:       " << _hitRate[1] << 
                " Hit Rate:         " 
                << std::fixed << std::setprecision(2)
                << rate << "%" << "\n";
}