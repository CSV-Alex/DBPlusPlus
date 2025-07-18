#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include <unordered_set>
#include <algorithm>
#include <string>
using namespace std;

const int BUCKET_SIZE = 2;  // Capacidad máxima por bucket

// --------------------------
// Estructura de Bucket
// --------------------------
struct Bucket {
    int localDepth;
    string idBinario;
    vector<int> valores;

    Bucket(int depth, const string& id)
        : localDepth(depth), idBinario(id) {}

    bool isFull() const {
        return valores.size() >= BUCKET_SIZE;
    }

    bool contains(int val) const {
        return find(valores.begin(), valores.end(), val) != valores.end();
    }
};

// --------------------------
// Función para obtener bits
// --------------------------
string obtenerBits(int valor, int bitsNecesarios) {
    string resultado;
    for (int i = bitsNecesarios - 1; i >= 0; --i) {
        resultado += ((valor >> i) & 1) ? '1' : '0';
    }
    return resultado;
}

// --------------------------
// Clase de Hash Extensible
// --------------------------
class TablaHashExtensible {
private:
    int globalDepth;
    bool aceptaRepetidos;
    vector<shared_ptr<Bucket>> directorio;

    // Calcula el hash binario con los globalDepth bits más significativos
    string hashBinario(int valor) {
        return obtenerBits(valor, globalDepth);
    }

    // Duplica el directorio (cuando localDepth == globalDepth)
    void doblarDirectorio() {
        int tam = directorio.size();
        for (int i = 0; i < tam; ++i)
            directorio.push_back(directorio[i]);
        globalDepth++;
    }

    // Divide un bucket que está lleno en dos
    void dividirBucket(int index) {
        auto bucketAntiguo = directorio[index];
        // Guardamos el prefijo antiguo antes de incrementar localDepth
        string oldPrefix = bucketAntiguo->idBinario;
        int nuevoLocalDepth = ++bucketAntiguo->localDepth;

        // Si ahora localDepth supera al global, primero duplicamos
        if (nuevoLocalDepth > globalDepth)
            doblarDirectorio();

        auto bucketNuevo = make_shared<Bucket>(nuevoLocalDepth, "");
        int N = directorio.size();

        // Reasignamos todos los punteros del directorio que coincidían
        for (int i = 0; i < N; ++i) {
            string bin = obtenerBits(i, globalDepth);
            // Coincide con el prefijo antiguo de tamaño (nuevoLocalDepth-1)
            if (bin.substr(0, nuevoLocalDepth - 1) == oldPrefix) {
                if (bin[nuevoLocalDepth - 1] == '1') {
                    directorio[i] = bucketNuevo;
                    bucketNuevo->idBinario = bin.substr(0, nuevoLocalDepth);
                } else {
                    directorio[i] = bucketAntiguo;
                    bucketAntiguo->idBinario = bin.substr(0, nuevoLocalDepth);
                }
            }
        }

        // Reinsertamos manualmente los valores del bucket antiguo
        vector<int> temp = bucketAntiguo->valores;
        bucketAntiguo->valores.clear();
        for (int v : temp) {
            string h = hashBinario(v);
            int idx = stoi(h, nullptr, 2);
            directorio[idx]->valores.push_back(v);
        }
    }

public:
    // Constructor: depth inicial 1, y flag de repetidos
    TablaHashExtensible(bool permiteRepetidos = false)
        : globalDepth(1), aceptaRepetidos(permiteRepetidos) {
        auto b = make_shared<Bucket>(1, "0");
        directorio.push_back(b);
        directorio.push_back(b);
    }

    // Inserta valor entero de forma iterativa para evitar recursión infinita
    void add(int valor) {
        while (true) {
            string h = hashBinario(valor);
            int idx = stoi(h, nullptr, 2);
            auto& bucket = directorio[idx];

            if (!aceptaRepetidos && bucket->contains(valor)) {
                cout << "Valor " << valor << " ya existe y no se permiten duplicados.\n";
                return;
            }
            if (!bucket->isFull()) {
                bucket->valores.push_back(valor);
                return;
            }
            // Bucket lleno → si es necesario, dobla, luego divide
            if (bucket->localDepth == globalDepth)
                doblarDirectorio();
            dividirBucket(idx);
            // el bucle vuelve a calcular hashBinario(valor) para reintentar
        }
    }

    // Muestra estado de la tabla (globalDepth, cada bucket único)
    void mostrarTabla() {
        cout << "GlobalDepth: " << globalDepth << "\n";
        unordered_set<Bucket*> seen;
        for (int i = 0; i < directorio.size(); ++i) {
            Bucket* b = directorio[i].get();
            if (seen.insert(b).second) {
                string bin = obtenerBits(i, globalDepth);
                cout << "[" << bin << "] ID=" << b->idBinario
                     << " LD=" << b->localDepth << " : ";
                for (int v : b->valores)
                    cout << v << " ";
                cout << "\n";
            }
        }
    }
};