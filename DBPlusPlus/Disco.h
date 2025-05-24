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

    void construirRutaSector(int p, int s, int pi, int se) {
        snprintf(bufferRuta, MAX_PATH_LEN,
            "DISCO/Plato%d/S%d/Pista%d/Sector%d.txt",
            p, s, pi, se);
    }

    void construirRutaBloque(int bloqueN) {
        // bloqueN: 1..nroBloques
        snprintf(bufferRuta, MAX_PATH_LEN,
            "DISCO/BLOQUES/Bloque%d.txt",
            bloqueN);
    }

    void construirRutaBloqueDesdeNombre(const char* basePath, const char* nombreBloque) {
        // …\\db
        snprintf(bufferRuta, MAX_PATH_LEN, "%s%s", basePath, nombreBloque);
    }

    bool leerLineaDirBloque(int lineaNum) {
        FILE* f = fopen(rutaDirBloques, "r");
        if (!f) return false;
        int contador = 0;
        while (fgets(bufferLectura, MAX_BUF, f)) {
            ++contador;
            if (contador == lineaNum) {
                // bufferLectura ya tiene la linea (sin \n)
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

    int parsearSectoresBloque(char sectoresOut[][MAX_STR_LEN], int maxSectores) {
        int count = 0;
        char* copia = bufferLectura;
        char* token = strtok(copia, "#_");

        for (int i = 0; i < 4; i++) {
            if (!token) break;
            token = strtok(NULL, "#_");
        }

        while (token && count < maxSectores) {
            token = strtok(NULL, "#_");
            if (!token) {
                break;
            } 
            strncpy(sectoresOut[count], token, MAX_STR_LEN);
            sectoresOut[count][MAX_STR_LEN - 1] = '\0';
            ++count;
            token = strtok(NULL, "#_");
        }
        return count;
    }

    void rutaSectorDesdeCodigo(const char* codigoSector) {
        int p, s, pi, se;
        if (sscanf(codigoSector, "%d/%d/%d/%d", &p, &s, &pi, &se) == 4) {
            construirRutaSector(p, s, pi, se);
        }
        else {
            bufferRuta[0] = '\0';
        }
    }

public:

    Disco(int _platos = 2, int _pistas = 100, int _sectores = 20,
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

        strcpy(rutaCatalogo, "catalogo.txt");
        strcpy(rutaDirBloques, "dirBloques.txt");

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
        std::cout << "  Capacidad total: " << capacidadTotal << " bytes\n";
        std::cout << "  Espacio libre:   " << capacidadLibre << " bytes\n";
        std::cout << "=================================\n\n";
    }

    void adicionarRelacion(const char* basePath, const char* fromTable) {
        FILE* fcatr = fopen(rutaCatalogo, "r");
        std::cout << rutaCatalogo;
        char lineaCat[MAX_BUF];
        bool yaExiste = false;
        if (fcatr) {
            while (fgets(lineaCat, MAX_BUF, fcatr)) {
                // recortar CR/LF
                size_t L = strlen(lineaCat);
                if (L > 0 && (lineaCat[L - 1] == '\r' || lineaCat[L - 1] == '\n')) {
                    lineaCat[L - 1] = '\0';
                }
                // extraer nombreRel en “lineaCat” hasta '|'
                char* sep = strchr(lineaCat, '|');
                if (!sep) continue;
                *sep = '\0';
                if (str_eq(lineaCat, fromTable)) {
                    yaExiste = true;
                    break;
                }
            }
            fclose(fcatr);
        }

        if (yaExiste) {
            std::cout << "> Relacion ya registrada en catalogo.txt: " << fromTable << "\n";
            return;
        }

        FILE* fdir = fopen(rutaDirBloques, "r+");
        if (!fdir) {
            std::perror("No se puede abrir dirBloques.txt para asignar bloque");
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
            if (strstr(lineaDir, "#ASIGNADO#") == NULL) {
                bloqueLibre = contador;
                encontrado = true;
                break;
            }
        }
        if (!encontrado) {
            std::cout << "> No quedan bloques libres para asignar.\n";
            fclose(fdir);
            return;
        }

        {
            char cabeceraEsperada[MAX_BUF];
            int lenCab = snprintf(cabeceraEsperada, MAX_BUF, "%lld#2#BLOQUE#%d#%lld##_",tamBloque, bloqueLibre, tamBloque);
            fseek(fdir, posInicioLinea + lenCab, SEEK_SET);
            fprintf(fdir, "ASIGNADO#");
        }
        fclose(fdir);

        FILE* fcatw = fopen(rutaCatalogo, "a");
        if (!fcatw) {
            std::perror("No se puede abrir catalogo.txt para append");
            return;
        }
        snprintf(bufferRuta, MAX_PATH_LEN, "DISCO\\BLOQUES\\Bloque%d.txt", bloqueLibre);
        fprintf(fcatw, "%s|%s\n", fromTable, bufferRuta);
        fclose(fcatw);

        capacidadLibre -= tamBloque;

        std::cout << "> Relacion “" << fromTable << "” asignada a Bloque "
            << bloqueLibre << "\n";
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
        std::snprintf(rutaBloque, sizeof(rutaBloque), "DISCO\\BLOQUES\\Bloque%d.txt", bloqueN);
        FILE* fbloc = fopen(rutaBloque, "rb");
        if (!fbloc) {
            std::perror("No se pudo abrir archivo BloqueN.txt");
            return;
        }

        while (true) {
            int ch = fgetc(fbloc);
            if (ch == EOF || ch == '\n') break;
        }

        char lineaCopia[MAX_BUF];
        std::strncpy(lineaCopia, bufferLectura, MAX_BUF);
        lineaCopia[MAX_BUF - 1] = '\0';

        char* token = strtok(lineaCopia, "#");
        for (int i = 0; i < 4 && token; ++i) {
            token = strtok(NULL, "#");
        }
        if (token) {
            token = strtok(NULL, "#");
        }

        while (true) {
            if (!token) break;
            token = strtok(NULL, "#");
            if (!token) break;

            rutaSectorDesdeCodigo(token);
            if (bufferRuta[0] == '\0') {
                std::cout << "> Codigo de sector invalido: " << token << "\n";
                break;
            }

            FILE* fsec = fopen(bufferRuta, "wb");
            if (!fsec) {
                std::perror("Error abriendo archivo de sector para escribir");
                break;
            }

            long bytesPendientes = (long)tamSector;
            while (bytesPendientes > 0 && !feof(fbloc)) {
                int chunk = (bytesPendientes < MAX_BUF ? (int)bytesPendientes : MAX_BUF);
                size_t leidos = fread(bufferLectura, 1, (size_t)chunk, fbloc);
                if (leidos == 0) break;
                fwrite(bufferLectura, 1, leidos, fsec);
                bytesPendientes -= (long)leidos;
            }

            fclose(fsec);

            token = strtok(NULL, "#");
        }

        fclose(fbloc);
        std::cout << "> Bloque " << bloqueN << " volcado a sectores fisicos.\n";
    }

    /*
    void volcarBloqueASectoresUnused(int bloqueN) {
        int maxSect = (int)(tamBloque / tamSector) + 2;
        //char sectoresFisicos[maxSect][MAX_STR_LEN];
        int totSect = parsearSectoresBloque(sectoresFisicos, maxSect);

        snprintf(bufferRuta, MAX_PATH_LEN, "DISCO\\BLOQUES\\Bloque%d.txt", bloqueN);
        FILE* fbloc = fopen(bufferRuta, "r");
 
        while (fgetc(fbloc) != EOF) {
            if (bufferLectura[0] == '\n') break;
            bufferLectura[0] = '\0'; // nos vale solo para avanzar
            if (feof(fbloc)) break;
            if (bufferLectura[0] == '\n') break;
            if (bufferLectura[0] != '\n') ungetc(bufferLectura[0], fbloc);
            break;
        }

        for (int i = 0; i < totSect; ++i) {
            rutaSectorDesdeCodigo(sectoresFisicos[i]);
            FILE* fsec = fopen(bufferRuta, "wb");

            int toRead = (int)tamSector;
            char chunk[MAX_BUF];
            while (toRead > 0 && !feof(fbloc)) {
                int n = (toRead < MAX_BUF ? toRead : MAX_BUF);
                size_t le = fread(chunk, 1, n, fbloc);
                if (le == 0) break;
                fwrite(chunk, 1, le, fsec);
                toRead -= (int)le;
            }
            fclose(fsec);
        }

        fclose(fbloc);
        std::cout << "> Bloque " << bloqueN << " volcado a sus sectores fisicos.\n";
    }
    */

    int obtenerBloqueDeRelacion(const char* relacion) {
        FILE* fcat = fopen(rutaCatalogo, "r");
        if (!fcat) return 0;
        char linea[MAX_BUF];
        while (fgets(linea, MAX_BUF, fcat)) {
            // CR/LF
            size_t L = strlen(linea);
            if (L > 0 && (linea[L - 1] == '\r' || linea[L - 1] == '\n')) linea[L - 1] = '\0';
            
            char* sep = strchr(linea, '|');
            if (!sep) continue;
            *sep = '\0';
            if (str_eq(linea, relacion)) {
                char* rutaBloque = sep + 1;
                char* p = strrchr(rutaBloque, '\\');
                if (!p) p = strrchr(rutaBloque, '/');
                if (!p) p = rutaBloque;
                int nro = 0;
                if (sscanf(p, "Bloque%d.txt", &nro) == 1) {
                    fclose(fcat);
                    return nro;
                }
            }
        }
        fclose(fcat);
        return 0;
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
};
