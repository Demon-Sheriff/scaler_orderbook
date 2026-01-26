#pragma once
#include <cstdint>
#include <cstring>

struct MarketMessage {
    char instrument[16];
    double bid;
    double ask;
    uint64_t timestamp_ns;

    void set_instrument(const char* name) {
        std::strncpy(instrument, name, sizeof(instrument) - 1);
        instrument[sizeof(instrument) - 1] = '\0';
    }
};

static_assert(sizeof(MarketMessage) == 40, "MarketMessage must be 40 bytes");
