#pragma once
#include "Titanic.h"
#include <vector>
#include <sstream>

class TitanicData {
private:
    std::vector<Titanic> passengers;

public:
    void readFromFile(const std::string& filename);
    size_t totalMemoryUsed() const;
    size_t count() const;
};
