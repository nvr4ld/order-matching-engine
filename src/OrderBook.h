#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include <vector>
#include <memory>
#include "Order.h"

class OrderBook {
public:
    OrderBook();
    void addSellOrder(std::unique_ptr<Order> newOrder);
    void addBuyOrder(std::unique_ptr<Order> newOrder);
    void popSellOrder();
    void popBuyOrder();
    Order* getFrontSellOrder();
    Order* getFrontBuyOrder();
    void saveToFile(const std::string& filename);
    void loadFromFile(const std::string& filename);

private:
    std::vector<std::unique_ptr<Order>> sellOrders;
    std::vector<std::unique_ptr<Order>> buyOrders;
};

#endif // ORDERBOOK_H