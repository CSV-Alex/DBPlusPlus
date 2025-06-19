#pragma once
#include <vector>
#include <string>

const char* rutaCatalogoUnused = "data/usr/db/catalogo.txt";
const char* rutaLongitudFija = "DISCO\\longitudFija.txt";
const char* rutaDirBloques = "DISCO\\dirBloques.txt";
const char* discoPath = "DISCO\\";
const char* rutaBloques = "DISCO\\BLOQUES\\";
const char* rutaBloque = "DISCO\\BLOQUES\\";
const char* bufferPoolPath = "BUFFERPOOL\\";
const char* bufferPagePath = "BUFFERPOOL\\BLOQUES\\";
const char* rutaCatalogo = "DISCO\\catalogo.txt";

std::vector<std::pair<long, std::string>> cambiosDirBloques;
std::vector<int> paginasModificadas;

void registrarPaginaModificada(int nroBloque);
void flushBufferToDisk(class Disco& disco);