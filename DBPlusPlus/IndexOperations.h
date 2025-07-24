#pragma once
#include <vector>
#include <string>
#include <tuple>
#include <algorithm>
#include <unordered_map>
#include "QueryTreeIndexing.h" // Include the header that defines BPlusTreeIndex

// Structure to track pending index operations globally
struct IndexOperation {
    int pageId;                // Page ID that was modified
    std::string field;         // Field/column name being indexed
    std::string key;           // Key value being inserted/removed
    bool isInsert;             // true = insert, false = delete
    std::string method;        // Index method (btree, hash, etc.)

    IndexOperation(int pid, const std::string& f, const std::string& k, bool ins, const std::string& m)
        : pageId(pid), field(f), key(k), isInsert(ins), method(m) {
    }
};

// Global vector to track pending index operations (similar to paginasModificadas)
inline std::vector<IndexOperation> pendingIndexOperations;

// Forward declare the indexCache with proper template parameters
extern std::unordered_map<std::string, BPlusTreeIndex*> indexCache;

// Register an index operation - called whenever a record is modified
inline void registrarOperacionIndice(int pageId, const std::string& field,
    const std::string& key, bool isInsert, const std::string& method) {
    pendingIndexOperations.emplace_back(pageId, field, key, isInsert, method);
}

// Apply pending operations for a specific page and remove them from the list
inline void aplicarOperacionesIndice(int pageId) {
    // Group operations by field name to avoid duplicate tree accesses
    std::unordered_map<std::string, std::vector<std::tuple<std::string, bool, std::string>>> fieldOps;

    // Find all operations for this page
    auto it = std::remove_if(pendingIndexOperations.begin(), pendingIndexOperations.end(),
        [pageId, &fieldOps](const IndexOperation& op) {
            if (op.pageId == pageId) {
                fieldOps[op.field].emplace_back(op.key, op.isInsert, op.method);
                return true;
            }
            return false;
        });

    // Actually remove the elements
    pendingIndexOperations.erase(it, pendingIndexOperations.end());

    // Apply all operations by field
    for (const auto& [fieldName, ops] : fieldOps) {
        auto it = indexCache.find(fieldName);
        if (it != indexCache.end() && it->second != nullptr) {
            BPlusTreeIndex* idx = it->second;
            std::string method = ""; // Default method string

            for (const auto& [key, isInsert, opMethod] : ops) {
                method = opMethod; // Save method for use in the filename

                try {
                    // Convert string key to int since that's what BPlusTree methods expect
                    int keyAsInt = std::stoi(key);

                    if (isInsert) {
                        idx->insertKey(keyAsInt, pageId);
                    }
                    else {
                        idx->removeKey(keyAsInt);
                    }
                }
                catch (const std::exception& e) {
                    // Log error if string can't be converted to int
                    std::cerr << "Error converting key '" << key << "' to int: " << e.what() << std::endl;
                }
            }

            // Save updated index to disk
            idx->dumpToTxt(fieldName + "_" + method + "_bptree.txt");
        }
    }
}