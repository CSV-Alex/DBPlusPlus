#pragma once

#include <vector>
#include <deque>
#include <unordered_map>
#include <memory>
#include <string>
#include <fstream>
#include <iostream>
#include <list>

#include "Disco.h"
#include "DiscoPaths.h"

struct Cambios;

class LRUReplacer {
public:
    void newPage(int pageId) { touch(pageId); }
    void pin(int pageId) { touch(pageId); }
    void unpin(int pageId) { touch(pageId); }
    void deletePage(int pageId) {
        auto it = pos.find(pageId);
        if (it != pos.end()) {
            lru.erase(it->second);
            pos.erase(it);
        }
    }
    int victim() {
        if (lru.empty()) return -1;
        return lru.front();
    }
private:
    std::list<int> lru;
    std::unordered_map<int, std::list<int>::iterator> pos;
    void touch(int pageId) {
        auto it = pos.find(pageId);
        if (it != pos.end()) {
            lru.erase(it->second);
            pos.erase(it);
        }
        lru.push_back(pageId);
        pos[pageId] = std::prev(lru.end());
    }
};


class Page {
public:
    Page(int id, Disco& disk, bool pinned = false)
        : _id(id),
        _path("BUFFERPOOL/Page" + std::to_string(id) + ".txt"),
        _pinned(pinned)
    {
        loadFromDisk(disk);
        _pinCount = 1;
    }
    ~Page() = default;

    int   getId() const { return _id; }
    bool  isDirty() const { return _dirty; }
    int   getPinCount() const { return _pinCount; }

    void pin(char op, bool makePermanent = false) {
        _pinCount++;
        bool isWrite = (op == 'W');
        _ops.emplace_back(op, isWrite);
        updateDirty();
        if (makePermanent) _pinned = true;
    }
    void unpin() {
        if (_pinCount > 0) _pinCount--;
        if (!_ops.empty()) {
            _ops.pop_front();
            updateDirty();
        }
    }
    void forceUnpin(bool saveChanges = true) {
        _pinCount = 0;
        if (saveChanges && _dirty) {
            // disk.addChanges
        }
        _ops.clear();
        _dirty = false;
    }
    void flush(Disco& disk) {
        std::ifstream in(_path, std::ios::binary);
        std::ofstream out(disk.getBloquePath(_id), std::ios::binary);
        out << in.rdbuf();
        disk.volcarBloqueASectores(_id);
        _dirty = false;
    }

private:
    int    _id;
    std::string _path;
    bool   _dirty{ false };
    int    _pinCount{ 0 };
    bool   _pinned{ false };
    std::deque<std::pair<char, bool>> _ops;
    std::vector<std::pair<std::string, std::vector<Cambios>>> _changes;
    std::string _relation;

    void loadFromDisk(Disco& disk) {
        std::ifstream in(disk.getBloquePath(_id), std::ios::binary);
        std::ofstream out(_path, std::ios::binary);
        out << in.rdbuf();
    }
    void updateDirty() {
        _dirty = false;
        for (auto& op : _ops) if (op.second) { _dirty = true; break; }
    }
};

struct Frame {
    std::unique_ptr<Page> page;
    int                    id;
    Frame(int id_) : id(id_) {}
};

class BufferPool {
public:
    BufferPool(size_t bufBytes, size_t pageBytes, Disco& disk)
        : _disk(disk), _bufBytes(bufBytes), _pageBytes(pageBytes)
    {
        size_t nFrames = bufBytes / pageBytes;
        _frames.reserve(nFrames);
        for (size_t i = 0; i < nFrames; ++i) _frames.emplace_back((int)i);
        _disk.createBufferDir(); // crea carpeta BUFFERPOOL
    }
    ~BufferPool() {
        flushAll();
    }

    Page* pinPage(int pageId, char op, bool pinned = false) {
        _reqCount++;
        auto it = _pageTable.find(pageId);
        if (it != _pageTable.end()) {
            int fidx = it->second;
            _hitCount++;
            _frames[fidx].page->pin(op, pinned);
            _lru.pin(pageId);
            return _frames[fidx].page.get();
        }
        return loadNewPage(pageId, op, pinned);
    }
    void unpinPage(int pageId) {
        auto it = _pageTable.find(pageId);
        if (it == _pageTable.end()) return;
        int fidx = it->second;
        _frames[fidx].page->unpin();
        _lru.unpin(pageId);
    }
    Page* getPage(int pageId, char op, bool pinned = false) {
        return pinPage(pageId, op, pinned);
    }

    void flushPage(int pageId) {
        auto it = _pageTable.find(pageId);
        if (it == _pageTable.end()) return;
        _frames[it->second].page->flush(_disk);
    }
    void flushAll() {
        for (auto& f : _frames)
            if (f.page && f.page->isDirty())
                f.page->flush(_disk);
    }

    void printStats() const {
        std::cout << "Requests: " << _reqCount
            << "  Hits: " << _hitCount
            << "  Misses: " << _missCount << "\n";
    }
    void printBuffer() const {
        std::cout << "---- BufferPool State ----\n";
        for (auto& f : _frames) {
            if (f.page) {
                std::cout << "Frame " << f.id
                    << " => Page " << f.page->getId()
                    << " | dirty=" << f.page->isDirty()
                    << " | pins=" << f.page->getPinCount()
                    << "\n";
            }
            else {
                std::cout << "Frame " << f.id << " [vacío]\n";
            }
        }
    }

private:
    bool evictOne() {
        int victim = _lru.victim();
        if (victim < 0) return false;
        int fidx = _pageTable[victim];
        auto& P = _frames[fidx].page;
        if (P->getPinCount() > 0) return false;
        if (P->isDirty()) P->flush(_disk);
        _pageTable.erase(victim);
        P.reset();
        _lru.deletePage(victim);
        return true;
    }
    Page* loadNewPage(int pageId, char op, bool pinned) {
        _missCount++;
        for (auto& f : _frames) {
            if (!f.page) {
                f.page.reset(new Page(pageId, _disk, pinned));
                f.page->pin(op, pinned);
                _pageTable[pageId] = f.id;
                _lru.newPage(pageId);
                return f.page.get();
            }
        }
        if (evictOne()) return loadNewPage(pageId, op, pinned);
        return nullptr;
    }

    Disco& _disk;
    size_t                          _bufBytes;
    size_t                          _pageBytes;
    std::vector<Frame>              _frames;
    std::unordered_map<int, int>     _pageTable;  // pageId → frameIdx

    LRUReplacer                     _lru;        // único reemplazador

    size_t                          _hitCount{ 0 }, _missCount{ 0 }, _reqCount{ 0 };
};
