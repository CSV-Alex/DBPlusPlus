#include <iostream>
#include <vector>
#include <unordered_set>
using namespace std;

const int BUCKET_SIZE = 2;

struct Bucket {
    int localDepth;
    vector<int> vals;
    Bucket(int ld) : localDepth(ld) {}
};

class ExtHash {
private:
    int globalDepth;
    vector<Bucket*> dir;

    int hashVal(int v) const {
        // Usamos los globalDepth bits menos significativos
        return v & ((1 << globalDepth) - 1);
    }

    void doubleDir() {
        int oldSize = dir.size();
        for (int i = 0; i < oldSize; ++i)
            dir.push_back(dir[i]);
        ++globalDepth;
    }

    void splitBucket(int idx) {
        Bucket* b = dir[idx];
        int newLD = ++b->localDepth;
        if (newLD > globalDepth)
            doubleDir();

        // Nuevo bucket con misma localDepth
        Bucket* nb = new Bucket(b->localDepth);
        int N = dir.size();
        // Reasignar punteros que antes apuntaban a b
        for (int i = 0; i < N; ++i) {
            if (dir[i] == b) {
                // Si el bit (newLD-1) de i es 1, va al bucket nuevo
                if ((i >> (newLD - 1)) & 1)
                    dir[i] = nb;
            }
        }
        // Reinsertar valores de b en sus nuevos buckets
        vector<int> tmp = b->vals;
        b->vals.clear();
        for (int v : tmp) {
            int h = hashVal(v);
            dir[h]->vals.push_back(v);
        }
    }

public:
    ExtHash() : globalDepth(1) {
        Bucket* b0 = new Bucket(1);
        dir.push_back(b0);
        dir.push_back(b0);
    }

    void add(int v) {
        while (true) {
            int h = hashVal(v);
            Bucket* b = dir[h];
            if (b->vals.size() < BUCKET_SIZE) {
                b->vals.push_back(v);
                return;
            }
            // Si está lleno, lo dividimos
            splitBucket(h);
        }
    }

    void print() const {
        cout << "GlobalDepth: " << globalDepth << "\n";
        unordered_set<Bucket*> seen;
        int N = dir.size();
        for (int i = 0; i < N; ++i) {
            if (seen.insert(dir[i]).second) {
                cout << "[";
                // Muestro el índice en binario usando globalDepth bits
                for (int j = globalDepth - 1; j >= 0; --j)
                    cout << ((i >> j) & 1);
                cout << "] LD=" << dir[i]->localDepth << ": ";
                for (int x : dir[i]->vals) cout << x << " ";
                cout << "\n";
            }
        }
    }
};

int main() {
    ExtHash h;
    vector<int> data = {5, 9, 13, 1, 17, 21, 25, 29, 33};
    for (int v : data){
         h.add(v);
    }
    h.print();
    return 0;
}
