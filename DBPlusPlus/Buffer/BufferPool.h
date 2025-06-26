#pragma once

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
        _lruAccess = ++_globalCounter;
    }

    void unpin() {
        if (_pinCount > 0) _pinCount--;
        _pinned = false;
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

/**
 * Clase Page extendida para soportar operaciones de registro fijo:
 * Objetivo: representar una página en memoria con capacidad de insertar,
 *           eliminar y modificar registros de longitud fija.
 * Input: identificador de página y referencia al disco.
 * Output: ninguna, modifica el contenido en memoria y marca la página dirty.
 * Autor: Alexander
 */
class PageWithRecords : public Page {
public:
    using Page::Page;


    bool esPagina = true;
    /**
     * Autor: Alex
     * Inserta un registro de longitud fija en la página.
     * @param relacion Nombre de la relación (e.g., "housing").
     * @param registroTxt Cadena con los campos separados por '#', termina con '\n'.
     * @return true si tuvo éxito, false en caso contrario.
     */
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

    /**
     * Autor: Alex
     * Elimina un registro de longitud fija en la posición global dada.
     * @param relacion Nombre de la relación.
     * @param posicion Posición global del registro a eliminar.
     * @return true si se eliminó, false si no.
     */
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

    /**
     * Modifica un registro de longitud fija en la posición dada.
     * @param relacion Nombre de la relación.
     * @param posicion Posición global.
     * @param nuevoRegistroTxt Cadena de nuevos campos separados por '#'.
     * @return true si se modificó correctamente
     * Autor: Alex
     */
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

    void viewContent() const {
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
    BufferPool(int n_frames, size_t pageBytes, Disco& disk, bool flag);
    ~BufferPool();

    Page* pinPage(int pageId, char op, bool pinned = false);
    void unpinPage(int pageId);
    Page* getPage(int pageId, char op, bool pinned = false);

    void flushPage(int pageId);
    void flushAll();

    void printStats() const;
    void Status();

private:
    bool evictOne();
    Page* loadNewPage(int pageId, char op, bool pinned);

    Disco& _disk;
    size_t                          _pageBytes;
    int                            _n_frames; //default value 4
    std::vector<Frame>              _frames;
    std::unordered_map<int, int>     _pageTable;  // pageId → frameIdx

    std::unique_ptr<ReplacementStrategy> _lru;        // la lista LRU de páginas

    size_t                          _hitCount{ 0 }, _totalCount{ 0 };
};

/**
 * Objetivo: Volcar una página individual del buffer al disco
 * Input: Disco& disco, int pageId
 * Output: Nada; escribe Bloque<pageId>.txt en disco y elimina Page<pageId>.txt
 * Autor: Alex
 */
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
}

/**
 * Objetivo: Seleccionar y expulsar una página según política LRU.
 * Input: Ninguno; usa estado interno (_pageTable, _frames, _lru).
 * Output: devuelve true si desalojó una página, false si no encontró víctima.
 * Autor: Alexander
 */
bool BufferPool::evictOne() {
    // 1) Elegir victimId según LRU (timestamp más antiguo), saltando páginas pineadas
    int victimId = -1;
    size_t oldest = std::numeric_limits<size_t>::max();
    for (auto& [pid, fidx] : _pageTable) {
        Page* pg = _frames[fidx].page.get();
        if (!pg->getPinStatus()) {
            size_t t = pg->getLastAccess();
            if (t < oldest) {
                oldest = t;
                victimId = pid;
            }
        }
    }
    // Si no hay víctima válida
    if (victimId < 0) return false;

    int   fidx = _pageTable[victimId];
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
    _frames[fidx].page.reset();
    _pageTable.erase(victimId);
    _lru->deletePage(victimId);

    return true;
}

/**
 * Autor: Alexander
 * Objetivo: Cargar una nueva página en buffer, expulsando si es necesario.
 * Input: int pageId, char op, bool pinned
 * Output: Page* puntero a la página cargada o nullptr si falla.
 */
Page* BufferPool::loadNewPage(int pageId, char op, bool pinned) {
    _totalCount++;
    for (auto& f : _frames) {
        if (!f.page) {
            std::cout << "[DEBUG] creando PageWithRecords para id=" << pageId << "\n";
            f.page.reset(new PageWithRecords(pageId, _disk, pinned));
            f.page->pin(op, pinned);
            _pageTable[pageId] = f.id;
            _lru->newPage(pageId);
            return f.page.get();
        }
    }
    if (evictOne()) return loadNewPage(pageId, op, pinned);
    return nullptr;
}


BufferPool::BufferPool(int n_frames, size_t pageBytes, Disco& disk, bool flag)
    : _disk(disk), _pageBytes(pageBytes), _n_frames(n_frames) {
    _frames.reserve(n_frames);
    for (size_t i = 0; i < n_frames; ++i) _frames.emplace_back((int)i);
    _disk.createBufferDir(); // crea carpeta BUFFERPOOL
    if (flag) {
        _lru = std::make_unique<LRU>();
    } else {
        _lru = std::make_unique<Clock>(n_frames);
    }
}
BufferPool::~BufferPool() {
    flushAll();
}


/**
 * Objetivo: Obtener (pin) una página; si existe, pregunta flush+reload, sino la trae.
 * Input: int pageId, char op, bool pinned
 * Output: Page* puntero a la página o nullptr si falla.
  * Autor: Alexander
 */
Page* BufferPool::pinPage(int pageId, char op, bool pinned) {

    std::cout << "[DEBUG] Mensaje antes del bucle infinito" << std::endl;
    ++_totalCount;
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
            // si dice 'n', descartamos; el vector paginasModificadas
            // quedará intacto hasta el próximo flush explícito
        }
        pg->reloadFromDisk();
        pg->pin(op, pinned);
        return pg;
    }
    return loadNewPage(pageId, op, pinned);
}


