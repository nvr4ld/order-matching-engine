#ifndef TRADERBASE_H
#define TRADERBASE_H

#include <vector>
#include <string>

class TraderBase {
public:
    void addTrader(const std::string& username);
    void saveToFile(const std::string& filename);
    void loadFromFile(const std::string& filename);

private:
    std::vector<std::string> traders;
};

#endif // TRADERBASE_H