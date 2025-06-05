#pragma once

#include <cstdint>
#include <stdexcept>
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <fstream>
#include "Disco.h"
#include "DiscoPaths.h"

using namespace std;

char schemapath[] = "data\\usr\\db\\esquema.txt";
char Datapath[] = "data\\usr\\db\\";

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
    if (!found) throw std::runtime_error("Relacion no encontrada en el esquema");
    // Cada par de '#' delimita dos metadatos (offset y size), por eso dividimos
    return (hash_count + 1) / 2;
}

int get_nth_attribute(const char relacion[], const char atributo[]) {
    std::ifstream ifs(schemapath);
    if (!ifs.is_open()) {
        std::cerr << "Error: no se pudo abrir esquema.txt\n";
        return -1;
    }

    std::string line;
    while (std::getline(ifs, line)) {
        // 1) Encontrar el primer '#' para comparar la parte antes de él con `relacion`
        size_t p = line.find('#');
        if (p == std::string::npos) {
            continue; // línea mal formada, ignorar
        }
        // Comparar la subcadena [0..p) con relacion
        if (line.compare(0, p, relacion) != 0) {
            continue; // no es la relación buscada
        }

        // 2) A partir de p+1, iterar pares atributo#tipo
        size_t i = p + 1;
        int n_atributo = 0;

        while (i < line.size()) {
            // Leer el nombre de atributo (desde i hasta el siguiente '#')
            size_t next_hash = line.find('#', i);
            if (next_hash == std::string::npos) break;
            std::string nombre_atrib = line.substr(i, next_hash - i);
            ++n_atributo;
            // Si coincide, retornamos n_atributo
            if (nombre_atrib == atributo) {
                ifs.close();
                return n_atributo+1;
            }
            // Avanzar i para saltar el atributo leido
            i = next_hash + 1;
            // Ahora i apunta al comienzo del “tipo”; saltamos hasta el próximo '#'
            next_hash = line.find('#', i);
            if (next_hash == std::string::npos) break;
            // Avanzamos i justo después del '#', para leer el siguiente atributo
            i = next_hash + 1;
        }
        ifs.close();
        cout<<"\n atributo no hallado en relacion"<<relacion<<endl;
        return -1;
    }
    ifs.close();
    cout<<"\n relacion no habida"<<relacion<<endl;
    return -1;
}


