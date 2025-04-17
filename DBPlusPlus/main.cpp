#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
using namespace std;

vector<string> parse_Line(const string& line) {
    vector<string> fields;
    string field;
    istringstream iss(line);

    while (getline(iss, field, ',')) {
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
        string data_type;
        
        char data_separator = ',';

        getline(file, field);
        getline(file, line1);
        stringstream str(field);
        stringstream str2(line1);

        while (getline(str, data, data_separator)) {
            schemaFile << " # " << data;
            data_type = "";
            getline(str2, data_type, data_separator);

            switch (convCondition(data_type)) {
            case 'I':
                schemaFile << "# int ";
                break;
            case 'D':
                schemaFile << "# double ";
                break;
            default: 
                schemaFile << "# int ";
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
        replace(line.begin(), line.end(), '\t', ',');
        outFile << line << '\n';
    }

    inFile.close();
    outFile.close();
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
    return 0;
}
