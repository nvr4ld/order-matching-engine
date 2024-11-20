#ifndef ORDER_H
#define ORDER_H

#include <string>
#include <memory>

class Order {
public:
    Order(int quantity, double totalPrice, time_t timestamp, const std::string& trader);
    std::string serialize() const;
    static std::unique_ptr<Order> deserialize(const std::string& data);
    double getPricePerOne() const;
    time_t getDate() const;
    int getQuantity() const;
    void changeQuantity(int change);
    std::string getTrader() const;

private:
    int quantity;
    double pricePerOne;
    time_t date;
    std::string trader;
};

#endif // ORDER_H