std::string read_variable_register(const char registro_variable[], const char relacion[]) {
    // 1) Obtener el número de atributos (n pares offset|size|)
    int n = get_n_attributes(relacion);
    if (n <= 0) {
        throw std::runtime_error("Número de atributos inválido");
    }

    // 2) Arreglos dinámicos para guardar los offsets y sizes
    int* offsets = new int[n];
    int* sizes   = new int[n];

    // 3) Leer exactamente n pares “offset|size|”
    const char* p = registro_variable;
    for (int i = 0; i < n; ++i) {
        char* endptr;

        // ------ Leer offset_i ------
        long off = std::strtol(p, &endptr, 10);
        if (endptr == p || *endptr != '|') {
            delete[] offsets;
            delete[] sizes;
            throw std::runtime_error("Offset no es numérico o falta '|'");
        }
        offsets[i] = static_cast<int>(off);

        // Avanzar p justo tras el '|'
        p = endptr + 1;

        // ------ Leer size_i ------
        long sz = std::strtol(p, &endptr, 10);
        if (endptr == p || *endptr != '|') {
            delete[] offsets;
            delete[] sizes;
            throw std::runtime_error("Size no es numérico o falta '|'");
        }
        sizes[i] = static_cast<int>(sz);

        // Avanzar p justo tras el '|'
        p = endptr + 1;
    }

    // 4) En este punto, p apunta al primer carácter de la zona de datos:
    //    "atributo1atributo2...atributoN"
    //    Extraer cada campo usando offsets[i] y sizes[i].
    int registro_len = static_cast<int>(std::strlen(registro_variable));
    std::string resultado;
    resultado.reserve(n * 16);  // reserva aproximada

    for (int i = 0; i < n; ++i) {
        int off_i = offsets[i];
        int sz_i  = sizes[i];
        // Validar rangos
        if (off_i < 0 || sz_i < 0 || off_i + sz_i > registro_len) {
            delete[] offsets;
            delete[] sizes;
            throw std::runtime_error("Offset o size fuera de rango");
        }
        resultado.append(registro_variable + off_i, sz_i);
        if (i + 1 < n) {
            resultado.push_back('#');
        }
    }

    delete[] offsets;
    delete[] sizes;
    return resultado;
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


int insert_registro_variable(const char registro_variable[], int& espacio_libre_bloque, char* bloque_variable, Disco& disco) {
    int bloqueSize = disco.getTamBloque();

    // 1) Leer cabecera existente “registro_n|last_pos|offset1|size1|…|”
    int registro_n = 0;
    int last_pos = 0;
    int old_meta_len = 0;

    std::vector<int> offsets_old;
    std::vector<int> sizes_old;

    if (bloque_variable[0] >= '0' && bloque_variable[0] <= '9') {
        // a) extraer registro_n
        const char* p = std::strchr(bloque_variable, '|');
        if (!p) {
            registro_n = 0;
            last_pos = bloqueSize - 1;
            old_meta_len = 0;
        } else {
            registro_n = std::stoi(std::string(bloque_variable, p - bloque_variable));
            // b) extraer last_pos
            const char* q = std::strchr(p + 1, '|');
            if (!q) {
                registro_n = 0;
                last_pos = bloqueSize - 1;
                old_meta_len = 0;
            } else {
                last_pos = std::stoi(std::string(p + 1, q - (p + 1)));
                // c) leer pares offset/size
                const char* r = q + 1;
                old_meta_len = static_cast<int>(q - bloque_variable) + 1;
                offsets_old.resize(registro_n);
                sizes_old.resize(registro_n);
                for (int i = 0; i < registro_n; ++i) {
                    const char* pr = std::strchr(r, '|');
                    if (!pr) {
                        // formato inesperado: tratar como bloque vacío
                        registro_n = 0;
                        last_pos = bloqueSize - 1;
                        offsets_old.clear();
                        sizes_old.clear();
                        old_meta_len = 0;
                        break;
                    }
                    int off_i = std::stoi(std::string(r, pr - r));

                    const char* ps = std::strchr(pr + 1, '|');
                    if (!ps) {
                        registro_n = 0;
                        last_pos = bloqueSize - 1;
                        offsets_old.clear();
                        sizes_old.clear();
                        old_meta_len = 0;
                        break;
                    }
                    int sz_i = std::stoi(std::string(pr + 1, ps - (pr + 1)));

                    offsets_old[i] = off_i;
                    sizes_old[i] = sz_i;

                    r = ps + 1;
                    old_meta_len = static_cast<int>(ps - bloque_variable) + 1;
                }
            }
        }
    } else {
        // Bloque inicialmente vacío
        registro_n = 0;
        last_pos = bloqueSize - 1;
        old_meta_len = 0;
        offsets_old.clear();
        sizes_old.clear();
    }

    // -- DEBUG --
    std::cerr << "[DEBUG] registro_n viejo = " << registro_n
              << "   last_pos viejo = " << last_pos
              << "   old_meta_len = " << old_meta_len << "\n";

    // 2) Longitud en bytes del registro completo (incluida su cabecera interna)
    int registro_len = static_cast<int>(std::strlen(registro_variable));
    std::cerr << "[DEBUG] registro_len = " << registro_len << "\n";

    // 3) Calcular write_pos para insertar el registro al lado izquierdo del último
    int write_pos = last_pos - registro_len + 1;
    std::cerr << "[DEBUG] write_pos = " << last_pos << " - " << registro_len << " + 1 = " << write_pos << "\n";
    if (write_pos < 0) {
        std::cerr << "[DEBUG] ¡write_pos < 0! No cabe el nuevo registro.\n";
        return 0;
    }

    // 4) Calcular espacio interno restante (entre final de old_meta_len y last_pos)
    int espacio_interno = last_pos - old_meta_len;
    std::cerr << "[DEBUG] espacio_interno = " << last_pos << " - " << old_meta_len << " = " << espacio_interno << "\n";

    // 5) Preparar nuevos vectores de offsets/sizes en cabecera de bloque
    int nuevo_registro_n = registro_n + 1;
    std::vector<int> new_offsets = offsets_old;
    std::vector<int> new_sizes   = sizes_old;
    new_offsets.push_back(write_pos);
    new_sizes.push_back(registro_len);

    // 6) Calcular new_last_pos (mínimo de todos los offsets)
    int new_last_pos = *std::min_element(new_offsets.begin(), new_offsets.end());
    std::cerr << "[DEBUG] new_last_pos = " << new_last_pos << "\n";

    // 7) Reconstruir cabecera de bloque: "n_registros|last_pos-1|offset1|size1|...|"
    std::ostringstream sh;
    sh << nuevo_registro_n << "|" << (new_last_pos - 1) << "|";
    for (int i = 0; i < nuevo_registro_n; ++i) {
        sh << new_offsets[i] << "|" << new_sizes[i] << "|";
    }
    std::string new_meta = sh.str();
    int new_meta_len = static_cast<int>(new_meta.size());
    std::cerr << "[DEBUG] new_meta = \"" << new_meta << "\"\n";
    std::cerr << "[DEBUG] new_meta_len = " << new_meta_len << "\n";

    // 8) Verificar que la cabecera nueva quepa antes del área de datos
    if (new_meta_len > write_pos) {
        std::cerr << "[DEBUG] new_meta_len > write_pos → no cabe\n";
        return 0;
    }
    int delta_meta = new_meta_len - old_meta_len;
    int total_used = registro_len + delta_meta;
    std::cerr << "[DEBUG] delta_meta = " << delta_meta << ", total_used = " << total_used << "\n";
    if (total_used > espacio_interno) {
        std::cerr << "[DEBUG] No hay suficiente espacio interno → " << total_used << " > " << espacio_interno << "\n";
        return 2;
    }

    // 9) Sobrescribir únicamente la cabecera en bloque_variable
    std::memmove(bloque_variable, new_meta.c_str(), new_meta_len);
    if (old_meta_len > new_meta_len) {
        std::memset(bloque_variable + new_meta_len, 0, old_meta_len - new_meta_len);
    }

    // 10) Copiar el registro tal cual (incluyendo su cabecera interna) en write_pos
    std::memcpy(bloque_variable + write_pos, registro_variable, registro_len);
    std::cerr << "[DEBUG] Registro copiado en offset " << write_pos << "\n";

    // 11) Actualizar espacio_libre_bloque
    espacio_libre_bloque = espacio_interno - total_used;
    std::cerr << "[DEBUG] espacio_libre_bloque ahora = " << espacio_libre_bloque << "\n";

    return 1;
}


bool eliminar_registro_variable(int bloqueN, int r_index, int tamBloque) {
    // --- 1) Abrir y leer TODO el contenido en fileData (binario) ---
    std::string filename = "Bloque" + std::to_string(bloqueN) + ".txt";
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs.is_open()) {
        std::cerr << "Error: no se pudo abrir " << filename << "\n";
        return false;
    }
    std::ostringstream oss;
    oss << ifs.rdbuf();
    std::string fileData = oss.str();
    ifs.close();

    if (fileData.empty()) {
        // Bloque vacío o formato inválido
        return false;
    }

    // --- 2) Parsear cabecera antigua: "registro_n|last_pos|offset1|size1|offset2|size2|...|" ---
    size_t pos1 = fileData.find('|');
    if (pos1 == std::string::npos) return false;
    int registro_n = std::stoi(fileData.substr(0, pos1));

    if (r_index < 1 || r_index > registro_n) {
        // Índice fuera de rango
        return false;
    }

    size_t pos2 = fileData.find('|', pos1 + 1);
    if (pos2 == std::string::npos) return false;
    int last_pos = std::stoi(fileData.substr(pos1 + 1, pos2 - (pos1 + 1)));

    // 3) Extraer offsets_old[0..registro_n-1] y sizes_old[0..registro_n-1]
    std::vector<int> offsets_old(registro_n);
    std::vector<int> sizes_old(registro_n);
    size_t cursor = pos2 + 1;
    for (int i = 0; i < registro_n; ++i) {
        // offset_i
        size_t off_end = fileData.find('|', cursor);
        if (off_end == std::string::npos) return false;
        offsets_old[i] = std::stoi(fileData.substr(cursor, off_end - cursor));
        cursor = off_end + 1;

        // size_i
        size_t sz_end = fileData.find('|', cursor);
        if (sz_end == std::string::npos) return false;
        sizes_old[i] = std::stoi(fileData.substr(cursor, sz_end - cursor));
        cursor = sz_end + 1;
    }
    size_t old_header_len = cursor;  // longitud (en bytes) del header antiguo

    // --- 4) Extraer “content” (datos) que viene después del header ---
    std::string content = fileData.substr(old_header_len);

    // --- 5) Identificar off_del y size_del del registro a eliminar (0‐based) ---
    int idx0    = r_index - 1;
    int off_del = offsets_old[idx0];
    int size_del= sizes_old[idx0];

    // 6) Verificar que ese registro efectivamente cabe en “content”
    int rel_pos = off_del - static_cast<int>(old_header_len);
    if (rel_pos < 0 || rel_pos + size_del > static_cast<int>(content.size())) {
        return false;
    }

    // --- 7) Borrar los bytes del registro en “content” y compactar (rellenar con espacios) ---
    content.erase(rel_pos, size_del);
    content.append(size_del, ' ');

    // --- 8) Construir nuevos offsets, sumando size_del a los offsets < off_del ---
    int new_registro_n = registro_n - 1;
    std::vector<int> new_offsets;
    std::vector<int> new_sizes;
    new_offsets.reserve(new_registro_n);
    new_sizes.reserve(new_registro_n);

    for (int i = 0; i < registro_n; ++i) {
        if (i == idx0) continue; // saltamos el registro eliminado
        int off_i = offsets_old[i];
        int sz_i  = sizes_old[i];
        int new_off_i = off_i;
        if (off_i < off_del) {
            // Si este registro estaba “a la izquierda” (offset menor),
            // lo movemos a la derecha sumándole size_del.
            new_off_i = off_i + size_del;
        }
        // Si off_i > off_del, no lo tocamos (queda igual).
        new_offsets.push_back(new_off_i);
        new_sizes.push_back(sz_i);
    }

    // --- 9) Calcular new_last_pos = mínimo de new_offsets (o tamBloque - 1 si vacío) ---
    int new_last_pos;
    if (new_registro_n > 0) {
        new_last_pos = new_offsets[0];
        for (int i = 1; i < new_registro_n; ++i) {
            if (new_offsets[i] < new_last_pos) {
                new_last_pos = new_offsets[i];
            }
        }
    } else {
        new_last_pos = tamBloque - 1;
    }

    // --- 10) Construir el nuevo header en texto: "<n>|<last_pos>|<offset1>|<size1>|...|" ---
    std::ostringstream sh;
    sh << new_registro_n << "|" << new_last_pos << "|";
    for (int i = 0; i < new_registro_n; ++i) {
        sh << new_offsets[i] << "|" << new_sizes[i] << "|";
    }
    std::string new_header = sh.str();

    // --- 11) Concatenar new_header + content compacto ---
    std::string nuevoFileData = new_header + content;

    // --- 12) Escribir de vuelta en modo binario/truncar ---
    std::ofstream ofs(filename, std::ios::binary | std::ios::trunc);
    if (!ofs.is_open()) {
        std::cerr << "Error: no se pudo reabrir " << filename << " para escritura\n";
        return false;
    }
    ofs << nuevoFileData;
    ofs.close();

    return true;
}

