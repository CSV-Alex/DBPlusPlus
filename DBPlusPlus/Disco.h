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

    // --- Add your getters here ---
    int getPlatos() const { return platos; }
    int getPistas() const { return pistas; }
    int getSectores() const { return sectores; }
    long long getTamSector() const { return tamSector; }
    long long getTamBloque() const { return tamBloque; }
    int getNroSuperficies() const { return nroSuperficies; }
    long long getCapacidadTotal() const { return capacidadTotal; }
    long long getCapacidadLibre() const { return capacidadLibre; }
    int getNroBloques() const { return nroBloques; }
    const char* getBufferRuta() const { return bufferRuta; }
    const char* getBufferLectura() const { return bufferLectura; }


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

        make_dir("DISCO\\BLOQUES");

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

    int get_tam_bloque() { //funcion para tamaño de disco
        return (int)tamBloque;
    }

    void volcarBloqueASectores(int bloqueN) {
        if (bloqueN < 1 || bloqueN > nroBloques) {
            std::cout << "> Bloque inválido: " << bloqueN << "\n";
            return;
        }

        // 1) Leer la línea correspondiente en dirBloques.txt (para parsear sectores asociados)
        if (!leerLineaDirBloque(bloqueN)) {
            std::cout << "> No se pudo leer dirBloques.txt en línea " << bloqueN << "\n";
            return;
        }

        // 2) Abrir el archivo del bloque en modo binario (para leer header + registros)
        char rutaBloque[MAX_PATH_LEN];
        snprintf(rutaBloque, sizeof(rutaBloque),
            "DISCO\\BLOQUES\\Bloque%d.txt", bloqueN);
        FILE* fbloc = fopen(rutaBloque, "rb");
        if (!fbloc) {
            std::perror("Error al abrir BloqueN.txt");
            return;
        }

        // 3) Determinar el tamaño total del bloque (header + registros)
        fseek(fbloc, 0, SEEK_END);
        long tamDatos = ftell(fbloc);
        fseek(fbloc, 0, SEEK_SET);

        int maxSect = (int)(tamBloque / tamSector);
        // reservar arreglo de cadenas para códigos "p/s/pi/se"
        char (*sectoresFisicos)[MAX_STR_LEN] =
            (char (*)[MAX_STR_LEN])malloc(maxSect * MAX_STR_LEN);
        if (!sectoresFisicos) {
            std::cout << "> Error de memoria al reservar sectoresFisicos\n";
            fclose(fbloc);
            return;
        }

        // 4) Parsear la lista de sectores (p/s/pi/se) que pertenecen a este bloque
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

        //// 5) SALTAR la cabecera hasta encontrar '/' (fin del header en BloqueN.txt)
        //while (true) {
        //    int ch = fgetc(fbloc);
        //    if (ch == EOF) {
        //        // no había nada más que la cabecera
        //        free(sectoresFisicos);
        //        fclose(fbloc);
        //        return;
        //    }
        //    if (ch == '/') {
        //        // hemos leído la '/' final del header
        //        break;
        //    }
        //}
        // Ahora fbloc apunta al primer byte del primer registro (o EOF si no hay registros).

        // 6) Inicializar variables de control de sector físico
        int sectorIdx = 0;                            // índice en sectoresFisicos[]
        long espacioRestante = tamSector;             // cuánto cabe aún en el sector actual
        FILE* fsec = nullptr;                         // puntero al archivo del sector en uso

        // Función auxiliar para abrir el siguiente sector (sectorIdx)
        auto abrirSiguienteSector = [&]() -> bool {
            if (fsec) {
                fclose(fsec);
                fsec = nullptr;
            }
            if (sectorIdx >= totSect) {
                // ya no quedan sectores físicos disponibles
                return false;
            }
            // Generar ruta real del sector: "DISCO\PlatoX\SX\PistaX\SectorX.txt"
            rutaSectorDesdeCodigo(sectoresFisicos[sectorIdx]);
            std::cout << "[DEBUG] Abriendo sector físico para escritura: "
                << bufferRuta << "\n";
            fsec = fopen(bufferRuta, "wb");
            if (!fsec) {
                std::perror("Error abriendo sector para escritura");
                return false;
            }
            espacioRestante = tamSector;
            return true;
            };

        // Abrir el primer sector (sectorIdx = 0)
        if (!abrirSiguienteSector()) {
            std::cout << "> No se pudo abrir el primer sector físico.\n";
            free(sectoresFisicos);
            fclose(fbloc);
            return;
        }

        // 7) Recorrer registro por registro: leer desde fbloc hasta encontrar '|'
        while (true) {
            // 7.1) Marcar posición de inicio del registro
            long inicioRegistro = ftell(fbloc);
            if (inicioRegistro < 0) break;

            // 7.2) Leer hasta encontrar '|' o EOF, contando longitud del registro
            long lenRegistro = 0;
            bool encontroSeparador = false;
            while (true) {
                int ch = fgetc(fbloc);
                if (ch == EOF) {
                    break;
                }
                lenRegistro++;
                if (ch == '|') {
                    encontroSeparador = true;
                    break;
                }
            }
            if (lenRegistro == 0) {
                // no quedan registros nuevos
                break;
            }
            // Si llegamos a EOF sin hallar '|', lenRegistro cuenta los bytes leídos; 
            // para este código asumimos que TODO registro válido debe terminar en '|'.
            if (!encontroSeparador) {
                // registro corrupto (sin '|'), salimos
                std::cout << "> Advertencia: registro sin separador '|' completo.\n";
                break;
            }

            // 7.3) Reservar buffer temporal para copiar ese registro completo
            char* registroBuf = (char*)malloc(lenRegistro);
            if (!registroBuf) {
                std::cout << "> Error de memoria al reservar registroBuf\n";
                break;
            }
            // 7.4) Volver al inicio de este registro y leerlo en registroBuf
            fseek(fbloc, inicioRegistro, SEEK_SET);
            size_t leidos = fread(registroBuf, 1, lenRegistro, fbloc);
            if ((long)leidos != lenRegistro) {
                std::cout << "> Error al leer registro desde Bloque" << bloqueN << "\n";
                free(registroBuf);
                break;
            }
            // Después de leer, el puntero de fbloc ya está posicionado justo luego del '|'.

            // 7.5) Si el registro NO cabe en espacioRestante, abrir nuevo sector:
            if (lenRegistro > espacioRestante) {
                sectorIdx++;
                if (!abrirSiguienteSector()) {
                    std::cout << "> No quedan sectores para colocar el registro.\n";
                    free(registroBuf);
                    break;
                }
            }

            // 7.6) Escribir el registro completo en el sector actual
            size_t escritos = fwrite(registroBuf, 1, lenRegistro, fsec);
            if ((long)escritos != lenRegistro) {
                std::perror("Error al escribir registro en sector");
                free(registroBuf);
                break;
            }
            espacioRestante -= lenRegistro;
            free(registroBuf);

            // 7.7) Continuar con el siguiente registro (fbloc ya está en posición)
        }

        // 8) Cerrar el último sector abierto
        if (fsec) {
            fclose(fsec);
            fsec = nullptr;
        }

        free(sectoresFisicos);
        fclose(fbloc);

        std::cout << "> Bloque " << bloqueN
            << " volcado en " << (sectorIdx + 1)
            << " sectores (registros enteros).\n";
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

};

