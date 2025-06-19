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
    char bloquePath[MAX_PATH_LEN];

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
        if (lineaNum < 1) return false;

        FILE* f = fopen(rutaDirBloques, "r");
        if (!f) return false;

        char lineaBuf[MAX_BUF];
        int len = 0, bloqueActual = 0;

        // Recorremos bloques hasta llegar al 'lineaNum'-ésimo
        while (true) {
            int c;
            do {
                c = fgetc(f);
                if (c == EOF) {
                    fclose(f);
                    return false;   // no llegamos a bloqueNum
                }
            } while (c != '|');

            // desde ese '|' hasta el siguiente '|'
            len = 0;
            lineaBuf[len++] = (char)c;   // guardamos el '|'
            while ((c = fgetc(f)) != EOF && len < MAX_BUF - 1) {
                lineaBuf[len++] = (char)c;
                if (c == '|') break;    // fin de este bloque
            }
            lineaBuf[len] = '\0';

            bloqueActual++;
            if (bloqueActual == lineaNum) {
                // Ya tenemos el bloque deseado en lineaBuf[0..len]
                strncpy(bufferLectura, lineaBuf, MAX_BUF);
                bufferLectura[MAX_BUF] = '\0';
                fclose(f);
                return true;
            }
            // si no es el bloque deseado, seguimos al siguiente iteración
        }
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
        strcpy(bloquePath, "DISCO\\BLOQUES\\");

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

    void crearBloqueFisico(int nroBloque) {
        // Asume que "discoNuevoPath" es el path base global
        char dirPath[MAX_PATH_LEN];
        snprintf(dirPath, sizeof(dirPath), "%sBLOQUES", discoNuevoPath);
        struct stat st = { 0 };
        if (stat(dirPath, &st) == -1) {
            _mkdir(dirPath);
        }
        char filePath[MAX_PATH_LEN];
        snprintf(filePath, sizeof(filePath), "%sBLOQUES\Bloque%d.txt", discoNuevoPath, nroBloque);
        // Crear archivo vacío, el código de adicionarRegistroUnico luego escribe la cabecera
        FILE* f = fopen(filePath, "wb");
        if (f) fclose(f);
    }

    void createBufferDir() {
        const std::string bufferDir = "BUFFERPOOL";
        make_dir(bufferDir.c_str());

        const std::string bloqueDir = "BUFFERPOOL\\BLOQUES";
        make_dir(bloqueDir.c_str());
    }

    const char* getBloquePath(int bloqueId) const { 
        char path[MAX_PATH_LEN];
        snprintf(path, sizeof(path), "%sBloque%d.txt", bloquePath, bloqueId);
        return path;
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


    static int leerBloqueConSeparador(FILE* f, char* lineaBuf, int maxLen) {
        int pos = 0;
        int c;

        // 1) Buscar el primer '|'
        while ((c = fgetc(f)) != EOF) {
            if (c == '|') {
                lineaBuf[pos++] = (char)c;
                break;
            }
        }
        if (c == EOF) {
            return 0; // no quedan bloques
        }

        // 2) Leer hasta el siguiente '|' o hasta llenarnos
        while ((c = fgetc(f)) != EOF && pos < maxLen - 1) {
            lineaBuf[pos++] = (char)c;
            if (c == '|') {
                break; // bloque completo
            }
        }
        lineaBuf[pos] = '\0';
        return pos;
    }

    // -----------------------------------------------------------------
    //  LEE TODOS LOS BLOQUES “|…|” EN dirBloques.txt Y RETORNA EL MÁXIMO
    // -----------------------------------------------------------------
    static int obtenerLongitudMaximaBloque1() {
        FILE* f = fopen("DISCO\\dirBloques.txt", "r");
        if (!f) return -1;

        char linea[MAX_BUF];
        int maxLen = 0;
        int len;

        // Leer bloque por bloque (delimitado por '|') hasta EOF
        while ((len = leerBloqueConSeparador(f, linea, MAX_BUF)) > 0) {
            // len incluye ambos caracteres '|' de apertura y cierre
            if (len > maxLen) {
                maxLen = len;
            }
        }

        fclose(f);
        return maxLen;  // será el tamaño fijo al que rellenar después
    }



    void crearBloquesLogicos() {
        // --------------------------------------------------
        //  1) Escribir dirBloques.txt “en crudo” (sin padding)
        // --------------------------------------------------
        std::ofstream fdir_crudo("DISCO\\dirBloques.txt", std::ios::out);
        if (!fdir_crudo.is_open()) {
            std::perror("Error creando dirBloques.txt (crudo)");
            return;
        }

        std::cout << " Creando disco (fase 1: crudo)... " << std::endl;

        make_dir("DISCO\\BLOQUES");

        int sectoresPorBloque = (int)(tamBloque / tamSector);
        int curPista = 1;
        int curSector = 1;

        // FASE 1: Generar cada bloque sin padding ni barras |…|
        for (int i = 1; i <= nroBloques; ++i) {
            // Escribimos el encabezado “crudo” como antes:
            fdir_crudo << tamBloque << "#2#BLOQUE#" << i << "#" << tamBloque << "#_";

            int asignados = 0;
            while (asignados < sectoresPorBloque) {
                for (int p = 1; p <= platos && asignados < sectoresPorBloque; ++p) {
                    for (int s = 1; s <= 2 && asignados < sectoresPorBloque; ++s) {
                        fdir_crudo << tamSector << "#"
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
            // Ahora ponemos un salto de línea para separar bloques crudos:
            fdir_crudo << std::endl;
        }
        fdir_crudo.close();

        // ------------------------------------------------------
        //  2) Calcular la longitud máxima entre todos los bloques crudos
        // ------------------------------------------------------
        // Para ello, primero tenemos que “envolver” cada línea
        // cruda dentro de barras “|…|” para medirla con leerBloqueConSeparador.
        // Entonces reescribimos temporalmente cada línea cruda con barras
        // y sin padding, luego midiendo su longitud, quedándonos con el mayor.
        //
        // A) Abrir el archivo crudo para leer:
        FILE* fcrudo = fopen("DISCO\\dirBloques.txt", "r");
        if (!fcrudo) {
            std::perror("Error al abrir dirBloques.txt (crudo) para calcular máximo");
            return;
        }

        // B) Leer línea a línea (cada bloque crudo está en su propia línea),
        //    envolverla con '|' y medir su longitud. Conservar el máximo.
        char lineaCruda[MAX_BUF];
        int maxLen = 0;
        while (fgets(lineaCruda, MAX_BUF, fcrudo)) {
            // Quitar '\r' o '\n' al final de la línea cruda:
            size_t L = strlen(lineaCruda);
            while (L > 0 && (lineaCruda[L - 1] == '\r' || lineaCruda[L - 1] == '\n')) {
                lineaCruda[--L] = '\0';
            }
            // Construir temporalmente "|<lineaCruda>|"
            int longTemp = (int)(L + 2); // +2 por ambos '|'
            if (longTemp > maxLen) {
                maxLen = longTemp;
            }
        }
        fclose(fcrudo);

        // Guardar ese valor en la variable global:
        int maxLenBloque = maxLen;
        std::cout << "  Longitud fija por bloque = " << maxLenBloque << " bytes\n";

        // -------------------------------------------------
        //  3) Borrar el archivo crudo y reescribir con padding
        // -------------------------------------------------
        // Borramos el dirBloques.txt “crudo”:
        std::remove("DISCO\\dirBloques.txt");

        // Ahora reabrimos para escribir “padded + delimitado”:
        std::ofstream fdir_pad("DISCO\\dirBloques.txt", std::ios::out);
        if (!fdir_pad.is_open()) {
            std::perror("Error recreando dirBloques.txt (pad)");
            return;
        }

        std::cout << " Creando disco (fase 2: padded)... " << std::endl;

        // Debemos volver a generar cada bloque EXACTAMENTE igual que antes,
        // pero rodearlo con '|' y rellenar con '@' hasta maxLenBloque.
        curPista = 1;
        curSector = 1;
        for (int i = 1; i <= nroBloques; ++i) {
            // 3.a) Armar el contenido “crudo” de este bloque en un buffer temporal:
            //     (sin barras ni padding aún)
            char tmp[MAX_BUF];
            int ofs = 0;
            ofs += snprintf(tmp + ofs, MAX_BUF - ofs,
                "%lld#2#BLOQUE#%d#%lld#_",
                tamBloque, i, tamBloque);

            int asignados = 0;
            while (asignados < sectoresPorBloque) {
                for (int p = 1; p <= platos && asignados < sectoresPorBloque; ++p) {
                    for (int s = 1; s <= 2 && asignados < sectoresPorBloque; ++s) {
                        ofs += snprintf(tmp + ofs, MAX_BUF - ofs,
                            "%lld#%d/%d/%d/%d#_",
                            tamSector, p, s, curPista, curSector);
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
            tmp[ofs] = '\0';  // finalizamos la cadena cruda

            // 3.b) Ahora construimos “|<tmp_contenido_puro> … <padding>@|”
            //     en otro buffer de tamaño fijo maxLenBloque:
            char bloqueFinal[MAX_BUF];
            int ptr = 0;

            // Agregar la barra inicial
            bloqueFinal[ptr++] = '|';

            // Copiar tmp (contenido crudo) justo después:
            int lenCrudo = (int)strlen(tmp);
            if (ptr + lenCrudo >= MAX_BUF) {
                // No debería suceder si MAX_BUF es lo suficientemente grande
                lenCrudo = MAX_BUF - ptr - 1;
            }
            memcpy(bloqueFinal + ptr, tmp, lenCrudo);
            ptr += lenCrudo;

            // Agregar padding '@' hasta llegar a (maxLenBloque - 1)
            while (ptr < maxLenBloque - 1) {
                bloqueFinal[ptr++] = '@';
            }
            // Finalmente, agregar la barra final '|' en la posición (maxLenBloque - 1)
            bloqueFinal[ptr++] = '|';

            // Por seguridad, si ptr < MAX_BUF, ponemos terminador:
            if (ptr < MAX_BUF) {
                bloqueFinal[ptr] = '\0';
            }

            // 3.c) Escribimos exactamente maxLenBloque bytes de bloqueFinal:
            fdir_pad.write(bloqueFinal, maxLenBloque);
            // NO escribimos '\n'. Seguimos con el siguiente bloque “en línea”.

            // Esa operación deja intactos todos los bloques posteriores,
            // porque cada uno ocupa EXACTAMENTE maxLenBloque bytes.
        }

        fdir_pad.close();
    }


    void mostrarArbolDisco() {
        std::cout << "\n=== Arbol de Creacion del Disco ===\n";
        for (int i = 0; i < platos; ++i) {
            std::cout << "Plato " << (i + 1) << "\n";

            for (int s = 0; s < nroSuperficies; ++s) {
                bool ultimaSuperficie = (s == nroSuperficies - 1);

                std::cout << (ultimaSuperficie ? "|_ " : "|- ")
                    << "Superficie " << (s + 1) << "\n";

                for (int p = 0; p < pistas; ++p) {
                    bool ultimaPista = (p == pistas - 1);

                    if (ultimaSuperficie) std::cout << "   ";
                    else std::cout << "|  ";

                    std::cout << (ultimaPista ? "|_ " : "|- ")
                        << "Pista " << (p + 1) << "\n";

                    for (int sec = 0; sec < sectores; ++sec) {
                        bool ultimoSector = (sec == sectores - 1);

                        if (ultimaSuperficie) std::cout << "   ";
                        else std::cout << "|  ";

                        if (ultimaPista) std::cout << "   ";
                        else std::cout << "|  ";

                        std::cout << (ultimoSector ? "|_ " : "|- ")
                            << "Sector " << (sec + 1) << "\n";
                    }
                }
            }

            if (i != platos - 1) std::cout << "\n";
        }
        std::cout << "===================================\n\n";
    }

    int get_tam_bloque() { //funcion para tamaño de disco
        return (int)tamBloque;
    }


    //ESTE ES DE REFERENCIA, ESTO ES UNA BASE A LO QUE FUNCIONABA ANTERIORMENTE
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

    void volcarBloqueASectoresVariable(int bloqueN) {
        if (bloqueN < 1 || bloqueN > nroBloques) {
            std::cout << "> Bloque inválido: " << bloqueN << "\n";
            return;
        }

        // 1) Leer la línea correspondiente en dirBloques.txt (para parsear sectores asociados)
        if (!leerLineaDirBloque(bloqueN)) {
            std::cout << "> No se pudo leer dirBloques.txt en línea " << bloqueN << "\n";
            return;
        }

        // 2) Abrir el archivo de BloqueN.txt en modo binario
        char rutaBloque[MAX_PATH_LEN];
        snprintf(rutaBloque, sizeof(rutaBloque),
            "DISCO\\BLOQUES\\Bloque%d.txt", bloqueN);
        FILE* fbloc = std::fopen(rutaBloque, "rb");
        if (!fbloc) {
            std::perror("Error al abrir BloqueN.txt");
            return;
        }

        // 3) Determinar el tamaño total de BloqueN.txt (header + registros)
        std::fseek(fbloc, 0, SEEK_END);
        long tamDatos = std::ftell(fbloc);
        std::fseek(fbloc, 0, SEEK_SET);

        // 4) Calcular cuántos sectores físicos necesitaría como máximo
        int tamSector = getTamSector();
        int maxSectores = static_cast<int>((getTamBloque() + tamSector - 1) / tamSector);
        // reservar array para códigos "plato/pista/…"
        char (*sectoresFisicos)[MAX_STR_LEN] =
            (char (*)[MAX_STR_LEN])std::malloc(maxSectores * MAX_STR_LEN);
        if (!sectoresFisicos) {
            std::cout << "> Error de memoria al reservar sectoresFisicos\n";
            std::fclose(fbloc);
            return;
        }

        // 5) Parsear la lista de sectores asignados a este bloque (los cargamos en sectoresFisicos[])
        int totSect = parsearSectoresBloque(sectoresFisicos, maxSectores, bufferLectura);
        if (totSect <= 0) {
            std::cout << "> No se encontraron sectores asignados para el Bloque "
                << bloqueN << ".\n";
            std::free(sectoresFisicos);
            std::fclose(fbloc);
            return;
        }

        // 6) Iterar, para cada sector físico, leyendo hasta tamSector bytes de fbloc
        //    y volcándolos al archivo de ese sector. Si fbloc entra en EOF, abrimos
        //    igualmente el siguiente sector (debug) y cerramos sin escribir nada más.

        // Buffer temporal para leer un bloque de tamaño 'tamSector'
        char* sectorBuf = (char*)std::malloc(tamSector);
        if (!sectorBuf) {
            std::cout << "> Error de memoria al reservar sectorBuf\n";
            std::free(sectoresFisicos);
            std::fclose(fbloc);
            return;
        }

        for (int idx = 0; idx < totSect; ++idx) {
            // Preparar ruta del sector físico actual
            rutaSectorDesdeCodigo(sectoresFisicos[idx]);
            std::cout << "[DEBUG] Abriendo sector físico para escritura: "
                << bufferRuta << "\n";

            FILE* fsec = std::fopen(bufferRuta, "wb");
            if (!fsec) {
                std::perror("Error abriendo sector para escritura");
                // aunque haya error, seguimos al siguiente sector
                continue;
            }

            // Intentar leer hasta 'tamSector' bytes de fbloc
            size_t bytesLeidos = std::fread(sectorBuf, 1, tamSector, fbloc);
            if (bytesLeidos > 0) {
                // Escribir exactamente los bytesLeidos en el sector
                size_t escritos = std::fwrite(sectorBuf, 1, bytesLeidos, fsec);
                if (escritos != bytesLeidos) {
                    std::perror("Error al escribir en sector físico");
                }
            }
            // Si bytesLeidos < tamSector, entonces fbloc ya estaba en EOF o se agotaron datos.
            // No escribimos nada más: el resto del sector queda “vacío” (padding implícito).

            std::fclose(fsec);
            // Pasamos al siguiente sector físico; si fbloc ya está en EOF, 
            // cada fopen/fclose solo mostrará el DEBUG de apertura sin escribir datos.
        }

        std::free(sectorBuf);
        std::free(sectoresFisicos);
        std::fclose(fbloc);

        std::cout << "> Bloque " << bloqueN
            << " volcado en " << totSect
            << " sectores (incluyendo aquellos vacíos por padding).\n";
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
        printf("=== ------------------- ===\n");
    }

    void printDiscoAnterior() {
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


    void printDisco() {
        std::cout << "\n=== Caracteristicas del Disco (dinámico desde dirBloques.txt) ===\n";
        std::cout << "  Platos: " << platos << "\n";
        std::cout << "  Superficies x plato: " << nroSuperficies << "\n";
        std::cout << "  Pistas por superficie: " << pistas << "\n";
        std::cout << "  Sectores por pista: " << sectores << "\n";
        std::cout << "  Tamaño de sector: " << tamSector << " bytes\n";
        std::cout << "  Tamaño de bloque: " << tamBloque << " bytes\n";

        // --- Leer dirBloques.txt para calcular dinámicamente nroBloques y espacio libre ---
        FILE* fdir = std::fopen(rutaDirBloques, "r");
        if (!fdir) {
            std::perror("ERROR: no se pudo abrir dirBloques.txt");
            std::cout << "=================================\n\n";
            return;
        }

        int fixedLen = obtenerLongitudMaximaBloque1();
        if (fixedLen <= 0) {
            std::cerr << "ERROR: fixedLen inválido para dirBloques\n";
            std::fclose(fdir);
            std::cout << "=================================\n\n";
            return;
        }

        int bloquesContados = 0;
        long totalLibre = 0;
        char buffer[MAX_BUF + 1];
        while (true) {
            int len = leerBloqueConSeparador(fdir, buffer, MAX_BUF);
            if (len <= 0) break;
            bloquesContados++;

            // Terminar en '\0' para parsear
            if (len < MAX_BUF) buffer[len] = '\0';
            else               buffer[MAX_BUF] = '\0';

            // buffer ≈ "|<espLibreBloque>#2#BLOQUE#...#_...|"
            // Extraer <espLibreBloque> (entre '|' y primer '#')
            char temp[MAX_BUF];
            std::memcpy(temp, buffer + 1, len - 2);
            temp[len - 2] = '\0';
            char* pHash = std::strchr(temp, '#');
            if (!pHash) continue;
            *pHash = '\0';
            int espBloque = std::atoi(temp);
            totalLibre += espBloque;
        }
        std::fclose(fdir);

        int bloquesPorPista = (sectores * tamSector) / tamBloque;
        int bloquesPorPlato = nroSuperficies * pistas * bloquesPorPista;
        long capacidadTotal = static_cast<long>(platos) *
            nroSuperficies * pistas *
            sectores * tamSector;
        std::cout << "  Bloques totales (en dirBloques.txt): " << bloquesContados << "\n";
        std::cout << "  Bloques por pista: " << bloquesPorPista << "\n";
        std::cout << "  Bloques por plato: " << bloquesPorPlato << "\n";
        std::cout << "  Capacidad total: " << capacidadTotal << " bytes\n";
            std::cout << "  Espacio libre (sumado en todos los bloques): " << totalLibre << " bytes\n";
            std::cout << "=================================\n\n";
        }
    };