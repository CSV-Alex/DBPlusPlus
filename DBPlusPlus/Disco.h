#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>   // _mkdir en Windows
#include <direct.h>     // tambien para _mkdir
#include <iostream>
#include <fstream>

//#define MAX_BUF      512
#define MAX_STR_LEN 64
#define MAX_FIELDS 32
//#define MAX_PATH_LEN 256
#define MAX_SCHEMA  4096
#define MAX_LINE 1024

static const int MAX_PATH_LEN = 256;
static const int MAX_BUF = 512;
static const int MAX_SECTORES = 100;

class Disco {
private:
    int platos;
    int pistas;
    int sectores;
    long long tamSector;
    long long tamBloque;
    int nroSuperficies;
    long long capacidadTotal;
    long long capacidadLibre;
    int nroBloques;

    char rutaCatalogo[MAX_PATH_LEN];
    char rutaDirBloques[MAX_PATH_LEN];
    char bufferRuta[MAX_PATH_LEN];
    char bufferLectura[MAX_BUF];
    char rutaLongitudFija[MAX_PATH_LEN];
    char discoNuevoPath[MAX_PATH_LEN];

    static bool str_eq(const char* a, const char* b) {
        while (*a && *b) {
            if (*a != *b) return false;
            ++a; ++b;
        }
        return *a == *b;
    }
    
    /// Importante
    static void make_dir(const char* dirPath) {
        if (_mkdir(dirPath) != 0) {
            if (errno != EEXIST) {
                std::perror("Error al crear directorio");
            }
        }
    }
    
    bool leerLineaDirBloque(int lineaNum) {
        FILE* f = fopen(rutaDirBloques, "r");
        if (!f) return false;
        int contador = 0;
        while (fgets(bufferLectura, MAX_BUF, f)) {
            ++contador;
            if (contador == lineaNum) {
                // bufferLectura ya tiene la linea
                size_t L = strlen(bufferLectura);
                if (L > 0 && (bufferLectura[L - 1] == '\r' || bufferLectura[L - 1] == '\n'))
                    bufferLectura[L - 1] = '\0';
                fclose(f);
                return true;
            }
        }
        fclose(f);
        return false;
    }

    /// Importante
    int parsearSectoresBloque(char sectoresOut[][MAX_STR_LEN],
        int maxSectores,
        char* inputBuffer)
    {
        int count = 0;

        char* tempBuffer = (char*)malloc(MAX_BUF);
        if (!tempBuffer) return 0;
        strncpy(tempBuffer, inputBuffer, MAX_BUF);
        tempBuffer[MAX_BUF - 1] = '\0';

        char* token = strtok(tempBuffer, "#");
        while (token && count < maxSectores) {
            // si token coincide con "%d/%d/%d/%d"
            int p, s, pi, se;
            if (sscanf(token, "%d/%d/%d/%d", &p, &s, &pi, &se) == 4) {
                strncpy(sectoresOut[count], token, MAX_STR_LEN - 1);
                sectoresOut[count][MAX_STR_LEN - 1] = '\0';
                ++count;
            }
            token = strtok(NULL, "#");
        }

        free(tempBuffer);
        return count;
    }

    void rutaSectorDesdeCodigo(const char* codigoSector) {
        int p, s, pi, se;
        if (sscanf(codigoSector, "%d/%d/%d/%d", &p, &s, &pi, &se) == 4) {
            snprintf(bufferRuta, MAX_PATH_LEN,
                "DISCO\\Plato%d\\S%d\\Pista%d\\Sector%d.txt",
                p, s, pi, se);
        }
        else {
            bufferRuta[0] = '\0';
        }
    }

public:

    Disco(int _platos, int _pistas, int _sectores,
        long long _capSector, long long _capBloque)
    {
        platos = _platos;
        pistas = _pistas;
        sectores = _sectores;
        tamSector = _capSector;
        tamBloque = _capBloque;
        nroSuperficies = 2;

        capacidadTotal = (long long)platos * nroSuperficies * pistas * sectores * tamSector;
        capacidadLibre = capacidadTotal;
        nroBloques = (int)(capacidadTotal / tamBloque);

        strcpy(rutaCatalogo, "data\\usr\\db\\catalogo.txt");
        strcpy(rutaDirBloques, "DISCO\\dirBloques.txt");
        strcpy(rutaLongitudFija, "DISCO\\longitudFija.txt");
        strcpy(discoNuevoPath, "DISCO\\");

        make_dir("DISCO");
        make_dir("DISCO\\BLOQUES");

        crearEstructuraDisco();
        crearBloquesLogicos();
    }

    ~Disco() {
    }
    
    void crearEstructuraDisco() {
        char rutaTemp[MAX_PATH_LEN];

        for (int p = 1; p <= platos; ++p) {
            // DISCO/Plato<p>
            snprintf(rutaTemp, MAX_PATH_LEN, "DISCO\\Plato%d", p);
            make_dir(rutaTemp);

            for (int s = 1; s <= nroSuperficies; ++s) {
                // DISCO/Plato<p>/S<s>
                snprintf(rutaTemp, MAX_PATH_LEN, "DISCO\\Plato%d\\S%d", p, s);
                make_dir(rutaTemp);

                for (int pi = 1; pi <= pistas; ++pi) {
                    // DISCO/Plato<p>/S<s>/Pista<pi>
                    snprintf(rutaTemp, MAX_PATH_LEN, "DISCO\\Plato%d\\S%d\\Pista%d", p, s, pi);
                    make_dir(rutaTemp);

                    for (int se = 1; se <= sectores; ++se) {
                        // Archivo DISCO/Plato<p>/S<s>/Pista<pi>/Sector<se>.txt
                        snprintf(rutaTemp, MAX_PATH_LEN,
                            "DISCO\\Plato%d\\S%d\\Pista%d\\Sector%d.txt",
                            p, s, pi, se);
                        FILE* fsec = fopen(rutaTemp, "w");
                        if (fsec) fclose(fsec);
                    }
                }
            }
        }
    }

    void crearBloquesLogicos() {
        std::ofstream fdir("DISCO\\dirBloques.txt", std::ios::out);
        if (!fdir.is_open()) {
            std::perror("Error creando dirBloques.txt");
            return;
        }

        std::cout << " Creando disco... " << std::endl;

        _mkdir("DISCO\\BLOQUES");

        int sectoresPorBloque = (int)(tamBloque / tamSector);
        int curPista = 1;
        int curSector = 1;

        for (int i = 1; i <= nroBloques; ++i) {
            fdir << tamBloque << "#2#BLOQUE#" << i << "#" << tamBloque << "#_";

            int asignados = 0;
            while (asignados < sectoresPorBloque) {
                for (int p = 1; p <= platos && asignados < sectoresPorBloque; ++p) {
                    for (int s = 1; s <= 2 && asignados < sectoresPorBloque; ++s) {
                        fdir << tamSector << "#"
                            << p << "/"
                            << s << "/"
                            << curPista << "/"
                            << curSector << "#_";
                        ++asignados;
                    }
                }
                curSector++;
                if (curSector > sectores) {
                    curSector = 1;
                    curPista++;
                    if (curPista > pistas) {
                        curPista = 1;
                    }
                }
            }

            fdir << std::endl;

            char rutaBloque[MAX_PATH_LEN];
            std::snprintf(rutaBloque, sizeof(rutaBloque), "DISCO\\BLOQUES\\Bloque%d.txt", i);

        }

        fdir.close();
    }

