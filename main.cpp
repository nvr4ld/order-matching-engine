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
using namespace std;

// TODO: output and input format (make it more beautiful)
// TODO: Add comments

enum class CommandType { BUY, SELL, ORDERS, TXLIST, EXIT, REGISTER, LOGIN, LOGOUT };

CommandType getOrderTypeFromString(const string& type) {
    if (type == "buy") return CommandType::BUY;
    if (type == "sell") return CommandType::SELL;
    if (type == "orders") return CommandType::ORDERS;
    if (type == "txlist") return CommandType::TXLIST;
    if (type == "exit") return CommandType::EXIT;
    if (type == "register") return CommandType::REGISTER;
    if (type == "login") return CommandType::LOGIN;
    if (type == "logout") return CommandType::LOGOUT;
    throw invalid_argument(format("Invalid command \"{}\"", type));
}

class Trader {
    private:
        string username;
        string password;
        double availableBalance;
    public:
        Trader(string username, string password, double availableBalance = 100.0){
            this->username = username;
            this->password = password;
            this->availableBalance = availableBalance;
        }

        void changeBalance(double change){
            if(change > 0.0 && availableBalance < change){
                throw runtime_error(format("Insufficient balance: {}! Attempted to spend {}.", availableBalance, change));
            }
            availableBalance -= change;
        }

        string getUsername() const{
            return username;
        }

        bool authenticate(string& password){
            return this->password == password;
        }

        string serialize() const {
            return format("{};{};{}", username, password, availableBalance);
        }

        static unique_ptr<Trader> deserialize(const string& data) {
            stringstream ss(data);
            string username, password;
            double availableBalance;

            if (getline(ss, username, ';') && getline(ss, password, ';') && (ss >> availableBalance)) {
                return make_unique<Trader>(username, password, availableBalance);
            }
            return nullptr;
        }
};

class TraderBase {
    private:
        unordered_map<string, unique_ptr<Trader>> traders;
    public:
        Trader* addTrader(const string& username, const string& password) {
            if (traders.find(username) != traders.end()) {
                return nullptr;
            }
            traders.insert({username, make_unique<Trader>(username, password)});
            return traders[username].get();
        }

        Trader* authenticate(const string& username, string& password) {
            auto it = traders.find(username);
            if (it != traders.end() && it->second->authenticate(password)) {
                return it->second.get();
            }
            return nullptr;
        } 

        Trader* getTraderByUsername(const string& username) {
            auto it = traders.find(username);
            if (it != traders.end()){
                return it->second.get();
            }
            return nullptr;
        }

        void saveToFile(const string& filename) {
            ofstream file;
            file.open(filename);
            for (const auto& trader : traders) {
                file << trader.second->serialize() << endl;
            }
            file.close();
        }

        void loadFromFile(const string& filename) {
            ifstream file;
            file.open(filename);
            string line;
            while (getline(file, line)) {
                auto trader = Trader::deserialize(line);
                if (trader) {
                    traders.insert({trader->getUsername(), std::move(trader)});
                }
            }
            file.close();
        }
};

class Order{
    private:
        int quantity;
        double pricePerOne;
        time_t date;
        Trader* trader;
    public:
        Order(int quantity, double totalPrice, time_t timestamp, Trader* trader){
            this->quantity = quantity;
            this->pricePerOne = totalPrice/quantity;
            this->date = timestamp;
            this->trader = trader;
        };

        string serialize() const {
            return format("{} {} {} {}", quantity, pricePerOne, date, trader->getUsername());
        }

