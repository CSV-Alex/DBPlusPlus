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


    // segunda
    /*
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
            // Asegurar que mida el '\n' si no está presente
            tamRegistro++;
        }

        int  espacioLibreBloque = 0;
        int  tamUtilAntes = 0;
        int  espacioLibreSectorAntes = 0;

        // 2) Recorrer líneas de dirBloques.txt buscando bloque y sector con espacio
        while (fgets(linea, MAX_BUF, fdir)) {
            ++nroBloque;
            posLineaBloque = ftell(fdir) - (long)strlen(linea);

            // --- DEBUG: Print the raw buffer before parsing ---
            printf("[DEBUG] linea buffer (raw): '%s'\n", linea);

            // 2.1) Extraer espacioLibreBloque (primer token antes de '#')
            char copiaBloc[MAX_BUF];
            strncpy(copiaBloc, linea, MAX_BUF);
            copiaBloc[MAX_BUF - 1] = '\0';

            // --- DEBUG: Print the copied buffer before strtok ---
            printf("[DEBUG] copiaBloc before strtok: '%s'\n", copiaBloc);

            char* tokBloc = strtok(copiaBloc, "#");

            // --- DEBUG: Print the token extracted ---
            printf("[DEBUG] tokBloc: '%s'\n", tokBloc ? tokBloc : "NULL");

            if (!tokBloc) continue;
            espacioLibreBloque = safe_atoi(tokBloc);

            // --- DEBUG: Print the parsed value ---
            printf("[DEBUG] espacioLibreBloque parsed: %d\n", espacioLibreBloque);

            tamUtilAntes = tamBloque - espacioLibreBloque;

            // --- DEBUG: Print the calculation result ---
            printf("[DEBUG] tamBloque: %lld, espacioLibreBloque: %d, tamUtilAntes: %d\n",
                tamBloque, espacioLibreBloque, tamUtilAntes);

            printf("[DEBUG] Bloque %d: espacioLibreBloque leído = %d, tamUtilAntes = %d, tamRegistro = %d, tamBloque = %lld\n",
                nroBloque, espacioLibreBloque, tamUtilAntes, tamRegistro, tamBloque);

            if (espacioLibreBloque < tamRegistro) {
                printf("> Bloque %d sin espacio suficiente. Espacio libre bloque: %d bytes; Tamaño del registro: %d bytes\n",
                    nroBloque, espacioLibreBloque, tamRegistro);
                continue;
            }

            // 2.2) Encontrar la primera aparición de "#_" (inicio de la lista de sectores)
            char* p = strstr(linea, "#_");
            if (!p) continue;
            p += 2; // avanzar justo después de "#_"

            // 2.3) Recorremos cada par "<espacioLibreSector>#<código>#_"
            while (*p) {
                // 2.3.1) Leer espacioLibreSector
                char* inicioEspacioSector = p;
                while (*p && *p != '#') p++;
                if (*p != '#') break;
                *p = '\0';
                int espacioLibreSector = atoi(inicioEspacioSector);
                *p = '#';
                p++; // avanzar al código del sector

                // 2.3.2) Extraer códigoSector hasta el siguiente '#'
                char* inicioCodSector = p;
                while (*p && *p != '#') p++;
                if (*p != '#') break;
                *p = '\0';
                char sectorCode[MAX_STR_LEN] = { 0 };
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

        if (espacioBloqueNuevo < 0 || espacioBloqueNuevo > tamBloque) {
            printf("[ERROR] Valor de espacioBloqueNuevo fuera de rango: %d (tamBloque=%lld, tamUtilNuevo=%d)\n",
                espacioBloqueNuevo, tamBloque, tamUtilNuevo);
            printf("[ERROR] Detalle: espacioLibreBloqueAntes=%d, tamRegistro=%d, tamUtilAntes=%d\n",
                espacioLibreBloqueAntes, tamRegistro, tamUtilAntes);
        }

        printf("[DEBUG] Bloque %d: espacioLibreBloqueAntes = %d, tamUtilNuevo = %d, espacioBloqueNuevo = %d\n",
            nroBloque, espacioLibreBloqueAntes, tamUtilNuevo, espacioBloqueNuevo);

        // 3) Actualizar la línea en dirBloques.txt: restar tamRegistro de bloque y sector
        fseek(fdir, posLineaBloque, SEEK_SET);
        fgets(linea, MAX_BUF, fdir);
        linea[strcspn(linea, "\r\n")] = '\0';

        // 3.1) Reconstruir línea completa en un buffer nuevo, sin depender de la longitud anterior
        char* inicioSectores = strstr(linea, "#_");
        if (!inicioSectores) {
            fclose(fdir);
            return false;
        }

        char bufferLineaNueva[MAX_BUF];
        int  ofs = 0;

        // 3.1.1) Escribir el nuevo espacioLibreBloque y campos fijos
        ofs += snprintf(bufferLineaNueva + ofs, MAX_BUF - ofs,
            "%d#2#BLOQUE#%d#%d#_",
            espacioBloqueNuevo,
            nroBloque,
            tamBloque
        );

        // 3.1.2) Ajustar cada par de sectores
        {
            char* psec2 = inicioSectores + 2; // justo después de "#_"
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

        // 3.1.3) Agregar salto de línea
        if (ofs < MAX_BUF - 1) {
            bufferLineaNueva[ofs++] = '\n';
            bufferLineaNueva[ofs] = '\0';
        }
        else {
            bufferLineaNueva[MAX_BUF - 1] = '\n';
            bufferLineaNueva[MAX_BUF - 0] = '\0';
        }

        printf("[DEBUG] posLineaBloque = %ld\n", posLineaBloque);
        printf("[DEBUG] linea original: '%s'\n", linea);
        printf("[DEBUG] bufferLineaNueva antes de rellenar: '%s'\n", bufferLineaNueva);
        printf("[DEBUG] len_original = %zu, len_nueva = %zu\n", strlen(linea), strlen(bufferLineaNueva));

        // --- BLOQUE PARA RELLENAR LA LÍNEA NUEVA SI ES MÁS CORTA QUE LA ORIGINAL ---
        size_t len_original = strlen(linea); // 'linea' ya sin \r\n
        size_t len_nueva = strlen(bufferLineaNueva);

        // Si la nueva línea es más corta, rellena con espacios
// Rellenar con espacios o truncar correctamente
// Siempre rellena hasta len_original-1 y termina con '\n'
        if (len_nueva < len_original) {
            memset(bufferLineaNueva + len_nueva, ' ', len_original - len_nueva - 1);
            bufferLineaNueva[len_original - 1] = '\n';
            bufferLineaNueva[len_original] = '\0';
        }
        else if (len_nueva > len_original) {
            // Truncar la nueva línea para que no sobrescriba la siguiente
            bufferLineaNueva[len_original - 1] = '\n';
            bufferLineaNueva[len_original] = '\0';
        }
        else {
            // Si son iguales, asegúrate de terminar con '\n'
            if (bufferLineaNueva[len_original - 1] != '\n') {
                bufferLineaNueva[len_original - 1] = '\n';
            }
        }

        // Después de rellenar
        printf("[DEBUG] bufferLineaNueva después de rellenar: '%s'\n", bufferLineaNueva);

        fseek(fdir, posLineaBloque, SEEK_SET);
        fwrite(bufferLineaNueva, 1, len_original, fdir);

        // Después de escribir
        long posDespuesFputs = ftell(fdir);
        printf("[DEBUG] ftell después de fputs: %ld\n", posDespuesFputs);

        fclose(fdir);

        printf("[DEBUG] Bloque %d: línea actualizada en dirBloques.txt: '%s'\n",
            nroBloque, bufferLineaNueva);

        // 4) Actualizar cabecera de BloqueN.txt
        char rutaBloqueFis[MAX_PATH_LEN];
        rutaBloqueFisico(nroBloque, rutaBloqueFis);
        FILE* fbloc = fopen(rutaBloqueFis, "r+");
        if (!fbloc) {
            perror("No se pudo abrir BloqueN.txt para actualización");
            return false;
        }
        long posBlocLinea = ftell(fbloc);
        char lineaBloc[MAX_BUF];
        fgets(lineaBloc, MAX_BUF, fbloc);
        lineaBloc[strcspn(lineaBloc, "\r\n")] = '\0';

        // Extraer espacioLibreBloqueActual de la líneaBloque
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
        // Ajustar lista de sectores en el bloque físico
        char sectoresModBloc[MAX_BUF] = { 0 };
        char* psec2b = inicioSBloc + 2;
        while (*psec2b) {
            int espSec = atoi(psec2b);
            while (*psec2b && *psec2b != '#') ++psec2b;
            if (!*psec2b) break;
            ++psec2b;

            char sectorCode2b[MAX_STR_LEN] = { 0 };
            int posb = 0;
            while (*psec2b && *psec2b != '#') {
                sectorCode2b[posb++] = *psec2b++;
            }
            sectorCode2b[posb] = '\0';

            int nuevoEspSec2b = espSec;
            if (strcmp(sectorCode2b, codSectorLibre) == 0) {
                nuevoEspSec2b = espSec - tamRegistro;
            }

            char bufferPar2[64];
            snprintf(bufferPar2, sizeof(bufferPar2), "%d#%s#_", nuevoEspSec2b, sectorCode2b);
            strncat(sectoresModBloc, bufferPar2, sizeof(sectoresModBloc) - strlen(sectoresModBloc) - 1);

            char* next2b = strstr(psec2b, "#_");
            if (!next2b) break;
            psec2b = next2b + 2;
        }

        // Reconstruir la línea dentro de BloqueN.txt
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

        //// 5) Escribir registro en sector físico
        //rutaSectorDesdeCodigo(codSectorLibre);
        //FILE* fsec = fopen(rutaSectorDesdeCodigo(codSectorLibre), "a");
        //if (!fsec) {
        //    perror("No se pudo abrir sector para escribir");
        //    return false;
        //}
        //int espacioLibreSectorDesp = espacioLibreSectorAntes - tamRegistro;

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
        printf("   Espacio libre bloque antes: %d bytes; después: %d bytes\n", espacioLibreBloqueAntes, espacioBloqueNuevo);
        printf("   Espacio libre sector antes: %d bytes; después: %d bytes\n", espacioLibreSectorAntes, espacioLibreSectorDesp);

        fprintf(fsec, "%s", registroTxt);
        fclose(fsec);

        return true;
    }
    */


