#include <sched.h>
#include <cstring>
#include <vector>
#include <thread>

#include <boost/asio/ip/tcp.hpp>
#include <fmt/core.h>

#include "../common/market_message.h"
#include "../common/tsc_clock.h"

constexpr char SERVER_ADDR[] = "127.0.0.1";
constexpr uint16_t SERVER_PORT = 9000;
constexpr int CONSUMER_TCP_CPU = 2;

void pin_to_cpu(int cpu) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);
}

int main() {
    pin_to_cpu(CONSUMER_TCP_CPU);
    fmt::print("[CONSUMER_TCP] Pinned to CPU {}\n", CONSUMER_TCP_CPU);

    boost::asio::io_context io_ctx;
    boost::asio::ip::tcp::socket socket(io_ctx);
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address(SERVER_ADDR), SERVER_PORT);

    boost::system::error_code ec;
    while (true) {
        socket.connect(endpoint, ec);
        if (!ec) {
            fmt::print("[CONSUMER_TCP] Connected to {}:{}\n", SERVER_ADDR, SERVER_PORT);
            break;
        }
        fmt::print("[CONSUMER_TCP] Connect error: {}. Retrying...\n", ec.message());
        std::this_thread::sleep_for(std::chrono::seconds(1));
        socket.close();
        socket = boost::asio::ip::tcp::socket(io_ctx);
    }

    socket.set_option(boost::asio::ip::tcp::no_delay(true));

    std::vector<uint8_t> buf;
    buf.reserve(4096);

    while (true) {
        uint8_t temp[1024];
        size_t n = socket.read_some(boost::asio::buffer(temp, sizeof(temp)), ec);
        if (ec) {
            fmt::print("[ERROR] TCP read error: {}\n", ec.message());
            return 1;
        }

        buf.insert(buf.end(), temp, temp + n);

        while (buf.size() >= sizeof(uint32_t)) {
            uint32_t msg_size = 0;
            std::memcpy(&msg_size, buf.data(), sizeof(uint32_t));

            if (buf.size() < sizeof(uint32_t) + msg_size) {
                break;
            }

            MarketMessage msg;
            std::memcpy(&msg, buf.data() + sizeof(uint32_t), sizeof(MarketMessage));

            std::string ts = TscClock::format_ns(msg.timestamp_ns);
            fmt::print("{} {} BID={:.2f} ASK={:.2f}\n", ts, msg.instrument, msg.bid, msg.ask);

            buf.erase(buf.begin(), buf.begin() + sizeof(uint32_t) + msg_size);
        }
    }

    return 0;
}
