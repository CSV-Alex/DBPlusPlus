#include "QueryHashBlocks.h"
#include "QueryBlocks.h"   // contiene readBlockHeader, loadRelationHeader, split(data,'|')
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <unordered_set>
#include <algorithm>
#include <filesystem>
#include <iostream>

// Divide cadena por delimitador
std::vector<std::string> HashIndex::split(const std::string& s, char delim) {
    std::vector<std::string> elems;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

// Constructor: detecta campo y construye buckets
HashIndex::HashIndex(const std::string& catalogPath,
    const std::string& blocksDir,
    const std::string& tableTxt,
    const std::string& relName)
    : _catalogPath(catalogPath),
    _blocksDir(blocksDir),
    _tableTxt(tableTxt),
    _relName(relName)
{
    // 1) Obtener lista de bloques de la relación
    std::vector<int> blocks = getBlocksFromCatalog(_catalogPath, _relName);
    if (blocks.empty())
        throw std::runtime_error("No hay bloques para " + _relName);
    _N = blocks.size();
    _buckets.assign(_N, {});

    // 2) Leer cabecera y determinar campo índice
    std::vector<std::string> hdr = loadRelationHeader(_tableTxt);
    _fieldIndex = -1;
    for (int i = 0; i < (int)hdr.size(); ++i) {
        const std::string& h = hdr[i];
        if ((h.size() > 2 && (h == "Id" || h.substr(h.size() - 2) == "Id"))) {
            _fieldIndex = i;
            _indexField = h;
            break;
        }
    }
    // Regla especial: housing → area
    if (_fieldIndex < 0 && _relName == "housing") {
        for (int i = 0; i < (int)hdr.size(); ++i) {
            if (hdr[i] == "area") {
                _fieldIndex = i;
                _indexField = hdr[i];
                break;
            }
        }
    }
    if (_fieldIndex < 0)
        throw std::runtime_error("No se pudo determinar campo índice para " + _relName);

    // 3) Repartir bloques en buckets
    for (size_t bi = 0; bi < blocks.size(); ++bi) {
        int blk = blocks[bi];
        std::string path = blocksDir + "Bloque" + std::to_string(blk) + ".txt";
        if (!std::filesystem::exists(path)) continue;

        int numRec;
        std::streampos off;
        if (!readBlockHeader(path, numRec, off)) continue;

        std::ifstream in(path, std::ios::binary);
        in.seekg(off);
        std::string data((std::istreambuf_iterator<char>(in)), {});
        in.close();

        std::vector<std::string> recs = split(data, '|');
        for (auto& rec : recs) {
            if (rec.empty()) continue;
            std::string clean;
            clean.reserve(rec.size());
            for (char c : rec) if (c != '@') clean.push_back(c);

            std::vector<std::string> fields = split(clean, '#');
            if (_fieldIndex < (int)fields.size()) {
                const std::string& key = fields[_fieldIndex];
                size_t h = std::hash<std::string>{}(key);
                size_t b = h % _N;
                auto& vec = _buckets[b];
                if (vec.empty() || vec.back() != blk)
                    vec.push_back(blk);
            }
        }
    }
}

// Ejecuta la consulta equality usando el índice hash
std::vector<std::vector<std::string>>
HashIndex::query(const std::string& whereVal) {
    std::vector<std::vector<std::string>> result;
    size_t h = std::hash<std::string>{}(whereVal);
    size_t b = h % _N;
    const auto& cands = _buckets[b];

    for (int blk : cands) {
        std::string path = _blocksDir + "Bloque" + std::to_string(blk) + ".txt";
        int numRec;
        std::streampos off;
        if (!readBlockHeader(path, numRec, off)) continue;

        std::ifstream in(path, std::ios::binary);
        in.seekg(off);
        std::string data((std::istreambuf_iterator<char>(in)), {});
        in.close();

        std::vector<std::string> recs = split(data, '|');
        for (auto& rec : recs) {
            if (rec.empty()) continue;
            std::string clean;
            clean.reserve(rec.size());
            for (char c : rec) if (c != '@') clean.push_back(c);

            std::vector<std::string> fields = split(clean, '#');
            if (_fieldIndex < (int)fields.size() && fields[_fieldIndex] == whereVal) {
                result.push_back(std::move(fields));
            }
        }
    }
    return result;
}

// relName: nombre de la relación (p.ej. "housing")
// indexField: nombre del campo indexado (p.ej. "area")
// buckets:   vector de buckets, donde buckets[b] es la lista de bloques en el bucket b
void printHashBuckets(
    const std::string& relName,
    const std::string& indexField,
    const std::vector<std::vector<int>>& buckets
) {
    size_t N = buckets.size();
    std::cout << "=== HashIndex sobre '" << relName
        << "' (" << N << " buckets) campo índice: '"
        << indexField << "' ===\n";
    for (size_t b = 0; b < N; ++b) {
        std::cout << "Bucket " << b << ": ";
        if (buckets[b].empty()) {
            std::cout << "<vacío>";
        }
        else {
            for (int blk : buckets[b]) {
                std::cout << blk << " ";
            }
        }
        std::cout << "\n";
    }
    std::cout << std::endl;
}

void printDetailedHashBuckets(const HashIndex& idx) {
    // 1) Leer todos los registros de la tabla
    std::ifstream in(idx.tableFile());
    if (!in) {
        std::cerr << "No se pudo abrir " << idx.tableFile() << "\n";
        return;
    }
    std::string line;
    // saltar cabecera
    std::getline(in, line);

    // 2) Preparar contenedores por bucket
    size_t N = idx.bucketCount();
    std::vector<std::vector<std::pair<int, std::string>>> recsInBucket(N);
    int recId = 0;

    // 3) Para cada registro: calcular bucket y almacenar par<recId,valor>
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        ++recId;
        // quitar padding '@' y partir por '|'
        std::string clean;
        clean.reserve(line.size());
        for (char c : line) if (c != '@') clean.push_back(c);
        auto fields = HashIndex::split(clean, '#');  // reutilizas tu split

        int fIdx = idx.fieldIndex();
        if (fIdx >= (int)fields.size()) continue;
        const std::string& key = fields[fIdx];
        size_t h = std::hash<std::string>{}(key);
        size_t b = h % N;

        recsInBucket[b].emplace_back(recId, key);
    }
    in.close();

    // 4) Imprimir
    std::cout << "=== Detalle de registros por bucket ===\n";
    for (size_t b = 0; b < N; ++b) {
        std::cout << "Bucket " << b << ":\n";
        if (recsInBucket[b].empty()) {
            std::cout << "  <vacío>\n";
        }
        else {
            for (auto& p : recsInBucket[b]) {
                // p.first = número de registro (línea), p.second = valor de campo
                std::cout << "  Rec#" << p.first
                    << " => " << p.second << "\n";
            }
        }
    }
}
