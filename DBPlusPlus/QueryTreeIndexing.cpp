
#include "QueryTreeIndexing.h"
#include <algorithm>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <fstream>
#include <filesystem>
#include "QueryBlocks.h"   // readBlockHeader, loadRelationHeader, getBlocksFromCatalog

BPlusTree::BPlusTree(int m) : m(m) {
    minKeys = (m + 1) / 2; // teoria B+Trees prueba
    root = new Node(true);
}

BPlusTree::~BPlusTree() {
    if (!root) return;
    std::vector<Node*> stk{ root };
    while (!stk.empty()) {
        Node* n = stk.back(); stk.pop_back();
        if (!n->isLeaf)
            for (auto c : n->children)
                stk.push_back(c);
        delete n;
    }
}

bool BPlusTree::search(int key) const {
    Node* leaf = findLeaf(key);
    auto it = std::lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
    return (it != leaf->keys.end() && *it == key);
}

bool BPlusTree::insertKey(int key, int blk) {
    Node* leaf = findLeaf(key);
    auto it = std::lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
    size_t pos = it - leaf->keys.begin();

    if (it != leaf->keys.end() && *it == key) {
        // La clave ya existe, agregamos el bloque si no está repetido
        auto& lista = leaf->blocks[pos];
        auto itBlk = std::lower_bound(lista.begin(), lista.end(), blk);
        if (itBlk == lista.end() || *itBlk != blk)
            lista.insert(itBlk, blk); // Inserta el bloque en orden
        return true;
    }
    else {
        // La clave no existe, la agregamos junto con el bloque
        leaf->keys.insert(it, key);
        leaf->blocks.insert(leaf->blocks.begin() + pos, std::vector<int>{blk});
        if (leaf->keys.size() > size_t(m))
            splitLeaf(leaf);
        return true;
    }
}

bool BPlusTree::removeKey(int key) {
    // 1) Bajamos hasta la hoja, guardando el path y los indices
    std::vector<Node*> path;
    std::vector<int>   idxs;
    Node* cur = root;
    path.push_back(cur);
    while (!cur->isLeaf) {
        int i = 0;
        while (i < (int)cur->keys.size() && key >= cur->keys[i]) ++i;
        idxs.push_back(i);
        cur = cur->children[i];
        path.push_back(cur);
    }
    Node* leaf = cur;

    auto it = std::lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
    if (it == leaf->keys.end() || *it != key)
        return false;

    // Era la primera clave de la hoja?
    bool wasFirst = (it == leaf->keys.begin());
    int  oldFirst = *it;

    size_t pos = it - leaf->keys.begin();

    // Remove key and corresponding block
    leaf->keys.erase(it);
    leaf->blocks.erase(leaf->blocks.begin() + pos);

    // 3) Rebalance si underflow
    if (leaf != root && leaf->keys.size() < size_t(minKeys)) {
        rebalance(leaf);
    }

    // 4) Colapsar raiz si quedo vacia
    if (!root->isLeaf && root->keys.empty()) {
        Node* only = root->children.front();
        only->parent = nullptr;
        delete root;
        root = only;
    }

    // 5) Actualizar separadores a lo largo del path si la primera clave cambio
    if (wasFirst && leaf != root) {
        int newFirst = leaf->keys.front();
        for (int level = (int)idxs.size() - 1; level >= 0; --level) {
            Node* parent = path[level];
            int   idx = idxs[level];
            if (idx > 0) {
                if (parent->keys[idx - 1] == oldFirst) {
                    parent->keys[idx - 1] = newFirst;
                    oldFirst = newFirst;
                }
                else {
                    break;
                }
            }
        }
    }
    return true;
}

bool BPlusTree::modifyKey(int oldKey, int newKey) {
    if (!removeKey(oldKey)) return false;
    return insertKey(newKey, 0);
}

BPlusTree::Node* BPlusTree::findLeaf(int key) const {
    Node* cur = root;
    while (!cur->isLeaf) {
        int i = 0;
        while (i < (int)cur->keys.size() && key >= cur->keys[i]) ++i;
        cur = cur->children[i];
    }
    return cur;
}

