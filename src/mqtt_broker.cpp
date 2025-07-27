#include "mqtt_broker.hpp"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <sstream>
#include "mqtt_utils.hpp"
#include "logger.hpp"

#define SUB_ACK 0x90

MqttBroker::MqttBroker(int port) : port(port), serverSock(-1) {}

void MqttBroker::start()
{
    serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock < 0)
    {
        Logger::log(LEVEL::WARNING, "Socket creation failed");
        return;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSock, (sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        Logger::log(LEVEL::WARNING, "Bind failed on port %d", port);
        return;
    }

    if (listen(serverSock, 5) < 0)
    {
        Logger::log(LEVEL::WARNING, "Listen failed on port %d", port);
        return;
    }

    Logger::log(LEVEL::INFO, "MQTT Broker started on port %d", port);

    while (true)
    {
        sockaddr_in clientAddr{};
        socklen_t len = sizeof(clientAddr);
        int clientSock = accept(serverSock, (sockaddr *)&clientAddr, &len);
        if (clientSock >= 0)
        {
            // Detaching the client handler to allow multiple clients
            std::thread(&MqttBroker::handleClient, this, clientSock).detach();
        }
    }
}

MqttBroker::~MqttBroker()
{
    if (serverSock >= 0)
    {
        close(serverSock);
        Logger::log(LEVEL::INFO, "MQTT Broker stopped");
    }
}

void MqttBroker::handleClient(int clientSock)
{
    while (true)
    {
        if (!processPacket(clientSock))
        {
            Logger::log(LEVEL::INFO, "Client disconnected : %d", clientSock);
            break;
        }
    }
    close(clientSock);
}

bool MqttBroker::processPacket(int clientSock)
{
    std::vector<uint8_t> buffer(1024);
    int bytes = recv(clientSock, buffer.data(), buffer.size(), 0);
    if (bytes <= 0)
    {
        if (bytes < 0)
        {
            Logger::log(LEVEL::WARNING, "Client disconnected or error ");
            Logger::log(LEVEL::WARNING, "Receive failed on socket %d", clientSock);
        }
        buffer.clear();
        close(clientSock);
        return false;
    }

    buffer.resize(bytes);

    uint8_t packetType = buffer[0] >> 4;
    switch (static_cast<Signal>(packetType))
    {
    case Signal::CONNECT: // CONNECT
        Logger::log(LEVEL::INFO, "Client %d connected", clientSock);
        sendConnack(clientSock);
        break;
    case Signal::PUBLISH: // PUBLISH
        handlePublish(clientSock, buffer, bytes);
        break;
    case Signal::SUBSCRIBE: // SUBSCRIBE
        handleSubscribe(clientSock, buffer);
        break;
    case Signal::DISCONNECT: // DISCONNECT
        close(clientSock);
        // Logger::log(LEVEL::INFO, "Client %d disconnected", clientSock);
        return false;
    default:
        Logger::log(LEVEL::WARNING, "Unsupported packet type: %d", packetType);
        break;
    }
    return true; // Continue processing
}

void MqttBroker::sendConnack(int clientSock)
{
    uint8_t connack[4] = {0x20, 0x02, 0x00, 0x00}; // CONNACK
    send(clientSock, connack, sizeof(connack), 0);
    Logger::log(LEVEL::INFO, "Sent CONNACK to client %d", clientSock);
}

void MqttBroker::logPublish(int, const std::string &topic, const std::string &message)
{
    Logger::log(LEVEL::INFO, "Published message to topic '%s': %s", topic.c_str(), message.c_str());
}

void MqttBroker::handlePublish(int clientSock, const std::vector<uint8_t> &buffer, int bytes)
{
    uint16_t topicLength = get_uint16(buffer, 2);
    size_t topicOffset = 4;

    std::string topic(buffer.begin() + topicOffset, buffer.begin() + topicOffset + topicLength);

    size_t payloadOffset = topicOffset + topicLength;

    std::string payload(buffer.begin() + payloadOffset, buffer.begin() + bytes);

    logPublish(clientSock, topic, payload);
    forwardToSubscribers(topic, payload, clientSock);
}

void MqttBroker::forwardToSubscribers(const std::string &topic, const std::string &message, int excludeSock)
{
    std::lock_guard<std::mutex> lock(subMutex);

    for (const auto &entry : topicSubscribers)
    {
        const std::string &subscription = entry.first;
        const std::unordered_set<int> &sockets = entry.second;

        if (matchTopic(subscription, topic))
        {
            for (int sock : sockets)
            {
                if (sock == excludeSock)
                    continue;

                std::vector<uint8_t> packet;
                std::string header = "\x30"; // PUBLISH, QoS 0
                std::string fullPayload;

                // Build payload: [topic length][topic][message]
                uint16_t len = topic.size();
                fullPayload.push_back((len >> 8) & 0xFF);
                fullPayload.push_back(len & 0xFF);
                fullPayload += topic;
                fullPayload += message;

                header += static_cast<char>(fullPayload.size());
                packet.insert(packet.end(), header.begin(), header.end());
                packet.insert(packet.end(), fullPayload.begin(), fullPayload.end());

                send(sock, packet.data(), packet.size(), 0);
            }
        }
    }
}

void MqttBroker::handleSubscribe(int clientSock, const std::vector<uint8_t> &buffer)
{
    uint16_t packetId = get_uint16(buffer, 2);

    size_t offset = 4;
    std::vector<uint8_t> returnCodes;
    while (offset + 2 < buffer.size())
    {
        uint16_t topicLen = get_uint16(buffer, offset);
        offset += 2;

        if (offset + topicLen > buffer.size())
            break;

        std::string topic(buffer.begin() + offset, buffer.begin() + offset + topicLen);
        offset += topicLen;

        uint8_t qos = buffer[offset]; // usually 0
        offset++;

        {
            std::lock_guard<std::mutex> lock(subMutex);
            topicSubscribers[topic].insert(clientSock);
        }

        Logger::log(LEVEL::INFO, "Client %d subscribed to topic '%s'", clientSock, topic.c_str());
        returnCodes.push_back(0x00); // Always grant QoS 0 for now
    }

    // Build SUBACK
    std::vector<uint8_t> suback;
    suback.push_back(SUB_ACK);                   // SUBACK
    suback.push_back(2 + returnCodes.size()); // Remaining length
    suback.push_back(static_cast<uint8_t>(packetId >> 8));
    suback.push_back(static_cast<uint8_t>(packetId & 0xFF));
    suback.insert(suback.end(), returnCodes.begin(), returnCodes.end());

    send(clientSock, suback.data(), suback.size(), 0);
}

bool MqttBroker::matchTopic(const std::string &subscription, const std::string &topic)
{
    Logger::log(LEVEL::DEBUG, "Matching subscription '%s' with topic '%s'", subscription.c_str(), topic.c_str());

    std::istringstream subStream(subscription);
    std::istringstream topicStream(topic);

    std::string subToken, topicToken;

    while (true)
    {
        bool hasSub = static_cast<bool>(std::getline(subStream, subToken, '/'));
        bool hasTopic = static_cast<bool>(std::getline(topicStream, topicToken, '/'));

        if (!hasSub && !hasTopic)
        {
            // Reached end of both, full match
            return true;
        }

        if (hasSub && subToken == "#")
        {
            return true; // Match everything after
        }

        if (!hasSub || !hasTopic)
        {
            // One stream ended before the other, not a match
            return false;
        }

        if (subToken != "+" && subToken != topicToken)
        {
            return false;
        }
    }

    return true;
}
