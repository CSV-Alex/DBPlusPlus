#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <sys/stat.h>   // _mkdir en Windows
#include <direct.h>     // tambien para _mkdir
#include <iostream>
#include <fstream>

//#define MAX_BUF      512
#define MAX_STR_LEN 32
#define MAX_FIELDS 32
//#define MAX_PATH_LEN 256
#define MAX_SCHEMA  4096

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

    static bool str_eq(const char* a, const char* b) {
        while (*a && *b) {
            if (*a != *b) return false;
            ++a; ++b;
        }
        return *a == *b;
    }
    static bool str_ne(const char* a, const char* b) {
        return !str_eq(a, b);
    }

    static void make_dir(const char* dirPath) {
        if (_mkdir(dirPath) != 0) {
            if (errno != EEXIST) {
                std::perror("Error al crear directorio");
            }
        }
    }

    static int extraerNumBloque(const char* rutaBloqueSimple) {
        // Ruta simple: "Bloque123.txt"
        int n;
        if (sscanf(rutaBloqueSimple, "Bloque%d.txt", &n) == 1) {
            return n;
        }
        return 0;
    }

    static void rutaBloqueFisico(int bloqueN, char* outRuta) {
        sprintf(outRuta, "DISCO\\BLOQUES\\Bloque%d.txt", bloqueN);
    }

    static void rutaSectorFisico(const char* codigoSector, char* outRuta) {
        int p, s, pi, se;
        if (sscanf(codigoSector, "%d/%d/%d/%d", &p, &s, &pi, &se) == 4) {
            sprintf(outRuta,
                "DISCO\\Plato%d\\S%d\\Pista%d\\Sector%d.txt",
                p, s, pi, se);
        }
        else {
            outRuta[0] = '\0';
        }
    }

    static bool leerLineaDirBloque(const char* dirBloquesPath,
        int lineaNum,
        char* bufferLectura)
    {
        FILE* f = fopen(dirBloquesPath, "r");
        if (!f) return false;
        int contador = 0;
        while (fgets(bufferLectura, MAX_BUF, f)) {
            ++contador;
            if (contador == lineaNum) {
                // quitar CR/LF
                size_t L = strlen(bufferLectura);
                if (L > 0 && (bufferLectura[L - 1] == '\r' || bufferLectura[L - 1] == '\n')) {
                    bufferLectura[L - 1] = '\0';
                }
                fclose(f);
                return true;
            }
        }
        fclose(f);
        return false;
    }
    void construirRutaSector(int p, int s, int pi, int se) {
        snprintf(bufferRuta, MAX_PATH_LEN,
            "DISCO\\Plato%d\\S%d\\Pista%d\\Sector%d.txt",
            p, s, pi, se);
    }

    void construirRutaBloque(int bloqueN) {
        snprintf(bufferRuta, MAX_PATH_LEN,
            "DISCO/BLOQUES/Bloque%d.txt",
            bloqueN);
    }

    void construirRutaBloqueDesdeNombre(const char* basePath, const char* nombreBloque) {
        snprintf(bufferRuta, MAX_PATH_LEN, "%s%s", basePath, nombreBloque);
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

    Disco(int _platos = 2, int _pistas = 10, int _sectores = 5,
        long long _capSector = 30, long long _capBloque = 120)
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

            std::ofstream fbloc(rutaBloque, std::ios::out | std::ios::binary);
            if (!fbloc.is_open()) {
                std::perror("Error creando archivo BloqueN.txt");
            }
            else {
                fbloc << tamBloque << "#2#BLOQUE#" << i << "#" << tamBloque << "#_";
                fbloc << std::endl;
                fbloc.close();
            }
        }

        fdir.close();
    }