    void printDisco() {
        std::cout << "\n=== Caracteristicas del Disco ===\n";
        std::cout << "  Platos: " << platos << "\n";
        std::cout << "  Superficies x plato: " << nroSuperficies << "\n";
        std::cout << "  Pistas por superficie: " << pistas << "\n";
        std::cout << "  Sectores por pista: " << sectores << "\n";
        std::cout << "  Tamaño de sector: " << tamSector << " bytes\n";
        std::cout << "  Bloques totales: " << nroBloques << "\n";
        std::cout << "  Tamaño de bloque: " << tamBloque << " bytes\n";

        // Calcular bloques por pista y plato
        int bloquesPorPista = (sectores * tamSector) / tamBloque;
        int bloquesPorPlato = nroSuperficies * pistas * bloquesPorPista;

        std::cout << "  Capacidad total: " << capacidadTotal << " bytes\n";
        std::cout << "  Espacio libre:   " << capacidadLibre << " bytes\n";
        std::cout << "  Bloques por pista: " << bloquesPorPista << "\n";
        std::cout << "  Bloques por plato: " << bloquesPorPlato << "\n";
        std::cout << "=================================\n\n";
    }

    void mostrarArbolDisco() {
        std::cout << "\n=== arbol de Creacion del Disco ===\n";
        for (int i = 0; i < platos; ++i) {
            std::cout << "Plato " << (i + 1) << "\n";
            for (int s = 0; s < nroSuperficies; ++s) {

                bool ultimaSuperficie = (s == nroSuperficies - 1);
                std::cout << (ultimaSuperficie ? "|_ " : "|- ")
                    << "Superficie " << (s + 1) << "\n";

                std::string sangria = ultimaSuperficie ? "   " : "|  ";

                std::cout << sangria << "|_ Pistas: " << pistas << "\n"
                    << sangria << "|_ Sectores por pista: " << sectores << "\n";
            }
            // Separador entre platos
            if (i != platos - 1) std::cout << "\n";
        }
        std::cout << "===================================\n\n";
    }

    int get_tam_bloque(){ //funcion para tamaño de disco
        return (int)tamBloque;
    }

    void volcarBloqueASectores(int bloqueN) {
        if (bloqueN < 1 || bloqueN > nroBloques) {
            std::cout << "> Bloque invalido: " << bloqueN << "\n";
            return;
        }

        if (!leerLineaDirBloque(bloqueN)) {
            std::cout << "> No se pudo leer dirBloques.txt en linea " << bloqueN << "\n";
            return;
        }

        char rutaBloque[MAX_PATH_LEN];
        snprintf(rutaBloque, sizeof(rutaBloque),
            "DISCO\\BLOQUES\\Bloque%d.txt", bloqueN);
        FILE* fbloc = fopen(rutaBloque, "rb");
        if (!fbloc) {
            std::perror("No se pudo abrir BloqueN.txt");
            return;
        }

        // Medir cuantos bytes hay en el bloque
        fseek(fbloc, 0, SEEK_END);
        long tamDatos = ftell(fbloc);
        fseek(fbloc, 0, SEEK_SET);

        int maxSect = (int)(tamBloque / tamSector);

        //[maxSect][MAX_STR_LEN]
        char (*sectoresFisicos)[MAX_STR_LEN] =
            (char (*)[MAX_STR_LEN])malloc(maxSect * MAX_STR_LEN);
        if (!sectoresFisicos) {
            std::cout << "> Error de memoria al reservar sectoresFisicos\n";
            fclose(fbloc);
            return;
        }

        // 6) Extraer p/s/pi/se
        int totSect = parsearSectoresBloque(
            sectoresFisicos,
            maxSect,
            bufferLectura
        );
        if (totSect <= 0) {
            std::cout << "> No se encontraron sectores asignados para el Bloque "
                << bloqueN << ".\n";
            free(sectoresFisicos);
            fclose(fbloc);
            return;
        }

        long bytesRestantes = tamDatos;
        int  sectorActual = 0;

        while (bytesRestantes > 0 && sectorActual < totSect) {
            // Construir ruta fisica del sector p/s/pi/se
            rutaSectorDesdeCodigo(sectoresFisicos[sectorActual]);
            std::cout << "[DEBUG] Abriendo sector fisico para escritura: "
                << bufferRuta << std::endl;

            FILE* fsec = fopen(bufferRuta, "wb");
            if (!fsec) {
                std::perror("Error abriendo sector para escritura");
                break;
            }

            // Leer hasta tamSector bytes de fbloc
            long bytesAEscribir = (bytesRestantes > tamSector)
                ? tamSector
                : bytesRestantes;
            char* bufferSector = (char*)malloc(bytesAEscribir);
            if (!bufferSector) {
                std::cout << "> Error: no alcanzo memoria para bufferSector\n";
                fclose(fsec);
                break;
            }

            size_t leidos = fread(bufferSector, 1, bytesAEscribir, fbloc);
            if (leidos > 0) {
                fwrite(bufferSector, 1, leidos, fsec);
            }

            free(bufferSector);
            fclose(fsec);

            bytesRestantes -= leidos;
            ++sectorActual;
        }

        free(sectoresFisicos);
        fclose(fbloc);

        std::cout << "> Bloque " << bloqueN
            << " volcado a " << sectorActual
            << " sectores.\n";
    }

    void volcarRelacionASectores(const char* relacion) {
        // buscar todas las lineas cuyo nombre de tabla coincida
        FILE* fcat = fopen(rutaCatalogo, "r");
        if (!fcat) {
            std::perror("No se puede abrir catalogo.txt");
            return;
        }

        char linea[MAX_BUF];
        while (fgets(linea, MAX_BUF, fcat)) {
            // quitar CR/LF final
            size_t L = strlen(linea);
            while (L > 0 && (linea[L - 1] == '\r' || linea[L - 1] == '\n')) {
                linea[--L] = '\0';
            }

            // separar nombreTabla y rutaBloque
            char* sep = strchr(linea, '|');
            if (!sep) continue;
            *sep = '\0';
            const char* nombreTabla = linea;
            const char* rutaBloque = sep + 1;

            if (str_eq(nombreTabla, relacion)) {
                // Extraer
                const char* p = strrchr(rutaBloque, '\\');
                if (!p) p = strrchr(rutaBloque, '/');
                p = (p ? p + 1 : rutaBloque);

                int nro = 0;
                if (sscanf(p, "Bloque%d.txt", &nro) == 1) {
                    volcarBloqueASectores(nro);
                }
            }
        }

        fclose(fcat);
    }

    // Convierte una cadena a entero de forma segura.
    // Retorna 0 si la cadena no es un número válido.
    /// Importante
    static int safe_atoi(const char* str) {
        if (!str) return 0;
        // Saltar espacios iniciales
        while (*str == ' ' || *str == '\t') ++str;

        int sign = 1;
        if (*str == '-') {
            sign = -1;
            ++str;
        }
        else if (*str == '+') {
            ++str;
        }

        int result = 0;
        bool found_digit = false;
        while (*str) {
            if (*str >= '0' && *str <= '9') {
                found_digit = true;
                result = result * 10 + (*str - '0');
            }
            else {
                // Si encuentra un caracter no numérico, termina
                break;
            }
            ++str;
        }
        return found_digit ? sign * result : 0;
    }

    /// Importante
    bool isBlockAllowed(const char* nombreRelacion, int nroBloque) {
        char rutaCatalogo[MAX_PATH_LEN];
        snprintf(rutaCatalogo, sizeof(rutaCatalogo),
            "%s%s", discoNuevoPath, "catalogo.txt");
        FILE* fcat = fopen(rutaCatalogo, "r");
        if (!fcat) {
            return true;
        }

        char linea[MAX_BUF];
        while (fgets(linea, MAX_BUF, fcat)) {
            linea[strcspn(linea, "\r\n")] = '\0';
            char* sep = strchr(linea, '|');
            if (!sep) continue;
            *sep = '\0';
            const char* rel = linea;
            const char* path = sep + 1;
            char bloquePath[MAX_PATH_LEN];
            snprintf(bloquePath, sizeof(bloquePath),
                "%sBLOQUES\\Bloque%d.txt",
                discoNuevoPath, nroBloque);
            if (strcmp(path, bloquePath) == 0) {
                fclose(fcat);
                return (strcmp(rel, nombreRelacion) == 0);
            }
        }
        fclose(fcat);
        return true;
    }

