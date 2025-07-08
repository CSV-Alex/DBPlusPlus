#pragma once

#include <string>
#include <vector>

class HashIndex {
public:
    HashIndex(const std::string& catalogPath,
        const std::string& blocksDir,
        const std::string& tableTxt,
        const std::string& relName);

    std::vector<std::vector<std::string>> query(const std::string& whereVal);

    // Nombre del campo usado como índice
    const std::string& indexField() const { return _indexField; }

private:
    std::string _catalogPath;
    std::string _blocksDir;
    std::string _tableTxt;
    std::string _relName;
    std::string _indexField;
    int         _fieldIndex;
    size_t      _N;                        // número de buckets
    std::vector<std::vector<int>> _buckets; // bucket -> lista de bloques

    static std::vector<std::string> split(const std::string& s, char delim);
};
