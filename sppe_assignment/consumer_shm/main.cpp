#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sched.h>
#include <cstring>
#include <thread>

#include <fmt/core.h>

#include "../common/ring_buffer.h"
#include "../common/market_message.h"
#include "../common/tsc_clock.h"

constexpr size_t RING_CAPACITY = 1024;
constexpr char SHM_NAME[] = "/market_data_shm";
constexpr int CONSUMER_SHM_CPU = 1;

using RingBufferType = RingBuffer<MarketMessage, RING_CAPACITY>;
constexpr size_t SHM_SIZE = sizeof(RingBufferType);

void pin_to_cpu(int cpu) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);
}

int main() {
    pin_to_cpu(CONSUMER_SHM_CPU);
    fmt::print("[CONSUMER_SHM] Pinned to CPU {}\n", CONSUMER_SHM_CPU);

    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd < 0) {
        fmt::print("[ERROR] Failed to open shm: {}\n", strerror(errno));
        return 1;
    }

    void* shm_ptr = mmap(nullptr, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        fmt::print("[ERROR] Failed to mmap: {}\n", strerror(errno));
        close(shm_fd);
        return 1;
    }

    auto* ring = reinterpret_cast<RingBufferType*>(shm_ptr);
    fmt::print("[CONSUMER_SHM] Connected to shared memory\n");

    MarketMessage msg;
    while (true) {
        if (ring->pop(msg)) {
            std::string ts = TscClock::format_ns(msg.timestamp_ns);
            fmt::print("{} {} BID={:.2f} ASK={:.2f}\n", ts, msg.instrument, msg.bid, msg.ask);
        } else {
            std::this_thread::yield();
        }
    }

    munmap(shm_ptr, SHM_SIZE);
    close(shm_fd);
    return 0;
}
