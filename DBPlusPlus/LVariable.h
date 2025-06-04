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

int get_nth_attribute(const char relacion[], const char atributo[]) {
    std::ifstream ifs("esquema.txt");
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
                return n_atributo;
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


std::string read_variable_register(const char registro_variable[]) {
    // 1) Recorrer la cabecera hasta terminar de leer todos los pares offset|size|
    std::vector<int> offsets;
    std::vector<int> sizes;

    const char* p = registro_variable;
    // a) primero, sabemos que la cabecera inicia inmediatamente:
    //    offset1|size1|offset2|size2|...|
    //    Repetimos hasta que ya no queden más pares (detectamos el final de cabecera cuando
    //    encontramos un carácter que no sea dígito en la siguiente posición de offset).
    bool seguir = true;
    while (seguir) {
        // Leer offset
        const char* delim1 = std::strchr(p, '|');
        if (!delim1) break;
        int off_i = std::atoi(std::string(p, delim1 - p).c_str());

        // Leer size
        p = delim1 + 1;
        const char* delim2 = std::strchr(p, '|');
        if (!delim2) break;
        int sz_i = std::atoi(std::string(p, delim2 - p).c_str());

        offsets.push_back(off_i);
        sizes.push_back(sz_i);

        p = delim2 + 1;
        // Si de aquí en adelante no vemos un dígito, asumimos fin de cabecera
        if (!std::isdigit(*p)) {
            seguir = false;
        }
    }

    if (offsets.empty()) {
        return ""; // sin campos
    }

    // 2) Determinar dónde inicia el área de datos: es el menor offset de todos
    int header_end = *std::min_element(offsets.begin(), offsets.end());
    // 3) Extraer cada campo usando offset y size
    std::vector<std::string> campos;
    for (size_t i = 0; i < offsets.size(); ++i) {
        int off_i = offsets[i];
        int sz_i = sizes[i];
        std::string campo;
        campo.assign(registro_variable + off_i, sz_i);
        campos.push_back(campo);
    }

    // 4) Construir string con separador "#"
    std::string resultado;
    for (size_t i = 0; i < campos.size(); ++i) {
        resultado += campos[i];
        if (i + 1 < campos.size()) resultado += '#';
    }
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


bool format_bloque_variable(const char registro_variable[],int& espacio_libre_bloque,char* bloque_variable, Disco& disco) {
    int bloqueSize = disco.getTamBloque();

    // 1) Leer cabecera existente “registro_n|last_pos|offset1|size1|…|offsetN|sizeN|”
    int registro_n = 0;
    int last_pos = 0;
    int old_meta_len = 0;

    // Vectores locales donde guardaremos offsets y tamaños de registros existentes
    std::vector<int> offsets;
    std::vector<int> sizes;

    if (bloque_variable[0] >= '0' && bloque_variable[0] <= '9') {
        // a) extraer registro_n
        const char* p = std::strchr(bloque_variable, '|');
        if (!p) {
            // Bloque corrupto: tratamos como vacío
            registro_n = 0;
            last_pos = bloqueSize - 1;
            old_meta_len = 0;
        } else {
            registro_n = std::stoi(std::string(bloque_variable, p - bloque_variable));
            // b) extraer last_pos
            const char* q = std::strchr(p + 1, '|');
            if (!q) {
                // Bloque corrupto: tratamos como vacío
                registro_n = 0;
                last_pos = bloqueSize - 1;
                old_meta_len = 0;
            } else {
                last_pos = std::stoi(std::string(p + 1, q - (p + 1)));
                // c) leer los pares (offset, size)
                const char* r = q + 1;
                old_meta_len = (int)(q - bloque_variable) + 1;
                for (int i = 0; i < registro_n; ++i) {
                    const char* pr = std::strchr(r, '|');
                    if (!pr) {
                        // Cabecera corrupta, tratamos como vacío
                        registro_n = 0;
                        last_pos = bloqueSize - 1;
                        offsets.clear();
                        sizes.clear();
                        old_meta_len = 0;
                        break;
                    }
                    int off_i = std::stoi(std::string(r, pr - r));

                    const char* ps = std::strchr(pr + 1, '|');
                    if (!ps) {
                        registro_n = 0;
                        last_pos = bloqueSize - 1;
                        offsets.clear();
                        sizes.clear();
                        old_meta_len = 0;
                        break;
                    }
                    int sz_i = std::stoi(std::string(pr + 1, ps - (pr + 1)));

                    offsets.push_back(off_i);
                    sizes.push_back(sz_i);

                    r = ps + 1;
                    old_meta_len = (int)(ps - bloque_variable) + 1;
                }
            }
        }
    } else {
        // Bloque vacío
        registro_n = 0;
        last_pos = bloqueSize - 1;
        old_meta_len = 0;
        offsets.clear();
        sizes.clear();
    }

    // 2) Longitud en bytes del registro nuevo
    int registro_len = (int)std::strlen(registro_variable);

    // 3) Calcular write_pos provisional
    int write_pos = last_pos - registro_len + 1;
    if (write_pos < 0) {
        return false; // ni caben los datos
    }

    // 4) Preparamos nuevos vectores con todos los registros + el nuevo
    int offset_nuevo = write_pos;
    int size_nuevo   = registro_len;
    int nuevo_registro_n = registro_n + 1;

    std::vector<int> new_offsets = offsets;
    std::vector<int> new_sizes   = sizes;
    new_offsets.push_back(offset_nuevo);
    new_sizes.push_back(size_nuevo);

    // 5) Reconstruir cabecera completa en texto:
    //    "nuevo_registro_n|offset_nuevo|offset1|size1|…|offset_nuevo|size_nuevo|"
    std::string new_meta;
    new_meta.reserve( 20 + nuevo_registro_n * 20 );
    new_meta += std::to_string(nuevo_registro_n);
    new_meta += '|';
    new_meta += std::to_string(offset_nuevo);
    new_meta += '|';
    for (int i = 0; i < nuevo_registro_n; ++i) {
        new_meta += std::to_string(new_offsets[i]);
        new_meta += '|';
        new_meta += std::to_string(new_sizes[i]);
        new_meta += '|';
    }
    int new_meta_len = (int)new_meta.size();

    // 6) Verificar que la cabecera no invada los datos:
    if (new_meta_len > write_pos) {
        return false;
    }

    // 7) Verificar espacio global:
    int delta_meta = new_meta_len - old_meta_len; // crece o se reduce
    int total_used = registro_len + delta_meta;
    if (total_used > espacio_libre_bloque) {
        return false;
    }

    // 8) Copiar datos del registro en write_pos
    std::memcpy(bloque_variable + write_pos, registro_variable, registro_len);

    // 9) Sobrescribir cabecera con new_meta
    std::memmove(bloque_variable, new_meta.c_str(), new_meta_len);

    // 10) Limpiar hueco si la vieja cabecera era más larga
    if (old_meta_len > new_meta_len) {
        std::memset(bloque_variable + new_meta_len, 0, old_meta_len - new_meta_len);
    }

    // 11) Actualizar espacio libre
    espacio_libre_bloque -= total_used;

    return true;
}


bool eliminar_registro_variable(int bloqueN, int r_index) {
    
    //r_index=r_index-1;//set to natural 

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
    cout<<"DEBUG: A ";
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
    cout<<"DEBUG: B ";
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
        cout<<"DEBUG: C ";
        int off_i = std::stoi(fileData.substr(prev, p_offset_end - prev)); //falla aqui

        // 7.b) Encontrar el siguiente '|' tras p_offset_end+1 → fin de size_i
        size_t p_size_end = fileData.find('|', p_offset_end + 1);
        if (p_size_end == std::string::npos) return false;
        cout<<"DEBUG: D ";
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

bool modificar(int NBloque, int r_index, const char relacion[], const char atributo[], const char new_value[],Disco&disco) {
    int tamBloque = disco.getTamBloque();
    std::string filename = "Bloque" + std::to_string(NBloque) + ".txt";

    // 1) Antes de eliminar, extraer el registro original completo (offset|size|...|datos)
    //    Para ello, abrimos el bloque y leemos todos los bytes en un buffer
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

    // 1.a) Parsear cabecera para ubicar offset y size del registro r_index
    //     Cabecera = registro_n|last_pos|offset1|size1|offset2|size2|...|
    size_t p1 = fileData.find('|');
    if (p1 == std::string::npos) return false;
    int registro_n = std::stoi(fileData.substr(0, p1));
    if (r_index < 1 || r_index > registro_n) {
        std::cerr << "Error: r_index fuera de rango\n";
        return false;
    }
    size_t p2 = fileData.find('|', p1 + 1);
    if (p2 == std::string::npos) return false;
    // Saltamos registro_n y last_pos:
    size_t cursor = p2 + 1;
    std::vector<int> offsets;
    std::vector<int> sizes;
    for (int i = 0; i < registro_n; ++i) {
        // offset
        size_t poff_end = fileData.find('|', cursor);
        if (poff_end == std::string::npos) return false;
        int off_i = std::stoi(fileData.substr(cursor, poff_end - cursor));
        cursor = poff_end + 1;
        // size
        size_t psize_end = fileData.find('|', cursor);
        if (psize_end == std::string::npos) return false;
        int sz_i = std::stoi(fileData.substr(cursor, psize_end - cursor));
        cursor = psize_end + 1;
        offsets.push_back(off_i);
        sizes.push_back(sz_i);
    }
    size_t header_len = cursor; // byte donde empiezan los datos

    // 1.b) Obtener offset y size del registro a modificar (0-based)
    int idx0 = r_index - 1;
    int off_old = offsets[idx0];
    int size_old = sizes[idx0];

    // 1.c) Extraer bytes crudos del registro antiguo
    if (off_old + size_old > (int)fileData.size()) return false;
    std::string registro_raw = fileData.substr(off_old, size_old);

    // 2) Obtener contenido “campo1#campo2#...”
    std::string contenido = read_variable_register(registro_raw.c_str());
    if (contenido.empty()) {
        std::cerr << "Error: no se pudo leer registro viejo\n";
        return false;
    }

    // 3) Averiguar posición (número) del atributo: get_nth_attribute
    int n_atributo = get_nth_attribute(relacion, atributo);
    if (n_atributo < 1) {
        std::cerr << "Error: atributo no encontrado en esquema\n";
        return false;
    }

    // 4) Separar contenido en fields[], reemplazar campo n_atributo-1
    std::vector<std::string> fields;
    {
        std::stringstream ss(contenido);
        std::string seg;
        while (std::getline(ss, seg, '#')) {
            fields.push_back(seg);
        }
    }
    if ((int)fields.size() < n_atributo) {
        std::cerr << "Error: el registro no tiene suficientes campos\n";
        return false;
    }
    fields[n_atributo - 1] = new_value;

    // 5) Reconstruir string temp con separadores '#'
    std::string temp;
    for (size_t i = 0; i < fields.size(); ++i) {
        temp += fields[i];
        if (i + 1 < fields.size()) temp += '#';
    }

    // 6) Formatear el nuevo registro con format_registro_variable
    //    (devuelve un char* recién allocado)
    extern char* format_registro_variable(const char registro[], const char relacion[]);
    char* nuevo_registro_bin = format_registro_variable(temp.c_str(), relacion);
    if (!nuevo_registro_bin) {
        std::cerr << "Error: format_registro_variable falló\n";
        return false;
    }

    // 7) Eliminar el registro viejo del bloque
    if (!eliminar_registro_variable(NBloque, r_index)) {
        std::cerr << "Error: no se pudo eliminar registro viejo\n";
        delete[] nuevo_registro_bin;
        return false;
    }

    // 8) Leer nuevamente el bloque actualizado para cargarlo en memoria
    std::ifstream ifs2(filename);
    if (!ifs2.is_open()) {
        std::cerr << "Error: no se pudo reabrir " << filename << "\n";
        delete[] nuevo_registro_bin;
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

    // 9) Pasar fileData2 a buffer de tamaño tamBloque, rellenando con '\0'
    char* bloque_buffer = new char[tamBloque];
    std::memset(bloque_buffer, 0, tamBloque);
    int copy_len = min((int)fileData2.size(), tamBloque);
    std::memcpy(bloque_buffer, fileData2.data(), copy_len);

    // 10) Calcular espacio libre actual como tamBloque - fileData2.size()
    int espacio_libre_bloque = tamBloque - (int)fileData2.size();

    // 11) Insertar el nuevo registro en el bloque en memoria
    if (!format_bloque_variable(nuevo_registro_bin, espacio_libre_bloque, bloque_buffer, disco)) {
        std::cerr << "Error: no se pudo insertar el registro modificado\n";
        delete[] nuevo_registro_bin;
        delete[] bloque_buffer;
        return false;
    }

    delete[] nuevo_registro_bin;

    // 12) Volcar bloque_buffer de regreso a "BloqueN.txt" (modo texto)
    //     Primero, encontrar el último byte no nulo:
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