bool modify_registro_variable(int NBloque, int r_index, const char relacion[], const char atributo[], const char new_value[],Disco&disco) {
    int tamBloque = disco.getTamBloque();
    std::string filename = "Bloque" + std::to_string(NBloque) + ".txt";

    // 1) Leer todo el bloque en fileData
    std::ifstream ifs(filename);
    if (!ifs.is_open()) {
        std::cerr << "Error: no se pudo abrir " << filename << "\n";
        return false;
    }
    std::string fileData;
    {
        std::string line;
        while (std::getline(ifs, line)) {
            fileData += line;
        }
    }
    ifs.close();

    if (fileData.empty()) {
        std::cerr << "Error: bloque vacío o formato inválido\n";
        return false;
    }

    // 2) Parsear la cabecera para extraer registro_n y los pares (offset, size)
    size_t p1 = fileData.find('|');
    if (p1 == std::string::npos) return false;
    int registro_n = std::stoi(fileData.substr(0, p1));
    if (r_index < 1 || r_index > registro_n) {
        std::cerr << "Error: r_index fuera de rango\n";
        return false;
    }

    size_t p2 = fileData.find('|', p1 + 1);
    if (p2 == std::string::npos) return false;
    // cursor para leer pares
    size_t cursor = p2 + 1;
    std::vector<int> offsets, sizes;
    offsets.reserve(registro_n);
    sizes.reserve(registro_n);

    for (int i = 0; i < registro_n; ++i) {
        // offset i
        size_t poff_end = fileData.find('|', cursor);
        if (poff_end == std::string::npos) return false;
        int off_i = std::stoi(fileData.substr(cursor, poff_end - cursor));
        cursor = poff_end + 1;
        // size i
        size_t psize_end = fileData.find('|', cursor);
        if (psize_end == std::string::npos) return false;
        int sz_i = std::stoi(fileData.substr(cursor, psize_end - cursor));
        cursor = psize_end + 1;
        offsets.push_back(off_i);
        sizes.push_back(sz_i);
    }
    size_t header_len = cursor;

    // 3) Obtener offset y size del registro a modificar (0-based)
    int idx0 = r_index - 1;
    int off_old = offsets[idx0];
    int size_old = sizes[idx0];
    if (off_old + size_old > (int)fileData.size()) return false;

    // 4) Extraer registro crudo y pasar a contenido “campo1#...”
    std::string registro_raw = fileData.substr(off_old, size_old);
    std::cout<<registro_raw<<std::endl;
    std::string contenido = read_variable_register(registro_raw.c_str(),relacion);

    if (contenido.empty() && size_old > 0) {
        std::cerr << "Error: no se pudo leer el registro viejo\n";
        return false;
    }

    // 5) Calcular n_atributo
    int n_atributo = get_nth_attribute(relacion, atributo);
    if (n_atributo < 1) {
        std::cerr << "Error: atributo no encontrado en esquema\n";
        return false;
    }

    // 6) Reconstruir temp: sustituir solo el campo n_atributo por new_value,
    //    sin usar split en vectores, sino contando ‘#’.
    std::string temp;
    temp.reserve(contenido.size() + std::strlen(new_value));
    int current_field = 1;
    size_t read_pos = 0;
    size_t len = contenido.size();

    // 6.a) Si es el primer campo
    if (n_atributo == 1) {
        // Encontrar primer ‘#’ (si existe) para copiar el sufijo
        size_t first_hash = contenido.find('#');
        temp += new_value;
        if (first_hash != std::string::npos) {
            temp += contenido.substr(first_hash); 
        }
    } else {
        // 6.b) Copiar campos 1..(n_atributo-1)
        // Encuentro la posición inmediata después del (n_atributo-1)-ésimo ‘#’
        size_t pos = 0;
        int hashes_seen = 0;
        while (pos < len && hashes_seen < (n_atributo - 1)) {
            if (contenido[pos] == '#') {
                ++hashes_seen;
            }
            ++pos;
        }
        // pos es el índice justo después del (n_atributo-1)-ésimo ‘#’
        // Copiamos hasta pos (excluyendo) para mantener los ‘#’ previos
        temp += contenido.substr(0, pos);

        // 6.c) Insertar new_value
        temp += new_value;

        // 6.d) Hallar siguiente ‘#’ a partir de pos (fin del campo reemplazado)
        size_t next_hash = contenido.find('#', pos);
        if (next_hash != std::string::npos) {
            // Copiar todo desde ese ‘#’ hasta el final
            temp += contenido.substr(next_hash);
        }
        // Si next_hash == npos, nada más que copiar (eliminamos el antiguo campo al final)
    }

    // 7) Formatear el nuevo registro con format_registro_variable
    char* nuevo_registro = format_registro_variable(temp.c_str(), relacion);
    if (!nuevo_registro) {
        std::cerr << "Error: format_registro_variable falló\n";
        return false;
    }

    // 8) Eliminar el registro viejo
    if (!eliminar_registro_variable(NBloque, r_index, disco.getTamBloque())) {
        std::cerr << "Error: no se pudo eliminar registro viejo\n";
        delete[] nuevo_registro;
        return false;
    }

    // 9) Leer el bloque actualizado de nuevo
    std::ifstream ifs2(filename);
    if (!ifs2.is_open()) {
        std::cerr << "Error: no se pudo reabrir " << filename << "\n";
        delete[] nuevo_registro;
        return false;
    }
    std::string fileData2;
    {
        std::string line;
        while (std::getline(ifs2, line)) {
            fileData2 += line;
        }
    }
    ifs2.close();

    // 10) Construir buffer de tamaño tamBloque con ceros y copiar fileData2
    char* bloque_buffer = new char[tamBloque];
    std::memset(bloque_buffer, 0, tamBloque);
    int copy_len = (int)std::min((size_t)tamBloque, fileData2.size());
    std::memcpy(bloque_buffer, fileData2.data(), copy_len);

    // 11) Calcular espacio libre actual
    int espacio_libre_bloque = tamBloque - (int)fileData2.size();

    // 12) Insertar el nuevo registro en el bloque en memoria
    if (!insert_registro_variable(nuevo_registro,espacio_libre_bloque,bloque_buffer, disco)) {
        std::cerr << "Error: no se pudo insertar el registro modificado\n";
        delete[] nuevo_registro;
        delete[] bloque_buffer;
        return false;
    }
    delete[] nuevo_registro;

    // 13) Volcar bloque_buffer de regreso a “BloqueN.txt” (modo texto),
    //      escribiendo solo hasta el último byte no nulo.
    int endPos = tamBloque - 1;
    while (endPos >= 0 && bloque_buffer[endPos] == '\0') {
        --endPos;
    }
    std::string newFileData(bloque_buffer, endPos + 1);
    delete[] bloque_buffer;

    std::ofstream ofs3(filename, std::ios::trunc);
    if (!ofs3.is_open()) {
        std::cerr << "Error: no se pudo abrir " << filename << " para reescribir\n";
        return false;
    }
    ofs3 << newFileData;
    ofs3.close();

    return true;
}
//AUXILIARES

