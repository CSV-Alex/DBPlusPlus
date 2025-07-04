// QueryBlocks.h
#pragma once

#include <string>
#include <vector>

// Si no hay WHERE, lee catalogo.txt y devuelve los números de bloque
std::vector<int> getBlocksFromCatalog(
    const std::string& catalogPath,
    const std::string& relationName
);

// Si hay WHERE, inspecciona bloque por bloque y devuelve
// los números de bloque que contienen al menos un registro
std::vector<int> findBlocksWithCondition(
    const std::string& blocksDir,
    const std::string& tableTxt,
    const std::string& whereField,
    const std::string& whereOp,
    const std::string& whereVal
);
