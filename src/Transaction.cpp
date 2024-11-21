#include "Transaction.h"
#include <sstream>
#include <format>

Transaction::Transaction(int quantity, double pricePerOne, time_t timestamp, const std::string& seller, const std::string& buyer)
    : quantity(quantity), pricePerOne(pricePerOne), totalPrice(pricePerOne * quantity), date(timestamp), seller(seller), buyer(buyer) {}
    // - `quantity`: The number of items in the order.
    // - `pricePerOne * quantity`: Sets the total price.
    // - `date`: The timestamp for when the order was created.
    // - `seller`: The username of a trader who placed sell order.
    // - `buyer`: The username of a trader who placed buy order.

// serialize transaction for file writing
std::string Transaction::serialize() const {
    return std::format("{} {} {} {} {}", quantity, pricePerOne, date, seller, buyer);
}

// deserialize transaction loaded from a file
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

// getter functions for private attributes
double Transaction::getPricePerOne() const { return pricePerOne; }
time_t Transaction::getDate() const { return date; }
int Transaction::getQuantity() const { return quantity; }
std::string Transaction::getSeller() const { return seller; }
std::string Transaction::getBuyer() const { return buyer; }
double Transaction::getTotalPrice() const { return totalPrice; }