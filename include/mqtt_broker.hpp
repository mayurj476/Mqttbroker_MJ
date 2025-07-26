#pragma once

#include <string>
#include <thread>
#include <vector>
#include <netinet/in.h>
#include <unordered_map>
#include <unordered_set>
#include <mutex>


class MqttBroker {
public:
    MqttBroker(int port);
    void start();
    virtual ~MqttBroker();

private:
    void handleClient(int clientSock);
    bool processPacket(int clientSock);
    void sendConnack(int clientSock);
    void logPublish(int clientSock, const std::string& topic, const std::string& message);



    void handlePublish(int clientSock, const std::vector<uint8_t>& buffer, int bytes);
    void handleSubscribe(int clientSock, const std::vector<uint8_t>& buffer);
    void forwardToSubscribers(const std::string& topic, const std::string& message, int excludeSock = -1);
    
    int serverSock;
    int port;
    std::unordered_map<std::string, std::unordered_set<int>> topicSubscribers;
    std::mutex subMutex;
};
