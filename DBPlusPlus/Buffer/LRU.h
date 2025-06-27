#pragma once
#include <list>
#include <unordered_map>

/**
 * Autor: Alexander
 * Objetivo: Implementar pol�tica de reemplazo LRU (Least Recently Used) para p�ginas.
 * Input: M�todos utilizan el identificador de p�gina.
 * Output: Gesti�n interna de la lista LRU y selecci�n de v�ctima.
 */
class LRU :public ReplacementStrategy {
public:
    LRU(); //constructor por defecto
    void newPage(int pageId) override; //no es exactamente una nueva pagina, solo la toca
    void touch(int pageId) override;
    void pin(int pageId);
    void unpin(int pageId);
    void deletePage(int pageId);
    int getpos(int pageId);
    int victim();
protected:
    int _capacity;
    std::list<int> lru;
    std::unordered_map<int, std::list<int>::iterator> pos;
};

LRU::LRU() {
    // Constructor por defecto, inicializa la lista LRU y el mapa de posiciones
    lru.clear();
    pos.clear();
}

void LRU::pin(int pageId) {
    // Quita de candidatas a víctima
    auto it = pos.find(pageId);
    if (it != pos.end()) {
        lru.erase(it->second);
        pos.erase(it);
    }
}

void LRU::unpin(int pageId) {
    // Reincorpora como MRU
    if (!pos.count(pageId)) {
        lru.push_back(pageId);
        pos[pageId] = std::prev(lru.end());
    }
}

void LRU::newPage(int pageId) {
    // Inserta sin sobrepasar capacidad, o expulsa el front antes  
    if ((int)lru.size() == _capacity) {
        int old = victim();
        deletePage(old);
    }
    unpin(pageId);  // lo pone como MRU  
}
/**
 * Autor: Alexander
 * Objetivo: Borrar una p�gina de las estructuras LRU.
 * Input: int pageId
 * Output: Ninguno
 */
void LRU::deletePage(int pageId) {
    auto it = pos.find(pageId);
    if (it != pos.end()) {
        lru.erase(it->second);
        pos.erase(it);
    }
}

/**
 * Autor: Alexander
 * Objetivo: Devolver la p�gina v�ctima (menos recientemente usada).
 * Input: Ninguno
 * Output: ID de la p�gina al frente o -1 si la lista est� vac�a
 */
int LRU::victim() { //first element from LRU list.
    if (lru.empty()) return -1;
    int v = lru.front();
    lru.pop_front();
    pos.erase(v);
    return v;
}

/**
 * Autor: Alexander
 * Objetivo: Reposicionar la p�gina al final de la lista LRU.
 * Input: int pageId
 * Output: Ninguno
 */
void LRU::touch(int pageId) { //reposiciona la pagina al final de la lista
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
 * Objetivo: Calcular la posici�n de una p�gina en la lista LRU.
 * Input: int pageId
 * Output: Distancia desde el frente o -1 si no existe
 */
int LRU::getpos(int pageId) {
    auto it = pos.find(pageId);
    if (it != pos.end()) {
        return std::distance(lru.begin(), it->second);
    }
    return -1; //not found
}