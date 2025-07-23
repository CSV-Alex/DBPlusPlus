#pragma once

#include <iostream>
#include <vector>
#include <list>
#include <queue>           // <–– añadido
#include <unordered_map>
#include <limits>
#include <iomanip>
#include <string>

struct Frame {
    char   page       = '-';
    bool   dirty      = false;
    int    opType     = 0;    // 0 lectura, 1 escritura
    int    pinCount   = 0;
    int    pinStatus  = 0;    // estado de pineo externo
    int    clock      = 0;    // bit de segunda oportunidad
    std::queue<int> opQueue;  // <–– cola de operaciones por frame
};

class BufferPool {
private:
    int _capacity;
    float _hitRate[2];
    std::vector<Frame> _frames;
    int hand_ = 0; // «manecilla» del reloj
public:
    BufferPool(int);
    bool access(char p, int op, int pinStatus);
    void printFrames();
    void printStats();
};

BufferPool::BufferPool(int capacity)
  : _capacity(capacity)
{
    _hitRate[0] = _hitRate[1] = 0.0f;
    _frames.resize(_capacity);
}

bool BufferPool::access(char p, int op, int pinStatus) {
    if (pinStatus == -48) pinStatus = 0; // corrección de char a int

    // 1) Primero buscamos un espacio libre o un hit
    for (size_t i = 0; i < _frames.size(); ++i) {
        Frame &f = _frames[i];
        if (f.page == '-') {
            // cargar nueva página
            f.page      = p;
            f.opType    = op;
            f.pinCount += 1;
            f.pinStatus = pinStatus;
            f.clock     = 1;           // bit de referencia en 1
            f.dirty     = (op == 1);
            f.opQueue.push(op);        // <–– registramos la operación
            _hitRate[1]++;
            return true;
        }
        if (f.page == p) {
            // hit
            f.opType    = op;
            f.pinCount += 1;
            f.dirty     = (op == 1) ? true : f.dirty;
            f.pinStatus = pinStatus;
            f.opQueue.push(op);        // <–– registramos la operación
            _hitRate[0]++;
            _hitRate[1]++;
            return true;
        }
    }

    // 2) No hay espacio libre ni hit → buscamos víctima con algoritmo reloj
    int victim = -1;
    while (victim == -1) {
        Frame &f = _frames[hand_];

        if (f.clock == 1) {
            // segunda oportunidad: sólo reducimos pinCount si no está 'pineada'
            if (f.pinStatus == 0 && f.pinCount > 0) {
                if (!f.opQueue.empty()) {
                    std::cout << "Procesando operación " 
                              << f.opQueue.front()
                              << " en página " << f.page << std::endl;
                    f.opQueue.pop();
                }
                f.pinCount -= 1;
                if (f.pinCount == 0) {
                    f.clock = 0;  // ya no referenciada
                }
            }
            else if (f.pinCount == 0) {
                // sin pins y con bit de reloj = 1 → se limpia bit
                f.clock = 0;
            }
            // si está pineada (pinStatus!=0) y pinCount>0, saltamos sin tocar nada
        }
        else if (f.clock == 0 && f.pinCount == 0 && f.pinStatus == 0) {
            // candidata final
            victim = hand_;
        }

        hand_ = (hand_ + 1) % _capacity;
    }

    // 3) Tenemos víctima
    Frame &vf = _frames[victim];
    if (vf.dirty) {
        std::cout << "Se escribe y descarta: " << vf.page << std::endl;
    }
    // reiniciamos estado del frame
    while (!vf.opQueue.empty()) vf.opQueue.pop();
    vf.page      = p;
    vf.opType    = op;
    vf.pinCount  = 1;
    vf.pinStatus = pinStatus;
    vf.clock     = 1;
    vf.dirty     = (op == 1);
    vf.opQueue.push(op); // registro de esta primera operación
    _hitRate[1]++;

    return true;
}

void BufferPool::printFrames() {
    std::cout << "|"<<std::setw(10)<<"Frame"
              <<"|"<<std::setw(10)<<"PageID"
              <<"|"<<std::setw(10)<<"Dirty"
              <<"|"<<std::setw(10)<<"pinCount"
              <<"|"<<std::setw(10)<<"OpType"
              <<"|"<<std::setw(15)<<"Pin Status"
              <<"|"<<std::setw(10)<<"Clock"
              <<"|"<<std::endl;
    for (size_t i = 0; i < _frames.size(); ++i) {
        Frame &f = _frames[i];
        std::cout << "|"<<std::setw(10)<<i
                  <<"|"<<std::setw(10)<<f.page
                  <<"|"<<std::setw(10)<<f.dirty
                  <<"|"<<std::setw(10)<<f.pinCount
                  <<"|"<<std::setw(10)<<f.opType
                  <<"|"<<std::setw(15)<<f.pinStatus
                  <<"|"<<std::setw(10)<<f.clock
                  <<"|"<<std::endl;
    }
}

void BufferPool::printStats() {
    double rate = (_hitRate[1] > 0)
        ? (_hitRate[0] / _hitRate[1] * 100.0)
        : 0.0;
    std::cout << "Number of Hits: " << _hitRate[0]
              << "  Total Calls: "  << _hitRate[1]
              << "  Hit Rate: "     << std::fixed << std::setprecision(2)
              << rate << "%" << std::endl;
}