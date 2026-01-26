#pragma once
#include <cstdint>
#include <chrono>
#include <thread>
#include <atomic>
#include <cstdio>
#include <ctime>

class TscClock {
private:
    std::atomic<bool> stop_flag_;
    uint64_t interval_ms_;
    std::thread bg_thread_;
    uint64_t base_tsc_ = 0;
    uint64_t base_ns_ = 0;
    double ns_per_cycle_ = 0.0;

    void recalibrate_loop() {
        while (!stop_flag_.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms_));
            calibrate();
        }
    }

    void calibrate() {
        using namespace std::chrono;
        auto t1 = steady_clock::now();
        uint64_t c1 = rdtscp();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        uint64_t c2 = rdtscp();
        auto t2 = steady_clock::now();
        ns_per_cycle_ = duration<double, std::nano>(t2 - t1).count() / (c2 - c1);
        base_tsc_ = c2;
        base_ns_ = duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
    }

public:
    explicit TscClock(uint64_t interval_ms = 10000) : stop_flag_(false), interval_ms_(interval_ms) {
        calibrate();
        bg_thread_ = std::thread([this]() { recalibrate_loop(); });
    }

    ~TscClock() {
        stop_flag_.store(true);
        if (bg_thread_.joinable()) bg_thread_.join();
    }

    TscClock(const TscClock&) = delete;
    TscClock& operator=(const TscClock&) = delete;

    uint64_t now_ns() const noexcept {
        uint64_t cycles = rdtscp();
        return base_ns_ + static_cast<uint64_t>((cycles - base_tsc_) * ns_per_cycle_);
    }

    static uint64_t rdtscp() noexcept {
        unsigned int tmp;
        return __builtin_ia32_rdtscp(&tmp);
    }

    static std::string format_ns(uint64_t ns) {
        time_t sec = ns / 1000000000ULL;
        uint32_t nsec = ns % 1000000000ULL;
        struct tm tmval;
        localtime_r(&sec, &tmval);
        char timebuf[32];
        std::snprintf(timebuf, sizeof(timebuf), "[%02d:%02d:%02d.%09u]",
            tmval.tm_hour, tmval.tm_min, tmval.tm_sec, nsec);
        return std::string(timebuf);
    }
};
