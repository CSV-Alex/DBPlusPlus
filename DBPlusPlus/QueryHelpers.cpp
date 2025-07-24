#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <stdexcept>

// trim de espacios en blanco (incluye '\r','\n','\t', etc.)
static std::string trim(const std::string& s) {
    size_t i = 0, j = s.size();
    while (i < j && std::isspace((unsigned char)s[i]))     ++i;
    while (j > i && std::isspace((unsigned char)s[j - 1])) --j;
    return s.substr(i, j - i);
}

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
    if (s.empty()) return false;
    bool hasDot = false;
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (c == '.') {
            if (hasDot) return false;
            hasDot = true;
        }
        else if ((c == '+' || c == '-') && i == 0) {
            // signo sólo al inicio
        }
        else if (!std::isdigit((unsigned char)c)) {
            return false;
        }
    }
    return true;
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
    // 0) Descarta todo hasta el primer '/'
    std::string data = pageData;
    auto slashPos = data.find('/');
    if (slashPos != std::string::npos) {
        data = data.substr(slashPos + 1);
    }
    // 0.1) Elimina todos los '@'
    data.erase(std::remove(data.begin(), data.end(), '@'), data.end());

    // 1) hallar índice de columna
    auto it = std::find(headers.begin(), headers.end(), whereField);
    if (it == headers.end())
        throw std::runtime_error("Campo no existe: " + whereField);
    size_t idx = it - headers.begin();

    std::vector<std::vector<std::string>> result;
    // 2) split por registros '|'
    auto recs = splitRecords(data, '|');
    std::cout << "[DEBUG] total de registros tras '/' y quitar '@': " << recs.size() << "\n";
    for (size_t r = 0; r < recs.size(); ++r) {
        std::string rawRec = recs[r];
        std::cout << "[DEBUG] rawRec[" << r << "] = \"" << rawRec << "\"\n";
        std::string rec = trim(rawRec);
        std::cout << "[DEBUG] trimmed rec[" << r << "] = \"" << rec << "\"\n";
        if (rec.empty()) {
            std::cout << "[DEBUG] rec[" << r << "] está vacía, salto\n";
            continue;
        }

        // 3) split por campos '#'
        auto fields = splitRecords(rec, '#');
        // trim de cada campo
        for (size_t f = 0; f < fields.size(); ++f) {
            fields[f] = trim(fields[f]);
            std::cout << "[DEBUG]   field[" << f << "] = \"" << fields[f] << "\"\n";
        }
        if (idx >= fields.size()) {
            std::cout << "[DEBUG] idx " << idx << " fuera de rango (" << fields.size() << "), salto\n";
            continue;
        }

        // 4) comparar y si concuerda, guardar
        const std::string& key = fields[idx];
        bool match = cmp(key, op, val);
        std::cout << "[DEBUG] comparar key=\"" << key
            << "\" " << op << " \"" << val
            << "\" => " << (match ? "MATCH\n" : "NO MATCH\n");
        if (match) {
            result.push_back(std::move(fields));
        }
    }
    std::cout << "[DEBUG] total filas filtradas: " << result.size() << "\n";
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