        static unique_ptr<Order> deserialize(const string& data, TraderBase& traderBase) {
            stringstream ss(data);
            int quantity;
            double pricePerOne;
            time_t date;
            string username;
            if (ss >> quantity >> pricePerOne >> date >> username) {
                Trader* trader = traderBase.getTraderByUsername(username);
                if(trader){
                    return make_unique<Order>(quantity, pricePerOne * quantity, date, trader);
                }
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

        Trader* getTrader(){
            return this->trader;
        }
};

class Transaction{
    private:
        int quantity;
        double pricePerOne;
        double totalPrice;
        time_t date;
        Trader* seller;
        Trader* buyer;
    public:
        Transaction(int quantity, double pricePerOne, time_t timestamp, Trader* seller, Trader* buyer){
            this->quantity = quantity;
            this->pricePerOne = pricePerOne;
            this->totalPrice = pricePerOne*quantity;
            this->date = timestamp;
            this->seller = seller;
            this->buyer = buyer;
        }

        string serialize() const {
            return format("{} {} {} {} {}", quantity, pricePerOne, date, seller->getUsername(), buyer->getUsername());
        }

        static unique_ptr<Transaction> deserialize(const string& data, TraderBase& traderBase) {
            stringstream ss(data);
            int quantity;
            double pricePerOne;
            time_t date;
            string sellerName, buyerName;
            if (ss >> quantity >> pricePerOne >> date >> sellerName >> buyerName) {
                Trader* seller = traderBase.getTraderByUsername(sellerName);
                Trader* buyer = traderBase.getTraderByUsername(buyerName);
                if (seller && buyer){
                    return make_unique<Transaction>(quantity, pricePerOne, date, seller, buyer);
                }
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

        Trader* getSeller(){
            return this->seller;
        }

        Trader* getBuyer(){
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
            ofstream file;
            file.open(filename);
            for (const auto& tx : txList) {
                file << tx->serialize() << endl;
            }
            file.close();
        }

        void loadFromFile(const string& filename, TraderBase& traderBase) {
            ifstream file;
            file.open(filename);
            string line;
            while (getline(file, line)) {
                auto tx = Transaction::deserialize(line, traderBase);
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
            ofstream file;
            file.open(filename);
            for (const auto& order : sellOrders) {
                file << "sell " << order->serialize() << endl;
            }
            for (const auto& order : buyOrders) {
                file << "buy " << order->serialize() << endl;
            }
            file.close();
        }

        void loadFromFile(const string& filename, TraderBase& traderBase) {
            ifstream file(filename);
            string line;
            while (getline(file, line)) {
                stringstream ss(line);
                string type, orderData;
                ss >> type;
                getline(ss, orderData);
                auto order = Order::deserialize(orderData, traderBase);
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

int main(){
    TraderBase traderBase;
    traderBase.loadFromFile("traders.txt");

    OrderBook orderBook;
    TransactionList txList;

    orderBook.loadFromFile("orders.txt", traderBase);
    txList.loadFromFile("transactions.txt", traderBase);

    string type, username, password, inputLine;
    double totalPrice;
    int quantity;
    CommandType commandType;
    Trader* currentTrader = nullptr;

    while(true){
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
        auto now = chrono::system_clock::now();
        time_t timestamp = chrono::system_clock::to_time_t(now);
        if (commandType == CommandType::EXIT)
            break;
        if (!currentTrader){
            if (commandType != CommandType::REGISTER && commandType != CommandType::LOGIN){
                cout << "You are not logged in! Please type register or login to continue: " << endl;;
            }
            else if(commandType == CommandType::REGISTER){
                if (!(ss >> username >> password)) {
                    cout << "Error: Invalid input. Please provide your username and password to register on the same line." << endl;
                }
                currentTrader = traderBase.addTrader(username, password);
                if (!currentTrader){
                    cout << "Username is already used!" << endl;
                }
                else{
                    cout << format("Welcome {}", username) << endl;
                }
            }
            else{
                if (!(ss >> username >> password)) {
                    cout << "Error: Invalid input. Please provide your username and password to login on the same line." << endl;
                }
                currentTrader = traderBase.authenticate(username, password);
                if (!currentTrader){
                    cout << "Username or password are incorrect!" << endl;
                }
                else{
                    cout << format("Welcome {}!", username) << endl;
                }
            }
        }
        else if (commandType == CommandType::REGISTER || commandType == CommandType::LOGIN){
            cout << "You are already logged in. Please logout if you want to register/login" << endl;
        }
        else if (commandType == CommandType::LOGOUT){
            currentTrader = nullptr;
        }
        else if (commandType == CommandType::TXLIST){
            if(txList.getSize()){
                for(int i = 0; i < txList.getSize(); i++){
                    Transaction* tx = txList.getAt(i);
                    auto txDate = tx->getDate();
                    cout << "Quantity: " << tx->getQuantity() << " | Total Price: " << tx->getTotalPrice() << " | Date: " << ctime(&txDate) << " | Buyer: " << tx->getBuyer()->getUsername() << " | Seller: " << tx->getSeller()->getUsername() << endl;
                }
            }
            else cout << "No available transactions!" << endl;
        }
        else if(commandType == CommandType::ORDERS){
            if(orderBook.getFrontBuyOrder() != nullptr){
                cout << "Top Buy Order: " << endl;
                Order* order = orderBook.getFrontBuyOrder();
                auto orderDate = order->getDate();
                cout << order->getQuantity() << " " << order->getPricePerOne() << " " << order->getTrader()->getUsername() << " " << ctime(&orderDate) << endl;
            }
            else{
                cout << "No Buy Orders!" << endl;
            }

            if(orderBook.getFrontSellOrder() != nullptr){
                cout << "Top Sell Order: " << endl;
                Order* order = orderBook.getFrontSellOrder();
                auto orderDate = order -> getDate();
                cout << order->getQuantity() << " " << order->getPricePerOne() << " " << order->getTrader()->getUsername() << " " << ctime(&orderDate) << endl;
            }
            else{
                cout << "No Sell Orders!" << endl;
            }
        }   
        else{
            if (!(ss >> totalPrice >> quantity)) {
                cout << "Error: Invalid input. Please provide command, total price and quantity on the same line." << endl;
                continue;
            }

            if (quantity <= 0){
                cout << "Quantity must be greater than 0." << endl;
            }
            auto newOrder = make_unique<Order>(quantity, totalPrice, timestamp, currentTrader);

            if(commandType == CommandType::BUY){
                try{
                    currentTrader->changeBalance(totalPrice);
                }
                catch(runtime_error& e){
                    cerr << e.what() << endl;
                    continue;
                }
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
                now = chrono::system_clock::now();
                timestamp = chrono::system_clock::to_time_t(now);
                if(minSellOrder->getQuantity() == maxBuyOrder->getQuantity()){
                    auto tx = make_unique<Transaction>(minSellOrder->getQuantity(), minSellOrder->getPricePerOne(), timestamp, minSellOrder->getTrader(), maxBuyOrder->getTrader());
                    minSellOrder->getTrader()->changeBalance(0.0 - minSellOrder->getPricePerOne() * minSellOrder->getQuantity());
                    txList.addTransaction(std::move(tx));
                    orderBook.popBuyOrder();
                    orderBook.popSellOrder();
                }
                else if(minSellOrder->getQuantity() > maxBuyOrder->getQuantity()){
                    auto tx = make_unique<Transaction>(maxBuyOrder->getQuantity(), minSellOrder->getPricePerOne(), timestamp, minSellOrder->getTrader(), maxBuyOrder->getTrader());
                    minSellOrder->getTrader()->changeBalance(0.0 - minSellOrder->getPricePerOne() * maxBuyOrder->getQuantity());
                    txList.addTransaction(std::move(tx));
                    minSellOrder->changeQuantity(maxBuyOrder->getQuantity());
                    orderBook.popBuyOrder();
                }
                else{
                    auto tx = make_unique<Transaction>(minSellOrder->getQuantity(), minSellOrder->getPricePerOne(), timestamp, minSellOrder->getTrader(), maxBuyOrder->getTrader());
                    minSellOrder->getTrader()->changeBalance(0.0 - minSellOrder->getPricePerOne() * minSellOrder->getQuantity());
                    txList.addTransaction(std::move(tx));
                    maxBuyOrder->changeQuantity(minSellOrder->getQuantity());
                    orderBook.popSellOrder();
                }
            }
        }
    }

    traderBase.saveToFile("traders.txt");
    orderBook.saveToFile("orders.txt");
    txList.saveToFile("transactions.txt");

    return 0;
}