#include "json_parser.hpp"
#include "logger.hpp"
BrokerConfig JsonParser::loadConfig(const std::string& path) {
    BrokerConfig cfg;
    std::ifstream file(path);
    if (!file) {
        Logger::log(LEVEL::WARNING, "Failed to open config file: ", path.c_str());
        return cfg;
    }

    json j;
    file >> j;

    if (j.contains("port")) cfg.port = j["port"];
    if (j.contains("max_clients")) cfg.maxClients = j["max_clients"];
    if (j.contains("log_level")) cfg.logLevel = j["log_level"];

    return cfg;
}
