#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>   // _mkdir en Windows
#include <direct.h>     // tambien para _mkdir
#include <iostream>
#include <fstream>
#include "Disco.h"
#include "DiscoPaths.h"
#include "Catalogo.cpp"

#define MAX_TOKENS    256

static const int MAX_FIXEDLEN = 1024;

// Lee catalogo.txt y retorna el número de último bloque asignado a `relacion` (0 si no existe)
// -----------------------------------------------------------------------------
// Función: leerBloqueDeCatalogo
// Objetivo de la función:
//     Leer el archivo de catálogo y retornar el número del último bloque
//     asignado a una relación dada.
// Input:
//     const char* rutaCatalogo  – ruta al archivo catalogo.txt.
//     const char* relacion      – nombre de la relación a buscar.
// Output:
//     int  – número de bloque hallado, o 0 si no existe o hay error al abrir.
// Autor: Alex Cañapataña
// -----------------------------------------------------------------------------
static int leerBloqueDeCatalogo(const char* rutaCatalogo, const char* relacion) {
    FILE* f = fopen(rutaCatalogo, "r");
    if (!f) return 0;
    char linea[512];
    int bloqueN = 0;
    while (fgets(linea, sizeof(linea), f)) {
        // Quitar CR/LF
        linea[strcspn(linea, "\r\n")] = '\0';
        // Separar por '|'
        char* sep = strchr(linea, '|');
        if (!sep) continue;
        *sep = '\0';
        if (strcmp(linea, relacion) == 0) {
            int n = 0;
            if (sscanf(sep + 1, "%*[^0-9]%d.txt", &n) == 1) {
                bloqueN = n;
            }
        }
    }
    fclose(f);
    return bloqueN;
}
// -----------------------------------------------------------------------------
// Función: obtenerRelacionDeBloque
// Objetivo de la función:
//     Obtener el nombre de la relación asociada a un bloque específico,
//     leyendo catalogo.txt.
// Input:
//     const char* rutaCatalogo  – ruta al archivo catalogo.txt.
//     int nroBloque             – número del bloque a consultar.
// Output:
//     char* – cadena estática con el nombre de la relación, o nullptr si no existe.
// Autor: Alex Cañapataña
// -----------------------------------------------------------------------------

static char* obtenerRelacionDeBloque(const char* rutaCatalogo, int nroBloque) {
    static char rel[MAX_STR_LEN];
    FILE* f = fopen(rutaCatalogo, "r");
    if (!f) return nullptr;
    char línea[256];
    char buscado[32];
    snprintf(buscado, sizeof(buscado), "Bloque%d.txt", nroBloque);
    while (fgets(línea, sizeof(línea), f)) {
        if (strstr(línea, buscado)) {
            // copia lo que hay antes de '|' como relación
            char* sep = strchr(línea, '|');
            if (sep) {
                size_t len = sep - línea;
                if (len >= sizeof(rel)) len = sizeof(rel) - 1;
                strncpy(rel, línea, len);
                rel[len] = '\0';
                fclose(f);
                return rel;
            }
        }
    }
    fclose(f);
    return nullptr;
}

