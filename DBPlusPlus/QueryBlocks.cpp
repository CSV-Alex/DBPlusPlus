// QueryBlocks.cpp
#include "QueryBlocks.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <set>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <iostream>

namespace fs = std::filesystem;

// NUEVO: particiona un string por un delimitador
static std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> elems;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim))
        elems.push_back(item);
    return elems;
}

// NUEVO: lee la cabecera de la relación (línea 0) y extrae los nombres de campo
static std::vector<std::string> loadRelationHeader(const std::string& tablePath) {
    std::ifstream in(tablePath);
    if (!in) throw std::runtime_error("No se pudo abrir " + tablePath);
    std::string headerLine;
    std::getline(in, headerLine);
    return split(headerLine, '#');
}

// NUEVO: evalúa una condición simple entre cell y val
static bool evalCondition(const std::string& cell,
    const std::string& op,
    const std::string& val)
{

    // 1) Ignorar padding '@'
    std::string cleanCell;
    cleanCell.reserve(cell.size());
    for (char c : cell) {
        if (c != '@') cleanCell.push_back(c);
    }

    // MODIFICACIÓN: manejo de string con comillas para prefix‐match
    if (!val.empty() && val.front() == '\'' && val.back() == '\'') {
        std::string prefix = val.substr(1, val.size() - 2);
        if (op == "=" || op == "==")
            return cell.rfind(prefix, 0) == 0;   // empieza con prefix
        if (op == "!=")
            return cell.rfind(prefix, 0) != 0;   // no empieza con prefix
        return false;  // otros operadores no soportados para este caso
    }

    auto isInt = [](const std::string& s) {
        return !s.empty() &&
            std::all_of(s.begin(), s.end(), [](char c) { return std::isdigit(c); });
        };
    auto isDouble = [](const std::string& s) {
        bool dot = false;
        if (s.empty()) return false;
        for (char c : s) {
            if (c == '.') {
                if (dot) return false;
                dot = true;
            }
            else if (!std::isdigit(c)) return false;
        }
        return dot;
        };

    // comparar enteros
    if (isInt(cell) && isInt(val)) {
        int a = std::stoi(cell), b = std::stoi(val);
        if (op == "=" || op == "==") return a == b;
        if (op == "!=")         return a != b;
        if (op == "<")          return a < b;
        if (op == ">")          return a > b;
        if (op == "<=")         return a <= b;
        if (op == ">=")         return a >= b;
    }
    // comparar reales
    else if (isDouble(cell) && isDouble(val)) {
        double a = std::stod(cell), b = std::stod(val);
        if (op == "=" || op == "==") return a == b;
        if (op == "!=")         return a != b;
        if (op == "<")          return a < b;
        if (op == ">")          return a > b;
        if (op == "<=")         return a <= b;
        if (op == ">=")         return a >= b;
    }
    // comparar cadenas
    else {
        if (op == "=" || op == "==") return cell == val;
        if (op == "!=")         return cell != val;
    }
    return false;
}

// NUEVO: lee el header de un BloqueN.txt hasta la '/' y extrae numRecords
static bool readBlockHeader(const std::string& blockPath,
    int& numRecords,
    std::streampos& dataOffset)
{
    std::ifstream in(blockPath, std::ios::binary);
    if (!in) return false;
    std::string header;
    char c;
    while (in.get(c) && c != '/') header.push_back(c);
    dataOffset = in.tellg();
    in.close();
    numRecords = std::stoi(header);
    return true;
}

std::vector<int> findBlocksWithCondition(
    const std::string& blocksDir,
    const std::string& tableTxt,
    const std::string& whereField,
    const std::string& whereOp,
    const std::string& whereVal
) {
    // 1) cargar cabecera de relación y hallar índice WHERE
    auto headers = loadRelationHeader(tableTxt);
    int whereIndex = -1;
    for (int i = 0; i < (int)headers.size(); ++i) {
        if (headers[i] == whereField) {
            whereIndex = i;
            break;
        }
    }
    if (whereIndex < 0)
        std::cout << "Campo WHERE no existe en " << tableTxt << std::endl;

    std::set<int> bloquesSet;

    // 2) iterar Bloque1.txt, Bloque2.txt, …
    for (int blk = 1; ; ++blk) {
        std::string path = blocksDir + "Bloque" + std::to_string(blk) + ".txt";
        if (!fs::exists(path)) break;

        int numRec;
        std::streampos off;
        if (!readBlockHeader(path, numRec, off)) continue;

        // 3) leer datos después del header
        std::ifstream in(path, std::ios::binary);
        in.seekg(off);
        std::string data((std::istreambuf_iterator<char>(in)), {});
        in.close();

        // 4) separar registros y evaluar
        auto records = split(data, '|');
        for (auto& rec : records) {
            if (rec.empty()) continue;

            // MODIFICACIÓN: limpiamos TODO '@' del registro antes de split
            std::string recClean;
            recClean.reserve(rec.size());
            for (char ch : rec) {
                if (ch != '@')
                    recClean.push_back(ch);
            }

            // ahora partimos recClean en campos
            auto fields = split(recClean, '#');

            if (whereIndex < (int)fields.size()
                && evalCondition(fields[whereIndex], whereOp, whereVal))
            {
                bloquesSet.insert(blk);
                break;
            }
        }
    }

    return { bloquesSet.begin(), bloquesSet.end() };
}

std::vector<int> getBlocksFromCatalog(
    const std::string& catalogPath,
    const std::string& relationName
) {
    std::ifstream in(catalogPath);
    if (!in) throw std::runtime_error("No se pudo abrir " + catalogPath);
    std::set<int> bloques;

    std::string line;
    while (std::getline(in, line)) {
        auto parts = split(line, '|');
        if (parts.size() == 2 && parts[0] == relationName) {
            auto pos = parts[1].find("Bloque");
            if (pos != std::string::npos) {
                pos += 6; // tras "Bloque"
                int n = std::stoi(parts[1].substr(pos));
                bloques.insert(n);
            }
        }
    }
    return { bloques.begin(), bloques.end() };
}
