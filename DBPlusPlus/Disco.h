#pragma once
#include <sstream>

class Disco {
private:

public:
    std::string schema = "esquema.txt";
    void relationFormat(const std::string& fileName, const std::string& path);
};
