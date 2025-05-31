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
        strcpy(rutaLongitudFija, "DISCO\\longitudFija.txt");
        strcpy(discoNuevoPath, "DISCO\\");

        make_dir("DISCO");
        make_dir("DISCO\\BLOQUES");

        crearEstructuraDisco();
        crearBloquesLogicos();
    }

    ~Disco() {
    }
    long long get_tam_bloque() const {
        return tamBloque;
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

    int get_tam_bloque(){ //funcion para tamaño de disco
        return (int)tamBloque;
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

    // Convierte una cadena a entero de forma segura.
    // Retorna 0 si la cadena no es un número válido.
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

    bool updateRegistrobackup(const char* nombreRelacion, int lineaObjetivo, const char* registroNuevo) {
        if (lineaObjetivo <= 0) return false;

        // a) Obtener registroSize y longitudes por campo:
        int registroSize;
        obtenerRegistroSize(nombreRelacion, &registroSize);
        if (registroSize <= 0) return false;

        // Separar campos en arreglo para validar cada longitud
        int numFields = 0;
        int maxLenArr[MAX_FIELDS] = { 0 };
        obtenerLongitudesPorCampo(nombreRelacion, &numFields, maxLenArr);
        if (numFields <= 0) return false;

        // Validar que registroNuevo no exceda tamaño máximo de cada campo:
        if (!validarCampos(registroNuevo, numFields, maxLenArr)) return false;
        int newLen = (int)strlen(registroNuevo) + 1;  // +1 por '|'
        if (newLen > registroSize) return false;

        // b) Leer catalogo.txt para obtener bloques asignados a esta relación
#define MAX_BLOCKS 1024
        int bloquesAsignados[MAX_BLOCKS];
        int totalBloques = 0;

        char rutaCatalogo[MAX_PATH_LEN];
        snprintf(rutaCatalogo, sizeof(rutaCatalogo),
            "%s%s", discoNuevoPath, "catalogo.txt");
        FILE* fcat = fopen(rutaCatalogo, "r");
        if (!fcat) return false;

        char linea[MAX_BUF];
        while (fgets(linea, MAX_BUF, fcat)) {
            linea[strcspn(linea, "\r\n")] = '\0';
            char* sep = strchr(linea, '|');
            if (!sep) continue;
            *sep = '\0';
            const char* rel = linea;
            const char* path = sep + 1;
            if (strcmp(rel, nombreRelacion) != 0) continue;
            const char* pN = strstr(path, "Bloque");
            if (!pN) continue;
            pN += strlen("Bloque");
            int n = atoi(pN);
            if (n > 0 && totalBloques < MAX_BLOCKS) {
                bloquesAsignados[totalBloques++] = n;
            }
        }
        fclose(fcat);

        if (totalBloques == 0) return false;
        // Ordenar bloques de menor a mayor
        for (int i = 0; i < totalBloques - 1; i++) {
            for (int j = i + 1; j < totalBloques; j++) {
                if (bloquesAsignados[j] < bloquesAsignados[i]) {
                    int tmp = bloquesAsignados[i];
                    bloquesAsignados[i] = bloquesAsignados[j];
                    bloquesAsignados[j] = tmp;
                }
            }
        }

        int cuentaHastaAhora = 0;

        // c) Recorrer bloques para encontrar dónde está el registro
        for (int bi = 0; bi < totalBloques; bi++) {
            int nroBloque = bloquesAsignados[bi];
            char rutaBloque[MAX_PATH_LEN];
            snprintf(rutaBloque, sizeof(rutaBloque),
                "%sBLOQUES\\Bloque%d.txt",
                discoNuevoPath, nroBloque);

            FILE* fbloc = fopen(rutaBloque, "r+");
            if (!fbloc) continue;

            // Leer bitmap (hasta '/')
            char bitmap[MAX_BUF];
            if (!fgets(bitmap, sizeof(bitmap), fbloc)) {
                fclose(fbloc);
                continue;
            }
            bitmap[strcspn(bitmap, "\r\n")] = '\0';
            int numMaxAnt = 0;
            while (bitmap[numMaxAnt] && bitmap[numMaxAnt] != '/') {
                numMaxAnt++;
            }
            if (numMaxAnt <= 0) {
                fclose(fbloc);
                continue;
            }

            // Contar cuántos '1' en bitmap
            int unosEnBloque = 0;
            for (int i = 0; i < numMaxAnt; i++) {
                if (bitmap[i] == '1') unosEnBloque++;
            }
            if (cuentaHastaAhora + unosEnBloque < lineaObjetivo) {
                cuentaHastaAhora += unosEnBloque;
                fclose(fbloc);
                continue;
            }

            // Está en este bloque → calcular idxLocal
            int idxLocal = lineaObjetivo - cuentaHastaAhora;
            int contador1s = 0;
            int bitPos = -1;
            for (int i = 0; i < numMaxAnt; i++) {
                if (bitmap[i] == '1') {
                    contador1s++;
                    if (contador1s == idxLocal) {
                        bitPos = i;
                        break;
                    }
                }
            }
            if (bitPos < 0) {
                fclose(fbloc);
                return false;
            }

            // Extraer numRegAnt de la cabecera
            size_t raw_header_len = 0;
            rewind(fbloc);
            int c;
            while ((c = fgetc(fbloc)) != EOF) {
                raw_header_len++;
                if (c == '/') break;
                if (raw_header_len >= MAX_BUF - 1) break;
            }
            rewind(fbloc);

            char cabTmp[MAX_BUF];
            if (raw_header_len > MAX_BUF - 1) raw_header_len = MAX_BUF - 1;
            fread(cabTmp, 1, raw_header_len, fbloc);
            cabTmp[raw_header_len] = '\0';
            char* p1 = strchr(cabTmp, '#');
            if (!p1) { fclose(fbloc); return false; }
            *p1 = '\0';
            int numRegAnt = safe_atoi(cabTmp);
            // No necesitamos numMaxAnt de nuevo

            // Buscar límite del registro antiguo:
            long posData = (long)raw_header_len;
            fseek(fbloc, posData, SEEK_SET);
            int barras = 0;
            long inicioReg = posData;
            while ((c = fgetc(fbloc)) != EOF) {
                if (c == '|') {
                    barras++;
                    if (barras == idxLocal) {
                        break;
                    }
                }
                inicioReg++;
            }
            long finReg = ftell(fbloc);
            long oldLen = finReg - inicioReg;
            if (oldLen <= 0) {
                fclose(fbloc);
                return false;
            }

            // Calcular delta:
            int delta = newLen - (int)oldLen;

            // d) Verificar si cabe dentro del bloque si newLen > oldLen
            if (delta > 0) {
                // Leer espacio libre actual en dirBloques.txt
                FILE* fdir = fopen(rutaDirBloques, "r+");
                if (!fdir) { fclose(fbloc); return false; }

                // Buscar línea de este bloque en dirBloques.txt para leer espacioLibreSector y espacioLibreBloque
                char lineaDir[MAX_BUF];
                int contadorDir = 0;
                long posLineaDir = 0;
                int espLibreSector = 0;
                int espLibreBloque = 0;
                rewind(fdir);
                while (fgets(lineaDir, MAX_BUF, fdir)) {
                    ++contadorDir;
                    if (contadorDir == nroBloque) {
                        posLineaDir = ftell(fdir) - (long)strlen(lineaDir);
                        char copiaDir[MAX_BUF];
                        strncpy(copiaDir, lineaDir, MAX_BUF);
                        copiaDir[MAX_BUF - 1] = '\0';
                        // Extraer espacioLibreBloque (antes del primer '#')
                        char* tokB = strtok(copiaDir, "#");
                        espLibreBloque = safe_atoi(tokB);
                        // Extraer primer sector libre: buscar "#_" y luego leer primer número
                        char* psect = strstr(lineaDir, "#_");
                        if (psect) {
                            psect += 2;
                            espLibreSector = atoi(psect);
                        }
                        break;
                    }
                }
                fclose(fdir);

                if (espLibreBloque < delta || espLibreSector < delta) {
                    fclose(fbloc);
                    return false; // No hay espacio suficiente
                }
            }

            // e) Desplazar datos y sobrescribir:
            // Leer hasta fin de archivo para desplazar remainder
            fseek(fbloc, 0, SEEK_END);
            long eofPos = ftell(fbloc);
            long moveStart = inicioReg + oldLen;      // byte donde termina registro viejo
            long moveEnd = eofPos;                  // byte final
            long shiftBy = delta;                   // +para derecha, -para izquierda

            if (delta > 0) {
                // DESPLAZAR HACIA LA DERECHA: copiar de atrás hacia adelante
                long readPos = moveEnd - 1;
                long writePos = readPos + shiftBy;
                while (readPos >= moveStart) {
                    fseek(fbloc, readPos, SEEK_SET);
                    int ch = fgetc(fbloc);
                    fseek(fbloc, writePos, SEEK_SET);
                    fputc(ch, fbloc);
                    readPos--;
                    writePos--;
                }
            }
            else if (delta < 0) {
                // DESPLAZAR HACIA LA IZQUIERDA: copiar de adelante hacia atrás
                long readPos = moveStart;
                long writePos = moveStart + shiftBy; // shiftBy es negativo
                while (readPos < moveEnd) {
                    fseek(fbloc, readPos, SEEK_SET);
                    int ch = fgetc(fbloc);
                    fseek(fbloc, writePos, SEEK_SET);
                    fputc(ch, fbloc);
                    readPos++;
                    writePos++;
                }
                // Rellenar el final sobrante con espacios (o '#')
                long fillPos = moveEnd + shiftBy;
                fseek(fbloc, fillPos, SEEK_SET);
                for (long k = fillPos; k < moveEnd; k++) {
                    fputc(' ', fbloc);
                }
            }

            // Sobreescribir el registro antiguo por “registroNuevo|”
            // Limpiar saltos de línea y asegurar separador '|'
            char regLimpio[MAX_BUF];
            size_t len = strlen(registroNuevo);
            // Copiar y limpiar '\n' y '\r'
            size_t k = 0;
            for (size_t i = 0; i < len && k < MAX_BUF - 2; ++i) {
                if (registroNuevo[i] != '\n' && registroNuevo[i] != '\r')
                    regLimpio[k++] = registroNuevo[i];
            }
            regLimpio[k] = '\0';

            // Escribir el registro limpio y el separador '|'
            fseek(fbloc, inicioReg, SEEK_SET);
            fwrite(regLimpio, 1, k, fbloc);
            fputc('|', fbloc);
            fflush(fbloc);

            // f) Actualizar bitmap (permanece igual porque sigue “ocupado”).
            // g) Actualizar dirBloques.txt restando (newLen - oldLen) de espacio libre
            int deltaEsp = (int)(oldLen - newLen); // si new>old: negativo, sumará espacio usado
            {
                FILE* fdir = fopen(rutaDirBloques, "r+");
                if (!fdir) {
                    fclose(fbloc);
                    return false;
                }
                char lineaD[MAX_BUF];
                int contadorD = 0;
                long posD = 0;
                rewind(fdir);
                while (fgets(lineaD, MAX_BUF, fdir)) {
                    ++contadorD;
                    if (contadorD == nroBloque) {
                        posD = ftell(fdir) - (long)strlen(lineaD);
                        break;
                    }
                }
                if (posD == 0) {
                    fclose(fdir);
                    fclose(fbloc);
                    return false;
                }
                fseek(fdir, posD, SEEK_SET);
                fgets(lineaD, MAX_BUF, fdir);
                lineaD[strcspn(lineaD, "\r\n")] = '\0';

                // Extraer espacioLibreBloque y reconstruir línea:
                char copiaD2[MAX_BUF];
                strncpy(copiaD2, lineaD, MAX_BUF);
                copiaD2[MAX_BUF - 1] = '\0';
                char* tokB2 = strtok(copiaD2, "#");
                int espBloqueAnt = safe_atoi(tokB2);
                int espBloqueNuevo = espBloqueAnt + deltaEsp;

                // Reconstruir sectores (misma lógica que en adicionar)
                char* ptrSect = strstr(lineaD, "#_");
                if (!ptrSect) {
                    fclose(fdir);
                    fclose(fbloc);
                    return false;
                }
                char sectoresStr[MAX_BUF];
                strncpy(sectoresStr, ptrSect + 2, MAX_BUF - 1);
                sectoresStr[MAX_BUF - 1] = '\0';

                char nuevaLinea[MAX_BUF];
                snprintf(nuevaLinea, MAX_BUF,
                    "%d#2#BLOQUE#%d#%d#_%s\n",
                    espBloqueNuevo,
                    nroBloque,
                    tamBloque,
                    sectoresStr);

                fseek(fdir, posD, SEEK_SET);
                fwrite(nuevaLinea, 1, strlen(nuevaLinea), fdir);
                fflush(fdir);
                fclose(fdir);
            }

            fclose(fbloc);
            return true;
        }

        return false;
    }

    bool updateRegistro(const char* nombreRelacion,
        int lineaObjetivo,
        const char* registroNuevo)
    {
        if (lineaObjetivo <= 0) return false;

        // 1) Leer registroSize y maxLenArr[] desde longitudfija.txt
        int registroSize;
        obtenerRegistroSize(nombreRelacion, &registroSize);
        if (registroSize <= 0) return false;

        int numFields = 0;
        int maxLenArr[MAX_FIELDS] = { 0 };
        obtenerLongitudesPorCampo(nombreRelacion, &numFields, maxLenArr);
        if (numFields <= 0) return false;

        // 2) Limpiar registroNuevo de '\r' y '\n' y validar campos
        char regLimpio[MAX_BUF];
        size_t raw = strlen(registroNuevo);
        size_t w = 0;
        for (size_t i = 0; i < raw && w < MAX_BUF - 2; i++) {
            if (registroNuevo[i] != '\n' && registroNuevo[i] != '\r') {
                regLimpio[w++] = registroNuevo[i];
            }
        }
        regLimpio[w] = '\0';

        // Validar número de campos y longitud de cada uno
        if (!validarCampos(regLimpio, numFields, maxLenArr)) return false;

        // newLen = w + 1 (por el '|')
        int newLen = (int)w + 1;
        if (newLen > registroSize) return false;

        // 3) Leer catalogo.txt y armar lista de bloques para esta relación
#define MAX_BLOCKS 1024
        int bloquesAsignados[MAX_BLOCKS];
        int totalBloques = 0;

        char rutaCatalogo[MAX_PATH_LEN];
        snprintf(rutaCatalogo, sizeof(rutaCatalogo), "%s%s", discoNuevoPath, "catalogo.txt");
        FILE* fcat = fopen(rutaCatalogo, "r");
        if (!fcat) return false;

        char linea[MAX_BUF];
        while (fgets(linea, MAX_BUF, fcat)) {
            linea[strcspn(linea, "\r\n")] = '\0';
            char* sep = strchr(linea, '|');
            if (!sep) continue;
            *sep = '\0';
            const char* rel = linea;
            const char* path = sep + 1;
            if (strcmp(rel, nombreRelacion) != 0) continue;
            const char* pN = strstr(path, "Bloque");
            if (!pN) continue;
            pN += strlen("Bloque");
            int n = atoi(pN);
            if (n > 0 && totalBloques < MAX_BLOCKS) {
                bloquesAsignados[totalBloques++] = n;
            }
        }
        fclose(fcat);
        if (totalBloques == 0) return false;

        // Ordenar bloques
        for (int i = 0; i < totalBloques - 1; i++) {
            for (int j = i + 1; j < totalBloques; j++) {
                if (bloquesAsignados[j] < bloquesAsignados[i]) {
                    int tmp = bloquesAsignados[i];
                    bloquesAsignados[i] = bloquesAsignados[j];
                    bloquesAsignados[j] = tmp;
                }
            }
        }

        // 4) Buscar qué bloque contiene la “líneaObjetivo”-ésima
        int cuentaHastaAhora = 0;
        for (int bi = 0; bi < totalBloques; bi++) {
            int nroBloque = bloquesAsignados[bi];
            char rutaBloque[MAX_PATH_LEN];
            snprintf(rutaBloque, sizeof(rutaBloque),
                "%sBLOQUES\\Bloque%d.txt", discoNuevoPath, nroBloque);

            FILE* fbloc = fopen(rutaBloque, "r+");
            if (!fbloc) continue;

            // 4.a) Leer el bitmap hasta '/'
            char bitmap[MAX_BUF];
            if (!fgets(bitmap, sizeof(bitmap), fbloc)) {
                fclose(fbloc);
                continue;
            }
            bitmap[strcspn(bitmap, "\r\n")] = '\0';
            int numMaxAnt = 0;
            while (bitmap[numMaxAnt] && bitmap[numMaxAnt] != '/') {
                numMaxAnt++;
            }
            if (numMaxAnt <= 0) {
                fclose(fbloc);
                continue;
            }
            int unosEnBloque = 0;
            for (int i = 0; i < numMaxAnt; i++) {
                if (bitmap[i] == '1') unosEnBloque++;
            }
            if (cuentaHastaAhora + unosEnBloque < lineaObjetivo) {
                cuentaHastaAhora += unosEnBloque;
                fclose(fbloc);
                continue;
            }

            // 4.b) Estamos en el bloque correcto. Calcular idxLocal (1-based dentro de este bloque)
            int idxLocal = lineaObjetivo - cuentaHastaAhora;
            int cnt1 = 0, bitPos = -1;
            for (int i = 0; i < numMaxAnt; i++) {
                if (bitmap[i] == '1') {
                    cnt1++;
                    if (cnt1 == idxLocal) {
                        bitPos = i;
                        break;
                    }
                }
            }
            if (bitPos < 0) {
                fclose(fbloc);
                return false;
            }

            // 4.c) Calcular raw_header_len (bytes hasta '/')
            size_t raw_header_len = 0;
            rewind(fbloc);
            int ch;
            while ((ch = fgetc(fbloc)) != EOF) {
                raw_header_len++;
                if (ch == '/') break;
                if (raw_header_len >= MAX_BUF - 1) break;
            }
            rewind(fbloc);

            // 4.d) Encontrar “inicioReg” y “finReg” del viejo registro:
            long posData = (long)raw_header_len;
            fseek(fbloc, posData, SEEK_SET);

            int barras = 0;
            long inicioReg = posData;
            long finReg = posData;
            while ((ch = fgetc(fbloc)) != EOF) {
                if (ch == '|') {
                    barras++;
                    finReg = ftell(fbloc); // justo después del '|'
                    if (barras == idxLocal) {
                        break;
                    }
                }
                inicioReg++;
            }
            long oldLen = finReg - inicioReg;
            if (oldLen <= 0) {
                fclose(fbloc);
                return false;
            }

            // 4.e) Calcular delta y, si es >0, verificar espacio en dirBloques.txt
            int delta = newLen - (int)oldLen;
            if (delta > 0) {
                FILE* fdir = fopen(rutaDirBloques, "r+");
                if (!fdir) {
                    fclose(fbloc);
                    return false;
                }
                char lineaDir[MAX_BUF];
                int cntDir = 0;
                int espBloque = 0, espSector = 0;
                rewind(fdir);
                while (fgets(lineaDir, MAX_BUF, fdir)) {
                    ++cntDir;
                    if (cntDir == nroBloque) {
                        // Extraer espacioLibreBloque
                        char copiaD[MAX_BUF];
                        strcpy(copiaD, lineaDir);
                        char* t = strtok(copiaD, "#");
                        espBloque = safe_atoi(t);
                        // Extraer primer espacioLibreSector
                        char* psect = strstr(lineaDir, "#_");
                        if (psect) {
                            psect += 2;
                            espSector = atoi(psect);
                        }
                        break;
                    }
                }
                fclose(fdir);
                if (espBloque < delta || espSector < delta) {
                    fclose(fbloc);
                    return false;
                }
            }

            // 4.f) Desplazar bytes: si delta>0: atrás→adelante; si delta<0: adelante→atrás.
            fseek(fbloc, 0, SEEK_END);
            long eofPos = ftell(fbloc);
            long moveStart = inicioReg + oldLen;
            long moveEnd = eofPos;
            long shiftBy = delta;
            if (delta > 0) {
                for (long r = moveEnd - 1; r >= moveStart; --r) {
                    fseek(fbloc, r, SEEK_SET);
                    int c2 = fgetc(fbloc);
                    fseek(fbloc, r + shiftBy, SEEK_SET);
                    fputc(c2, fbloc);
                }
            }
            else if (delta < 0) {
                for (long r = moveStart; r < moveEnd; ++r) {
                    fseek(fbloc, r, SEEK_SET);
                    int c2 = fgetc(fbloc);
                    fseek(fbloc, r + shiftBy, SEEK_SET);
                    fputc(c2, fbloc);
                }
                long fillFrom = moveEnd + shiftBy;
                fseek(fbloc, fillFrom, SEEK_SET);
                for (long x = fillFrom; x < moveEnd; x++) {
                    fputc(' ', fbloc);
                }
            }

            // 4.g) Sobrescribir el viejo registro por “regLimpio|”
            fseek(fbloc, inicioReg, SEEK_SET);
            fwrite(regLimpio, 1, w, fbloc);
            fputc('|', fbloc);
            fflush(fbloc);

            // 4.h) Actualizar dirBloques.txt: restar (newLen - oldLen) al espacio libre
            int ajuste = oldLen - newLen;
            {
                FILE* fdir = fopen(rutaDirBloques, "r+");
                if (!fdir) {
                    fclose(fbloc);
                    return false;
                }
                char lineaD2[MAX_BUF];
                int cnt2 = 0;
                long posD = 0;
                rewind(fdir);
                while (fgets(lineaD2, MAX_BUF, fdir)) {
                    ++cnt2;
                    if (cnt2 == nroBloque) {
                        posD = ftell(fdir) - (long)strlen(lineaD2);
                        break;
                    }
                }
                if (posD == 0) {
                    fclose(fdir);
                    fclose(fbloc);
                    return false;
                }
                fseek(fdir, posD, SEEK_SET);
                fgets(lineaD2, MAX_BUF, fdir);
                lineaD2[strcspn(lineaD2, "\r\n")] = '\0';

                char copia2[MAX_BUF];
                strncpy(copia2, lineaD2, MAX_BUF);
                copia2[MAX_BUF - 1] = '\0';
                char* t2 = strtok(copia2, "#");
                int espBloAnte = safe_atoi(t2);
                int espBloNuev = espBloAnte + ajuste;

                char* ptrSec = strstr(lineaD2, "#_");
                if (!ptrSec) {
                    fclose(fdir);
                    fclose(fbloc);
                    return false;
                }
                char sectoresStr[MAX_BUF];
                strncpy(sectoresStr, ptrSec + 2, MAX_BUF - 1);
                sectoresStr[MAX_BUF - 1] = '\0';

                char nuevaLinea[MAX_BUF];
                snprintf(nuevaLinea, MAX_BUF,
                    "%d#2#BLOQUE#%d#%d#_%s\n",
                    espBloNuev,
                    nroBloque,
                    tamBloque,
                    sectoresStr);

                fseek(fdir, posD, SEEK_SET);
                fwrite(nuevaLinea, 1, strlen(nuevaLinea), fdir);
                fflush(fdir);
                fclose(fdir);
            }

            fclose(fbloc);
            return true;
        }

        return false;
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

    bool eliminarRegistroNoLoUsamos(const char* nombreRelacion, int lineaObjetivo) {
        if (lineaObjetivo <= 0) return false;

        const char rutaBloque[] = "DISCO/BLOQUES/Bloque1.txt";
        printf("[DEBUG] Abriendo archivo: %s\n", rutaBloque);
        FILE* archivo = fopen(rutaBloque, "r+");
        if (!archivo) {
            printf("[DEBUG] No se pudo abrir el archivo del bloque\n");
            return false;
        }

        // --- (1) Leer la primera "línea" completa con fgets, que en este
        //          caso contenga: header + "/" + posible resto de datos si no había CRLF ---
        char buffer[MAX_LINE];
        if (!fgets(buffer, sizeof(buffer), archivo)) {
            printf("[DEBUG] No se pudo leer la cabecera del bloque\n");
            fclose(archivo);
            return false;
        }

        // --- (2) Quitar CR/LF al final de ese "buffer" ---
        size_t lenBmp = strlen(buffer);
        while (lenBmp > 0 && (buffer[lenBmp - 1] == '\n' || buffer[lenBmp - 1] == '\r')) {
            buffer[--lenBmp] = '\0';
        }
        printf("[DEBUG] Buffer tras quitar CRLF: '%s' (len=%zu)\n", buffer, lenBmp);

        // --- (3) Truncar justo en el slash '/', que marca el fin del bitmap ---
        char* slash = strchr(buffer, '/');
        if (!slash) {
            printf("[DEBUG] ERROR: No se encontró '/' en la cabecera\n");
            fclose(archivo);
            return false;
        }
        // longitud real de header = (posición del slash - inicio) + 1
        size_t headerBitsLen = (slash - buffer) + 1;
        buffer[headerBitsLen] = '\0';
        printf("[DEBUG] Header puro leido (hasta '/'): '%s' (bytes = %zu)\n",
            buffer, headerBitsLen);

        // --- (4) Buscar la línea objetivo dentro de ese bitmap puro ---
        int contador1s = 0;
        int bitIndex = -1;
        // El bitmap está en buffer[0..headerBitsLen-2], y buffer[headerBitsLen-1]=='/'
        // Recorremos solo hasta 'headerBitsLen-1' y no más allá:
        for (size_t i = 0; i + 1 < headerBitsLen; i++) {
            if (buffer[i] == '1') {
                contador1s++;
                if (contador1s == lineaObjetivo) {
                    bitIndex = (int)i;
                    break;
                }
            }
        }
        printf("[DEBUG] contador1s=%d, bitIndex=%d\n", contador1s, bitIndex);
        if (bitIndex < 0) {
            printf("[DEBUG] No hay suficientes '1' en el bitmap para lineaObjetivo=%d\n",
                lineaObjetivo);
            fclose(archivo);
            return false;
        }

        // --- (5) Marcar esa posición a '0' ---
        buffer[bitIndex] = '0';
        printf("[DEBUG] Bitmap modificado a: '%s'\n", buffer);

        // --- (6) Reescribir SOLO ESOS 'headerBitsLen' bytes + "\r\n" ---
        fseek(archivo, 0, SEEK_SET);
        fwrite(buffer, 1, headerBitsLen, archivo);
        fputc('\r', archivo);
        fputc('\n', archivo);
        fflush(archivo);

        // --- (7) Calcular raw_header_len = headerBitsLen + 2 bytes de CRLF ---
        size_t raw_header_len = headerBitsLen + 2;
        printf("[DEBUG] raw_header_len (cabecera en bytes): %zu\n", raw_header_len);

        // --- (8) Movernos justo al byte siguiente a la cabecera física ---
        fseek(archivo, (long)raw_header_len, SEEK_SET);

        // --- (9) Ahora contamos separadores '|' para hallar dónde empieza el registro viejo ---
        int contSeparadores = 0;
        long inicioReg = ftell(archivo);  // si lineaObjetivo==1, comenzamos aquí
        int c;
        while ((c = fgetc(archivo)) != EOF) {
            if (c == '|') {
                contSeparadores++;
                // en el momento en que contSeparadores == (lineaObjetivo - 1),
                // el siguiente byte es el inicio real del registro a eliminar.
                if (contSeparadores == lineaObjetivo - 1) {
                    inicioReg = ftell(archivo);
                    break;
                }
            }
        }
        printf("[DEBUG] contSeparadores=%d, inicioReg=%ld\n", contSeparadores, inicioReg);
        if (contSeparadores < lineaObjetivo - 1) {
            printf("[DEBUG] No se encontraron suficientes '|' para lineaObjetivo=%d\n",
                lineaObjetivo);
            fclose(archivo);
            return false;
        }

        // --- (10) Encontrar el '|' final que cierra EL registro VIEJO ---
        long finReg = inicioReg;
        while ((c = fgetc(archivo)) != EOF) {
            finReg = ftell(archivo);
            if (c == '|') {
                break;
            }
        }
        printf("[DEBUG] finReg=%ld\n", finReg);
        if (finReg <= inicioReg) {
            printf("[DEBUG] No se encontró el separador '|' final para el registro\n");
            fclose(archivo);
            return false;
        }

        // --- (11) Calcular cuántos bytes ocupa el registro viejo (oldLen) ---
        long oldLen = finReg - inicioReg;
        printf("[DEBUG] oldLen (bytes del registro viejo): %ld\n", oldLen);

        // --- (12) Sobrescribir ESOS oldLen bytes con '#' para "eliminar" ese registro ---
        fseek(archivo, inicioReg, SEEK_SET);
        for (long i = 0; i < oldLen; i++) {
            fputc('#', archivo);
        }
        fflush(archivo);

        printf("[DEBUG] Registro eliminado correctamente\n");
        fclose(archivo);
        return true;
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

    /*
    bool eliminarRegistroNoFuncionaCorrecto(const char* nombreRelacion, int lineaObjetivo) {
        if (lineaObjetivo <= 0) return false;

        // === PARTE A: actualizar header y borrar registro dentro de Bloque1.txt ===

        // 1) Abrir archivo de bloque en modo "r+"
        const char rutaBloque[] = "DISCO/BLOQUES/Bloque1.txt";
        FILE* archivo = fopen(rutaBloque, "r+");
        if (!archivo) {
            return false;
        }

        // 2) Leer la primera “línea” completa del bloque (cabecera + “/” + posible resto)
        char buffer[MAX_LINE];
        if (!fgets(buffer, sizeof(buffer), archivo)) {
            fclose(archivo);
            return false;
        }
        // buffer_incl contiene la línea tal cual la lee fgets (incluye '\n' o "\r\n")
        size_t len_incl = strlen(buffer);

        // 3) Crear una copia “sin CR/LF” para procesar el bitmap
        char buffer_sin[MAX_LINE];
        strncpy(buffer_sin, buffer, MAX_LINE - 1);
        buffer_sin[MAX_LINE - 1] = '\0';
        size_t len_no_crlf = strlen(buffer_sin);
        while (len_no_crlf > 0 && (buffer_sin[len_no_crlf - 1] == '\n' || buffer_sin[len_no_crlf - 1] == '\r')) {
            buffer_sin[--len_no_crlf] = '\0';
        }

        // 4) Buscar el slash '/' que marca el fin del bitmap
        char* slash = strchr(buffer_sin, '/');
        if (!slash) {
            fclose(archivo);
            return false;
        }
        // headerBitsLen = posición del slash - inicio + 1 (incluye '/')
        size_t headerBitsLen = (slash - buffer_sin) + 1;

        // 5) Contar cuántas '1' hasta alcanzar lineaObjetivo → encontrar bitIndex
        int contador1s = 0;
        int bitIndex = -1;
        for (size_t i = 0; i + 1 < headerBitsLen; i++) {
            if (buffer_sin[i] == '1') {
                contador1s++;
                if (contador1s == lineaObjetivo) {
                    bitIndex = (int)i;
                    break;
                }
            }
        }
        if (bitIndex < 0) {
            // No había suficientes '1'
            fclose(archivo);
            return false;
        }

        // 6) Marcar ese bit a '0' en buffer_sin
        buffer_sin[bitIndex] = '0';

        // 7) Sobrescribir EN EL ARCHIVO SOLO los primeros headerBitsLen bytes + "\r\n"
        //    Para ello debemos ir a la posición EXACTA donde comienza esta línea:
        //    posLineaBloque = ftell (después de fgets) - len_incl
        long posLineaBloque = ftell(archivo) - (long)len_incl;
        //   raw_header_len = headerBitsLen + 2 (para CRLF)
        size_t raw_header_len = headerBitsLen + 2;

        //   Construimos un buffer temporal para escribir:
        //   • copiamos buffer_sin[0..headerBitsLen-1],
        //   • luego ponemos '\r' y '\n'
        char tmpHeader[MAX_LINE];
        // Copiamos los headerBitsLen bytes
        memcpy(tmpHeader, buffer_sin, headerBitsLen);
        // A continuación CRLF
        tmpHeader[headerBitsLen] = '\r';
        tmpHeader[headerBitsLen + 1] = '\n';

        // 8) Escribimos exactamente raw_header_len bytes
        fseek(archivo, posLineaBloque, SEEK_SET);
        fwrite(tmpHeader, 1, raw_header_len, archivo);
        fflush(archivo);

        // 9) Ahora detectamos el tamaño “oldLen” del registro a eliminar (después de la cabecera física)
        fseek(archivo, (long)raw_header_len, SEEK_SET);
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
            // No encontramos suficientes '|'
            fclose(archivo);
            return false;
        }
        // Buscar el siguiente '|' que cierra el registro viejo
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
        long oldLen = finReg - inicioReg;

        // 10) Sobrescribir esos oldLen bytes con '#' (invalida el registro)
        fseek(archivo, inicioReg, SEEK_SET);
        for (long i = 0; i < oldLen; i++) {
            fputc('#', archivo);
        }
        fflush(archivo);
        fclose(archivo);

        // === PARTE C: actualizar dirBloques.txt ===

        // 1) Abrir dirBloques.txt en “r+”
        FILE* fdir = fopen(rutaDirBloques, "r+");
        if (!fdir) {
            return false;
        }

        // 2) Leer línea a línea hasta encontrar “#BLOQUE#1#...” (nroBloque=1)
        char linea[MAX_BUF];
        bool foundBlock = false;
        int  lineaNum = 0;
        long posDirLinea = 0;    // posición donde empieza la línea actual
        size_t len_incl_dir = 0; // longitud real de la línea leída (incluyendo '\n' o "\r\n")

        while (fgets(linea, sizeof(linea), fdir)) {
            lineaNum++;
            len_incl_dir = strlen(linea);
            posDirLinea = ftell(fdir) - (long)len_incl_dir;

            // Creamos copia sin CRLF para parseo
            char linea_sin[MAX_BUF];
            strncpy(linea_sin, linea, MAX_BUF - 1);
            linea_sin[MAX_BUF - 1] = '\0';
            size_t len_sin = strlen(linea_sin);
            while (len_sin > 0 && (linea_sin[len_sin - 1] == '\n' || linea_sin[len_sin - 1] == '\r')) {
                linea_sin[--len_sin] = '\0';
            }

            // Buscar “#BLOQUE#” y extraer el número
            char* pb = strstr(linea_sin, "#BLOQUE#");
            if (!pb) continue;
            pb += strlen("#BLOQUE#");
            int nro = atoi(pb);
            if (nro != 1) continue;

            // Esta es la línea para Bloque1
            foundBlock = true;
            // Reconocemos len_no_crlf_dir = len_sin
            break;
        }
        if (!foundBlock) {
            fclose(fdir);
            return false;
        }

        // 3) Ahora “linea_sin” contiene la línea ENTENDIBLE sin CRLF
        //    len_sin = strlen(linea_sin). Raw total = len_sin + 2.
        size_t len_sin_dir = strlen(linea_sin);
        size_t raw_len_total_dir = len_sin_dir + 2;

        // 4) Parsear espacioLibreBloque (primer token antes de '#')
        char copia_dir[MAX_BUF];
        strncpy(copia_dir, linea_sin, MAX_BUF - 1);
        copia_dir[MAX_BUF - 1] = '\0';
        char* tokEspBloq = strtok(copia_dir, "#");
        if (!tokEspBloq) {
            fclose(fdir);
            return false;
        }
        int espacioBloqueAntes = atoi(tokEspBloq);
        int espacioBloqueNuevo = espacioBloqueAntes + (int)oldLen;

        // 5) Extraer “#2#BLOQUE#1#<tamBloque>#”
        char* tok2 = strtok(NULL, "#"); // “2”
        char* tokBLOQ = strtok(NULL, "#"); // “BLOQUE”
        char* tokNumBloq = strtok(NULL, "#"); // “1”
        char* tokTamBloq = strtok(NULL, "#"); // e.g. “22400”
        if (!tokTamBloq) {
            fclose(fdir);
            return false;
        }
        int tamBloqLeido = atoi(tokTamBloq);

        // 6) Reconstruir la parte “<espBloqueNuevo>#2#BLOQUE#1#<tamBloque>#_”
        //    en un buffer temporal “lineaNueva” de longitud EXACTA raw_len_total_dir.
        char lineaNueva[MAX_BUF];
        // Inicializar con espacios (hasta raw_len_total_dir), luego corregir al final CRLF.
        for (size_t i = 0; i < raw_len_total_dir; i++) {
            lineaNueva[i] = ' ';
        }
        // Pondremos el CRLF en las dos últimas posiciones:
        if (raw_len_total_dir >= 2) {
            lineaNueva[raw_len_total_dir - 2] = '\r';
            lineaNueva[raw_len_total_dir - 1] = '\n';
        }

        // Escribir el encabezado útil al inicio (sin sobrepasar raw_len_total_dir-2)
        int ofs = snprintf(
            lineaNueva,
            (raw_len_total_dir >= 2 ? raw_len_total_dir - 2 : 0),
            "%d#2#BLOQUE#%d#%d#_",
            espacioBloqueNuevo,
            1,
            tamBloqLeido
        );
        if (ofs < 0) ofs = 0;
        if ((size_t)ofs > raw_len_total_dir - 2) {
            ofs = (int)raw_len_total_dir - 2;
        }

        // 7) Ahora recorremos la lista de sectores a partir de “#_<lista>” en linea_sin
        char* inicioSect = strstr(linea_sin, "#_");
        if (!inicioSect) {
            // No había lista de sectores (caso extraño), pero aún así escribimos el header
            fseek(fdir, posDirLinea, SEEK_SET);
            fwrite(lineaNueva, 1, raw_len_total_dir, fdir);
            fflush(fdir);
            fclose(fdir);
            return true;
        }
        // Saltamos "#_"
        inicioSect += 2;

        bool sectorActualizado = false;
        char* p = inicioSect;
        while (*p && (size_t)ofs < raw_len_total_dir - 2) {
            // Leer espLibreSector (cadena numérica hasta '#')
            char* p_iniEsp = p;
            while (*p && *p != '#') p++;
            if (!*p) break;
            *p = '\0';
            int espSecAntes = atoi(p_iniEsp);
            *p = '#';
            p++;

            // Leer codSector (hasta siguiente '#')
            char* p_iniCod = p;
            while (*p && *p != '#') p++;
            if (!*p) break;
            *p = '\0';
            char codSector[MAX_STR_LEN];
            // Copiar con límite
            strncpy(codSector, p_iniCod, MAX_STR_LEN - 1);
            codSector[MAX_STR_LEN - 1] = '\0';
            *p = '#';
            p++;

            // Determinar el nuevo espLibreSector
            int espSecNuevo = espSecAntes;
            // Heurística: si codSector empieza por “1/” → bloque 1
            if (!sectorActualizado && strncmp(codSector, "1/", 2) == 0) {
                espSecNuevo += (int)oldLen;
                sectorActualizado = true;
            }

            // Escribir en lineaNueva en ofs: "<espSecNuevo>#<codSector>#_"
            int written = snprintf(
                lineaNueva + ofs,
                (raw_len_total_dir - 2 >= (size_t)ofs ? raw_len_total_dir - 2 - ofs : 0),
                "%d#%s#_",
                espSecNuevo,
                codSector
            );
            if (written < 0) written = 0;
            if ((size_t)written > raw_len_total_dir - 2 - (size_t)ofs) {
                written = (int)(raw_len_total_dir - 2 - (size_t)ofs);
            }
            ofs += written;

            // Avanzar p a la próxima ocurrencia de "#_"
            char* nextPair = strstr(p, "#_");
            if (!nextPair) break;
            p = nextPair + 2;
        }

        // 8) Si nunca actualizamos ningún sector (sectorActualizado==false), copiamos idéntico el resto de la lista
        if (!sectorActualizado) {
            // Queremos copiar todo “<esp>#<cod>#_...” desde inicioSect-2 hasta el final de linea_sin
            // (pero no lo copiamos si excede raw_len_total_dir-2, pues lo restante queda en espacios).
            size_t restoLen = strlen(inicioSect);
            if ((size_t)ofs + restoLen > raw_len_total_dir - 2) {
                restoLen = (raw_len_total_dir - 2) - (size_t)ofs;
            }
            if (restoLen > 0) {
                memcpy(lineaNueva + ofs, inicioSect, restoLen);
                ofs += (int)restoLen;
            }
        }

        // 9) Finalmente escribimos EXACTAMENTE raw_len_total_dir bytes en posDirLinea
        fseek(fdir, posDirLinea, SEEK_SET);
        fwrite(lineaNueva, 1, raw_len_total_dir, fdir);
        fflush(fdir);
        fclose(fdir);

        return true;
    }
    */


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

    /*
    bool adicionarRegistroUnico(const char* registroTxt, const char* relacion) {
        // --- 0) Obtener tamaño fijo del registro ---
        int registroSize;
        obtenerRegistroSize(relacion, &registroSize);
        if (registroSize <= 0) {
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

        int tamRegistro = registroSize;
        int espacioLibreBloque = 0;
        int tamUtilAntes = 0;
        int espacioLibreSectorAntes = 0;

        // 2) Buscar el primer bloque/sector con espacio suficiente
        while (true) {
            posLineaBloque = ftell(fdir);
            if (!fgets(linea, MAX_BUF, fdir)) break;
            nroBloque++;

            // Quitar CRLF
            size_t len_linea = strlen(linea);
            if (len_linea > 0 && linea[len_linea - 1] == '\n')  linea[--len_linea] = '\0';
            if (len_linea > 0 && linea[len_linea - 1] == '\r')  linea[--len_linea] = '\0';

            // Extraer espacioLibreBloque (primer token antes de '#')
            char copiaBloc[MAX_BUF];
            strncpy(copiaBloc, linea, MAX_BUF - 1);
            copiaBloc[MAX_BUF - 1] = '\0';
            char* tokBloc = strtok(copiaBloc, "#");
            if (!tokBloc) continue;
            espacioLibreBloque = safe_atoi(tokBloc);
            tamUtilAntes = tamBloque - espacioLibreBloque;
            if (espacioLibreBloque < tamRegistro) {
                continue;
            }

            // Leer la lista de sectores: buscar primer "#_"
            char* p = strstr(linea, "#_");
            if (!p) continue;
            // Trabajar sobre copia para no modificar la original
            strncpy(copiaBloc, linea, MAX_BUF - 1);
            copiaBloc[MAX_BUF - 1] = '\0';
            p = strstr(copiaBloc, "#_");
            if (!p) continue;
            p += 2;

            // Cada par "<espLibreSector>#<codSector>#_"
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

        // 3) Volver a la posición de la línea en dirBloques.txt y releerla para calcular raw_len
        fseek(fdir, posLineaBloque, SEEK_SET);
        if (!fgets(linea, MAX_BUF, fdir)) {
            fclose(fdir);
            return false;
        }

        // Medir longitud en disco (incluye CRLF)
        size_t len_linea = strlen(linea);
        if (len_linea > 0 && linea[len_linea - 1] == '\n') {
            len_linea--;
        }
        size_t raw_len_total = len_linea + 1;
        {
            size_t lenNoCrLf = raw_len_total;
            if (lenNoCrLf > 0 && linea[lenNoCrLf - 1] == '\n') {
                lenNoCrLf--;
            }
            if (lenNoCrLf > 0 && linea[lenNoCrLf - 1] == '\r') {
                raw_len_total = lenNoCrLf + 2;
            }
            else {
                raw_len_total = lenNoCrLf + 1;
            }
        }

        // 4) Reconstruir la nueva línea en dirBloques.txt
        fseek(fdir, posLineaBloque, SEEK_SET);
        fgets(linea, MAX_BUF, fdir);
        linea[strcspn(linea, "\r\n")] = '\0';
        char* inicioSect = strstr(linea, "#_");
        if (!inicioSect) {
            fclose(fdir);
            return false;
        }

        char bufferNueva[MAX_BUF];
        int ofsN = 0;
        // "<espBloqueNuevo>#2#BLOQUE#<nroBloque>#<tamBloque>#_"
        ofsN += snprintf(bufferNueva + ofsN, MAX_BUF - ofsN,
            "%d#2#BLOQUE#%d#%d#_",
            espacioBloqueNuevo,
            nroBloque,
            tamBloque);

        // Copiar lista de sectores, restando tamRegistro al usado
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
        if ((size_t)ofsN > raw_len_total - 2) {
            // Si la porción útil ya excede raw_len_total-2, truncamos y ponemos CRLF
            if (raw_len_total >= 2) {
                bufferNueva[raw_len_total - 2] = '\r';
                bufferNueva[raw_len_total - 1] = '\n';
            }
        }
        else {
            // Rellenar con espacios hasta raw_len_total-2
            for (int i = ofsN; i < (int)raw_len_total - 2; i++) {
                bufferNueva[i] = ' ';
            }
            // Poner CRLF al final
            bufferNueva[raw_len_total - 2] = '\r';
            bufferNueva[raw_len_total - 1] = '\n';
        }

        // 6) Sobreescribir EXACTAMENTE raw_len_total bytes
        fseek(fdir, posLineaBloque, SEEK_SET);
        fwrite(bufferNueva, 1, raw_len_total, fdir);
        fflush(fdir);
        fclose(fdir);

        // -------------------------------------------------------------
        // 7) Actualizar BloqueN.txt: reconstruir bitmap y APPEND de registro fijo
        // -------------------------------------------------------------
        {
            char rutaBloque[MAX_PATH_LEN];
            snprintf(rutaBloque, sizeof(rutaBloque),
                "%sBLOQUES\\Bloque%d.txt",
                discoNuevoPath, nroBloque);

            FILE* fbloc = fopen(rutaBloque, "r+");
            size_t raw_header_len = 0;
            int    numRegAnt = 0;
            int    numMaxAnt = 0;
            char   bitmapAnt[MAX_BUF] = { 0 };

            if (!fbloc) {
                // Bloque no existe: crear y genera cabecera
                char headerBuf[MAX_BUF];
                size_t headerLen;
                int numMax;
                calcularCabeceraBloque(tamBloque, registroSize,
                    headerBuf, &headerLen, &numMax);
                fbloc = fopen(rutaBloque, "wb");
                if (fbloc) {
                    fwrite(headerBuf, 1, headerLen, fbloc);
                    fflush(fbloc);
                    fseek(fbloc, -1, SEEK_END);
                    raw_header_len = headerLen;
                }
                numRegAnt = 0;
                numMaxAnt = numMax;
                for (int i = 0; i < numMax; i++) {
                    bitmapAnt[i] = '0';
                }
                bitmapAnt[numMax] = '\0';
            }
            else {
                // Leer cabecera existente hasta '/'
                size_t pos = 0;
                int c;
                rewind(fbloc);
                while ((c = fgetc(fbloc)) != EOF) {
                    pos++;
                    if (c == '/') break;
                    if (pos >= MAX_BUF - 1) break;
                }
                raw_header_len = pos;
                if (raw_header_len > MAX_BUF - 1) raw_header_len = MAX_BUF - 1;

                rewind(fbloc);
                char cabTmp[MAX_BUF];
                fread(cabTmp, 1, raw_header_len, fbloc);
                cabTmp[raw_header_len] = '\0';

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
                char* slash2 = strchr(p4, '/');
                if (!slash2) { fclose(fbloc); return false; }
                size_t bmpLen = (size_t)(slash2 - p4);
                if (bmpLen >= MAX_BUF) bmpLen = MAX_BUF - 1;
                strncpy(bitmapAnt, p4, bmpLen);
                bitmapAnt[bmpLen] = '\0';
            }

            // 7.4) Ubicar primer bit '0' en bitmapAnt
            int idxLibre = -1;
            for (int i = 0; i < numMaxAnt; i++) {
                if (bitmapAnt[i] == '0') {
                    idxLibre = i;
                    break;
                }
            }
            if (idxLibre < 0) {
                fclose(fbloc);
                return false;
            }

            // 7.5) Actualizar numReg y bitmapAnt
            numRegAnt++;
            bitmapAnt[idxLibre] = '1';

            // 7.6) Reconstruir cabecera nueva EXACTAMENTE raw_header_len bytes
            char newHeader[MAX_BUF];
            int  ofh = 0;
            ofh += snprintf(newHeader + ofh, MAX_BUF - ofh, "%d#%d#", numRegAnt, numMaxAnt);
            for (int i = 0; i < numMaxAnt && ofh < (int)(raw_header_len - 1); i++) {
                newHeader[ofh++] = bitmapAnt[i];
            }
            newHeader[ofh++] = '/';
            while (ofh < (int)raw_header_len) {
                newHeader[ofh++] = ' ';
            }
            newHeader[ofh] = '\0';

            // 7.7) Sobreescribir cabecera
            rewind(fbloc);
            fwrite(newHeader, 1, raw_header_len, fbloc);
            fflush(fbloc);

            // 7.8) APPEND del registro de longitud fija relleno con '#'
            fseek(fbloc, 0, SEEK_END);
            // Crear buffer fijo de tamRegistro bytes
            char registroBuf[registroSize];
            // Llenar todo con '#'
            memset(registroBuf, '#', registroSize);
            // Copiar registroTxt (sin CR/LF al final) al inicio de registroBuf
            size_t lenTxt = strlen(registroTxt);
            while (lenTxt > 0 && (registroTxt[lenTxt - 1] == '\n' || registroTxt[lenTxt - 1] == '\r')) {
                lenTxt--;
            }
            if (lenTxt > (size_t)registroSize - 1) {
                lenTxt = (size_t)registroSize - 1;
            }
            memcpy(registroBuf, registroTxt, lenTxt);
            // Asegurar que el último byte sea '|'
            registroBuf[registroSize - 1] = '|';

            fwrite(registroBuf, 1, registroSize, fbloc);
            fflush(fbloc);
            fclose(fbloc);

            // Volcar bloque a sectores
            this->volcarBloqueASectores(nroBloque);
        }

        // 8) Actualizar catalogo.txt (igual que antes)
        {
            char rutaCatalogo[MAX_PATH_LEN];
            snprintf(rutaCatalogo, sizeof(rutaCatalogo),
                "%s%s", discoNuevoPath, "catalogo.txt");
            FILE* fcat = fopen(rutaCatalogo, "a");
            if (fcat) {
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

    */


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

    /**
     * calcularCabeceraBloque:
     *   - Dado el tamaño del bloque (tamBloque) y el tamaño fijo de cada registro (registroSize),
     *     calcula cuántos registros caben (numMax) teniendo en cuenta el espacio que ocupa la
     *     propia cabecera (“<numReg>#<numMax>#<bitmap>” + '\n').
     *   - Asume que, al crear el bloque, todavía no hay registros (numReg = 0), y que todos los
     *     bits del bitmap deben ser '0' (espacio libre).
     *   - Escribe en bufferHeader (de al menos MAX_BUF bytes) la cadena:
     *        "0#<numMax>#<numMax ceros>\n"
     *   - Devuelve en *outHeaderLen la longitud exacta en bytes de esa línea (incluyendo '\n'),
     *     y en *outNumMax el número calculado de registros máximos.
     *
     *   Restricciones: NO usa memoria dinámica ni STL. Solo buffers estáticos y snprintf/strlen.
     */

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

    bool leerCabeceraBloque(int nroBloque, int* outNumAct, int* outNumMax, char* bitmapStr) {
        char path[MAX_PATH_LEN];
        snprintf(path, MAX_PATH_LEN, "%s/Bloque%d.txt", rutaDirBloques, nroBloque);

        FILE* fblk = fopen(path, "r");
        if (!fblk) return false;

        char linea[MAX_BUF];
        if (!fgets(linea, MAX_BUF, fblk)) {
            fclose(fblk);
            return false;
        }
        linea[strcspn(linea, "\r\n")] = '\0';

        char* tok = strtok(linea, "#");
        if (!tok) { fclose(fblk); return false; }
        *outNumAct = atoi(tok);

        tok = strtok(NULL, "#");
        if (!tok) { fclose(fblk); return false; }
        *outNumMax = atoi(tok);

        tok = strtok(NULL, "#");
        if (!tok) { fclose(fblk); return false; }
        strncpy(bitmapStr, tok, *outNumMax);
        bitmapStr[*outNumMax] = '\0';

        fclose(fblk);
        return true;
    }

    bool escribirCabeceraBloque(int nroBloque, int numAct, int numMax, const char* bitmapStr) {
        char path[MAX_PATH_LEN];
        snprintf(path, MAX_PATH_LEN, "%s/Bloque%d.txt", rutaLongitudFija, nroBloque);

        FILE* fblk = fopen(path, "w");
        if (!fblk) return false;
        fprintf(fblk, "%d#%d#%s\n", numAct, numMax, bitmapStr);
        fclose(fblk);
        return true;
    }

    bool inicializarBloque(int nroBloque, const char* nombreRel) {
        int registroSize = obtenerTamañoRegistro(nombreRel);
        if (registroSize <= 0) {
            fprintf(stderr, "Error: no se encontró '%s' en %s\n", nombreRel, rutaLongitudFija);
            return false;
        }

        int capacidad = tamBloque / registroSize;
        if (capacidad <= 0) {
            fprintf(stderr,
                "Error: registroSize=%d no cabe en bloque de %d B\n",
                registroSize, tamBloque);
            return false;
        }

        char* bitmap = (char*)malloc(capacidad + 1);
        if (!bitmap) return false;
        for (int i = 0; i < capacidad; i++) bitmap[i] = '0';
        bitmap[capacidad] = '\0';

        bool ok = escribirCabeceraBloque(nroBloque, 0, capacidad, bitmap);
        free(bitmap);
        return ok;
    }

    bool adicionarRegistroUnicoBitmap(const char* nombreRel, const char* registroTxt) {
        int registroSize = obtenerTamañoRegistro(nombreRel);
        if (registroSize <= 0) {
            fprintf(stderr, "Error: no se encontró '%s' en %s\n", nombreRel, rutaLongitudFija);
            return false;
        }
        int capacidadPorBloque = tamBloque / registroSize;
        if (capacidadPorBloque <= 0) {
            fprintf(stderr,
                "Error: registro (%d B) no cabe en bloque de %d B\n",
                registroSize, tamBloque);
            return false;
        }

        for (int nroBloque = 1; nroBloque <= tamBloque; nroBloque++) {
            int numAct = 0, numMax = 0;
            char* bitmap = (char*)malloc(capacidadPorBloque + 1);
            if (!bitmap) return false;

            bool existe = leerCabeceraBloque(nroBloque, &numAct, &numMax, bitmap);
            if (!existe) {
                free(bitmap);
                if (!inicializarBloque(nroBloque, nombreRel)) return false;
                bitmap = (char*)malloc(capacidadPorBloque + 1);
                if (!bitmap) return false;
                if (!leerCabeceraBloque(nroBloque, &numAct, &numMax, bitmap)) {
                    free(bitmap);
                    return false;
                }
            }

            if (numAct >= numMax) {
                free(bitmap);
                continue;
            }

            int idxLibre = -1;
            for (int i = 0; i < numMax; i++) {
                if (bitmap[i] == '0') { idxLibre = i; break; }
            }
            if (idxLibre < 0) {
                free(bitmap);
                continue;
            }

            bitmap[idxLibre] = '1';
            numAct++;
            if (!escribirCabeceraBloque(nroBloque, numAct, numMax, bitmap)) {
                free(bitmap);
                return false;
            }
            free(bitmap);

        }

        return false;
    }
};