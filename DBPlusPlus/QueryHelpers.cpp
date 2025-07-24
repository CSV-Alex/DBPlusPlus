#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>

// Split por un caracter
static std::vector<std::string> splitRecords(const std::string& s, char delim) {
    std::vector<std::string> out;
    std::istringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim))
        out.push_back(std::move(item));
    return out;
}

// Detecta si la cadena representa un número (entero o decimal)
static bool isNumberRecords(const std::string& s) {
    bool hasDot = false;
    for (char c : s) {
        if (c == '.') {
            if (hasDot) return false;
            hasDot = true;
        }
        else if (c == '+' || c == '-') {
            // permite signo
        }
        else if (!std::isdigit(c)) return false;
    }
    return !s.empty();
}

// Compara lhs op rhs
static bool cmp(const std::string& lhs, const std::string& op, const std::string& rhs) {
    if (isNumberRecords(lhs) && isNumberRecords(rhs)) {
        double a = std::stod(lhs), b = std::stod(rhs);
        if (op == "=")  return a == b;
        if (op == "!=") return a != b;
        if (op == "<")  return a < b;
        if (op == "<=") return a <= b;
        if (op == ">")  return a > b;
        if (op == ">=") return a >= b;
    }
    else {
        if (op == "=")  return lhs == rhs;
        if (op == "!=") return lhs != rhs;
        if (op == "<")  return lhs < rhs;
        if (op == "<=") return lhs <= rhs;
        if (op == ">")  return lhs > rhs;
        if (op == ">=") return lhs >= rhs;
    }
    return false;
}

// Filtra los registros de una página in-memory
static std::vector<std::vector<std::string>>
filterPageRecords(
    const std::string& pageData,
    const std::vector<std::string>& headers,
    const std::string& whereField,
    const std::string& op,
    const std::string& val
) {
    // 1) hallar índice de columna
    auto it = std::find(headers.begin(), headers.end(), whereField);
    if (it == headers.end())
        throw std::runtime_error("Campo no existe: " + whereField);
    size_t idx = it - headers.begin();

    std::vector<std::vector<std::string>> result;
    // 2) split por registros
    for (auto& rec : splitRecords(pageData, '|')) {
        if (rec.empty()) continue;
        // 3) split por campos
        auto fields = splitRecords(rec, '#');
        if (idx >= fields.size()) continue;
        // 4) comparar y si concuerda, guardar
        if (cmp(fields[idx], op, val))
            result.push_back(std::move(fields));
    }
    return result;
}

// Vuelca el resultado a un txt
static void saveQueryResultToFile(
    const std::string& outFile,
    const std::vector<std::string>& headers,
    const std::vector<std::vector<std::string>>& rows
) {
    std::ofstream out(outFile);
    if (!out) throw std::runtime_error("No se pudo abrir " + outFile);
    // cabecera
    for (size_t i = 0; i < headers.size(); ++i) {
        out << headers[i] << (i + 1 < headers.size() ? '#' : '\n');
    }
    // filas
    for (auto& row : rows) {
        for (size_t j = 0; j < row.size(); ++j) {
            out << row[j] << (j + 1 < row.size() ? '#' : '\n');
        }
    }
}