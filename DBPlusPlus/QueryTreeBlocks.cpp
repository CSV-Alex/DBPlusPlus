#include "QueryTreeBlocks.h"
#include <queue>
#include <unordered_set>

// ---------------- Constructor & buildIndex ----------------

BPlusTreeIndex::BPlusTreeIndex(const std::string& catalogPath,
    const std::string& blocksDir,
    const std::string& tableTxt,
    const std::string& relName,
    const std::string& indexField,
    size_t degree)
    : _degree(degree),
    _catalogPath(catalogPath),
    _blocksDir(blocksDir),
    _tableTxt(tableTxt),
    _relName(relName),
    _indexField(indexField),
    _root(nullptr)
{
    // 1) Creamos la carpeta de bloques si no existe
    std::filesystem::create_directories(_blocksDir);

    // 2) Inicializamos un solo nodo hoja vacío como root
    _root = new Node(true);

    // 3) Construimos el índice leyendo todos los bloques
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
        throw std::runtime_error("No se encontró campo índice: " + _indexField);

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
                insertEntry(fields[_fieldIndex], blk);
            }
        }
    }
}

// ---------------- Inserción en B+ Tree ----------------

// Inserta la pareja (key, blk) en la hoja adecuada
void BPlusTreeIndex::insertEntry(const std::string& key, int blk) {
    // 1) Buscamos la hoja donde debe ir
    Node* leaf = findLeaf(key);

    // 2) Insertamos o actualizamos:
    //    Si ya existe la key, añadimos blk si no estaba
    auto it = std::lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
    size_t pos = it - leaf->keys.begin();
    if (it != leaf->keys.end() && *it == key) {
        // key existente: añadimos blk si es nuevo
        auto& blks = leaf->ptrs[pos];
        if (std::find(blks.begin(), blks.end(), blk) == blks.end())
            blks.push_back(blk);
    }
    else {
        // nueva llave
        leaf->keys.insert(it, key);
        leaf->ptrs.insert(leaf->ptrs.begin() + pos, std::vector<int>{blk});
    }

    // 3) Si overflow, spliteamos
    if (leaf->keys.size() > 2 * _degree)
        splitLeaf(leaf);
}

// Encuentra la hoja correspondiente a key
BPlusTreeIndex::Node* BPlusTreeIndex::findLeaf(const std::string& key) const {
    Node* cur = _root;
    while (!cur->isLeaf) {
        // buscamos el primer child cuyo rango contiene key
        size_t i = 0;
        while (i < cur->keys.size() && key >= cur->keys[i]) ++i;
        cur = cur->children[i];
    }
    return cur;
}

// ---------------- Splits ----------------

// Divide un nodo hoja overflow
void BPlusTreeIndex::splitLeaf(Node* leaf) {
    // creamos nuevo hermano
    Node* bro = new Node(true);
    bro->parent = leaf->parent;

    // determinamos punto medio
    size_t mid = leaf->keys.size() / 2;

    // movemos keys+ptrs a bro
    bro->keys.assign(leaf->keys.begin() + mid, leaf->keys.end());
    bro->ptrs.assign(leaf->ptrs.begin() + mid, leaf->ptrs.end());

    leaf->keys.resize(mid);
    leaf->ptrs.resize(mid);

    // enlazamos hermanos
    bro->next = leaf->next;
    leaf->next = bro;

    // la key promovida es la primera de bro
    std::string upKey = bro->keys.front();

    // si leaf era root, creamos nuevo root
    if (!leaf->parent) {
        _root = new Node(false);
        _root->keys = { upKey };
        _root->children = { leaf, bro };
        leaf->parent = bro->parent = _root;
    }
    else {
        // insertamos upKey en el padre
        Node* parent = leaf->parent;
        auto it = std::upper_bound(parent->keys.begin(), parent->keys.end(), upKey);
        size_t pos = it - parent->keys.begin();
        parent->keys.insert(it, upKey);
        parent->children.insert(parent->children.begin() + pos + 1, bro);
        bro->parent = parent;
        // si overflow en interno
        if (parent->keys.size() > 2 * _degree)
            splitInternal(parent);
    }
}

// Divide un nodo interno overflow
void BPlusTreeIndex::splitInternal(Node* in) {
    // creamos nuevo hermano
    Node* bro = new Node(false);
    bro->parent = in->parent;

    // punto medio: la key mid se eleva al padre
    size_t mid = in->keys.size() / 2;
    std::string upKey = in->keys[mid];

    // movemos llaves y children tras mid a bro
    bro->keys.assign(in->keys.begin() + mid + 1, in->keys.end());
    bro->children.assign(in->children.begin() + mid + 1, in->children.end());

    // actualizamos parent en children de bro
    for (auto ch : bro->children) ch->parent = bro;

    // recortamos el nodo original
    in->keys.resize(mid);
    in->children.resize(mid + 1);

    // si in era root
    if (!in->parent) {
        _root = new Node(false);
        _root->keys = { upKey };
        _root->children = { in, bro };
        in->parent = bro->parent = _root;
    }
    else {
        // insertar upKey en el padre
        Node* parent = in->parent;
        auto it = std::upper_bound(parent->keys.begin(), parent->keys.end(), upKey);
        size_t pos = it - parent->keys.begin();
        parent->keys.insert(it, upKey);
        parent->children.insert(parent->children.begin() + pos + 1, bro);
        bro->parent = parent;
        if (parent->keys.size() > 2 * _degree)
            splitInternal(parent);
    }
}

// ---------------- Consulta ----------------

std::vector<Hit> BPlusTreeIndex::queryWithBlocks(const std::string& whereVal) {
    std::vector<Hit> result;
    Node* leaf = findLeaf(whereVal);

    // Recorremos llaves iguales
    for (size_t i = 0; i < leaf->keys.size(); ++i) {
        if (leaf->keys[i] != whereVal) continue;
        // por cada bloque apuntado
        for (int blk : leaf->ptrs[i]) {
            collectRecords(blk, whereVal, result);
        }
    }
    return result;
}

// Lee el bloque, filtra registros y añade a result
void BPlusTreeIndex::collectRecords(int blk,
    const std::string& whereVal,
    std::vector<Hit>& out) const
{
    std::string path = getBlockPath(blk);
    int numRec; std::streampos off;
    if (!readBlockHeader(path, numRec, off)) return;
    std::ifstream in(path, std::ios::binary);
    in.seekg(off);
    std::string data((std::istreambuf_iterator<char>(in)), {});
    in.close();

    std::stringstream ss(data);
    std::string rec;
    while (std::getline(ss, rec, '|')) {
        if (rec.empty()) continue;
        std::string clean;
        for (char c : rec) if (c != '@') clean.push_back(c);
        auto fields = split(clean, '#');
        if (_fieldIndex < (int)fields.size() && fields[_fieldIndex] == whereVal) {
            out.emplace_back(fields, path);
        }
    }
}

// ---------------- Impresión ----------------

void BPlusTreeIndex::printTree() const {
    std::cout << "=== B+ Tree (order=" << _degree << ") ===\n";
    printNode(_root, 0);
}

void BPlusTreeIndex::printNode(Node* node, int level) const {
    if (!node) return;
    std::cout << std::string(level * 2, ' ')
        << (node->isLeaf ? "Leaf: " : "Int : ");
    for (auto& k : node->keys) std::cout << k << " ";
    std::cout << "\n";
    if (!node->isLeaf) {
        for (auto ch : node->children)
            printNode(ch, level + 1);
    }
}