void BPlusTree::splitLeaf(Node* leaf) {
    Node* bro = new Node(true);
    bro->parent = leaf->parent;
    size_t total = leaf->keys.size();
    size_t mid = total / 2;

    bro->keys.assign(leaf->keys.begin() + mid, leaf->keys.end());
    leaf->keys.resize(mid);

    // Split blocks
    bro->blocks.assign(leaf->blocks.begin() + mid, leaf->blocks.end());
    leaf->blocks.resize(mid);

    bro->next = leaf->next;
    leaf->next = bro;

    int up = bro->keys.front();
    if (!leaf->parent) {
        root = new Node(false);
        root->keys = { up };
        root->children = { leaf, bro };
        leaf->parent = bro->parent = root;
    }
    else {
        Node* p = leaf->parent;
        auto it = std::upper_bound(p->keys.begin(), p->keys.end(), up);
        int pos = it - p->keys.begin();
        p->keys.insert(it, up);
        p->children.insert(p->children.begin() + pos + 1, bro);
        bro->parent = p;
        if (p->keys.size() > size_t(m))
            splitInternal(p);
    }
}

void BPlusTree::splitInternal(Node* node) {
    Node* bro = new Node(false);
    bro->parent = node->parent;
    size_t total = node->keys.size();
    size_t mid = total / 2;
    int up = node->keys[mid];

    bro->keys.assign(node->keys.begin() + mid + 1, node->keys.end());
    bro->children.assign(node->children.begin() + mid + 1, node->children.end());
    for (auto c : bro->children) c->parent = bro;

    node->keys.resize(mid);
    node->children.resize(mid + 1);

    if (!node->parent) {
        root = new Node(false);
        root->keys = { up };
        root->children = { node, bro };
        node->parent = bro->parent = root;
    }
    else {
        Node* p = node->parent;
        auto it = std::upper_bound(p->keys.begin(), p->keys.end(), up);
        int pos = it - p->keys.begin();
        p->keys.insert(it, up);
        p->children.insert(p->children.begin() + pos + 1, bro);
        bro->parent = p;
        if (p->keys.size() > size_t(m))
            splitInternal(p);
    }
}

void BPlusTree::updateSeparators(Node* parent) {
    for (size_t i = 0; i + 1 < parent->children.size(); ++i) {
        parent->keys[i] = parent->children[i + 1]->keys.front();
    }
}

