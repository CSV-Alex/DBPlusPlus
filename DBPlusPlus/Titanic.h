#pragma once
#include <vector>
#include <sstream>

class Titanic {
private:
    int passengerId;
    int survived;
    int pclass;
    std::string name;
    std::string sex;
    double age;
    int sibSp;
    int parch;
    std::string ticket;
    double fare;
    std::string cabin;
    std::string embarked;

    int safe_stoi(const std::string& s);
    double safe_stod(const std::string& s);

public:
    Titanic(std::vector<std::string>& data);
    size_t getMemorySize() const;
};