    /// Importante
    bool validarCampos(const char* registro, int numFields, int* maxLenArr) {
        char copy[MAX_BUF];
        strncpy(copy, registro, MAX_BUF);
        copy[MAX_BUF - 1] = '\0';

        char* tok = strtok(copy, "#");
        int idx = 0;
        while (tok && idx < numFields) {
            if ((int)strlen(tok) > maxLenArr[idx]) return false;
            idx++;
            tok = strtok(NULL, "#");
        }
        return (idx == numFields);
    }

    void calcularLongitudFija(const char* rutaTXT) {
        FILE* ftxt = fopen(rutaTXT, "r");
        if (!ftxt) {
            perror("No se puede abrir el archivo para calcular longitudes fijas");
            return;
        }

        char linea[MAX_BUF];
        // Paso 1: leer encabezado para contar campos, pero NO usarlo para maxLen
        if (!fgets(linea, MAX_BUF, ftxt)) {
            fclose(ftxt);
            return;
        }
        // Eliminar CRLF del encabezado
        linea[strcspn(linea, "\r\n")] = '\0';

        // Contar numFields = (número de ‘#’) + 1
        int numFields = 1;
        for (char* p = linea; *p; ++p) {
            if (*p == '#') numFields++;
        }
        if (numFields < 1) numFields = 1;
        if (numFields > MAX_FIELDS) numFields = MAX_FIELDS;

        // Paso 2: inicializar maxLen[i] = 0
        int maxLen[MAX_FIELDS] = { 0 };

        // Paso 3: procesar cada línea de datos (las que quedan después del encabezado)
        while (fgets(linea, MAX_BUF, ftxt)) {
            // Quitar CRLF
            linea[strcspn(linea, "\r\n")] = '\0';

            // Separar por '#'
            char copy[MAX_BUF];
            strncpy(copy, linea, MAX_BUF);
            copy[MAX_BUF - 1] = '\0';

            char* tok = strtok(copy, "#");
            int idx = 0;
            while (tok && idx < numFields) {
                int len = (int)strlen(tok);
                if (len > maxLen[idx]) {
                    maxLen[idx] = len;
                }
                idx++;
                tok = strtok(NULL, "#");
            }
            // Si hay menos campos de los esperados, los omitimos; si hay más campos,
            // los ignoramos porque solo consideramos numFields = “número de '#'+1” del encabezado.
        }
        fclose(ftxt);

        // Paso 4: extraer nombre de relación (sin ruta ni “.txt”)
        const char* slash = strrchr(rutaTXT, '/');
        const char* backslash = strrchr(rutaTXT, '\\');
        const char* fname = slash ? slash + 1 : (backslash ? backslash + 1 : rutaTXT);
        char nombreRel[MAX_STR_LEN];
        strncpy(nombreRel, fname, MAX_STR_LEN - 1);
        nombreRel[MAX_STR_LEN - 1] = '\0';
        // Quitar extensión (por ejemplo ".txt" o ".csv")
        char* ext = strrchr(nombreRel, '.');
        if (ext) *ext = '\0';

        // Abrir (o crear) longitudfija.txt y agregar la línea
        FILE* flog = fopen(rutaLongitudFija, "a");
        if (!flog) {
            perror("No se puede abrir longitudfija.txt");
            return;
        }
        // Formato: <nombre_relación>|<numFields>#<maxLen1>#<maxLen2>#...#<maxLenN>\n
        fprintf(flog, "%s|%d", nombreRel, numFields);
        for (int i = 0; i < numFields; i++) {
            fprintf(flog, "#%d", maxLen[i]);
        }
        fprintf(flog, "\n");
        fclose(flog);
    }


    /// Buscar forma de reemplazarlo por obtenerTamañoRegistro
    void obtenerRegistroSize(const char* relacion, int* outRegistroSize) {
        FILE* flog = fopen(rutaLongitudFija, "r");
        if (!flog) {
            perror("No se puede abrir longitudfija.txt para lectura");
            *outRegistroSize = 0;
            return;
        }

        char linea[MAX_BUF];
        *outRegistroSize = 0;

        while (fgets(linea, MAX_BUF, flog)) {
            linea[strcspn(linea, "\r\n")] = '\0';
            // Verificar si la línea comienza con "<relacion>|"
            size_t relLen = strlen(relacion);
            if (strncmp(linea, relacion, relLen) == 0 && linea[relLen] == '|') {
                // Formato: "<relacion>|<numFields>#<len1>#<len2>#...#<lenN>"
                char* p = linea + relLen + 1; // justo después del '|'

                // 1) Leer numFields
                int numFields = atoi(p);
                // 2) Avanzar hasta el primer '#'
                while (*p && *p != '#') p++;
                if (*p == '#') p++;

                // 3) Acumular longitudes máximas de cada campo
                int sumaLong = 0;
                for (int i = 0; i < numFields; i++) {
                    int campoLen = atoi(p);
                    sumaLong += campoLen;
                    // Avanzar al siguiente '#'
                    while (*p && *p != '#') p++;
                    if (*p == '#') p++;
                }

                // 4) Cada campo va separado en un registro por un '#', 
                //    lo que suma (numFields - 1) bytes adicionales.
                int separadores = (numFields > 1 ? numFields - 1 : 0);
                // 5) Cada registro se guarda terminado en '|' → +1 byte
                int trailingBar = 1;

                *outRegistroSize = sumaLong + separadores + trailingBar;
                fclose(flog);
                return;
            }
        }

        // Si llegamos aquí, no encontramos la relación
        fclose(flog);
        *outRegistroSize = 0;
    }

    /// Observar
    void obtenerLongitudesPorCampo(const char* relacion, int* numFields, int* maxLenArr) {
        FILE* flog = fopen(rutaLongitudFija, "r");
        if (!flog) {
            perror("No se puede abrir longitudfija.txt para lectura");
            *numFields = 0;
            return;
        }

        char linea[MAX_BUF];
        *numFields = 0;

        while (fgets(linea, MAX_BUF, flog)) {
            // Eliminar CRLF
            linea[strcspn(linea, "\r\n")] = '\0';

            // Verificar si la línea comienza con "<relacion>|"
            size_t relLen = strlen(relacion);
            if (strncmp(linea, relacion, relLen) == 0 && linea[relLen] == '|') {
                // Avanzar justo después de 'relacion|'
                char* p = linea + relLen + 1;

                // Leer numFields (hasta el primer '#')
                int nf = atoi(p);
                if (nf < 1) {
                    fclose(flog);
                    *numFields = 0;
                    return;
                }
                if (nf > MAX_FIELDS) nf = MAX_FIELDS;
                *numFields = nf;

                // Avanzar al primer '#' que sigue a numFields
                while (*p && *p != '#') p++;
                if (*p == '#') p++;

                // Ahora, extraer los nf valores sucesivos
                for (int i = 0; i < *numFields; i++) {
                    if (!*p) {
                        maxLenArr[i] = 0;
                    }
                    else {
                        maxLenArr[i] = atoi(p);
                        // Avanzar al siguiente '#'
                        while (*p && *p != '#') p++;
                        if (*p == '#') p++;
                    }
                }

                fclose(flog);
                return;
            }
        }

        // Si llegamos aquí, no encontramos la relación
        fclose(flog);
        *numFields = 0;
    }