void BPlusTree::rebalance(Node* node) {
    Node* parent = node->parent;
    // 1) indice de node en el padre
    auto it = std::find(parent->children.begin(),
        parent->children.end(),
        node);
    int idx = int(it - parent->children.begin());

    // 2) determina hermanos
    Node* left = (idx > 0
        ? parent->children[idx - 1]
        : nullptr);
    Node* right = (idx + 1 < (int)parent->children.size()
        ? parent->children[idx + 1]
        : nullptr);

    auto recalcKeys = [&](Node* p) {
        p->keys.resize(p->children.size() - 1);
        for (size_t i = 0; i + 1 < p->children.size(); ++i) {
            p->keys[i] = p->children[i + 1]->keys.front();
        }
        };

    // CASO A: nodo hoja
    if (node->isLeaf) {
        // A.1) hoja quedo vacia -> fusion forzada
        if (node->keys.empty()) {
            if (left) {
                // fusiona node en left 
                left->keys.insert(
                    left->keys.end(),
                    node->keys.begin(),
                    node->keys.end()
                );
                // Also merge blocks
                left->blocks.insert(
                    left->blocks.end(),
                    node->blocks.begin(),
                    node->blocks.end()
                );
                left->next = node->next;
                parent->children.erase(parent->children.begin() + idx);
                parent->keys.erase(parent->keys.begin() + (idx - 1));
                delete node;
            }
            else {
                // solo hay hermano derecho
                parent->children.erase(parent->children.begin() + idx);
                parent->keys.erase(parent->keys.begin() + idx);
                delete node;
            }
            recalcKeys(parent);
            // caer al final para colapsar raiz o propagar hacia arriba
        }
        // A.2) underflow “normal” (pero no vacia)
        else if (node->keys.size() < size_t(minKeys)) {
            // prestamo desde LEFT
            if (left && left->keys.size() > size_t(minKeys)) {
                node->keys.insert(node->keys.begin(),
                    left->keys.back());
                left->keys.pop_back();
                recalcKeys(parent);
                return;
            }
            // prestamo desde RIGHT
            if (right && right->keys.size() > size_t(minKeys)) {
                node->keys.push_back(right->keys.front());
                right->keys.erase(right->keys.begin());
                recalcKeys(parent);
                return;
            }
            // fusion con left o right
            if (left) {
                left->keys.insert(
                    left->keys.end(),
                    node->keys.begin(),
                    node->keys.end()
                );
                left->blocks.insert(
                    left->blocks.end(),
                    node->blocks.begin(),
                    node->blocks.end()
                );
                left->next = node->next;
                parent->children.erase(parent->children.begin() + idx);
                parent->keys.erase(parent->keys.begin() + (idx - 1));
                delete node;
            }
            else {
                right->keys.insert(
                    right->keys.begin(),
                    node->keys.begin(),
                    node->keys.end()
                );
                right->blocks.insert(
                    right->blocks.end(),
                    node->blocks.begin(),
                    node->blocks.end()
                );
                parent->children.erase(parent->children.begin() + idx);
                parent->keys.erase(parent->keys.begin() + idx);
                delete node;
            }
            recalcKeys(parent);
            // caer al final para colapsar raiz o propagar hacia arriba
        }
        else {
            // sin underflow
            return;
        }
    }

    // CASO B: nodo interno
    else {
        if (node->keys.size() < size_t(minKeys)) {
            // prestamo desde left
            if (left && left->keys.size() > size_t(minKeys)) {
                node->keys.insert(node->keys.begin(),
                    parent->keys[idx - 1]);
                parent->keys[idx - 1] = left->keys.back();
                left->keys.pop_back();
                node->children.insert(node->children.begin(),
                    left->children.back());
                node->children.front()->parent = node;
                left->children.pop_back();
                recalcKeys(parent);
                return;
            }
            // prestamo desde right
            if (right && right->keys.size() > size_t(minKeys)) {
                node->keys.push_back(parent->keys[idx]);
                parent->keys[idx] = right->keys.front();
                right->keys.erase(right->keys.begin());
                node->children.push_back(right->children.front());
                node->children.back()->parent = node;
                right->children.erase(right->children.begin());
                recalcKeys(parent);
                return;
            }
            // fusion con left o right
            if (left) {
                int sep = parent->keys[idx - 1];
                left->keys.push_back(sep);
                left->keys.insert(
                    left->keys.end(),
                    node->keys.begin(),
                    node->keys.end()
                );
                left->blocks.insert(
                    left->blocks.end(),
                    node->blocks.begin(),
                    node->blocks.end()
                );
                left->children.insert(
                    left->children.end(),
                    node->children.begin(),
                    node->children.end()
                );
                for (Node* c : node->children)
                    c->parent = left;
                parent->children.erase(parent->children.begin() + idx);
                parent->keys.erase(parent->keys.begin() + (idx - 1));
                delete node;
            }
            else {
                int sep = parent->keys[idx];
                node->keys.push_back(sep);
                node->keys.insert(
                    node->keys.end(),
                    right->keys.begin(),
                    right->keys.end()
                );
                right->blocks.insert(
                    right->blocks.end(),
                    node->blocks.begin(),
                    node->blocks.end()
                );
                node->children.insert(
                    node->children.end(),
                    right->children.begin(),
                    right->children.end()
                );
                for (Node* c : right->children)
                    c->parent = node;
                parent->children.erase(parent->children.begin() + (idx + 1));
                parent->keys.erase(parent->keys.begin() + idx);
                delete right;
            }
            recalcKeys(parent);
            // caer al final para colapsar raiz o propagar hacia arriba
        }
        else {
            // sin underflow
            return;
        }
    }

    // 3) colapsar raiz o propagar
    if (parent == root) {
        if (root->keys.empty()) {
            Node* only = root->children.front();
            only->parent = nullptr;
            delete root;
            root = only;
            if (!root->isLeaf)
                recalcKeys(root);
        }
    }
    else if (parent->keys.size() < size_t(minKeys)) {
        rebalance(parent);
    }
}

