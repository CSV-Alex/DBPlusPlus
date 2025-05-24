#define _CRT_SECURE_NO_WARNINGS

#include "TitanicData.h"
#include "Disco.h"
#include "Utils.h"
#include <windows.h>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>

#define MAX_BUF      512
#define MAX_STR_LEN 32
#define MAX_FIELDS 32
#define MAX_PATH_LEN 256
#define MAX_SCHEMA  4096
using namespace std;

size_t relationSizeBytes(const char* fileName, const char* basePath) {
    char tablePath[512];
    snprintf(tablePath, sizeof(tablePath), "%s%s.txt", basePath, fileName);
    std::ifstream in(tablePath, std::ios::binary);

    if (!in) {
        std::cerr << "Error al abrir el dasdasdsarchivo: " << tablePath << std::endl;
        return 0;
    }

    char c;
    size_t count = 0;
    while (in.get(c)) {
        ++count;
    }
    in.close();
    return count;
}

char convConditionMin(const char* data, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        if (data[i] == '.')       return 'D';
        if (!std::isdigit(data[i])) return 'S';
    }
    return 'I';
}

void convertCsvToTxt(const string& csvFile, const string& txtFile) {
    std::ifstream in(csvFile, std::ios::binary);
    std::ofstream out(txtFile, std::ios::binary);
    char c;

    while (in.get(c)) {
        out.put(c == ',' ? '#' : c);
    }

    in.close();
    out.close();
}

void relationFormatMin(const char* fileName, const char* path) {
    FILE* in = fopen(fileName, "rb");
    FILE* out = fopen(path, "ab");
    if (!in || !out) {
        if (in)  fclose(in);
        if (out) fclose(out);
        return;
    }

    const char* p = fileName;
    const char* lastSlash = NULL;
    const char* lastDot = NULL;
    while (*p) {
        if (*p == '/' || *p == '\\') lastSlash = p;
        else if (*p == '.')           lastDot = p;
        ++p;
    }
    const char* nameBegin = lastSlash ? lastSlash + 1 : fileName;
    const char* nameEnd = (lastDot && lastDot > nameBegin) ? lastDot : p;
    size_t nameLen = nameEnd - nameBegin;

    char line0[MAX_BUF] = { 0 }, line1[MAX_BUF] = { 0 }, line2[MAX_BUF] = { 0 };
    size_t len[3] = { 0,0,0 };
    char* lines[3] = { line0, line1, line2 };

    for (int L = 0; L < 3; ++L) {
        int c;
        do {
            c = fgetc(in);
            if (c == EOF) break;
        } while (c == '\r' || c == '\n');
        if (c == EOF) break;

        size_t idx = 0;
        do {
            if (c != '\r' && idx + 1 < MAX_BUF) {
                lines[L][idx++] = (char)c;
            }
            c = fgetc(in);
        } while (c != EOF && c != '\n');
        lines[L][idx] = '\0';
        len[L] = idx;
    }
    fclose(in);

    char schema[MAX_SCHEMA];
    size_t off = 0;

    if (nameLen + 1 < MAX_SCHEMA) {
        memcpy(schema + off, nameBegin, nameLen);
        off += nameLen;
    }

    size_t i0 = 0, i1 = 0, i2 = 0;
    while (i0 < len[0]) {
        if (lines[0][i0] == '#') ++i0;
        if (lines[1][i1] == '#') ++i1;
        if (lines[2][i2] == '#') ++i2;

        size_t s0 = i0;
        while (i0 < len[0] && lines[0][i0] != '#') ++i0;
        size_t tk0 = i0 - s0;

        size_t s1 = i1;
        while (i1 < len[1] && lines[1][i1] != '#') ++i1;
        size_t tk1 = i1 - s1;

        size_t s2 = i2;
        while (i2 < len[2] && lines[2][i2] != '#') ++i2;
        size_t tk2 = i2 - s2;

        if (off + 1 + tk0 < MAX_SCHEMA) {
            schema[off++] = '#';
            memcpy(schema + off, lines[0] + s0, tk0);
            off += tk0;
        }

        char t1 = convConditionMin(lines[1] + s1, tk1);
        char t2 = convConditionMin(lines[2] + s2, tk2);
        char finalType = (t1 == 'D' || t2 == 'D') ? 'D'
            : (t1 == 'S' || t2 == 'S') ? 'S'
            : 'I';
        const char* typstr = (finalType == 'D' ? "#double"
            : finalType == 'S' ? "#string"
            : "#int");
        size_t typlen = strlen(typstr);

        if (off + typlen < MAX_SCHEMA) {
            memcpy(schema + off, typstr, typlen);
            off += typlen;
        }
    }

    fwrite(schema, 1, off, out);
    fputc('\n', out);
    fclose(out);
}

