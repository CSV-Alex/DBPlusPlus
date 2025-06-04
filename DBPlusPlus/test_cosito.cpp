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
    const int tamBloque = 180;
    Disco miDisco(platos, pistas, sectores, tamSector, tamBloque); //prototype_disk


    //char relacion[] = "housing";
    //char registro[] = "13300000#7420#4#2#3#yes#no#no#no#yes#2#yes#furnished";
    char relacion[] = "titanic";
    char registro[] = "1#0#3#Braund# Mr. Owen Harris#male#22#1#0#A/5 21171#7.25##S";
    char registro2[] = "2#1#1#Cumings# Mrs. John Bradley (Florence Briggs Thayer)#female#38#1#0#PC 17599#71.2833#C85#C";

    char* reg_var = format_registro_variable(registro, relacion);
    char* reg_var2 = format_registro_variable(registro2, relacion);
    // Prepara bloque vac�o con meta inicial
    char bloque[180] = "";
    //std::memset(bloque, 0, sizeof(bloque));//solo para este bloque
    // registro_n=0, last_pos=0
    bool ok = format_bloque_variable(reg_var, relacion, 180, bloque, miDisco);
    assert(ok);
    std::cout << "format_bloque_variable paso la prueba." << std::endl;
    ok = format_bloque_variable(reg_var2, relacion, 180, bloque, miDisco);
    assert(ok);
    std::cout << "format_bloque_variable2 paso la prueba." << std::endl;


    // Volcar el bloque completo a un fichero binario "bloque.txt"
    std::ofstream fout("bloque.txt", std::ios::binary | std::ios::trunc);
    if (!fout) {
        std::cerr << "Error abriendo bloque.txt para escritura\n";
    }
    else {
        fout.write(bloque, tamBloque);
        fout.close();
        std::cout << "Bloque volcado a bloque.txt\n";
    }
    delete[] reg_var;

    char* tail = read_bloque_content(bloque, relacion);
    std::cout << "contenido " << tail << std::endl;
    delete[] tail;


    return 0;


    //int field_number = get_n_attributes(relacion);
    //std::cout<<field_number<<std:: endl;

    //char* registro_variable=format_registro_variable(registro, relacion); 
    //char* parsed = nullptr;

    /*
    int start_pos = field_number*4;
    std::string text_part;
    for (int i = start_pos; i < 512 && registro_variable[i] != '\0'; ++i) {
            text_part.push_back(registro_variable[i]);
        }
        std::cout << "Texto desde posici�n " << start_pos << ": " << text_part << std::endl; //esto falla en titanic

    // Liberar memoria si se reserv� din�micamente
    */
    //parsed=read_registro_variable(registro_variable,field_number,parsed);
    //std::printf("Parsed: %s\n", parsed);

    //delete[] registro_variable;

    //prototipo longitud variable



}

/*
        int header_len = field_number * 4;
        // Para datos, podemos volver a separar campos y sumar longitudes, o medir hasta el primer '\0'
        int data_len = header_len;
        while (data_len < 512 && registro_variable[data_len] != '\0'){ ++data_len;}
        int total_len = data_len;

        // Abrir archivo de salida en modo binario
        std::FILE* f = std::fopen("salida.txt", "wb");
        if (!f) throw std::runtime_error("No se pudo abrir salida.txt para escritura");
        // Escribir todo el buffer formateado (solo los bytes relevantes)
        std::fwrite(registro_variable, 1, total_len, f);
        std::fclose(f);

        std::cout << "Buffer binario escrito en salida.txt (" << total_len << " bytes)" << std::endl;
        delete[] registro_variable;*/