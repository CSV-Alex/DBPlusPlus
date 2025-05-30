#pragma once

#include <cstdint>
#include <stdexcept>
#include <cstdio>
#include <cstring>

char schemapath[]="data\\usr\\db\\esquema.txt";

/**
 * Convierte un entero en su representación de 2 bytes y la almacena en el arreglo apuntado por par_byte.
 * @param n          El número entero a convertir (debe estar en rango 0..65535).
 * @param par_byte   Puntero a un arreglo de 2 bytes donde se guardará el valor en orden little-endian:
 *                   (*par_byte)[0] = byte de orden más bajo (LSB).
 *                   (*par_byte)[1] = byte de orden más alto (MSB).
 * @throws std::out_of_range si n está fuera del rango permitido.
 */
void num_to_binary(int n, char (*par_byte)[2]) {
    if (n < 0 || n > 0xFFFF) {
        throw std::out_of_range("El valor debe estar entre 0 y 65535");
    }
    (*par_byte)[0] = static_cast<char>(n & 0xFF);
    (*par_byte)[1] = static_cast<char>((n >> 8) & 0xFF);
}

/**
 * Formatea un registro de longitud variable según la definición en el esquema.
 * @param registro           Cadena que contiene los campos separados por '#'.
 * @param relacion           Clave de relación para buscar en el esquema.
 * @param registro_variable  Buffer de 512 bytes donde se almacenará el registro formateado.
 * @throws std::runtime_error si no se encuentra la relación en el esquema o hay error de E/S.
 */
//registro[] todos con contenido
//declaracion:
//char registro_variable[]=format_registro_variable(registro, relacion);
//delete[] registro_variable; al terminar de usar registro variable, y antes de hacer otra llamada a la funcion
char* format_registro_variable(const char registro[], const char relacion[]) {
    // Abre esquema y cuenta cuántos campos (#)
    FILE* schema = std::fopen(schemapath, "r");
    if (!schema) {
        throw std::runtime_error("No se pudo abrir el archivo de esquema");
    }
    char line[1024];
    int field_number = 1;
    size_t rel_len = std::strlen(relacion);
    bool found = false;
    while (std::fgets(line, sizeof(line), schema)) {
        if (std::strncmp(line, relacion, rel_len) == 0) {
            for (char* p = line; *p; ++p) if (*p == '#') ++field_number;
            found = true;
            break;
        }
    }
    std::fclose(schema);
    if (!found) {
        throw std::runtime_error("Relación no encontrada en el esquema");
    }

    // Reserva buffer de 512 bytes en heap
    char* registro_variable = new char[512];
    std::memset(registro_variable, 0, 512);

    char binary_buffer[2];
    int index_pos = (field_number * 2) - 1;
    int index_registro = 0;
    int field_size = 0;

    // Escribe posición inicial del primer campo (little-endian)
    num_to_binary(index_pos, &binary_buffer);
    registro_variable[index_registro++] = binary_buffer[0];
    registro_variable[index_registro++] = binary_buffer[1];

    // Recorre datos y construye registro
    for (const char* p = registro; *p; ++p) {
        if (*p != '#') {
            registro_variable[index_registro++] = *p;
            ++field_size;
            ++index_pos;
        } else {
            // Límite de campo: escribe tamaño del campo previo
            num_to_binary(field_size, &binary_buffer);
            registro_variable[index_registro++] = binary_buffer[0];
            registro_variable[index_registro++] = binary_buffer[1];
            index_pos += 2;

            // Reinicia contador y escribe posición inicial siguiente campo
            field_size = 0;
            num_to_binary(index_pos, &binary_buffer);
            registro_variable[index_registro++] = binary_buffer[0];
            registro_variable[index_registro++] = binary_buffer[1];
        }
    }

    // Escribe tamaño del último campo si existe
    if (field_size > 0) {
        num_to_binary(field_size, &binary_buffer);
        registro_variable[index_registro++] = binary_buffer[0];
        registro_variable[index_registro++] = binary_buffer[1];
    }

    return registro_variable;
}