int op_code(const char* op) {
    if (op[0] == '=' && op[1] == '=' && op[2] == '\0') return 1;
    if (op[0] == '=' && op[1] == '\0') return 1;
    if (op[0] == '!' && op[1] == '=' && op[2] == '\0') return 2;
    if (op[0] == '<' && op[1] == '\0') return 3;
    if (op[0] == '>' && op[1] == '\0') return 4;
    if (op[0] == '<' && op[1] == '=' && op[2] == '\0') return 5;
    if (op[0] == '>' && op[1] == '=' && op[2] == '\0') return 6;
    return 0; // invalido
}

bool str_eq(const char* a, const char* b) {
    while (*a && *b) {
        if (*a != *b) return false;
        ++a; ++b;
    }
    return *a == *b;
}

bool str_ne(const char* a, const char* b) {
    return !str_eq(a, b);
}

bool eval_condition(const char* buf, const char* op, const char* val, char type) {
    int opc = op_code(op);
    if (opc == 0) return false;

    if (type == 'I') {
        int a = atoi(buf), b = atoi(val);
        switch (opc) {
        case 1: return a == b;
        case 2: return a != b;
        case 3: return a < b;
        case 4: return a > b;
        case 5: return a <= b;
        case 6: return a >= b;
        }
    }
    else if (type == 'D') {
        double a = atof(buf), b = atof(val);
        switch (opc) {
        case 1: return a == b;
        case 2: return a != b;
        case 3: return a < b;
        case 4: return a > b;
        case 5: return a <= b;
        case 6: return a >= b;
        }
    }
    else {
        switch (opc) {
        case 1: return str_eq(buf, val);
        case 2: return str_ne(buf, val);
        default: return false;
        }
    }

    return false;
}

void mostrar_tabla_console(const char* dataPath,
    const char* esquemaPath,
    const char* fromTable,
    int    fieldCount,
    const char types[],
    int    whereIndex,
    const char whereOp[],
    const char whereVal[])
{
    size_t widths[MAX_FIELDS] = { 0 };
    char   cell[MAX_STR_LEN];
    char   line[512];
    int    c, col, pos;

    FILE* data = fopen(dataPath, "r");
    if (!data) { perror("abrir datos"); return; }

    while (true) {
        long recStart = ftell(data);
        col = pos = 0;
        bool match = false;

        // leer linea
        while ((c = fgetc(data)) != EOF && c != '\n') {
            if (c == '#') {
                cell[pos] = '\0';
                if ((size_t)pos > widths[col]) widths[col] = pos;
                if (col == whereIndex &&
                    eval_condition(cell, whereOp, whereVal, types[col]))
                    match = true;
                ++col; pos = 0;
            }
            else if (pos + 1 < MAX_STR_LEN) {
                cell[pos++] = (char)c;
            }
        }
        cell[pos] = '\0';
        if ((size_t)pos > widths[col]) widths[col] = pos;

        if (c == EOF && recStart == ftell(data)) break;
    }
    fclose(data);

    FILE* sch = fopen(esquemaPath, "r");
    if (!sch) { perror("abrir esquema"); return; }
    while (fgets(line, sizeof(line), sch)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (strncmp(line, fromTable, strlen(fromTable)) != 0)
            continue;
        // saltar "Tabla#"
        char* p = line + strlen(fromTable) + 1;
        for (int i = 0; i < fieldCount; ++i) {
            pos = 0;
            while (*p && *p != '#') {
                if (pos + 1 < MAX_STR_LEN) cell[pos++] = *p;
                ++p;
            }
            cell[pos] = '\0';
            if ((size_t)pos > widths[i]) widths[i] = pos;
            // saltar "#tipo#"
            int hashes = 0;
            while (*p && hashes < 2) {
                if (*p == '#') ++hashes;
                ++p;
            }
        }
        break;
    }
    fclose(sch);

    sch = fopen(esquemaPath, "r");
    if (!sch) return;
    while (fgets(line, sizeof(line), sch)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (strncmp(line, fromTable, strlen(fromTable)) != 0)
            continue;
        char* p = line + strlen(fromTable) + 1;
        for (int i = 0; i < fieldCount; ++i) {
            pos = 0;
            while (*p && *p != '#') {
                if (pos + 1 < MAX_STR_LEN) cell[pos++] = *p;
                ++p;
            }
            cell[pos] = '\0';
            printf("%-*s", (int)widths[i], cell);
            if (i < fieldCount - 1) printf(" | ");
            // saltar "#tipo#"
            int hashes = 0;
            while (*p && hashes < 2) {
                if (*p == '#') ++hashes;
                ++p;
            }
        }
        putchar('\n');
        break;
    }
    fclose(sch);

    // linea separadora
    for (int i = 0; i < fieldCount; ++i) {
        for (size_t j = 0; j < widths[i]; ++j) putchar('-');
        if (i < fieldCount - 1) fputs("-+-", stdout);
    }
    putchar('\n');

    data = fopen(dataPath, "r");

    char rowBufs[MAX_FIELDS][MAX_STR_LEN];

    while (true) {
        long recStart = ftell(data);
        col = pos = 0;
        bool match = false;

        while ((c = fgetc(data)) != EOF && c != '\n') {
            if (c == '#') {
                cell[pos] = '\0';

                if (col == whereIndex &&
                    eval_condition(cell, whereOp, whereVal, types[col]))
                    match = true;

                strncpy(rowBufs[col], cell, MAX_STR_LEN);
                rowBufs[col][MAX_STR_LEN - 1] = '\0';

                ++col; pos = 0;
            }
            else if (pos + 1 < MAX_STR_LEN) {
                cell[pos++] = (char)c;
            }
        }
        cell[pos] = '\0';
        strncpy(rowBufs[col], cell, MAX_STR_LEN);
        rowBufs[col][MAX_STR_LEN - 1] = '\0';

        if (c == EOF && recStart == ftell(data)) break;

        if (match) {
            for (int i = 0; i < fieldCount; ++i) {
                printf("%-*s", (int)widths[i], rowBufs[i]);
                if (i < fieldCount - 1) printf(" | ");
            }
            putchar('\n');
        }
    }
    fclose(data);
}