BPlusTree::Node* BPlusTree::getSibling(Node* node, int& idx, bool left) const {
    Node* p = node->parent;
    auto it = std::find(p->children.begin(), p->children.end(), node);
    int i = it - p->children.begin();
    idx = left ? (i - 1) : i;
    if (left) return i > 0 ? p->children[i - 1] : nullptr;
    return i + 1 < (int)p->children.size() ? p->children[i + 1] : nullptr;
}


// ---------------- Constructor & buildIndex ----------------

BPlusTreeIndex::BPlusTreeIndex(const std::string& catalogPath,
    const std::string& blocksDir,
    const std::string& tableTxt,
    const std::string& relName,
    const std::string& indexField,
    int degree)
    : BPlusTree(degree),
    _catalogPath(catalogPath),
    _blocksDir(blocksDir),
    _tableTxt(tableTxt),
    _relName(relName),
    _indexField(indexField),
    _fieldIndex(-1)
{
    // 1) Creamos la carpeta de bloques si no existe
    std::filesystem::create_directories(_blocksDir);

    // 2) Construimos el índice leyendo todos los bloques
    buildIndex();
}

// Lee cada bloque y cada registro, llama insertEntry(key, blk)
void BPlusTreeIndex::buildIndex() {
    // 1) Obtener lista de bloques para la relación
    auto blocks = getBlocksFromCatalog(_catalogPath, _relName);

    // 2) Cargar header para hallar la posición de indexField
    auto hdr = loadRelationHeader(_tableTxt);
    _fieldIndex = -1;
    for (int i = 0; i < (int)hdr.size(); ++i) {
        if (hdr[i] == _indexField) {
            _fieldIndex = i;
            break;
        }
    }
    if (_fieldIndex < 0)
        throw std::runtime_error("No se encontro campo índice: " + _indexField);

    // 3) Recorremos cada bloque
    for (int blk : blocks) {
        std::string path = getBlockPath(blk);
        int numRec; std::streampos off;
        if (!readBlockHeader(path, numRec, off)) continue;

        // Leemos datos del bloque
        std::ifstream in(path, std::ios::binary);
        in.seekg(off);
        std::string data((std::istreambuf_iterator<char>(in)), {});
        in.close();

        // Separamos registros por '|'
        std::stringstream ss(data);
        std::string rec;
        while (std::getline(ss, rec, '|')) {
            if (rec.empty()) continue;
            // Limpieza y split por '#'
            std::string clean;
            for (char c : rec) if (c != '@') clean.push_back(c);
            auto fields = split(clean, '#');
            if (_fieldIndex < (int)fields.size()) {
                // Convertir a entero y luego insertar
                try {
                    int keyValue = std::stoi(fields[_fieldIndex]);
                    insertKey(keyValue, blk);
                }
                catch (const std::exception& e) {
                    // Si no se puede convertir a entero, seguimos
                    std::cerr << "Error al convertir clave: " << e.what() << std::endl;
                }
            }
        }
    }
}

// Construye ruta del bloque
std::string BPlusTreeIndex::getBlockPath(int blk) const {
    return _blocksDir + "Bloque" + std::to_string(blk) + ".txt";
}

std::vector<std::string> BPlusTreeIndex::split(const std::string& s, char delim) {
    std::vector<std::string> elems;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) elems.push_back(item);
    return elems;
}

