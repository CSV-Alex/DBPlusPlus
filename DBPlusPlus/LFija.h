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

#define MAX_TOKENS    256

static int getTamBloqueFromDisco(Disco& disco) {
    return disco.getTamBloque();
}

static const char* getBufferRutaFromDisco(const Disco& disco) {
    return disco.getBufferRuta();
}

static const char* getBufferLecturaFromDisco(const Disco& disco) {
    return disco.getBufferLectura();
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
static bool isBlockAllowed(const char* nombreRelacion, int nroBloque) {
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

static bool ajustarDirBloques(int bloqueN,
    const char* codSector,
    int  deltaBloque,
    int  deltaSector)
{
    // 1) Abrir dirBloques.txt en "r+"
    FILE* fdir = fopen(rutaDirBloques, "r+");
    if (!fdir) return false;

    // 2) Buscar línea bloqueN
    char linea[MAX_BUF];
    int  numLinea = 0;
    long posLinea = 0;
    while (fgets(linea, MAX_BUF, fdir)) {
        numLinea++;
        if (numLinea == bloqueN) break;
        posLinea = ftell(fdir);
    }
    if (numLinea != bloqueN) {
        fclose(fdir);
        return false;
    }

    // 3) Determinar raw_len_total: longitud en disco de esa línea
    size_t lenNoCrLf = strlen(linea);
    // Ajuste Windows vs Unix:
    size_t raw_len_total = lenNoCrLf + 1;
    if (lenNoCrLf > 0 && linea[lenNoCrLf - 1] == '\r') raw_len_total++;

    // 4) Limpiar el '\r\n'
    while (lenNoCrLf > 0 && (linea[lenNoCrLf - 1] == '\n' || linea[lenNoCrLf - 1] == '\r')) {
        linea[--lenNoCrLf] = '\0';
    }

    // 5) Tokenizar con strtok(linea, "#")
    char copia[MAX_BUF];
    strncpy(copia, linea, MAX_BUF - 1);
    copia[MAX_BUF - 1] = '\0';
    char* tokens[MAX_TOKENS];
    int   ntok = 0;
    char* tk = strtok(copia, "#");
    while (tk && ntok < MAX_TOKENS) {
        tokens[ntok++] = tk;
        tk = strtok(nullptr, "#");
    }

    if (ntok < 5) { fclose(fdir); return false; }
    // tokens[0]=espLibreBloque, tokens[1]="2", tokens[2]="BLOQUE",
    // tokens[3]=nroBloque, tokens[4]=tamBloque,
    // tokens[5]=_<espLibreSector1>, tokens[6]=<codSector1>, tokens[7]=_<espLibreSector2>, ...

    // 6) Calcular nuevo espacio libre del bloque:
    int espacioLibreBloqueAntes = atoi(tokens[0]);
    int espacioLibreBloqueNuevo = espacioLibreBloqueAntes + deltaBloque;

    // 7) Volver a armar la línea completa:
    //    a) Primer bloque: "<espLibreBloqueNuevo>#2#BLOQUE#<bloqueN>#<tamBloque>#_"
    char bufferNueva[MAX_BUF];
    int  ofs = 0;
    int  tamBloqueTotal = atoi(tokens[4]);
    ofs += snprintf(bufferNueva + ofs, MAX_BUF - ofs,
        "%d#2#BLOQUE#%d#%d#_",
        espacioLibreBloqueNuevo,
        bloqueN,
        tamBloqueTotal);

    //    b) Para cada par i=5,7,9,... (tokens[i] empieza con '_' = "_<espLibreSector>"):
    for (int i = 5; i + 1 < ntok; i += 2) {
        if (tokens[i][0] != '_') continue;
        int espLibreSectorAntes = atoi(tokens[i] + 1);
        const char* codSectorX = tokens[i + 1];
        int nuevoEsp = espLibreSectorAntes;
        if (strcmp(codSectorX, codSector) == 0) {
            nuevoEsp = espLibreSectorAntes + deltaSector;
        }
        ofs += snprintf(bufferNueva + ofs, MAX_BUF - ofs,
            "%d#%s#_",
            nuevoEsp,
            codSectorX);
    }

    // 8) Rellenar con espacios hasta raw_len_total-1, luego "\n"
    if (ofs > (int)raw_len_total - 1) {
        // Si se pasó, truncamos justo antes de '\n'
        if (raw_len_total >= 1) bufferNueva[raw_len_total - 1] = '\n';
    }
    else {
        for (int i = ofs; i < (int)raw_len_total - 1; i++) {
            bufferNueva[i] = ' ';
        }
        bufferNueva[raw_len_total - 1] = '\n';
    }
    // (no agregamos '\0' final porque usaremos fwrite)

    // 9) Sobreescribir esa línea en el archivo
    fseek(fdir, posLinea, SEEK_SET);
    fwrite(bufferNueva, 1, raw_len_total, fdir);
    fflush(fdir);
    fclose(fdir);
    return true;
}

static bool crearRLF(const char* registroTxt, const char* relacion, char* outBuffer, int* outLen) {
    int numFields = 0;
    int maxLen[MAX_FIELDS] = { 0 };
    printf("DEBUG: Llamando a crearRLF para relación '%s' con registroTxt: '%s'\n", relacion, registroTxt);

    if (!obtenerLongitudesPorCampo(relacion, &numFields, maxLen)) {
        printf("DEBUG: obtenerLongitudesPorCampo falló para '%s'\n", relacion);
        return false;
    }
    printf("DEBUG: Longitudes por campo para '%s': ", relacion);
    for (int i = 0; i < numFields; ++i) {
        printf("%d ", maxLen[i]);
    }
    printf("\n");

    // Hacemos una copia local para usar strtok sin modificar el original
    char copy[MAX_BUF];
    strncpy(copy, registroTxt, MAX_BUF - 1);
    copy[MAX_BUF - 1] = '\0';

    char* tok = strtok(copy, "#");
    int ofs = 0;

    for (int i = 0; i < numFields; i++) {
        if (!tok) {
            printf("DEBUG: Faltan campos en registroTxt para el campo %d\n", i);
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
        printf("DEBUG: Campo %d (fijo): '", i);
        for (int k = 0; k < fieldLen; ++k) {
            if (k < toCopy) putchar(tok[k]);
            else putchar('@');
        }
        printf("'\n");

        ofs += fieldLen;

        // Insertamos el separador '#' justo después de cada campo
        outBuffer[ofs] = '#';
        ofs += 1;

        // Debug: mostramos el '#' insertado
        printf("DEBUG: Añadido separador '#'\n");

        // Continuamos al siguiente token
        tok = strtok(NULL, "#");
    }

    *outLen = ofs;

    // Mostrar el registro fijo completo (con '#' tras cada campo)
    printf("DEBUG: Registro fijo generado (longitud %d): [", *outLen);
    for (int i = 0; i < *outLen; ++i) {
        putchar(outBuffer[i]);
    }
    printf("]\n");

    return true;
}

static bool eliminarRegistro(const char* relacion, int posicion) {
    std::cout << "Entrando a eliminarRegistro para relación '" << relacion
        << "', posición " << posicion << "\n";
    char rutaBloques[MAX_PATH_LEN] = "DISCO\\BLOQUES\\";
    // 1) Determinar bloque (aquí simplificamos: Bloque1.txt).
    int bloqueN = 1;
    char rutaBloque[MAX_PATH_LEN];

    snprintf(rutaBloque, sizeof(rutaBloque), "%sBloque%d.txt", rutaBloques, bloqueN);

    // 2) Abrir BloqueN.txt en modo lectura/escritura binario
    FILE* fbloc = fopen(rutaBloque, "r+b");

    std::cout << rutaBloques << std::endl;
    std::cout << rutaBloque << std::endl;

    if (!fbloc) {
        std::perror("Error al abrir BloqueN.txt");
        return false;
    }

    // 3) Leer cabecera hasta '/' para extraer raw_header_len, numRegAnt, numMaxAnt y bitmap[]
    char cabTmp[MAX_BUF];
    size_t raw_header_len = 0;
    int numRegAnt = 0, numMaxAnt = 0;
    // Aseguramos espacio para bitmap con terminador
    static char bitmap[MAX_FIELDS + 1];

    // 3.1) Medir bytes hasta '/'
    {
        long pos = 0;
        int ch;
        rewind(fbloc);
        while ((ch = fgetc(fbloc)) != EOF) {
            pos++;
            if (ch == '/') break;
            if (pos >= MAX_BUF - 1) break;
        }
        if (ch != '/') {
            std::cerr << "Cabecera corrupta o faltante '/' en Bloque" << bloqueN << "\n";
            fclose(fbloc);
            return false;
        }
        raw_header_len = pos;
        if (raw_header_len > MAX_BUF - 1) raw_header_len = MAX_BUF - 1;
    }

    // 3.2) Leer la cabecera en cabTmp
    rewind(fbloc);
    fread(cabTmp, 1, raw_header_len, fbloc);
    cabTmp[raw_header_len] = '\0';

    // 3.3) Parsear "<numRegAnt>#<numMaxAnt>#<bitmap>/"
    {
        // "<numRegAnt>"
        char* p1 = strchr(cabTmp, '#');
        if (!p1) {
            std::cerr << "Formato inválido de cabecera (sin '#')\n";
            fclose(fbloc);
            return false;
        }
        *p1 = '\0';
        numRegAnt = atoi(cabTmp);

        // "#<numMaxAnt>"
        char* p2 = p1 + 1;
        char* p3 = strchr(p2, '#');
        if (!p3) {
            std::cerr << "Formato inválido de cabecera (sin segundo '#')\n";
            fclose(fbloc);
            return false;
        }
        *p3 = '\0';
        numMaxAnt = atoi(p2);
        if (numMaxAnt < 1) {
            std::cerr << "numMaxAnt inválido: " << numMaxAnt << "\n";
            fclose(fbloc);
            return false;
        }
        if (numMaxAnt > MAX_FIELDS) {
            std::cerr << "Advertencia: numMaxAnt (" << numMaxAnt
                << ") excede MAX_FIELDS (" << MAX_FIELDS << ").\n";
            numMaxAnt = MAX_FIELDS;
        }

        // "#<bitmap>/"
        char* p4 = p3 + 1; // apunta al primer dígito del bitmap
        char* slash = strchr(p4, '/');
        if (!slash) {
            std::cerr << "Formato inválido de cabecera (sin '/')\n";
            fclose(fbloc);
            return false;
        }
        size_t bmpLen = (size_t)(slash - p4);
        if ((int)bmpLen > numMaxAnt) bmpLen = numMaxAnt;
        strncpy(bitmap, p4, bmpLen);
        bitmap[bmpLen] = '\0';
    }

    // 4) Verificar rango de posición [1..numMaxAnt]
    if (posicion < 1 || posicion > numMaxAnt) {
        std::cout << "La posición " << posicion << " está fuera de rango [1.." << numMaxAnt << "]\n";
        fclose(fbloc);
        return false;
    }

    int idx = posicion - 1; // índice base 0
    if (bitmap[idx] == '0') {
        std::cout << "La línea " << posicion << " no contiene ningún registro.\n";
        fclose(fbloc);
        return false;
    }

    // 5) Cambiar bitmap[idx] de '1' a '0' y decrementar numRegAnt
    bitmap[idx] = '0';
    numRegAnt--;

    // 6) Reconstruir nueva cabecera EXACTAMENTE con raw_header_len bytes
    char newHeader[MAX_BUF];
    int ofh = 0;
    ofh += snprintf(newHeader + ofh, MAX_BUF - ofh, "%d#%d#", numRegAnt, numMaxAnt);
    // Copiar bitmap actualizado
    for (int i = 0; i < numMaxAnt && ofh < (int)(raw_header_len - 1); i++) {
        newHeader[ofh++] = bitmap[i];
    }
    // Poner '/'
    if (ofh < (int)raw_header_len) {
        newHeader[ofh++] = '/';
    }
    // Rellenar con espacios hasta raw_header_len
    while (ofh < (int)raw_header_len) {
        newHeader[ofh++] = ' ';
    }
    // No agregamos '\0' extra
    newHeader[ofh] = '\0';

    // 7) Sobrescribir cabecera en BloqueN.txt
    rewind(fbloc);
    fwrite(newHeader, 1, raw_header_len, fbloc);
    fflush(fbloc);

    // 8) Obtener registroSize (longitud fija en bytes) usando la función correcta
    int registroSize = 0;
    obtenerRegistroSize(relacion, &registroSize);
    if (registroSize <= 0) {
        std::cerr << "Error obteniendo tamaño fijo de registro para '" << relacion << "'.\n";
        return false;
    }

    // 9) Calcular offset donde empieza el registro (sin contar '|')
    //    Cada registro ocupa (registroSize + 1) bytes en el archivo (el +1 es el delimitador '|').
    long offsetRegistro = (long)raw_header_len + (long)idx * (registroSize + 1);

    // Posicionar el cursor en el primer byte del registro
    if (fseek(fbloc, offsetRegistro, SEEK_SET) != 0) {
        std::cerr << "Error en fseek hacia offset de registro: " << offsetRegistro << "\n";
        fclose(fbloc);
        return false;
    }

    // 10) Reemplazar esos registroSize bytes por '@'
    char* relleno = (char*)malloc(registroSize);
    if (!relleno) {
        std::cerr << "Error de memoria al reservar relleno.\n";
        fclose(fbloc);
        return false;
    }
    memset(relleno, '@', registroSize);
    size_t escr = fwrite(relleno, 1, registroSize, fbloc);
    free(relleno);
    if ((int)escr != registroSize) {
        std::cerr << "Error al escribir '@' en el registro. Bytes escritos: " << escr << "\n";
        fclose(fbloc);
        return false;
    }
    fflush(fbloc);
    // NOTA: No movemos el separador '|'; permanece intacto.

    fclose(fbloc);
    std::cout << "Registro en posición " << posicion << " eliminado correctamente.\n";
    return true;
}

static bool modificarRegistro(const char* relacion, int posicion, const char* nuevoRegistroTxt) {
    std::cout << "Entrando a modificarRegistro para relación '" << relacion
        << "', posición " << posicion << "\n";

    // 1) Determinar bloque (aquí simplificamos: Bloque1.txt).
    int bloqueN = 1;
    char rutaBloques[MAX_PATH_LEN] = "DISCO\\BLOQUES\\";
    char rutaBloque[MAX_PATH_LEN];
    snprintf(rutaBloque, sizeof(rutaBloque), "%sBloque%d.txt", rutaBloques, bloqueN);

    // 2) Abrir BloqueN.txt en modo lectura/escritura binario
    FILE* fbloc = fopen(rutaBloque, "r+b");
    if (!fbloc) {
        std::perror("Error al abrir BloqueN.txt");
        return false;
    }

    // 3) Leer cabecera igual que en eliminarRegistro
    char cabTmp[MAX_BUF];
    size_t raw_header_len = 0;
    int    numRegAnt = 0, numMaxAnt = 0;
    static char bitmap[MAX_FIELDS + 1] = { 0 };

    {
        // 3.1) Medir hasta encontrar '/'
        long pos = 0;
        int  ch;
        rewind(fbloc);
        while ((ch = fgetc(fbloc)) != EOF) {
            pos++;
            if (ch == '/') break;
            if (pos >= MAX_BUF - 1) break;
        }
        if (ch != '/') {
            std::cerr << "Cabecera corrupta o faltante '/' en Bloque" << bloqueN << "\n";
            fclose(fbloc);
            return false;
        }
        raw_header_len = pos;
        if (raw_header_len > MAX_BUF - 1) raw_header_len = MAX_BUF - 1;
    }

    // 3.2) Leer la cabecera en cabTmp
    rewind(fbloc);
    fread(cabTmp, 1, raw_header_len, fbloc);
    cabTmp[raw_header_len] = '\0';

    // 3.3) Parsear "<numRegAnt>#<numMaxAnt>#<bitmap>/"
    {
        char* p1 = strchr(cabTmp, '#');
        if (!p1) { fclose(fbloc); return false; }
        *p1 = '\0';
        numRegAnt = atoi(cabTmp);

        char* p2 = p1 + 1;
        char* p3 = strchr(p2, '#');
        if (!p3) { fclose(fbloc); return false; }
        *p3 = '\0';
        numMaxAnt = atoi(p2);
        if (numMaxAnt < 1) {
            std::cerr << "numMaxAnt inválido: " << numMaxAnt << "\n";
            fclose(fbloc);
            return false;
        }
        if (numMaxAnt > MAX_FIELDS) {
            // Si excede MAX_FIELDS, lo recortamos para no desbordar bitmap[]
            std::cerr << "Advertencia: numMaxAnt (" << numMaxAnt
                << ") excede MAX_FIELDS (" << MAX_FIELDS << ").\n";
            numMaxAnt = MAX_FIELDS;
        }

        char* p4 = p3 + 1; // inicia bitmap
        char* slash = strchr(p4, '/');
        if (!slash) {
            std::cerr << "Formato inválido de cabecera (sin '/')\n";
            fclose(fbloc);
            return false;
        }
        size_t bmpLen = (size_t)(slash - p4);
        if ((int)bmpLen > numMaxAnt) bmpLen = numMaxAnt;
        strncpy(bitmap, p4, bmpLen);
        bitmap[bmpLen] = '\0';
    }
    fclose(fbloc);

    // 4) Verificar posición válida
    if (posicion < 1 || posicion > numMaxAnt) {
        std::cout << "La posición " << posicion << " está fuera de rango [1.." << numMaxAnt << "]\n";
        return false;
    }
    int idx = posicion - 1;
    if (bitmap[idx] == '0') {
        std::cout << "No hay registro en la línea " << posicion << " para modificar.\n";
        return false;
    }

    // 5) “Borrar” el registro existente usando eliminarRegistro
    if (!eliminarRegistro(relacion, posicion)) {
        std::cout << "Error al eliminar el registro existente.\n";
        return false;
    }

    // 6) Generar buffer fijo del nuevo registro
    char regBuf[MAX_BUF];
    int  regLen = 0;
    if (!crearRLF(nuevoRegistroTxt, relacion, regBuf, &regLen)) {
        std::cout << "Error al generar registro fijo para modificación.\n";
        return false;
    }

    // 7) Reabrir BloqueN.txt para escribir el nuevo registro en la misma posición
    fbloc = fopen(rutaBloque, "r+b");
    if (!fbloc) {
        std::perror("Error al reabrir BloqueN.txt");
        return false;
    }

    // 8) Calcular registroSize (longitud fija sin contar '|')
    int registroSize = 0;
    obtenerRegistroSize(relacion, &registroSize);
    std::cout << "DEBUG" << registroSize << std::endl;
    if (registroSize <= 0) {
        // No hay definición de longitud para esta relación
        fprintf(stderr, "No se encontró longitud fija para %s\n", relacion);
        return false;
    }

    // 9) Calcular offset: raw_header_len + idx * (registroSize + 1)
    long offsetRegistro = (long)raw_header_len + (long)idx * (registroSize + 1);
    if (fseek(fbloc, offsetRegistro, SEEK_SET) != 0) {
        std::cerr << "Error en fseek hacia offset de registro: " << offsetRegistro << "\n";
        fclose(fbloc);
        return false;
    }

    // 10) Escribir el nuevo registro fijo + '|'
    fwrite(regBuf, 1, registroSize, fbloc);
    fputc('|', fbloc);
    fflush(fbloc);
    fclose(fbloc);

    // 11) Actualizar el bitmap: cambiar bitmap[idx] de '0' a '1' y reescribir cabecera
    fbloc = fopen(rutaBloque, "r+b");
    if (!fbloc) {
        std::perror("Error al reabrir BloqueN.txt para actualizar cabecera");
        return false;
    }

    // Leer cabecera de nuevo
    {
        char cabTmp2[MAX_BUF];
        size_t raw_header_len2 = 0;
        int c, count = 0;
        rewind(fbloc);
        while ((c = fgetc(fbloc)) != EOF) {
            count++;
            if (c == '/') break;
            if (count >= MAX_BUF - 1) break;
        }
        if (c != '/') {
            std::cerr << "Cabecera corrupta al reescribir bitmap.\n";
            fclose(fbloc);
            return false;
        }
        raw_header_len2 = count;
        if (raw_header_len2 > MAX_BUF - 1) raw_header_len2 = MAX_BUF - 1;

        rewind(fbloc);
        fread(cabTmp2, 1, raw_header_len2, fbloc);
        cabTmp2[raw_header_len2] = '\0';

        char* p1 = strchr(cabTmp2, '#');
        *p1 = '\0';
        numRegAnt = atoi(cabTmp2);

        char* p2 = p1 + 1;
        char* p3 = strchr(p2, '#');
        *p3 = '\0';
        numMaxAnt = atoi(p2);
        if (numMaxAnt > MAX_FIELDS) numMaxAnt = MAX_FIELDS;

        char* p4 = p3 + 1;
        char* slash = strchr(p4, '/');
        size_t bmpLen = (size_t)(slash - p4);
        if ((int)bmpLen > numMaxAnt) bmpLen = numMaxAnt;
        strncpy(bitmap, p4, bmpLen);
        bitmap[bmpLen] = '\0';

        raw_header_len = raw_header_len2;
    }

    // Cambiar el bit idx de '0' a '1' y numRegAnt++
    bitmap[idx] = '1';
    numRegAnt++;

    // Reconstruir cabecera
    char newHeader[MAX_BUF];
    int ofh = 0;
    ofh += snprintf(newHeader + ofh, MAX_BUF - ofh, "%d#%d#", numRegAnt, numMaxAnt);
    for (int i = 0; i < numMaxAnt && ofh < (int)(raw_header_len - 1); i++) {
        newHeader[ofh++] = bitmap[i];
    }
    if (ofh < (int)raw_header_len) {
        newHeader[ofh++] = '/';
    }
    while (ofh < (int)raw_header_len) {
        newHeader[ofh++] = ' ';
    }
    newHeader[ofh] = '\0';

    // Sobrescribir cabecera
    rewind(fbloc);
    fwrite(newHeader, 1, raw_header_len, fbloc);
    fflush(fbloc);
    fclose(fbloc);

    std::cout << "Registro en posición " << posicion << " modificado correctamente.\n";
    return true;
}

static bool adicionarRegistroUnicoBitmap(const char* nombreRel, const char* registroTxt) {
    return false;
}

//////////////////// insertar de forma fija

/// #P1#Works#BeforeTheCorruption
static bool adicionarRegistroUnico(const char* registroTxt, const char* relacion, Disco& disco) {
    // --- 0) Preparar datos y calcular longitud fija de bloque ---
    int registroSize;
    static char regBuf[MAX_BUF]; // Buffer para el RLF
    static int regLen;           // Longitud de regBuf

    // 0.1) Debug entrada
    printf("DEBUG: Entrando a adicionarRegistroUnico para relacion='%s', registroTxt='%s'\n",
        relacion, registroTxt);

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
        printf("DEBUG: Error en crearRLF. Saliendo.\n");
        return false;
    }
    printf("DEBUG: crearRLF terminó correctamente. regLen = %d\n", regLen);
    // Verificar que no haya saltos de línea en regBuf[0..regLen-1]
    for (int i = 0; i < regLen; ++i) {
        if (regBuf[i] == '\n' || regBuf[i] == '\r') {
            printf("ERROR: detectado salto de línea en regBuf en posición %d\n", i);
        }
    }

    // 0.4) Mostrar contenido de longitudFija.txt (solo debug)
    printf(">>> Debug: Leyendo %s para ver su contenido:\n", rutaLongitudFija);
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
    printf("DEBUG: registroSize calculado = %d\n", registroSize);
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
    printf("DEBUG: fixedLen (longitud de cada bloque) = %d bytes\n", fixedLen);

    // 1) Abrir dirBloques.txt en modo lectura y escritura
    printf("DEBUG: intentando abrir dirBloques en '%s'\n", rutaDirBloques);
    FILE* fdir = fopen(rutaDirBloques, "r+");
    if (!fdir) {
        perror("ERROR: No se puede abrir dirBloques.txt");
        return false;
    }
    printf("DEBUG: dirBloques.txt abierto con éxito.\n");

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

    // 2) Buscar bloque y sector libres (cada bloque ocupa fixedLen bytes total)
    printf("DEBUG: Buscando bloque libre para tamaño de registro = %d\n", tamRegistro);
    while (true) {
        posLineaBloque = ftell(fdir);

        int lenBloque = disco.leerBloqueConSeparador(fdir, lineaBloque, MAX_BUF);
        if (lenBloque <= 0) {
            printf("DEBUG: leerBloqueConSeparador devolvió %d (fin de bloques)\n", lenBloque);
            break;
        }

        // Saltar bloques vacíos generados por "||"
        if (lenBloque == 2 && lineaBloque[0] == '|' && lineaBloque[1] == '|') {
            continue;
        }

        nroBloque++;
        printf("DEBUG: Leyendo Bloque #%d (lenBloque = %d bytes)\n", nroBloque, lenBloque);

        // 2.1) Extraer espacioLibreBloque sin strtok:
        char tempEspacio[MAX_BUF];
        strncpy(tempEspacio, lineaBloque + 1, MAX_BUF - 1);
        tempEspacio[MAX_BUF - 1] = '\0';

        char* posHash = strchr(tempEspacio, '#');
        if (!posHash) {
            printf("DEBUG: Bloque #%d sin token inicial. Continúa.\n", nroBloque);
            continue;
        }
        *posHash = '\0';
        espacioLibreBloque = safe_atoi(tempEspacio);
        *posHash = '#';
        printf("DEBUG: Bloque #%d espacioLibreBloque = %d\n", nroBloque, espacioLibreBloque);

        tamUtilAntes = getTamBloqueFromDisco(disco) - espacioLibreBloque;
        if (espacioLibreBloque < tamRegistro) {
            printf("DEBUG: Bloque #%d NO cabe (espacio %d < %d). Pasa al siguiente.\n",
                nroBloque, espacioLibreBloque, tamRegistro);
            continue;
        }

        // 2.2) Extraer la lista de sectores en copiaSectores
        char copiaSectores[MAX_BUF];
        strncpy(copiaSectores, lineaBloque + 1, MAX_BUF - 1);
        copiaSectores[MAX_BUF - 1] = '\0';

        char* p = strstr(copiaSectores, "#_");
        if (!p) {
            printf("DEBUG: Bloque #%d no tiene '#_' (no hay lista de sectores). Continúa.\n", nroBloque);
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

            printf("DEBUG: Bloque #%d chequeando sector '%s' con espacio %d\n",
                nroBloque, sectorCode, espacioLibreSector);

            char* nextPair = strstr(p, "#_");
            if (espacioLibreSector < tamRegistro) {
                printf("DEBUG: Sector '%s' no cabe (espacio %d < %d). Siguiente.\n",
                    sectorCode, espacioLibreSector, tamRegistro);
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
            printf("DEBUG: Seleccionado Bloque #%d, Sector '%s'. espacioLibreSectorAntes = %d\n",
                nroBloque, codSectorLibre, espacioLibreSectorAntes);
            break;
        }
        if (foundBlock) break;
    }

    if (!foundBlock) {
        printf("DEBUG: No se encontró ningún bloque con espacio suficiente.\n");
        fclose(fdir);
        return false;
    }

    // 2.4) Calcular nuevo espacio de bloque
    int espacioLibreBloqueAntes = espacioLibreBloque;
    int tamUtilNuevo = tamUtilAntes + tamRegistro;
    int espacioBloqueNuevo = getTamBloqueFromDisco(disco) - tamUtilNuevo;
    printf("DEBUG: Bloque #%d espacioLibreBloqueAntes = %d  => espacioBloqueNuevo = %d\n",
        nroBloque, espacioLibreBloqueAntes, espacioBloqueNuevo);
    if (espacioBloqueNuevo < 0 || espacioBloqueNuevo > getTamBloqueFromDisco(disco)) {
        printf("ERROR: valores fuera de rango en Bloque #%d: espacioBloqueNuevo = %d\n",
            nroBloque, espacioBloqueNuevo);
    }

    // 3) Volver al inicio exacto del bloque en dirBloques.txt (posición en bytes)
    printf("DEBUG: Volviendo a byte offset %ld para sobreescribir bloque #%d\n",
        posLineaBloque, nroBloque);
    fseek(fdir, posLineaBloque, SEEK_SET);

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
    printf("DEBUG: bufferNueva hasta prefijo = '%.*s'\n", ofsN, bufferNueva);

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
                printf("DEBUG: Añadiendo \"%d#%s#_\" => n = %d\n", nuevoEsp, sectorCode2, n);
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

    printf("DEBUG: Bloque reconstruido (fixedLen %d bytes):\n", fixedLen);
    printf("       %.*s\n", fixedLen, bufferNueva);

    // 4.5) Sobreescribir exactamente fixedLen bytes en dirBloques.txt
    fseek(fdir, posLineaBloque, SEEK_SET);
    size_t escritosDir = fwrite(bufferNueva, 1, fixedLen, fdir);
    fflush(fdir);
    fclose(fdir);
    printf("DEBUG: Reescritura de bloque #%d en dirBloques.txt: %zu bytes escritos (esperados %d)\n",
        nroBloque, escritosDir, fixedLen);

    // -------------------------------------------------------------
    // 5) Ahora crear/abrir BloqueN.txt, actualizar su cabecera y escribir el registro
    // -------------------------------------------------------------
    char rutaBloque[MAX_PATH_LEN];
    snprintf(rutaBloque, sizeof(rutaBloque),
        "%sBLOQUES\\Bloque%d.txt",
        discoNuevoPath, nroBloque);
    printf("DEBUG: Ruta BloqueN.txt = '%s'\n", rutaBloque);

    FILE* fbloc = fopen(rutaBloque, "r+");
    size_t raw_header_len = 0;
    int numRegAnt = 0;
    int numMaxAnt = 0;
    char bitmapAnt[MAX_BUF] = { 0 };

    if (!fbloc) {
        // Bloque no existe: crearlo con calcularCabeceraBloque(...)
        printf("DEBUG: BloqueN.txt NO existe. Se intentará crearlo.\n");
        char headerBuf[MAX_BUF];
        size_t headerLen;
        int numMax;
        calcularCabeceraBloque(getTamBloqueFromDisco(disco), registroSize,
            headerBuf, &headerLen, &numMax);
        printf("DEBUG: calcularCabeceraBloque devolvió: headerLen=%zu, numMax=%d\n", headerLen, numMax);
        printf("DEBUG: Contenido headerBuf: '%.*s'\n", (int)headerLen, headerBuf);

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
            printf("DEBUG: Cabecera escrita correctamente en BloqueN.txt (%zu bytes)\n", escritosHeader);
        }
        // Mover cursor justo antes del '|'
        fseek(fbloc, -1, SEEK_END);
        raw_header_len = headerLen;
        numRegAnt = 0;
        numMaxAnt = numMax;
        for (int i = 0; i < numMax; i++) bitmapAnt[i] = '0';
        bitmapAnt[numMax] = '\0';
        printf("DEBUG: numRegAnt=0, numMaxAnt=%d, bitmapAnt='%s'\n", numMaxAnt, bitmapAnt);
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
        char cabTmp[MAX_BUF];
        if (raw_header_len > MAX_BUF - 1) raw_header_len = MAX_BUF - 1;
        size_t leidosCab = fread(cabTmp, 1, raw_header_len, fbloc);
        cabTmp[leidosCab] = '\0';
        printf("DEBUG: Contenido leído de cabecera (%zu bytes): '%.*s'\n",
            leidosCab, (int)leidosCab, cabTmp);

        // Parsear numRegAnt#numMaxAnt#bitmapAnt/
        char* p1 = strchr(cabTmp, '#');
        if (!p1) {
            printf("ERROR: No se encontró '#' en cabTmp. Cabecera corrupta.\n");
            fclose(fbloc);
            return false;
        }
        *p1 = '\0';
        numRegAnt = safe_atoi(cabTmp);
        char* p2 = p1 + 1;
        char* p3 = strchr(p2, '#');
        if (!p3) {
            printf("ERROR: No se encontró segundo '#' en cabTmp. Cabecera corrupta.\n");
            fclose(fbloc);
            return false;
        }
        *p3 = '\0';
        numMaxAnt = safe_atoi(p2);
        char* p4 = p3 + 1;
        char* slashPtr = strchr(p4, '/');
        if (!slashPtr) {
            printf("ERROR: No se encontró '/' en cabTmp. Cabecera corrupta.\n");
            fclose(fbloc);
            return false;
        }
        size_t bmpLen2 = (size_t)(slashPtr - p4);
        if (bmpLen2 >= MAX_BUF) bmpLen2 = MAX_BUF - 1;
        strncpy(bitmapAnt, p4, bmpLen2);
        bitmapAnt[bmpLen2] = '\0';
        printf("DEBUG: Lectura cabecera exitosa: numRegAnt=%d, numMaxAnt=%d, bitmapAnt='%s'\n",
            numRegAnt, numMaxAnt, bitmapAnt);
    }

    // 6) Buscar slot libre en bitmapAnt[]
    int idxLibre = -1;
    for (int i = 0; i < numMaxAnt; i++) {
        if (bitmapAnt[i] == '0') {
            idxLibre = i;
            break;
        }
    }
    if (idxLibre < 0) {
        printf("DEBUG: El bloque #%d está lleno (bitmap completo)\n", nroBloque);
        fclose(fbloc);
        return false;
    }
    printf("DEBUG: idxLibre en bitmap = %d (bit pasa de '0' a '1')\n", idxLibre);

    // 7) Actualizar numRegAnt y bitmapAnt[idxLibre]
    numRegAnt++;
    bitmapAnt[idxLibre] = '1';
    printf("DEBUG: Nuevo numRegAnt = %d, bitmapAnt = '%s'\n", numRegAnt, bitmapAnt);

    // 8) Reconstruir cabecera EXACTAMENTE raw_header_len bytes
    char newHeader[MAX_BUF];
    int ofh = 0;
    ofh += snprintf(newHeader + ofh, MAX_BUF - ofh, "%d#%d#", numRegAnt, numMaxAnt);
    for (int i = 0; i < numMaxAnt && ofh < (int)(raw_header_len - 1); i++) {
        newHeader[ofh++] = bitmapAnt[i];
    }
    newHeader[ofh++] = '/';
    while (ofh < (int)raw_header_len) {
        newHeader[ofh++] = ' ';
    }
    newHeader[ofh] = '\0';
    printf("DEBUG: Contenido de newHeader (long %zu): '%.*s'\n",
        raw_header_len, (int)raw_header_len, newHeader);

    // Sobreescribir cabecera en BloqueN.txt
    rewind(fbloc);
    size_t escritosCabecera = fwrite(newHeader, 1, raw_header_len, fbloc);
    fflush(fbloc);
    if (escritosCabecera != raw_header_len) {
        printf("ERROR: Se escribieron %zu bytes de cabecera, esperados %zu\n",
            escritosCabecera, raw_header_len);
    }
    else {
        printf("DEBUG: Cabecera reescrita exitosamente (%zu bytes)\n", escritosCabecera);
    }

    // 9) Agregar el RLF (regBuf) + '|' al final
    fseek(fbloc, 0, SEEK_END);
    printf("DEBUG: Escribiendo registro en BloqueN.txt al final...\n");
    size_t escritosReg = fwrite(regBuf, 1, regLen, fbloc);
    fputc('|', fbloc);
    fflush(fbloc);
    if (escritosReg != (size_t)regLen) {
        printf("ERROR: Se escribieron %zu bytes de registro, esperados %d\n", escritosReg, regLen);
    }
    else {
        printf("DEBUG: Registro de %d bytes escrito correctamente (+ '|').\n", regLen);
    }
    fclose(fbloc);
    printf("DEBUG: BloqueN.txt cerrado.\n");

    // 10) Volcar a sectores
    printf("DEBUG: Llamando a volcarBloqueASectores para bloque #%d\n", nroBloque);
    disco.volcarBloqueASectores(nroBloque);

    // 11) Actualizar catalogo.txt
    char rutaCatalogo[MAX_PATH_LEN];
    snprintf(rutaCatalogo, sizeof(rutaCatalogo),
        "%s%s", discoNuevoPath, "catalogo.txt");
    printf("DEBUG: Abriendo catalogo.txt en modo 'a' para agregar %s|Bloque%d.txt\n", relacion, nroBloque);
    FILE* fcat = fopen(rutaCatalogo, "a");
    if (fcat) {
        char rutaBloqueCat[MAX_PATH_LEN];
        snprintf(rutaBloqueCat, sizeof(rutaBloqueCat),
            "%sBLOQUES\\Bloque%d.txt", discoNuevoPath, nroBloque);
        fprintf(fcat, "%s|%s\n", relacion, rutaBloqueCat);
        fclose(fcat);
        printf("DEBUG: Entrada agregada a catalogo.txt: '%s|%s'\n", relacion, rutaBloqueCat);
    }
    else {
        perror("ERROR: No se pudo abrir catalogo.txt para escritura");
    }

    printf("DEBUG: adicionarRegistroUnico finalizado con éxito para Bloque #%d\n", nroBloque);
    return true;
}

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