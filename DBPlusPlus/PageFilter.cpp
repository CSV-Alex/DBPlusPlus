#include "PageFilter.h"
#include "QueryBlocks.h"     // para loadRelationHeader
#include <sstream>
#include <cctype>
#include <stdexcept>

PageFilter::PageFilter(const std::string& rawData)
    : _rawData(rawData)
{
}

std::vector<std::string> PageFilter::split(const std::string& s, char delim) {
    std::vector<std::string> out;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim))
        out.push_back(item);
    return out;
}

static bool isNumber(const std::string& s) {
    bool hasDot = false;
    for (char c : s) {
        if (c == '.') {
            if (hasDot) return false;
            hasDot = true;
        }
        else if (!std::isdigit(c) && c != '-' && c != '+') {
            return false;
        }
    }
    return !s.empty();
}

bool PageFilter::compare(
    const std::string& lhs,
    const std::string& op,
    const std::string& rhs
) {
    // Si ambos son números, comparamos numéricamente
    if (isNumber(lhs) && isNumber(rhs)) {
        double a = std::stod(lhs);
        double b = std::stod(rhs);
        if (op == "=")  return a == b;
        if (op == "!=") return a != b;
        if (op == "<")  return a < b;
        if (op == "<=") return a <= b;
        if (op == ">")  return a > b;
        if (op == ">=") return a >= b;
        throw std::runtime_error("Operador inválido: " + op);
    }
    // Sino, comparamos lexicográficamente
    if (op == "=")  return lhs == rhs;
    if (op == "!=") return lhs != rhs;
    if (op == "<")  return lhs < rhs;
    if (op == "<=") return lhs <= rhs;
    if (op == ">")  return lhs > rhs;
    if (op == ">=") return lhs >= rhs;
    throw std::runtime_error("Operador inválido: " + op);
}

std::vector<std::vector<std::string>>
PageFilter::filterRecords(
    const std::string& tableTxt,
    const std::string& whereField,
    const std::string& whereOp,
    const std::string& whereVal
) const {
    // 1) Carga el header de la relación
    auto headers = loadRelationHeader(tableTxt);
    int idx = -1;
    for (int i = 0; i < (int)headers.size(); ++i) {
        if (headers[i] == whereField) {
            idx = i;
            break;
        }
    }
    if (idx < 0)
        throw std::runtime_error("filterRecords: campo '" + whereField + "' no encontrado");

    // 2) Para cada registro en _rawData, limpias padding y splits
    std::vector<std::vector<std::string>> result;
    for (auto rec : split(_rawData, '|')) {
        if (rec.empty()) continue;

        // Quitar '@' de padding
        std::string clean;
        clean.reserve(rec.size());
        for (char c : rec)
            if (c != '@')
                clean.push_back(c);

        auto fields = split(clean, '#');
        if (idx >= (int)fields.size()) continue;

        // 3) Comparar campo vs valor
        if (compare(fields[idx], whereOp, whereVal))
            result.push_back(std::move(fields));
    }
    return result;
}
