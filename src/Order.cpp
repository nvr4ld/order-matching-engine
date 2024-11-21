#include "Order.h"
#include <sstream>
#include <format>

Order::Order(int quantity, double totalPrice, time_t timestamp, const std::string& trader)
    : quantity(quantity), pricePerOne(totalPrice / quantity), date(timestamp), trader(trader) {}
    // - `quantity`: The number of items in the order.
    // - `totalPrice / quantity`: Sets the price per item.
    // - `date`: The timestamp for when the order was created.
    // - `trader`: The username of a trader who placed the order.

// serialize order for file writing
std::string Order::serialize() const {
    return std::format("{} {} {} {}", quantity, pricePerOne, date, trader);
}

// deserialize order received from a file
std::unique_ptr<Order> Order::deserialize(const std::string& data) {
    std::stringstream ss(data);
    int quantity;
    double pricePerOne;
    time_t date;
    std::string trader;
    if (ss >> quantity >> pricePerOne >> date >> trader) {
        return std::make_unique<Order>(quantity, pricePerOne * quantity, date, trader);
    }
    // Return nullptr if deserialization fails
    return nullptr;
}

// getter functions for private attributes
double Order::getPricePerOne() const { return pricePerOne; }
time_t Order::getDate() const { return date; }
int Order::getQuantity() const { return quantity; }
std::string Order::getTrader() const { return trader; }

// modify the quantity (used when order matched)
void Order::changeQuantity(int change) { quantity -= change; }