bool adicionarRegistroUniconoseusa(const char* registroTxt, const char* relacion) {
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

    //// 3) Actualizar la linea en dirBloques.txt: restar tamRegistro de bloque y sector
    //fseek(fdir, posLineaBloque, SEEK_SET);
    //fgets(linea, MAX_BUF, fdir);
    //linea[strcspn(linea, "\r\n")] = '\0';

    // 3) Volver a la posición donde empieza la línea encontrada
    fseek(fdir, posLineaBloque, SEEK_SET);

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


    //fseek(fdir, posLineaBloque, SEEK_SET);
    //fputs(bufferLineaNueva, fdir);
    //fclose(fdir);

    // 3) Volver a la posición donde empieza la línea encontrada
    fseek(fdir, posLineaBloque, SEEK_SET);

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


bool adicionarRegistroUnicobackuppp(const char* registroTxt, const char* relacion) {
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
        tamRegistro++;
    }

    int  espacioLibreBloque = 0;
    int  tamUtilAntes = 0;
    int  espacioLibreSectorAntes = 0;

    // 2) Recorrer líneas de dirBloques.txt buscando bloque y sector con espacio
    while (1) {
        posLineaBloque = ftell(fdir);
        if (!fgets(linea, MAX_BUF, fdir)) break;
        nroBloque++;

        // --- Eliminar '\n' y luego '\r' al final, si existen ---
        size_t len_linea = strlen(linea);
        if (len_linea > 0 && linea[len_linea - 1] == '\n') {
            linea[--len_linea] = '\0';
        }
        if (len_linea > 0 && linea[len_linea - 1] == '\r') {
            linea[--len_linea] = '\0';
        }

        // 2.1) Extraer espacioLibreBloque
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

        // 2.2) Encontrar "#_" en la línea original
        char* p = strstr(linea, "#_");
        if (!p) continue;
        // Para modificar, copiamos linea en buffer mutable
        strncpy(copiaBloc, linea, MAX_BUF);
        copiaBloc[MAX_BUF - 1] = '\0';
        p = strstr(copiaBloc, "#_");
        if (!p) continue;
        p += 2;

        // 2.3) Recorrer pares "<espacioLibreSector>#<código>#_"
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
        // Valor inesperado; pero continuamos a efectos de debug
    }

    // 3) Volver a la posición donde empieza la línea encontrada
    fseek(fdir, posLineaBloque, SEEK_SET);


    // 3.1) Leer de nuevo la línea original para conocer su longitud real en bytes
    if (!fgets(linea, MAX_BUF, fdir)) {
        fclose(fdir);
        return false;
    }
    // Quitar '\n' y '\r'
    size_t len_original = strlen(linea);
    if (len_original > 0 && linea[len_original - 1] == '\n') {
        linea[--len_original] = '\0';
    }
    if (len_original > 0 && linea[len_original - 1] == '\r') {
        linea[--len_original] = '\0';
    }
    // En disco, esa línea ocupaba len_original (texto) + 1 byte de '\n'
    len_original += 1;

    // 3.2) Construir la nueva línea (sin '\r', sólo un '\n' al final)
    char* bufferNueva = (char*)malloc(MAX_BUF);
    if (!bufferNueva) {
        fclose(fdir);
        return false;
    }
    int ofs = 0;
    ofs += snprintf(bufferNueva + ofs, MAX_BUF - ofs,
        "%d#2#BLOQUE#%d#%d#_",
        espacioBloqueNuevo,
        nroBloque,
        tamBloque);

    // Reconstruir la parte de sectores sobre copia de linea
    {
        char copia2[MAX_BUF];
        strncpy(copia2, linea, MAX_BUF);
        copia2[MAX_BUF - 1] = '\0';
        char* inicioSect = strstr(copia2, "#_");
        if (inicioSect) {
            char* p2 = inicioSect + 2;
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
                ofs += snprintf(bufferNueva + ofs, MAX_BUF - ofs,
                    "%d#%s#_",
                    nuevoEsp,
                    sectorCode2);
                char* next2 = strstr(p2, "#_");
                if (!next2) break;
                p2 = next2 + 2;
            }
        }
    }

    // Agregar '\n' al final
    if (ofs < MAX_BUF - 1) {
        bufferNueva[ofs++] = '\n';
        bufferNueva[ofs] = '\0';
    }
    else {
        bufferNueva[MAX_BUF - 1] = '\n';
        bufferNueva[MAX_BUF] = '\0';
    }

    // 3.3) Ajustar longitud para que sea EXACTAMENTE len_original bytes
    size_t len_nueva = strlen(bufferNueva);
    if (len_nueva > 0 && linea[len_nueva - 1] == '\n') {
        linea[--len_nueva] = '\0';    // ahora linea = "180#2#BLOQUE#1#180#_\r"
    }
    if (len_nueva > 0 && linea[len_nueva - 1] == '\r') {
        linea[--len_nueva] = '\0';    // ahora linea = "180#2#BLOQUE#1#180#_"
    }
    // else: (len_nueva == len_original) ya termina en '\n'

    // 3.4) Sobreescribir exactamente len_original bytes
    fseek(fdir, posLineaBloque, SEEK_SET);
    fwrite(bufferNueva, 1, len_original, fdir);

    fflush(fdir);
    free(bufferNueva);
    fclose(fdir);

    // 4) Actualizar cabecera de BloqueN.txt (misma lógica que antes)
    char rutaBloqueFis[MAX_PATH_LEN];
    rutaBloqueFisico(nroBloque, rutaBloqueFis);
    FILE* fbloc = fopen(rutaBloqueFis, "r+");
    if (!fbloc) {
        perror("No se pudo abrir BloqueN.txt para actualización");
        return false;
    }
    long posBlocLinea = ftell(fbloc);
    char lineaBloc[MAX_BUF];
    fgets(lineaBloc, MAX_BUF, fbloc);
    lineaBloc[strcspn(lineaBloc, "\r\n")] = '\0';

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
    char sectoresModBloc[MAX_BUF] = { 0 };
    char* psec2b = inicioSBloc + 2;
    while (*psec2b) {
        int espSec = atoi(psec2b);
        while (*psec2b && *psec2b != '#') ++psec2b;
        if (!*psec2b) break;
        ++psec2b;

        char sectorCode2b[MAX_STR_LEN] = { 0 };
        int posb = 0;
        while (*psec2b && *psec2b != '#') {
            sectorCode2b[posb++] = *psec2b++;
        }
        sectorCode2b[posb] = '\0';

        int nuevoEspSec2b = espSec;
        if (strcmp(sectorCode2b, codSectorLibre) == 0) {
            nuevoEspSec2b = espSec - tamRegistro;
        }

        char bufferPar2[64];
        snprintf(bufferPar2, sizeof(bufferPar2), "%d#%s#_",
            nuevoEspSec2b, sectorCode2b);
        strncat(sectoresModBloc, bufferPar2,
            sizeof(sectoresModBloc) - strlen(sectoresModBloc) - 1);

        char* next2b = strstr(psec2b, "#_");
        if (!next2b) break;
        psec2b = next2b + 2;
    }

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

    // 5) Escribir registro en sector físico
    rutaSectorDesdeCodigo(codSectorLibre);
    FILE* fsec = fopen(bufferRuta, "a");
    if (!fsec) {
        perror("No se pudo abrir sector para escribir");
        return false;
    }
    int espacioLibreSectorDesp = espacioLibreSectorAntes - tamRegistro;

    int pl, su, pi, se;
    sscanf(codSectorLibre, "%d/%d/%d/%d", &pl, &su, &pi, &se);

    printf("-> Insertando registro en Plato %d, Superficie %d, Pista %d, Sector %d\n",
        pl, su, pi, se);
    printf("   Espacio libre bloque antes: %d bytes; después: %d bytes\n",
        espacioLibreBloqueAntes, espacioBloqueNuevo);
    printf("   Espacio libre sector antes: %d bytes; después: %d bytes\n",
        espacioLibreSectorAntes, espacioLibreSectorDesp);

    fprintf(fsec, "%s", registroTxt);
    fclose(fsec);

    return true;
}


