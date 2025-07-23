// ===================== QueryHashBlocks.cpp =====================
#include "QueryHashBlocks.h"
#include "QueryBlocks.h"   // readBlockHeader, loadRelationHeader, getBlocksFromCatalog
#include <iostream>
#include <algorithm>
#include <unordered_set>
#include <filesystem>

// Constructor
HashIndex::HashIndex(const std::string& catalogPath,
    const std::string& blocksDir,
    const std::string& tableTxt,
    const std::string& relName,
    const std::string& indexField)
    : _catalogPath(catalogPath)
    , _blocksDir(blocksDir)
    , _tableTxt(tableTxt)
    , _relName(relName)
    , _indexField(indexField)
{
    std::filesystem::create_directories(_blocksDir);

    // Inicializo directorio mínimo
    _globalDepth = 1;
    _bucketSizeThreshold = 100; // ajusta a tu capacidad de bloque
    auto b0 = new ExtBucket();
    b0->localDepth = 1;
    _directory.assign(2, b0);

    // Construyo índice (detecta campo y llena buckets)
    buildIndex();

    // Preparo archivo de persistencia
    _persistFile = _blocksDir + _relName + "_" + _indexField + "_ext_index.txt";
    std::cout << "[DEBUG] Persist file path: " << _persistFile << "\n";
    if (std::filesystem::exists(_persistFile)) {
        loadPersistedIndex();
    }
    else {
        writePersistedIndex();
        std::cout << "[DEBUG] Persist file creado en: " << _persistFile << "\n";
    }
}

// Separa por delimitador
std::vector<std::string> HashIndex::split(const std::string& s, char delim) {
    std::vector<std::string> elems;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) elems.push_back(item);
    return elems;
}

// Carga todos los bloques, extrae la columna y llama a insertKey()
void HashIndex::buildIndex() {
    auto blocks = getBlocksFromCatalog(_catalogPath, _relName);
    auto hdr = loadRelationHeader(_tableTxt);

    // Busco exactamente el campo indexField
    _fieldIndex = -1;
    for (int i = 0; i < (int)hdr.size(); ++i) {
        if (hdr[i] == _indexField) {
            _fieldIndex = i;
            break;
        }
    }
    if (_fieldIndex < 0)
        throw std::runtime_error("No se encontró campo índice: " + _indexField);

    // Recorro cada bloque y cada registro
    for (int blk : blocks) {
        std::string path = getBlockPath(blk);
        int numRec; std::streampos off;
        if (!readBlockHeader(path, numRec, off)) continue;
        std::ifstream in(path, std::ios::binary);
        in.seekg(off);
        std::string data{ std::istreambuf_iterator<char>(in), {} };
        in.close();

        for (auto& rec : split(data, '|')) {
            if (rec.empty()) continue;
            // Limpio el registro y lo separo en campos
            std::string clean;
            for (char c : rec) if (c != '@') clean.push_back(c);
            auto fields = split(clean, '#');
            if (_fieldIndex < (int)fields.size()) {
                insertKey(blk, fields[_fieldIndex]);
            }
        }
    }
}

// MODIFICADO: inserta la pareja (key,blk) en el bucket adecuado
void HashIndex::insertKey(int blk, const std::string& key) {
    size_t h = std::hash<std::string>{}(key);
    size_t dirIdx = h & ((1ULL << _globalDepth) - 1);
    auto bucket = _directory[dirIdx];

    // Nuevo: añado la entrada
    bucket->entries.emplace_back(key, blk);

    // Si supera threshold, partimos
    if (bucket->entries.size() > _bucketSizeThreshold)
        splitBucket(dirIdx);
}

// Duplica el directorio
void HashIndex::doubleDirectory() {
    size_t old = _directory.size();
    _directory.resize(old * 2);
    for (size_t i = 0; i < old; ++i)
        _directory[i + old] = _directory[i];
    _globalDepth++;
}

// MODIFICADO: reparto según bits de hash(key), no de blk
void HashIndex::splitBucket(size_t dirIdx) {
    auto bucket = _directory[dirIdx];
    size_t oldDepth = bucket->localDepth;

    if (oldDepth == _globalDepth) doubleDirectory();

    auto newB = new ExtBucket();
    newB->localDepth = oldDepth + 1;
    bucket->localDepth++;

    // Reasigno punteros Dir → buckets
    for (size_t i = 0; i < _directory.size(); ++i) {
        if (_directory[i] == bucket &&
            (((i >> oldDepth) & 1) == 1))
        {
            _directory[i] = newB;
        }
    }

    // Reparto las entradas
    auto oldEntries = std::move(bucket->entries);
    bucket->entries.clear();
    for (auto& e : oldEntries) {
        const auto& key = e.first;
        int blk = e.second;
        size_t h = std::hash<std::string>{}(key);
        if (((h >> oldDepth) & 1) == 0)
            bucket->entries.push_back(e);
        else
            newB->entries.push_back(e);
    }
}

