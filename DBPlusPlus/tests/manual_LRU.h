#pragma once

#include <iostream>
#include <vector>
#include <limits>
#include <iomanip>
#include <string>
#include <queue>

struct Frame { // PIN STATUS and operation queue per frame
    char   page         = '-';
    bool   dirty        = false;
    int    opType       = 0;    // 0 lectura, 1 escritura
    int    pinCount     = 0;
    int    lastAccessed = 0;
    int    pin_status   = 0;
    std::queue<std::pair<int,int>> opQueue; // pair<opType, pinStatus>
};

class BufferPool{
private:
    int _capacity;
    float _hitRate[2];
    std::vector<Frame> _frames;
    int _op_order = 1; // simula numero de operacion
public:
    BufferPool(int);
    bool access(char p, int op, int pinStatus);
    void printFrames();
    void printStats();
};

BufferPool::BufferPool(int capacity): _capacity(capacity) {
    _hitRate[0] = 0.0f;
    _hitRate[1] = 0.0f;
    _frames.resize(_capacity);
}

bool BufferPool::access(char p, int op, int pinStatus) {
    if(pinStatus == -48) pinStatus = 0;
    // inserción o hit
    for (auto &f : _frames) {
        if (f.page == '-') {
            // slot libre
            _op_order++;
            f.page = p;
            f.opType = op;
            f.pinCount = 1;
            f.lastAccessed = _op_order;
            f.pin_status = pinStatus;
            f.dirty = (op == 1);
            // limpiar cola y agregar operación
            std::queue<std::pair<int,int>> empty;
            std::swap(f.opQueue, empty);
            f.opQueue.push({op, pinStatus});
            _hitRate[1]++;
            return true;
        }
        if (f.page == p) {
            // hit en pool
            _op_order++;
            f.opType = op;
            f.pinCount++;
            f.pin_status = pinStatus;
            if (op == 1) f.dirty = true;
            f.lastAccessed = _op_order;
            f.opQueue.push({op, pinStatus});
            _hitRate[0]++;
            _hitRate[1]++;
            return true;
        }
    }

    // búsqueda de víctima LRU sin sort
    int victim = -1;
    bool processed;
    do {
        processed = false;
        int minTime = std::numeric_limits<int>::max();
        // buscar candidato y procesar pendientes
        for (int i = 0; i < _capacity; ++i) {
            Frame &f = _frames[i];
            if (f.pin_status != 0) continue;
            if (f.pinCount > 1) {
                std::cout<< "resolviendo op pendiente en frame "<< f.opQueue.front().first << std::endl;
                f.opQueue.pop();
                f.pinCount--;
                _op_order++;
                f.lastAccessed = _op_order;
                processed = true;
                break; // reiniciar búsqueda
            }
            // posible candidato
            if (f.pinCount == 1 && f.lastAccessed < minTime) {
                minTime = f.lastAccessed;
                victim = i;
            }
        }
    } while (processed);

    if (victim < 0) {
        std::cout << "Error: todos los frames están ocupados o tienen pinStatus != 0" << std::endl;
        return false;
    }

    // reemplazar víctima
    Frame &vf = _frames[victim];
    if (vf.dirty) std::cout << "Se escribe y descarta: " << vf.page << std::endl;
    _op_order++;
    vf.page = p;
    vf.opType = op;
    vf.pinCount = 1;
    vf.lastAccessed = _op_order;
    vf.pin_status = pinStatus;
    vf.dirty = (op == 1);
    std::queue<std::pair<int,int>> empty;
    std::swap(vf.opQueue, empty);
    vf.opQueue.push({op, pinStatus});
    _hitRate[1]++;
    return true;
}

void BufferPool::printFrames() {
    std::cout   <<"|"<<std::setw(10)<< "Frame"<<"|"<<std::setw(10)<< "PageID"
                <<"|"<<std::setw(10)<< "Dirty" <<"|"<<std::setw(10)<< "pinCount"
                <<"|"<<std::setw(10)<< "OpType" <<"|"<<std::setw(15)<< "LastAccessed"
                <<"|"<<std::setw(10)<< "Pin Status" <<"|"<< std::setw(15)<< "QueueSize" <<"|"<< std::endl;
    for (size_t i = 0; i < _frames.size(); ++i) {
        const Frame& f = _frames[i];
        std::cout   <<"|"<<std::setw(10)<< i<<"|"<<std::setw(10)<< f.page
                <<"|"<<std::setw(10)<< f.dirty <<"|"<<std::setw(10)<< f.pinCount
                <<"|"<<std::setw(10)<< f.opType <<"|"<<std::setw(15)<< f.lastAccessed
                <<"|"<<std::setw(10)<<f.pin_status<<"|"<<std::setw(15)<<f.opQueue.size()<<"|"<< std::endl;
    }
}

void BufferPool::printStats() {
    double rate = _hitRate[0]/_hitRate[1] * 100.0;
    std::cout <<"Number of Hits:    " << _hitRate[0]
              <<" Total Calls:       " << _hitRate[1]
              <<" Hit Rate:         " 
              << std::fixed << std::setprecision(2)
              << rate << "%" << "\n";
}