void BufferPool::unpinPage(int pageId) {
    auto it = _pageTable.find(pageId);
    if (it == _pageTable.end()) return;
    int fidx = it->second;
    _frames[fidx].page->unpin();
    _lru->unpin(pageId);
}

Page* BufferPool::getPage(int pageId, char op, bool pinned) {
    return pinPage(pageId, op, pinned);
}

/**
 * Autor: Alexander
 * Objetivo: Volcar una sola página al disco (copia y borra temp + volcar sectores)
 * Input: Disco& disco, int pageId
 * Output: Ninguno; escribe Bloque<pageId>.txt y elimina Page<pageId>.txt
 */
void BufferPool::flushPage(int pageId) {
    auto it = _pageTable.find(pageId);
    if (it == _pageTable.end()) return;
    _frames[it->second].page->flush(_disk);
}

/**
 * Autor: Alexander
 * Objetivo: Volcar todas las páginas sucias del buffer al disco
 * Input: Ninguno
 * Output: Ninguno; recorre frames y llama flush() en cada página dirty
 */
void BufferPool::flushAll() { //indiscriminate
    for (auto& f : _frames)
        if (f.page && f.page->isDirty())
            f.page->flush(_disk);
}

/**
 * Autor: Alex
 * Objetivo: Mostrar estadísticas de accesos y aciertos
 * Input: Ninguno
 * Output: Imprime Requests, Hits y Hit rate
 */
void BufferPool::printStats() const {
    std::cout << "Requests: " << _totalCount
        << "  Hits: " << _hitCount
        << "  Hitrate(): " << std::fixed << std::setprecision(2) << float(_hitCount) / float(_totalCount) << "\n";
}

/**
 * Autor: Alex
 * Objetivo: Mostrar estado de cada frame (ID, página, dirty, pinCount, op, lastAccess, pinStatus)
 * Input: Ninguno
 * Output: Imprime tabla de estado
 */
void BufferPool::Status() {
    std::cout << "| Frame | PageID | Dirty | PinCnt | OpType | LastAcc | PinStat |\n";
    for (auto& f : _frames) {
        if (f.page) {
            std::cout << "| " << std::setw(5) << f.id
                << " | " << std::setw(6) << f.page->getId()
                << " | " << std::setw(5) << f.page->isDirty()
                << " | " << std::setw(6) << f.page->getPinCount()
                << " | " << std::setw(6) << f.page->getOp()
                << " | " << std::setw(7) << f.page->getLastAccess()  // NUEVO: mostrar timestamp
                << " | " << std::setw(7) << f.page->getPinStatus()
                << " |\n";
        }
        else {
            std::cout << "| " << std::setw(5) << f.id << " |   -    |   0   |   0    |   -    |    0    |    0    |\n";
        }
    }
}

inline void registrarPaginaModificada(int nroBloque) {
    paginasModificadas.push_back(nroBloque);
}

/**
 * Autor: Alex
 * Objetivo: Volcar todas las páginas registradas en paginasModificadas al disco
 * Input: Disco& disco
 * Output: Copia y borra cada PageN.txt, revierte cambios en dirBloques.txt
 */
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
