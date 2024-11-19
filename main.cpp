#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include <chrono>
#include <ctime>
#include <format>
#include <memory>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
using namespace std;

// TODO: output and input format (make it more beautiful)
// TODO: Add comments

enum class CommandType { BUY, SELL, ORDERS, TXLIST, EXIT, REGISTER};

CommandType getOrderTypeFromString(const string& type) {
    if (type == "buy") return CommandType::BUY;
    if (type == "sell") return CommandType::SELL;
    if (type == "orders") return CommandType::ORDERS;
    if (type == "txlist") return CommandType::TXLIST;
    if (type == "exit") return CommandType::EXIT;
    if (type == "register") return CommandType::REGISTER;
    throw invalid_argument(format("Invalid command \"{}\"", type));
}

class TraderBase {
    private:
        vector<string> traders;
    public:
        void addTrader(const string& username) {
            if (find(traders.begin(), traders.end(), username) == traders.end()) {
                traders.push_back(username);
            }
        }

        void saveToFile(const string& filename) {
            ofstream file(filename);
            for (const auto& username : traders) {
                file << username << endl;
            }
            file.close();
        }

        void loadFromFile(const string& filename) {
            ifstream file;
            file.open(filename);
            string line;
            while (getline(file, line)) {
                traders.push_back(line);
            }
            file.close();
        }
};

class Order{
    private:
        int quantity;
        double pricePerOne;
        time_t date;
        string trader;
    public:
        Order(int quantity, double totalPrice, time_t timestamp, string trader){
            this->quantity = quantity;
            this->pricePerOne = totalPrice/quantity;
            this->date = timestamp;
            this->trader = trader;
        };

        string serialize() const {
            return format("{} {} {} {}", quantity, pricePerOne, date, trader);
        }

        static unique_ptr<Order> deserialize(const string& data) {
            stringstream ss(data);
            int quantity;
            double pricePerOne;
            time_t date;
            string username;
            if (ss >> quantity >> pricePerOne >> date >> username) {
                return make_unique<Order>(quantity, pricePerOne * quantity, date, username);
            }
            return nullptr;
        }

        double getPricePerOne(){
            return this->pricePerOne;
        }

        time_t getDate(){
            return this->date;
        }

        int getQuantity(){
            return this->quantity;
        }

        void changeQuantity(int change){
            this->quantity -= change;
        }

        string getTrader(){
            return this->trader;
        }
};

class Transaction{
    private:
        int quantity;
        double pricePerOne;
        double totalPrice;
        time_t date;
        string seller;
        string buyer;
    public:
        Transaction(int quantity, double pricePerOne, time_t timestamp, string seller, string buyer){
            this->quantity = quantity;
            this->pricePerOne = pricePerOne;
            this->totalPrice = pricePerOne*quantity;
            this->date = timestamp;
            this->seller = seller;
            this->buyer = buyer;
        }

        string serialize() const {
            return format("{} {} {} {} {}", quantity, pricePerOne, date, seller, buyer);
        }

        static unique_ptr<Transaction> deserialize(const string& data) {
            stringstream ss(data);
            int quantity;
            double pricePerOne;
            time_t date;
            string sellerName, buyerName;
            if (ss >> quantity >> pricePerOne >> date >> sellerName >> buyerName) {
                return make_unique<Transaction>(quantity, pricePerOne, date, sellerName, buyerName);
            }
            return nullptr;
        }

        double getPricePerOne(){
            return this->pricePerOne;
        }

        time_t getDate(){
            return this->date;
        }

        int getQuantity(){
            return this->quantity;
        }

        string getSeller(){
            return this->seller;
        }

        string getBuyer(){
            return this->buyer;
        }

        double getTotalPrice(){
            return this->totalPrice;
        }
};

class TransactionList{
    private:
        vector<unique_ptr<Transaction>> txList;
    public:

        void addTransaction(unique_ptr<Transaction> tx){
            txList.push_back(std::move(tx));
        }

        int getSize() const{
            return txList.size();
        }

        Transaction* getAt(int i) const{
            return txList[i].get();
        }

        void saveToFile(const string& filename) {
            ofstream file(filename);
            for (const auto& tx : txList) {
                file << tx->serialize() << endl;
            }
            file.close();
        }

        void loadFromFile(const string& filename) {
            ifstream file;
            file.open(filename);
            string line;
            while (getline(file, line)) {
                auto tx = Transaction::deserialize(line);
                if (tx) {
                    addTransaction(std::move(tx));
                }
            }
            file.close();
        }
};