    bool eliminarRegistro(const char* nombreRelacion, int lineaObjetivo) {
        if (lineaObjetivo <= 0) return false;

        // --- 1) Abrir el archivo de bloque (aquí, Bloque1.txt) en modo "r+" ---
        const char rutaBloque[] = "DISCO/BLOQUES/Bloque1.txt";
        FILE* archivo = fopen(rutaBloque, "r+");
        if (!archivo) {
            return false;
        }

        //------------------------------------------------------------
        // (A)  Leer y actualizar la cabecera interna del bloque (bitmap)
        //------------------------------------------------------------
        // Leer la primera “línea” completa (puede incluir datos extras si no había CRLF)
        char buffer[MAX_LINE];
        if (!fgets(buffer, sizeof(buffer), archivo)) {
            fclose(archivo);
            return false;
        }
        // Quitar CR/LF al final
        size_t lenBmp = strlen(buffer);
        while (lenBmp > 0 && (buffer[lenBmp - 1] == '\n' || buffer[lenBmp - 1] == '\r')) {
            buffer[--lenBmp] = '\0';
        }

        // Buscar el slash '/' que marca el fin del bitmap
        char* slash = strchr(buffer, '/');
        if (!slash) {
            fclose(archivo);
            return false;
        }
        // headerBitsLen = (posSlash - inicio) + 1  → incluye el '/'
        size_t headerBitsLen = (slash - buffer) + 1;

        // Contar cuántas '1' hay hasta la líneaObjetivo, y ubicar el índice del bit
        int contador1s = 0;
        int bitIndex = -1;
        for (size_t i = 0; i + 1 < headerBitsLen; i++) {
            if (buffer[i] == '1') {
                contador1s++;
                if (contador1s == lineaObjetivo) {
                    bitIndex = (int)i;
                    break;
                }
            }
        }
        if (bitIndex < 0) {
            // No había suficientes '1' en el bitmap
            fclose(archivo);
            return false;
        }

        // Marcar esa posición a '0'
        buffer[bitIndex] = '0';

        // Ahora reescribimos los primeros headerBitsLen bytes + "\r\n"
        fseek(archivo, 0, SEEK_SET);
        fwrite(buffer, 1, headerBitsLen, archivo);
        fputc('\r', archivo);
        fputc('\n', archivo);
        fflush(archivo);

        // raw_header_len = headerBitsLen + 2 (CRLF)
        size_t raw_header_len = headerBitsLen + 2;

        //------------------------------------------------------------
        // (B)  Detectar cuánto mide el registro “viejo” para liberarlo internamente
        //------------------------------------------------------------
        // Posicionarse justo después de la cabecera física
        fseek(archivo, (long)raw_header_len, SEEK_SET);

        // Contar hasta lineaObjetivo-1 separadores '|' para hallar inicio del registro
        long inicioReg = ftell(archivo);
        int contSeparadores = 0;
        int c;
        while ((c = fgetc(archivo)) != EOF) {
            if (c == '|') {
                contSeparadores++;
                if (contSeparadores == lineaObjetivo - 1) {
                    inicioReg = ftell(archivo);
                    break;
                }
            }
        }
        if (contSeparadores < lineaObjetivo - 1) {
            fclose(archivo);
            return false;
        }

        // Ahora buscar el '|' que cierra el registro viejo
        long finReg = inicioReg;
        while ((c = fgetc(archivo)) != EOF) {
            finReg = ftell(archivo);
            if (c == '|') {
                break;
            }
        }
        if (finReg <= inicioReg) {
            fclose(archivo);
            return false;
        }

        // oldLen = cuántos bytes ocupa el registro (incluyendo todos sus campos, sin el '|')
        long oldLen = finReg - inicioReg;

        // Sobrescribir oldLen bytes con '#' (invalida el registro)
        fseek(archivo, inicioReg, SEEK_SET);
        for (long i = 0; i < oldLen; i++) {
            fputc('#', archivo);
        }
        fflush(archivo);
        fclose(archivo);

        //------------------------------------------------------------
        // (C)  ACTUALIZAR dirBloques.txt: sumar registroSize a bloque y a sector
        //------------------------------------------------------------
        // 1) Abrir dirBloques.txt en “r+” para lectura/modificación in-place
        FILE* fdir = fopen(rutaDirBloques, "r+");
        if (!fdir) {
            return false;
        }

        char linea[MAX_BUF];
        long  posLineaBloque = 0;
        int   nroBloque = 0;
        bool  foundBlock = false;

        // Leer línea por línea hasta dar con “BLOQUE#1” (nroBloque == 1)
        while (fgets(linea, MAX_BUF, fdir)) {
            // Guardar posición de inicio de esta línea (antes de leerla)
            posLineaBloque = ftell(fdir) - (long)strlen(linea);
            nroBloque++;

            // Quitar CR/LF al final para parsear
            size_t len_linea = strlen(linea);
            while (len_linea > 0 && (linea[len_linea - 1] == '\n' || linea[len_linea - 1] == '\r')) {
                linea[--len_linea] = '\0';
            }

            // Extraer el token “#BLOQUE#<nroBloque>#” para comparar
            // Primera parte = "<espLibreBloque>#2#BLOQUE#<nroBloque>#<tamBloque>#_…"
            // Podemos hacer strchr para encontrar “#BLOQUE#” y luego atoi(nroBloque).
            char* pb = strstr(linea, "#BLOQUE#");
            if (!pb) continue;
            // Avanzar “#BLOQUE#”
            pb += strlen("#BLOQUE#");
            // Leer el número de bloque de texto
            int bloqueoLeido = atoi(pb);
            if (bloqueoLeido != 1) continue; // aquí solo nos interesa Bloque1

            // Si llegamos acá, esta es la línea a modificar
            foundBlock = true;
            break;
        }

        if (!foundBlock) {
            fclose(fdir);
            return false;
        }

        // “línea” contiene la línea completa (sin CRLF) y posLineaBloque apunta a su inicio en el archivo.
        // Debemos:
        //   a) extraer espacioLibreBloque (primer token antes de '#'),
        //   b) sumarle oldLen,
        //   c) recorrer cada par “<espLibreSector>#<codSector>#_”,
        //      y, para el primer sector que corresponda a nuestro Bloque1, sumarle oldLen,
        //   d) reconstruir la línea con los nuevos espacios.

        // --- (C.1) Extraer espacioLibreBloque y tamBloque (aunque tamBloque se mantiene igual) ---
        char copia[MAX_BUF];
        strncpy(copia, linea, MAX_BUF - 1);
        copia[MAX_BUF - 1] = '\0';

        // Primer token = espacioLibreBloque
        char* tokEspBloq = strtok(copia, "#");
        if (!tokEspBloq) {
            fclose(fdir);
            return false;
        }
        int espacioLibreBloqueAntes = atoi(tokEspBloq);
        int espacioLibreBloqueNuevo = espacioLibreBloqueAntes + (int)oldLen;

        // Saltar “#2#BLOQUE#<nroBloque>#”
        // Ya avanzamos un token; ahora salteamos dos más (“2” y “BLOQUE”) y leemos <nroBloque> y <tamBloque>
        char* tok_2 = strtok(NULL, "#");        // “2”
        char* tok_BLOQUE = strtok(NULL, "#");   // “BLOQUE”
        char* tok_numBloque = strtok(NULL, "#");// “1”
        char* tok_tamBloque = strtok(NULL, "#");// “<tamBloque>”
        if (!tok_tamBloque) {
            fclose(fdir);
            return false;
        }
        int tamBloqLeido = atoi(tok_tamBloque);

        // (C.2) Ahora reconstruimos la cabecera hasta “#_”
        // Empezaremos poniendo “<espacioLibreBloqueNuevo>#2#BLOQUE#1#<tamBloque>#_”
        char nuevaLinea[MAX_BUF];
        int ofs = 0;
        ofs += snprintf(nuevaLinea + ofs, MAX_BUF - ofs,
            "%d#2#BLOQUE#%d#%d#_",
            espacioLibreBloqueNuevo,
            1,
            tamBloqLeido);

        // (C.3) Ahora recorremos la lista de sectores: cada par “<espLibreSector>#<codSector>#_”
        // Para el primer sector cuyo “<codSector>” empiece por “BLOQUE1” (o contenga ese identificador),
        // le sumamos oldLen. A los demás, los copiamos igual.
        //
        // Como no sabemos exactamente el formato de <codSector> (¿“1/1/1/3”?),
        // usaremos la heurística: el primer <codSector> que empiece por “1/” corresponde a Bloque1.
        // (Si su esquema real varía, basta ajustar esa comparación.)

        // Volvemos a apuntar a la parte “#_<lista sectores>” en la línea original:
        char* inicioSect = strstr(linea, "#_");
        if (!inicioSect) {
            fclose(fdir);
            return false;
        }
        // Avanzar dos caracteres para saltar “#_”
        inicioSect += 2;

        bool sectorActualizado = false;
        char* p = inicioSect;
        while (*p) {
            // Leer espacioLibreSector (cadena numérica hasta '#')
            char* p_iniEsp = p;
            while (*p && *p != '#') p++;
            if (!*p) break;
            *p = '\0';
            int espLibreSectorAntes = atoi(p_iniEsp);
            *p = '#';
            p++;

            // Leer codSector (cadena hasta siguiente '#')
            char* p_iniCod = p;
            while (*p && *p != '#') p++;
            if (!*p) break;
            *p = '\0';
            char codSector[MAX_STR_LEN];
            strncpy(codSector, p_iniCod, MAX_STR_LEN - 1);
            codSector[MAX_STR_LEN - 1] = '\0';
            *p = '#';
            p++;

            // Saber dónde termina este par: buscamos "#_" desde p
            char* nextPair = strstr(p, "#_");

            // Decidir si este es el sector que queremos “liberar”:
            //   (aquí, heurística: si codSector empieza con “1/”  → bloque 1)
            //   Tú puedes cambiar la condición a: strstr(codSector, "<tuIdentificador>") == codSector
            int espLibreSectorNuevo = espLibreSectorAntes;
            if (!sectorActualizado && strncmp(codSector, "1/", 2) == 0) {
                espLibreSectorNuevo += (int)oldLen;
                sectorActualizado = true;
            }

            // Agregar al buffer: “<espLibreSectorNuevo>#<codSector>#_”
            ofs += snprintf(nuevaLinea + ofs, MAX_BUF - ofs,
                "%d#%s#_",
                espLibreSectorNuevo,
                codSector);

            // Avanzar p a “nextPair + 2” o romper
            if (!nextPair) break;
            p = nextPair + 2;
        }

        // Si nunca encontramos un sector “1/…” (muy raro), dejamos la lista idéntica a como estaba.
        if (!sectorActualizado) {
            // Copiamos “\<espLibreSectorAntes>#<codSector>#_” tal cual de la línea original
            // para todos los pares; como ya tenemos “nuevaLinea” parcial, simplemente reescribimos todo lo que
            // quedaba en “linea” desde “#_<lista sectores completa>” hasta el final.
            char* todaListaSect = strstr(linea, "#_");
            if (todaListaSect) {
                ofs += snprintf(nuevaLinea + ofs, MAX_BUF - ofs,
                    "%s", todaListaSect + 2);
            }
        }

        // (C.4) Ahora “nuevaLinea” tiene la línea completa SIN CR/LF. Debemos imponernos
        // EXACTAMENTE el mismo número de bytes que ocupaba en disco (medido con fgets),
        // de modo que no “corramos” el resto del archivo. Para ello, medimos la longitud original:
        size_t len_original = strlen(linea);   // ya quitamos CR/LF antes, así que esto es la longitud sin CRLF
        // raw_len_total = len_original + 2 (CRLF)
        size_t raw_len_total = len_original + 2;

        // (C.5) Si “ofs” (longitud de parte útil en nuevaLinea) > raw_len_total-2, truncamos.
        //       En otro caso, rellenamos con espacios hasta raw_len_total-1, y al final ponemos '\n'.
        if ((size_t)ofs > raw_len_total - 2) {
            // Truncar y asegurar que en la penúltima posición quede '\r', última '\n'
            if (raw_len_total >= 2) {
                nuevaLinea[raw_len_total - 2] = '\r';
                nuevaLinea[raw_len_total - 1] = '\n';
            }
        }
        else {
            // Rellenar de espacios hasta raw_len_total-2
            size_t i;
            for (i = ofs; i < raw_len_total - 2; i++) {
                nuevaLinea[i] = ' ';
            }
            // Luego CRLF
            nuevaLinea[raw_len_total - 2] = '\r';
            nuevaLinea[raw_len_total - 1] = '\n';
        }

        // (C.6) Escribir EXACTAMENTE raw_len_total bytes en position posLineaBloque
        fseek(fdir, posLineaBloque, SEEK_SET);
        fwrite(nuevaLinea, 1, raw_len_total, fdir);
        fflush(fdir);
        fclose(fdir);

        return true;
    }