bool adicionarRegistroUnicoEstaBien(const char* registroTxt, const char* relacion) {
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
        tamRegistro++;
    }

    int  espacioLibreBloque = 0;
    int  tamUtilAntes = 0;
    int  espacioLibreSectorAntes = 0;

    // 2) Recorrer líneas de dirBloques.txt buscando bloque y sector con espacio
    while (1) {
        posLineaBloque = ftell(fdir);
        if (!fgets(linea, MAX_BUF, fdir)) break;
        nroBloque++;

        // --- Eliminar '\n' y luego '\r' al final, si existen ---
        size_t len_linea = strlen(linea);
        if (len_linea > 0 && linea[len_linea - 1] == '\n') {
            linea[--len_linea] = '\0';
        }
        if (len_linea > 0 && linea[len_linea - 1] == '\r') {
            linea[--len_linea] = '\0';
        }

        // 2.1) Extraer espacioLibreBloque
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

        // 2.2) Encontrar "#_" en la línea original
        char* p = strstr(linea, "#_");
        if (!p) continue;
        // Para modificar, copiamos linea en buffer mutable
        strncpy(copiaBloc, linea, MAX_BUF);
        copiaBloc[MAX_BUF - 1] = '\0';
        p = strstr(copiaBloc, "#_");
        if (!p) continue;
        p += 2;

        // 2.3) Recorrer pares "<espacioLibreSector>#<código>#_"
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
        // Valor inesperado; pero continuamos a efectos de debug
    }

    // 3) Volver a la posición donde empieza la línea encontrada
    fseek(fdir, posLineaBloque, SEEK_SET);


    // 3.1) Leer de nuevo la línea original para conocer su longitud real en bytes
    if (!fgets(linea, MAX_BUF, fdir)) {
        fclose(fdir);
        return false;
    }

    /*
    // Quitar '\n' y '\r'
    size_t len_original = strlen(linea);
    if (len_original > 0 && linea[len_original - 1] == '\n') {
        linea[--len_original] = '\0';
    }
    if (len_original > 0 && linea[len_original - 1] == '\r') {
        linea[--len_original] = '\0';
    }
    // En disco, esa línea ocupaba len_original (texto) + 1 byte de '\n'
    len_original += 1;
    */

    size_t raw_len = strlen(linea);  // incluye '\n' y quizá '\r'
    // Quitar '\n' y '\r' para obtener la parte de texto
    if (raw_len > 0 && linea[raw_len - 1] == '\n') {
        raw_len--;
    }
    if (raw_len > 0 && linea[raw_len - 1] == '\r') {
        raw_len--;
    }
    // En disco, la línea antigua ocupaba raw_len + (posiblemente) 2 bytes de CRLF.
    // Pero strlen(linea) contaba ambos antes de quitar. Así raw_len_original = strlen(linea_with_CRLF).
    raw_len = strlen(linea) + ((linea[raw_len] == '\r') ? 2 : 1);
    // Para simplificar asumimos 2 bytes CRLF: raw_len = strlen(linea) + 2.
    // Si no hay '\r', strlen incluía solo '\n', añadimos 1.
    // Podemos recontar:
    size_t len1 = strlen(linea);
    if (linea[len1] == '\r') {
        raw_len = len1 + 2;
    }
    else {
        raw_len = len1 + 1;
    }


    // 3.2) Construir la nueva línea (sin '\r', sólo un '\n' al final)
    char* bufferNueva = (char*)malloc(MAX_BUF);
    if (!bufferNueva) {
        fclose(fdir);
        return false;
    }
    int ofs = 0;
    ofs += snprintf(bufferNueva + ofs, MAX_BUF - ofs,
        "%d#2#BLOQUE#%d#%d#_",
        espacioBloqueNuevo,
        nroBloque,
        tamBloque);

    // Reconstruir la parte de sectores sobre copia de linea
    {
        char copia2[MAX_BUF];
        strncpy(copia2, linea, MAX_BUF);
        copia2[MAX_BUF - 1] = '\0';
        char* inicioSect = strstr(copia2, "#_");
        if (inicioSect) {
            char* p2 = inicioSect + 2;
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
                ofs += snprintf(bufferNueva + ofs, MAX_BUF - ofs,
                    "%d#%s#_",
                    nuevoEsp,
                    sectorCode2);
                char* next2 = strstr(p2, "#_");
                if (!next2) break;
                p2 = next2 + 2;
            }
        }
    }

    /*
    //// Agregar '\n' al final
    //if (ofs < MAX_BUF - 1) {
    //    bufferNueva[ofs++] = '\n';
    //    bufferNueva[ofs] = '\0';
    //}
    //else {
    //    bufferNueva[MAX_BUF - 1] = '\n';
    //    bufferNueva[MAX_BUF] = '\0';
    //}

    //// 3.3) Ajustar longitud para que sea EXACTAMENTE len_original bytes
    //size_t len_nueva = strlen(bufferNueva);
    //if (len_nueva > 0 && linea[len_nueva - 1] == '\n') {
    //    linea[--len_nueva] = '\0';    // ahora linea = "180#2#BLOQUE#1#180#_\r"
    //}
    //if (len_nueva > 0 && linea[len_nueva - 1] == '\r') {
    //    linea[--len_nueva] = '\0';    // ahora linea = "180#2#BLOQUE#1#180#_"
    //}
    //// else: (len_nueva == len_original) ya termina en '\n'
    */


    //esto modifique
    // 3.3) Ajustar para que bufferNueva ocupe exactamente raw_len bytes (incluyendo CRLF)
    int parteUtil = ofs;
    if (parteUtil > (int)raw_len - 2) {
        // truncar a raw_len-2 y luego CRLF
        bufferNueva[raw_len - 2] = '\r';
        bufferNueva[raw_len - 1] = '\n';
        bufferNueva[raw_len] = '\0';
    }
    else {
        int i;
        for (i = parteUtil; i < (int)raw_len - 2; i++) {
            bufferNueva[i] = ' ';
        }
        bufferNueva[raw_len - 2] = '\r';
        bufferNueva[raw_len - 1] = '\n';
        bufferNueva[raw_len] = '\0';
    }

    size_t len_original = strlen(linea);
    // 3.4) Sobreescribir exactamente len_original bytes
    fseek(fdir, posLineaBloque, SEEK_SET);
    fwrite(bufferNueva, 1, len_original, fdir);

    fflush(fdir);
    free(bufferNueva);
    fclose(fdir);

    // 4) Actualizar cabecera de BloqueN.txt (misma lógica que antes)
    char rutaBloqueFis[MAX_PATH_LEN];
    rutaBloqueFisico(nroBloque, rutaBloqueFis);
    FILE* fbloc = fopen(rutaBloqueFis, "r+");
    if (!fbloc) {
        perror("No se pudo abrir BloqueN.txt para actualización");
        return false;
    }
    long posBlocLinea = ftell(fbloc);
    char lineaBloc[MAX_BUF];
    fgets(lineaBloc, MAX_BUF, fbloc);
    lineaBloc[strcspn(lineaBloc, "\r\n")] = '\0';

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
    char sectoresModBloc[MAX_BUF] = { 0 };
    char* psec2b = inicioSBloc + 2;
    while (*psec2b) {
        int espSec = atoi(psec2b);
        while (*psec2b && *psec2b != '#') ++psec2b;
        if (!*psec2b) break;
        ++psec2b;

        char sectorCode2b[MAX_STR_LEN] = { 0 };
        int posb = 0;
        while (*psec2b && *psec2b != '#') {
            sectorCode2b[posb++] = *psec2b++;
        }
        sectorCode2b[posb] = '\0';

        int nuevoEspSec2b = espSec;
        if (strcmp(sectorCode2b, codSectorLibre) == 0) {
            nuevoEspSec2b = espSec - tamRegistro;
        }

        char bufferPar2[64];
        snprintf(bufferPar2, sizeof(bufferPar2), "%d#%s#_",
            nuevoEspSec2b, sectorCode2b);
        strncat(sectoresModBloc, bufferPar2,
            sizeof(sectoresModBloc) - strlen(sectoresModBloc) - 1);

        char* next2b = strstr(psec2b, "#_");
        if (!next2b) break;
        psec2b = next2b + 2;
    }

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

    // 5) Escribir registro en sector físico
    rutaSectorDesdeCodigo(codSectorLibre);
    FILE* fsec = fopen(bufferRuta, "a");
    if (!fsec) {
        perror("No se pudo abrir sector para escribir");
        return false;
    }
    int espacioLibreSectorDesp = espacioLibreSectorAntes - tamRegistro;

    int pl, su, pi, se;
    sscanf(codSectorLibre, "%d/%d/%d/%d", &pl, &su, &pi, &se);

    printf("-> Insertando registro en Plato %d, Superficie %d, Pista %d, Sector %d\n",
        pl, su, pi, se);
    printf("   Espacio libre bloque antes: %d bytes; después: %d bytes\n",
        espacioLibreBloqueAntes, espacioBloqueNuevo);
    printf("   Espacio libre sector antes: %d bytes; después: %d bytes\n",
        espacioLibreSectorAntes, espacioLibreSectorDesp);

    fprintf(fsec, "%s", registroTxt);
    fclose(fsec);


    // 6) VOLCAR/INSERTAR el mismo registro en “DISCO\\BLOQUES\\BloqueN.txt”
    char rutaBloque[MAX_PATH_LEN];
    snprintf(rutaBloque, sizeof(rutaBloque),
        "%sBLOQUES\\Bloque%d.txt",
        discoNuevoPath, nroBloque);
    FILE* fblocAppend = fopen(rutaBloque, "ab");
    if (fblocAppend) {
        fwrite(registroTxt, 1, tamRegistro, fblocAppend);
        fclose(fblocAppend);
    }

    // 7) MODIFICAR catalogo.txt: agregar “relacion|rutaBloque\n”
    char rutaCatalogo[MAX_PATH_LEN];
    snprintf(rutaCatalogo, sizeof(rutaCatalogo),
        "%s%s", discoNuevoPath, "catalogo.txt");
    FILE* fcat = fopen(rutaCatalogo, "a");
    if (fcat) {
        fprintf(fcat, "%s|%s\n", relacion, rutaBloque);
        fclose(fcat);
    }


    return true;
}

