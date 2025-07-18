// ===================== QueryHashBlocks.h =====================
#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>

using Entry = std::pair<std::string, int>;

// Estructura para buckets dinámicos
struct ExtBucket {
    std::vector<Entry> entries; // NUEVO: pares (valorDeCampo, IDBloque)
    size_t localDepth; // Profundidad local
};

class HashIndex {
public:
    // Constructor: rutas y nombre de relación
    HashIndex(const std::string& catalogPath,
        const std::string& blocksDir,
        const std::string& tableTxt,
        const std::string& relName,
        const std::string& indexField);     

    // Query original: devuelve solo registros (campos)
    std::vector<std::vector<std::string>> query(const std::string& whereVal);

    // Tipo auxiliar: (registro, rutaBloque)
    using Record = std::vector<std::string>;
    using Hit = std::pair<Record, std::string>;

    // Query mejorada: devuelve (registro, ruta de BloqueN.txt)
    std::vector<Hit> queryWithBlocks(const std::string& whereVal);

    // Devuelve ruta completa de un bloque dado su ID
    std::string getBlockPath(int blk) const;

    // Depuración: imprime directorio y buckets
    void printHashBuckets() const;

    std::string indexField() const { return _indexField; }

private:
    // Metadatos y configuración original
    std::string _catalogPath;
    std::string _blocksDir;
    std::string _tableTxt;
    std::string _relName;
    std::string _indexField;
    int         _fieldIndex;

    // Helpers
    std::vector<std::string> split(const std::string& s, char delim);

    // Hashing extendido
    size_t                _globalDepth;
    size_t                _bucketSizeThreshold;
    std::vector<ExtBucket*> _directory;
    std::string           _persistFile;

    // Construcción y mantenimiento de índice
    void buildIndex();
    void insertKey(int blk, const std::string& key);
    void splitBucket(size_t dirIdx);
    void doubleDirectory();
    void loadPersistedIndex();
    void writePersistedIndex();
};