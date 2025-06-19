#pragma once
#include <vector>
#include <string>

extern const char* rutaCatalogo;
extern const char* rutaLongitudFija;
extern const char* rutaDirBloques;
extern const char* discoPath;
extern const char* bufferPoolPath;
extern const char* rutaCatalogo;
extern const char* bufferPagePath;

extern std::vector<std::pair<long, std::string>> cambiosDirBloques;
extern std::vector<int> paginasModificadas;

void registrarPaginaModificada(int nroBloque);
void flushBufferToDisk(class Disco& disco);