/*
void calcularLongitudFija(const char* rutaCSV) {
    FILE* fcsv = fopen(rutaCSV, "r");
    if (!fcsv) {
        perror("No se puede abrir el CSV para calcular longitudes fijas");
        return;
    }

    std::cout << rutaCSV << std::endl;

    char linea[MAX_BUF];
    if (!fgets(linea, MAX_BUF, fcsv)) {
        fclose(fcsv);
        return;
    }
    // Eliminar CRLF al final
    linea[strcspn(linea, "\r\n")] = '\0';

    // Contar comas (o el delimitador que uses) para determinar numFields
    int numFields = 1;
    for (char* p = linea; *p; ++p) {
        if (*p == ',') numFields++;
    }
    if (numFields < 1) numFields = 1;
    if (numFields > MAX_FIELDS) numFields = MAX_FIELDS;

    // Array estático para guardar la máxima longitud de cada campo
    int maxLen[MAX_FIELDS] = { 0 };

    // Procesar primera línea
    {
        char copy[MAX_BUF];
        strncpy(copy, linea, MAX_BUF);
        copy[MAX_BUF - 1] = '\0';
        char* tok = strtok(copy, ",");
        int idx = 0;
        while (tok && idx < numFields) {
            int len = (int)strlen(tok);
            if (len > maxLen[idx]) maxLen[idx] = len;
            idx++;
            tok = strtok(NULL, ",");
        }
    }

    // Procesar el resto
    while (fgets(linea, MAX_BUF, fcsv)) {
        linea[strcspn(linea, "\r\n")] = '\0';
        char copy[MAX_BUF];
        strncpy(copy, linea, MAX_BUF);
        copy[MAX_BUF - 1] = '\0';
        char* tok = strtok(copy, ",");
        int idx = 0;
        while (tok && idx < numFields) {
            int len = (int)strlen(tok);
            if (len > maxLen[idx]) maxLen[idx] = len;
            idx++;
            tok = strtok(NULL, ",");
        }
    }
    fclose(fcsv);

    // Obtener nombre de la relación (sin ruta ni ".csv")
    const char* slash = strrchr(rutaCSV, '/');
    const char* backslash = strrchr(rutaCSV, '\\');
    const char* fname = slash ? slash + 1 : (backslash ? backslash + 1 : rutaCSV);
    char nombreRel[MAX_STR_LEN];
    strncpy(nombreRel, fname, MAX_STR_LEN - 1);
    nombreRel[MAX_STR_LEN - 1] = '\0';
    char* ext = strstr(nombreRel, ".csv");
    if (ext) *ext = '\0';

    // Escribir en longitudFija.txt
    FILE* flog = fopen(rutaLongitudFija, "a");
    if (!flog) {
        perror("No se puede abrir longitudFija.txt");
        return;
    }
    // Formato: <nombre_relacion>|<numFields>#<maxLen1>#<maxLen2>#...#<maxLenN>\n
    fprintf(flog, "%s|%d", nombreRel, numFields);
    for (int i = 0; i < numFields; i++) {
        fprintf(flog, "#%d", maxLen[i]);
    }
    fprintf(flog, "\n");
    fclose(flog);
}
*/

