// mqtt_utils.hpp
#pragma once

#include <cstdint>
#include <vector>
#include <cstring>
#include <string>

inline uint16_t get_uint16(const std::vector<uint8_t>& data, size_t offset) {
    return (static_cast<uint16_t>(data[offset]) << 8) | data[offset + 1];
}

inline uint8_t get_uint8(const std::vector<uint8_t>& data, size_t offset) {
    return data[offset];
}

inline void put_uint16(std::vector<uint8_t>& data, size_t offset, uint16_t value) {
    data[offset] = static_cast<uint8_t>((value >> 8) & 0xFF);
    data[offset + 1] = static_cast<uint8_t>(value & 0xFF);
}

inline void put_string(std::vector<uint8_t>& data, const std::string& str) {
    data.push_back((str.size() >> 8) & 0xFF);
    data.push_back(str.size() & 0xFF);
    data.insert(data.end(), str.begin(), str.end());
}

inline std::string get_string(const std::vector<uint8_t>& data, size_t& offset) {
    uint16_t length = get_uint16(data, offset);
    offset += 2;
    std::string result(data.begin() + offset, data.begin() + offset + length);
    offset += length;
    return result;
}
