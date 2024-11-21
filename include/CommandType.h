#ifndef COMMANDTYPE_H
#define COMMANDTYPE_H

#include <string>
#include <stdexcept>

enum class CommandType { BUY, SELL, TXLIST, EXIT };

CommandType getOrderTypeFromString(const std::string& type);

#endif