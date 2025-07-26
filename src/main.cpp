#include "mqtt_broker.hpp"
#include "json_parser.hpp"
#include "logger.hpp"

int main() {
    BrokerConfig config = JsonParser::loadConfig("config.json");


    Logger::log(LEVEL::INFO, "MQTT Broker starting...");
    Logger::log(LEVEL::INFO, "Port:%d ", config.port);
    Logger::log(LEVEL::INFO, "Max Clients:%d ", config.maxClients);


    MqttBroker broker(config.port);
    broker.start();
    return 0;
}
