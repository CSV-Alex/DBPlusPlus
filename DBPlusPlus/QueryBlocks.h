// QueryBlocks.h
#pragma once

#include <string>
#include <vector>
#include <sstream>

// Resultado de una consulta: bloques únicos, registros y cabeceras
struct QueryResult {
    std::vector<int> bloques;                   // números de bloque
    std::vector<std::vector<std::string>> records; // cada registro partido en campos
    std::vector<std::string> headers;           // nombres de columna
};


// Si no hay WHERE, lee catalogo.txt y devuelve los números de bloque
std::vector<int> getBlocksFromCatalog(
    const std::string& catalogPath,
    const std::string& relationName
);

// Si hay WHERE, inspecciona bloque por bloque y devuelve
// los números de bloque que contienen al menos un registro
std::vector<int> findBlocksWithCondition(
    const std::string& catalogPath,
    const std::string& blocksDir,
    const std::string& tableTxt,
    const std::string& relationName,
    const std::string& whereField,
    const std::string& whereOp,
    const std::string& whereVal
);

std::vector<std::vector<std::string>> getRecordsFromBlocks(
    const std::vector<int>& blocks,
    const std::string& blocksDir,
    const std::string& tableTxt,
    const std::string& relationName,
    const std::string& whereField,
    const std::string& whereOp,
    const std::string& whereVal
);

std::vector<std::string> getRelationHeader(const std::string& tableTxt);

bool readBlockHeader(const std::string& path, int& numRec, std::streampos& off);
std::vector<std::string> loadRelationHeader(const std::string& tableTxt);

inline std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> result;
    std::string token;
    std::stringstream ss(str);
    while (std::getline(ss, token, delimiter)) {
        result.push_back(token);
    }
    return result;
}
