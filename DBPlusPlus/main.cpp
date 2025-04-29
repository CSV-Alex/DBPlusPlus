#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <tuple>

#define SEPARATOR #

using namespace std;


vector<string> parse_Line(const string& line) {
    vector<string> fields;
    string field;
    istringstream iss(line);

    while (getline(iss, field, '#')) {
        fields.push_back(field);
    }

    return fields;
}

char convCondition(string data) {
    for (int i = 0; i < data.size(); i++) {
        if (data[i] == '.') return 'D';
        if (!isdigit(data[i])) return 'S';
    }
    return 'I';
}

class Titanic {
private:
    int passengerId;
    int survived;
    int pclass;
    string name;
    string sex;
    double age;
    int sibSp;
    int parch;
    string ticket;
    double fare;
    string cabin;
    string embarked;

    int safe_stoi(const string& s) {
        if (s.empty()) return -1;
        try {
            return stoi(s);
        }
        catch (...) {
            return -1;
        }
    }

    double safe_stod(const string& s) {
        if (s.empty()) return -1.0;
        try {
            string fix = s;
            replace(fix.begin(), fix.end(), ',', '.');
            return stod(fix);
        }
        catch (...) {
            return -1.0;
        }
    }


public:
    Titanic(vector<string>& data) {
        try {
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
        catch (const exception& e) {
            cerr << "Error: " << e.what() << endl;
        }
    }

    size_t getMemorySize() const {
        return sizeof(passengerId) + sizeof(survived) + sizeof(pclass) + sizeof(age) +
            sizeof(sibSp) + sizeof(parch) + sizeof(fare) +
            name.capacity() + sex.capacity() + ticket.capacity() +
            cabin.capacity() + embarked.capacity();
    }
};

class TitanicData {
private:
    vector<Titanic> passengers;

public:
    void readFromFile(const string& filename) {
        ifstream file(filename);

        string line;
        getline(file, line);

        int lineNum = 1;
        while (getline(file, line)) {
            lineNum++;
            vector<string> row = parse_Line(line);

            passengers.emplace_back(row);
        }

        file.close();
    }

    size_t totalMemoryUsed() const {
        size_t total = 0;
        for (const auto& p : passengers) {
            total += p.getMemorySize();
        }
        return total;
    }

    size_t count() const {
        return passengers.size();
    }
};

class Disco {
private:
    string schema = "esquema.txt";
public:
    void relationFormat(string fileName) {
        ofstream schemaFile(schema, ios::app);
        ifstream file(fileName);

        string schemaShortName = fileName.substr(fileName.find_last_of("\\") + 1);
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
        stringstream str(field);
        stringstream str1(line1);
        stringstream str2(line2);
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
        schemaFile << endl;
        file.close();
    }
};

void convertTsvToTxt(const string& tsvFile, const string& txtFile) {
    ifstream inFile(tsvFile);
    ofstream outFile(txtFile);

    string line;
    while (getline(inFile, line)) {
        replace(line.begin(), line.end(), '\t', '#');
        outFile << line << '\n';
    }

    inFile.close();
    outFile.close();
}

string getRelationR(string schema, string relation_name) {
    ifstream file(schema);
    string fields;
    string name;

    char data_separator = '#';
    getline(file, fields);
    stringstream str(fields);
    getline(str, name, data_separator);

    return name;
}

string getAttributeA(string schema, string attribute_name) {
    ifstream file(schema);
    string fields;
    string name;

    char data_separator = '#';

    if (getline(file, fields)) {

        stringstream str(fields);
        while (getline(str, name, data_separator)) {

            if (name == attribute_name) {
                name = attribute_name;
                break;
            }
        }
    }
   
    return name;
}

string parseQueryCondition(string& input) {
    vector<string> operators = { "!=", ">=", "<=", ">", "<", "=" };

    size_t earliestPos = string::npos;
    string foundOp;
    for (string& op : operators) {
        size_t pos = input.find(op);
        while (pos != string::npos) {
            if (pos > 0 && input[pos - 1] == '#' &&
                (pos + op.length() < input.length()) &&
                input[pos + op.length()] == '#') {
                if (earliestPos == string::npos || pos < earliestPos) {
                    earliestPos = pos;
                    foundOp = op;
                }
                break;
            }
            pos = input.find(op, pos + 1);
        }
    }

    if (earliestPos == string::npos) {
        return input;
    }

    string modified = input.substr(0, earliestPos - 1) + foundOp +
                      input.substr(earliestPos + foundOp.length() + 1);

    return modified;
}

int getIndexHeaders(string fileName, string relationR) {

    char data_separator = '#';

    ifstream file(fileName);
    string line;
    getline(file, line);

    vector<string> fields;
    istringstream str(line);
    string field;

    int index = 0;

    while (getline(str, field, data_separator)) {
        fields.push_back(field);
        if (fields[index] == relationR) break;
        index++;
    }

    return index;
}

int main() {
    TitanicData titanicData;

    const string tsv = "D:\\DBPlusPlus\\DBPlusPlus\\data\\usr\\db\\titanic.tsv";
    const string txt = "D:\\DBPlusPlus\\DBPlusPlus\\data\\usr\\db\\titanic.txt";

    convertTsvToTxt(tsv, txt);

    titanicData.readFromFile(txt);

    const size_t diskCapacity = 2 * 1024 * 1024;
    size_t used = titanicData.totalMemoryUsed();
    size_t freeSpace = diskCapacity - used;

    cout << "Disco total: " << diskCapacity << " bytes" << endl;
    cout << "Espacio usado: " << used << " bytes" << endl;
    cout << "Espacio libre: " << freeSpace << " bytes" << endl;

    Disco Disco1;
    Disco1.relationFormat("D:\\DBPlusPlus\\DBPlusPlus\\data\\usr\\db\\titanic.txt");

    string path = "D:\\DBPlusPlus\\DBPlusPlus\\esquema.txt";

    cout << getRelationR(path, "titanic") << endl;
    cout << getAttributeA(path, "Survivehd") << endl;

    string testQuery1 = "&#SELECT#*#FROM#titanic#WHERE#id#>#20##";
    string parsedQuery1 = parseQueryCondition(testQuery1);
    cout << parsedQuery1 << endl;

    string testQuery2 = "&#SELECT#name#FROM#titanic#WHERE#age#!=#30##";
    string parsedQuery2 = parseQueryCondition(testQuery2);
    cout << parsedQuery2 << endl;

    return 0;
}
