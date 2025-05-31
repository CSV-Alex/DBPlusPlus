







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



