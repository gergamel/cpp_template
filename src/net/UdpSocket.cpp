#include "net/UdpSocket.hpp"

#ifdef _WIN32
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  pragma comment(lib, "Ws2_32.lib")
#else
#  include <arpa/inet.h>
#  include <netinet/in.h>
#  include <sys/select.h>
#  include <sys/socket.h>
#  include <unistd.h>
#  include <fcntl.h>
#endif

#include <cstring>
#include <type_traits>

namespace net {

namespace {

#ifdef _WIN32
struct WinSockInitializer {
    WinSockInitializer() {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
    }
    ~WinSockInitializer() {
        WSACleanup();
    }
};

static WinSockInitializer wsa_initializer;
#endif

UdpSocket::socket_type invalidSocket() noexcept {
#ifdef _WIN32
    return INVALID_SOCKET;
#else
    return -1;
#endif
}

void closeSocket(UdpSocket::socket_type sock) noexcept {
#ifdef _WIN32
    closesocket(static_cast<SOCKET>(sock));
#else
    close(sock);
#endif
}

} // namespace

UdpSocket::UdpSocket() = default;

UdpSocket::~UdpSocket()
{
    if (sockfd_ != invalidSocket()) {
        closeSocket(sockfd_);
    }
}

bool UdpSocket::open()
{
    sockfd_ = static_cast<socket_type>(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));
    return sockfd_ != invalidSocket();
}

bool UdpSocket::bind(uint16_t port)
{
    if (sockfd_ == invalidSocket() && !open()) {
        return false;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);

    return ::bind(sockfd_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0;
}

bool UdpSocket::sendTo(std::string const& host, uint16_t port, std::vector<uint8_t> const& payload)
{
    if (sockfd_ == invalidSocket() && !open()) {
        return false;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    inet_pton(AF_INET, host.c_str(), &address.sin_addr);
    address.sin_port = htons(port);

    const auto sent = sendto(sockfd_, reinterpret_cast<char const*>(payload.data()), static_cast<int>(payload.size()), 0,
                            reinterpret_cast<sockaddr const*>(&address), sizeof(address));
    using sent_t = std::remove_const_t<decltype(sent)>;
    return sent == static_cast<sent_t>(payload.size());
}

std::optional<UdpPacket> UdpSocket::receiveOne(int timeoutMs)
{
    if (sockfd_ == invalidSocket()) {
        return std::nullopt;
    }

    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(sockfd_, &readSet);

    timeval timeout{};
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;

    const int ready = select(sockfd_ + 1, &readSet, nullptr, nullptr, &timeout);
    if (ready <= 0) {
        return std::nullopt;
    }

    std::vector<uint8_t> buffer(2048);
    sockaddr_in sender{};
    socklen_t senderSize = sizeof(sender);
    const auto count = recvfrom(sockfd_, reinterpret_cast<char*>(buffer.data()), static_cast<int>(buffer.size()), 0,
                               reinterpret_cast<sockaddr*>(&sender), &senderSize);
    if (count <= 0) {
        return std::nullopt;
    }

    buffer.resize(static_cast<size_t>(count));
    char fromIp[INET_ADDRSTRLEN] = {};
    inet_ntop(AF_INET, &sender.sin_addr, fromIp, sizeof(fromIp));

    return UdpPacket{std::move(buffer), std::string(fromIp), ntohs(sender.sin_port)};
}

} // namespace net
