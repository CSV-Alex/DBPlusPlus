#define _CRT_SECURE_NO_WARNINGS

#include <cstdio>
#include <cstring>
#include <cstdlib>


// Agrega una línea "relacion|DISCO\\BLOQUES\\BloqueN.txt" al final de catalogo.txt
static void agregarCatalogo(const char* rutaCatalogo, const char* relacion, int nroBloque) {
    FILE* f = fopen(rutaCatalogo, "a");
    if (!f) return;
    fprintf(f, "%s|DISCO\\BLOQUES\\Bloque%d.txt\n", relacion, nroBloque);
    fclose(f);
}

// Lee exactamente `fixedLen` bytes desde fdir en dirBloques.txt y los copia a lineaBloque
static bool leerLineaDirBloqueEspecifica(FILE* f, char* buf, int len) {
    size_t n = fread(buf, 1, len, f);
    buf[n] = '\0';      // colocar el '\0' en buf[n], no en buf[len]
    return n > 0;
}

// De una línea completa de dirBloques.txt, extrae espacioLibreBloque y savedSectores
static void extraerEspaciosYLstSectores(const char* lineaBloque, int* espacioLibreBloque, char* savedSectores) {
    if (!lineaBloque || !espacioLibreBloque || !savedSectores) return;
    // lineaBloque inicia con '|'
    const char* p = lineaBloque + 1;
    *espacioLibreBloque = atoi(p);
    const char* p2 = strstr(lineaBloque, "#_");
    if (!p2) {
        savedSectores[0] = '\0';
        return;
    }
    const char* start = p2 + 2;
    size_t len = 0;
    while (start[len] && start[len] != '|') len++;
    strncpy(savedSectores, start, len);
    savedSectores[len] = '\0';
}

// Nueva función: contarBloques
// Abre dirBloques.txt y cuenta cuántas líneas de bloques válidas hay
static int contarBloques(const char* rutaDirBloques) {
    FILE* f = fopen(rutaDirBloques, "r");
    if (!f) return 0;
    char buf[512];
    int count = 0;
    while (fgets(buf, sizeof(buf), f)) {
        // Cada línea válida inicia con '|'
        if (buf[0] == '|') count++;
    }
    fclose(f);
    return count;
}