bool adicionarRegistroUnicoVersionFinal(const char* registroTxt, const char* relacion) {
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
        tamRegistro++;
    }

    int  espacioLibreBloque = 0;
    int  tamUtilAntes = 0;
    int  espacioLibreSectorAntes = 0;

    // 2) Recorrer líneas de dirBloques.txt buscando bloque y sector con espacio
    while (1) {
        posLineaBloque = ftell(fdir);
        if (!fgets(linea, MAX_BUF, fdir)) break;
        nroBloque++;

        // --- Eliminar '\n' y luego '\r' al final, si existen ---
        size_t len_linea = strlen(linea);
        if (len_linea > 0 && linea[len_linea - 1] == '\n') {
            linea[--len_linea] = '\0';
        }
        if (len_linea > 0 && linea[len_linea - 1] == '\r') {
            linea[--len_linea] = '\0';
        }

        // 2.1) Extraer espacioLibreBloque
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

        // 2.2) Encontrar "#_" en la línea original
        char* p = strstr(linea, "#_");
        if (!p) continue;
        // Para modificar, copiamos linea en buffer mutable
        strncpy(copiaBloc, linea, MAX_BUF);
        copiaBloc[MAX_BUF - 1] = '\0';
        p = strstr(copiaBloc, "#_");
        if (!p) continue;
        p += 2;

        // 2.3) Recorrer pares "<espacioLibreSector>#<código>#_"
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
        // Valor inesperado; pero continuamos a efectos de debug
    }

    // 3) Volver a la posición donde empieza la línea encontrada
    fseek(fdir, posLineaBloque, SEEK_SET);


    // 3.1) Leer de nuevo la línea original para conocer su longitud real en bytes
    if (!fgets(linea, MAX_BUF, fdir)) {
        fclose(fdir);
        return false;
    }

    /*
    // Quitar '\n' y '\r'
    size_t len_original = strlen(linea);
    if (len_original > 0 && linea[len_original - 1] == '\n') {
        linea[--len_original] = '\0';
    }
    if (len_original > 0 && linea[len_original - 1] == '\r') {
        linea[--len_original] = '\0';
    }
    // En disco, esa línea ocupaba len_original (texto) + 1 byte de '\n'
    len_original += 1;
    */

    size_t raw_len = strlen(linea);  // incluye '\n' y quizá '\r'
    // Quitar '\n' y '\r' para obtener la parte de texto
    if (raw_len > 0 && linea[raw_len - 1] == '\n') {
        raw_len--;
    }
    if (raw_len > 0 && linea[raw_len - 1] == '\r') {
        raw_len--;
    }
    // En disco, la línea antigua ocupaba raw_len + (posiblemente) 2 bytes de CRLF.
    // Pero strlen(linea) contaba ambos antes de quitar. Así raw_len_original = strlen(linea_with_CRLF).
    raw_len = strlen(linea) + ((linea[raw_len] == '\r') ? 2 : 1);
    // Para simplificar asumimos 2 bytes CRLF: raw_len = strlen(linea) + 2.
    // Si no hay '\r', strlen incluía solo '\n', añadimos 1.
    // Podemos recontar:
    size_t len1 = strlen(linea);
    if (linea[len1] == '\r') {
        raw_len = len1 + 2;
    }
    else {
        raw_len = len1 + 1;
    }


    // 3.2) Construir la nueva línea (sin '\r', sólo un '\n' al final)
    char* bufferNueva = (char*)malloc(MAX_BUF);
    if (!bufferNueva) {
        fclose(fdir);
        return false;
    }
    int ofs = 0;
    ofs += snprintf(bufferNueva + ofs, MAX_BUF - ofs,
        "%d#2#BLOQUE#%d#%d#_",
        espacioBloqueNuevo,
        nroBloque,
        tamBloque);

    // Reconstruir la parte de sectores sobre copia de linea
    {
        char copia2[MAX_BUF];
        strncpy(copia2, linea, MAX_BUF);
        copia2[MAX_BUF - 1] = '\0';
        char* inicioSect = strstr(copia2, "#_");
        if (inicioSect) {
            char* p2 = inicioSect + 2;
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
                ofs += snprintf(bufferNueva + ofs, MAX_BUF - ofs,
                    "%d#%s#_",
                    nuevoEsp,
                    sectorCode2);
                char* next2 = strstr(p2, "#_");
                if (!next2) break;
                p2 = next2 + 2;
            }
        }
    }

    /*
    //// Agregar '\n' al final
    //if (ofs < MAX_BUF - 1) {
    //    bufferNueva[ofs++] = '\n';
    //    bufferNueva[ofs] = '\0';
    //}
    //else {
    //    bufferNueva[MAX_BUF - 1] = '\n';
    //    bufferNueva[MAX_BUF] = '\0';
    //}

    //// 3.3) Ajustar longitud para que sea EXACTAMENTE len_original bytes
    //size_t len_nueva = strlen(bufferNueva);
    //if (len_nueva > 0 && linea[len_nueva - 1] == '\n') {
    //    linea[--len_nueva] = '\0';    // ahora linea = "180#2#BLOQUE#1#180#_\r"
    //}
    //if (len_nueva > 0 && linea[len_nueva - 1] == '\r') {
    //    linea[--len_nueva] = '\0';    // ahora linea = "180#2#BLOQUE#1#180#_"
    //}
    //// else: (len_nueva == len_original) ya termina en '\n'
    */


    //esto modifique
    // 3.3) Ajustar para que bufferNueva ocupe exactamente raw_len bytes (incluyendo CRLF)
    int parteUtil = ofs;
    if (parteUtil > (int)raw_len - 2) {
        // truncar a raw_len-2 y luego CRLF
        bufferNueva[raw_len - 2] = '\r';
        bufferNueva[raw_len - 1] = '\n';
        bufferNueva[raw_len] = '\0';
    }
    else {
        int i;
        for (i = parteUtil; i < (int)raw_len - 2; i++) {
            bufferNueva[i] = ' ';
        }
        bufferNueva[raw_len - 2] = '\r';
        bufferNueva[raw_len - 1] = '\n';
        bufferNueva[raw_len] = '\0';
    }

    size_t len_original = strlen(linea);
    // 3.4) Sobreescribir exactamente len_original bytes
    fseek(fdir, posLineaBloque, SEEK_SET);
    fwrite(bufferNueva, 1, len_original, fdir);

    fflush(fdir);
    free(bufferNueva);
    fclose(fdir);

    // 4) Actualizar cabecera de BloqueN.txt (misma lógica que antes)
    char rutaBloqueFis[MAX_PATH_LEN];
    rutaBloqueFisico(nroBloque, rutaBloqueFis);
    FILE* fbloc = fopen(rutaBloqueFis, "r+");
    if (!fbloc) {
        perror("No se pudo abrir BloqueN.txt para actualización");
        return false;
    }
    long posBlocLinea = ftell(fbloc);
    char lineaBloc[MAX_BUF];
    fgets(lineaBloc, MAX_BUF, fbloc);
    lineaBloc[strcspn(lineaBloc, "\r\n")] = '\0';

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
    char sectoresModBloc[MAX_BUF] = { 0 };
    char* psec2b = inicioSBloc + 2;
    while (*psec2b) {
        int espSec = atoi(psec2b);
        while (*psec2b && *psec2b != '#') ++psec2b;
        if (!*psec2b) break;
        ++psec2b;

        char sectorCode2b[MAX_STR_LEN] = { 0 };
        int posb = 0;
        while (*psec2b && *psec2b != '#') {
            sectorCode2b[posb++] = *psec2b++;
        }
        sectorCode2b[posb] = '\0';

        int nuevoEspSec2b = espSec;
        if (strcmp(sectorCode2b, codSectorLibre) == 0) {
            nuevoEspSec2b = espSec - tamRegistro;
        }

        char bufferPar2[64];
        snprintf(bufferPar2, sizeof(bufferPar2), "%d#%s#_",
            nuevoEspSec2b, sectorCode2b);
        strncat(sectoresModBloc, bufferPar2,
            sizeof(sectoresModBloc) - strlen(sectoresModBloc) - 1);

        char* next2b = strstr(psec2b, "#_");
        if (!next2b) break;
        psec2b = next2b + 2;
    }

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

    // 5) Escribir registro en sector físico
    rutaSectorDesdeCodigo(codSectorLibre);
    FILE* fsec = fopen(bufferRuta, "a");
    if (!fsec) {
        perror("No se pudo abrir sector para escribir");
        return false;
    }
    int espacioLibreSectorDesp = espacioLibreSectorAntes - tamRegistro;

    int pl, su, pi, se;
    sscanf(codSectorLibre, "%d/%d/%d/%d", &pl, &su, &pi, &se);

    printf("-> Insertando registro en Plato %d, Superficie %d, Pista %d, Sector %d\n",
        pl, su, pi, se);
    printf("   Espacio libre bloque antes: %d bytes; después: %d bytes\n",
        espacioLibreBloqueAntes, espacioBloqueNuevo);
    printf("   Espacio libre sector antes: %d bytes; después: %d bytes\n",
        espacioLibreSectorAntes, espacioLibreSectorDesp);

    fprintf(fsec, "%s", registroTxt);
    fclose(fsec);


    // 6) VOLCAR/INSERTAR el mismo registro en “DISCO\\BLOQUES\\BloqueN.txt”
    char rutaBloque[MAX_PATH_LEN];
    snprintf(rutaBloque, sizeof(rutaBloque),
        "%sBLOQUES\\Bloque%d.txt",
        discoNuevoPath, nroBloque);
    FILE* fblocAppend = fopen(rutaBloque, "ab");
    if (fblocAppend) {
        fwrite(registroTxt, 1, tamRegistro, fblocAppend);
        fclose(fblocAppend);
    }

    // 7) MODIFICAR catalogo.txt: agregar “relacion|rutaBloque\n”
    char rutaCatalogo[MAX_PATH_LEN];
    snprintf(rutaCatalogo, sizeof(rutaCatalogo),
        "%s%s", discoNuevoPath, "catalogo.txt");
    FILE* fcat = fopen(rutaCatalogo, "a");
    if (fcat) {
        fprintf(fcat, "%s|%s\n", relacion, rutaBloque);
        fclose(fcat);
    }


    return true;
}