bool fileToSaveName(const char* input, char* outName, size_t maxLen) {
    const char* pipe = std::strchr(input, '|');
    if (!pipe) return false;

    const char* p = pipe + 1;
    while (*p && std::isspace((unsigned char)*p)) ++p;
    if (!*p) return false;

    const char* start = p;
    while (*p && !std::isspace((unsigned char)*p) && *p != '#' && *p != ';')
        ++p;
    size_t nameLen = p - start;
    if (nameLen == 0) return false;

    const char* ext = ".txt";
    size_t extLen = 4;
    if (nameLen + extLen + 1 > maxLen) return false;

    std::memcpy(outName, start, nameLen);
    std::memcpy(outName + nameLen, ext, extLen + 1);

    return true;
}

void agregarTablaCatalogo(const char* basePath, const char* fromTable) {
    char input[MAX_STR_LEN];
    std::strncpy(input, fromTable, MAX_STR_LEN - 1);
    input[MAX_STR_LEN - 1] = '\0'; // nulo

    char catalogoPath[512];
    snprintf(catalogoPath, sizeof(catalogoPath), "%s%s", basePath, "catalog.txt");

    std::fstream catalogo(catalogoPath, std::ios::in);
    if (catalogo.is_open()) {
        char line[MAX_PATH_LEN];
        bool exists = false;

        while (catalogo.getline(line, MAX_PATH_LEN, '\n')) {
            char* sep = std::strchr(line, '|');
            if (sep) {
                *sep = '\0';
                if (strcmp(line, input) == 0) {
                    exists = true;
                    break;
                }
            }
        }
        catalogo.close();
    }

    catalogo.open(catalogoPath, std::ios::app);
    if (catalogo.is_open()) {
        catalogo << input << "|" << basePath << input << ".txt" << "\n";
        catalogo.close();
        std::cout << "Entrada agregada exitosamente!\n";
    }
    else {
        std::perror("Error al abrir catalogo");
    }
}

