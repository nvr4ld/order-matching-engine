#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <string>
#include <memory>

class Transaction {
public:
    Transaction(int quantity, double pricePerOne, time_t timestamp, const std::string& seller, const std::string& buyer);
    std::string serialize() const;
    static std::unique_ptr<Transaction> deserialize(const std::string& data);
    double getPricePerOne() const;
    time_t getDate() const;
    int getQuantity() const;
    std::string getSeller() const;
    std::string getBuyer() const;
    double getTotalPrice() const;

private:
    int quantity;
    double pricePerOne;
    double totalPrice;
    time_t date;
    std::string seller;
    std::string buyer;
};

#endif // TRANSACTION_H