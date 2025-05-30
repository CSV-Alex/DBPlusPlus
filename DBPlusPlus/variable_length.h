#pragma once

#include <cstdint>
#include <stdexcept>
#include <cstdio>
#include <cstring>
#include<vector>
#include <string>
#include <iostream>
#include "Disco.h"  

char schemapath[]="data\\usr\\db\\esquema.txt";

int get_n_attributes(const char* relacion) {
    FILE* schema = std::fopen(schemapath, "r");
    if (!schema) throw std::runtime_error("No se pudo abrir el archivo de esquema");
    char line[1024];
    size_t rel_len = std::strlen(relacion);
    bool found = false;
    int hash_count = 0;
    while (std::fgets(line, sizeof(line), schema)) {
        if (std::strncmp(line, relacion, rel_len) == 0) {
            for (char* p = line; *p; ++p) {
                if (*p == '#') ++hash_count;
            }
            found = true;
            break;
        }
    }
    std::fclose(schema);
    if (!found) throw std::runtime_error("Relación no encontrada en el esquema");
    // Cada par de '#' delimita dos metadatos (offset y size), por eso dividimos
    return (hash_count + 1) / 2;
}

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

int binary_to_num(const char (*par_byte)[2]) {
    uint8_t lo = static_cast<uint8_t>((*par_byte)[0]);
    uint8_t hi = static_cast<uint8_t>((*par_byte)[1]);
    return static_cast<int>(lo | (hi << 8));
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
    // 1) Obtener número de atributos
    int field_number = get_n_attributes(relacion);
    // 2) Separar campos de texto
    std::vector<std::string> fields;
    const char* start = registro;
    for (;;) {
        const char* sep = std::strchr(start, '#');
        if (!sep) { fields.emplace_back(start); break; }
        fields.emplace_back(start, sep - start);
        start = sep + 1;
    }
    if ((int)fields.size() != field_number) field_number = fields.size();

    // 3) Preparar buffer
    int header_len = field_number * 4;
    char* buffer = new char[512];
    std::memset(buffer, 0, 512);
    char bin[2];
    int write_idx = 0;
    int data_offset = header_len;
    int running_offset = data_offset;

    // 4) Escribir cabecera (offset + size) por cada campo
    for (int i = 0; i < field_number; ++i) {
        int sz = (int)fields[i].size();
        num_to_binary(running_offset, &bin);
        buffer[write_idx++] = bin[0];
        buffer[write_idx++] = bin[1];
        num_to_binary(sz, &bin);
        buffer[write_idx++] = bin[0];
        buffer[write_idx++] = bin[1];
        running_offset += sz;
    }
    // 5) Copiar datos de campos
    for (int i = 0; i < field_number; ++i) {
        for (char c : fields[i]) buffer[write_idx++] = c;
    }

    // 6) Imprimir datos
    std::cout << "Datos: ";
    for (int i = data_offset; i < write_idx; ++i) std::cout << buffer[i];
    std::cout << std::endl;

    return buffer;
}


/**
 * Inserta un registro variable en un bloque y actualiza metadatos.
 * @param registro_variable Buffer con formato de registro (512 bytes).
 * @param relacion          Clave de relación para identificar esquema.
 * @param espacio_libre_bloque Espacio libre en el bloque.
 * @param bloque_variable   Puntero al buffer del bloque donde insertar.
 * @return true si se inserta correctamente; false si no cabe.
 */
bool format_bloque_variable(const char registro_variable[], const char relacion[], int espacio_libre_bloque, char* bloque_variable, Disco& disco) {
    // 1. Obtener número de atributos usando get_n_attributes
    int field_number = get_n_attributes(relacion);
    int header_len = field_number * 4;

    // 2. Calcular longitud total del registro: suma cabecera y tamaños
    int registro_len = header_len;
    for (int i = 0; i < field_number; ++i) {
        // offset at index 2*i*2+2 gives size at header
        char bin_size[2] = { registro_variable[4*i + 2], registro_variable[4*i + 3] };
        registro_len += binary_to_num(reinterpret_cast<const char(*)[2]>(&bin_size));
    }

    // 3. Leer metadatos del bloque: registro_n y last_pos
    // Asegurar que bin_meta esté limpio antes de usar
    char bin_meta[2] = {0, 0};
    int registro_n = 0;
    int last_pos = 0;
    // bytes 0-1 = registro_n
    bin_meta[0] = bloque_variable[0];
    bin_meta[1] = bloque_variable[1];
    registro_n = binary_to_num(&bin_meta);
    bin_meta[0] = {0}; //reinicio del buffer
    bin_meta[1] = {0};
    // bytes 2-3 = last_position
    bin_meta[0] = bloque_variable[2];
    bin_meta[1] = bloque_variable[3];
    last_pos = binary_to_num(&bin_meta);
    if (last_pos == 0) last_pos = disco.get_tam_bloque() - 1;

    // 4. Verificar espacio y escribir registro
    if (espacio_libre_bloque - last_pos < registro_len) return false;
    int write_pos = last_pos - registro_len + 1;
    std::memcpy(bloque_variable + write_pos, registro_variable, registro_len);
    last_pos = write_pos;
    registro_n++;

    // 5. Actualizar cabecera en bloque: nueva last_pos y tamaño
    int idx = 4 + (registro_n - 1) * 4;
    num_to_binary(last_pos, &bin_meta);
    bloque_variable[idx]     = bin_meta[0];
    bloque_variable[idx + 1] = bin_meta[1];
    idx += 2;
    num_to_binary(registro_len, &bin_meta);
    bloque_variable[idx]     = bin_meta[0];
    bloque_variable[idx + 1] = bin_meta[1];
    return true;
}


