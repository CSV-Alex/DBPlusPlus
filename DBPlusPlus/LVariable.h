#pragma once

#include <cstdint>
#include <stdexcept>
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include "Disco.h"
#include "DiscoPaths.h"

using namespace std;

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



//PRINCIPALES
char* format_registro_variable(const char registro[], const char relacion[]) {
    // 1) Obtener número de atributos según “relacion”
    int field_number = get_n_attributes(relacion);

    // 2) Separar los campos de texto (división por ‘#’)
    std::vector<std::string> fields;
    const char* start = registro;
    for (;;) {
        const char* sep = std::strchr(start, '#');
        if (!sep) {
            fields.emplace_back(start);
            break;
        }
        fields.emplace_back(start, sep - start);
        start = sep + 1;
    }
    // Si la cantidad real de campos difiere de field_number, lo ajustamos
    if ((int)fields.size() != field_number) {
        field_number = (int)fields.size();
    }

    // 3) Calcular el tamaño de cada campo y su offset relativo sin contar el "header"
    //    offsetWithoutHeader[i] = suma de longitudes de fields[0..i-1]
    std::vector<int> sizes(field_number);
    std::vector<int> offsetWithoutHeader(field_number);
    for (int i = 0; i < field_number; ++i) {
        sizes[i] = (int)fields[i].size();
        if (i == 0) {
            offsetWithoutHeader[i] = 0;
        } else {
            offsetWithoutHeader[i] = offsetWithoutHeader[i-1] + sizes[i-1];
        }
    }

    // 4) Iterar para determinar cuánto ocupa en texto (headerLen) la parte “offset|size|” de todos los campos
    //    Necesitamos saber el número de dígitos de cada offset definitivo:
    //      offsetDef[i] = headerLen + offsetWithoutHeader[i]
    //    Pero headerLen depende de la longitud (en caracteres) de cada “offsetDef[i]” y “sizes[i]”.
    int headerLen = 0;
    int prevHeaderLen = -1;
    std::vector<int> offsets(field_number, 0);

    while (headerLen != prevHeaderLen) {
        prevHeaderLen = headerLen;

        // Calcular cada offset provisional: offsetDef[i] = prevHeaderLen + offsetWithoutHeader[i]
        // (En la primera iteración prevHeaderLen == -1, pero headerLen se inicializa a 0, así que la
        //  primera pasada se hace con offsets = offsetWithoutHeader[i], y luego se ajusta.)
        for (int i = 0; i < field_number; ++i) {
            offsets[i] = prevHeaderLen < 0
                         ? offsetWithoutHeader[i]
                         : (prevHeaderLen + offsetWithoutHeader[i]);
        }

        // Calcular cuánto ocupa en texto el encabezado completo:
        //    para cada i, se suman:
        //      dígitos(offsets[i]) + 1 (por el ‘|’)
        //    + dígitos(sizes[i]) + 1 (por el ‘|’)
        int sumText = 0;
        for (int i = 0; i < field_number; ++i) {
            // Número de dígitos de offsets[i]:
            int off = offsets[i];
            int digitsOff = (off == 0 ? 1 : 0);
            for (int tmp = off; tmp != 0; tmp /= 10) {
                ++digitsOff;
            }
            // Número de dígitos de sizes[i]:
            int sz = sizes[i];
            int digitsSz = (sz == 0 ? 1 : 0);
            for (int tmp = sz; tmp != 0; tmp /= 10) {
                ++digitsSz;
            }
            // Cada uno añade “digitsOff + 1 + digitsSz + 1”
            sumText += digitsOff + 1 + digitsSz + 1;
        }
        headerLen = sumText;
    }

    // 5) Con headerLen ya estabilizado, recalcular offsets definitivos:
    for (int i = 0; i < field_number; ++i) {
        offsets[i] = headerLen + offsetWithoutHeader[i];
    }

    // 6) Construir la cadena de encabezado en texto:
    //    “offset1|size1|offset2|size2|…|”
    std::string headerStr;
    headerStr.reserve(headerLen); // Reservamos el espacio exacto
    for (int i = 0; i < field_number; ++i) {
        headerStr += std::to_string(offsets[i]);
        headerStr += '|';
        headerStr += std::to_string(sizes[i]);
        headerStr += '|';
    }
    // headerStr.length() debe ser == headerLen en este punto.

    // 7) Construir el contenido concatenando todos los campos tal cual
    std::string contentStr;
    // Podemos reservar el espacio exacto para optimizar:
    int totalContentSize = 0;
    for (int i = 0; i < field_number; ++i) {
        totalContentSize += sizes[i];
    }
    contentStr.reserve(totalContentSize);
    for (int i = 0; i < field_number; ++i) {
        contentStr += fields[i];
    }

    // 8) Juntar todo en una sola cadena: “headerStr + contentStr”
    std::string fullStr = headerStr + contentStr;
    /*
    // 9) Opcional: Imprimir por pantalla solo la parte de “Datos” (igual que antes)
    std::cout << "Datos: ";
    // La parte de “datos” empieza en la posición headerLen de fullStr:
    for (size_t i = headerLen; i < fullStr.size(); ++i) {
        std::cout << fullStr[i];
    }
    std::cout << std::endl;
    */
    // 10) Copiar a un buffer dinámico para devolver un char*
    char* buffer = new char[fullStr.size() + 1];
    std::memcpy(buffer, fullStr.c_str(), fullStr.size() + 1); // copia el '\0' final
    return buffer;
}


