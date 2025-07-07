// QueryHashBlocks.cpp
#include "QueryHashBlocks.h"

#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <functional>
#include <unordered_set>

namespace QueryHash {

    // NUEVO: ayuda a dividir cadenas por un carácter
    static std::vector<std::string> split(const std::string& s, char delim) {
        std::vector<std::string> elems;
        std::stringstream ss(s);
        std::string item;
        while (std::getline(ss, item, delim)) {
            elems.push_back(item);
        }
        return elems;
    }

    // NUEVO: convierte a minúsculas
    static std::string toLower(const std::string& s) {
        std::string r = s;
        std::transform(r.begin(), r.end(), r.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return r;
    }

    // NUEVO: Constructor — carga cabecera, detecta el campo índice y llena los buckets
    HashIndex::HashIndex(const std::string& tablePath,
        const std::string& relationName)
        : tablePath_(tablePath),
        relationName_(relationName)
    {
        // 1) Leer cabecera y contar registros
        std::ifstream in(tablePath_);
        if (!in) throw std::runtime_error("No se pudo abrir " + tablePath_);
        std::string line;
        std::getline(in, line);
        headers_ = split(line, '#');

        size_t recordCount = 0;
        while (std::getline(in, line))
            if (!line.empty()) ++recordCount;
        if (recordCount == 0)
            throw std::runtime_error("No hay registros en " + tablePath_);

        // 2) Detectar campo índice automáticamente
        fieldIndex_ = std::string::npos;
        for (size_t i = 0; i < headers_.size(); ++i) {
            std::string h = toLower(headers_[i]);
            if (h == "id" || (h.size() > 2 && h.substr(h.size() - 2) == "id")) {
                fieldIndex_ = i;
                indexField_ = headers_[i];
                break;
            }
        }
        // Si no hay “Id”, aplicar reglas especiales
        if (fieldIndex_ == std::string::npos) {
            if (relationName_ == "housing") {
                for (size_t i = 0; i < headers_.size(); ++i)
                    if (toLower(headers_[i]) == "price") {
                        fieldIndex_ = i;
                        indexField_ = headers_[i];
                        break;
                    }
            }
            else if (relationName_ == "titanic") {
                // asumimos PassengerId en columna 0
                fieldIndex_ = 0;
                indexField_ = headers_[0];
            }
        }
        if (fieldIndex_ == std::string::npos)
            throw std::runtime_error("No se pudo determinar campo índice para “"
                + relationName_ + "”");

        // 3) Crear buckets
        N_ = recordCount;
        buckets_.assign(N_, {});

        // 4) Segundo pase: repartir registros en buckets
        in.clear(); in.seekg(0);
        std::getline(in, line); // saltar cabecera
        size_t recLine = 0;
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            ++recLine;
            auto campos = split(line, '#');
            if (fieldIndex_ >= campos.size()) continue;
            const std::string& key = campos[fieldIndex_];
            size_t h = std::hash<std::string>{}(key);
            size_t b = h % N_;
            buckets_[b].push_back(recLine);
        }
    }

    // NUEVO: Ejecutar query de igualdad usando el índice construido
    std::vector<std::string> HashIndex::query(const std::string& value) {
        std::vector<std::string> resultados;
        std::ifstream in(tablePath_);
        if (!in) throw std::runtime_error("No se pudo abrir " + tablePath_);

        // 1) Agregar cabecera
        std::string line;
        std::getline(in, line);
        resultados.push_back(line);

        // 2) Calcular bucket del valor buscado
        size_t h = std::hash<std::string>{}(value);
        size_t b = h % N_;
        const auto& bucket = buckets_[b];

        // 3) Filtrar sólo las líneas de ese bucket
        std::unordered_set<size_t> targets(bucket.begin(), bucket.end());
        size_t recLine = 0;
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            ++recLine;
            if (targets.count(recLine)) {
                auto campos = split(line, '#');
                if (fieldIndex_ < campos.size() && campos[fieldIndex_] == value) {
                    resultados.push_back(line);
                }
            }
        }
        return resultados;
    }

    // NUEVO: devuelve el nombre del campo índice
    const std::string& HashIndex::indexField() const {
        return indexField_;
    }

} // namespace QueryHash
