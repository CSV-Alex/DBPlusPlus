#include "Disco.h"
#include "Utils.h"
#include <fstream>
#include <sstream>
#include <algorithm>

void Disco::relationFormat(const std::string& fileName, const std::string& path) {
    std::ofstream schemaFile(path + schema, std::ios::app);
    std::ifstream file(fileName);

    std::string schemaShortName = fileName.substr(fileName.find_last_of("\\") + 1);
    schemaShortName = schemaShortName.substr(0, schemaShortName.find_last_of('.'));

    schemaFile << schemaShortName;

    string field;
    string line1;
    string data;
    string line2;
    string type1;
    string type2;
    // string data_type;

    char data_separator = '#';

    getline(file, field);
    getline(file, line1);
    getline(file, line2);
    std::stringstream str(field);
    std::stringstream str1(line1);
    std::stringstream str2(line2);
    while (getline(str, data, data_separator)) {
        schemaFile << "#" << data;

        // data_type = '';

        getline(str1, type1, data_separator);
        getline(str2, type2, data_separator);

        char str1_type = convCondition(type1);
        char str2_type = convCondition(type2);

        char final_type;
        if (str1_type == 'D' || str2_type == 'D') {
            final_type = 'D';
        }
        else if (str1_type == 'S' || str2_type == 'S') {
            final_type = 'S';
        }
        else {
            final_type = 'I';
        }

        switch (final_type) {
        case 'I':
            schemaFile << "#int";
            break;
        case 'D':
            schemaFile << "#double";
            break;
        case 'S':
            schemaFile << "#string";
            break;
        default:
            break;
        }
    }
    schemaFile << std::endl;
    file.close();
}