void BPlusTree::dumpToTxt(const std::string& filename) const {

    std::string dir = "data\\usr\\db";
    std::filesystem::create_directories(dir);
    std::string fullpath = dir + "\\" + filename;
    std::ofstream out(fullpath);

    out << "ORDER=" << m << "\n";
    out << "ROOT=0\n\n";

    // Asignar IDs a cada nodo (BFS)
    std::unordered_map<const Node*, int> idMap;
    std::vector<const Node*> nodes;
    std::queue<const Node*> q;
    q.push(root);
    idMap[root] = 0;
    nodes.push_back(root);
    int nextId = 1;
    while (!q.empty()) {
        const Node* n = q.front(); q.pop();
        if (!n->isLeaf) {
            for (const Node* c : n->children) {
                if (!idMap.count(c)) {
                    idMap[c] = nextId++;
                    nodes.push_back(c);
                    q.push(c);
                }
            }
        }
    }

    // volcar cada página en orden de PAGE_ID
    for (int pid = 0; pid < (int)nodes.size(); ++pid) {
        const Node* n = nodes[pid];
        out << "PAGE_ID=" << pid << "\n";
        out << "IS_LEAF=" << (n->isLeaf ? 1 : 0) << "\n";
        out << "NUM_KEYS=" << n->keys.size() << "\n";

        // KEYS
        out << "KEYS=";
        for (size_t i = 0; i < n->keys.size(); ++i) {
            out << n->keys[i] << (i + 1 < n->keys.size() ? "," : "");
        }
        out << "\n";

        // PTRS
        out << "PTRS=";
        if (n->isLeaf) {
            // imprime bloques de datos asociados a cada clave
            for (size_t i = 0; i < n->blocks.size(); ++i) {
                for (size_t j = 0; j < n->blocks[i].size(); ++j) {
                    out << n->blocks[i][j] << (j + 1 < n->blocks[i].size() ? "," : "");
                }
                if (i + 1 < n->blocks.size()) out << " ";
            }
        }
        else {
            // imprime IDs de páginas hijas
            for (size_t i = 0; i < n->children.size(); ++i) {
                out << idMap.at(n->children[i])
                    << (i + 1 < n->children.size() ? "," : "");
            }
        }
        out << "\n";

        // NEXT_LEAF
        out << "NEXT_LEAF=";
        if (n->isLeaf && n->next)
            out << idMap.at(n->next);
        else
            out << -1;
        out << "\n\n";
    }

    out.close();
}

void BPlusTree::printTree() const {
    std::queue<Node*> q;
    q.push(root);
    int lvl = 0;
    while (!q.empty()) {
        std::cout << "Nivel " << lvl++ << ": ";
        int sz = q.size();
        for (int i = 0; i < sz; ++i) {
            Node* n = q.front(); q.pop();
            std::cout << "[";
            for (int k : n->keys) std::cout << k << " ";
            std::cout << "] ";
            if (!n->isLeaf)
                for (auto c : n->children)
                    q.push(c);
        }
        std::cout << "\n";
    }
}

void BPlusTree::exportDot(const std::string& filename) const {
    std::ofstream out(filename);
    out << "digraph BPlusTree {\n";
    out << "  node [shape=record];\n";
    std::queue<Node*> q;
    std::unordered_set<Node*> seen;
    q.push(root);
    seen.insert(root);
    while (!q.empty()) {
        Node* n = q.front(); q.pop();
        std::string id = "node" + std::to_string(reinterpret_cast<uintptr_t>(n));
        out << "  " << id << " [label=\"";
        for (size_t i = 0; i < n->keys.size(); ++i) {
            out << n->keys[i];
            if (i + 1 < n->keys.size()) out << "|";
        }
        out << "\"];\n";
        if (!n->isLeaf) {
            for (Node* c : n->children) {
                std::string cid = "node" + std::to_string(reinterpret_cast<uintptr_t>(c));
                out << "  " << id << " -> " << cid << ";\n";
                if (seen.insert(c).second) q.push(c);
            }
        }
    }
    out << "}\n";
    out.close();
}
