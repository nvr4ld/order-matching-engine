#include "TransactionList.h"
#include <iostream>
#include <fstream>

void TransactionList::addTransaction(std::unique_ptr<Transaction> tx) {
    txList.push_back(std::move(tx));
}

int TransactionList::getSize() const {
    return txList.size();
}

// return latest n transactions
std::vector<Transaction*> TransactionList::getLastN(int n) {
    int txCounter = 0;
    std::vector<Transaction*> ans;
    ans.reserve(n);
    for(auto it = txList.rbegin(); it != txList.rend() && n > 0; ++it, --n){
        ans.push_back(it->get());
    }
    return ans;
}

void TransactionList::saveToFile(const std::string& filename) {
    std::ofstream file(filename);
    if(file.is_open()){
        for (const auto& tx : txList) {
            file << tx->serialize() << std::endl;
        }
        file.close();
    }
    else{
        std::cerr << "Error opening a file: " << filename << std::endl;
    }
}

void TransactionList::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if(file.is_open()){
        std::string line;
        while (std::getline(file, line)) {
            auto tx = Transaction::deserialize(line);
            if (tx) {
                addTransaction(std::move(tx));
            }
        }
    }
    else{
        std::cerr << "Error opening a file: " << filename << std::endl;
    }
}