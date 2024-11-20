#include "Order.h"
#include <sstream>
#include <format>

Order::Order(int quantity, double totalPrice, time_t timestamp, const std::string& trader)
    : quantity(quantity), pricePerOne(totalPrice / quantity), date(timestamp), trader(trader) {}

std::string Order::serialize() const {
    return std::format("{} {} {} {}", quantity, pricePerOne, date, trader);
}

std::unique_ptr<Order> Order::deserialize(const std::string& data) {
    std::stringstream ss(data);
    int quantity;
    double pricePerOne;
    time_t date;
    std::string trader;
    if (ss >> quantity >> pricePerOne >> date >> trader) {
        return std::make_unique<Order>(quantity, pricePerOne * quantity, date, trader);
    }
    return nullptr;
}

double Order::getPricePerOne() const { return pricePerOne; }
time_t Order::getDate() const { return date; }
int Order::getQuantity() const { return quantity; }
void Order::changeQuantity(int change) { quantity -= change; }
std::string Order::getTrader() const { return trader; }