// Persistencia simplificada: sólo guardo globalDepth y por bucket los pares blk:key
void HashIndex::writePersistedIndex() {
    std::ofstream out(_persistFile);
    out << _globalDepth << "\n";
    for (size_t i = 0; i < _directory.size(); ++i) {
        auto b = _directory[i];
        out << i << " " << b->localDepth;
        for (auto& e : b->entries)
            out << " " << e.second << ":" << e.first;
        out << "\n";
    }
}

// Carga el mismo formato
void HashIndex::loadPersistedIndex() {
    std::ifstream in(_persistFile);
    in >> _globalDepth;
    size_t dirSz = 1ULL << _globalDepth;
    _directory.clear();
    _directory.resize(dirSz);
    std::string line;
    std::getline(in, line); // salta al siguiente
    for (size_t i = 0; i < dirSz && std::getline(in, line); ++i) {
        std::istringstream iss(line);
        size_t idx, ld;
        iss >> idx >> ld;
        auto b = new ExtBucket();
        b->localDepth = ld;
        std::string token;
        while (iss >> token) {
            // token = "blk:key"
            auto pos = token.find(':');
            int blk = std::stoi(token.substr(0, pos));
            std::string key = token.substr(pos + 1);
            b->entries.emplace_back(key, blk);
        }
        _directory[idx] = b;
    }
}

// Query original (lee sólo el bucket correspondiente)
std::vector<std::vector<std::string>> HashIndex::query(const std::string& whereVal) {
    std::vector<std::vector<std::string>> res;
    size_t h = std::hash<std::string>{}(whereVal);
    size_t dirIdx = h & ((1ULL << _globalDepth) - 1);
    for (auto& e : _directory[dirIdx]->entries) {
        if (e.first != whereVal) continue;
        // leo BloqueN.txt completo y filtro registros...
        std::string path = getBlockPath(e.second);
        int numRec; std::streampos off;
        if (!readBlockHeader(path, numRec, off)) continue;
        std::ifstream in(path, std::ios::binary);
        in.seekg(off);
        std::string data{ std::istreambuf_iterator<char>(in), {} };
        in.close();
        for (auto& rec : split(data, '|')) {
            if (rec.empty()) continue;
            std::string clean;
            for (char c : rec) if (c != '@') clean.push_back(c);
            auto fields = split(clean, '#');
            if (_fieldIndex < (int)fields.size() && fields[_fieldIndex] == whereVal)
                res.push_back(fields);
        }
    }
    return res;
}

// Query con rutas de bloque
std::vector<HashIndex::Hit> HashIndex::queryWithBlocks(const std::string& whereVal) {
    std::vector<Hit> result;
    size_t h = std::hash<std::string>{}(whereVal);
    size_t dirIdx = h & ((1ULL << _globalDepth) - 1);
    std::unordered_set<int> seen;
    for (auto& e : _directory[dirIdx]->entries) {
        if (e.first != whereVal) continue;
        int blk = e.second;
        if (!seen.insert(blk).second) continue;
        std::string path = getBlockPath(blk);
        // leo y filtro igual que en query()
        int numRec; std::streampos off;
        if (!readBlockHeader(path, numRec, off)) continue;
        std::ifstream in(path, std::ios::binary);
        in.seekg(off);
        std::string data{ std::istreambuf_iterator<char>(in), {} };
        in.close();
        for (auto& rec : split(data, '|')) {
            if (rec.empty()) continue;
            std::string clean;
            for (char c : rec) if (c != '@') clean.push_back(c);
            auto fields = split(clean, '#');
            if (_fieldIndex < (int)fields.size() && fields[_fieldIndex] == whereVal)
                result.emplace_back(fields, path);
        }
    }
    return result;
}

// Construye ruta del bloque
std::string HashIndex::getBlockPath(int blk) const {
    return _blocksDir + "Bloque" + std::to_string(blk) + ".txt";
}

// Imprime directorio y sus buckets
void HashIndex::printHashBuckets() const {
    std::cout << "=== Directorio (globalDepth=" << _globalDepth << ") ===\n";
    for (size_t i = 0; i < _directory.size(); ++i) {
        auto b = _directory[i];
        std::cout << "Dir[" << i << "] (LD=" << b->localDepth << "): ";
        for (auto& e : b->entries)
            std::cout << "(" << e.first << "," << e.second << ") ";
        std::cout << "\n";
    }
}