struct greaterBuy{
    bool operator()(const unique_ptr<Order>& a,const unique_ptr<Order>& b) const{
        if(a->getPricePerOne() == b->getPricePerOne()){
            return a->getDate() > b->getDate();
        }
        return a->getPricePerOne() < b->getPricePerOne();
    }
};

struct greaterSell{
    bool operator()(const unique_ptr<Order>& a,const unique_ptr<Order>& b) const{
        if(a->getPricePerOne() == b->getPricePerOne()){
            return a->getDate() > b->getDate();
        }
        return a->getPricePerOne() > b->getPricePerOne();
    }
};

class OrderBook{
    private:
        vector<unique_ptr<Order>> sellOrders;
        vector<unique_ptr<Order>> buyOrders;
    public:
        OrderBook(){
            make_heap(sellOrders.begin(), sellOrders.end(), greaterSell());
            make_heap(buyOrders.begin(), buyOrders.end(), greaterBuy());
        };

        void addSellOrder(unique_ptr<Order> newOrder){
            sellOrders.push_back(std::move(newOrder));
            push_heap(sellOrders.begin(), sellOrders.end(), greaterSell());
        }

        void addBuyOrder(unique_ptr<Order> newOrder){
            buyOrders.push_back(std::move(newOrder));
            push_heap(buyOrders.begin(), buyOrders.end(), greaterBuy());
        }

        void popSellOrder(){
            if(!sellOrders.empty()){
                pop_heap(sellOrders.begin(), sellOrders.end(), greaterSell());
                sellOrders.pop_back();
            }
        }
        
        void popBuyOrder(){
            if(!buyOrders.empty()){
                pop_heap(buyOrders.begin(), buyOrders.end(), greaterBuy());
                buyOrders.pop_back();
            }
        }

        Order* getFrontSellOrder(){
            return sellOrders.empty() ? nullptr : sellOrders.front().get();
        }
        
        Order* getFrontBuyOrder(){
            return buyOrders.empty() ? nullptr : buyOrders.front().get();
        }

        void saveToFile(const string& filename) {
            ofstream file(filename);
            for (const auto& order : sellOrders) {
                file << "sell " << order->serialize() << endl;
            }
            for (const auto& order : buyOrders) {
                file << "buy " << order->serialize() << endl;
            }
            file.close();
        }

        void loadFromFile(const string& filename) {
            ifstream file(filename);
            string line;
            while (getline(file, line)) {
                stringstream ss(line);
                string type, orderData;
                ss >> type;
                getline(ss, orderData);
                auto order = Order::deserialize(orderData);
                if (order) {
                    if (type == "sell") {
                        addSellOrder(std::move(order));
                    } else if (type == "buy") {
                        addBuyOrder(std::move(order));
                    }
                }
            }
        }
};

queue<string> messageQueue;
mutex queueMutex, fileMutex;
condition_variable queueCv;
bool done = false;

void writeFile(const string& filename, const string& topBuyOrder, const string& topSellOrder) {
    std::lock_guard<std::mutex> lock(fileMutex);
    std::ofstream file(filename, std::ios::trunc);
    if (file.is_open()) {
        file << "Top Buy Order: " << topBuyOrder << endl;
        file << "Top Sell Order: " << topSellOrder << endl;
    }
}

void inputHandler(TraderBase& traderBase, OrderBook& orderBook, TransactionList& txList) {
    string inputLine, type, username;
    CommandType commandType;
    double totalPrice;
    int quantity;
    while (true) {
        cout << "> ";
        getline(cin, inputLine);
        stringstream ss(inputLine);
        if(inputLine.empty()){
            continue;
        }
        ss >> type;
        try{
            commandType = getOrderTypeFromString(type);
        }
        catch(invalid_argument& e){
            cerr << e.what() << endl;
            continue;
        }
        if (commandType == CommandType::EXIT) {
            done = true;
            queueCv.notify_all();
            cout << "Input thread has finished" << endl;
            break;
        }
        if (commandType == CommandType::TXLIST){
            if(txList.getSize()){
                for(int i = 0; i < txList.getSize(); i++){
                    Transaction* tx = txList.getAt(i);
                    auto txDate = tx->getDate();
                    cout << "Quantity: " << tx->getQuantity() << " | Total Price: " << tx->getTotalPrice() << " | Date: " << ctime(&txDate) << " | Buyer: " << tx->getBuyer() << " | Seller: " << tx->getSeller() << endl;
                }
            }
            else cout << "No available transactions!" << endl;
        }
        else {
            if (!(ss >> username >> totalPrice >> quantity)) {
                cout << "Error: Invalid input. Please provide your username, totalPrice and quantity to create order" << endl;
                continue;
            }
            if(quantity <= 0){
                cout << "Quantity must be greater than 0." << endl;
                continue;
            }
            traderBase.addTrader(username);
            lock_guard<mutex> lock(queueMutex);
            messageQueue.push(inputLine);
        }
        
        
        queueCv.notify_one();
    }
}

