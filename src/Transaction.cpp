#include "Transaction.h"
#include <sstream>
#include <format>

Transaction::Transaction(int quantity, double pricePerOne, time_t timestamp, const std::string& seller, const std::string& buyer)
    : quantity(quantity), pricePerOne(pricePerOne), totalPrice(pricePerOne * quantity), date(timestamp), seller(seller), buyer(buyer) {}

std::string Transaction::serialize() const {
    return std::format("{} {} {} {} {}", quantity, pricePerOne, date, seller, buyer);
}

std::unique_ptr<Transaction> Transaction::deserialize(const std::string& data) {
    std::stringstream ss(data);
    int quantity;
    double pricePerOne;
    time_t date;
    std::string seller, buyer;
    if (ss >> quantity >> pricePerOne >> date >> seller >> buyer) {
        return std::make_unique<Transaction>(quantity, pricePerOne, date, seller, buyer);
    }
    return nullptr;
}

double Transaction::getPricePerOne() const { return pricePerOne; }
time_t Transaction::getDate() const { return date; }
int Transaction::getQuantity() const { return quantity; }
std::string Transaction::getSeller() const { return seller; }
std::string Transaction::getBuyer() const { return buyer; }
double Transaction::getTotalPrice() const { return totalPrice; }