public:
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

    void adicionarRelacion(const char* discoPath, const char* fromTable, const char* basePath) {
        char rutaTabla[MAX_PATH_LEN];
        snprintf(rutaTabla, MAX_PATH_LEN, "%s%s.txt", basePath, fromTable);
        FILE* ftabla = fopen(rutaTabla, "rb");
        if (!ftabla) {
            perror("No se pudo abrir archivo de tabla");
            return;
        }

        // 2) Abrir catalogo.txt en basePath para añadir lineas “tabla|rutaBloque”
        char rutaCatalogo[MAX_PATH_LEN];
        snprintf(rutaCatalogo, MAX_PATH_LEN, "%s%s", basePath, "catalogo.txt");
        std::ofstream fcatw(rutaCatalogo, std::ios::out | std::ios::app);
        if (!fcatw.is_open()) {
            std::perror("No se puede abrir catalogo.txt para append");
            fclose(ftabla);
            return;
        }

        // 3) Reservar buffer para un bloque completo
        char* bufferBloque = (char*)malloc((size_t)tamBloque);
        if (!bufferBloque) {
            std::cout << "> Error: no hay memoria para bufferBloque\n";
            fclose(ftabla);
            fcatw.close();
            return;
        }

        // 4) Leer la cabecera (primera linea) para asegurarlo en el Bloque 1
        char lineaHeader[MAX_BUF];
        size_t headerLen = 0;
        if (fgets(lineaHeader, MAX_BUF, ftabla)) {
            headerLen = strlen(lineaHeader);
            if (headerLen > (size_t)tamBloque) headerLen = (size_t)tamBloque;
            memcpy(bufferBloque, lineaHeader, headerLen);
        }

        // 5) Leer el resto hasta llenar tamBloque
        size_t bytesDatos = 0;
        if (headerLen < (size_t)tamBloque) {
            bytesDatos = fread(bufferBloque + headerLen,
                1,
                (size_t)tamBloque - headerLen,
                ftabla);
        }
        size_t bytesEnEsteBloque = headerLen + bytesDatos;

        // 6) PROCESAR el primer bloque (cabeza + fragmento datos)
        if (bytesEnEsteBloque > 0) {
            // 6.1) Abrir dirBloques.txt en discoPath
            char rutaDirBloques[MAX_PATH_LEN];
            snprintf(rutaDirBloques, MAX_PATH_LEN, "%s%s", discoPath, "dirBloques.txt");
            FILE* fdir = fopen(rutaDirBloques, "r+");
            if (!fdir) {
                std::perror("No se puede abrir dirBloques.txt para asignar bloque");
                free(bufferBloque);
                fclose(ftabla);
                fcatw.close();
                return;
            }

            bool encontrado = false;
            int bloqueLibre = 0;
            long posInicioLinea = 0;
            char lineaDir[MAX_BUF];
            int contador = 0;

            while (fgets(lineaDir, MAX_BUF, fdir)) {
                ++contador;
                posInicioLinea = ftell(fdir) - (long)strlen(lineaDir);
                // Si no contiene "#ASIGNADO#", ese bloque esta libre
                if (strstr(lineaDir, "#ASIGNADO#") == NULL) {
                    bloqueLibre = contador;
                    encontrado = true;
                    break;
                }
            }
            if (!encontrado) {
                std::cout << "> No quedan bloques libres para asignar.\n";
                fclose(fdir);
                free(bufferBloque);
                fclose(ftabla);
                fcatw.close();
                return;
            }

            // 6.2) Escribir fragmento en “DISCO\\BLOQUES\\Bloque<bloqueLibre>.txt” dentro de discoPath
            char rutaBloque[MAX_PATH_LEN];
            snprintf(rutaBloque, sizeof(rutaBloque),
                "%sBLOQUES\\Bloque%d.txt",
                discoPath, bloqueLibre);
            std::ofstream fbloc(rutaBloque, std::ios::out | std::ios::binary);
            if (!fbloc.is_open()) {
                std::perror("Error creando archivo BloqueN.txt");
                fclose(fdir);
                free(bufferBloque);
                fclose(ftabla);
                fcatw.close();
                return;
            }
            fbloc.write(bufferBloque, (std::streamsize)bytesEnEsteBloque);
            fbloc.close();

            // 6.3) Actualizar linea en dirBloques.txt
            fseek(fdir, posInicioLinea, SEEK_SET);
            fgets(lineaDir, MAX_BUF, fdir);
            lineaDir[strcspn(lineaDir, "\r\n")] = '\0';

            char* ptrSectores = strstr(lineaDir, "#_");
            if (!ptrSectores) {
                std::cout << "> Formato invalido en dirBloques.txt linea " << bloqueLibre << "\n";
                fclose(fdir);
                free(bufferBloque);
                fclose(ftabla);
                fcatw.close();
                return;
            }
            char sectoresStr[MAX_BUF];
            strncpy(sectoresStr, ptrSectores + 2, MAX_BUF - 1);
            sectoresStr[MAX_BUF - 1] = '\0';

            long long espacioBloqueNuevo = (long long)tamBloque - (long long)bytesEnEsteBloque;
            char nuevaLinea[MAX_BUF];
            snprintf(nuevaLinea, MAX_BUF,
                "%lld#2#BLOQUE#%d#%lld#_ASIGNADO#%s\n",
                espacioBloqueNuevo,
                bloqueLibre,
                (long long)tamBloque,
                sectoresStr);

            fseek(fdir, posInicioLinea, SEEK_SET);
            fprintf(fdir, "%s", nuevaLinea);
            fclose(fdir);

            // 6.4) Agregar referencia en catalogo.txt: “tabla|rutaBloque”
            fcatw << fromTable << "|" << rutaBloque << "\n";

            // 6.5) Reducir espacio libre del disco
            capacidadLibre -= (long long)bytesEnEsteBloque;
        }

        // 7) PROCESAR el resto de bloques
        while (true) {
            size_t leidos = fread(bufferBloque, 1, (size_t)tamBloque, ftabla);
            if (leidos == 0) break;

            // 7.1) Abrir dirBloques.txt en discoPath
            char rutaDirBloques2[MAX_PATH_LEN];
            snprintf(rutaDirBloques2, MAX_PATH_LEN, "%s%s", discoPath, "dirBloques.txt");
            FILE* fdir2 = fopen(rutaDirBloques2, "r+");
            if (!fdir2) {
                std::perror("No se puede abrir dirBloques.txt para asignar bloque");
                break;
            }

            bool encontrado2 = false;
            int bloqueLibre2 = 0;
            long posInicioLinea2 = 0;
            char lineaDir2[MAX_BUF];
            int contador2 = 0;

            while (fgets(lineaDir2, MAX_BUF, fdir2)) {
                ++contador2;
                posInicioLinea2 = ftell(fdir2) - (long)strlen(lineaDir2);
                if (strstr(lineaDir2, "#ASIGNADO#") == NULL) {
                    bloqueLibre2 = contador2;
                    encontrado2 = true;
                    break;
                }
            }
            if (!encontrado2) {
                std::cout << "> No quedan bloques libres para asignar.\n";
                fclose(fdir2);
                break;
            }

            // 7.2) Escribir fragmento en “DISCO\\BLOQUES\\Bloque<bloqueLibre2>.txt”
            char rutaBloque2[MAX_PATH_LEN];
            snprintf(rutaBloque2, MAX_PATH_LEN,
                "%sBLOQUES\\Bloque%d.txt",
                discoPath, bloqueLibre2);
            FILE* fbloque2 = fopen(rutaBloque2, "wb");
            if (!fbloque2) {
                std::perror("Error abriendo bloque para escritura");
                fclose(fdir2);
                break;
            }
            fwrite(bufferBloque, 1, leidos, fbloque2);
            fclose(fbloque2);

            // 7.3) Actualizar metadatos en dirBloques.txt
            fseek(fdir2, posInicioLinea2, SEEK_SET);
            fgets(lineaDir2, MAX_BUF, fdir2);
            lineaDir2[strcspn(lineaDir2, "\r\n")] = '\0';

            char* ptrSectores2 = strstr(lineaDir2, "#_");
            if (!ptrSectores2) {
                std::cout << "> Formato invalido en dirBloques.txt linea " << bloqueLibre2 << "\n";
                fclose(fdir2);
                break;
            }
            char sectoresStr2[MAX_BUF];
            strncpy(sectoresStr2, ptrSectores2 + 2, MAX_BUF - 1);
            sectoresStr2[MAX_BUF - 1] = '\0';

            long long espacioBloqueNuevo2 = (long long)tamBloque - (long long)leidos;
            char nuevaLinea2[MAX_BUF];
            snprintf(nuevaLinea2, MAX_BUF,
                "%lld#2#BLOQUE#%d#%lld#_ASIGNADO#%s\n",
                espacioBloqueNuevo2,
                bloqueLibre2,
                (long long)tamBloque,
                sectoresStr2);

            fseek(fdir2, posInicioLinea2, SEEK_SET);
            fprintf(fdir2, "%s", nuevaLinea2);
            fclose(fdir2);

            // 7.4) Agregar en catalogo.txt
            fcatw << fromTable << "|" << rutaBloque2 << "\n";

            // 7.5) Ajustar espacio libre
            capacidadLibre -= (long long)leidos;
        }

        free(bufferBloque);
        fclose(ftabla);
        fcatw.close();
    }




    int obtenerBloqueDeRelacion(const char* relacion) {
        FILE* fcat = fopen(rutaCatalogo, "r");
        std::cout << rutaCatalogo << " " << relacion << std::endl;
        if (!fcat) return 0;

        char linea[MAX_BUF];
        while (fgets(linea, MAX_BUF, fcat)) {
            size_t L = strlen(linea);
            while (L > 0 && (linea[L - 1] == '\r' || linea[L - 1] == '\n')) {
                linea[--L] = '\0';
            }

            char* sep = strchr(linea, '|');
            if (!sep) continue;
            *sep = '\0';

            if (str_eq(linea, relacion)) {
                char* rutaBloque = sep + 1;

                // Eliminar \r o \n residuales en rutaBloque
                char* q = rutaBloque + strlen(rutaBloque) - 1;
                while (q >= rutaBloque && (*q == '\n' || *q == '\r')) {
                    *q-- = '\0';
                }

                char* p = strrchr(rutaBloque, '\\');
                if (!p) p = strrchr(rutaBloque, '/');
                p = (p) ? p + 1 : rutaBloque;

                int nro = 0;

                std::cout << "[DEBUG] ruta completa: " << rutaBloque << std::endl;
                std::cout << "[DEBUG] parte final: " << p << std::endl;

                if (sscanf(p, "Bloque%d.txt", &nro) == 1) {
                    std::cout << "[DEBUG] Bloque encontrado: " << nro << std::endl;
                    fclose(fcat);
                    return nro;
                }
                else {
                    std::cout << "[ERROR] sscanf no pudo extraer el numero de bloque desde: '" << p << "'" << std::endl;
                }
            }
        }

        fclose(fcat);
        return 0;
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


    void consultarBloque(int bloqueN) {
        if (bloqueN < 1 || bloqueN > nroBloques) {
            std::cout << "> Bloque invalido.\n";
            return;
        }
        snprintf(bufferRuta, MAX_PATH_LEN, "DISCO\\BLOQUES\\Bloque%d.txt", bloqueN);
        FILE* fbloc = fopen(bufferRuta, "rb");
        if (!fbloc) {
            std::perror("No se pudo abrir Bloque para consulta");
            return;
        }
        std::cout << "\n--- Contenido bruto de Bloque " << bloqueN << " (aprox "
            << tamBloque << " bytes) ---\n";

        int restante = (int)tamBloque;
        while (restante > 0) {
            int n = (restante < MAX_BUF ? restante : MAX_BUF);
            size_t le = fread(bufferLectura, 1, n, fbloc);
            if (le == 0) {
                break;
            }

            for (size_t i = 0; i < le; ++i) {
                char ch = bufferLectura[i];
                if (ch >= 32 && ch < 127) {
                    putchar(ch);
                }
                else {
                    putchar('.');
                }
            }
            restante -= (int)le;
        }
        putchar('\n');
        fclose(fbloc);
    }

    void consultarBloquePorRelacion(const char* relacion) {
        int blk = obtenerBloqueDeRelacion(relacion);
        if (blk == 0) {
            std::cout << "> Relacion \"" << relacion << "\" no esta en catalogo.\n";
        }
        else {
            consultarBloque(blk);
        }
    }

    void consultarRelacionBloques(const char* relacion) {
        FILE* fcat = fopen(rutaCatalogo, "r");
        if (!fcat) {
            std::perror("No se puede abrir catalogo.txt");
            return;
        }
        char linea[MAX_BUF];
        bool foundAny = false;
        while (fgets(linea, MAX_BUF, fcat)) {
            size_t L = strlen(linea);
            while (L > 0 && (linea[L - 1] == '\r' || linea[L - 1] == '\n')) {
                linea[--L] = '\0';
            }
            char* sep = strchr(linea, '|');
            if (!sep) continue;
            *sep = '\0';
            const char* nombreTabla = linea;
            const char* rutaBloque = sep + 1;
            if (str_eq(nombreTabla, relacion)) {
                // Extraer el numero de bloque de "BloqueN.txt"
                const char* p = strrchr(rutaBloque, '\\');
                if (!p) p = strrchr(rutaBloque, '/');
                p = (p ? p + 1 : rutaBloque);
                int nro = 0;
                if (sscanf(p, "Bloque%d.txt", &nro) == 1) {
                    std::cout << "\n>>> Contenido bruto de Bloque " << nro << ":\n";
                    consultarBloque(nro);
                    foundAny = true;
                }
            }
        }
        if (!foundAny) {
            std::cout << "> Relacion \"" << relacion << "\" no encontrada en catalogo.txt.\n";
        }
        fclose(fcat);
    }

    bool adicionarRegistroUnico(const char* registroTxt, const char* relacion) {
        // 1) Abrir dirBloques.txt en modo lectura/escritura
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
        int  tamRegistro = (int)strlen(registroTxt);
        if (registroTxt[tamRegistro - 1] != '\n') {
            // Asegurar que mida el '\n' si no esta presente
            tamRegistro++;
        }

        int  espacioLibreBloque = 0;
        int  tamUtilAntes = 0;
        int  espacioLibreSectorAntes = 0;

        // 2) Recorrer lineas de dirBloques.txt buscando bloque y sector con espacio
        while (fgets(linea, MAX_BUF, fdir)) {
            ++nroBloque;
            posLineaBloque = ftell(fdir) - (long)strlen(linea);

            // 2.1) Extraer espacioLibreBloque (primer token antes de '#')
            char copiaBloc[MAX_BUF];
            strncpy(copiaBloc, linea, MAX_BUF);
            copiaBloc[MAX_BUF - 1] = '\0';
            char* tokBloc = strtok(copiaBloc, "#");
            if (!tokBloc) continue;
            espacioLibreBloque = atoi(tokBloc);
            tamUtilAntes = tamBloque - espacioLibreBloque;
            if (espacioLibreBloque < tamRegistro) {
                printf("> Bloque %d sin espacio suficiente. Espacio libre bloque: %d bytes; Tamaño del registro: %d bytes\n",
                    nroBloque, espacioLibreBloque, tamRegistro);
                continue;
            }

            // 2.2) Encontrar la primera aparicion de "#_" (inicio de la lista de sectores)
            char* p = strstr(linea, "#_");
            if (!p) continue;
            p += 2; // avanzar justo despues de "#_"

            // 2.3) Recorremos cada par "<espacioLibreSector>#<p>/<s>/<pi>/<se>#_"
            while (*p) {
                // 2.3.1) Leer espacioLibreSector
                char* inicioEspacioSector = p;
                while (*p && *p != '#') p++;
                if (*p != '#') break;
                *p = '\0';
                int espacioLibreSector = atoi(inicioEspacioSector);
                *p = '#';
                p++; // avanzar al codigo del sector

                // 2.3.2) Extraer codigoSector hasta el siguiente '#'
                char* inicioCodSector = p;
                while (*p && *p != '#') p++;
                if (*p != '#') break;
                *p = '\0';
                char sectorCode[MAX_STR_LEN];
                strncpy(sectorCode, inicioCodSector, MAX_STR_LEN - 1);
                sectorCode[MAX_STR_LEN - 1] = '\0';
                *p = '#';

                // Avanzar al siguiente par
                char* nextPair = strstr(p, "#_");
                if (espacioLibreSector < tamRegistro) {
                    printf("> Bloque %d, Sector %s sin espacio. Espacio libre sector: %d bytes; Tamaño registro: %d bytes\n",
                        nroBloque, sectorCode, espacioLibreSector, tamRegistro);
                    if (!nextPair) break;
                    p = nextPair + 2;
                    continue;
                }

                // Sector adecuado encontrado
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

        // 3) Actualizar la linea en dirBloques.txt: restar tamRegistro de bloque y sector
        fseek(fdir, posLineaBloque, SEEK_SET);
        fgets(linea, MAX_BUF, fdir);
        linea[strcspn(linea, "\r\n")] = '\0';

        // 3.1) Reconstruir linea completa en un buffer nuevo, sin depender de la longitud anterior
        char* inicioSectores = strstr(linea, "#_");
        if (!inicioSectores) {
            fclose(fdir);
            return false;
        }

        char bufferLineaNueva[MAX_BUF];
        int ofs = 0;

        // 3.1.1) Escribir el nuevo espacioLibreBloque y campos fijos
        ofs += snprintf(bufferLineaNueva + ofs, MAX_BUF - ofs,
            "%d#2#BLOQUE#%d#%d#_",
            espacioBloqueNuevo,
            nroBloque,
            tamBloque
        );

        // 3.1.2) Ajustar cada par de sectores
        {
            char* psec2 = inicioSectores + 2; // justo despues de "#_"
            while (*psec2) {
                int espSec = atoi(psec2);
                while (*psec2 && *psec2 != '#') ++psec2;
                if (!*psec2) break;
                ++psec2;

                char sectorCode2[MAX_STR_LEN] = { 0 };
                int pos2 = 0;
                while (*psec2 && *psec2 != '#') {
                    sectorCode2[pos2++] = *psec2++;
                }
                sectorCode2[pos2] = '\0';

                int nuevoEspSec2 = espSec;
                if (strcmp(sectorCode2, codSectorLibre) == 0) {
                    nuevoEspSec2 = espSec - tamRegistro;
                }

                ofs += snprintf(bufferLineaNueva + ofs, MAX_BUF - ofs,
                    "%d#%s#_",
                    nuevoEspSec2,
                    sectorCode2
                );

                char* next2 = strstr(psec2, "#_");
                if (!next2) break;
                psec2 = next2 + 2;
            }
        }

        // 3.1.3) Agregar salto de linea
        if (ofs < MAX_BUF - 1) {
            bufferLineaNueva[ofs++] = '\n';
            bufferLineaNueva[ofs] = '\0';
        }
        else {
            bufferLineaNueva[MAX_BUF - 1] = '\n';
            bufferLineaNueva[MAX_BUF - 0] = '\0';
        }

        fseek(fdir, posLineaBloque, SEEK_SET);
        fputs(bufferLineaNueva, fdir);
        fclose(fdir);

        // 4) Actualizar cabecera de BloqueN.txt
        char rutaBloqueFis[MAX_PATH_LEN];
        rutaBloqueFisico(nroBloque, rutaBloqueFis);
        FILE* fbloc = fopen(rutaBloqueFis, "r+");
        if (!fbloc) {
            perror("No se pudo abrir BloqueN.txt para actualizacion");
            return false;
        }
        long posBlocLinea = ftell(fbloc);
        char lineaBloc[MAX_BUF];
        fgets(lineaBloc, MAX_BUF, fbloc);
        lineaBloc[strcspn(lineaBloc, "\r\n")] = '\0';

        // Extraer espacioLibreBloqueActual de la lineaBloque
        char copiaBloc2[MAX_BUF];
        strncpy(copiaBloc2, lineaBloc, MAX_BUF);
        copiaBloc2[MAX_BUF - 1] = '\0';
        char* tok2 = strtok(copiaBloc2, "#");
        int espacioLibreBloqueBloque = atoi(tok2);
        int tamUtilAntesBloc = tamBloque - espacioLibreBloqueBloque;
        int tamUtilNuevoBloc = tamUtilAntesBloc + tamRegistro;
        int espacioBloqueNuevoBloc = tamBloque - tamUtilNuevoBloc;

        char* inicioSBloc = strstr(lineaBloc, "#_");
        if (!inicioSBloc) {
            fclose(fbloc);
            return false;
        }
        // Ajustar lista de sectores en el bloque fisico
        char sectoresModBloc[MAX_BUF] = { 0 };
        char* psec2 = inicioSBloc + 2;
        while (*psec2) {
            int espSec = atoi(psec2);
            while (*psec2 && *psec2 != '#') ++psec2;
            if (!*psec2) break;
            ++psec2;

            char sectorCode2[MAX_STR_LEN] = { 0 };
            int pos2 = 0;
            while (*psec2 && *psec2 != '#') {
                sectorCode2[pos2++] = *psec2++;
            }
            sectorCode2[pos2] = '\0';

            int nuevoEspSec2 = espSec;
            if (strcmp(sectorCode2, codSectorLibre) == 0) {
                nuevoEspSec2 = espSec - tamRegistro;
            }

            char bufferPar2[64];
            snprintf(bufferPar2, sizeof(bufferPar2), "%d#%s#_", nuevoEspSec2, sectorCode2);
            strncat(sectoresModBloc, bufferPar2, sizeof(sectoresModBloc) - strlen(sectoresModBloc) - 1);

            char* next2 = strstr(psec2, "#_");
            if (!next2) break;
            psec2 = next2 + 2;
        }

        // reconstruir
        char nuevaLineaBloc[MAX_BUF];
        snprintf(nuevaLineaBloc, MAX_BUF,
            "%d#2#BLOQUE#%d#%d#_%s\n",
            espacioBloqueNuevoBloc,
            nroBloque,
            tamBloque,
            sectoresModBloc);

        fseek(fbloc, posBlocLinea, SEEK_SET);
        fprintf(fbloc, "%s", nuevaLineaBloc);
        fclose(fbloc);

        // escribir registro en sector fisico
        rutaSectorDesdeCodigo(codSectorLibre);
        FILE* fsec = fopen(bufferRuta, "a");
        if (!fsec) {
            perror("No se pudo abrir sector para escribir");
            return false;
        }
        int espacioLibreSectorDesp = espacioLibreSectorAntes - tamRegistro;

        int pl, su, pi, se;
        sscanf(codSectorLibre, "%d/%d/%d/%d", &pl, &su, &pi, &se);

        printf("-> Insertando registro en Plato %d, Superficie %d, Pista %d, Sector %d\n", pl, su, pi, se);
        printf("   Espacio libre bloque antes: %d bytes; despues: %d bytes\n", espacioLibreBloqueAntes, espacioBloqueNuevo);
        printf("   Espacio libre sector antes: %d bytes; despues: %d bytes\n", espacioLibreSectorAntes, espacioLibreSectorDesp);

        fprintf(fsec, "%s", registroTxt);
        fclose(fsec);

        return true;
    }



    bool adicionarNRegistros(int n, const char* csvPath, const char* tabla) {
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

            // 3) Insertar ese unico registro
            bool ok = this->adicionarRegistroUnico(registroTxt, tabla);
            if (!ok) {
                // Si falla la insercion, abortar y cerrar
                fclose(fcsv);
                return false;
            }
            // — Al llamar a adicionarRegistroUnico, se imprime toda la informacion de bloque/sector.
        }

        fclose(fcsv);
        return true;
    }

    bool adicionarTodoCSV(const char* csvPath, const char* tabla) {
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

            bool ok = this->adicionarRegistroUnico(registroTxt, tabla);
            if (!ok) {
                fclose(fcsv);
                return false;
            }
            // — Cada insercion imprime ubicacion y validaciones
        }

        fclose(fcsv);
        return true;
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

};