bool format_bloque_variable(
    const char registro_variable[],    // Cadena C: "offset|size|offset|size|...|campos..."
    int& espacio_libre_bloque,         // ahora es referencia; se actualiza tras inserción
    char* bloque_variable,             // buffer de tamaño disco.getTamBloque()
    Disco& disco
) {
    // 1) Tamaño fijo del bloque (para índices de last_pos si está vacío)
    int bloqueSize = disco.getTamBloque();

    // 2) Extraer metadata textual existente “registro_n|last_pos|”
    int registro_n = 0;
    int last_pos = 0;
    int old_meta_len = 0;

    if (bloque_variable[0] >= '0' && bloque_variable[0] <= '9') {
        const char* p = std::strchr(bloque_variable, '|');
        if (!p) {
            // Formato inesperado: tratamos como vacío
            registro_n = 0;
            last_pos = bloqueSize - 1;
            old_meta_len = 0;
        } else {
            std::string s_reg(bloque_variable, p - bloque_variable);
            registro_n = std::atoi(s_reg.c_str());
            const char* q = std::strchr(p + 1, '|');
            if (!q) {
                registro_n = 0;
                last_pos = bloqueSize - 1;
                old_meta_len = 0;
            } else {
                std::string s_last(p + 1, q - (p + 1));
                last_pos = std::atoi(s_last.c_str());
                old_meta_len = (int)(q - bloque_variable) + 1;
            }
        }
    } else {
        // Bloque vacío
        registro_n = 0;
        last_pos = bloqueSize - 1;
        old_meta_len = 0;
    }

    // 3) Calcular longitud en bytes de registro_variable
    int registro_len = (int)std::strlen(registro_variable);
    cout<<"tamaño del registro: "<<registro_len<<endl;

    // 4) Posición provisional de escritura según last_pos actual
    int write_pos = last_pos - registro_len + 1;
    if (write_pos < 0) {
        // Ni siquiera caben los datos en el espacio derecho
        return false;
        cout<<"no hay pa más\n";
    }

    // 5) Construir metadata nueva usando registro_n+1 y write_pos
    int nuevo_registro_n = registro_n + 1;
    std::string new_meta = std::to_string(nuevo_registro_n) + "|" + std::to_string(write_pos) + "|";
    int new_meta_len = (int)new_meta.size();

    // 6) Verificar que la metadata nueva no invada el espacio de datos:
    //    debe cumplirse new_meta_len <= write_pos
    if (new_meta_len > write_pos) {
        return false;
        cout<<"se la come la metadata\n";
    }

    // 7) Verificar también, en términos de espacio libre global, que
    //    registro_len + (new_meta_len - old_meta_len) <= espacio_libre_bloque
    int delta_meta = new_meta_len - old_meta_len; // si es negativo, libera espacio
    int total_used = registro_len + delta_meta;
    if (total_used > espacio_libre_bloque) {
        return false;
        cout<<"falta espacio\n";
    }

    // 8) Copiar registro_variable en bloque_variable + write_pos
    std::memcpy(bloque_variable + write_pos, registro_variable, registro_len);

    // 9) Actualizar last_pos y registro_n
    last_pos = write_pos;
    registro_n = nuevo_registro_n;

    // 10) Escribir metadata nueva en el comienzo del bloque
    std::memmove(bloque_variable, new_meta.c_str(), new_meta_len);

    // 11) Limpiar bytes sobrantes si la metadata anterior era más larga
    if (new_meta_len < old_meta_len) {
        std::memset(bloque_variable + new_meta_len, '\0', old_meta_len - new_meta_len);
    }

    // 12) Ajustar el espacio libre global descontando lo usado
    espacio_libre_bloque -= total_used;

    return true;
}


