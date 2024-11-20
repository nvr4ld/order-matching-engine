#include <gtest/gtest.h>
#include <sstream>
#include <string>

#include "../src/Order.h"
#include "../src/OrderBook.h"
#include "../src/TraderBase.h"
#include "../src/Transaction.h"
#include "../src/TransactionList.h"
#include "CommandType.h"

// Helper function to simulate input and process orders
void simulateInput(OrderBook& orderBook, TransactionList& txList, const std::string& input) {
    std::stringstream ss(input);
    std::string type, username;
    double totalPrice;
    int quantity;

    ss >> type >> username >> totalPrice >> quantity;
    CommandType commandType = getOrderTypeFromString(type);

    auto now = std::chrono::system_clock::now();
    time_t timestamp = std::chrono::system_clock::to_time_t(now);

    auto newOrder = std::make_unique<Order>(quantity, totalPrice, timestamp, username);

    if (commandType == CommandType::BUY) {
        orderBook.addBuyOrder(std::move(newOrder));
    } else if (commandType == CommandType::SELL) {
        orderBook.addSellOrder(std::move(newOrder));
    }

    // Process orders
    while (orderBook.getFrontSellOrder() && orderBook.getFrontBuyOrder() &&
           orderBook.getFrontSellOrder()->getPricePerOne() <= orderBook.getFrontBuyOrder()->getPricePerOne() &&
           orderBook.getFrontSellOrder()->getTrader() != orderBook.getFrontBuyOrder()->getTrader()) {
        Order* minSellOrder = orderBook.getFrontSellOrder();
        Order* maxBuyOrder = orderBook.getFrontBuyOrder();
        if (minSellOrder->getQuantity() == maxBuyOrder->getQuantity()) {
            auto tx = std::make_unique<Transaction>(minSellOrder->getQuantity(), minSellOrder->getPricePerOne(), timestamp, minSellOrder->getTrader(), maxBuyOrder->getTrader());
            txList.addTransaction(std::move(tx));
            orderBook.popBuyOrder();
            orderBook.popSellOrder();
        } else if (minSellOrder->getQuantity() > maxBuyOrder->getQuantity()) {
            auto tx = std::make_unique<Transaction>(maxBuyOrder->getQuantity(), minSellOrder->getPricePerOne(), timestamp, minSellOrder->getTrader(), maxBuyOrder->getTrader());
            txList.addTransaction(std::move(tx));
            minSellOrder->changeQuantity(maxBuyOrder->getQuantity());
            orderBook.popBuyOrder();
        } else {
            auto tx = std::make_unique<Transaction>(minSellOrder->getQuantity(), minSellOrder->getPricePerOne(), timestamp, minSellOrder->getTrader(), maxBuyOrder->getTrader());
            txList.addTransaction(std::move(tx));
            maxBuyOrder->changeQuantity(minSellOrder->getQuantity());
            orderBook.popSellOrder();
        }
    }
}

TEST(OrderBookTest, SimpleUsage) {
    OrderBook orderBook;
    TransactionList txList;

    simulateInput(orderBook, txList, "buy Alice 100 1");
    simulateInput(orderBook, txList, "sell Bob 100 1");

    EXPECT_EQ(orderBook.getFrontBuyOrder(), nullptr);
    EXPECT_EQ(orderBook.getFrontSellOrder(), nullptr);
    EXPECT_EQ(txList.getSize(), 1);
}

TEST(OrderBookTest, NoMatch) {
    OrderBook orderBook;
    TransactionList txList;

    simulateInput(orderBook, txList, "buy Alice 90 1");
    simulateInput(orderBook, txList, "sell Bob 100 1");

    EXPECT_NE(orderBook.getFrontBuyOrder(), nullptr);
    EXPECT_NE(orderBook.getFrontSellOrder(), nullptr);
    EXPECT_EQ(txList.getSize(), 0);
}

TEST(OrderBookTest, MultipleOrdersSomeMatch) {
    OrderBook orderBook;
    TransactionList txList;

    simulateInput(orderBook, txList, "buy Alice 100 1");
    simulateInput(orderBook, txList, "buy Charlie 90 1");
    simulateInput(orderBook, txList, "sell Bob 100 1");
    simulateInput(orderBook, txList, "sell Dave 110 1");

    EXPECT_NE(orderBook.getFrontBuyOrder(), nullptr);
    EXPECT_NE(orderBook.getFrontSellOrder(), nullptr);
    EXPECT_EQ(txList.getSize(), 1);
}

TEST(OrderBookTest, FullMatch) {
    OrderBook orderBook;
    TransactionList txList;

    for (int i = 0; i < 10; ++i) {
        simulateInput(orderBook, txList, "buy Alice 10 1");
    }
    simulateInput(orderBook, txList, "sell Bob 100 10");

    EXPECT_EQ(orderBook.getFrontBuyOrder(), nullptr);
    EXPECT_EQ(orderBook.getFrontSellOrder(), nullptr);
    EXPECT_EQ(txList.getSize(), 10);
}

TEST(OrderBookTest, SameUserNoMatch) {
    OrderBook orderBook;
    TransactionList txList;

    simulateInput(orderBook, txList, "buy Alice 100 1");
    simulateInput(orderBook, txList, "sell Alice 100 1");

    EXPECT_NE(orderBook.getFrontBuyOrder(), nullptr);
    EXPECT_NE(orderBook.getFrontSellOrder(), nullptr);
    EXPECT_EQ(txList.getSize(), 0);
}

TEST(PerformanceTest, TimePerformace) {
    OrderBook orderBook;
    TransactionList txList;
    auto startAdd = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100000; ++i) {
        simulateInput(orderBook, txList, "buy Alice 100000 1");
    }
    auto endAdd = std::chrono::high_resolution_clock::now();

    auto startMatch = std::chrono::high_resolution_clock::now();

    simulateInput(orderBook, txList, "sell Bob 100000 100000");

    auto endMatch = std::chrono::high_resolution_clock::now();

    auto durationAdding = std::chrono::duration_cast<std::chrono::milliseconds>(endAdd - startAdd).count();
    auto durationMatching = std::chrono::duration_cast<std::chrono::milliseconds>(endMatch - startMatch).count();
    std::cout << "Duration for Adding: " << durationAdding << " ms" << std::endl;
    std::cout << "Duration for Matching: " << durationMatching << " ms" << std::endl;


    EXPECT_EQ(orderBook.getFrontBuyOrder(), nullptr);
    EXPECT_EQ(orderBook.getFrontSellOrder(), nullptr);
    EXPECT_EQ(txList.getSize(), 100000);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}