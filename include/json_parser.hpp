#pragma once
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
using namespace std;

using json = nlohmann::json;

struct BrokerConfig {
    int port ;
    int maxClients ;
    std::string logLevel ;
};

class JsonParser {
public:
    static BrokerConfig loadConfig(const std::string& path);
};