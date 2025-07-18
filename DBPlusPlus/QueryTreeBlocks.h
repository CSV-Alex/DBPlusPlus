#pragma once

#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include "QueryBlocks.h"   // readBlockHeader, loadRelationHeader, getBlocksFromCatalog
#include "QueryHashBlocks.h"

// Un par (registro, rutaBloque) para la consulta
using Record = std::vector<std::string>;
using Hit = std::pair<Record, std::string>;

class BPlusTreeIndex {
public:
    // Construye el índice B+Tree sobre el campo indexField (Search Key)
    // degree = d (orden), cada nodo tiene entre d y 2d llaves
    BPlusTreeIndex(const std::string& catalogPath,
        const std::string& blocksDir,
        const std::string& tableTxt,
        const std::string& relName,
        const std::string& indexField,
        size_t degree = 2);

    // Consulta de igualdad: devuelve (registro, rutaBloque)
    std::vector<Hit> queryWithBlocks(const std::string& whereVal);

    // Imprime una representación del árbol (simple)
    void printTree() const;

    // Devuelve la ruta completa de un bloque dado su ID
    std::string getBlockPath(int blk) const {
        return _blocksDir + "Bloque" + std::to_string(blk) + ".txt";
    }

private:
    struct Node {
        bool isLeaf;
        std::vector<std::string>     keys;       // llaves
        std::vector<Node*>           children;   // hijos (size = keys.size()+1 si !isLeaf)
        std::vector<std::vector<int>> ptrs;       // para hojas: ptrs[i] = lista de bloques de keys[i]
        Node* parent;
        Node* next;       // para hojas: enlace a siguiente hoja

        Node(bool leaf) : isLeaf(leaf), parent(nullptr), next(nullptr) {}
    };

    size_t _degree;               // d
    Node* _root;

    // metadata de archivos
    std::string _catalogPath;
    std::string _blocksDir;
    std::string _tableTxt;
    std::string _relName;
    std::string _indexField;
    int         _fieldIndex;

    // Construcción de índice
    void buildIndex();
    void insertEntry(const std::string& key, int blk);

    // Splits
    void splitLeaf(Node* leaf);
    void splitInternal(Node* internal);

    // Búsqueda de la hoja donde viviría la key
    Node* findLeaf(const std::string& key) const;

    // Lectura de registros en bloque y filtrado
    void collectRecords(int blk,
        const std::string& whereVal,
        std::vector<Hit>& out) const;

    // Impresión recursiva
    void printNode(Node* node, int level) const;
};
