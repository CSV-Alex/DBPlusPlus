#pragma once

#include <mutex>
#include <vector>
#include <deque>
#include <unordered_map>
#include <memory>
#include <string>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <filesystem>
#include <vector>
#include <algorithm>
#include "../Disco.h"
#include "../DiscoPaths.h"
#include "../LFija.h"
#include "ReplacementStrategy.h"
#include "Clock.h"
#include "LRU.h"

#include <QtCore/QObject>


class PageWithRecords;

struct Cambios {
    enum class Tipo { Insertar, Eliminar } tipo;  // que operacion
    int registroId;                                // clave del registro
    int paginaId;                                  // id de la pagina afectada

    Cambios(Tipo t, int reg, int pag)
        : tipo(t), registroId(reg), paginaId(pag) {
    }
};

class Page {
public:
    Page(int id, Disco& disk, char op, bool pinned = false)
        : _id(id),
        _disk(disk),
        _path("BUFFERPOOL\\BLOQUES\\Page" + std::to_string(id) + ".txt"),
        _pinned(pinned), operation(op),
        _dirty(false)
    {
        loadFromDisk(disk);
        _pinCount = 0;
    }
    virtual ~Page() = default;

    int   getId() const { return _id; }
    bool  isDirty() const { return _dirty; }
    int   getPinCount() const { return _pinCount; }
    int   getPinStatus() const { return _pinned; }
    char   getOp() const { return operation; }
    size_t getLastAccess() const { return _lruAccess; }
    std::string getBufferPath() const { return _path; }

    void setPinnedPermanent(bool v) {
        _pinned = v;
    }

    void pin(char op, bool makePermanent = false) {
        ++_pinCount;
        if (op == 'W') {
            operation = 'W';
            _dirty = true;
            registrarPaginaModificada(_id);   // NUEVO: registro para flushBufferToDisk
        }
        else {
            operation = 'R';
        }
        if (makePermanent) _pinned = true;
        else _pinned = false;
        _lruAccess = ++_globalCounter;
    }

    void unpin() {
        if (_pinCount > 0) _pinCount--;
        //_pinned = false; //pin es permanente
    }

