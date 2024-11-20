#include "OrderBook.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <format>

struct greaterBuy{
    bool operator()(const std::unique_ptr<Order>& a,const std::unique_ptr<Order>& b) const{
        if(a->getPricePerOne() == b->getPricePerOne()){
            return a->getDate() > b->getDate();
        }
        return a->getPricePerOne() < b->getPricePerOne();
    }
};

struct greaterSell{
    bool operator()(const std::unique_ptr<Order>& a,const std::unique_ptr<Order>& b) const{
        if(a->getPricePerOne() == b->getPricePerOne()){
            return a->getDate() > b->getDate();
        }
        return a->getPricePerOne() > b->getPricePerOne();
    }
};

OrderBook::OrderBook() {
    std::make_heap(sellOrders.begin(), sellOrders.end(), greaterSell());
    std::make_heap(buyOrders.begin(), buyOrders.end(), greaterBuy());
}

void OrderBook::addSellOrder(std::unique_ptr<Order> newOrder) {
    sellOrders.push_back(std::move(newOrder));
    std::push_heap(sellOrders.begin(), sellOrders.end(), greaterSell());
}

void OrderBook::addBuyOrder(std::unique_ptr<Order> newOrder) {
    buyOrders.push_back(std::move(newOrder));
    std::push_heap(buyOrders.begin(), buyOrders.end(), greaterBuy());
}

void OrderBook::popSellOrder() {
    if (!sellOrders.empty()) {
        std::pop_heap(sellOrders.begin(), sellOrders.end(), greaterSell());
        sellOrders.pop_back();
    }
}

void OrderBook::popBuyOrder() {
    if (!buyOrders.empty()) {
        std::pop_heap(buyOrders.begin(), buyOrders.end(), greaterBuy());
        buyOrders.pop_back();
    }
}

Order* OrderBook::getFrontSellOrder() {
    return sellOrders.empty() ? nullptr : sellOrders.front().get();
}

Order* OrderBook::getFrontBuyOrder() {
    return buyOrders.empty() ? nullptr : buyOrders.front().get();
}

void OrderBook::saveToFile(const std::string& filename) {
    std::ofstream file(filename);
    if(file.is_open()){
        for (const auto& order : sellOrders) {
            file << "sell " << order->serialize() << std::endl;
        }
        for (const auto& order : buyOrders) {
            file << "buy " << order->serialize() << std::endl;
        }
        file.close();
    }
    else{
        std::cerr << "Error opening a file: " << filename << std::endl;
    }
}

void OrderBook::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    std::string line;
    if(file.is_open()){
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string type, orderData;
            ss >> type;
            std::getline(ss, orderData);
            auto order = Order::deserialize(orderData);
            if (order) {
                if (type == "sell") {
                    addSellOrder(std::move(order));
                } else if (type == "buy") {
                    addBuyOrder(std::move(order));
                }
            }
        }
        file.close();
    }
    else{
        std::cerr << "Error opening a file: " << filename << std::endl;
    }
    
}