void ejecutar_query(const char* selectField,
    const char* fromTable,
    const char* whereInput,
    const char* schema,
    const char* basePath)
{
    char tablePath[512];
    snprintf(tablePath, sizeof(tablePath), "%s%s", basePath, fromTable);
    char catalogoPath[512];
    snprintf(catalogoPath, sizeof(catalogoPath), "%s%s", basePath, "catalog.txt");
    char esquemaPath[512];
    snprintf(esquemaPath, sizeof(esquemaPath), "%s%s", basePath, "esquema.txt");

    char whereField[MAX_STR_LEN], whereOp[3], whereVal[MAX_STR_LEN];
    if (sscanf(whereInput, "%31s %2s %31s",
        whereField, whereOp, whereVal) != 3) {
        fprintf(stderr, "Formato WHERE incorrecto: '%s'\n", whereInput);
        return;
    }

    const bool selectAll = (strcmp(selectField, "*") == 0);

    FILE* catalogo = fopen(catalogoPath, "r");
    if (!catalogo) { perror("abrir catalog.txt"); return; }

    char line[256], name[64], path[192];
    bool table_found = false;
    while (fgets(line, sizeof(line), catalogo)) {
        line[strcspn(line, "\r\n")] = '\0';
        char* sep = strchr(line, '|');
        if (!sep) {
            fprintf(stderr, "formato catalogo\n");
            fclose(catalogo);
            return;
        }
        *sep = '\0';
        strcpy(name, line);
        strcpy(path, sep + 1);
        if (strcmp(name, fromTable) == 0) {
            table_found = true;
            break;
        }
    }
    fclose(catalogo);
    if (!table_found) {
        fprintf(stderr, "Tabla '%s' no hallada\n", fromTable);
        return;
    }
    FILE* sch = fopen(esquemaPath, "r");
    if (!sch) {
        perror("abrir esquema.txt");
        return;
    }

    char buf[512]; //512
    int fieldCount = 0;
    char types[MAX_FIELDS];
    int whereIndex = -1;

    while (fgets(buf, sizeof(buf), sch)) {
        buf[strcspn(buf, "\r\n")] = '\0';
        if (buf[0] == '\0' || buf[0] == '#')
            continue;

        long pos = ftell(sch);
        char peek[MAX_PATH_LEN];
        if (fgets(peek, sizeof(peek), sch)) {
            if (peek[0] == '#') {
                peek[strcspn(peek, "\r\n")] = '\0';
                strncat(buf, peek, sizeof(buf) - strlen(buf) - 1);
            }
            else {
                fseek(sch, pos, SEEK_SET);
            }
        }

        char* tableName = strtok(buf, "#");
        if (!tableName || strcmp(tableName, fromTable) != 0)
            continue;

        char* tok;
        int idx = 0;
        while ((tok = strtok(nullptr, "#")) && idx < MAX_FIELDS) {
            char fld[MAX_STR_LEN];
            strncpy(fld, tok, MAX_STR_LEN);
            fld[MAX_STR_LEN - 1] = '\0';

            tok = strtok(nullptr, "#");
            if (!tok) break;

            char t = (tok[0] == 'i') ? 'I'
                : (tok[0] == 'd') ? 'D'
                : 'S';
            types[idx] = t;

            if (strcmp(fld, whereField) == 0)
                whereIndex = idx;

            idx++;
        }
        fieldCount = idx;
        break;
    }
    fclose(sch);

    if (whereIndex < 0) {
        fprintf(stderr, "Campo '%s' no en esquema\n", whereField);
        return;
    }

    char outNameBuf[MAX_STR_LEN];
    bool hasSave = fileToSaveName(whereInput, outNameBuf, sizeof(outNameBuf));
    const char* saveName = hasSave ? outNameBuf : nullptr;

    if (!saveName) {
        mostrar_tabla_console(path, esquemaPath, fromTable,
            fieldCount, types, whereIndex,
            whereOp, whereVal);
        return;
    }
    else {
        char saveNamePath[512];
        snprintf(saveNamePath, sizeof(saveNamePath), "%s%s", basePath, saveName);

        FILE* data = fopen(path, "r");
        ofstream outFile(saveNamePath);

        int headerChar;
        while ((headerChar = fgetc(data)) != EOF && headerChar != '\n') {
            outFile.put(headerChar);
        }
        outFile.put('\n');

        char field_buf[MAX_STR_LEN];
        int col = 0, pos = 0;
        bool match = false;
        long recStart = ftell(data);

        while (1) {
            int c = fgetc(data);
            if (c == EOF) break;

            if (c == '#' || c == '\n') {
                field_buf[pos] = '\0';
                if (col == whereIndex) {
                    if (eval_condition(field_buf, whereOp, whereVal, types[col]))
                        match = true;
                }
                pos = 0;

                if (c == '\n') {
                    if (match) {
                        if (selectAll) {
                            long after = ftell(data);
                            fseek(data, recStart, SEEK_SET);

                            int ch;
                            while ((ch = fgetc(data)) != '\n' && ch != EOF)
                                outFile.put(ch);
                            outFile.put('\n');
                            fseek(data, after, SEEK_SET);
                        }
                    }

                    recStart = ftell(data);
                    col = 0;
                    match = false;
                }
                else {
                    col++;
                }
            }
            else {
                if (pos + 1 < MAX_STR_LEN)
                    field_buf[pos++] = (char)c;
            }
        }
        fclose(data);
        outFile.close();
        char bareSaveName[MAX_STR_LEN];
        {
            size_t L = strlen(saveName);
            if (L > 4 && strcmp(saveName + L - 4, ".txt") == 0) {
                memcpy(bareSaveName, saveName, L - 4);
                bareSaveName[L - 4] = '\0';
            }
            else {
                strcpy(bareSaveName, saveName);
            }
        }

        agregarTablaCatalogo(basePath, bareSaveName);
        relationFormatMin(saveNamePath, esquemaPath);

        return;
    }
}