    //////////////////// insertar de forma fija

    /// #P1#Works#BeforeTheCorruption
    bool adicionarRegistroUnico(const char* registroTxt, const char* relacion) {
        // --- 0) Antes de abrir dirBloques, obtenemos el tamaño fijo del registro ---
        int registroSize;
        std::cout << "DEBUG0" << relacion << std::endl;
        std::cout << "DEBUG1" << registroTxt << std::endl;

        // Antes de llamar a obtenerRegistroSize:
        printf(">>> Leyendo %s para ver su contenido:\n", rutaLongitudFija);
        FILE* ftmp = fopen(rutaLongitudFija, "r");
        if (ftmp) {
            char buf[MAX_BUF];
            while (fgets(buf, MAX_BUF, ftmp)) {
                printf("   %s", buf);
            }
            fclose(ftmp);
        }
        else {
            printf("   ¡NO se pudo abrir %s!\n", rutaLongitudFija);
        }

        obtenerRegistroSize(relacion, &registroSize);
        std::cout << "DEBUG" << registroSize << std::endl;
        if (registroSize <= 0) {
            // No hay definición de longitud para esta relación
            fprintf(stderr, "No se encontró longitud fija para %s\n", relacion);
            return false;
        }

        // 1) Abrir dirBloques.txt para buscar bloque+sector libres.
        FILE* fdir = fopen(rutaDirBloques, "r+");
        if (!fdir) {
            perror("No se puede abrir dirBloques.txt");
            return false;
        }

        char linea[MAX_BUF];
        int  nroBloque = 0;
        bool foundBlock = false;
        char codSectorLibre[MAX_STR_LEN] = { 0 };
        long posLineaBloque = 0;

        //// Ajuste del tamaño del registro (incluimos un '|' final)  
        //int tamRegistro = (int)strlen(registroTxt) + 1;
        //// (sumamos +1 para el separador '|')

            // --- 2) En vez de usar strlen(registroTxt)+1, usamos registroSize: ---
        int tamRegistro = registroSize;

        // 2) Recorrer dirBloques.txt en busca de un bloque y sector con espacio
        int espacioLibreBloque = 0;
        int tamUtilAntes = 0;
        int espacioLibreSectorAntes = 0;

        while (1) {
            posLineaBloque = ftell(fdir);
            if (!fgets(linea, MAX_BUF, fdir)) break;
            nroBloque++;

            // Quitar CRLF si existe
            size_t len_linea = strlen(linea);
            if (len_linea > 0 && linea[len_linea - 1] == '\n')  linea[--len_linea] = '\0';
            if (len_linea > 0 && linea[len_linea - 1] == '\r')  linea[--len_linea] = '\0';

            // 2.1) Extraer espacioLibreBloque (primer token antes de '#')
            char copiaBloc[MAX_BUF];
            strncpy(copiaBloc, linea, MAX_BUF);
            copiaBloc[MAX_BUF - 1] = '\0';
            char* tokBloc = strtok(copiaBloc, "#");
            if (!tokBloc) continue;
            espacioLibreBloque = safe_atoi(tokBloc);
            tamUtilAntes = tamBloque - espacioLibreBloque;
            if (espacioLibreBloque < tamRegistro) {
                continue;
            }

            // 2.2) Leer la lista de sectores: buscar primer "#_"
            char* p = strstr(linea, "#_");
            if (!p) continue;
            // Para no modificar la original, trabajamos sobre copiaBloc
            strncpy(copiaBloc, linea, MAX_BUF);
            copiaBloc[MAX_BUF - 1] = '\0';
            p = strstr(copiaBloc, "#_");
            if (!p) continue;
            p += 2;

            // 2.3) Cada par "<espLibreSector>#<codSector>#_"
            while (*p) {
                char* inicioEspacioSector = p;
                while (*p && *p != '#') p++;
                if (*p != '#') break;
                *p = '\0';
                int espacioLibreSector = atoi(inicioEspacioSector);
                *p = '#';
                p++;

                char* inicioCodSector = p;
                while (*p && *p != '#') p++;
                if (*p != '#') break;
                *p = '\0';
                char sectorCode[MAX_STR_LEN];
                strncpy(sectorCode, inicioCodSector, MAX_STR_LEN - 1);
                sectorCode[MAX_STR_LEN - 1] = '\0';
                *p = '#';

                char* nextPair = strstr(p, "#_");
                if (espacioLibreSector < tamRegistro) {
                    if (!nextPair) break;
                    p = nextPair + 2;
                    continue;
                }

                // Encontré sector válido
                strncpy(codSectorLibre, sectorCode, MAX_STR_LEN - 1);
                espacioLibreSectorAntes = espacioLibreSector;
                foundBlock = true;
                break;
            }
            if (foundBlock) break;
        }

        if (!foundBlock) {
            fclose(fdir);
            return false;
        }

        int espacioLibreBloqueAntes = espacioLibreBloque;
        int tamUtilNuevo = tamUtilAntes + tamRegistro;
        int espacioBloqueNuevo = tamBloque - tamUtilNuevo;
        if (espacioBloqueNuevo < 0 || espacioBloqueNuevo > tamBloque) {
            // Debugging: valores fuera de rango
        }

        // 3) Volver a la posición de la línea en dirBloques.txt y releerla para calcular raw_len
        fseek(fdir, posLineaBloque, SEEK_SET);
        if (!fgets(linea, MAX_BUF, fdir)) {
            fclose(fdir);
            return false;
        }

        // Quitar '\n' para medir longitud real
        size_t len_linea = strlen(linea);
        if (len_linea > 0 && linea[len_linea - 1] == '\n') {
            len_linea--;
        }
        // raw_len_total = strlen(linea) en disco (incluye "\r\n" si existe)
        size_t raw_len_total = len_linea + 1;
        {
            // En Windows, fgets lee "\r\n". raw_len_total incluye CR y LF.
            // Para asegurarnos, si hay CR justo antes del '\n', contamos +2; si solo LF, +1.
            size_t lenNoCrLf = raw_len_total;
            if (lenNoCrLf > 0 && linea[lenNoCrLf - 1] == '\n') {
                lenNoCrLf--;
            }
            if (lenNoCrLf > 0 && linea[lenNoCrLf - 1] == '\r') {
                // Había CRLF
                raw_len_total = lenNoCrLf + 2;
            }
            else {
                // Solo LF (Unix)
                raw_len_total = lenNoCrLf + 1;
            }
        }

        // 4) Reconstruir la nueva línea de dirBloques.txt (espacio del bloque y lista de sectores):
        //     solo se modifica el "espacioLibreBloque" al inicio, restando tamRegistro
        fseek(fdir, posLineaBloque, SEEK_SET);
        // Leer de nuevo la línea completa, sin CRLF
        fgets(linea, MAX_BUF, fdir);
        linea[strcspn(linea, "\r\n")] = '\0';
        // Reconstrucción de la parte “<espBloqueNuevo>#2#BLOQUE#<nroBloque>#<tamBloque>#_…”
        char* inicioSect = strstr(linea, "#_");
        if (!inicioSect) {
            fclose(fdir);
            return false;
        }
        // Construyo temporalmente (sin finalizar CRLF)
        char bufferNueva[MAX_BUF] = { 0 };
        int ofsN = 0;
        ofsN += snprintf(bufferNueva + ofsN, MAX_BUF - ofsN,
            "%d#2#BLOQUE#%d#%d#_",
            espacioBloqueNuevo,
            nroBloque,
            tamBloque);
        // Copiar la lista de sectores (igual que antes), restando tamRegistro al sector usado:
        {
            char copia2[MAX_BUF];
            strncpy(copia2, linea, MAX_BUF - 1);
            copia2[MAX_BUF - 1] = '\0';
            char* p2 = strstr(copia2, "#_");
            if (p2) {
                p2 += 2;
                while (*p2) {
                    int espSec = atoi(p2);
                    while (*p2 && *p2 != '#') ++p2;
                    if (!*p2) break;
                    ++p2;
                    char sectorCode2[MAX_STR_LEN] = { 0 };
                    int pos2 = 0;
                    while (*p2 && *p2 != '#') {
                        sectorCode2[pos2++] = *p2++;
                    }
                    sectorCode2[pos2] = '\0';
                    int nuevoEsp = espSec;
                    if (strcmp(sectorCode2, codSectorLibre) == 0) {
                        nuevoEsp = espSec - tamRegistro;
                    }
                    ofsN += snprintf(bufferNueva + ofsN, MAX_BUF - ofsN,
                        "%d#%s#_",
                        nuevoEsp,
                        sectorCode2);
                    char* next2 = strstr(p2, "#_");
                    if (!next2) break;
                    p2 = next2 + 2;
                }
            }
        }

        // 5) Ajustar bufferNueva para que ocupe EXACTAMENTE raw_len_total bytes (con CRLF)
        //    Rellenar con espacios hasta raw_len_total-2, luego poner "\r\n".
        if (ofsN > (int)raw_len_total - 1) {
            // Truncar si excede longitud útil
            if (raw_len_total >= 1) {
                bufferNueva[raw_len_total - 1] = '\n';
            }
        }
        else {
            // Rellenar con espacios hasta raw_len_total - 1, luego '\n'
            for (int i = ofsN; i < (int)raw_len_total - 1; i++) {
                bufferNueva[i] = ' ';
            }
            bufferNueva[raw_len_total - 1] = '\n';
        }

        size_t len_original = strlen(linea);
        // 6) Sobreescribir EXACTAMENTE raw_len_total bytes
        fseek(fdir, posLineaBloque, SEEK_SET);
        fwrite(bufferNueva, 1, raw_len_total, fdir);
        fflush(fdir);
        fclose(fdir);

        // -------------------------------------------------------------
        // 7) Ahora actualizamos la cabecera de BloqueN.txt con el formato
        //    "<numRegistros>#<numMaxRegistros>#<bitmap>/"
        //    y luego escribimos el registro en contiguo usando '|'.
        // -------------------------------------------------------------
        {
            // 7.1) Ruta completa de BloqueN.txt
            char rutaBloque[MAX_PATH_LEN];
            snprintf(rutaBloque, sizeof(rutaBloque),
                "%sBLOQUES\\Bloque%d.txt",
                discoNuevoPath, nroBloque);

            // 7.2) Comprobar si BloqueN.txt existe y obtener su raw_header_len:
            FILE* fbloc = fopen(rutaBloque, "r+");
            size_t raw_header_len = 0;
            int    numRegAnt = 0;
            int    numMaxAnt = 0;
            char   bitmapAnt[MAX_BUF] = { 0 };

            if (!fbloc) {
                // El bloque no existe: lo creamos y calculamos la cabecera inicial.
                // >>> a) llamar a calcularCabeceraBloque
                char headerBuf[MAX_BUF];
                size_t headerLen;
                int numMax;
                calcularCabeceraBloque(tamBloque, registroSize,
                    headerBuf, &headerLen, &numMax);
                // headerBuf = "0#numMax#000...0/"   headerLen = longitud de esa cadena

                // >>> b) Abrir en "wb" para crear/truncar
                fbloc = fopen(rutaBloque, "wb");
                if (fbloc) {
                    fwrite(headerBuf, 1, headerLen, fbloc);
                    fflush(fbloc);
                    fseek(fbloc, -1, SEEK_END); // Move back one byte to overwrite the newline
                    raw_header_len = headerLen;
                }
                numRegAnt = 0;
                numMaxAnt = numMax;
                // bitmapAnt[] = todos '0' (ya viene en headerBuf pero lo guardamos aparte)
                for (int i = 0; i < numMax; i++) {
                    bitmapAnt[i] = '0';
                }
                bitmapAnt[numMax] = '\0';
            }
            else {
                // El bloque ya existe: leer la cabecera hasta el '/' para obtener raw_header_len,
                // numRegAnt, numMaxAnt y bitmapAnt[].
                size_t pos = 0;
                int    c;
                rewind(fbloc);
                // 7.2.1) Leer hasta topar con '/' (incluido) para medir raw_header_len
                while ((c = fgetc(fbloc)) != EOF) {
                    pos++;
                    if (c == '/') break;
                    if (pos >= MAX_BUF - 1) break;
                }
                raw_header_len = pos;  // bytes desde byte[0] hasta '/' incluido

                // 7.2.2) Ahora rebobinamos y leemos esa parte en un buffer temporal
                rewind(fbloc);
                char cabTmp[MAX_BUF];
                if (raw_header_len > MAX_BUF - 1) raw_header_len = MAX_BUF - 1;
                fread(cabTmp, 1, raw_header_len, fbloc);
                cabTmp[raw_header_len] = '\0';

                // 7.2.3) Parsear "numRegAnt#numMaxAnt#bitmapAnt/"
                //        -> basta con strtok sobre '#' y luego extraer bitmap
                //        El '/' indica el fin del bitmap.
                char* p1 = strchr(cabTmp, '#');
                if (!p1) { fclose(fbloc); return false; }
                *p1 = '\0';
                numRegAnt = safe_atoi(cabTmp);
                char* p2 = p1 + 1;
                char* p3 = strchr(p2, '#');
                if (!p3) { fclose(fbloc); return false; }
                *p3 = '\0';
                numMaxAnt = safe_atoi(p2);
                char* p4 = p3 + 1;
                // p4 apunta al primer carácter del bitmap; hasta llegar a '/'
                char* slash = strchr(p4, '/');
                if (!slash) { fclose(fbloc); return false; }
                size_t bmpLen = (size_t)(slash - p4);
                if (bmpLen >= MAX_BUF) bmpLen = MAX_BUF - 1;
                strncpy(bitmapAnt, p4, bmpLen);
                bitmapAnt[bmpLen] = '\0';

                // Dejamos fbloc abierto para reescribir la cabecera más abajo
            }

            // 7.3) Ahora tenemos:
            //     raw_header_len = nº bytes fijos de la cabecera
            //     numRegAnt      = cont actual de registros (0..numMaxAnt)
            //     numMaxAnt      = capacidad (bitmapAnt tiene length = numMaxAnt)
            //     bitmapAnt[]    = secuencia de '0'/'1' de longitud numMaxAnt

            // 7.4) Computar la posición donde colocaremos el nuevo registro en el bitmap:
            int idxLibre = -1;
            for (int i = 0; i < numMaxAnt; i++) {
                if (bitmapAnt[i] == '0') {
                    idxLibre = i;
                    break;
                }
            }
            if (idxLibre < 0) {
                // No hay espacio en el bloque (bitmap lleno)
                fclose(fbloc);
                return false;
            }

            // 7.5) Actualizar numReg y bitmapAnt
            numRegAnt++;
            bitmapAnt[idxLibre] = '1';

            // 7.6) Reconstruir la cabecera NUEVA de EXACTAMENTE raw_header_len bytes:
            //      Formato: "<numRegAnt>#<numMaxAnt>#<bitmapAnt>/"
            //      Rellenar con espacios si hiciera falta, para completar raw_header_len.
            char newHeader[MAX_BUF];
            int  ofh = 0;
            ofh += snprintf(newHeader + ofh, MAX_BUF - ofh, "%d#%d#", numRegAnt, numMaxAnt);
            for (int i = 0; i < numMaxAnt && ofh < (int)(raw_header_len - 1); i++) {
                newHeader[ofh++] = bitmapAnt[i];
            }
            newHeader[ofh++] = '/';
            // Si por alguna razón ofh < raw_header_len, rellenamos con espacios hasta raw_header_len:
            while (ofh < (int)raw_header_len) {
                newHeader[ofh++] = ' ';
            }
            // Convertimos a C-string (no nos interesa el '\0' para el bloque, 
            // pues por fwrite solo escribiremos raw_header_len bytes):
            newHeader[ofh] = '\0';

            // 7.7) Sobreescribir cabecera en fbloc
            rewind(fbloc);
            fwrite(newHeader, 1, raw_header_len, fbloc);
            fflush(fbloc);

            // 7.8) Ir al final del archivo y **hacer append del registro** + '|'
            fseek(fbloc, 0, SEEK_END);
            // Aquí va el nuevo código:
            size_t len = strlen(registroTxt);
            while (len > 0 && (registroTxt[len - 1] == '\n' || registroTxt[len - 1] == '\r')) {
                len--;
            }
            // Si registroTxt NO incluye un '|' al final, nosotros añadimos el '|':
            fwrite(registroTxt, 1, len, fbloc);
            fputc('|', fbloc);
            fflush(fbloc);

            fclose(fbloc);

            // --- NUEVO: Volcar el bloque a sectores ---
            this->volcarBloqueASectores(nroBloque);
        }

        // -------------------------------------------------
        // 8) POR ÚLTIMO, actualizar catalogo.txt (igual que antes):
        //    agregamos “relacion|rutaBloque\n”
        // -------------------------------------------------
        {
            char rutaCatalogo[MAX_PATH_LEN];
            snprintf(rutaCatalogo, sizeof(rutaCatalogo),
                "%s%s", discoNuevoPath, "catalogo.txt");
            FILE* fcat = fopen(rutaCatalogo, "a");
            if (fcat) {
                // La rutaBloque es “...\\BLOQUES\\Bloque<numero>.txt”
                char rutaBloque[MAX_PATH_LEN];
                snprintf(rutaBloque, sizeof(rutaBloque),
                    "%sBLOQUES\\Bloque%d.txt",
                    discoNuevoPath, nroBloque);
                fprintf(fcat, "%s|%s\n", relacion, rutaBloque);
                fclose(fcat);
            }
        }

        return true;
    }

