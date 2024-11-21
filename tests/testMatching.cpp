#include <gtest/gtest.h>
#include <sstream>
#include <string>

#include "../src/Order.h"
#include "../src/OrderBook.h"
#include "../src/TraderBase.h"
#include "../src/Transaction.h"
#include "../src/TransactionList.h"
#include "../src/CommandType.h"

// Helper function to simulate input and process orders
void simulateInput(OrderBook& orderBook, TransactionList& txList, const std::string& input) {
    std::stringstream ss(input);
    std::string type, username;
    double totalPrice;
    int quantity;

    ss >> type;
    CommandType commandType;
    commandType = getOrderTypeFromString(type);

    if (!(ss >> username >> totalPrice >> quantity)) {
        return;
    }
    if (quantity <= 0) {
        return;
    }
    if (totalPrice <= 0) {
        return;
    }

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

// Test:        Simple match
// Input:       Buy order with price = 100 and Sell order with price = 100
// Expected:    Orders are popped, 1 transaction is processed
TEST(OrderBookTest, Match) {
    OrderBook orderBook;
    TransactionList txList;

    simulateInput(orderBook, txList, "buy Alice 100 1");
    simulateInput(orderBook, txList, "sell Bob 100 1");

    EXPECT_EQ(orderBook.getFrontBuyOrder(), nullptr);
    EXPECT_EQ(orderBook.getFrontSellOrder(), nullptr);
    EXPECT_EQ(txList.getSize(), 1);
}

// Test:        No match
// Input:       Buy order with price = 90 and Sell order with price = 100
// Expected:    Orders remain, no transaction processed
TEST(OrderBookTest, NoMatch) {
    OrderBook orderBook;
    TransactionList txList;

    simulateInput(orderBook, txList, "buy Alice 90 1");
    simulateInput(orderBook, txList, "sell Bob 100 1");

    EXPECT_NE(orderBook.getFrontBuyOrder(), nullptr);
    EXPECT_NE(orderBook.getFrontSellOrder(), nullptr);
    EXPECT_EQ(txList.getSize(), 0);
}

// Test:        Partial matching and quantity change
// Input:       Buy order with quantity = 2 and Sell order with quantity = 5
// Expected:    Sell order remains with quantity = 3, one transaction is processed
TEST(OrderBookTest, PartialMatch) {
    OrderBook orderBook;
    TransactionList txList;

    simulateInput(orderBook, txList, "buy Alice 50 2");
    simulateInput(orderBook, txList, "sell Bob 100 5");

    EXPECT_EQ(txList.getSize(), 1);
    EXPECT_NE(orderBook.getFrontSellOrder(), nullptr);
    EXPECT_EQ(orderBook.getFrontBuyOrder(), nullptr);

    EXPECT_EQ(orderBook.getFrontSellOrder()->getQuantity(), 3);
}

// Test:        Correct matching with multiple orders in the orderbook
// Input:       2 buy and 2 sell orders of different prices
// Expected:    1 transaction and other 2 orders remain in the orderbook
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

// Test:        While loop continues to match unless no match
// Input:       10 Buy orders of price = 10 and quantity = 1, and 1 Sell order of price = 100 and quantity = 10.
// Expected:    10 transactions and empty orderbook
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

// Test:        Buy and Sell orders from the same user must not match
// Input:       Buy and Sell orders with matching prices from one user
// Expected:    Orders remain in orderBook and txList size equals 0
TEST(OrderBookTest, SameUserNoMatch) {
    OrderBook orderBook;
    TransactionList txList;

    simulateInput(orderBook, txList, "buy Alice 100 1");
    simulateInput(orderBook, txList, "sell Alice 100 1");

    EXPECT_NE(orderBook.getFrontBuyOrder(), nullptr);
    EXPECT_NE(orderBook.getFrontSellOrder(), nullptr);
    EXPECT_EQ(txList.getSize(), 0);
}

// Test:        Send invalid command type and check if exception is thrown
// Input:       invalid command "invalid Alice 100 1"
// Expected:    invalid_argument to be thrown
TEST(OrderBookTest, InvalidCommand) {
    OrderBook orderBook;
    TransactionList txList;
    EXPECT_THROW({
        try{
            simulateInput(orderBook, txList, "invalid Alice 100 1");
        }
        catch (std::invalid_argument& e) {
            EXPECT_STREQ("Invalid command: \"invalid\"", e.what() );
            return;
        }
    }, std::invalid_argument);
}

// Test:        Send invalid command format and check if order is processed
// Input:       negative quantity, negative price and missing parameters
// Expected:    empty orderBook and txList
TEST(OrderBookTest, InvalidInput) {
    OrderBook orderBook;
    TransactionList txList;

    simulateInput(orderBook, txList, "buy Alice -100 1");    // Negative price
    EXPECT_EQ(orderBook.getFrontBuyOrder(), nullptr);

    simulateInput(orderBook, txList, "sell Bob 100 -1");     // Negative quantity
    EXPECT_EQ(orderBook.getFrontSellOrder(), nullptr);

    simulateInput(orderBook, txList, "buy Charlie");         // Missing parameters
    EXPECT_EQ(orderBook.getFrontBuyOrder(), nullptr);

    EXPECT_EQ(txList.getSize(), 0);                         // No valid transactions should occur
}

// Test:        Performance test by adding 100.000 orders and matching 100.000 times.
// Input:       100.000 buy orders with quantity = 1 and price = 1
//              and 1 order with quantity = 100.000 and price = 100.000
// Expected:    100.000 transactions and empty orderbook.
TEST(OrderBookTest, PerformanceTest) {
    OrderBook orderBook;
    TransactionList txList;
    auto startAdd = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100000; ++i) {
        simulateInput(orderBook, txList, "buy Alice 1 1");
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