    void forceUnpin(bool saveChanges = true) {
        _pinCount = 0;
        if (saveChanges && _dirty) {
            flush(_disk);
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

    void reloadFromDisk() {
        // usa tu función interna loadFromDisk
        loadFromDisk(_disk);
        // la página ya no está “dirty” tras la recarga
        _dirty = false;
    }

protected:
    int    _id;
    Disco& _disk;
    std::string _path;
    bool   _dirty{ false };
    int    _pinCount{ 0 };
    bool   _pinned{ false };
    std::deque<std::pair<char, bool>> _ops;
    char operation; //R or W
    std::string _relation;

    static inline size_t _globalCounter = 0;            // NUEVO: contador global
    size_t _lruAccess = 0;              // NUEVO: timestamp local
    int    clock = 0;

    void loadFromDisk(Disco& disk) {
        // Construyo la ruta absoluta al bloque en DISCO\BLOQUES
        char src[MAX_PATH_LEN];
        snprintf(src, sizeof(src), "DISCO\\BLOQUES\\Bloque%d.txt", _id);

        std::ifstream in(src, std::ios::binary);
        if (!in) {
            std::cerr << "ERROR: No se pudo abrir " << src << std::endl;
            return;
        }
        std::ofstream out(_path, std::ios::binary);
        out << in.rdbuf();
    }

    void resetDirty() {
        _dirty = false;
        for (auto& op : _ops) if (op.second) { _dirty = true; break; }
    }
};


class PageWithRecords : public Page {
public:
    using Page::Page;


    bool esPagina = true;

    bool insertFixed(const std::string& relacion, const std::string& registroTxt) {
        // Operamos sobre el archivo temporal BUFFERPOOL/Page{id}.txt
        // Llamamos a la función global adicionarRegistroUnico, pero redirigimos el file
        bool ok = adicionarRegistroUnico(
            registroTxt.c_str(),
            relacion.c_str(),
            _disk, // Se copia NEW bloque físico luego de flush
            true
        );
        if (ok) {
            registrarPaginaModificada(getId());
            _dirty = true;
        }
        return ok;
    }


    bool deleteFixed(const std::string& relacion, int posicion) {
        bool ok = eliminarRegistro(
            relacion.c_str(),
            posicion,
            _disk,
            true
        );
        if (ok) {
            _dirty = true;
        }
        return ok;
    }


    bool modifyFixed(const std::string& relacion, int posicion, const std::string& nuevoRegistroTxt) {
        bool ok = modificarRegistro(
            relacion.c_str(),
            posicion,
            nuevoRegistroTxt.c_str(),
            _disk,
            true
        );
        if (ok) {
            _dirty = true;
        }
        return ok;
    }

    void viewContent() {
        std::ifstream in(_path, std::ios::binary);
        if (!in.is_open()) {
            std::cout << "ERROR: No se puede abrir " << _path << std::endl;
            return;
        }
        std::cout << "--- Contenido de Page" << _id << " ---" << std::endl;
        char c;
        while (in.get(c)) std::cout << c;
        std::cout << std::endl;
    }
};

struct Frame {
    std::unique_ptr<Page> page;
    int                    id;
    Frame(int id_) : id(id_) {}
};

class BufferPool {
public:
    BufferPool(int n_frames, 
               size_t pageBytes, 
               Disco& disk, 
               std::unique_ptr<ReplacementStrategy> replacer);

    ~BufferPool();

    Page* pinPage(int pageId, char op, bool pinned = false);
    void unpinPage(int pageId);
    Page* getPage(int pageId, char op, bool pinned = false);

    void printStats() const;
    void printEventsStatus();
    void Status();

    bool pinPermanent(int pageId);
    bool unpinPermanent(int pageId);

private:
    void publishEvent(const std::string& evt, int pageId);
    bool evictOne();
    Page* loadNewPage(int pageId, char op, bool pinned);

    Disco& _disk;
    size_t                          _pageBytes;
    int                            _n_frames; //default value 4
    std::vector<Frame>              _frames;
    std::unordered_map<int, int>     _pageTable;  // pageId → frameIdx

    std::unique_ptr<ReplacementStrategy> _replacer;        // la lista LRU de páginas

    size_t                          _hitCount{ 0 }, _totalCount{ 0 };
};

// Un mutex para serializar accesos concurrentes
inline std::mutex& getEventFileMutex() {
    static std::mutex m; return m;
}

// Append de un evento al log
inline void appendEvent(const std::string& evt, int pageId) {
    std::lock_guard<std::mutex> lk(getEventFileMutex());
    std::ofstream f("events.log", std::ios::app);
    if (f) {
        f << "[EVENT] " << evt << " " << pageId << "\n";
    }
}

// NUEVO: helper para imprimir eventos al stdout
void BufferPool::publishEvent(const std::string& evt, int pageId) {
    //appendEvent(evt, pageId);
}


void flushPageToDisk(Disco& disco, int pageId) {
    namespace fs = std::filesystem;
    // Construir rutas
    fs::path src = fs::path(bufferPagePath) / ("Page" + std::to_string(pageId) + ".txt");
    fs::path dest = fs::path(discoPath) / "BLOQUES" / ("Bloque" + std::to_string(pageId) + ".txt");

    std::error_code ec;
    // 1) Copiar (sobrescribe si existe)
    fs::copy_file(src, dest, fs::copy_options::overwrite_existing, ec);
    if (ec) {
        std::cerr << "[FLUSH ERROR] copy_file "
            << src.string() << " → " << dest.string()
            << ": " << ec.message() << "\n";
        return;
    }
    // 2) Eliminar versión temporal en BUFFERPOOL
    fs::remove(src, ec);
    if (ec) {
        std::cerr << "[FLUSH ERROR] remove "
            << src.string() << ": " << ec.message() << "\n";
    }
    // 3) Volcar sectores al block real
    disco.volcarBloqueASectores(pageId);

    // 4) Limpiar de la lista de modificadas
    paginasModificadas.erase(
        std::remove(paginasModificadas.begin(), paginasModificadas.end(), pageId),
        paginasModificadas.end()
    );

    // NUEVO: evento de flush
    cout << "[EVENT] pageFlushed " << pageId << endl;
}


bool BufferPool::pinPermanent(int pageId) {
    auto it = _pageTable.find(pageId);
    if (it == _pageTable.end()) return false;
    _frames[it->second].page->setPinnedPermanent(true);           // NUEVO: marcamos internamente
    _replacer->pin(pageId,'R', true);                              // NUEVO: avisamos a Clock
    return true;
}

bool BufferPool::unpinPermanent(int pageId) {
    auto it = _pageTable.find(pageId);
    if (it == _pageTable.end()) return false;
    _frames[it->second].page->setPinnedPermanent(false);            // NUEVO
    _replacer->unpin(pageId);                            // NUEVO
    return true;
}



bool BufferPool::evictOne() {
    std::cout << "ENTRADA a EVICT ONE" << std::endl;
    //// 1) Elegir victimId según LRU (timestamp más antiguo), saltando páginas pineadas
    //int victimId = -1;
    //size_t oldest = std::numeric_limits<size_t>::max();
    //for (auto& [pid, fidx] : _pageTable) {
    //    Page* pg = _frames[fidx].page.get();
    //    if (!pg->getPinStatus()) {
    //        size_t t = pg->getLastAccess();
    //        if (t < oldest) {
    //            oldest = t;
    //            victimId = pid;
    //        }
    //    }
    //}
    //// Si no hay víctima válida
    //if (victimId < 0) return false;

    int victimId = _replacer->victim();
    std::cout << "DEBUG: VICTIM" << victimId;
    if (victimId < 0) return false;
    int fidx = _pageTable[victimId];

    Page* victimPg = _frames[fidx].page.get();

    // 2) Si está sucia, preguntar y elegir flush o descartar
    if (victimPg->isDirty()) {
        std::cout << "Página " << victimId
            << " sucia. ¿Guardar antes de expulsar? (s/n): ";
        char r; std::cin >> r;
        if (r == 's' || r == 'S') {
            // Flush individual: copia, borra temp y volcar sectores
            flushPageToDisk(_disk, victimId);
        }
        else {
            // Descartar: eliminar sólo el archivo temporal
            std::filesystem::remove(victimPg->getBufferPath());
        }
    }
    else {
        // No está sucia → borrar directamente el archivo temporal
        std::filesystem::remove(victimPg->getBufferPath());
    }

    // 3) Expulsar la página de memoria y estructuras
    _replacer->deletePage(victimPg->getId());
    _frames[fidx].page.reset();
    _pageTable.erase(victimId);


    // NUEVO: evento de expulsión
    publishEvent("pageEvicted", victimId);

    return true;
}


Page* BufferPool::loadNewPage(int pageId, char op, bool pinned) {
    //++_totalCount;
    // Intentar crear en un frame vacío
    std::cout<<"[DEBUG] Entrda a lOADNEWPAGE"<<std::endl;
    while (true) {
        for (auto& f : _frames) {
            if (!f.page) {
                std::cout << "[DEBUG] creando PageWithRecords para id=" << pageId << "\n";
                f.page.reset(new PageWithRecords(pageId, _disk, op, pinned)); // se pasa 'op'
                f.page->pin(op, pinned);
                _pageTable[pageId] = f.id;
                _replacer->newPage(pageId,op, pinned);
                if (pinned) {
                    _replacer->newPage(pageId,op, pinned);
                }

                publishEvent("pageLoaded", pageId);

                return f.page.get();
            }
        }
        // Si no quedó espacio, expulsar uno y reintentar
        if (!evictOne()) break;
    }
    return nullptr;
}


BufferPool::BufferPool(int n_frames, size_t pageBytes, Disco& disk, std::unique_ptr<ReplacementStrategy> replacer)
    : _disk(disk), 
      _pageBytes(pageBytes), 
      _n_frames(n_frames),
      _replacer(std::move(replacer))
{
    _frames.reserve(n_frames);
    for (size_t i = 0; i < n_frames; ++i) _frames.emplace_back((int)i);
    _disk.createBufferDir(); // crea carpeta BUFFERPOOL
}

BufferPool::~BufferPool() {
    flushBufferToDisk(_disk);
}

Page* BufferPool::pinPage(int pageId, char op, bool pinned) {

    std::cout << "[DEBUG] Mensaje antes del bucle infinito" << std::endl;
    //++_totalCount;
    auto it = _pageTable.find(pageId);
    if (it != _pageTable.end()) {
        ++_hitCount;
        Page* pg = _frames[it->second].page.get();
        // --- NUEVO: solo aquí preguntamos ---
        if (pg->getOp() == 'W' && pg->isDirty()) {
            std::cout << "La página " << pageId
                << " está sucia. ¿Guardar cambios antes de continuar? (s/n): ";
            char r; std::cin >> r;
            if (r == 's' || r == 'S') {
                // flushBufferToDisk recorrerá todas las páginas modificadas
                flushPageToDisk(_disk, pageId);

            }
            else {
                // descartamos la copia temporal
                std::filesystem::remove(pg->getBufferPath());
                // opcional
                //pg->forceUnpin(false);
            }
        }
        pg->pin(op, pinned);
        _replacer->pin(pageId,op, pinned);

        // NUEVO: evento de pin
        publishEvent("pagePinned", pageId);

        return pg;
    }
    return loadNewPage(pageId, op, pinned);
}


void BufferPool::unpinPage(int pageId) {
    auto it = _pageTable.find(pageId);
    if (it == _pageTable.end()) return;
    int fidx = it->second;
    _frames[fidx].page->unpin();

    if (_frames[fidx].page->getPinCount() == 0) {
        _replacer->unpin(pageId);

        // NUEVO: evento de unpin
        publishEvent("pageUnpinned", pageId);
    }
}

Page* BufferPool::getPage(int pageId, char op, bool pinned) {
    ++_totalCount;  // NUEVO: contar todas las peticiones entrantes :contentReference[oaicite:3]{index=3}
    return pinPage(pageId, op, pinned);
}

void BufferPool::printStats() const {
    std::cout << "Requests: " << _totalCount
        << "  Hits: " << _hitCount
        << "  Hitrate(): " << std::fixed << std::setprecision(2) << float(_hitCount) / float(_totalCount) << "\n";
}

void BufferPool::printEventsStatus()
{
    std::lock_guard<std::mutex> lk(getEventFileMutex());
    std::ofstream f("events.log", std::ios::trunc);
    if (!f.is_open())
        return;

    // Cabecera para que el watcher (o tú) reconozca bloque de estado
    f << "#STATUS\n";

    for (auto& fr : _frames) {
        if (fr.page) {
            f
                << fr.id << ' '                              // Frame
                << fr.page->getId() << ' '                   // PageID
                << fr.page->isDirty() << ' '                 // Dirty
                << fr.page->getPinCount() << ' '             // PinCnt
                << fr.page->getOp() << ' '                   // OpType
                << fr.page->getLastAccess() << ' '           // LastAcc
                << fr.page->getPinStatus()                   // PinStat
                << "\n";
        }
        else {
            // frame vacío: PageID=-1
            f << fr.id << " -1 0 0 - 0 0\n";
        }
    }
}


//CLOCK
void BufferPool::Status() {
    _replacer->Status();
}



inline void registrarPaginaModificada(int nroBloque) {
    paginasModificadas.push_back(nroBloque);
}


void flushBufferToDisk(Disco& disco) {
    namespace fs = std::filesystem;

    // 1) Reemplaza PageN.txt → BloqueN.txt + volcar sectores
    for (int N : paginasModificadas) {
        fs::path src = fs::path(bufferPagePath) / ("Page" + std::to_string(N) + ".txt");
        fs::path dest = fs::path(discoPath) / "BLOQUES" / ("Bloque" + std::to_string(N) + ".txt");

        std::cout << dest << endl;
        std::cout << src << endl;
        std::cout << "Paginas en memoria: " << N << endl;

        std::error_code ec;

        // Copiamos (sobrescribe si ya existe)
        fs::copy_file(src, dest,
            fs::copy_options::overwrite_existing, ec);
        if (ec) {
            std::cerr << "[FLUSH ERROR] copy_file "
                << src.string() << " → " << dest.string()
                << ": " << ec.message() << "\n";
            continue;
        }

        // borra versión vieja
        // Borramos la página del buffer pool
        fs::remove(src, ec);
        if (ec) {
            std::cerr << "[FLUSH ERROR] remove "
                << src.string()
                << ": " << ec.message() << "\n";
            // no salimos, igual intentamos volcar sectores
        }

        disco.volcarBloqueASectores(N);  // reescribe sectores
    }
    paginasModificadas.clear();

    // 2) Pon los parches en dirBloques.txt
    fs::path dirFile = fs::path(discoPath) / "dirBloques.txt";
    FILE* f = fopen(dirFile.string().c_str(), "r+b");
    if (!f) return;

    // Para mayor seguridad, aplica en orden de offset
    sort(cambiosDirBloques.begin(), cambiosDirBloques.end(),
        [](auto& a, auto& b) { return a.first < b.first; });

    for (auto& parche : cambiosDirBloques) {
        long offset = parche.first;
        const std::string& data = parche.second;
        fseek(f, offset, SEEK_SET);
        fwrite(data.data(), 1, data.size(), f);
    }
    fflush(f);
    fclose(f);
    cambiosDirBloques.clear();
}
