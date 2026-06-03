#include "net/UdpSocket.hpp"
#include <gtest/gtest.h>

TEST(UdpSocket, SendAndReceiveLoopback)
{
    net::UdpSocket server;
    ASSERT_TRUE(server.open());
    ASSERT_TRUE(server.bind(34567));

    net::UdpSocket client;
    ASSERT_TRUE(client.open());

    const std::vector<uint8_t> payload = {'h', 'e', 'l', 'l', 'o'};
    ASSERT_TRUE(client.sendTo("127.0.0.1", 34567, payload));

    const auto packet = server.receiveOne(1000);
    ASSERT_TRUE(packet.has_value());
    EXPECT_EQ(packet->data, payload);
    EXPECT_EQ(packet->from, "127.0.0.1");
    EXPECT_NE(packet->port, 0u);
}
