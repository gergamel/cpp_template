#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace net {

struct UdpPacket {
    std::vector<uint8_t> data;
    std::string from;
    uint16_t port = 0;
};

class UdpSocket {
public:
#ifdef _WIN32
    using socket_type = SOCKET;
    static constexpr socket_type invalid_socket = INVALID_SOCKET;
#else
    using socket_type = int;
    static constexpr socket_type invalid_socket = -1;
#endif

    UdpSocket();
    ~UdpSocket();

    bool open();
    bool bind(uint16_t port);
    bool sendTo(std::string const& host, uint16_t port, std::vector<uint8_t> const& payload);
    std::optional<UdpPacket> receiveOne(int timeoutMs = 500);

private:
    socket_type sockfd_ = invalid_socket;
};

} // namespace net
