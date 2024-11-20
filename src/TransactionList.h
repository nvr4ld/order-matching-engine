#ifndef TRANSACTIONLIST_H
#define TRANSACTIONLIST_H

#include <vector>
#include <memory>
#include "Transaction.h"

class TransactionList {
public:
    void addTransaction(std::unique_ptr<Transaction> tx);
    int getSize() const;
    Transaction* getAt(int i) const;
    void saveToFile(const std::string& filename);
    void loadFromFile(const std::string& filename);

private:
    std::vector<std::unique_ptr<Transaction>> txList;
};

#endif // TRANSACTIONLIST_H