// -----------------------------------------------------------------------------
// Función: getTamBloqueFromDisco
// Objetivo de la función:
//     Proveer el tamaño de bloque consultando al objeto Disco.
// Input:
//     Disco& disco  – referencia al objeto Disco.
// Output:
//     int  – tamaño en bytes de cada bloque.
// Autor: Alex Cañapataña
// -----------------------------------------------------------------------------
static int getTamBloqueFromDisco(Disco& disco) {
    return disco.getTamBloque();
}
// -----------------------------------------------------------------------------
// Función: getBufferRutaFromDisco
// Objetivo de la función:
//     Obtener el buffer de ruta interna del disco.
// Input:
//     const Disco& disco  – referencia constante al objeto Disco.
// Output:
//     const char*  – puntero al buffer de ruta del Disco.
// Autor: Alex Cañapataña
// -----------------------------------------------------------------------------
static const char* getBufferRutaFromDisco(const Disco& disco) {
    return disco.getBufferRuta();
}
// -----------------------------------------------------------------------------
// Función: getBufferLecturaFromDisco
// Objetivo de la función:
//     Obtener el buffer de lectura interna del disco.
// Input:
//     const Disco& disco  – referencia constante al objeto Disco.
// Output:
//     const char*  – puntero al buffer de lectura del Disco.
// Autor: Alex Cañapataña
// -----------------------------------------------------------------------------
static const char* getBufferLecturaFromDisco(const Disco& disco) {
    return disco.getBufferLectura();
}
// -----------------------------------------------------------------------------
// Función: safe_atoi
// Objetivo de la función:
//     Convertir de forma segura una cadena a entero, devolviendo 0 si no es numérica.
// Input:
//     const char* str  – cadena a convertir.
// Output:
//     int  – valor numérico convertido o 0 en caso de fallo.
// Autor: Alex Cañapataña
// ----------------------------------------------------------------------------
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
// -----------------------------------------------------------------------------
// Función: isBlockAllowed
// Objetivo de la función:
//     Verificar si un bloque está permitido para una relación dada,
//     comparando rutas en catalogo.txt.
// Input:
//     const char* nombreRelacion  – nombre de la relación.
//     int nroBloque               – número del bloque a validar.
// Output:
//     bool  – true si el bloque está permitido o no está en catálogo; false en caso contrario.
// Autor: Alex Cañapataña
// -----------------------------------------------------------------------------    
/// Importante
static bool isBlockAllowed(const char* nombreRelacion, int nroBloque) {
    char rutaCatalogo[MAX_PATH_LEN];
    snprintf(rutaCatalogo, sizeof(rutaCatalogo),
        "%s%s", discoPath, "catalogo.txt");
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
            discoPath, nroBloque);
        if (strcmp(path, bloquePath) == 0) {
            fclose(fcat);
            return (strcmp(rel, nombreRelacion) == 0);
        }
    }
    fclose(fcat);
    return true;
}
// -----------------------------------------------------------------------------
// Función: validarCampos
// Objetivo de la función:
//     Validar que cada campo de un registro separado por ‘#’ no exceda su longitud máxima.
// Input:
//     const char* registro    – cadena con campos separados por ‘#’.
//     int numFields           – número esperado de campos.
//     int* maxLenArr          – arreglo de longitudes máximas para cada campo.
// Output:
//     bool  – true si todos los campos cumplen su longitud; false si alguno excede.
// Autor: Alex Cañapataña
// -----------------------------------------------------------------------------
/// Importante
static bool validarCampos(const char* registro, int numFields, int* maxLenArr) {
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

// -----------------------------------------------------------------------------
// Función: calcularCabeceraBloque
// Objetivo de la función:
//     Calcular y construir el encabezado de un bloque de longitud fija,
//     estimando cuántos registros caben con bitmap.
// Input:
//     int tamBloque          – tamaño total del bloque en bytes.
//     int registroSize       – tamaño en bytes de cada registro fijo.
//     char* bufferHeader     – buffer de salida para la cabecera generada.
//     size_t* outHeaderLen   – puntero para longitud de la cabecera generada.
//     int* outNumMax         – puntero para número máximo de registros cabables.
// Output:
//     void (resultados en outHeaderLen y outNumMax).
// Autor: Alex Cañapataña
// -----------------------------------------------------------------------------
static void calcularCabeceraBloque(int tamBloque, int registroSize,
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

// -----------------------------------------------------------------------------
// Función: calcularLongitudFija
// Objetivo de la función:
//     Leer un archivo de texto de datos (separados por ‘#’), determinar
//     la longitud máxima de cada campo y registrar la información en longitudfija.txt.
// Input:
//     const char* rutaTXT  – ruta al archivo de texto de datos.
// Output:
//     void (registra en longitudfija.txt o imprime error por stderr).
// Autor: Alex Cañapataña
// -----------------------------------------------------------------------------

static void calcularLongitudFija(const char* rutaTXT) {
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

// -----------------------------------------------------------------------------
// Función: obtenerRegistroSize
// Objetivo de la función:
//     Leer longitudfija.txt para calcular el tamaño total en bytes de un registro fijo.
// Input:
//     const char* relacion    – nombre de la relación.
//     int* outRegistroSize    – puntero para recibir el tamaño calculado.
// Output:
//     void (outRegistroSize ajustado; 0 si no lo encuentra).
// Autor: Alex Cañapataña
// -----------------------------------------------------------------------------
/// Buscar forma de reemplazarlo por obtenerTamañoRegistro
static void obtenerRegistroSize(const char* relacion, int* outRegistroSize) {
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
// -----------------------------------------------------------------------------
// Función: obtenerLongitudesPorCampo
// Objetivo de la función:
//     Leer longitudfija.txt y extraer el número de campos y sus longitudes máximas.
// Input:
//     const char* relacion  – nombre de la relación.
//     int* numFields        – puntero para recibir número de campos.
//     int* maxLenArr        – arreglo para recibir longitudes máximas.
// Output:
//     bool  – true si se cargaron correctamente; false si falla.
// Autor: Alex Cañapataña
// -----------------------------------------------------------------------------

/// Observar
static bool obtenerLongitudesPorCampo(const char* relacion, int* numFields, int* maxLenArr) {
    FILE* flog = fopen(rutaLongitudFija, "r");
    if (!flog) {
        perror("No se puede abrir longitudfija.txt para lectura");
        *numFields = 0;
        return false;
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
                return false;
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
            return true;
        }
    }

    // Si llegamos aquí, no encontramos la relación
    fclose(flog);
    *numFields = 0;
    return false;
}
// -----------------------------------------------------------------------------
// Función: mostrarSectoresDeBloque
// Objetivo de la función:
//     Mostrar información de sectores de un bloque variable, ya sea espacio libre
//     o rutas completas según opción.
// Input:
//     int bloqueN    – número de bloque a consultar.
//     int opcion     – 1 para espacio libre, 2 para solo rutas.
//     Disco disco    – objeto Disco usado para resolver rutas.
// Output:
//     void (imprime al stdout el detalle de sectores).
// Autor: Alex Cañapataña
// -----------------------------------------------------------------------------
static void mostrarSectoresDeBloque(int bloqueN, int opcion, Disco disco) {
    if (opcion != 1 && opcion != 2) {
        std::cout << "Opción inválida. Use 1 (mostrar espacios libres) o 2 (solo rutas).\n";
        return;
    }

    // 1) Abrir dirBloques.txt
    FILE* fdir = fopen(rutaDirBloques, "r");
    if (!fdir) {
        std::perror("Error al abrir dirBloques.txt");
        return;
    }

    // 2) Leer línea por línea hasta llegar a bloqueN
    char linea[MAX_BUF];
    int  numLinea = 0;
    bool encontrado = false;

    while (fgets(linea, sizeof(linea), fdir)) {
        numLinea++;
        if (numLinea == bloqueN) {
            encontrado = true;
            break;
        }
    }
    fclose(fdir);

    if (!encontrado) {
        std::cout << "No existe el bloque " << bloqueN << " en dirBloques.txt.\n";
        return;
    }

    // 3) Eliminar CR/LF final
    size_t len = strlen(linea);
    while (len > 0 && (linea[len - 1] == '\n' || linea[len - 1] == '\r')) {
        linea[--len] = '\0';
    }

    // 4) Tokenizar la línea usando '#' como separador
    //    tokens[] almacenará punteros a cada subcadena.
    char buffer[MAX_BUF];
    strncpy(buffer, linea, MAX_BUF - 1);
    buffer[MAX_BUF - 1] = '\0';

    char* tokens[MAX_TOKENS];
    int    ntok = 0;
    char* tk = strtok(buffer, "#");
    while (tk && ntok < MAX_TOKENS) {
        tokens[ntok++] = tk;
        tk = strtok(nullptr, "#");
    }

    if (ntok < 5) {
        std::cout << "Formato inesperado en dirBloques.txt (muy pocos tokens).\n";
        return;
    }

    // 5) tokens[0] = <espLibreBloque>
    //    tokens[1] = "2"
    //    tokens[2] = "BLOQUE"
    //    tokens[3] = "<nroBloque>"
    //    tokens[4] = "<tamBloque>"
    const char* espLibreBloque = tokens[0];

    // 6) A partir de tokens[5], buscaremos pares:
    //       tokens[i] que empiece con '_'  -> representa "_<espLibreSector>"
    //       tokens[i+1]                    -> "<codSector>"
    //    Repetir hasta ntok-1

    std::cout << "Bloque " << bloqueN;
    if (opcion == 1) {
        std::cout << " (espacio libre del bloque: " << espLibreBloque << ")";
    }
    std::cout << ":\n";

    int sectorCount = 0;
    for (int i = 5; i + 1 < ntok; i++) {
        // Verificar que tokens[i][0] sea '_'
        if (tokens[i][0] != '_') {
            continue;
        }
        // Extraer espacio libre de sector (después del '_')
        const char* espLibreSector = tokens[i] + 1;
        // El siguiente token debe ser el codSector
        const char* codSector = tokens[i + 1];

        // Guardar la ruta completa en buffer interno de Disco (usando codSector)
        disco.rutaSectorDesdeCodigo(codSector);
        const char* rutaCompleta = disco.getBufferRuta();

        // Mostrar según la opción
        if (opcion == 1) {
            std::cout << "  > Sector: " << rutaCompleta
                << " (espacio libre: " << espLibreSector << ")\n";
        }
        else { // opcion == 2
            std::cout << "  > " << rutaCompleta << "\n";
        }

        sectorCount++;
        i++; // Saltar el token de codSector para la próxima iteración
    }

    std::cout << "Total de sectores mostrados: " << sectorCount << "\n";
}
// -----------------------------------------------------------------------------
// Función: crearRLF
// Objetivo de la función:
//     Construir en un buffer un Registro de Longitud Fija (RLF) a partir de un
//     texto de campos separados por ‘#’, rellenando y separando según longitudes.
// Input:
//     const char* registroTxt  – cadena original de campos separados por ‘#’.
//     const char* relacion     – relación para obtener longitudes de campo.
//     char* outBuffer          – buffer de salida para el RLF generado.
//     int* outLen              – puntero para recibir longitud final del RLF.
// Output:
//     bool  – true si se genera correctamente; false en caso de error.
// Autor: Alex Cañapataña
// -----------------------------------------------------------------------------
static bool crearRLF(const char* registroTxt, const char* relacion, char* outBuffer, int* outLen) {
    int numFields = 0;
    int maxLen[MAX_FIELDS] = { 0 };
    //printf("DEBUG: Llamando a crearRLF para relación '%s' con registroTxt: '%s'\n", relacion, registroTxt);

    if (!obtenerLongitudesPorCampo(relacion, &numFields, maxLen)) {
        printf("DEBUG: obtenerLongitudesPorCampo falló para '%s'\n", relacion);
        return false;
    }
    //printf("DEBUG: Longitudes por campo para '%s': ", relacion);
    for (int i = 0; i < numFields; ++i) {
        //printf("%d ", maxLen[i]);
    }
    //printf("\n");

    char copy[MAX_BUF];
    strncpy(copy, registroTxt, MAX_BUF - 1);
    copy[MAX_BUF - 1] = '\0';

    char* tok = strtok(copy, "#");
    int ofs = 0;

    for (int i = 0; i < numFields; i++) {
        if (!tok) {
            //printf("DEBUG: Faltan campos en registroTxt para el campo %d\n", i);
            return false;
        }

        int lenVal = (int)strlen(tok);
        int fieldLen = maxLen[i];
        int toCopy = (lenVal < fieldLen) ? lenVal : fieldLen;

        // Copiamos el contenido real (o truncado) del campo
        memcpy(outBuffer + ofs, tok, toCopy);

        // Si el valor es más corto, rellenamos con '@'
        if (lenVal < fieldLen) {
            for (int k = toCopy; k < fieldLen; k++) {
                outBuffer[ofs + k] = '@';
            }
        }

        // Debug: mostramos cómo queda el campo antes del '#'
        //printf("DEBUG: Campo %d (fijo): '", i);
        //for (int k = 0; k < fieldLen; ++k) {
        //    if (k < toCopy) putchar(tok[k]);
        //    else putchar('@');
        //}
        //printf("'\n");

        ofs += fieldLen;

        // Insertamos el separador '#' justo después de cada campo
        outBuffer[ofs] = '#';
        ofs += 1;

        // Debug: mostramos el '#' insertado
        //printf("DEBUG: Añadido separador '#'\n");

        // Continuamos al siguiente token
        tok = strtok(NULL, "#");
    }

    *outLen = ofs;

    // Mostrar el registro fijo completo (con '#' tras cada campo)
    //printf("DEBUG: Registro fijo generado (longitud %d): [", *outLen);
    //for (int i = 0; i < *outLen; ++i) {
    //    putchar(outBuffer[i]);
    //}
    //printf("]\n");

    return true;
}
// -----------------------------------------------------------------------------
// Función: eliminarRegistro
// Objetivo de la función:
//     Eliminar un registro en una posición global de una tabla paginada o en disco,
//     actualizando bitmap, volcando bloques y directorios según sea página o disco.
// Input:
//     const char* relacion      – nombre de la relación.
//     int posicionGlobal        – índice global del registro a borrar (1-based).
//     Disco disco               – objeto Disco para operaciones de bloque/sector.
//     bool esPagina             – true si opera en bufferpool, false si en disco físico.
// Output:
//     bool  – true si se completa la eliminación; false en error.
// Autor: Alex Cañapataña
// -----------------------------------------------------------------------------
static bool eliminarRegistro(const char* relacion, int posicionGlobal, Disco disco, bool esPagina = false) {
    printf("DEBUG: Entrando a eliminarRegistro para relación='%s', posiciónGlobal=%d\n",
        relacion, posicionGlobal);

    const char* discoNuevoPath = esPagina
        ? "BUFFERPOOL\\BLOQUES"
        : discoPath;

    // 1) Determinar en qué bloque está la posiciónGlobal.
    int bloqueN = 0;
    int rem = posicionGlobal;
    int numMaxAnt = 0;
    size_t raw_header_len = 0;
    char rutaBloque[MAX_PATH_LEN];

    const char* rutaBaseBloque = esPagina
        ? "BUFFERPOOL\\BLOQUES\\"
        : rutaBloque;

    while (true) {
        bloqueN++;
        snprintf(rutaBloque, sizeof(rutaBloque),
            "DISCO\\BLOQUES\\Bloque%d.txt", bloqueN);
        FILE* fblocTest = fopen(rutaBloque, "rb");
        if (!fblocTest) {
            printf("ERROR: No existe Bloque%d.txt; posiciónGlobal fuera de rango.\n", bloqueN);
            return false;
        }
        {
            // Leer solo la cabecera hasta '/' para extraer numMaxAnt
            size_t headerLen = 0;
            int ch;
            rewind(fblocTest);
            while ((ch = fgetc(fblocTest)) != EOF) {
                headerLen++;
                if (ch == '/') break;
                if (headerLen >= MAX_BUF - 1) break;
            }
            if (ch != '/') {
                fprintf(stderr, "ERROR: Cabecera corrupta en Bloque%d\n", bloqueN);
                fclose(fblocTest);
                return false;
            }
            raw_header_len = (headerLen > MAX_BUF - 1 ? MAX_BUF - 1 : headerLen);
        }
        rewind(fblocTest);
        char cabBuf[MAX_BUF];
        fread(cabBuf, 1, raw_header_len, fblocTest);
        cabBuf[raw_header_len] = '\0';

        {
            // Parsear "<numRegAnt>#<numMaxAnt>#<bitmap>/"
            char* p1 = strchr(cabBuf, '#');
            if (!p1) { fclose(fblocTest); return false; }
            char* p2 = p1 + 1;
            char* p3 = strchr(p2, '#');
            if (!p3) { fclose(fblocTest); return false; }
            *p3 = '\0';
            numMaxAnt = atoi(p2);
            *p3 = '#';
        }

        fclose(fblocTest);

        if (rem <= numMaxAnt) {
            printf("DEBUG: La posiciónGlobal %d cae en Bloque%d (numMaxAnt=%d)\n",
                posicionGlobal, bloqueN, numMaxAnt);
            break;
        }
        rem -= numMaxAnt;
    }
    int idxLocal = rem - 1; // 0-based dentro del bloque
    printf("DEBUG: posicionLocal en Bloque%d = %d (índice 0-based)\n", bloqueN, idxLocal);

    // 2) Abrir BloqueN.txt y eliminar registro local
    FILE* fbloc = fopen(rutaBloque, "r+b");
    if (!fbloc) {
        perror("ERROR: No se pudo abrir BloqueN.txt");
        return false;
    }

    // 2.1) Leer cabecera para extraer raw_header_len, numRegAnt, numMaxAnt y bitmap[]
    char cabTmp[MAX_BUF];
    int numRegAnt = 0;
    static char bitmap[MAX_BUF];

    {
        size_t pos = 0;
        int ch;
        rewind(fbloc);
        while ((ch = fgetc(fbloc)) != EOF) {
            pos++;
            if (ch == '/') break;
            if (pos >= MAX_BUF - 1) break;
        }
        if (ch != '/') {
            fprintf(stderr, "ERROR: Cabecera corrupta o falta '/' en Bloque%d\n", bloqueN);
            fclose(fbloc);
            return false;
        }
        raw_header_len = (pos > MAX_BUF - 1 ? MAX_BUF - 1 : pos);
    }
    rewind(fbloc);
    fread(cabTmp, 1, raw_header_len, fbloc);
    cabTmp[raw_header_len] = '\0';

    {
        // Parsear "<numRegAnt>#<numMaxAnt>#<bitmap>/"
        char* p1 = strchr(cabTmp, '#');
        *p1 = '\0';
        numRegAnt = atoi(cabTmp);
        *p1 = '#';
        printf("DEBUG: numRegAnt = %d\n", numRegAnt);

        char* p2 = p1 + 1;
        char* p3 = strchr(p2, '#');
        *p3 = '\0';
        // numMaxAnt ya lo tenemos de antes
        *p3 = '#';
        printf("DEBUG: numMaxAnt = %d\n", numMaxAnt);

        char* p4 = p3 + 1;
        char* slash = strchr(p4, '/');
        size_t bmpLen = (size_t)(slash - p4);
        if ((int)bmpLen > numMaxAnt) bmpLen = numMaxAnt;
        strncpy(bitmap, p4, bmpLen);
        bitmap[bmpLen] = '\0';
        printf("DEBUG: bitmap resultante: '%s'\n", bitmap);
    }

    // 2.2) Verificar que el slot idxLocal esté ocupado
    if (idxLocal < 0 || idxLocal >= numMaxAnt) {
        fprintf(stderr, "ERROR: Índice local %d fuera de rango en Bloque%d\n", idxLocal, bloqueN);
        fclose(fbloc);
        return false;
    }
    if (bitmap[idxLocal] == '0') {
        printf("DEBUG: El registro local %d ya estaba eliminado (bitmap=0)\n", idxLocal + 1);
        fclose(fbloc);
        return false;
    }

    // 2.3) Actualizar bitmap y numRegAnt (numMaxAnt permanece igual)
    bitmap[idxLocal] = '0';
    numRegAnt--;
    printf("DEBUG: bitmap[%d] cambiado a '0', numRegAnt ahora = %d\n", idxLocal, numRegAnt);

    // 2.4) Reconstruir la cabecera EXACTA de longitud raw_header_len
    {
        char newHeader[MAX_BUF];
        int ofh = 0;
        ofh += snprintf(newHeader + ofh, MAX_BUF - ofh, "%d#%d#", numRegAnt, numMaxAnt);
        for (int i = 0; i < numMaxAnt && ofh < (int)(raw_header_len - 1); i++) {
            newHeader[ofh++] = bitmap[i];
        }
        newHeader[ofh++] = '/';
        while (ofh < (int)raw_header_len) {
            newHeader[ofh++] = ' ';
        }
        newHeader[ofh] = '\0';

        rewind(fbloc);
        fwrite(newHeader, 1, raw_header_len, fbloc);
        fflush(fbloc);
        printf("DEBUG: Cabecera de Bloque%d reescrita: '%.*s'\n",
            bloqueN, (int)raw_header_len, newHeader);
    }

    // 2.5) Eliminar registro: obtener registroSize
    int registroSize = 0;
    obtenerRegistroSize(relacion, &registroSize);
    if (registroSize <= 0) {
        fprintf(stderr, "ERROR: No se encontró registroSize para '%s'\n", relacion);
        fclose(fbloc);
        return false;
    }
    long offsetRegistro = (long)raw_header_len + (long)idxLocal * (registroSize + 1);
    printf("DEBUG: offsetRegistro en Bloque%d = %ld\n", bloqueN, offsetRegistro);

    // 2.6) Reemplazar los bytes del registro con '@'
    if (fseek(fbloc, offsetRegistro, SEEK_SET) != 0) {
        fprintf(stderr, "ERROR: fseek falló al offsetRegistro\n");
        fclose(fbloc);
        return false;
    }
    {
        char* relleno = (char*)malloc(registroSize);
        memset(relleno, '@', registroSize);
        size_t escritos = fwrite(relleno, 1, registroSize, fbloc);
        free(relleno);
        if ((int)escritos != registroSize) {
            fprintf(stderr, "ERROR: Se escribieron %zu bytes, esperados %d\n", escritos, registroSize);
            fclose(fbloc);
            return false;
        }
    }
    fflush(fbloc);
    printf("DEBUG: Registro de %d bytes en Bloque%d reemplazado por '@'\n", registroSize, bloqueN);

    // 2.7) Calcular sector físico donde residía ese registro
    int tamSector = disco.getTamSector();
    long byteOffset = offsetRegistro;
    int sectorIndex = (int)(byteOffset / tamSector);
    printf("DEBUG: byteOffset=%ld => sectorIndex=%d (0-based)\n", byteOffset, sectorIndex);

    // 2.8) Obtener código del sector desde dirBloques.txt para BloqueN
    FILE* fdir = fopen(rutaDirBloques, "r");
    if (!fdir) {
        perror("ERROR: No se pudo abrir dirBloques.txt");
        fclose(fbloc);
        return false;
    }
    char sectorCodeEncontrado[MAX_STR_LEN] = { 0 };
    {
        // Leemos bloque por bloque usando leerBloqueConSeparador
        int lineaNum = 0;
        char bufferBlock[MAX_BUF];
        while (true) {
            int lenDB = disco.leerBloqueConSeparador(fdir, bufferBlock, MAX_BUF);
            if (lenDB <= 0) break;
            lineaNum++;
            if (lineaNum != bloqueN) continue;

            // bufferBlock tiene exactamente "|...|" sin '\0'
            bufferBlock[lenDB] = '\0';
            // Extraer lista de sectores tras "#_"
            char* pList = strstr(bufferBlock, "#_");
            if (!pList) break;
            pList += 2;
            int cuenta = 0;
            const char* p = pList;
            while (*p && *p != '|') {
                // extraer espacio
                int espSec = atoi(p);
                while (*p && *p != '#') ++p;
                if (!*p) break;
                ++p; // inicio de sectorCod

                // extraer sectorCod
                char sectorCod[MAX_STR_LEN] = { 0 };
                int j = 0;

                while (*p && *p != '#') {
                    if (j < MAX_STR_LEN - 1) sectorCod[j++] = *p;
                    ++p;
                }

                sectorCod[j] = '\0';
                if (!*p) break;
                ++p; // p apunta a '_'
                if (*p == '_') ++p;

                if (cuenta == sectorIndex) {
                    strncpy(sectorCodeEncontrado, sectorCod, MAX_STR_LEN - 1);
                    sectorCodeEncontrado[MAX_STR_LEN - 1] = '\0';
                    break;
                }
                cuenta++;
            }
            break;
        }
    }
    fclose(fdir);

    if (sectorCodeEncontrado[0]) {
        int p, s, pi, se;
        if (sscanf(sectorCodeEncontrado, "%d/%d/%d/%d", &p, &s, &pi, &se) == 4) {
            printf("DEBUG: El registro estaba en sector físico '%s' → Plato %d, Pista %d, Sector %d\n",
                sectorCodeEncontrado, p, pi, se);
        }
        else {
            printf("DEBUG: Código de sector '%s' malformado\n", sectorCodeEncontrado);
        }
    }
    else {
        printf("DEBUG: No se encontró sectorCode para sectorIndex=%d\n", sectorIndex);
    }

    // 2.9) Después de eliminar, volcar BloqueN a sectores para que el cambio se refleje
    printf("DEBUG: Llamando a volcarBloqueASectores tras eliminación en Bloque %d\n", bloqueN);
    
    if (!esPagina) {
        disco.volcarBloqueASectores(bloqueN);
    }

    fclose(fbloc);

    // 3) Actualizar dirBloques.txt: sumar espacio libre a bloque y al sector correspondiente
    int fixedLen = disco.obtenerLongitudMaximaBloque1();
    if (fixedLen <= 0) {
        fprintf(stderr, "ERROR: No se pudo obtener fixedLen para dirBloques\n");
        return true; // Ya eliminamos el registro en BloqueN.txt
    }
    printf("DEBUG: fixedLen (dirBloques) = %d\n", fixedLen);

    fdir = fopen(rutaDirBloques, "r+");
    if (!fdir) {
        perror("ERROR: No se pudo abrir dirBloques.txt");
        return true;
    }

    bool actualizado = false;
    {
        int lineaNum = 0;
        char bufferBlock[MAX_BUF];
        long posDB;
        while (true) {
            posDB = ftell(fdir);
            int lenDB = disco.leerBloqueConSeparador(fdir, bufferBlock, MAX_BUF);
            printf("DEBUG: ftell(fdir) = %ld, lenDB = %d\n", posDB, lenDB);
            if (lenDB <= 0) {
                printf("DEBUG: leerBloqueConSeparador devolvió %d, saliendo del bucle\n", lenDB);
                break;
            }
            lineaNum++;
            printf("DEBUG: lineaNum = %d, bloqueN = %d\n", lineaNum, bloqueN);
            if (lineaNum != bloqueN) continue;

            // bufferBlock es "|...|" de longitud lenDB
            bufferBlock[lenDB] = '\0';
            printf("DEBUG: Línea original en dirBloques (bloque %d): '%s'\n", bloqueN, bufferBlock);

            // Extraer espacioLibreBloqueDir (entre '|' y primer '#')
            char temp[MAX_BUF];
            strncpy(temp, bufferBlock + 1, lenDB - 2);
            temp[lenDB - 2] = '\0';
            char* ph = strchr(temp, '#');
            *ph = '\0';
            int espacioLibreBloqueDir = safe_atoi(temp);
            *ph = '#';
            int nuevoBloqueEsp = espacioLibreBloqueDir + registroSize;
            int tamBloqueFis = getTamBloqueFromDisco(disco);
            printf("DEBUG: espacioLibreBloqueDir = %d, nuevoBloqueEsp = %d, tamBloqueFis = %d\n",
                espacioLibreBloqueDir, nuevoBloqueEsp, tamBloqueFis);

            // Encontrar inicio de la lista de sectores en bufferBlock
            char* inicioSectores = strstr(bufferBlock, "#_");
            if (!inicioSectores) {
                printf("ERROR: No se encontró '#_' en bufferBlock: '%s'\n", bufferBlock);
            }
            inicioSectores += 2;
            printf("DEBUG: inicioSectores apunta a: '%s'\n", inicioSectores);

            // Construir la nueva línea en un buffer de tamaño fixedLen
            char nuevoBloqueStr[MAX_BUF] = { 0 };
            int ofs = 0;
            // 1) '|' inicial
            nuevoBloqueStr[ofs++] = '|';
            // 2) montar el prefijo actualizado
            int nPref = snprintf(nuevoBloqueStr + ofs, MAX_BUF - ofs,
                "%d#2#BLOQUE#%d#%d#_", nuevoBloqueEsp, bloqueN, tamBloqueFis);
            printf("DEBUG: snprintf prefijo devolvió %d\n", nPref);
            ofs += nPref;
            printf("DEBUG: Prefijo nuevoBloqueStr: '%.*s'\n", ofs, nuevoBloqueStr);

            // 3) A partir de inicioSectores, iterar cada par "<esp>#<cod>#_"
            const char* p = inicioSectores;
            int cuenta = 0;
            while (*p && *p != '|') {
                if (!isdigit(*p)) break;
                int espSec = atoi(p);
                while (*p && *p != '#') ++p;
                if (!*p) {
                    printf("ERROR: Fin de cadena buscando '#' tras espSec\n");
                    break;
                }
                ++p; // p apunta a inicio de sectorCod

                char sectorCod[MAX_STR_LEN] = { 0 };
                int j = 0;
                while (*p && *p != '#') {
                    if (j < MAX_STR_LEN - 1) sectorCod[j++] = *p;
                    ++p;
                }
                sectorCod[j] = '\0';
                if (!*p) {
                    printf("ERROR: Fin de cadena buscando '#' tras sectorCod\n");
                    break;
                }
                ++p; // p apunta a '_'
                if (*p == '_') ++p;

                printf("DEBUG: Par sector #%d: espSec=%d, sectorCod='%s'\n", cuenta, espSec, sectorCod);

                int espSecNuevo = espSec;
                if (cuenta == sectorIndex) {
                    espSecNuevo = espSec + registroSize;
                    printf("DEBUG: SectorIndex coincide (%d); espSecNuevo = %d\n", sectorIndex, espSecNuevo);
                }

                int n = snprintf(nuevoBloqueStr + ofs, MAX_BUF - ofs,
                    "%d#%s#_", espSecNuevo, sectorCod);
                printf("DEBUG: Añadiendo \"%d#%s#_\" => n = %d\n", espSecNuevo, sectorCod, n);
                ofs += n;

                cuenta++;
            }
            printf("DEBUG: Después de procesar sectores, ofs = %d\n", ofs);

            // 4) Cerrar con '|' final y rellenar con '@' hasta fixedLen - 1
            if (ofs < fixedLen - 1) {
                memset(nuevoBloqueStr + ofs, '@', fixedLen - 1 - ofs);
                nuevoBloqueStr[fixedLen - 1] = '|';
            }
            else {
                nuevoBloqueStr[fixedLen - 1] = '|';
            }
            printf("DEBUG: nuevoBloqueStr final (fixedLen=%d): '%.*s'\n",
                fixedLen, fixedLen, nuevoBloqueStr);

            // 5) Sobreescribir exactamente fixedLen bytes en dirBloques.txt
            fseek(fdir, posDB, SEEK_SET);
            size_t escritos = fwrite(nuevoBloqueStr, 1, fixedLen, fdir);
            fflush(fdir);
            printf("DEBUG: fwrite sobre dirBloques escribió %zu bytes (esperados %d)\n",
                escritos, fixedLen);
            actualizado = true;
            break;
        }
    }
    fclose(fdir);

    if (actualizado) {
        printf("DEBUG: dirBloques.txt actualizado correctamente para Bloque%d\n", bloqueN);
    }
    else {
        printf("ERROR: No se actualizó dirBloques.txt para Bloque%d\n", bloqueN);
    }

    return true;
}
// -----------------------------------------------------------------------------
// Función: modificarRegistro
// Objetivo de la función:
//     Modificar un registro en una posición global: elimina el antiguo y escribe
//     el nuevo RLF correspondiente, actualizando bloques y volcando cambios.
// Input:
//     const char* relacion      – nombre de la relación.
//     int posicion              – índice global del registro a modificar.
//     const char* nuevoRegistroTxt – texto del nuevo registro separado por ‘#’.
//     Disco disco               – objeto Disco para operaciones de bloque/sector.
//     bool esPagina             – true si opera en bufferpool, false si en disco físico.
// Output:
//     bool  – true si la modificación fue exitosa; false en caso contrario.
// Autor: Alex Cañapataña
// -----------------------------------------------------------------------------
static bool modificarRegistro(const char* relacion, int posicion, const char* nuevoRegistroTxt, Disco disco, bool esPagina = false) {
    printf("DEBUG: Entrando a modificarRegistro para relación='%s', posición=%d\n", relacion, posicion);

    // 1) Determinar en qué bloque y qué índice local corresponde a la posición global.
    int bloqueN = 0;
    int rem = posicion;
    int numMaxAnt = 0;
    size_t raw_header_len = 0;
    char rutaBloque[MAX_PATH_LEN];

    while (true) {
        bloqueN++;
        snprintf(rutaBloque, sizeof(rutaBloque),
            "DISCO\\BLOQUES\\Bloque%d.txt", bloqueN);
        FILE* ftest = fopen(rutaBloque, "rb");
        if (!ftest) {
            printf("ERROR: No existe Bloque%d.txt; posición %d fuera de rango.\n", bloqueN, posicion);
            return false;
        }
        // Leer solo hasta '/' para extraer numMaxAnt
        size_t headerLen = 0;
        int ch;
        rewind(ftest);
        while ((ch = fgetc(ftest)) != EOF) {
            headerLen++;
            if (ch == '/') break;
            if (headerLen >= MAX_BUF - 1) break;
        }
        if (ch != '/') {
            fprintf(stderr, "ERROR: Cabecera corrupta en Bloque%d\n", bloqueN);
            fclose(ftest);
            return false;
        }
        raw_header_len = (headerLen > MAX_BUF - 1 ? MAX_BUF - 1 : headerLen);

        rewind(ftest);
        char cabBuf[MAX_BUF];
        fread(cabBuf, 1, raw_header_len, ftest);
        cabBuf[raw_header_len] = '\0';

        char* p1 = strchr(cabBuf, '#');
        if (!p1) { fclose(ftest); return false; }
        char* p2 = p1 + 1;
        char* p3 = strchr(p2, '#');
        if (!p3) { fclose(ftest); return false; }
        *p3 = '\0';
        numMaxAnt = atoi(p2);
        *p3 = '#';

        fclose(ftest);

        if (rem <= numMaxAnt) break;
        rem -= numMaxAnt;
    }
    int idxLocal = rem - 1; // índice 0-based dentro del bloque
    printf("DEBUG: La posición %d cae en Bloque%d, idxLocal=%d\n", posicion, bloqueN, idxLocal);

    // 2) Eliminar el registro antiguo en esa posición global
    if (!eliminarRegistro(relacion, posicion, disco, false)) {
        printf("ERROR: eliminarRegistro falló para posición %d\n", posicion);
        return false;
    }
    printf("DEBUG: eliminarRegistro completado (Bloque%d, idxLocal=%d)\n", bloqueN, idxLocal);

    // 3) Generar RLF del nuevo registro
    char regBuf[MAX_BUF];
    int regLen = 0;
    if (!crearRLF(nuevoRegistroTxt, relacion, regBuf, &regLen)) {
        printf("ERROR: crearRLF falló para modificación.\n");
        return false;
    }

    int registroSize = 0;
    obtenerRegistroSize(relacion, &registroSize);
    if (registroSize <= 0) {
        fprintf(stderr, "ERROR: No se encontró registroSize para '%s'\n", relacion);
        return false;
    }

    // 4) Reabrir BloqueN.txt para insertar en idxLocal
    FILE* fbloc = fopen(rutaBloque, "r+b");
    if (!fbloc) {
        perror("ERROR: No se pudo reabrir BloqueN.txt");
        return false;
    }
    // 4.1) Leer cabecera completa hasta '/'
    char cabTmp[MAX_BUF];
    rewind(fbloc);
    size_t pos = 0;
    int c;
    while ((c = fgetc(fbloc)) != EOF) {
        pos++;
        if (c == '/') break;
        if (pos >= MAX_BUF - 1) break;
    }
    if (c != '/') {
        std::cerr << "ERROR: Cabecera corrupta al reabrir Bloque" << bloqueN << "\n";
        fclose(fbloc);
        return false;
    }
    raw_header_len = (pos > MAX_BUF - 1 ? MAX_BUF - 1 : pos);
    rewind(fbloc);
    fread(cabTmp, 1, raw_header_len, fbloc);
    cabTmp[raw_header_len] = '\0';

    // Extraer numRegAnt, numMaxAnt y bitmap
    int numRegAnt = 0, numMax = 0;
    static char bitmap[MAX_BUF];
    char* q1 = strchr(cabTmp, '#');
    *q1 = '\0';
    numRegAnt = atoi(cabTmp);
    *q1 = '#';
    char* q2 = q1 + 1;
    char* q3 = strchr(q2, '#');
    *q3 = '\0';
    numMax = atoi(q2);
    *q3 = '#';
    char* q4 = q3 + 1;
    char* slash4 = strchr(q4, '/');
    size_t bmpLen = (size_t)(slash4 - q4);
    if ((int)bmpLen > numMax) bmpLen = numMax;
    strncpy(bitmap, q4, bmpLen);
    bitmap[bmpLen] = '\0';

    // 4.2) Marcar bitmap[idxLocal]='1' y numRegAnt++
    bitmap[idxLocal] = '1';
    numRegAnt++;
    printf("DEBUG: (modificar) bitmap actualizado: '%.*s'\n", numMax, bitmap);

    // Reconstruir cabecera exacta al mismo largo raw_header_len
    char newHead[MAX_BUF];
    int ofh = 0;
    ofh += snprintf(newHead + ofh, MAX_BUF - ofh, "%d#%d#", numRegAnt, numMax);
    for (int i = 0; i < numMax && ofh < (int)(raw_header_len - 1); i++) {
        newHead[ofh++] = bitmap[i];
    }
    newHead[ofh++] = '/';
    while (ofh < (int)raw_header_len) {
        newHead[ofh++] = ' ';
    }
    newHead[ofh] = '\0';

    rewind(fbloc);
    fwrite(newHead, 1, raw_header_len, fbloc);
    fflush(fbloc);
    printf("DEBUG: Cabecera de Bloque%d reescrita (modificar): '%.*s'\n",
        bloqueN, (int)raw_header_len, newHead);

    // 4.3) Calcular offset fijo y escribir regBuf + '|'
    long offsetRegistro = (long)raw_header_len + (long)idxLocal * (registroSize + 1);
    if (fseek(fbloc, offsetRegistro, SEEK_SET) != 0) {
        fprintf(stderr, "ERROR: fseek falló al offsetRegistro %ld\n", offsetRegistro);
        fclose(fbloc);
        return false;
    }
    fwrite(regBuf, 1, registroSize, fbloc);
    fputc('|', fbloc);
    fflush(fbloc);
    fclose(fbloc);
    printf("DEBUG: Nuevo RLF escrito en Bloque%d, idxLocal=%d\n", bloqueN, idxLocal);

    // 5) Volcar BloqueN a sectores para que el cambio se refleje en disco físico
    printf("DEBUG: Llamando a volcarBloqueASectores (modificar) para Bloque%d\n", bloqueN);
    if (!esPagina) {
        disco.volcarBloqueASectores(bloqueN);
    }

    // 6) No es necesario ajustar dirBloques.txt de nuevo, porque eliminarRegistro ya sumó al espacio libre
    //    y aquí volvimos a ocupar exactamente ese hueco, de modo que el espacio libre total queda igual.
    printf("DEBUG: modificarRegistro completado para Bloque%d, posición %d\n", bloqueN, posicion);
    return true;
}
// -----------------------------------------------------------------------------
// Función: adicionarRegistroUnicoBitmap
// Objetivo de la función:
//     (Stub) Insertar un registro en modo bitmap — pendiente de implementar.
// Input:
//     const char* nombreRel     – nombre de la relación.
//     const char* registroTxt   – campos separados por ‘#’.
// Output:
//     bool  – false (no implementado).
// Autor: Alex Cañapataña
// -----------------------------------------------------------------------------
static bool adicionarRegistroUnicoBitmap(const char* nombreRel, const char* registroTxt) {
    return false;
}

//////////////////// insertar de forma fija

/// #P1#Works#BeforeTheCorruption
// -----------------------------------------------------------------------------
// Función: adicionarRegistroUnico
// Objetivo de la función:
//     Insertar un registro fijo o variable en un bloque, manejando creación,
//     actualización de bitmap, reutilización o anexado según el caso.
// Input:
//     const char* registroTxt  – texto con campos separados por ‘#’.
//     const char* relacion     – nombre de la relación.
//     Disco& disco             – objeto Disco para operaciones de bloque.
//     bool esPagina            – true para bufferpool, false para disco físico.
// Output:
//     bool  – true si la inserción tuvo éxito; false en error.
// Autor: Alex Cañapataña
// -----------------------------------------------------------------------------
static bool adicionarRegistroUnico(const char* registroTxt, const char* relacion, Disco& disco, bool esPagina = false) {

    const char* discoNuevoPath = esPagina
        ? bufferPoolPath
        : discoPath;


    // --- 0) Preparar datos y calcular longitud fija de bloque ---
    int registroSize;
    static char regBuf[MAX_BUF]; // Buffer para el RLF
    static int regLen;           // Longitud de regBuf


    // 0.1) Debug entrada
    //printf("DEBUG: Entrando a adicionarRegistroUnico para relacion='%s', registroTxt='%s'\n",
    //    relacion, registroTxt);

    // 0.2) Copia segura de registroTxt sin CR/LF
    char registroSinLF[MAX_BUF];
    strncpy(registroSinLF, registroTxt, MAX_BUF - 1);
    registroSinLF[MAX_BUF - 1] = '\0';
    size_t l = strlen(registroSinLF);
    while (l > 0 && (registroSinLF[l - 1] == '\n' || registroSinLF[l - 1] == '\r')) {
        registroSinLF[--l] = '\0';
    }

    // 0.3) Generar RLF (registro de longitud fija)
    if (!crearRLF(registroSinLF, relacion, regBuf, &regLen)) {
        //printf("DEBUG: Error en crearRLF. Saliendo.\n");
        return false;
    }
    //printf("DEBUG: crearRLF terminó correctamente. regLen = %d\n", regLen);
    // Verificar que no haya saltos de línea en regBuf[0..regLen-1]
    for (int i = 0; i < regLen; ++i) {
        if (regBuf[i] == '\n' || regBuf[i] == '\r') {
            printf("ERROR: detectado salto de línea en regBuf en posición %d\n", i);
        }
    }

    // 0.4) Mostrar contenido de longitudFija.txt (solo debug)
    //printf(">>> Debug: Leyendo %s para ver su contenido:\n", rutaLongitudFija);
    FILE* ftmp = fopen(rutaLongitudFija, "r");
    if (ftmp) {
        char buf[MAX_BUF];
        while (fgets(buf, MAX_BUF, ftmp)) {
            printf("   %s", buf);
        }
        fclose(ftmp);
    }
    else {
        printf("   ¡ERROR: no se pudo abrir %s!\n", rutaLongitudFija);
    }

    // 0.5) Obtener registroSize
    obtenerRegistroSize(relacion, &registroSize);
    //printf("DEBUG: registroSize calculado = %d\n", registroSize);
    if (registroSize <= 0) {
        fprintf(stderr, "ERROR: No se encontró longitud fija para %s\n", relacion);
        return false;
    }

    // 0.6) Calcular longitud fija máxima de "bloque" en dirBloques.txt
    int fixedLen = disco.obtenerLongitudMaximaBloque1();
    if (fixedLen <= 0) {
        printf("ERROR: No se pudo obtener longitud fija máxima de bloque.\n");
        return false;
    }
    //printf("DEBUG: fixedLen (longitud de cada bloque) = %d bytes\n", fixedLen);

    // 1) Abrir dirBloques.txt en modo lectura y escritura
    //printf("DEBUG: intentando abrir dirBloques en '%s'\n", rutaDirBloques);
    FILE* fdir = fopen(rutaDirBloques, "r+");
    if (!fdir) {
        perror("ERROR: No se puede abrir dirBloques.txt");
        return false;
    }
    //printf("DEBUG: dirBloques.txt abierto con éxito.\n");

    printf("DEBUG: Buscando bloque para '%s' en %s\n",
        relacion,
        esPagina ? "BUFFERPOOL" : "DISCO");

    char lineaBloque[MAX_BUF];
    int nroBloque = 0;
    bool foundBlock = false;
    char codSectorLibre[MAX_STR_LEN] = { 0 };
    long posLineaBloque = 0;

    int tamRegistro = registroSize;
    int espacioLibreBloque = 0;
    int tamUtilAntes = 0;
    int espacioLibreSectorAntes = 0;

    char savedSectores[MAX_BUF] = { 0 }; // Para guardar sectores del bloque elegido

    printf("DEBUG: Entrando al bucle de búsqueda de bloque para relacion='%s'\n", relacion);
    // 2) Buscar bloque y sector libres (cada bloque ocupa fixedLen bytes total)
    //printf("DEBUG: Buscando bloque libre para tamaño de registro = %d\n", tamRegistro);
    while (true) {
        posLineaBloque = ftell(fdir);

        int lenBloque = disco.leerBloqueConSeparador(fdir, lineaBloque, MAX_BUF);
        if (lenBloque <= 0) {
            //printf("DEBUG: leerBloqueConSeparador devolvió %d (fin de bloques)\n", lenBloque);
            break;
        }

        // 3) Avanzamos el contador de bloques
        nroBloque++;
        printf("DEBUG: Procesando Bloque #%d\n", nroBloque);

        // 4) SALTO: si ya está asignado a otra relación, lo omitimos
        const char* relAsig = obtenerRelacionDeBloque(rutaCatalogo, nroBloque);
        printf("DEBUG: Bloque#%d asignado a '%s' (o NULL si no existe)\n",
            nroBloque, relAsig ? relAsig : "NULL");
        if (relAsig && strcmp(relAsig, relacion) != 0) {
            printf("DEBUG: Bloque#%d NO es de '%s', salto al siguiente\n", nroBloque, relacion);
            continue;
        }

        // Saltar bloques vacíos generados por "||"
        if (lenBloque == 2 && lineaBloque[0] == '|' && lineaBloque[1] == '|') {
            continue;
        }

        printf("DEBUG: Leyendo Bloque #%d (lenBloque = %d bytes)\n", nroBloque, lenBloque);

        // 2.1) Extraer espacioLibreBloque sin strtok:
        char tempEspacio[MAX_BUF];
        strncpy(tempEspacio, lineaBloque + 1, MAX_BUF - 1);
        tempEspacio[MAX_BUF - 1] = '\0';

        char* posHash = strchr(tempEspacio, '#');
        if (!posHash) {
            //printf("DEBUG: Bloque #%d sin token inicial. Continúa.\n", nroBloque);
            continue;
        }
        *posHash = '\0';
        espacioLibreBloque = safe_atoi(tempEspacio);
        *posHash = '#';
        //printf("DEBUG: Bloque #%d espacioLibreBloque = %d\n", nroBloque, espacioLibreBloque);

        tamUtilAntes = getTamBloqueFromDisco(disco) - espacioLibreBloque;
        if (espacioLibreBloque < tamRegistro) {
            //printf("DEBUG: Bloque #%d NO cabe (espacio %d < %d). Pasa al siguiente.\n",
            //    nroBloque, espacioLibreBloque, tamRegistro);
            continue;
        }

        // 2.2) Extraer la lista de sectores en copiaSectores
        char copiaSectores[MAX_BUF];
        strncpy(copiaSectores, lineaBloque + 1, MAX_BUF - 1);
        copiaSectores[MAX_BUF - 1] = '\0';

        char* p = strstr(copiaSectores, "#_");
        if (!p) {
            //printf("DEBUG: Bloque #%d no tiene '#_' (no hay lista de sectores). Continúa.\n", nroBloque);
            continue;
        }
        p += 2;

        // 2.3) Iterar cada par "<espLibreSector>#<codSector>#_"
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

            //printf("DEBUG: Bloque #%d chequeando sector '%s' con espacio %d\n",
            //    nroBloque, sectorCode, espacioLibreSector);

            char* nextPair = strstr(p, "#_");
            if (espacioLibreSector < tamRegistro) {
                //printf("DEBUG: Sector '%s' no cabe (espacio %d < %d). Siguiente.\n",
                //    sectorCode, espacioLibreSector, tamRegistro);
                if (!nextPair) break;
                p = nextPair + 2;
                continue;
            }

            // Sector válido encontrado
            strncpy(codSectorLibre, sectorCode, MAX_STR_LEN - 1);
            espacioLibreSectorAntes = espacioLibreSector;
            // Guardar la lista completa de "sectores" para usarla después
            strncpy(savedSectores, copiaSectores, MAX_BUF - 1);
            savedSectores[MAX_BUF - 1] = '\0';
            foundBlock = true;
            //printf("DEBUG: Seleccionado Bloque #%d, Sector '%s'. espacioLibreSectorAntes = %d\n",
            //    nroBloque, codSectorLibre, espacioLibreSectorAntes);
            break;
        }
        if (foundBlock) break;
    }

    if (!foundBlock) {
        //printf("DEBUG: No se encontró ningún bloque con espacio suficiente.\n");
        fclose(fdir);
        return false;
    }

    // 2.4) Calcular nuevo espacio de bloque
    int espacioLibreBloqueAntes = espacioLibreBloque;
    int tamUtilNuevo = tamUtilAntes + tamRegistro;
    int espacioBloqueNuevo = getTamBloqueFromDisco(disco) - tamUtilNuevo;
    //printf("DEBUG: Bloque #%d espacioLibreBloqueAntes = %d  => espacioBloqueNuevo = %d\n",
    //    nroBloque, espacioLibreBloqueAntes, espacioBloqueNuevo);
    if (espacioBloqueNuevo < 0 || espacioBloqueNuevo > getTamBloqueFromDisco(disco)) {
        printf("ERROR: valores fuera de rango en Bloque #%d: espacioBloqueNuevo = %d\n",
            nroBloque, espacioBloqueNuevo);
    }

    // 3) Volver al inicio exacto del bloque en dirBloques.txt (posición en bytes)
    //printf("DEBUG: Volviendo a byte offset %ld para sobreescribir bloque #%d\n",
    //    posLineaBloque, nroBloque);

    // 4) Reconstruir el bloque completo de exactly fixedLen bytes
    char bufferNueva[MAX_BUF] = { 0 };
    int ofsN = 0;

    // 4.1) Escribir el '|' inicial
    bufferNueva[ofsN++] = '|';

    // 4.2) Prefijo actualizado "<espBloqueNuevo>#2#BLOQUE#<nroBloque>#<tamBloque>#_"
    ofsN += snprintf(bufferNueva + ofsN, MAX_BUF - ofsN,
        "%d#2#BLOQUE#%d#%d#_",
        espacioBloqueNuevo,
        nroBloque,
        getTamBloqueFromDisco(disco));
    //printf("DEBUG: bufferNueva hasta prefijo = '%.*s'\n", ofsN, bufferNueva);

    // 4.3) Copiar cada par "<esp>#<cod>#_", ajustando el sector elegido
    {
        char* p2 = strstr(savedSectores, "#_");
        if (p2) {
            p2 += 2; // al inicio del primer "<espLibreSector>"
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
                int n = snprintf(bufferNueva + ofsN, MAX_BUF - ofsN,
                    "%d#%s#_", nuevoEsp, sectorCode2);
                //printf("DEBUG: Añadiendo \"%d#%s#_\" => n = %d\n", nuevoEsp, sectorCode2, n);
                ofsN += n;

                char* next2 = strstr(p2, "#_");
                if (!next2) break;
                p2 = next2 + 2;
            }
        }
    }

    // 4.4) Poner padding '@' hasta fixedLen-1, y cerrar con '|'
    if (ofsN < fixedLen - 1) {
        for (int i = ofsN; i < fixedLen - 1; i++) {
            bufferNueva[i] = '@';
        }
        bufferNueva[fixedLen - 1] = '|';
    }
    else {
        // Si por alguna razón se pasó, truncar y forzar '|' al final
        bufferNueva[fixedLen - 1] = '|';
    }



    //printf("DEBUG: Bloque reconstruido (fixedLen %d bytes):\n", fixedLen);
    //printf("       %.*s\n", fixedLen, bufferNueva);

    // 4.5) Sobreescribir exactamente fixedLen bytes en dirBloques.txt
    fseek(fdir, posLineaBloque, SEEK_SET);
    if (!esPagina) {
        fwrite(bufferNueva, 1, fixedLen, fdir);
        fflush(fdir);
    }
    else {
        disco.registrarCambioDirBloques(posLineaBloque, bufferNueva, fixedLen);
        registrarPaginaModificada(nroBloque);
    }
    fclose(fdir);
    //printf("DEBUG: Reescritura de bloque #%d en dirBloques.txt: %zu bytes escritos (esperados %d)\n",
    //    nroBloque, escritosDir, fixedLen);

    // -------------------------------------------------------------
    // 5) Ahora crear/abrir BloqueN.txt y aprovechar espacio libre (bitmap)
    // -------------------------------------------------------------
    char rutaBloque[MAX_PATH_LEN];
    if (!esPagina) {
        // modo DISCO
        snprintf(rutaBloque, sizeof(rutaBloque),
            "%sBLOQUES\\Bloque%d.txt",
            discoPath, nroBloque);
    }
    else {
        // modo PÁGINA (buffer)
        snprintf(rutaBloque, sizeof(rutaBloque),
            "%sPage%d.txt",
            bufferPagePath, nroBloque);
    }

    printf("DEBUG: Ruta BloqueN.txt = '%s'\n", rutaBloque);

    FILE* fbloc = fopen(rutaBloque, esPagina ? "r+b" : "r+b");
    size_t raw_header_len = 0;
    int numRegAnt = 0;
    int numMaxAnt = 0;
    static char bitmapAnt[MAX_BUF];

    if (!fbloc) {
        // Bloque no existe: crearlo con calcularCabeceraBloque(...)
        printf("DEBUG: BloqueN.txt NO existe. Se intentará crearlo.\n");
        char headerBuf[MAX_BUF];
        size_t headerLen;
        int numMax;
        calcularCabeceraBloque(getTamBloqueFromDisco(disco), registroSize,
            headerBuf, &headerLen, &numMax);
        //printf("DEBUG: calcularCabeceraBloque devolvió: headerLen=%zu, numMax=%d\n", headerLen, numMax);
        //printf("DEBUG: Contenido headerBuf: '%.*s'\n", (int)headerLen, headerBuf);

        fbloc = fopen(rutaBloque, "wb");
        if (!fbloc) {
            perror("ERROR: No se pudo crear BloqueN.txt");
            return false;
        }
        size_t escritosHeader = fwrite(headerBuf, 1, headerLen, fbloc);
        fflush(fbloc);
        if (escritosHeader != headerLen) {
            printf("ERROR: se escribieron %zu bytes de header, esperados %zu\n", escritosHeader, headerLen);
        }
        else {
            //printf("DEBUG: Cabecera escrita correctamente en BloqueN.txt (%zu bytes)\n", escritosHeader);
        }
        // Mover cursor justo antes del '|'
        fseek(fbloc, -1, SEEK_END);
        raw_header_len = headerLen;
        numRegAnt = 1;                // Ya insertamos este registro
        numMaxAnt = 0;                // Se determinará después
        for (int i = 0; i < numMaxAnt; i++) bitmapAnt[i] = '0';
        bitmapAnt[numMaxAnt] = '\0';

        // 10) Escribir el registro inmediatamente después de la cabecera
        long offsetInicial = raw_header_len;
        fseek(fbloc, offsetInicial, SEEK_SET);
        size_t escritosReg2 = fwrite(regBuf, 1, regLen, fbloc);
        fputc('|', fbloc);
        fflush(fbloc);
        if (escritosReg2 != (size_t)regLen) {
            printf("ERROR: Se escribieron %zu bytes de registro, esperados %d\n", escritosReg2, regLen);
        }
        else {
            //printf("DEBUG: Registro de %d bytes escrito correctamente (+ '|').\n", regLen);
        }
        fclose(fbloc);

        // 11) Volcar a sectores
        //printf("DEBUG: Llamando a volcarBloqueASectores para nuevo bloque #%d\n", nroBloque);
        if (!esPagina) {
            disco.volcarBloqueASectores(nroBloque);
        }

        // 12) Actualizar catalogo.txt
        char rutaCatalogo2[MAX_PATH_LEN];
        snprintf(rutaCatalogo2, sizeof(rutaCatalogo2),
            "%s%s", discoNuevoPath, "catalogo.txt");
        //printf("DEBUG: Abriendo catalogo.txt en modo 'a' para agregar %s|Bloque%d.txt\n",
        //    relacion, nroBloque);
        FILE* fcat2 = fopen(rutaCatalogo2, "a");
        if (fcat2) {
            char rutaBloqueCat[MAX_PATH_LEN];
            snprintf(rutaBloqueCat, sizeof(rutaBloqueCat),
                "%sBLOQUES\\Bloque%d.txt", discoNuevoPath, nroBloque);
            fprintf(fcat2, "%s|%s\n", relacion, rutaBloqueCat);
            fclose(fcat2);
            //printf("DEBUG: Entrada agregada a catalogo.txt: '%s|%s'\n", relacion, rutaBloqueCat);
        }
        else {
            perror("ERROR: No se pudo abrir catalogo.txt para escritura");
        }

        printf("DEBUG: adicionarRegistroUnico (nuevo bloque) finalizado para Bloque #%d\n", nroBloque);
        return true;
    }
    else {
        // Bloque existe: leer cabecera hasta '/'
        printf("DEBUG: BloqueN.txt ya existe. Releyendo cabecera...\n");
        size_t pos = 0;
        int c;
        rewind(fbloc);
        while ((c = fgetc(fbloc)) != EOF) {
            pos++;
            if (c == '/') break;
            if (pos >= MAX_BUF - 1) break;
        }
        raw_header_len = pos;
        printf("DEBUG: raw_header_len detectado = %zu\n", raw_header_len);

        rewind(fbloc);
        char cabTmp3[MAX_BUF];
        if (raw_header_len > MAX_BUF - 1) raw_header_len = MAX_BUF - 1;
        size_t leidosCab3 = fread(cabTmp3, 1, raw_header_len, fbloc);
        cabTmp3[leidosCab3] = '\0';
        //printf("DEBUG: Contenido leído de cabecera (%zu bytes): '%.*s'\n",
        //    leidosCab3, (int)leidosCab3, cabTmp3);

        // Parsear numRegAnt#numMaxAnt#bitmapAnt/
        char* q1 = strchr(cabTmp3, '#');
        *q1 = '\0';
        numRegAnt = safe_atoi(cabTmp3);
        *q1 = '#';
        char* q2 = q1 + 1;
        char* q3 = strchr(q2, '#');
        *q3 = '\0';
        numMaxAnt = safe_atoi(q2);
        *q3 = '#';
        char* q4 = q3 + 1;
        char* slash3 = strchr(q4, '/');
        size_t bmpLen3 = (size_t)(slash3 - q4);
        if (bmpLen3 >= MAX_BUF) bmpLen3 = MAX_BUF - 1;
        strncpy(bitmapAnt, q4, bmpLen3);
        bitmapAnt[bmpLen3] = '\0';
        //printf("DEBUG: (Anexar/Reuso) Lectura cabecera existente: numRegAnt=%d, numMaxAnt=%d, bitmapAnt='%s'\n",
        //    numRegAnt, numMaxAnt, bitmapAnt);

        // 6) Buscar slot libre en bitmapAnt[] para reuso
        int idxLibre = -1;
        for (int i = 0; i < numMaxAnt; i++) {
            if (bitmapAnt[i] == '0') {
                idxLibre = i;
                break;
            }
        }

        if (idxLibre >= 0) {
            // Reutilizar hueco: idxLibre
            //printf("DEBUG: Se encontró espacio libre en Bloque%d en índice %d\n", nroBloque, idxLibre);
            // 7) Actualizar numRegAnt y bitmapAnt[idxLibre]
            numRegAnt++;
            bitmapAnt[idxLibre] = '1';
            //printf("DEBUG: (Reuso) Nuevo numRegAnt = %d, bitmapAnt = '%s'\n", numRegAnt, bitmapAnt);

            // 8) Reconstruir cabecera EXACTA: poner espacios hasta raw_header_len-1, luego '/'
            rewind(fbloc);
            char newHeader2[MAX_BUF];
            int ofh2 = 0;
            ofh2 += snprintf(newHeader2 + ofh2, MAX_BUF - ofh2, "%d#%d#", numRegAnt, numMaxAnt);
            for (int i = 0; i < numMaxAnt && ofh2 < (int)(raw_header_len - 1); i++) {
                newHeader2[ofh2++] = bitmapAnt[i];
            }
            // Llenar con espacios hasta la posición raw_header_len-1
            while (ofh2 < (int)(raw_header_len - 1)) {
                newHeader2[ofh2++] = ' ';
            }
            // Poner '/' en la última posición de cabecera
            newHeader2[raw_header_len - 1] = '/';
            // No más caracteres
            newHeader2[raw_header_len] = '\0';
            //printf("DEBUG: (Reuso) Reconstruyendo cabecera: '%.*s'\n", (int)raw_header_len, newHeader2);

            // 9) Sobrescribir cabecera en BloqueN.txt
            fwrite(newHeader2, 1, raw_header_len, fbloc);
            fflush(fbloc);

            // 10) Calcular offset para insertar el registro reusado
            long offsetRegistroReuse = (long)raw_header_len + (long)idxLibre * (registroSize + 1);
            //printf("DEBUG: offsetRegistroReuse en Bloque%d = %ld\n", nroBloque, offsetRegistroReuse);

            // 11) Reemplazar '@' con regBuf en esa posición
            if (fseek(fbloc, offsetRegistroReuse, SEEK_SET) != 0) {
                fprintf(stderr, "ERROR: fseek falló al offsetRegistroReuse\n");
                fclose(fbloc);
                return false;
            }
            size_t escritosReuse = fwrite(regBuf, 1, regLen, fbloc);
            fputc('|', fbloc);
            fflush(fbloc);
            if (escritosReuse != (size_t)regLen) {
                //printf("ERROR: Se escribieron %zu bytes de RLF reusado, esperados %d\n", escritosReuse, regLen);
            }
            else {
                //printf("DEBUG: Registro de %d bytes reusado en posición %d (+ '|').\n", regLen, idxLibre);
            }

            fclose(fbloc);
            // 12) Volcar bloque reasignado a sectores
            //printf("DEBUG: Llamando a volcarBloqueASectores para bloque #%d (reuso)\n", nroBloque);

            if (!esPagina) {
                disco.volcarBloqueASectores(nroBloque);
            }

            //printf("DEBUG: adicionarRegistroUnico (reuso) finalizado para Bloque #%d\n", nroBloque);
            return true;
        }

        // 13) No hay hueco: anexar al final
        printf("DEBUG: Bloque #%d sin espacio libre, se agregará al final.\n", nroBloque);
        // Calcular offsetAppend = raw_header_len + numRegAnt_old * (registroSize+1)
        long offsetAppend = (long)raw_header_len + (long)(numRegAnt) * (registroSize + 1);
        printf("DEBUG: offsetAppend en Bloque%d = %ld\n", nroBloque, offsetAppend);

        // Actualizar cabecera (numRegAnt+1, marcar nuevo bitmap)
        numRegAnt++;
        if (numRegAnt <= numMaxAnt) {
            bitmapAnt[numRegAnt - 1] = '1';
        }
        //printf("DEBUG: (Anexar) Nuevo numRegAnt = %d, bitmapAnt = '%s'\n",
        //    numRegAnt, bitmapAnt);

        // Reconstruir cabecera EXACTA: espacios hasta raw_header_len-1, luego '/'
        rewind(fbloc);
        char newHeader3[MAX_BUF];
        int ofh3 = 0;
        ofh3 += snprintf(newHeader3 + ofh3, MAX_BUF - ofh3, "%d#%d#", numRegAnt, numMaxAnt);
        for (int i = 0; i < numMaxAnt && ofh3 < (int)(raw_header_len - 1); i++) {
            newHeader3[ofh3++] = bitmapAnt[i];
        }
        while (ofh3 < (int)(raw_header_len - 1)) {
            newHeader3[ofh3++] = ' ';
        }
        newHeader3[raw_header_len - 1] = '/';
        newHeader3[raw_header_len] = '\0';
        //printf("DEBUG: (Anexar) Reconstruyendo cabecera: '%.*s'\n", (int)raw_header_len, newHeader3);

        fwrite(newHeader3, 1, raw_header_len, fbloc);
        fflush(fbloc);

        // Insertar regBuf + '|' en offsetAppend
        if (fseek(fbloc, offsetAppend, SEEK_SET) != 0) {
            fprintf(stderr, "ERROR: fseek falló al offsetAppend\n");
            fclose(fbloc);
            return false;
        }
        size_t escritosAppend = fwrite(regBuf, 1, regLen, fbloc);
        fputc('|', fbloc);
        fflush(fbloc);
        if (escritosAppend != (size_t)regLen) {
            printf("ERROR: Se escribieron %zu bytes de RLF anexado, esperados %d\n", escritosAppend, regLen);
        }
        else {
            printf("DEBUG: Registro de %d bytes anexado correctamente (+ '|').\n", regLen);
        }

        fclose(fbloc);
        // 14) Volcar bloque anexado a sectores
        printf("DEBUG: Llamando a volcarBloqueASectores para bloque #%d (anexar)\n", nroBloque);
        if (!esPagina) {
            disco.volcarBloqueASectores(nroBloque);
        }

        printf("DEBUG: adicionarRegistroUnico (anexar) finalizado para Bloque #%d\n", nroBloque);
        return true;
    }
}

// -----------------------------------------------------------------------------
// Función: adicionarNRegistros
// Objetivo de la función:
//     Leer n líneas de un CSV, transformar comas a ‘#’ e insertar cada registro
//     llamando a adicionarRegistroUnico o bitmap según opción.
// Input:
//     int n                  – número de registros a procesar.
//     const char* csvPath    – ruta al archivo CSV.
//     const char* tabla      – nombre de la tabla/relación.
//     int opcion             – 1 para bitmap, otro valor para fijo.
//     Disco& disco           – objeto Disco para inserción.
// Output:
//     bool  – true si todas las inserciones fueron exitosas; false si falla alguna.
// Autor: Alex Cañapataña
// -----------------------------------------------------------------------------
static bool adicionarNRegistros(int n, const char* csvPath, const char* tabla, int opcion, Disco& disco) {
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
            if (linea[j] == ';') registroTxt[pos++] = '#';
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
            ok = adicionarRegistroUnicoBitmap(tabla, registroTxt);
        }
        else {
            ok = adicionarRegistroUnico(registroTxt, tabla, disco);
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

// -----------------------------------------------------------------------------
// Función: adicionarTodoCSV
// Objetivo de la función:
//     Leer todas las líneas de un CSV tras el encabezado, convertir comas a ‘#’
//     e insertar cada registro llamando a adicionarRegistroUnico o bitmap.
// Input:
//     const char* csvPath    – ruta al archivo CSV.
//     const char* tabla      – nombre de la tabla/relación.
//     int opcion             – 1 para bitmap, otro valor para fijo.
//     Disco& disco           – objeto Disco para inserción.
// Output:
//     bool  – true si todas las inserciones fueron exitosas; false si falla alguna.
// Autor: Alex Cañapataña
// -----------------------------------------------------------------------------
static bool adicionarTodoCSV(const char* csvPath, const char* tabla, int opcion, Disco& disco) {
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
            ok = adicionarRegistroUnicoBitmap(tabla, registroTxt);
        }
        else {
            ok = adicionarRegistroUnico(registroTxt, tabla, disco);
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