//AUXILIARES
/**
 * Parsea un registro formateado y construye un string con offsets y datos.
 * @param registro_variable  Buffer binario del registro.
 * @param field_num          Número de campos en la cabecera.
 * @param output             Referencia a puntero donde se devolverá el buffer (heap).
 * @return Puntero al buffer con el contenido parseado (liberar con delete[]).
 */

 /*char* read_registro_variable(const char registro_variable[], int field_num, char*& output) {
    char tmp[512];
    int pos = 0;
    char binary_buffer[2];
    // 1) Leer cada par de bytes en la cabecera, convertir y mostrar
    for (int i = 0; i < field_num; ++i) {
        binary_buffer[0] = registro_variable[2*i];
        binary_buffer[1] = registro_variable[2*i + 1];
        int value = binary_to_num(reinterpret_cast<const char(*)[2]>(&binary_buffer));
        std::cout << value << " ";
        pos += std::snprintf(tmp + pos, sizeof(tmp) - pos, "%d#", value);
    }
    std::cout << std::endl;
    // 2) Mostrar mensaje antes de copiar la cadena restante
    std::cout << "cadena" << std::endl;
    // 3) Copiar datos a partir de la cabecera hasta '\0'
    int data_start = 2 * field_num;
    int i = data_start;
    while (i < 512 && registro_variable[i] != '\0') {
        tmp[pos++] = registro_variable[i++];
    }
    tmp[pos] = '\0';
    // Reservar output
    output = new char[pos + 1];
    std::memcpy(output, tmp, pos + 1);
    return output;
}
    */
char* read_registro_variable(const char registro_variable[], int field_num, char*& output) {
    int header_len = field_num * 4;
    std::vector<int> offsets(field_num), sizes(field_num);
    char bin[2];
    int idx = 0;
    for (int i = 0; i < field_num; ++i) {
        bin[0] = registro_variable[idx++];
        bin[1] = registro_variable[idx++];
        offsets[i] = binary_to_num(reinterpret_cast<const char(*)[2]>(&bin));
        bin[0] = registro_variable[idx++];
        bin[1] = registro_variable[idx++];
        sizes[i] = binary_to_num(reinterpret_cast<const char(*)[2]>(&bin));
    }
    std::string tmp;
    for (int i = 0; i < field_num; ++i) {
        int off = offsets[i];
        int sz  = sizes[i];
        tmp.append(registro_variable + off, sz);
        if (i < field_num - 1) tmp.push_back('#');
    }
    output = new char[tmp.size() + 1];
    std::memcpy(output, tmp.c_str(), tmp.size());
    output[tmp.size()] = '\0';
    return output;
}
// Ejemplo de uso de read_registro_variable:
// char* parsed;
// read_registro_variable(registro_variable, field_count, parsed);
// std::printf("%s\n", parsed);
// delete[] parsed;


/* //original
bool format_bloque_variable(const char registro_variable[], const char relacion[], int espacio_libre_bloque, char* bloque_variable, Disco& Disco) {
    // 1. Contar campos en esquema
    FILE* schema = std::fopen(schemapath, "r");
    if (!schema) throw std::runtime_error("No se pudo abrir el archivo de esquema");
    char line[1024]; int field_number = 1; size_t rel_len = std::strlen(relacion);
    bool found = false;
    while (std::fgets(line, sizeof(line), schema)) {
        if (std::strncmp(line, relacion, rel_len) == 0) {
            for (char* p = line; *p; ++p) if (*p == '#') ++field_number;
            found = true; break;
        }
    }
    std::fclose(schema);
    if (!found) throw std::runtime_error("Relación no encontrada en el esquema");

    // 2. Calcular longitud total del registro
    int total_fields = field_number;
    int registro_len = 4 * total_fields;
    char binary_buffer[2] = {0,0};
    int reg_idx = 2;
    int fields = total_fields;
    while (fields > 1) {
        char buf[2] = { registro_variable[reg_idx], registro_variable[reg_idx+1] };
        registro_len += binary_to_num(reinterpret_cast<const char(*)[2]>(&buf));
        fields--; reg_idx += 4;
    }

    // 3. Leer contadores y posiciones actuales del bloque
    int registro_n; int last_pos;
    // bytes 0-1 = registro_n
    binary_buffer[0] = bloque_variable[0];
    binary_buffer[1] = bloque_variable[1];
    registro_n = binary_to_num(&binary_buffer);
    // bytes 2-3 = last_position
    binary_buffer[0] = bloque_variable[2];
    binary_buffer[1] = bloque_variable[3];
    last_pos = binary_to_num(&binary_buffer);
    if (last_pos == 0) last_pos = Disco.get_tam_bloque() - 1;

    int index_array = 4 + registro_n * 4;

    // 4. Chequear espacio y escribir
    if (espacio_libre_bloque - last_pos >= registro_len) {
        int write_pos = last_pos - registro_len + 1;
        // Copiar datos
        for (int i = 0; i < registro_len; ++i) {
            bloque_variable[write_pos + i] = registro_variable[i];
        }
        last_pos = write_pos;
        registro_n++;
        // Actualizar cabecera: nueva last_pos
        num_to_binary(last_pos, &binary_buffer);
        bloque_variable[index_array]     = binary_buffer[0];
        bloque_variable[index_array + 1] = binary_buffer[1];
        index_array += 2;
        // Tamaño del registro
        num_to_binary(registro_len, &binary_buffer);
        bloque_variable[index_array]     = binary_buffer[0];
        bloque_variable[index_array + 1] = binary_buffer[1];
        // index_array += 2; // no necesario a fin
        return true;
    }
    return false;
}
*/