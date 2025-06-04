#include <iostream>
#include <fstream>
#include <cstring>  
#include <cstdio>
#include "LVariable.h"
#include "Disco.h"
#include <cassert>

using namespace std;


int main() {
    const int platos = 4;
    const int pistas = 10;
    const int sectores = 15;
    const int tamSector = 60;
    const int tamBloque = 512;
    Disco miDisco(platos, pistas, sectores, tamSector, tamBloque); //prototype_disk


    //char relacion[] = "housing";
    //char registro[] = "13300000#7420#4#2#3#yes#no#no#no#yes#2#yes#furnished";
    char relacion[] = "titanic";
    char registro[] = "1#0#3#Braund# Mr. Owen Harris#male#22#1#0#A/5 21171#7.25##S";
    char registro2[] = "2#1#1#Cumings# Mrs. John Bradley (Florence Briggs Thayer)#female#38#1#0#PC 17599#71.2833#C85#C";
    char registro3[] = "2#1#1#Cumings# Mrs. John Bradley ()#female#38#1#0#PC 17599#71.2833#C85#C";

    char* reg_var = format_registro_variable(registro, relacion);
    char* reg_var2 = format_registro_variable(registro2, relacion);
    char* reg_var3 = format_registro_variable(registro3, relacion);

    printf("%s\n",reg_var);
    printf("%s\n",reg_var2);
    // Prepara bloque vac�o con meta inicial
    char bloque[512] = "";
    int espacio=tamBloque;
    //std::memset(bloque, 0, sizeof(bloque));//solo para este bloque
    // registro_n=0, last_pos=0
    bool ok = format_bloque_variable(reg_var, espacio, bloque, miDisco);
    assert(ok);
    std::cout << "format_bloque_variable paso la prueba." << std::endl;
    ok = format_bloque_variable(reg_var2, espacio, bloque, miDisco);
    assert(ok);
    std::cout << "format_bloque_variable2 paso la prueba." << std::endl;

    ok = format_bloque_variable(reg_var3, espacio, bloque, miDisco);
    assert(ok);
    std::cout << "format_bloque_variable2 paso la prueba." << std::endl;

    if (volcar_bloque_a_archivo(bloque, tamBloque)) {
    std::cout << "Se volcó el bloque con éxito en bloque.txt\n";
    } else {
    std::cout << "No se pudo volcar el bloque a bloque.txt\n";
    }
    
    ok=eliminar_registro_variable(1,2);
    assert(ok);
    std::cout << "Eliminación" << std::endl;


   




    return 0;
}

