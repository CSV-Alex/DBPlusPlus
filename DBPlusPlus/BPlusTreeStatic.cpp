#include "BPlusTreeStatic.h"
#include <algorithm>
#include <queue>
#include <unordered_set>
#include <fstream>

BPlusTree::BPlusTree(int m) : m(m) {
    minKeys = (m + 1) / 2; // teoria B+Trees
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

bool BPlusTree::insert(int key) {
    Node* leaf = findLeaf(key);
    auto it = std::lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
    if (it != leaf->keys.end() && *it == key) return false;
    leaf->keys.insert(it, key);
    if (leaf->keys.size() > size_t(m))
        splitLeaf(leaf);
    return true;
}

bool BPlusTree::remove(int key) {
    Node* leaf = findLeaf(key);
    auto it = std::lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
    if (it == leaf->keys.end() || *it != key) return false;
    leaf->keys.erase(it);
    if (leaf != root && leaf->keys.size() < size_t(minKeys))
        rebalance(leaf);
    if (!root->isLeaf && root->keys.empty()) {
        Node* child = root->children.front();
        child->parent = nullptr;
        delete root;
        root = child;
    }
    return true;
}

bool BPlusTree::modify(int oldKey, int newKey) {
    if (!remove(oldKey)) return false;
    return insert(newKey);
}

void BPlusTree::print() const {
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
    size_t mid = (total + 1) / 2;
    bro->keys.assign(leaf->keys.begin() + mid, leaf->keys.end());
    leaf->keys.resize(mid);

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

void BPlusTree::rebalance(Node* node) {
    int idx;
    Node* sib = getSibling(node, idx, true);
    if (sib && sib->keys.size() > size_t(minKeys)) {
        node->keys.insert(node->keys.begin(), sib->keys.back());
        sib->keys.pop_back();
        node->parent->keys[idx] = node->keys.front();
        return;
    }
    sib = getSibling(node, idx, false);
    if (sib && sib->keys.size() > size_t(minKeys)) {
        node->keys.push_back(sib->keys.front());
        sib->keys.erase(sib->keys.begin());
        node->parent->keys[idx] = sib->keys.front();
        return;
    }
    if ((sib = getSibling(node, idx, true))) {
        // Fusion con hermano izquierdo
        sib->keys.insert(sib->keys.end(), node->keys.begin(), node->keys.end());
        sib->next = node->next;
        Node* p = node->parent;
        p->keys.erase(p->keys.begin() + idx);
        p->children.erase(std::find(p->children.begin(), p->children.end(), node));
        delete node;
        if (p != root && p->keys.size() < size_t(minKeys))
            rebalance(p);
    }
    else {
        // Fusion con hermano derecho
        sib = getSibling(node, idx, false);
        node->keys.insert(node->keys.end(), sib->keys.begin(), sib->keys.end());
        node->next = sib->next;
        Node* p = node->parent;
        p->keys.erase(p->keys.begin() + idx);
        p->children.erase(std::find(p->children.begin(), p->children.end(), sib));
        delete sib;
        if (p != root && p->keys.size() < size_t(minKeys))
            rebalance(p);
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

int main() {
    int m;
    std::cout << "Numero maximo de llaves por nodo: ";
    std::cin >> m;
    BPlusTree tree(m);
    std::cout << "Comandos disponibles:\n"
        " insert <key>\n"
        " delete <key>\n"
        " modify <old> <new>\n"
        " search <key>\n"
        " print\n"
        " fin\n";
    while (true) {
        std::cout << "> ";
        std::string cmd;
        std::cin >> cmd;
        if (cmd == "fin") break;
        if (cmd == "insert") { int k; std::cin >> k; tree.insert(k); }
        else if (cmd == "delete") { int k; std::cin >> k; tree.remove(k); }
        else if (cmd == "modify") { int o, n; std::cin >> o >> n; if (!tree.modify(o, n)) std::cout << "Llave no existe.\n"; }
        else if (cmd == "search") { int k; std::cin >> k; std::cout << (tree.search(k) ? "Encontrado\n" : "No encontrado\n"); }
        else if (cmd == "print") { tree.print(); }
        else if (cmd == "dot") { std::string file; std::cin >> file; tree.exportDot(file); std::cout << "Guardado en " << file << "\n"; }
        else std::cout << "Comando invalido\n";
    }
    return 0;
}