    bool adicionarNRegistros(int n, const char* csvPath, const char* tabla, int opcion) {
        FILE* fcsv = fopen(csvPath, "r");
        if (!fcsv) {
            perror("No se puede abrir CSV para lectura");
            return false;
        }
        char linea[MAX_BUF];

        // 1) Saltar encabezado
        if (!fgets(linea, MAX_BUF, fcsv)) {
            fclose(fcsv);
            return false;
        }

        // 2) Para cada una de las proximas n lineas, transformar comas→# y llamar a adicionarRegistroUnico
        char registroTxt[MAX_BUF];
        for (int i = 0; i < n; ++i) {
            if (!fgets(linea, MAX_BUF, fcsv)) break;

            // Reemplazar cada coma por '#'
            int len = (int)strlen(linea);
            int pos = 0;
            for (int j = 0; j < len && pos + 1 < MAX_BUF; ++j) {
                if (linea[j] == ',') registroTxt[pos++] = '#';
                else                  registroTxt[pos++] = linea[j];
            }
            // Asegurar que termine en '\n'
            if (pos == 0 || registroTxt[pos - 1] != '\n') {
                if (pos + 1 < MAX_BUF) registroTxt[pos++] = '\n';
            }
            registroTxt[pos] = '\0';

            // 3) Insertar ese único registro según la opción
            bool ok = false;
            if (opcion == 1) {
                ok = this->adicionarRegistroUnicoBitmap(tabla, registroTxt);
            }
            else {
                ok = this->adicionarRegistroUnico(registroTxt, tabla);
            }
            if (!ok) {
                fclose(fcsv);
                return false;
            }
            // — Al llamar a adicionarRegistroUnico, se imprime toda la informacion de bloque/sector.
        }

        fclose(fcsv);
        return true;
    }

