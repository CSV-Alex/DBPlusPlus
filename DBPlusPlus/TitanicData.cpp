#include "TitanicData.h"
#include "Utils.h"
#include <fstream>
#include <sstream>

void TitanicData::readFromFile(const std::string& filename) {
    std::ifstream file(filename);
    std::string line;
    getline(file, line); // header
    while (getline(file, line)) {
        auto row = parse_Line(line);
        passengers.emplace_back(row);
    }
    file.close();
}

size_t TitanicData::totalMemoryUsed() const {
    size_t total = 0;
    for (const auto& p : passengers) {
        total += p.getMemorySize();
    }
    return total;
}

size_t TitanicData::count() const {
    return passengers.size();
}
