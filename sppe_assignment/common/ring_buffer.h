#pragma once
#include <atomic>
#include <cstddef>

constexpr size_t kCacheLineSize = 64;

template <typename T, size_t Capacity>
struct alignas(kCacheLineSize) RingBuffer {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two");

    alignas(kCacheLineSize) std::atomic<size_t> write_idx{0};
    alignas(kCacheLineSize) std::atomic<size_t> read_idx{0};
    alignas(kCacheLineSize) T buffer[Capacity];

    bool push(const T& item) {
        size_t w = write_idx.load(std::memory_order_relaxed);
        size_t next = (w + 1) & (Capacity - 1);
        if (next == read_idx.load(std::memory_order_acquire)) return false;
        buffer[w] = item;
        write_idx.store(next, std::memory_order_release);
        return true;
    }

    bool pop(T& item) {
        size_t r = read_idx.load(std::memory_order_relaxed);
        if (r == write_idx.load(std::memory_order_acquire)) return false;
        item = buffer[r];
        read_idx.store((r + 1) & (Capacity - 1), std::memory_order_release);
        return true;
    }

    size_t size() const {
        size_t w = write_idx.load(std::memory_order_acquire);
        size_t r = read_idx.load(std::memory_order_acquire);
        return (w + Capacity - r) & (Capacity - 1);
    }

    constexpr size_t capacity() const { return Capacity - 1; }

    bool empty() const {
        return write_idx.load(std::memory_order_acquire) == read_idx.load(std::memory_order_acquire);
    }

    bool full() const {
        return ((write_idx.load(std::memory_order_acquire) + 1) & (Capacity - 1)) == read_idx.load(std::memory_order_acquire);
    }
};
