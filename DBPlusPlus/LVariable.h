#pragma once

#include <cstdint>
#include <stdexcept>
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
#include <iostream>
#include "Disco.h"

char schemapath[] = "data\\usr\\db\\esquema.txt";

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
    if (!found) throw std::runtime_error("Relaci?n no encontrada en el esquema");
    // Cada par de '#' delimita dos metadatos (offset y size), por eso dividimos
    return (hash_count + 1) / 2;
}

//AUXILIARES BINARIOS, UNUSED
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

//PRINCIPALES
char* format_registro_variable(const char registro[], const char relacion[]) {
    // 1) Obtener n?mero de atributos
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

bool format_bloque_variable(const char registro_variable[], const char relacion[], int espacio_libre_bloque, char* bloque_variable, Disco& disco) {
    // 1. Obtener n?mero de atributos usando get_n_attributes
    int field_number = get_n_attributes(relacion);
    int header_len = field_number * 4;

    // 2. Calcular longitud total del registro: suma cabecera y tama?os
    int registro_len = header_len;
    for (int i = 0; i < field_number; ++i) {
        // offset at index 2*i*2+2 gives size at header
        char bin_size[2] = { registro_variable[4 * i + 2], registro_variable[4 * i + 3] };
        registro_len += binary_to_num(reinterpret_cast<const char(*)[2]>(&bin_size));
    }

    // 3. Leer metadatos del bloque: registro_n y last_pos
    // Asegurar que bin_meta est? limpio antes de usar
    char bin_meta[2] = { 0, 0 };
    int registro_n = 0;
    int last_pos = 0;
    // bytes 0-1 = registro_n
    bin_meta[0] = bloque_variable[0];
    bin_meta[1] = bloque_variable[1];
    registro_n = binary_to_num(&bin_meta);

    bin_meta[0] = 0;//reinicio del buffer
    bin_meta[1] = 0;
    // bytes 2-3 = last_position
    bin_meta[0] = bloque_variable[2];
    bin_meta[1] = bloque_variable[3];
    last_pos = binary_to_num(&bin_meta);
    if (last_pos == 0) last_pos = disco.get_tam_bloque() - 1;

    // 4. Verificar espacio y escribir registro
    if (last_pos + 1 < registro_len) return false;
    int write_pos = last_pos - registro_len + 1;
    std::memcpy(bloque_variable + write_pos, registro_variable, registro_len);
    last_pos = write_pos;
    registro_n++;

    // 5. Actualizar cabecera en bloque: nueva last_pos y tama?o
    int idx = 4 + (registro_n - 1) * 4;
    num_to_binary(last_pos, &bin_meta);
    bloque_variable[idx] = bin_meta[0];
    bloque_variable[idx + 1] = bin_meta[1];
    idx += 2;
    num_to_binary(registro_len, &bin_meta);
    bloque_variable[idx] = bin_meta[0];
    bloque_variable[idx + 1] = bin_meta[1];
    return true;
}


//AUXILIARES

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
        int sz = sizes[i];
        tmp.append(registro_variable + off, sz);
        if (i < field_num - 1) tmp.push_back('#');
    }
    output = new char[tmp.size() + 1];
    std::memcpy(output, tmp.c_str(), tmp.size());
    output[tmp.size()] = '\0';
    return output;
}


char* read_record_from_block(const char bloque[], int write_pos, int registro_len) {
    char* out = new char[registro_len + 1];
    std::memcpy(out, bloque + write_pos, registro_len);
    out[registro_len] = '\0';
    return out;
}

char* read_bloque_content(const char bloque[], const char relacion[]) {
    // 1) Leer n?mero de registros en la cabecera del bloque (bytes 0-1)
    char binary_buffer[2] = { 0, 0 };
    binary_buffer[0] = bloque[0];
    binary_buffer[1] = bloque[1];
    int n_registros = binary_to_num(reinterpret_cast<const char(*)[2]>(&binary_buffer));

    // 2) Inicializar ?ndices para offset y size en la cabecera del bloque
    int offset_idx = 4; // despu?s de registro_n (2 bytes) y last_pos (2 bytes)
    int size_idx = 6;

    // 3) Obtener longitud de cabecera interna para cada registro
    int field_count = get_n_attributes(relacion);
    int header_len = field_count * 4;

    // 4) Buffer temporal y acumulador de resultados
    char temp[180];
    std::string result;

    // 5) Iterar sobre cada registro en el bloque
    for (int r = 0; r < n_registros; ++r) {
        // Leer offset del registro
        binary_buffer[0] = bloque[offset_idx];
        binary_buffer[1] = bloque[offset_idx + 1];
        int offset = binary_to_num(reinterpret_cast<const char(*)[2]>(&binary_buffer));

        // Leer tama?o total del registro
        binary_buffer[0] = bloque[size_idx];
        binary_buffer[1] = bloque[size_idx + 1];
        int size = binary_to_num(reinterpret_cast<const char(*)[2]>(&binary_buffer));

        // 6) Copiar s?lo la parte de datos (saltando la cabecera interna)
        int data_len = size - header_len;
        for (int i = 0; i < data_len && i < 511; ++i) {
            temp[i] = bloque[offset + header_len + i];
        }
        temp[data_len] = '\0';

        // Agregar al string de resultado
        result += temp;
        if (r < n_registros - 1) result.push_back('\n');

        // Avanzar los ?ndices para el siguiente registro
        offset_idx += 4;
        size_idx += 4;
    }

    // 7) Reservar buffer final en heap y copiar
    char* output = new char[result.size() + 1];
    std::memcpy(output, result.c_str(), result.size());
    output[result.size()] = '\0';
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
    if (!found) throw std::runtime_error("Relaci?n no encontrada en el esquema");

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
        // Tama?o del registro
        num_to_binary(registro_len, &binary_buffer);
        bloque_variable[index_array]     = binary_buffer[0];
        bloque_variable[index_array + 1] = binary_buffer[1];
        // index_array += 2; // no necesario a fin
        return true;
    }
    return false;
}
*/