bool eliminar_registro_variable(int bloqueN, int r_index) {
    // 1) Construir nombre de archivo: "BloqueN.txt"
    std::string filename = "Bloque" + std::to_string(bloqueN) + ".txt";

    // 2) Leer todo el contenido del archivo en una sola cadena
    std::ifstream ifs(filename);
    if (!ifs.is_open()) {
        std::cerr << "No se pudo abrir el archivo " << filename << "\n";
        return false;
    }
    std::string fileData;
    {
        std::string line;
        // Leerlo todo, incluyendo saltos de línea si los hubiera (pero asumimos texto plano continuo).
        while (std::getline(ifs, line)) {
            fileData += line;
            // Al leer con getline, desaparecen los '\n'. 
            // Si el bloque no los usaba, no importa. 
            // Si los usaba, habría que agregarlos de nuevo, pero aquí asumimos que no.
        }
    }
    ifs.close();

    // 3) Si el archivo está vacío, nada que eliminar
    if (fileData.empty()) {
        return false;
    }

    // 4) Parsear la cabecera textual: "registro_n|last_pos|offset1|size1|offset2|size2|...|"
    //    a) Encontrar la primera barra '|' para extraer registro_n
    size_t pos1 = fileData.find('|');
    if (pos1 == std::string::npos) {
        // Formato inválido
        return false;
    }
    int registro_n = std::stoi(fileData.substr(0, pos1));

    // 5) Validar que r_index esté en [1 .. registro_n]
    if (r_index < 1 || r_index > registro_n) {
        return false;
    }

    // 6) Encontrar la segunda barra '|' para extraer last_pos
    size_t pos2 = fileData.find('|', pos1 + 1);
    if (pos2 == std::string::npos) {
        return false;
    }
    int last_pos = std::stoi(fileData.substr(pos1 + 1, pos2 - (pos1 + 1)));

    // 7) Calcular cuántas entradas de offset/size hay: registro_n * 2
    //    Y recorrerlas para obtener vectores offsets[] y sizes[].
    std::vector<int> offsets;
    std::vector<int> sizes;
    offsets.reserve(registro_n);
    sizes.reserve(registro_n);

    size_t prev = pos2 + 1; // inicio de offset1
    for (int i = 0; i < registro_n; ++i) {
        // 7.a) Encontrar el siguiente '|' tras prev → fin de offset_i
        size_t p_offset_end = fileData.find('|', prev);
        if (p_offset_end == std::string::npos) return false;
        int off_i = std::stoi(fileData.substr(prev, p_offset_end - prev));

        // 7.b) Encontrar el siguiente '|' tras p_offset_end+1 → fin de size_i
        size_t p_size_end = fileData.find('|', p_offset_end + 1);
        if (p_size_end == std::string::npos) return false;
        int sz_i = std::stoi(fileData.substr(p_offset_end + 1, p_size_end - (p_offset_end + 1)));

        offsets.push_back(off_i);
        sizes.push_back(sz_i);

        prev = p_size_end + 1;
    }
    // Ahora, 'prev' apunta al primer byte después del último '|' de la cabecera
    size_t header_len = prev;

    // 8) Identificar el registro a eliminar (índice base 0)
    int idx0 = r_index - 1;
    int off_del = offsets[idx0];
    int size_del = sizes[idx0];

    // 9) Extraer la parte de "contenido" (datos) que comienza en header_len
    std::string content = fileData.substr(header_len);

    // 10) Calcular posición relativa en content donde comienza el registro:
    int rel_pos = off_del - (int)header_len;
    if (rel_pos < 0 || rel_pos + size_del > (int)content.size()) {
        return false;
    }

    // 11) Eliminar (borrar) los bytes del registro en 'content', llenando con espacios
    //      Para luego compactar, primero quitamos el rango [rel_pos .. rel_pos+size_del-1]:
    content.erase(rel_pos, size_del);
    //      Luego agregamos 'size_del' espacios al final, para mantener el tamaño original.
    content.append(size_del, ' ');

    // 12) Recalcular offsets para los registros que quedan:
    std::vector<int> new_offsets;
    std::vector<int> new_sizes;
    new_offsets.reserve(registro_n - 1);
    new_sizes.reserve(registro_n - 1);

    for (int i = 0; i < registro_n; ++i) {
        if (i == idx0) continue; // el que borramos, se salta

        int off_i = offsets[i];
        int sz_i = sizes[i];
        int new_off_i = off_i;
        // Si este registro estaba a la derecha (off_i > off_del), se mueve left by size_del
        if (off_i > off_del) {
            new_off_i = off_i - size_del;
        }
        new_offsets.push_back(new_off_i);
        new_sizes.push_back(sz_i);
    }

    // 13) Nuevo número de registros y nuevo last_pos (mínimo offset de new_offsets, si hay alguno)
    int new_registro_n = registro_n - 1;
    int new_last_pos = 0;
    if (new_registro_n > 0) {
        new_last_pos = *std::min_element(new_offsets.begin(), new_offsets.end());
    } else {
        // Si ya no quedan registros, dejamos last_pos apuntando al final del bloque:
        // asumimos un tamaño fijo, por ejemplo 256, pero NO lo necesitamos explícitamente aquí:
        new_last_pos = header_len; 
        // Podemos asignar header_len para indicar que no hay datos; el bloque quedará con solo metadata.
    }

    // 14) Reconstruir la nueva cabecera textual:
    //     "new_registro_n|new_last_pos|offset1|size1|offset2|size2|...|"
    std::string new_header = std::to_string(new_registro_n) + "|" + std::to_string(new_last_pos) + "|";
    for (int i = 0; i < new_registro_n; ++i) {
        new_header += std::to_string(new_offsets[i]) + "|";
        new_header += std::to_string(new_sizes[i]) + "|";
    }

    // 15) Armar el nuevo contenido completo del archivo: header + content
    std::string nuevoFileData = new_header + content;

    // 16) Escribir de vuelta en el mismo archivo, truncándolo primero
    std::ofstream ofs(filename, std::ios::trunc);
    if (!ofs.is_open()) {
        std::cerr << "No se pudo reabrir " << filename << " para escritura.\n";
        return false;
    }
    ofs << nuevoFileData;
    if (!ofs) {
        std::cerr << "Error al escribir en " << filename << "\n";
        ofs.close();
        return false;
    }
    ofs.close();

    return true;
}


//AUXILIARES, UNUSED
/*
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

*/