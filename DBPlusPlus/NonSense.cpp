






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



static bool str_ne(const char* a, const char* b) {
    return !str_eq(a, b);
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

        // 4.f) Desplazar bytes: si delta>0: atrás?adelante; si delta<0: adelante?atrás.
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