bool volcar_bloque_a_archivo(const char* bloque_variable, int tamBloque) {
    // 1) Encontrar el índice del último byte no nulo ('\0')
    int endPos = tamBloque - 1;
    while (endPos >= 0 && bloque_variable[endPos] == '\0') {
        --endPos;
    }
    if (endPos < 0) {
        // Todo el bloque está lleno de '\0', no hay nada que volcar
        std::ofstream ofs("Bloque1.txt");
        if (!ofs.is_open()) {
            std::cerr << "Error: no se pudo abrir bloque.txt para escritura.\n";
            return false;
        }
        ofs.close();
        return true;
    }

    // 2) Abrir bloque.txt en modo texto (por defecto) y escribir desde 0 hasta endPos
    std::ofstream ofs("Bloque1.txt");
    if (!ofs.is_open()) {
        std::cerr << "Error: no se pudo abrir bloque.txt para escritura.\n";
        return false;
    }

    // Construimos un std::string a partir del buffer hasta endPos+1 bytes
    std::string contenido(bloque_variable, endPos + 1);
    ofs << contenido;
    if (!ofs) {
        std::cerr << "Error: fallo al escribir en bloque.txt.\n";
        ofs.close();
        return false;
    }

    ofs.close();
    return true;
}

static bool adicionarRegistroUnico(const char* registroTxt, const char* relacion, Disco& disco) {
    // --- 0) Preparar datos y calcular longitud variable del registro ---
    static char regBuf[MAX_BUF];   // Aquí almacenaremos el registro ya “formateado” (variable)
    static int regLen;             // Longitud efectivamente ocupada en regBuf

    // 0.1) Debug entrada
    printf("DEBUG: Entrando a adicionarRegistroUnico para relacion='%s', registroTxt='%s'\n",
           relacion, registroTxt);

    // 0.2) Copia segura de registroTxt sin CR/LF
    char registroSinLF[MAX_BUF];
    strncpy(registroSinLF, registroTxt, MAX_BUF - 1);
    registroSinLF[MAX_BUF - 1] = '\0';
    size_t l = strlen(registroSinLF);
    while (l > 0 && (registroSinLF[l - 1] == '\n' || registroSinLF[l - 1] == '\r')) {
        registroSinLF[--l] = '\0';
    }

    // 0.3) Generar registro en formato variable (en lugar de RLF)
    //     Supongamos que format_registro_variable toma:
    //       (texto_original, nombre_relacion, buffer_salida, &longitud_salida)
    //     y devuelve true/false según éxito.
    if (!format_registro_variable(registroSinLF, relacion)) {
        printf("DEBUG: Error en format_registro_variable. Saliendo.\n");
        return false;
    }
    printf("DEBUG: format_registro_variable terminó correctamente. regLen = %d\n", regLen);
    // Verificar que no haya saltos de línea en regBuf[0..regLen-1]
    for (int i = 0; i < regLen; ++i) {
        if (regBuf[i] == '\n' || regBuf[i] == '\r') {
            printf("ERROR: detectado salto de línea en regBuf en posición %d\n", i);
        }
    }

    // 0.4) Mostrar contenido de longitudFija.txt (solo debug; podría omitirse si ya no aplica)
    printf(">>> Debug: Leyendo %s para ver su contenido:\n", rutaLongitudFija);
    FILE* ftmp = fopen(rutaLongitudFija, "r");
    if (ftmp) {
        char buf[MAX_BUF];
        while (fgets(buf, MAX_BUF, ftmp)) {
            printf("   %s", buf);
        }
        fclose(ftmp);
    }
    else {
        printf("   ¡ERROR: no se pudo abrir %s!\n", rutaLongitudFija);
    }

    // 0.5) Con registro variable, ya no necesitamos solicitar “longitud fija” de registro
    //     (antes: obtenerRegistroSize(relacion, &registroSize)).
    //     En su lugar, el tamaño a insertar será regLen. 
    int tamRegistro = regLen;
    printf("DEBUG: tamRegistro (variable) = %d\n", tamRegistro);
    if (tamRegistro <= 0) {
        fprintf(stderr, "ERROR: registro variable de longitud inválida para %s\n", relacion);
        return false;
    }

    // 0.6) Calcular longitud fija máxima de “bloque” en dirBloques.txt (se mantiene igual)
    int fixedLen = disco.obtenerLongitudMaximaBloque1();
    if (fixedLen <= 0) {
        printf("ERROR: No se pudo obtener longitud fija máxima de bloque.\n");
        return false;
    }
    printf("DEBUG: fixedLen (longitud de cada bloque) = %d bytes\n", fixedLen);

    // 1) Abrir dirBloques.txt en modo lectura y escritura
    printf("DEBUG: intentando abrir dirBloques en '%s'\n", rutaDirBloques);
    FILE* fdir = fopen(rutaDirBloques, "r+");
    if (!fdir) {
        perror("ERROR: No se puede abrir dirBloques.txt");
        return false;
    }
    printf("DEBUG: dirBloques.txt abierto con éxito.\n");

    char lineaBloque[MAX_BUF];
    int nroBloque = 0;
    bool foundBlock = false;
    char codSectorLibre[MAX_STR_LEN] = { 0 };
    long posLineaBloque = 0;

    int espacioLibreBloque = 0;
    int tamUtilAntes = 0;
    int espacioLibreSectorAntes = 0;

    char savedSectores[MAX_BUF] = { 0 }; // Para guardar sectores del bloque elegido

    // 2) Buscar bloque y sector libres (cada bloque ocupa fixedLen bytes total)
    printf("DEBUG: Buscando bloque libre para tamaño de registro = %d\n", tamRegistro);
    while (true) {
        posLineaBloque = ftell(fdir);

        int lenBloque = disco.leerBloqueConSeparador(fdir, lineaBloque, MAX_BUF);
        if (lenBloque <= 0) {
            printf("DEBUG: leerBloqueConSeparador devolvió %d (fin de bloques)\n", lenBloque);
            break;
        }

        // Saltar bloques vacíos generados por "||"
        if (lenBloque == 2 && lineaBloque[0] == '|' && lineaBloque[1] == '|') {
            continue;
        }

        nroBloque++;
        printf("DEBUG: Leyendo Bloque #%d (lenBloque = %d bytes)\n", nroBloque, lenBloque);

        // 2.1) Extraer espacioLibreBloque sin strtok:
        char tempEspacio[MAX_BUF];
        strncpy(tempEspacio, lineaBloque + 1, MAX_BUF - 1);
        tempEspacio[MAX_BUF - 1] = '\0';

        char* posHash = strchr(tempEspacio, '#');
        if (!posHash) {
            printf("DEBUG: Bloque #%d sin token inicial. Continúa.\n", nroBloque);
            continue;
        }
        *posHash = '\0';
        espacioLibreBloque = safe_atoi(tempEspacio);
        *posHash = '#';
        printf("DEBUG: Bloque #%d espacioLibreBloque = %d\n", nroBloque, espacioLibreBloque);

        tamUtilAntes = getTamBloqueFromDisco(disco) - espacioLibreBloque;
        if (espacioLibreBloque < tamRegistro) {
            printf("DEBUG: Bloque #%d NO cabe (espacio %d < %d). Pasa al siguiente.\n",
                   nroBloque, espacioLibreBloque, tamRegistro);
            continue;
        }

        // 2.2) Extraer la lista de sectores en copiaSectores
        char copiaSectores[MAX_BUF];
        strncpy(copiaSectores, lineaBloque + 1, MAX_BUF - 1);
        copiaSectores[MAX_BUF - 1] = '\0';

        char* p = strstr(copiaSectores, "#_");
        if (!p) {
            printf("DEBUG: Bloque #%d no tiene '#_' (no hay lista de sectores). Continúa.\n", nroBloque);
            continue;
        }
        p += 2;

        // 2.3) Iterar cada par "<espLibreSector>#<codSector>#_"
        while (*p) {
            char* inicioEspacioSector = p;
            while (*p && *p != '#') p++;
            if (*p != '#') break;
            *p = '\0';
            int espacioLibreSector = atoi(inicioEspacioSector);
            *p = '#';
            p++;

            char* inicioCodSector = p;
            while (*p && *p != '#') p++;
            if (*p != '#') break;
            *p = '\0';
            char sectorCode[MAX_STR_LEN];
            strncpy(sectorCode, inicioCodSector, MAX_STR_LEN - 1);
            sectorCode[MAX_STR_LEN - 1] = '\0';
            *p = '#';

            printf("DEBUG: Bloque #%d chequeando sector '%s' con espacio %d\n",
                   nroBloque, sectorCode, espacioLibreSector);

            char* nextPair = strstr(p, "#_");
            if (espacioLibreSector < tamRegistro) {
                printf("DEBUG: Sector '%s' no cabe (espacio %d < %d). Siguiente.\n",
                       sectorCode, espacioLibreSector, tamRegistro);
                if (!nextPair) break;
                p = nextPair + 2;
                continue;
            }

            // Sector válido encontrado
            strncpy(codSectorLibre, sectorCode, MAX_STR_LEN - 1);
            espacioLibreSectorAntes = espacioLibreSector;
            // Guardar la lista completa de "sectores" para usarla después
            strncpy(savedSectores, copiaSectores, MAX_BUF - 1);
            savedSectores[MAX_BUF - 1] = '\0';
            foundBlock = true;
            printf("DEBUG: Seleccionado Bloque #%d, Sector '%s'. espacioLibreSectorAntes = %d\n",
                   nroBloque, codSectorLibre, espacioLibreSectorAntes);
            break;
        }
        if (foundBlock) break;
    }

    if (!foundBlock) {
        printf("DEBUG: No se encontró ningún bloque con espacio suficiente.\n");
        fclose(fdir);
        return false;
    }

    // 2.4) Calcular nuevo espacio de bloque
    int espacioLibreBloqueAntes = espacioLibreBloque;
    int tamUtilNuevo = tamUtilAntes + tamRegistro;
    int espacioBloqueNuevo = getTamBloqueFromDisco(disco) - tamUtilNuevo;
    printf("DEBUG: Bloque #%d espacioLibreBloqueAntes = %d  => espacioBloqueNuevo = %d\n",
           nroBloque, espacioLibreBloqueAntes, espacioBloqueNuevo);
    if (espacioBloqueNuevo < 0 || espacioBloqueNuevo > getTamBloqueFromDisco(disco)) {
        printf("ERROR: valores fuera de rango en Bloque #%d: espacioBloqueNuevo = %d\n",
               nroBloque, espacioBloqueNuevo);
    }

    // 3) Volver al inicio exacto del bloque en dirBloques.txt (posición en bytes)
    printf("DEBUG: Volviendo a byte offset %ld para sobreescribir bloque #%d\n",
           posLineaBloque, nroBloque);
    fseek(fdir, posLineaBloque, SEEK_SET);

    // 4) Reconstruir el bloque completo de exactly fixedLen bytes
    char bufferNueva[MAX_BUF] = { 0 };
    int ofsN = 0;

    // 4.1) Escribir el '|' inicial
    bufferNueva[ofsN++] = '|';

    // 4.2) Prefijo actualizado "<espBloqueNuevo>#2#BLOQUE#<nroBloque>#<tamBloque>#_"
    ofsN += snprintf(bufferNueva + ofsN, MAX_BUF - ofsN,
                     "%d#2#BLOQUE#%d#%d#_",
                     espacioBloqueNuevo,
                     nroBloque,
                     getTamBloqueFromDisco(disco));
    printf("DEBUG: bufferNueva hasta prefijo = '%.*s'\n", ofsN, bufferNueva);

    // 4.3) Copiar cada par "<esp>#<cod>#_", ajustando el sector elegido
    {
        char* p2 = strstr(savedSectores, "#_");
        if (p2) {
            p2 += 2; // al inicio del primer "<espLibreSector>"
            while (*p2) {
                int espSec = atoi(p2);
                while (*p2 && *p2 != '#') ++p2;
                if (!*p2) break;
                ++p2;
                char sectorCode2[MAX_STR_LEN] = { 0 };
                int pos2 = 0;
                while (*p2 && *p2 != '#') {
                    sectorCode2[pos2++] = *p2++;
                }
                sectorCode2[pos2] = '\0';

                int nuevoEsp = espSec;
                if (strcmp(sectorCode2, codSectorLibre) == 0) {
                    nuevoEsp = espSec - tamRegistro;
                }
                int n = snprintf(bufferNueva + ofsN, MAX_BUF - ofsN,
                                 "%d#%s#_", nuevoEsp, sectorCode2);
                printf("DEBUG: Añadiendo \"%d#%s#_\" => n = %d\n", nuevoEsp, sectorCode2, n);
                ofsN += n;

                char* next2 = strstr(p2, "#_");
                if (!next2) break;
                p2 = next2 + 2;
            }
        }
    }

    // 4.4) Poner padding '@' hasta fixedLen-1, y cerrar con '|'
    if (ofsN < fixedLen - 1) {
        for (int i = ofsN; i < fixedLen - 1; i++) {
            bufferNueva[i] = '@';
        }
        bufferNueva[fixedLen - 1] = '|';
    }
    else {
        // Si por alguna razón se pasó, truncar y forzar '|' al final
        bufferNueva[fixedLen - 1] = '|';
    }

    printf("DEBUG: Bloque reconstruido (fixedLen %d bytes):\n", fixedLen);
    printf("       %.*s\n", fixedLen, bufferNueva);

    // 4.5) Sobreescribir exactamente fixedLen bytes en dirBloques.txt
    fseek(fdir, posLineaBloque, SEEK_SET);
    size_t escritosDir = fwrite(bufferNueva, 1, fixedLen, fdir);
    fflush(fdir);
    fclose(fdir);
    printf("DEBUG: Reescritura de bloque #%d en dirBloques.txt: %zu bytes escritos (esperados %d)\n",
           nroBloque, escritosDir, fixedLen);

    // -------------------------------------------------------------
    // 5) Ahora crear/abrir BloqueN.txt y aprovechar espacio libre (bitmap)
    // -------------------------------------------------------------
    char rutaBloque[MAX_PATH_LEN];
    snprintf(rutaBloque, sizeof(rutaBloque),
             "%sBLOQUES\\Bloque%d.txt",
             discoNuevoPath, nroBloque);
    printf("DEBUG: Ruta BloqueN.txt = '%s'\n", rutaBloque);

    FILE* fbloc = fopen(rutaBloque, "r+b");
    size_t raw_header_len = 0;
    int numRegAnt = 0;
    int numMaxAnt = 0;
    static char bitmapAnt[MAX_BUF];

    if (!fbloc) {
        // Bloque no existe: crearlo con calcularCabeceraBloque(...) 
        //   NOTA: antes se usaba registroSize, ahora pasamos tamRegistro (variable)
        printf("DEBUG: BloqueN.txt NO existe. Se intentará crearlo.\n");
        char headerBuf[MAX_BUF];
        size_t headerLen;
        int numMax;
        calcularCabeceraBloque(getTamBloqueFromDisco(disco), tamRegistro,
                               headerBuf, &headerLen, &numMax);
        printf("DEBUG: calcularCabeceraBloque devolvió: headerLen=%zu, numMax=%d\n", headerLen, numMax);
        printf("DEBUG: Contenido headerBuf: '%.*s'\n", (int)headerLen, headerBuf);

        fbloc = fopen(rutaBloque, "wb");
        if (!fbloc) {
            perror("ERROR: No se pudo crear BloqueN.txt");
            return false;
        }
        size_t escritosHeader = fwrite(headerBuf, 1, headerLen, fbloc);
        fflush(fbloc);
        if (escritosHeader != headerLen) {
            printf("ERROR: se escribieron %zu bytes de header, esperados %zu\n", escritosHeader, headerLen);
        }
        else {
            printf("DEBUG: Cabecera escrita correctamente en BloqueN.txt (%zu bytes)\n", escritosHeader);
        }
        // Mover cursor justo antes del '/'
        fseek(fbloc, -1, SEEK_END);
        raw_header_len = headerLen;
        numRegAnt = 1;                // Ya insertamos este registro
        numMaxAnt = 0;                // Se determinará después en calcularCabeceraBloque
        for (int i = 0; i < numMaxAnt; i++) bitmapAnt[i] = '0';
        bitmapAnt[numMaxAnt] = '\0';

        // 10) Escribir el registro variable inmediatamente después de la cabecera
        long offsetInicial = raw_header_len;
        fseek(fbloc, offsetInicial, SEEK_SET);
        size_t escritosReg2 = fwrite(regBuf, 1, regLen, fbloc);
        fputc('|', fbloc);
        fflush(fbloc);
        if (escritosReg2 != (size_t)regLen) {
            printf("ERROR: Se escribieron %zu bytes de registro variable, esperados %d\n", escritosReg2, regLen);
        }
        else {
            printf("DEBUG: Registro variable de %d bytes escrito correctamente (+ '|').\n", regLen);
        }
        fclose(fbloc);

        // 11) Volcar a sectores
        printf("DEBUG: Llamando a volcarBloqueASectores para nuevo bloque #%d\n", nroBloque);
        disco.volcarBloqueASectores(nroBloque);

        // 12) Actualizar catalogo.txt
        char rutaCatalogo2[MAX_PATH_LEN];
        snprintf(rutaCatalogo2, sizeof(rutaCatalogo2),
                 "%s%s", discoNuevoPath, "catalogo.txt");
        printf("DEBUG: Abriendo catalogo.txt en modo 'a' para agregar %s|Bloque%d.txt\n",
               relacion, nroBloque);
        FILE* fcat2 = fopen(rutaCatalogo2, "a");
        if (fcat2) {
            char rutaBloqueCat[MAX_PATH_LEN];
            snprintf(rutaBloqueCat, sizeof(rutaBloqueCat),
                     "%sBLOQUES\\Bloque%d.txt", discoNuevoPath, nroBloque);
            fprintf(fcat2, "%s|%s\n", relacion, rutaBloqueCat);
            fclose(fcat2);
            printf("DEBUG: Entrada agregada a catalogo.txt: '%s|%s'\n", relacion, rutaBloqueCat);
        }
        else {
            perror("ERROR: No se pudo abrir catalogo.txt para escritura");
        }

        printf("DEBUG: adicionarRegistroUnico (nuevo bloque) finalizado para Bloque #%d\n", nroBloque);
        return true;
    }
    else {
        // Bloque existe: leer cabecera hasta '/'
        printf("DEBUG: BloqueN.txt ya existe. Releyendo cabecera...\n");
        size_t pos = 0;
        int c;
        rewind(fbloc);
        while ((c = fgetc(fbloc)) != EOF) {
            pos++;
            if (c == '/') break;
            if (pos >= MAX_BUF - 1) break;
        }
        raw_header_len = pos;
        printf("DEBUG: raw_header_len detectado = %zu\n", raw_header_len);

        rewind(fbloc);
        char cabTmp3[MAX_BUF];
        if (raw_header_len > MAX_BUF - 1) raw_header_len = MAX_BUF - 1;
        size_t leidosCab3 = fread(cabTmp3, 1, raw_header_len, fbloc);
        cabTmp3[leidosCab3] = '\0';
        printf("DEBUG: Contenido leído de cabecera (%zu bytes): '%.*s'\n",
               leidosCab3, (int)raw_header_len, cabTmp3);

        // Parsear numRegAnt#numMaxAnt#bitmapAnt/
        char* q1 = strchr(cabTmp3, '#');
        *q1 = '\0';
        numRegAnt = safe_atoi(cabTmp3);
        *q1 = '#';
        char* q2 = q1 + 1;
        char* q3 = strchr(q2, '#');
        *q3 = '\0';
        numMaxAnt = safe_atoi(q2);
        *q3 = '#';
        char* q4 = q3 + 1;
        char* slash3 = strchr(q4, '/');
        size_t bmpLen3 = (size_t)(slash3 - q4);
        if (bmpLen3 >= MAX_BUF) bmpLen3 = MAX_BUF - 1;
        strncpy(bitmapAnt, q4, bmpLen3);
        bitmapAnt[bmpLen3] = '\0';
        printf("DEBUG: (Anexar/Reuso) Lectura cabecera existente: numRegAnt=%d, numMaxAnt=%d, bitmapAnt='%s'\n",
               numRegAnt, numMaxAnt, bitmapAnt);

        // 6) Buscar slot libre en bitmapAnt[] para reuso
        int idxLibre = -1;
        for (int i = 0; i < numMaxAnt; i++) {
            if (bitmapAnt[i] == '0') {
                idxLibre = i;
                break;
            }
        }

        if (idxLibre >= 0) {
            // Reutilizar hueco: idxLibre
            printf("DEBUG: Se encontró espacio libre en Bloque%d en índice %d\n", nroBloque, idxLibre);
            // 7) Actualizar numRegAnt y bitmapAnt[idxLibre]
            numRegAnt++;
            bitmapAnt[idxLibre] = '1';
            printf("DEBUG: (Reuso) Nuevo numRegAnt = %d, bitmapAnt = '%s'\n", numRegAnt, bitmapAnt);

            // 8) Reconstruir cabecera EXACTA: poner espacios hasta raw_header_len-1, luego '/'
            rewind(fbloc);
            char newHeader2[MAX_BUF];
            int ofh2 = 0;
            ofh2 += snprintf(newHeader2 + ofh2, MAX_BUF - ofh2, "%d#%d#", numRegAnt, numMaxAnt);
            for (int i = 0; i < numMaxAnt && ofh2 < (int)(raw_header_len - 1); i++) {
                newHeader2[ofh2++] = bitmapAnt[i];
            }
            // Llenar con espacios hasta la posición raw_header_len-1
            while (ofh2 < (int)(raw_header_len - 1)) {
                newHeader2[ofh2++] = ' ';
            }
            // Poner '/' en la última posición de cabecera
            newHeader2[raw_header_len - 1] = '/';
            // No más caracteres
            newHeader2[raw_header_len] = '\0';
            printf("DEBUG: (Reuso) Reconstruyendo cabecera: '%.*s'\n", (int)raw_header_len, newHeader2);

            // 9) Sobrescribir cabecera en BloqueN.txt
            fwrite(newHeader2, 1, raw_header_len, fbloc);
            fflush(fbloc);

            // 10) Calcular offset para insertar el registro reusado
            long offsetRegistroReuse = (long)raw_header_len + (long)idxLibre * (tamRegistro + 1);
            printf("DEBUG: offsetRegistroReuse en Bloque%d = %ld\n", nroBloque, offsetRegistroReuse);

            // 11) Reemplazar '@' con regBuf en esa posición
            if (fseek(fbloc, offsetRegistroReuse, SEEK_SET) != 0) {
                fprintf(stderr, "ERROR: fseek falló al offsetRegistroReuse\n");
                fclose(fbloc);
                return false;
            }
            size_t escritosReuse = fwrite(regBuf, 1, regLen, fbloc);
            fputc('|', fbloc);
            fflush(fbloc);
            if (escritosReuse != (size_t)regLen) {
                printf("ERROR: Se escribieron %zu bytes de registro variable reusado, esperados %d\n", escritosReuse, regLen);
            }
            else {
                printf("DEBUG: Registro variable de %d bytes reusado en posición %d (+ '|').\n", regLen, idxLibre);
            }

            fclose(fbloc);
            // 12) Volcar bloque reasignado a sectores
            printf("DEBUG: Llamando a volcarBloqueASectores para bloque #%d (reuso)\n", nroBloque);
            disco.volcarBloqueASectores(nroBloque);

            printf("DEBUG: adicionarRegistroUnico (reuso) finalizado para Bloque #%d\n", nroBloque);
            return true;
        }

        // 13) No hay hueco: anexar al final
        printf("DEBUG: Bloque #%d sin espacio libre, se agregará al final.\n", nroBloque);
        // Calcular offsetAppend = raw_header_len + numRegAnt_old * (tamRegistro+1)
        long offsetAppend = (long)raw_header_len + (long)(numRegAnt) * (tamRegistro + 1);
        printf("DEBUG: offsetAppend en Bloque%d = %ld\n", nroBloque, offsetAppend);

        // Actualizar cabecera (numRegAnt+1, marcar nuevo bitmap)
        numRegAnt++;
        if (numRegAnt <= numMaxAnt) {
            bitmapAnt[numRegAnt - 1] = '1';
        }
        printf("DEBUG: (Anexar) Nuevo numRegAnt = %d, bitmapAnt = '%s'\n",
               numRegAnt, bitmapAnt);

        // Reconstruir cabecera EXACTA: espacios hasta raw_header_len-1, luego '/'
        rewind(fbloc);
        char newHeader3[MAX_BUF];
        int ofh3 = 0;
        ofh3 += snprintf(newHeader3 + ofh3, MAX_BUF - ofh3, "%d#%d#", numRegAnt, numMaxAnt);
        for (int i = 0; i < numMaxAnt && ofh3 < (int)(raw_header_len - 1); i++) {
            newHeader3[ofh3++] = bitmapAnt[i];
        }
        while (ofh3 < (int)(raw_header_len - 1)) {
            newHeader3[ofh3++] = ' ';
        }
        newHeader3[raw_header_len - 1] = '/';
        newHeader3[raw_header_len] = '\0';
        printf("DEBUG: (Anexar) Reconstruyendo cabecera: '%.*s'\n", (int)raw_header_len, newHeader3);

        fwrite(newHeader3, 1, raw_header_len, fbloc);
        fflush(fbloc);

        // Insertar regBuf + '|' en offsetAppend
        if (fseek(fbloc, offsetAppend, SEEK_SET) != 0) {
            fprintf(stderr, "ERROR: fseek falló al offsetAppend\n");
            fclose(fbloc);
            return false;
        }
        size_t escritosAppend = fwrite(regBuf, 1, regLen, fbloc);
        fputc('|', fbloc);
        fflush(fbloc);
        if (escritosAppend != (size_t)regLen) {
            printf("ERROR: Se escribieron %zu bytes de registro variable anexado, esperados %d\n", escritosAppend, regLen);
        }
        else {
            printf("DEBUG: Registro variable de %d bytes anexado correctamente (+ '|').\n", regLen);
        }

        fclose(fbloc);
        // 14) Volcar bloque anexado a sectores
        printf("DEBUG: Llamando a volcarBloqueASectores para bloque #%d (anexar)\n", nroBloque);
        disco.volcarBloqueASectores(nroBloque);

        printf("DEBUG: adicionarRegistroUnico (anexar) finalizado para Bloque #%d\n", nroBloque);
        return true;
    }
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