int main() {

    Disco miDisco(/*platos=*/2, /*pistas=*/100, /*sectores=*/20,
        /*tamSector=*/30, /*tamBloque=*/120);

    const string basePath = "D:\\DBPlusPlus\\DBPlusPlus\\data\\usr\\db\\";
    const string schemaPath = basePath + "esquema.txt";
    const string titanicCSV = basePath + "titanicG.csv";
    const string housingCSV = basePath + "Housing.csv";
    const string titanicTXT = basePath + "titanic.txt";
    const string housingTXT = basePath + "housing.txt";

    convertCsvToTxt(titanicCSV, titanicTXT);
    convertCsvToTxt(housingCSV, housingTXT);

    const char* txt_Char = housingTXT.c_str();

    const char* pathSchema_Char = schemaPath.c_str();

    const char* txtTitanic_Char = titanicTXT.c_str();

    const char* pathSchemaTitanic_Char = schemaPath.c_str();

    relationFormatMin(txt_Char, pathSchema_Char);
    relationFormatMin(txtTitanic_Char, pathSchemaTitanic_Char);

    cout << "*************************************************************************************" << endl;
    cout << "Welcome to MEGATRON 3000 (con simulador de disco)\n\n";

    while (true) {
        cout << "1) Adicionar relacion y asignar bloque\n";
        cout << "2) Ejecutar consulta SQL (SELECT/FROM/WHERE…)\n";
        cout << "3) Consultar bloque por relacion\n";
        cout << "4) Volcar bloque a sectores fisicos\n";
        cout << "5) Mostrar caracteristicas del disco\n";
        cout << "6) Quit\n> ";

        string opt;
        getline(cin, opt);
        if (opt == "6" || opt == "quit") break;

        if (opt == "1") {
            cout << "Nombre de tabla a adicionar (p.ej. titanic): ";
            string tabla;
            getline(cin, tabla);

            miDisco.adicionarRelacion("ignoradoBasePath", tabla.c_str());
            // basePath
            // archivos ya estan en “DISCO/...”
            // modificar “construirRutaBloqueDesdeNombre”.
        }
        else if (opt == "2") {

            string select, fromTable, whereCondition;
            cout << "SELECT "; getline(cin, select);
            cout << "FROM ";   getline(cin, fromTable);
            cout << "WHERE ";  getline(cin, whereCondition);

            agregarTablaCatalogo(basePath.c_str(), fromTable.c_str());
            ejecutar_query(select.c_str(), fromTable.c_str(), whereCondition.c_str(), schemaPath.c_str(), basePath.c_str());
            cout << "Tabla: " << fromTable << " tama;o de: " << relationSizeBytes(fromTable.c_str(), basePath.c_str()) << "\n";
            //remove_duplicates(schemaPath.c_str(), '#');
            //remove_duplicates(schemaPath.c_str(), '#');
        }
        else if (opt == "3") {
            cout << "¿Que relacion quieres consultar? ";
            string tabla; getline(cin, tabla);
            miDisco.consultarBloquePorRelacion(tabla.c_str());
        }
        else if (opt == "4") {
            cout << "¿Que relacion quieres volcar a sectores? ";
            string tabla; getline(cin, tabla);
            int blk = miDisco.obtenerBloqueDeRelacion(tabla.c_str());
            if (blk == 0) {
                cout << "> Relacion no encontrada en catalogo.txt.\n";
            }
            else {
                miDisco.volcarBloqueASectores(blk);
            }
        }
        else if (opt == "5") {
            miDisco.printDisco();
        }
        else {
            cout << "Opcion invalida\n";
        }
    }
    return 0;
}