    bool adicionarTodoCSV(const char* csvPath, const char* tabla, int opcion) {
        FILE* fcsv = fopen(csvPath, "r");
        if (!fcsv) {
            perror("No se puede abrir CSV para lectura");
            return false;
        }
        char linea[MAX_BUF];

        // 1) Saltar encabezado
        if (!fgets(linea, MAX_BUF, fcsv)) {
            fclose(fcsv);
            return false;
        }

        // 2) Para cada linea restante, convertir comas→# y llamar a adicionarRegistroUnico
        char registroTxt[MAX_BUF];
        while (fgets(linea, MAX_BUF, fcsv)) {
            int len = (int)strlen(linea);
            int pos = 0;
            for (int j = 0; j < len && pos + 1 < MAX_BUF; ++j) {
                if (linea[j] == ',') registroTxt[pos++] = '#';
                else                  registroTxt[pos++] = linea[j];
            }
            // Asegurar que termine en '\n'
            if (pos == 0 || registroTxt[pos - 1] != '\n') {
                if (pos + 1 < MAX_BUF) registroTxt[pos++] = '\n';
            }
            registroTxt[pos] = '\0';

            bool ok = false;
            if (opcion == 1) {
                ok = this->adicionarRegistroUnicoBitmap(tabla, registroTxt);
            }
            else {
                ok = this->adicionarRegistroUnico(registroTxt, tabla);
            }
            if (!ok) {
                fclose(fcsv);
                return false;
            }
            // — Cada insercion imprime ubicacion y validaciones
        }

        fclose(fcsv);
        return true;
    }

