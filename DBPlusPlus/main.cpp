#include "TitanicData.h"
#include "Disco.h"
#include "Utils.h"
#include <windows.h>
#include <iostream>
#include <fstream>

using namespace std;

int main() {
    TitanicData titanicData;

    const string tsv = "D:\\DBPlusPlus\\DBPlusPlus\\data\\usr\\db\\titanic.tsv";
    const string txt = "D:\\DBPlusPlus\\DBPlusPlus\\data\\usr\\db\\titanic.txt";
    const string pathGeneric = "D:\\DBPlusPlus\\DBPlusPlus\\data\\usr\\db\\";

    convertTsvToTxt(tsv, txt);

    titanicData.readFromFile(txt);

    const size_t diskCapacity = 2 * 1024 * 1024;
    size_t used = titanicData.totalMemoryUsed();
    size_t freeSpace = diskCapacity - used;

    cout << "Disco total: " << diskCapacity << " bytes" << endl;
    cout << "Espacio usado: " << used << " bytes" << endl;
    cout << "Espacio libre: " << freeSpace << " bytes" << endl;

    Disco Disco1;
    Disco1.relationFormat(txt, pathGeneric);

    const string pathDB = "D:\\DBPlusPlus\\DBPlusPlus\\data\\usr\\db\\";
    const string pathSchema = "D:\\DBPlusPlus\\DBPlusPlus\\data\\usr\\db\\esquema.txt";

    cout << "*************************************************************************************" << endl;
    cout << "Welcome to MEGATRON 3000\n\n";

    while (true) {
        duplicatesLines(pathSchema);
        
        cout << "1) Consulta SQL\n"
             << "2) quit\n\n> ";

        string option;
        string firstInput;
        string secondInput;
        string thirdInput;

        getline(cin, option);

        //if (option == "2" || option == "quit") break;
        //if (option != "1") continue;

        //cout << "SQL> ";
        //string userQueryInput;
        //string userQuery;
        //getline(cin, userQueryInput);

        if (option == "1")
        if (option == "1") {
            cout << "SELECT ";
            getline(cin, firstInput);
            
            cout << "FROM ";
            getline(cin, secondInput);

            cout << "\n 2) WHERE? \n";
            if (true) {
                cout << "WHERE ";
                getline(cin, thirdInput);
            }
        }

        if (option == "2" || option == "quit") break;

        if (option != "1" && option != "2" && option != "3") continue;

        // cout << "SQL> ";
        
        string userQueryInput = "SELECT " + firstInput + " " + "FROM " + secondInput + " " + "WHERE " + thirdInput + " ";
        string userQuery;
        //

        if (option == "3") {
            getline(cin, userQueryInput);
        }

        if (userQueryInput == "ejemplo1") {
            userQuery = "& SELECT * FROM titanic WHERE PassengerId >= 30 #";
            cout << userQuery << endl;
        }
        else if (userQueryInput == "ejemplo2") {
            userQuery = "& SELECT PassengerId , Name , Survived , Sex FROM titanic WHERE Sex = female AND PassengerId >= 30 #";
            cout << userQuery << endl;
        }
        else if (userQueryInput == "ejemplo3") {
            userQuery = "& SELECT PassengerId , Name , Survived , Sex FROM titanic WHERE Sex = male AND PassengerId >= 30 | HighId #";
            cout << userQuery << endl;
        }
        else if (userQueryInput == "ejemplo4") {
            userQuery = "& SELECT PassengerId , Name , Survived , Sex FROM HighId WHERE Sex = male AND PassengerId >= 40 | HighId #";
            cout << userQuery << endl;
        }
        else if (userQueryInput == "ejemplo5") {
            userQuery = "& SELECT PassengerId , Name , Survived , Sex FROM HighId WHERE Sex = female AND PassengerId >= 40 | IdHighFemale #";
            cout << userQuery << endl;
        }
        else {
            userQuery = userQueryInput;
        }

        string parsedQuery = parseQueryCondition(userQuery);

        string saveName = fileToSaveName(parsedQuery);

        string relationR = getRelationR(pathSchema, parsedQuery);
        if (relationR.empty()) {
            cout << "Relacion '" << relationR << "' no encontrada en el esquema.\n\n";
            continue;
        }

        // Obtener .txt
        string pathData = pathDB + relationR + ".txt";

        vector<string> filteredData = filterAndModify(
            pathSchema,
            pathData,
            relationR,
            parsedQuery,
            pathDB,
            saveName
        );

        if(filteredData.empty()){
            cout << "No se encontro las condiciones validas en el esquema.\n\n";
            continue;
        }
        cout << endl;
        cout << endl;

    }
    return 0;
}
