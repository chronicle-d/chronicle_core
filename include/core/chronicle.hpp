#ifndef CHRONICLE_H
#define CHRONICLE_H
#include <vector>
#include <string>
#include "core/config.hpp"
#include "core/device_factory.hpp"

// Device operations
std::vector<std::string> getConfig(connectionInfo ci, std::vector<OperationMap> getConfig);

#endif // CHRONICLE_H