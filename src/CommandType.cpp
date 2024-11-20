#include "CommandType.h"

CommandType getOrderTypeFromString(const std::string& type) {
    if (type == "buy") return CommandType::BUY;
    if (type == "sell") return CommandType::SELL;
    if (type == "txlist") return CommandType::TXLIST;
    if (type == "exit") return CommandType::EXIT;
    throw std::invalid_argument("Invalid command: " + type);
}