    bool adicionarRegistroUnicoBitmap(const char* nombreRel, const char* registroTxt) {
        return false;
    }

    void printCapacidadesDetalle() {
        printf("=== Resumen de capacidades ===\n");
        printf("Capacidad total del disco: %lld bytes\n", capacidadTotal);
        printf("Capacidad libre del disco: %lld bytes\n", capacidadLibre);
        printf("Capacidad ocupada del disco: %lld bytes\n\n", capacidadTotal - capacidadLibre);

        FILE* fdir = fopen(rutaDirBloques, "r");
        if (!fdir) { perror("abrir dirBloques.txt"); return; }

        char linea[MAX_BUF];
        int bloqueN = 0;
        int bloquesConDatos = 0;
        long long ocupacionBloques = 0;
        long long ocupacionSectores = 0;

        while (fgets(linea, MAX_BUF, fdir)) {
            ++bloqueN;
            // 1) Extraer espacioLibreBloque y tamUtil
            char copia[MAX_BUF];
            strncpy(copia, linea, MAX_BUF);
            copia[MAX_BUF - 1] = '\0';
            char* tk = strtok(copia, "#"); // espacioLibreBloque
            int espacioLibreBloque = atoi(tk);
            int tamUtil = (int)(tamBloque - espacioLibreBloque);
            if (tamUtil > 0) {
                ++bloquesConDatos;
                ocupacionBloques += tamUtil;
            }

            // 2) Recorrer pares "<espacioLibreSector>#<p>/<s>/<pi>/<se>#_" para cada sector
            char* resto = strstr(linea, "#_");
            if (!resto) continue;
            char* ptr = resto + 2; // saltar "#_"
            while (*ptr) {
                int espacioLibreSector = atoi(ptr);
                while (*ptr && *ptr != '#') ++ptr;
                if (!*ptr) break;
                ++ptr;
                // Leer el codigo de sector
                int p, s, pi, se;
                if (sscanf(ptr, "%d/%d/%d/%d", &p, &s, &pi, &se) == 4) {
                    long long usadoSector = (long long)tamSector - espacioLibreSector;
                    if (usadoSector > 0) ocupacionSectores += usadoSector;
                }
                // Avanzar hasta siguiente "_"
                while (*ptr && *ptr != '_') ++ptr;
                if (*ptr == '_') ++ptr;
            }
        }
        fclose(fdir);

        printf("Numero de bloques con datos: %d\n", bloquesConDatos);
        printf("Capacidad usada total en bloques (suma tamUtil): %lld bytes\n", ocupacionBloques);
        printf("Capacidad usada total en sectores (suma de ocupacion sectores): %lld bytes\n", ocupacionSectores);
        printf("==============================\n\n");
    }

    void calcularCabeceraBloque(int tamBloque, int registroSize,
        char* bufferHeader,
        size_t* outHeaderLen,
        int* outNumMax)
    {
        // 1) Estimar inicialmente cuántos registros caben sin cabecera:
        int numMax = tamBloque / registroSize;
        if (numMax < 0) numMax = 0;

        // 2) Iterar hasta que numMax se estabilice al considerar el espacio de la cabecera misma:
        int numMaxPrev = -1;
        char tmpHeader[MAX_BUF];

        while (numMax != numMaxPrev) {
            numMaxPrev = numMax;

            // 2.1) Construir temporalmente la cabecera con el numMax actual:
            //      Formato (temporal): "0#<numMax>#<numMax de '0's>/"
            int ofs = 0;
            ofs += snprintf(tmpHeader + ofs, MAX_BUF - ofs, "0#%d#", numMax);

            // Llenar con exactamente numMax ceros ('0')
            for (int i = 0; i < numMax && ofs < MAX_BUF - 1; i++) {
                tmpHeader[ofs++] = '0';
            }

            // Terminar con '/' (en lugar de '\n')
            tmpHeader[ofs++] = '/';
            tmpHeader[ofs] = '\0';

            // 2.2) Calcular longitud de esa cabecera temporal:
            size_t headerLen = strlen(tmpHeader);

            // 2.3) Comprobar cuánto espacio queda para registros:
            if ((int)headerLen >= tamBloque) {
                // La cabecera ya consume todo el bloque → cero registros caben
                numMax = 0;
            }
            else {
                int espacioRestante = tamBloque - (int)headerLen;
                int posibleMax = espacioRestante / registroSize;
                if (posibleMax < 0) posibleMax = 0;
                numMax = posibleMax;
            }
            // Repetir hasta que numMax deje de cambiar.
        }

        // 3) Construir la cabecera definitiva en bufferHeader:
        int ofsFinal = 0;
        ofsFinal += snprintf(bufferHeader + ofsFinal, MAX_BUF - ofsFinal, "0#%d#", numMax);

        // Insertar bitmap de '0's (numMax veces)
        for (int i = 0; i < numMax && ofsFinal < MAX_BUF - 1; i++) {
            bufferHeader[ofsFinal++] = '0';
        }

        // Terminar con '/'
        bufferHeader[ofsFinal++] = '/';
        bufferHeader[ofsFinal] = '\0';

        // 4) Devolver resultados
        *outHeaderLen = ofsFinal;    // no contamos '\0', es exactamente la cantidad de bytes antes del '\0'
        *outNumMax = numMax;
    }

    /// Buscar forma de que se use esta funcion en lugar de obtenerRegistroSize
    int obtenerTamañoRegistro(const char* nombreRel) {
        FILE* f = fopen(rutaLongitudFija, "r");
        if (!f) return -1;

        char linea[MAX_BUF];
        int registroSize = -1;
        while (fgets(linea, MAX_BUF, f)) {
            linea[strcspn(linea, "\r\n")] = '\0';
            char prefijo[MAX_STR_LEN];
            snprintf(prefijo, MAX_STR_LEN, "%s|", nombreRel);
            if (strncmp(linea, prefijo, strlen(prefijo)) != 0) continue;

            char* p = strchr(linea, '|');
            if (!p) continue;
            p++;

            char* copy = _strdup(p);
            char* tok = strtok(copy, "#");
            if (!tok) { free(copy); break; }
            int numFields = atoi(tok);
            if (numFields <= 0) { free(copy); break; }

            int sumMax = 0;
            for (int i = 0; i < numFields; i++) {
                tok = strtok(NULL, "#");
                if (!tok) { sumMax = -1; break; }
                sumMax += atoi(tok);
            }
            free(copy);
            if (sumMax < 0) break;

            registroSize = sumMax + (numFields - 1) + 1;
            break;
        }
        fclose(f);
        return registroSize;
    }

};