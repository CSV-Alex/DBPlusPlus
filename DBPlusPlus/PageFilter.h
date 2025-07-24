#pragma once

#include <string>
#include <vector>

class PageFilter {
public:
    // registros separados por '|'
    PageFilter(const std::string& rawData);

    std::vector<std::vector<std::string>> filterRecords(
        const std::string& tableTxt,
        const std::string& whereField,
        const std::string& whereOp,
        const std::string& whereVal
    ) const;

private:
    std::string _rawData;

    static std::vector<std::string> split(const std::string& s, char delim);
    static bool compare(const std::string& lhs, const std::string& op, const std::string& rhs);
};