void processor(TraderBase& traderBase, OrderBook& orderBook, TransactionList& txList) {
    vector<future<void>> futures;
    while (true) {
        double totalPrice;
        int quantity;
        string type, username;
        CommandType commandType;

        unique_lock<mutex> lock(queueMutex);
        queueCv.wait(lock, []{ return !messageQueue.empty() || done; });

        if (done && messageQueue.empty()){
            cout << "Processor has finished" << endl;
            break;
        }

        string message = messageQueue.front();
        messageQueue.pop();
        lock.unlock();

        auto now = chrono::system_clock::now();
        time_t timestamp = chrono::system_clock::to_time_t(now);

        stringstream ss(message);

        ss >> type;
        commandType = getOrderTypeFromString(type);

        ss >> username >> totalPrice >> quantity;
        auto newOrder = make_unique<Order>(quantity, totalPrice, timestamp, username);

        if(commandType == CommandType::BUY){
            orderBook.addBuyOrder(std::move(newOrder));
        }
        else{
            orderBook.addSellOrder(std::move(newOrder));
        }

        while(orderBook.getFrontSellOrder() && orderBook.getFrontBuyOrder() 
        && orderBook.getFrontSellOrder()->getPricePerOne() <= orderBook.getFrontBuyOrder()->getPricePerOne() 
        && orderBook.getFrontSellOrder()->getTrader() != orderBook.getFrontBuyOrder()->getTrader()){
            Order* minSellOrder = orderBook.getFrontSellOrder();
            Order* maxBuyOrder = orderBook.getFrontBuyOrder();
            if(minSellOrder->getQuantity() == maxBuyOrder->getQuantity()){
                auto tx = make_unique<Transaction>(minSellOrder->getQuantity(), minSellOrder->getPricePerOne(), timestamp, minSellOrder->getTrader(), maxBuyOrder->getTrader());
                txList.addTransaction(std::move(tx));
                orderBook.popBuyOrder();
                orderBook.popSellOrder();
            }
            else if(minSellOrder->getQuantity() > maxBuyOrder->getQuantity()){
                auto tx = make_unique<Transaction>(maxBuyOrder->getQuantity(), minSellOrder->getPricePerOne(), timestamp, minSellOrder->getTrader(), maxBuyOrder->getTrader());
                txList.addTransaction(std::move(tx));
                minSellOrder->changeQuantity(maxBuyOrder->getQuantity());
                orderBook.popBuyOrder();
            }
            else{
                auto tx = make_unique<Transaction>(minSellOrder->getQuantity(), minSellOrder->getPricePerOne(), timestamp, minSellOrder->getTrader(), maxBuyOrder->getTrader());
                txList.addTransaction(std::move(tx));
                maxBuyOrder->changeQuantity(minSellOrder->getQuantity());
                orderBook.popSellOrder();
            }
        }

        Order* topBuyOrder = orderBook.getFrontBuyOrder();
        Order* topSellOrder = orderBook.getFrontSellOrder();
        string topBuyOrderStr = topBuyOrder ? topBuyOrder->serialize() : "No Buy Orders";
        string topSellOrderStr = topSellOrder ? topSellOrder->serialize() : "No Sell Orders";
        futures.push_back(async(launch::async, writeFile, "topOrders.txt", topBuyOrderStr, topSellOrderStr));
    }
}

int main() {
    TraderBase traderBase;
    OrderBook orderBook;
    TransactionList txList;

    traderBase.loadFromFile("traders.txt");
    orderBook.loadFromFile("orders.txt");
    txList.loadFromFile("transactions.txt");

    thread inputThread(inputHandler, ref(traderBase), ref(orderBook), ref(txList));
    thread processingThread(processor, ref(traderBase), ref(orderBook), ref(txList));

    inputThread.join();
    processingThread.join();

    traderBase.saveToFile("traders.txt");
    orderBook.saveToFile("orders.txt");
    txList.saveToFile("transactions.txt");
    return 0;
}