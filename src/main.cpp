#include "TraderBase.h"
#include "OrderBook.h"
#include "TransactionList.h"
#include "CommandType.h"
#include <iostream>
#include <fstream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <sstream>

std::queue<std::string> messageQueue;
std::mutex queueMutex, fileMutex, txListMutex;
std::condition_variable queueCv;
bool inputFinished = false;
int TXLIST_OUTPUT_SIZE = 5;

void writeTopOrders(const std::string& filename, const std::string& topBuyOrder, const std::string& topSellOrder) {
    std::lock_guard<std::mutex> lock(fileMutex);
    std::ofstream file(filename, std::ios::trunc);
    if (file.is_open()) {
        file << "Top Buy Order: " << topBuyOrder << std::endl;
        file << "Top Sell Order: " << topSellOrder << std::endl;
        file.close();
    }
    else{
        std::cerr << "Error opening a file: " << filename << std::endl;
    }
}

void inputHandler(TraderBase& traderBase, OrderBook& orderBook, TransactionList& txList) {
    std::string inputLine, type, username;
    CommandType commandType;
    double totalPrice;
    int quantity;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, inputLine);
        std::stringstream ss(inputLine);
        if (inputLine.empty()) continue;
        ss >> type;
        try {
            commandType = getOrderTypeFromString(type);
        }
        catch (std::invalid_argument& e) {
            std::cerr << e.what() << std::endl;
            continue;
        }
        if (commandType == CommandType::EXIT) {
            inputFinished = true;
            queueCv.notify_all();
            std::cout << "Input thread has finished" << std::endl;
            break;
        }
        if (commandType == CommandType::TXLIST) {
            int txListSize = txList.getSize();
            if (txListSize) {
                int outputSize = std::min(TXLIST_OUTPUT_SIZE, txListSize);
                std::unique_lock<std::mutex> lock(txListMutex);
                std::vector<Transaction*> txArr = txList.getLastN(outputSize);
                txListMutex.unlock();
                for (auto tx: txArr) {
                    auto txDate = tx->getDate();
                    std::cout << "Quantity: " << tx->getQuantity()
                              << " | Total Price: " << tx->getTotalPrice()
                              << " | Date: " << std::ctime(&txDate)
                              << " | Buyer: " << tx->getBuyer()
                              << " | Seller: " << tx->getSeller() << std::endl;
                }
            } else {
                std::cout << "No available transactions!" << std::endl;
            }
        } else {
            if (!(ss >> username >> totalPrice >> quantity)) {
                std::cout << "Error: Invalid input. Please provide your username, totalPrice and quantity to create order" << std::endl;
                continue;
            }
            if (quantity <= 0) {
                std::cout << "Quantity must be greater than 0." << std::endl;
                continue;
            }
            traderBase.addTrader(username);
            std::lock_guard<std::mutex> lock(queueMutex);
            messageQueue.push(inputLine);
        }
        queueCv.notify_one();
    }
}

void processor(TraderBase& traderBase, OrderBook& orderBook, TransactionList& txList) {
    std::vector<std::future<void>> futures;
    while (true) {
        double totalPrice;
        int quantity;
        std::string type, username;
        CommandType commandType;

        std::unique_lock<std::mutex> lockMsgQueue(queueMutex);
        queueCv.wait(lockMsgQueue, [] { return !messageQueue.empty() || inputFinished; });

        if (inputFinished && messageQueue.empty()) {
            std::cout << "Processor has finished" << std::endl;
            break;
        }

        std::string message = messageQueue.front();
        messageQueue.pop();
        lockMsgQueue.unlock();

        auto now = std::chrono::system_clock::now();
        time_t timestamp = std::chrono::system_clock::to_time_t(now);

        std::stringstream ss(message);

        ss >> type;
        commandType = getOrderTypeFromString(type);

        ss >> username >> totalPrice >> quantity;
        auto newOrder = std::make_unique<Order>(quantity, totalPrice, timestamp, username);

        if (commandType == CommandType::BUY) {
            orderBook.addBuyOrder(std::move(newOrder));
        } else {
            orderBook.addSellOrder(std::move(newOrder));
        }

        while (orderBook.getFrontSellOrder() && orderBook.getFrontBuyOrder()
            && orderBook.getFrontSellOrder()->getPricePerOne() <= orderBook.getFrontBuyOrder()->getPricePerOne()
            && orderBook.getFrontSellOrder()->getTrader() != orderBook.getFrontBuyOrder()->getTrader()) {
            Order* minSellOrder = orderBook.getFrontSellOrder();
            Order* maxBuyOrder = orderBook.getFrontBuyOrder();
            std::unique_lock<std::mutex> lockTxList(txListMutex, std::defer_lock);
            auto res = minSellOrder->getQuantity() <=> maxBuyOrder->getQuantity();
            if (res == 0) {
                auto tx = std::make_unique<Transaction>(minSellOrder->getQuantity(), minSellOrder->getPricePerOne(), timestamp, minSellOrder->getTrader(), maxBuyOrder->getTrader());
                lockTxList.lock();
                txList.addTransaction(std::move(tx));
                lockTxList.unlock();
                orderBook.popBuyOrder();
                orderBook.popSellOrder();
            } 
            else if (res > 0) {
                auto tx = std::make_unique<Transaction>(maxBuyOrder->getQuantity(), minSellOrder->getPricePerOne(), timestamp, minSellOrder->getTrader(), maxBuyOrder->getTrader());
                lockTxList.lock();
                txList.addTransaction(std::move(tx));
                lockTxList.unlock();
                minSellOrder->changeQuantity(maxBuyOrder->getQuantity());
                orderBook.popBuyOrder();
            } 
            else {
                auto tx = std::make_unique<Transaction>(minSellOrder->getQuantity(), minSellOrder->getPricePerOne(), timestamp, minSellOrder->getTrader(), maxBuyOrder->getTrader());
                lockTxList.lock();
                txList.addTransaction(std::move(tx));
                lockTxList.unlock();
                maxBuyOrder->changeQuantity(minSellOrder->getQuantity());
                orderBook.popSellOrder();
            }
        }

        Order* topBuyOrder = orderBook.getFrontBuyOrder();
        Order* topSellOrder = orderBook.getFrontSellOrder();
        std::string topBuyOrderStr = topBuyOrder ? topBuyOrder->serialize() : "No Buy Orders";
        std::string topSellOrderStr = topSellOrder ? topSellOrder->serialize() : "No Sell Orders";
        futures.push_back(std::async(std::launch::async, writeTopOrders, "../storage/topOrders.txt", topBuyOrderStr, topSellOrderStr));
    }
}

int main() {
    TraderBase traderBase;
    OrderBook orderBook;
    TransactionList txList;

    traderBase.loadFromFile("../storage/traders.txt");
    orderBook.loadFromFile("../storage/orders.txt");
    txList.loadFromFile("../storage/transactions.txt");

    std::thread inputThread(inputHandler, std::ref(traderBase), std::ref(orderBook), std::ref(txList));
    std::thread processingThread(processor, std::ref(traderBase), std::ref(orderBook), std::ref(txList));

    inputThread.join();
    processingThread.join();

    traderBase.saveToFile("../storage/traders.txt");
    orderBook.saveToFile("../storage/orders.txt");
    txList.saveToFile("../storage/transactions.txt");
    return 0;
}