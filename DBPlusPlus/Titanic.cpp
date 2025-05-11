#include "Titanic.h"
#include <algorithm>
#include <stdexcept>
#include <vector> 
#include <sstream>

Titanic::Titanic(std::vector<std::string>& data) {
    passengerId = safe_stoi(data[0]);
    survived = safe_stoi(data[1]);
    pclass = safe_stoi(data[2]);
    name = data[3];
    sex = data[4];
    age = safe_stod(data[5]);
    sibSp = safe_stoi(data[6]);
    parch = safe_stoi(data[7]);
    ticket = data[8];
    fare = safe_stod(data[9]);
    cabin = data[10];
    embarked = data[11];
}

int Titanic::safe_stoi(const std::string& s) {
    try {
        return std::stoi(s);
    }
    catch (...) {
        return -1;
    }
}

double Titanic::safe_stod(const std::string& s) {
    try {
        std::string fixed = s;
        std::replace(fixed.begin(), fixed.end(), ',', '.');
        return std::stod(fixed);
    }
    catch (...) {
        return -1.0;
    }
}

size_t Titanic::getMemorySize() const {
    return sizeof(passengerId) + sizeof(survived) + sizeof(pclass) + sizeof(age) +
        sizeof(sibSp) + sizeof(parch) + sizeof(fare) +
        name.capacity() + sex.capacity() + ticket.capacity() +
        cabin.capacity() + embarked.capacity();
}
