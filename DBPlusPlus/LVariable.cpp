/* \\places exchanged, this is the definition of the .h

#ifndef DBPLUSPLUS_LVARIABLE_H
#define DBPLUSPLUS_LVARIABLE_H

#include <cstdint>
#include <stdexcept>
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
#include <iostream>
#include "Disco.h"

char schemapath[] = "data\\usr\\db\\esquema.txt";

//AUXILIAR, Posiblemente global
int get_n_attributes(const char* relacion); //cuenta n atributos de una relacion en esquema

//AUXILIARES, Bit lecture and conversion
void num_to_binary(int n, char (*par_byte)[2]); 

int binary_to_num(const char (*par_byte)[2]) ;

//FORMATO DE REGISTRO VARIABLE

char* format_registro_variable(const char registro[], const char relacion[]) ;

bool format_bloque_variable(const char registro_variable[], const char relacion[], int espacio_libre_bloque, char* bloque_variable, Disco& disco);

bool format_registro_variable_long(const char registro_variable[], const char relacion[]);

//AUXILIARES, BLOQUE HIBRIDO (BITs, CARACTERES)
char* read_registro_variable(const char registro_variable[], int field_num, char*& output);

char* read_record_from_block(const char bloque[], int write_pos, int registro_len);  

char* read_bloque_content(const char bloque[], const char relacion[]);

#endif 
*/