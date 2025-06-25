#pragma once
#include <list>
#include <unordered_map>

/**
 * Autor: Alexander
 * Objetivo: Implementar política de reemplazo LRU (Least Recently Used) para páginas.
 * Input: Métodos utilizan el identificador de página.
 * Output: Gestión interna de la lista LRU y selección de víctima.
 */
class LRUReplacer {
public:
    void newPage(int pageId) { touch(pageId); } //no es exactamente una nueva pagina, solo la toca
    void pin(int pageId) { touch(pageId); }
    void unpin(int pageId) { touch(pageId); }
    void deletePage(int pageId);
    int getpos(int pageId);
    int victim()   ;
private:
    std::list<int> lru;
    std::unordered_map<int, std::list<int>::iterator> pos;
    void touch(int pageId);
};

/**
 * Autor: Alexander
 * Objetivo: Borrar una página de las estructuras LRU.
 * Input: int pageId
 * Output: Ninguno
 */
void LRUReplacer::deletePage(int pageId) {
    auto it = pos.find(pageId);
    if (it != pos.end()) {
        lru.erase(it->second);
        pos.erase(it);
    }
}

/**
 * Autor: Alexander
 * Objetivo: Devolver la página víctima (menos recientemente usada).
 * Input: Ninguno
 * Output: ID de la página al frente o -1 si la lista está vacía
 */
int LRUReplacer::victim() { //first element from LRU list.
    if (lru.empty()) return -1;
    return lru.front();
}

/**
 * Autor: Alexander
 * Objetivo: Reposicionar la página al final de la lista LRU.
 * Input: int pageId
 * Output: Ninguno
 */
void LRUReplacer::touch(int pageId) { //reposiciona la pagina al final de la lista
    auto it = pos.find(pageId);
    if (it != pos.end()) {
        lru.erase(it->second);
        pos.erase(it);
    }
    lru.push_back(pageId);
    pos[pageId] = std::prev(lru.end());
}


/**
 * Autor: Alexander
 * Objetivo: Calcular la posición de una página en la lista LRU.
 * Input: int pageId
 * Output: Distancia desde el frente o -1 si no existe
 */
int LRUReplacer::getpos(int pageId) {
    auto it = pos.find(pageId);
    if (it != pos.end()) {
        return std::distance(lru.begin(), it->second);
    }
    return -1; //not found
}