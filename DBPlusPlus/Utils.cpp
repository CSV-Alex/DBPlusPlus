//#include "Utils.h"
//#include <vector>
//#include <optional>
//#include <iostream>
//#include <fstream>
//#include <sstream>
//#include <iomanip>
//#include <algorithm>
//#include <cctype>
//
//using std::vector;
//using std::string;
//
//std::string fileToSaveName(const std::string& input) {
//    size_t pipePos = input.find('|');
//    if (pipePos == std::string::npos) return "";
//    return input.substr(pipePos + 2, input.find('#', pipePos + 2) - (pipePos + 2)) + ".txt";
//}
//
//std::string parseQueryInput(std::string input) {
//    std::string line = input;
//    replace(line.begin(), line.end(), ' ', '#');
//    return line;
//}
//
//std::vector<std::string> parse_Line(const std::string& line) {
//    std::vector<std::string> fields;
//    std::istringstream iss(line);
//    std::string field;
//    while (getline(iss, field, '#')) {
//        fields.push_back(field);
//    }
//    return fields;
//}
//
////char convCondition(const std::string& data) {
////    for (size_t i = 0; i < data.size(); i++) {
////        if (data[i] == '.') return 'D';
////        if (!isdigit(data[i])) return 'S';
////    }
////    return 'I';
////}
//
//std::string getRelationR(std::string schema, std::string input) {
//    std::ifstream file(schema);
//    std::string fields;
//    std::string name;
//
//    char data_separator = '#';
//
//    while (std::getline(file, fields)) {
//        std::stringstream str(fields);
//        std::istringstream inputStream(input);
//
//        std::getline(str, fields, data_separator);
//        std::string token;
//
//        while (std::getline(inputStream, token, data_separator)) {
//            if (fields == token) {
//                return token;
//            }
//        }
//    }
//
//    return "";
//}
//
//std::string parseQueryCondition(std::string query) { // Use std::string explicitly
//    std::vector<std::string> operators = { "!=", ">=", "<=", ">", "<", "=" }; // Use std::vector and std::string explicitly
//
//    std::string input = parseQueryInput(query);
//
//    bool found;
//    do {
//        found = false;
//        for (const auto& op : operators) {
//            std::string pattern = "#" + op + "#"; // Use std::string explicitly
//            size_t pos = input.find(pattern);
//            if (pos != std::string::npos) {
//                input.replace(pos, pattern.length(), op);
//                found = true;
//                break;
//            }
//        }
//    } while (found);
//
//    return input;
//}
//
//
//void convertCsvToTxt(const string& csvFile, const string& txtFile) {
//    std::ifstream in(csvFile, std::ios::binary);
//    std::ofstream out(txtFile, std::ios::binary);
//    char c;
//
//    while (in.get(c)) {
//        out.put(c == ',' ? '#' : c);
//    }
//
//    in.close();
//    out.close();
//}
//
//
//void duplicatesLines(const string& schema) {
//    vector<string> relationsR;
//    vector<string> keepLines;
//    std::ifstream file(schema);
//    string line;
//    const char sep = '#';
//
//    while (getline(file, line)) {
//        if (line.empty()) continue;
//
//        string rel = line.substr(0, line.find(sep));
//
//        bool duplicate = false;
//        for (string& r : relationsR) {
//            if (r == rel) {
//                duplicate = true;
//                break;
//            }
//        }
//
//        if (!duplicate) {
//            relationsR.push_back(rel);
//            keepLines.push_back(line);
//        }
//    }
//    file.close();
//
//    std::ofstream out(schema, std::ios::trunc);
//    for (auto& l : keepLines) {
//        out << l << "\n";
//    }
//    out.close();
//}
//
//std::vector<std::string> getFields(std::string input, std::string schema, std::string relation) {
//    std::vector<std::string> fields;
//    std::ifstream file(schema);
//
//    std::string line;
//
//    if (relation.empty()) {
//        return {};
//    }
//
//    while (std::getline(file, line)) {
//        if (line.rfind(relation + "#", 0) != 0)
//            continue;
//
//        std::istringstream iss(line);
//        std::string tableName;
//        std::getline(iss, tableName, '#');
//
//        std::vector<std::pair<std::string, std::string>> schemaFields;
//        std::string field, type;
//
//        while (std::getline(iss, field, '#') && std::getline(iss, type, '#')) {
//            schemaFields.push_back({ field, type });
//        }
//
//        std::istringstream inputStream(input);
//        std::string token;
//        while (std::getline(inputStream, token, '#')) {
//            if (token == "*") {
//                for (auto& pair : schemaFields) {
//                    fields.push_back(pair.first);
//                    fields.push_back(pair.second);
//                }
//                return fields;
//            }
//
//            for (auto& pair : schemaFields) {
//                if (token == pair.first) {
//                    fields.push_back(pair.first);
//                    fields.push_back(pair.second);
//                    break;
//                }
//            }
//        }
//
//        return fields;
//    }
//
//    file.close();
//    return {};
//}
//
//
//vector<vector<string>> getConditions(string& input) {
//    vector<vector<string>> conditions; std::istringstream inputStream(input);
//    string token;
//    const vector<string> ops = { ">=", "<=", "!=", ">", "<", "=" };
//
//    while (getline(inputStream, token, '#')) {
//        for (auto& op : ops) {
//            size_t foundPos = token.find(op);
//            if (foundPos != string::npos) {
//                vector<string> cond;
//                cond.push_back(token.substr(0, foundPos)); // field 
//                cond.push_back(op); // operator 
//                cond.push_back(token.substr(foundPos + op.length())); // value 
//                conditions.push_back(cond);
//                break;
//            }
//        }
//    }
//
//    return conditions;
//}
//
//
//int getIndexHeaders(string fileName, string relationR) {
//
//    char data_separator = '#';
//
//    std::ifstream file(fileName);
//    string line;
//    getline(file, line);
//
//    vector<string> fields;
//    std::istringstream str(line);
//    string field;
//
//    int index = 0;
//
//    while (getline(str, field, data_separator)) {
//        fields.push_back(field);
//        if (fields[index] == relationR)
//            return index;
//        index++;
//    }
//
//    return -1;
//}
//
//
//std::vector<std::string> filterAndModify
//    (
//    const std::string& schema,
//    const std::string& fileName,
//    const std::string& relationR,
//    const std::string& input,
//    const std::string& path,
//    std::optional<std::string> saveFile
//    )
//
//{
//    // Campos
//    std::vector<std::string> selectedFields = getFields(input, schema, relationR);
//
//    // Condiciones
//    std::vector<std::vector<std::string>> conditions = getConditions(const_cast<std::string&>(input));
//
//    std::vector<int> fieldIndices;
//    for (size_t i = 0; i < selectedFields.size(); i += 2) {
//        int idx = getIndexHeaders(fileName, selectedFields[i]);
//        if (idx != -1) fieldIndices.push_back(idx);
//    }
//
//    std::vector<int> conditionIndices;
//    for (auto& cond : conditions) {
//        int idx = getIndexHeaders(fileName, cond[0]);
//        if (idx != -1) conditionIndices.push_back(idx);
//    }
//
//    // Procesar
//    std::ifstream dataFile(fileName);
//    std::vector<std::string> results;
//    std::string line;
//
//    std::getline(dataFile, line);
//    int recNum = 0;
//
//    while (std::getline(dataFile, line)) {
//        recNum++;
//        std::vector<std::string> record;
//        std::istringstream lineStream(line);
//        std::string field;
//
//        while (std::getline(lineStream, field, '#')) {
//            record.push_back(field);
//        }
//
//        bool valid = true;
//        for (size_t i = 0; i < conditions.size(); ++i) {
//            int idx = conditionIndices[i];
//            if (idx >= record.size()) {
//                valid = false;
//                break;
//            }
//
//            std::string fieldValue = record[idx];
//            std::string op = conditions[i][1];
//            std::string condValue = conditions[i][2];
//
//            bool ok = true;
//
//            try {
//                double numValue = std::stod(fieldValue);
//                double numCond = std::stod(condValue);
//                if (op == ">=") ok = (numValue >= numCond);
//                else if (op == "<=") ok = (numValue <= numCond);
//                else if (op == ">") ok = (numValue > numCond);
//                else if (op == "<") ok = (numValue < numCond);
//                else if (op == "=") ok = (numValue == numCond);
//                else if (op == "!=") ok = (numValue != numCond);
//            }
//            catch (...) {
//                if (op == "=") ok = (fieldValue == condValue);
//                else if (op == "!=") ok = (fieldValue != condValue);
//            }
//
//            if (!ok) {
//                valid = false;
//                break;
//            }
//        }
//
//        if (valid) {
//            std::string resultLine;
//            for (int idx : fieldIndices) {
//                if (idx < record.size()) {
//                    resultLine += record[idx] + "#";
//                }
//            }
//            if (!resultLine.empty()) {
//                resultLine.pop_back();
//                results.push_back(resultLine);
//            }
//        }
//    }
//
//    std::cout << std::endl;
//
//    if (saveFile.has_value()) {
//        std::string fileName = saveFile.value();
//
//        if (!fileName.empty()) {
//            std::ofstream outputFile(path + saveFile.value());
//
//            for (int i = 0; i < selectedFields.size(); i += 2) {
//                outputFile << selectedFields[i] << "#";
//            }
//            outputFile << std::endl;
//
//            for (auto& line : results) {
//                outputFile << line << std::endl;
//            }
//            outputFile.close();
//
//            // Esquema
//            std::string baseName = saveFile.value();
//            size_t dotPos = baseName.rfind('.');
//            if (dotPos != std::string::npos) {
//                baseName = baseName.substr(0, dotPos);
//            }
//
//            std::ofstream schemaOut(schema, std::ios::app);
//
//            schemaOut << baseName;
//            for (const auto& field : selectedFields) {
//                schemaOut << "#" << field;
//            }
//            schemaOut << std::endl;
//            schemaOut.close();
//
//            const string savedPath = path + saveFile.value();
//            std::cout << "File saved on Path: " << savedPath;
//        }
//        else
//        {
//            std::vector<std::vector<std::string>> table;
//            std::vector<std::string> header;
//
//            if (!selectedFields.empty()) {
//                for (int i = 0; i < selectedFields.size(); i += 2) {
//                    header.push_back(selectedFields[i]);
//                }
//            }
//
//            table.push_back(header);
//
//            // Añadir
//            for (auto& line : results) {
//                std::vector<std::string> row;
//                std::istringstream ss(line);
//                std::string cell;
//                while (std::getline(ss, cell, '#')) {
//                    row.push_back(cell);
//                }
//
//                if (row.size() < header.size()) {
//                    row.resize(header.size(), "");
//                }
//
//                table.push_back(row);
//            }
//
//            // Ancho
//            int cols = header.size();
//            std::vector<size_t> widths(cols, 0);
//
//            for (int j = 0; j < cols; ++j) {
//                for (auto& row : table) {
//                    if (j < row.size()) {
//                        widths[j] = std::max(widths[j], row[j].length());
//                    }
//                }
//            }
//
//            for (int j = 0; j < cols; ++j) {
//                std::cout << std::left << std::setw(widths[j]) << header[j];
//                if (j < cols - 1) std::cout << " | ";
//            }
//            std::cout << "\n";
//
//            for (int j = 0; j < cols; ++j) {
//                std::cout << std::string(widths[j], '-');
//                if (j < cols - 1) std::cout << "|";
//            }
//            std::cout << "\n";
//
//            for (size_t i = 1; i < table.size(); ++i) {
//                for (int j = 0; j < cols; ++j) {
//                    if (j < table[i].size()) {
//                        std::cout << std::left << std::setw(widths[j]) << table[i][j];
//                    }
//                    else {
//                        std::cout << std::string(widths[j], ' ');
//                    }
//                    if (j < cols - 1) std::cout << " | ";
//                }
//                std::cout << "\n";
//            }
//        }
//    }
//
//    std::cout << std::endl;
//
//    dataFile.close();
//    return results;
//}
//
