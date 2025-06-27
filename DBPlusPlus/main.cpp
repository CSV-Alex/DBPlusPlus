#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include "Disco.h"
#include <windows.h>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <limits>
#include <cassert>
#include "LVariable.h"
#include "LFija.h"
#include "Buffer/BufferPool.h"
#include "Buffer/clock.h"
#include "Buffer/replacementStrategy.h"

#define MAX_BUF      1024
#define MAX_STR_LEN 64
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
    snprintf(catalogoPath, sizeof(catalogoPath), "%s%s", basePath, "catalogo.txt");

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
    const char* basePath,
    Disco& disco,
    const char* discoPath)
{
    char tablePath[512];
    snprintf(tablePath, sizeof(tablePath), "%s%s", basePath, fromTable);
    char catalogoPath[512];
    snprintf(catalogoPath, sizeof(catalogoPath), "%s%s", basePath, "catalogo.txt");
    char esquemaPath[512];
    snprintf(esquemaPath, sizeof(esquemaPath), "%s%s", basePath, "esquema.txt");

    // Parsear "WHERE campo op valor"
    char whereField[MAX_STR_LEN], whereOp[3], whereVal[MAX_STR_LEN];
    if (sscanf(whereInput, "%31s %2s %31s",
        whereField, whereOp, whereVal) != 3) {
        fprintf(stderr, "Formato WHERE incorrecto: '%s'\n", whereInput);
        return;
    }

    const bool selectAll = (strcmp(selectField, "*") == 0);

    // 1. Buscar en catalogo.txt la ultima ruta real de la tabla
    FILE* catalogo = fopen(catalogoPath, "r");
    if (!catalogo) { perror("abrir catalogo.txt"); return; }

    char line[256], name[64], path[192];
    bool table_found = false;
    char matchedPath[192] = { 0 };

    while (fgets(line, sizeof(line), catalogo)) {
        line[strcspn(line, "\r\n")] = '\0';
        char* sep = strchr(line, '|');
        if (!sep) {
            fclose(catalogo);
            return;
        }
        *sep = '\0';
        strcpy(name, line);
        strcpy(path, sep + 1);
        if (strcmp(name, fromTable) == 0) {
            table_found = true;
            strncpy(matchedPath, path, sizeof(matchedPath) - 1);
        }
    }
    fclose(catalogo);

    if (!table_found) {
        fprintf(stderr, "Tabla '%s' no hallada\n", fromTable);
        return;
    }

    // 2. Leer esquema para determinar indices, tipos y conteo de campos
    FILE* sch = fopen(esquemaPath, "r");
    if (!sch) { perror("abrir esquema.txt"); return; }

    char buf[512];
    int fieldCount = 0;
    char types[MAX_FIELDS];
    int whereIndex = -1;

    while (fgets(buf, sizeof(buf), sch)) {
        buf[strcspn(buf, "\r\n")] = '\0';
        if (buf[0] == '\0' || buf[0] == '#') continue;

        long pos = ftell(sch);
        char peek[512];
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
        if (!tableName || strcmp(tableName, fromTable) != 0) continue;

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

            if (strcmp(fld, whereField) == 0) {
                whereIndex = idx;
            }
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

    // 3. Determinar si hay que guardar en nuevo archivo
    char outNameBuf[MAX_STR_LEN];
    bool hasSave = fileToSaveName(whereInput, outNameBuf, sizeof(outNameBuf));
    const char* saveName = hasSave ? outNameBuf : nullptr;

    // 4. Mostrar o guardar segun corresponda, usando matchedPath
    if (!saveName) {
        mostrar_tabla_console(matchedPath, esquemaPath, fromTable,
            fieldCount, types, whereIndex,
            whereOp, whereVal);
        return;
    }
    else {
        // —— Crear <saveName>.txt y escribir filas que cumplan
        char saveNamePath[512];
        snprintf(saveNamePath, sizeof(saveNamePath), "%s%s", basePath, saveName);

        FILE* data = fopen(path, "r");
        if (!data) {
            perror("abrir archivo de datos");
            return;
        }

        std::ofstream outFile(saveNamePath);

        // Copiar cabecera (primer linea) sin filtrar
        int headerChar;
        while ((headerChar = fgetc(data)) != EOF && headerChar != '\n') {
            outFile.put(headerChar);
        }
        outFile.put('\n');

        char field_buf[MAX_STR_LEN];
        int col = 0, pos = 0;
        bool match = false;
        long recStart = ftell(data);

        while (true) {
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
                if (pos + 1 < MAX_STR_LEN) {
                    field_buf[pos++] = (char)c;
                }
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
        //disco.adicionarRelacion(discoPath, bareSaveName, basePath);

        return;
    }
}

std::string leerArchivo(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("No se pudo abrir el archivo: " + path);
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}


int main() {
    // Variables para configuración de disco
    bool discoConfigDone = false;
    BufferPool* pBufPool = nullptr;

    const string basePath = "data\\usr\\db\\";
    const string discoPath = "DISCO\\";
    const string schemaPath = basePath + "esquema.txt";
    const string titanicCSV = basePath + "titanicG.csv";
    const string housingCSV = basePath + "Housing.csv";
    const string titanicTXT = basePath + "titanic.txt";
    const string housingTXT = basePath + "housing.txt";

    // Valores por defecto
    const int def_platos = 4;
    const int def_pistas = 5;
    const int def_sectores = 10;
    const int def_tamSector = 800;
    const int def_tamBloque = 6400;
    Disco miDisco(def_platos, def_pistas, def_sectores, def_tamSector, def_tamBloque);


    while (true) {
        // Menú principal
        cout << "\n=== MEGATRON 3000 - MENU PRINCIPAL ===\n";
        cout << "1) Menu Disco" << (discoConfigDone ? " (ya configurado)" : "") << "\n";
        cout << "2) Menu Buffer" << (discoConfigDone ? "" : " (requiere config de disco)") << "\n";
        cout << "3) Salir\n";
        cout << ">> ";
        int choice;
        if (!(cin >> choice)) break;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        if (choice == 1) {
            // Menú Disco
            if (!discoConfigDone) {
                cout << "\n--- Configuración del Disco ---\n";
                cout << "1) Usar configuración por defecto\n";
                cout << "2) Ingresar configuración personalizada\n";
                cout << "3) Usar disco ya existente\n";
                cout << ">> ";
                int modo;
                cin >> modo;
                cin.ignore(numeric_limits<streamsize>::max(), '\n');

                int platos, pistas, sectores, tamSector, sectores_per_bloque, tamBloque;
                if (modo == 2) {
                    cout << "Numero de platos: "; cin >> platos;
                    cout << "Numero de pistas: "; cin >> pistas;
                    cout << "Numero de sectores: "; cin >> sectores;
                    cout << "Tamaño del sector (bytes): "; cin >> tamSector;
                    cout << "Sectores por bloque: "; cin >> sectores_per_bloque;
                    tamBloque = tamSector * sectores_per_bloque;
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                }
                else if (modo == 1) {
                    platos = def_platos;
                    pistas = def_pistas;
                    sectores = def_sectores;
                    tamSector = def_tamSector;
                    tamBloque = def_tamBloque;
                }
                else {
                    platos = def_platos;
                    pistas = def_pistas;
                    sectores = def_sectores;
                    tamSector = def_tamSector;
                    tamBloque = def_tamBloque;
                    cout << "> Continuando con disco existente...\n";
                }

                miDisco = Disco(platos, pistas, sectores, tamSector, tamBloque);

                miDisco.printDisco();
                miDisco.mostrarArbolDisco();
                convertCsvToTxt(titanicCSV, titanicTXT);
                convertCsvToTxt(housingCSV, housingTXT);
                relationFormatMin(titanicTXT.c_str(), schemaPath.c_str());
                relationFormatMin(housingTXT.c_str(), schemaPath.c_str());
                calcularLongitudFija(titanicTXT.c_str());
                calcularLongitudFija(housingTXT.c_str());

                /*--------------------------Estatico----------------------------------*/
                bool usarLRU = false;  // o true, según quieras LRU por defecto
                /*--------------------------------------------------------------------*/

                // --- o bien preguntar en consola ---
                //std::cout << "Usar LRU (1) o Clock (2)? ";
                //int opc;
                //std::cin >> opc;
                //bool usarLRU = (opc == 1);

                /*--------------------------------------------------------------------*/

                // Inicializar BufferPool
                if (pBufPool) delete pBufPool;
                int n_frames = 2; // Númdsaero dde frames por defecto


                // Creamos segun eleccion
                std::unique_ptr<ReplacementStrategy> replacer;
                if (usarLRU)
                    replacer = std::make_unique<LRU>();
                else
                    replacer = std::make_unique<Clock>(n_frames);

                pBufPool = new BufferPool(n_frames, 
                                          (size_t)miDisco.getTamBloque(),
                                          miDisco, 
                                          std::move(replacer));

                discoConfigDone = true;
            }
            else {

                cout << "\n***** Bienvenido a MEGATRON 3000 *****\n\n";

                while (true) {
                    cout << "0) Mostrar Ruta Bloque (Dinamico)\n"; ///
                    cout << "1) Mostrar caracteristicas del disco\n"; ///
                    cout << "2) Adicionar registro\n"; ///
                    cout << "3) Adicionar N registros desde CSV\n"; ///
                    cout << "4) Adicionar todo CSV\n"; ///
                    cout << "5) Eliminar registro\n"; ///
                    cout << "6) Modificar registro\n"; ///
                    cout << "7) Inserción de longitud variabl (Demo)\n"; ///
                    cout << "8) Ejecutar consulta SQL\n"; ///
                    cout << "9) Adicionar/Volcar relacion a Sectores\n"; ///
                    cout << "10) Salir\n"; ///
                    cout << "11) Auxiliar Mostrar\n";
                    cout << ">> ";

                    string opt;
                    getline(cin, opt);
                    if (opt == "10") break;

                    if (opt == "0") {
                        int numeroBloque;
                        int opcion;
                        cout << "Numero de Bloque que desea consultar: ";
                        cin >> numeroBloque;
                        cin.ignore(1, '\n');

                        // Mostrar PATHs
                        cout << "Ingrese opcion\n";
                        cout << "1) Con Espacios Libres\n";
                        cout << "2) Rutas Solamente\n";
                        cout << ">> ";
                        cin >> opcion;
                        mostrarSectoresDeBloque(numeroBloque, opcion, miDisco);
                        cout << "Mostrado Correctamente\n";
                        cout << "Mensaje Probar Final\n";
                    }
                    else if (opt == "1") {
                        miDisco.printDisco();
                        miDisco.printCapacidadesDetalle();
                        miDisco.mostrarArbolDisco();
                    }
                    // Supongamos tabla="titanic"
                    else if (opt == "2") {

                        cout << "Ingrese registro (campos separados por #, p.ej. \"1#John Doe#30\\n\"): ";
                        cout << "1790000#4000#3#1#2#yes#no#no#nohfdhjf#no#0#no#unfurnished\n" << endl;
                        string input = "890#1#1#\"Behr# Mr.Karl Howell\"#male#26#0#0#111369#30#C148#C\n";
                        if (input.back() != '\n')
                            input.push_back('\n');

                        cout << "¿Tipo de inserción?\n";
                        cout << "1) Longitud variable\n";
                        cout << "2) Longitud fija (bitmap)\n";
                        cout << ">> ";
                        int opcion;
                        cin >> opcion;
                        cin.ignore(1, '\n');

                        bool ok = false;
                        if (opcion == 1)
                            std::cout << "" << std::endl;
                        else if (opcion == 2) {
                            ok = adicionarRegistroUnico(input.c_str(), "titanic", miDisco);
                        }
                        else {
                            std::cout << "Opcion Invalida " << std::endl; break;
                        }

                        if (ok)
                            cout << "Registro agregado correctamente\n";
                        else
                            cout << "No se pudo agregar el registro\n";
                    }

                    else if (opt == "3") {
                        int n;
                        cout << "¿Cuantos registros desea agregar? ";
                        cin >> n;
                        cin.ignore(1, '\n');

                        cout << "¿Tipo de inserción?\n";
                        cout << "1) Longitud variable\n";
                        cout << "2) Longitud fija (bitmap)\n";
                        cout << ">> ";
                        int opcion;
                        cin >> opcion;
                        cin.ignore(1, '\n');

                        if (adicionarNRegistros(n, "D:\\DBPlusPlus\\DBPlusPlus\\data\\usr\\db\\Housing.csv", "housing", opcion, miDisco)) {
                            cout << "HRegdsadsaistros agregados correctamente.\n";
                            //
                        }
                        else
                            cout << "No se puddsaieron dsaadsdsagregar los registros.\n";
                    }

                    else if (opt == "4") {
                        cout << "¿Tipo de insercion?\n";
                        cout << "1) Longitud variable\n";
                        cout << "2) Longitud fija (bitmap)\n";
                        cout << ">> ";
                        int opcion;
                        cin >> opcion;
                        cin.ignore(1, '\n');

                        if (adicionarTodoCSV("D:\\DBPlusPlus\\DBPlusPlus\\data\\usr\\db\\Housing.csv", "housing", opcion, miDisco))
                            cout << "Todos los registros del CSV agregados correctamente.\n";
                        else
                            cout << "No se pudieron agregar todos los registros.\n";
                    }
                    else if (opt == "5") {
                        cout << "¿Tipo de eliminacion?\n";
                        cout << "1) Longitud variable\n";
                        cout << "2) Longitud fija (bitmap)\n";
                        cout << ">> ";
                        int opcion;
                        cin >> opcion;
                        cin.ignore(1, '\n');

                        if (opcion == 2) {
                            if (eliminarRegistro("housing", 193, miDisco)) {
                                cout << "Eliminado correctamente\n";
                                cout << "Entrando a Eliminar Registro \n" << std::endl;
                            }
                        }
                        if (opcion == 1) {
                            bool ok = eliminar_registro_variable(1, 2, miDisco.getTamBloque(), miDisco);
                            assert(ok);
                        }
                        else {
                            cout << "No se pudo eliminar el registro \n";
                        }
                    }

                    else if (opt == "6") {
                        cout << "Tipo de Modificacion?\n";
                        cout << "1) Longitud variable\n";
                        cout << "2) Longitud fija\n";
                        cout << ">> ";
                        int n; cin >> n;

                        const char* nuevoReg = "1790000#9#3#1#2#yes#no#no#no#no#0#no#semi-furnished";
                        if (n == 2) {
                            if (modificarRegistro("housing", 4, nuevoReg, miDisco)) {
                                cout << "Modificado correctamente\n";
                            }
                        }
                        else if (n == 1) {
                            if (modify_registro_variable(1, 1, "housing", "furnishingstatus", "semi-furnished", miDisco)) {
                                cout << "Modificado correctamente_var\n";
                            }
                        }
                    }

                    else if (opt == "7") { //demo
                        int n;
                        cin >> n;

                        if (!adicionarNRegistrosVariable(n, "data\\usr\\db\\titanic.txt", "titanic", miDisco)) {
                            std::cout << "Al menos un registro no se pudo insedsrtar en modo variable.\n";
                        }
                        else {
                            std::cout << "Los 5 registros se insertaron exitosamente en Bloque1.\n";
                        }

                    }

                    else if (opt == "8") {
                        string sel, from, where;
                        cout << "SELECT "; getline(cin, sel);
                        cout << "FROM ";   getline(cin, from);
                        cout << "WHERE ";  getline(cin, where);
                        agregarTablaCatalogo(basePath.c_str(), from.c_str());
                        ejecutar_query(sel.c_str(), from.c_str(), where.c_str(),
                            schemaPath.c_str(), basePath.c_str(),
                            miDisco, discoPath.c_str());
                        cout << "Tamaño de la tabla: " << relationSizeBytes(from.c_str(), basePath.c_str()) << " bytes\n";
                    }

                    else if (opt == "9") {
                        cout << "Nombre de la relacion: ";
                        string tabla; getline(cin, tabla);
                        miDisco.volcarRelacionASectores(tabla.c_str());
                    }

                    else if (opt == "11") {
                                            
                        int numero;
                        std::cout << "Ingrese el número de página: ";
                        std::cin >> numero;
                        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                        std::string ruta = std::string(rutaBloque)
                            + "\Bloque" + std::to_string(numero) + ".txt";

                        try {
                            std::string contenido = leerArchivo(ruta);
                            std::cout << " " << ruta << "\n"
                                << contenido << "\n";
                        }
                        catch (const std::exception& e) {
                            std::cerr << "Error al abrir " << ruta << ": " << e.what() << "\n";
                        } 
                        std::cerr << "Error: " << "\n";
                        
                    }
                    else {
                        cout << "Opcion invalida\n";
                    }
                }
            }
        }
        else if (choice == 2) {
            // Menú Buffer
            if (!discoConfigDone) {
                cout << "Configura primero el disco.\n";
            }
            else {
                while (true) {
                    cout << "\n--- MENU BUFFER ---\n";
                    cout << "1) Piasdsadadsn pagina\n";
                    cout << "2) Unpin pagina\n";
                    cout << "3) Mostrar estado\n";
                    cout << "4) Flush all\n";
                    cout << "5) Mostrar stats\n";
                    cout << "6) Cambios en pagina\n";
                    cout << "7) Ver contenido pagina\n";
                    cout << "8) Volver al menu principal\n";
                    cout << "9) Mostrar\n";
                    cout << ">> ";
                    int opc; cin >> opc; cin.ignore();
                    if (opc == 8) break;
                    switch (opc) {
                    case 1: {
                        int pid; char op; bool pin;
                        cout << "ID: "; cin >> pid;
                        cout << "R/W: "; cin >> op;
                        cout << "Pin? "; cin >> pin;
                        cout << "Mensaje Previo" << endl;
                        auto p = pBufPool->pinPage(pid, op, pin);
                        cout << (p ? "OK" : "Fallo") << "\n";
                        cout << "Mensaje Siguiente" << endl;
                        break;
                    }
                    case 2: {
                        int pid; cout << "ID: "; cin >> pid;
                        pBufPool->unpinPage(pid);
                        break;
                    }
                    case 3: pBufPool->Status(); break;
                    case 4: 
                        
                        flushBufferToDisk(miDisco);
                        std::cout << "Todos los cambios del buffer han sido volcados a didasdsasco.\n";
                        break;

                    case 5: pBufPool->printStats(); break;
                    case 6: {
                        int pid;
                        cout << "ID de página para cambios: ";
                        cin >> pid;
                        // Cargamos en modo escritura (¿dirty?) y la mantenemos pineada
                        Page* base = pBufPool->pinPage(pid, 'W', true);
                        if (!base) {
                            cout << "Página " << pid << " no está en buffer.\n";
                            break;
                        }

                        std::cout << "[DEBUG] base apunta a un objeto de tipo "
                            << typeid(*base).name() << "\n";
                        auto page = dynamic_cast<PageWithRecords*>(base);
                        if (!page) {
                            cout << "ERRDOR: Esta página no soporta ops de registro.\n";
                            pBufPool->unpinPage(pid);
                            break;
                        }

                        // Mini-menú de operaciones
                        cout << "\n--- OPERACIONES EN PÁGINA " << pid << " ---\n";
                        cout << "1) Insertar\n";
                        cout << "2) Eliminar\n";
                        cout << "3) Modificar\n";
                        cout << ">> ";
                        int op; cin >> op; cin.ignore();

                        if (op == 1) {
                            cout << "Registro (# separados, termina '\\n'): ";
                            string reg = "1790000#4123#3#1#2#yes#nosdffdad#no#no#no#0#no#unfurnished\n";
                            if (page->insertFixed("housing", reg))
                                cout << "Insertado OK en memoria.\n";
                            else
                                cout << "Fallo al insertar.\n";
                        }
                        else if (op == 2) {
                            cout << "Posición global a eliminar: ";
                            int pos; cin >> pos;
                            if (page->deleteFixed("housing", pos))
                                cout << "Eliminado OK en memoria.\n";
                            else
                                cout << "Fallo al eliminar.\n";
                        }
                        else if (op == 3) {
                            cout << "Posición global a modificar: ";
                            int pos; cin >> pos; cin.ignore();
                            cout << "Nuevo registro (# separados, termina '\\n'): ";
                            string reg = "1790000#4#3#1#2#yes#nosdffdad#no#no#no#0#no#unfurnished";
                            if (page->modifyFixed("housing", pos, reg))
                                cout << "Modificado OK en memoria.\n";
                            else
                                cout << "Fallo al modificar.\n";
                        }
                        else {
                            cout << "Opción inválida\n";
                        }

                        // Despineamos; si quieres flush inmediato, úsalo tú mismo:
                        pBufPool->unpinPage(pid);
                        break;
                    }

                    case 7: {
                        int pid; std::cout << "ID de la pagina: "; std::cin >> pid;
                        Page* p = pBufPool->pinPage(pid, 'L', false);
                        if (p) {
                            auto pr = static_cast<PageWithRecords*>(p);
                            pr->viewContent();
                            pBufPool->unpinPage(pid);
                        }
                        else {
                            std::cout << "ERROR: falla al cargar pagina " << pid << std::endl;
                        }
                        break;
                    }
                    
                    case 9: {
                        std::string base = bufferPagePath;

                        int numero;
                        std::cout << "Ingrese el número de página: ";
                        std::cin >> numero;
                        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                        std::string ruta = base + "/Page" + std::to_string(numero) + ".txt";

                        try {
                            std::string contenido = leerArchivo(ruta);
                            std::cout << "--- Contenido de " << ruta << " ---\n"
                                << contenido << "\n";
                        }
                        catch (const std::exception& e) {
                            std::cerr << "Error al abrir " << ruta << ": " << e.what() << "\n";
                        }
                    }

                    default: cout << "Inválida\n";
                    }
                }
            }
        }
        else if (choice == 3) {
            break;
        }
        else {
            cout << "Opción inválida\n";
        }
    }

    // Liberar
    delete pBufPool;
    return 0;
}

