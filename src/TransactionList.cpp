#include "TransactionList.h"
#include <fstream>

void TransactionList::addTransaction(std::unique_ptr<Transaction> tx) {
    txList.push_back(std::move(tx));
}

int TransactionList::getSize() const {
    return txList.size();
}

Transaction* TransactionList::getAt(int i) const {
    return txList[i].get();
}

void TransactionList::saveToFile(const std::string& filename) {
    std::ofstream file(filename);
    for (const auto& tx : txList) {
        file << tx->serialize() << std::endl;
    }
}

void TransactionList::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    std::string line;
    while (std::getline(file, line)) {
        auto tx = Transaction::deserialize(line);
        if (tx) {
            addTransaction(std::move(tx));
        }
    }
}