void calcularLongitudFija(const char* rutaTXT) {
    FILE* ftxt = fopen(rutaTXT, "r");
    if (!ftxt) {
        perror("No se puede abrir el archivo para calcular longitudes fijas");
        return;
    }

    char linea[MAX_BUF];
    if (!fgets(linea, MAX_BUF, ftxt)) {
        fclose(ftxt);
        return;
    }
    // Eliminar CRLF
    linea[strcspn(linea, "\r\n")] = '\0';

    // Contar cuántos campos hay (número de '#' + 1)
    int numFields = 1;
    for (char* p = linea; *p; ++p) {
        if (*p == '#') numFields++;
    }
    if (numFields < 1) numFields = 1;
    if (numFields > MAX_FIELDS) numFields = MAX_FIELDS;

    // Array estático para guardar la máxima longitud de cada campo
    int maxLen[MAX_FIELDS] = { 0 };

    // Procesar la primera línea
    {
        char copy[MAX_BUF];
        strncpy(copy, linea, MAX_BUF);
        copy[MAX_BUF - 1] = '\0';

        char* tok = strtok(copy, "#");
        int idx = 0;
        while (tok && idx < numFields) {
            int len = (int)strlen(tok);
            if (len > maxLen[idx]) maxLen[idx] = len;
            idx++;
            tok = strtok(NULL, "#");
        }
    }

    // Procesar el resto de las líneas
    while (fgets(linea, MAX_BUF, ftxt)) {
        linea[strcspn(linea, "\r\n")] = '\0';
        char copy[MAX_BUF];
        strncpy(copy, linea, MAX_BUF);
        copy[MAX_BUF - 1] = '\0';

        char* tok = strtok(copy, "#");
        int idx = 0;
        while (tok && idx < numFields) {
            int len = (int)strlen(tok);
            if (len > maxLen[idx]) maxLen[idx] = len;
            idx++;
            tok = strtok(NULL, "#");
        }
    }
    fclose(ftxt);

    // Extraer nombre de relación (sin ruta ni extensión)
    const char* slash = strrchr(rutaTXT, '/');
    const char* backslash = strrchr(rutaTXT, '\\');
    const char* fname = slash ? slash + 1 : (backslash ? backslash + 1 : rutaTXT);
    char nombreRel[MAX_STR_LEN];
    strncpy(nombreRel, fname, MAX_STR_LEN - 1);
    nombreRel[MAX_STR_LEN - 1] = '\0';
    // Quitar extensión (por ejemplo ".csv" o ".txt")
    char* ext = strrchr(nombreRel, '.');
    if (ext) *ext = '\0';

    // Escribir en longitudfija.txt
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

/**
 * obtenerRegistroSize:
 *   - Abre longitudFija.txt, busca la línea que comience con "<relacion>|".
 *   - Parámetros:
 *       relacion: nombre de la relación (sin ".csv").
 *   - Retorna en *outRegistroSize el tamaño máximo fijo de un registro:
 *       suma de todas las longitudes máximas de campo +
 *       (numFields - 1) bytes para los separadores ',' o '#' entre campos +
 *       1 byte adicional para el separador '|' final.
 *   - Si no encuentra la relación, devuelve 0 en *outRegistroSize.
 *
 *   No usa memoria dinámica ni STL. Emplea buffers locales.
 */
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
        // Verificar si la línea comienza con "relacion|"
        size_t relLen = strlen(relacion);
        if (strncmp(linea, relacion, relLen) == 0 && linea[relLen] == '|') {
            // Formato: "<relacion>|<numFields>#<len1>#<len2>#...#<lenN>"
            char* p = linea + relLen + 1; // apunta justo después del '|'

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

            // (numFields - 1) bytes de separadores '#' entre los campos
            int separadores = (numFields > 1 ? numFields - 1 : 0);
            // +1 byte para el separador '|' final
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


////////////////////Version propia para Longitud Fija, reemplazar cuando quieres adicionarRegistroUnico por default, sin uso de
//////////////////// insertar de forma fija
bool adicionarRegistroUnico(const char* registroTxt, const char* relacion) {
    // --- 0) Antes de abrir dirBloques, obtenemos el tamaño fijo del registro ---
    int registroSize;
    std::cout << "DEBUG0" << relacion << std::endl;
    std::cout << "DEBUG1" << registroTxt << std::endl;
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
    // raw_len_total = strlen(linea) en disco (incluye "\r\n" si existe)
    size_t raw_len_total = strlen(linea);
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
        strncpy(copia2, linea, MAX_BUF);
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
    if (ofsN > (int)raw_len_total - 2) {
        // Si la “parte útil” ya sobrepasa raw_len_total-2, truncar
        bufferNueva[raw_len_total - 2] = '\r';
        bufferNueva[raw_len_total - 1] = '\n';
        bufferNueva[raw_len_total] = '\0';
    }
    else {
        // Rellenar espacios entre ofsN y raw_len_total-2
        for (int i = ofsN; i < (int)raw_len_total - 2; i++) {
            bufferNueva[i] = ' ';
        }
        bufferNueva[raw_len_total - 2] = '\r';
        bufferNueva[raw_len_total - 1] = '\n';
        bufferNueva[raw_len_total] = '\0';
    }

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
        // Si registroTxt NO incluye un '|' al final, nosotros añadimos el '|':
        fwrite(registroTxt, 1, strlen(registroTxt), fbloc);
        fputc('|', fbloc);
        fflush(fbloc);

        fclose(fbloc);
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


    /*
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
    */


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
        // 1. Estimación inicial: si no hubiera cabecera, cuántos registroSize caben en tamBloque.
        int numMax = tamBloque / registroSize;
        if (numMax < 1) {
            // Ni siquiera cabe un solo registro. Forzamos mínimo 0.
            numMax = 0;
        }

        // Variables para la iteración
        int    numMaxPrev = -1;
        size_t headerLenPrev = 0;
        char   tmpHeader[MAX_BUF];

        // 2. Iteramos hasta que numMax deje de cambiar:
        while (numMax != numMaxPrev) {
            numMaxPrev = numMax;

            // 2.1 Construir temporalmente la cabecera con numReg=0, numMax, y bitmap de '0's.
            //     Formato → "0#<numMax>#0000...0\n"
            //     Longitud del campo "<numMax>" varía con la cantidad de dígitos.
            int  ofs = 0;
            ofs += snprintf(tmpHeader + ofs, MAX_BUF - ofs, "0#%d#", numMax);
            // Llenar con '0' numMax veces:
            for (int i = 0; i < numMax && ofs < MAX_BUF - 2; i++) {
                tmpHeader[ofs++] = '0';
            }
            // Insertar salto de línea final:
            tmpHeader[ofs++] = '\n';
            tmpHeader[ofs] = '\0';

            // 2.2 Medir cuánto ocupa en bytes esa cabecera:
            size_t headerLen = (size_t)strlen(tmpHeader);

            // 2.3 Ahora calculamos cuántos registros caben realmente si reservamos
            //     espacio 'headerLen' para la cabecera:
            if (headerLen >= (size_t)tamBloque) {
                // La cabecera ya ocupa todo el bloque: 
                // No cabe ningún registro.
                numMax = 0;
            }
            else {
                int espacioRestante = tamBloque - (int)headerLen;
                int posibleMax = espacioRestante / registroSize;
                if (posibleMax < 0) posibleMax = 0;
                numMax = posibleMax;
            }

            // 2.4 Si numMax cambió, repetimos; si no, salimos.
            headerLenPrev = headerLen;
        }

        // 3. Resultado final: generar la cadena definitiva en bufferHeader:
        //    “0#<numMax>#<numMax ceros>\n”
        int ofsFinal = 0;
        ofsFinal += snprintf(bufferHeader + ofsFinal, MAX_BUF - ofsFinal, "0#%d#", numMax);
        for (int i = 0; i < numMax && ofsFinal < MAX_BUF - 2; i++) {
            bufferHeader[ofsFinal++] = '0';
        }
        bufferHeader[ofsFinal++] = '\n';
        bufferHeader[ofsFinal] = '\0';

        // 4. Devolver los valores calculados:
        *outHeaderLen = (size_t)strlen(bufferHeader);
        *outNumMax = numMax;
    }

    /*
    void calcularLongitudFija(const char* rutaCSV) {
        FILE* fcsv = fopen(rutaCSV, "r");
        if (!fcsv) {
            perror("No se puede abrir el CSV para calcular longitudes fijas");
            return;
        }

        char linea[MAX_BUF];
        if (!fgets(linea, MAX_BUF, fcsv)) {
            fclose(fcsv);
            return;
        }
        linea[strcspn(linea, "\r\n")] = '\0';

        int numFields = 1;
        for (char* p = linea; *p; ++p) {
            if (*p == ',') numFields++;
        }

        int* maxLen = (int*)malloc(sizeof(int) * numFields);
        if (!maxLen) { fclose(fcsv); return; }
        for (int i = 0; i < numFields; i++) maxLen[i] = 0;

        {
            char* copy = _strdup(linea);
            char* tok = strtok(copy, ",");
            int idx = 0;
            while (tok && idx < numFields) {
                int len = (int)strlen(tok);
                if (len > maxLen[idx]) maxLen[idx] = len;
                idx++;
                tok = strtok(NULL, ",");
            }
            free(copy);
        }

        while (fgets(linea, MAX_BUF, fcsv)) {
            linea[strcspn(linea, "\r\n")] = '\0';
            char* copy = _strdup(linea);
            char* tok = strtok(copy, ",");
            int idx = 0;
            while (tok && idx < numFields) {
                int len = (int)strlen(tok);
                if (len > maxLen[idx]) maxLen[idx] = len;
                idx++;
                tok = strtok(NULL, ",");
            }
            free(copy);
        }
        fclose(fcsv);

        const char* slash = strrchr(rutaCSV, '/');
        const char* fname = slash ? slash + 1 : rutaCSV;
        char nombreRel[MAX_STR_LEN];
        strncpy(nombreRel, fname, MAX_STR_LEN - 1);
        nombreRel[MAX_STR_LEN - 1] = '\0';
        char* ext = strstr(nombreRel, ".csv");
        if (ext) *ext = '\0';

        FILE* flog = fopen(rutaLongitudFija, "a");
        if (!flog) {
            perror("No se puede abrir longitudFija.txt");
            free(maxLen);
            return;
        }
        fprintf(flog, "%s|%d", nombreRel, numFields);
        for (int i = 0; i < numFields; i++) {
            fprintf(flog, "#%d", maxLen[i]);
        }
        fprintf(flog, "\n");
        fclose(flog);
        free(maxLen);
    }

    */

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
