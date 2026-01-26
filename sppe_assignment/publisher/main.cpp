#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sched.h>
#include <cstring>
#include <random>
#include <vector>
#include <thread>

#include <boost/asio/ip/tcp.hpp>
#include <fmt/core.h>

#include "../common/ring_buffer.h"
#include "../common/market_message.h"
#include "../common/tsc_clock.h"

constexpr size_t RING_CAPACITY = 1024;
constexpr char SHM_NAME[] = "/market_data_shm";
constexpr uint16_t TCP_PORT = 9000;
constexpr int PUBLISHER_CPU = 0;

using RingBufferType = RingBuffer<MarketMessage, RING_CAPACITY>;
constexpr size_t SHM_SIZE = sizeof(RingBufferType);

void pin_to_cpu(int cpu) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);
}

class DataGenerator {
public:
    DataGenerator() : instruments_{"RELIANCE", "TCS", "INFY", "HDFCBANK", "ICICIBANK"},
                      rng_(std::random_device{}()),
                      price_dist_(1000.0, 3000.0),
                      spread_dist_(0.01, 0.5),
                      instrument_dist_(0, instruments_.size() - 1) {}

    MarketMessage generate(TscClock& clock) {
        MarketMessage msg{};
        msg.set_instrument(instruments_[instrument_dist_(rng_)].c_str());
        double mid = price_dist_(rng_);
        double spread = spread_dist_(rng_);
        msg.bid = mid - spread / 2.0;
        msg.ask = mid + spread / 2.0;
        msg.timestamp_ns = clock.now_ns();
        return msg;
    }

private:
    std::vector<std::string> instruments_;
    std::mt19937 rng_;
    std::uniform_real_distribution<double> price_dist_;
    std::uniform_real_distribution<double> spread_dist_;
    std::uniform_int_distribution<size_t> instrument_dist_;
};

int main() {
    pin_to_cpu(PUBLISHER_CPU);
    fmt::print("[PUBLISHER] Pinned to CPU {}\n", PUBLISHER_CPU);

    shm_unlink(SHM_NAME);
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd < 0) {
        fmt::print("[ERROR] Failed to create shm: {}\n", strerror(errno));
        return 1;
    }

    if (ftruncate(shm_fd, SHM_SIZE) < 0) {
        fmt::print("[ERROR] Failed to truncate shm: {}\n", strerror(errno));
        close(shm_fd);
        return 1;
    }

    void* shm_ptr = mmap(nullptr, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        fmt::print("[ERROR] Failed to mmap: {}\n", strerror(errno));
        close(shm_fd);
        return 1;
    }

    auto* ring = new (shm_ptr) RingBufferType();
    fmt::print("[PUBLISHER] Shared memory initialized\n");

    boost::asio::io_context io_ctx;
    boost::asio::ip::tcp::acceptor acceptor(io_ctx, {boost::asio::ip::tcp::v4(), TCP_PORT});
    fmt::print("[PUBLISHER] TCP server listening on port {}\n", TCP_PORT);

    std::optional<boost::asio::ip::tcp::socket> tcp_client;

    std::thread accept_thread([&]() {
        while (true) {
            boost::asio::ip::tcp::socket socket(io_ctx);
            boost::system::error_code ec;
            acceptor.accept(socket, ec);
            if (!ec) {
                socket.set_option(boost::asio::ip::tcp::no_delay(true));
                tcp_client = std::move(socket);
                fmt::print("[PUBLISHER] TCP client connected\n");
            }
        }
    });
    accept_thread.detach();

    TscClock clock;
    DataGenerator generator;

    while (true) {
        MarketMessage msg = generator.generate(clock);

        while (!ring->push(msg)) {
            std::this_thread::yield();
        }

        if (tcp_client && tcp_client->is_open()) {
            uint32_t msg_size = sizeof(MarketMessage);
            boost::system::error_code ec;
            boost::asio::write(*tcp_client, boost::asio::buffer(&msg_size, sizeof(msg_size)), ec);
            if (!ec) {
                boost::asio::write(*tcp_client, boost::asio::buffer(&msg, sizeof(msg)), ec);
            }
            if (ec) {
                fmt::print("[PUBLISHER] TCP write error: {}\n", ec.message());
                tcp_client->close();
                tcp_client.reset();
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    munmap(shm_ptr, SHM_SIZE);
    close(shm_fd);
    shm_unlink(SHM_NAME);
    return 0;
}
