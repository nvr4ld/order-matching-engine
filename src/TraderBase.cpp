#include "TraderBase.h"
#include <fstream>
#include <algorithm>

void TraderBase::addTrader(const std::string& username) {
    if (std::find(traders.begin(), traders.end(), username) == traders.end()) {
        traders.push_back(username);
    }
}

void TraderBase::saveToFile(const std::string& filename) {
    std::ofstream file(filename);
    for (const auto& username : traders) {
        file << username << std::endl;
    }
}

void TraderBase::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    std::string line;
    while (std::getline(file, line)) {
        traders.push_back(line);
    }
}