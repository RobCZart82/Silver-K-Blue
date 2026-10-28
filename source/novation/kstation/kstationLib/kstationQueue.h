#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace kstation
{
    // One producer and one consumer; neither side allocates or waits. A batch
    // publishes atomically so an overflowing MIDI dump cannot reach the guest
    // with its end missing. Reset only while both endpoints are stopped.
    template<typename T, size_t Capacity> class Queue
    {
        static_assert(Capacity && (Capacity & (Capacity - 1)) == 0, "Power-of-two capacity required");
        static_assert(std::is_trivially_copyable<T>::value, "Queue items must not allocate");
    public:
        bool push(const T& item) { return push(&item, 1); }
        bool push(const T* items, size_t count)
        {
            return pushGenerated(count, [items](size_t i) { return items[i]; });
        }

        template<typename Generator> bool pushGenerated(size_t count, Generator generate)
        {
            const auto write = m_write.load(std::memory_order_relaxed);
            const auto read = m_read.load(std::memory_order_acquire);
            if (count > Capacity - (write - read)) return false;
            for (size_t i = 0; i < count; ++i) m_data[(write + i) & (Capacity - 1)] = generate(i);
            m_write.store(write + count, std::memory_order_release);
            return true;
        }

        bool peek(T& item) const
        {
            return peekAt(0, item);
        }

        bool peekAt(size_t offset, T& item) const
        {
            const auto read = m_read.load(std::memory_order_relaxed);
            if (offset >= m_write.load(std::memory_order_acquire) - read) return false;
            item = m_data[(read + offset) & (Capacity - 1)];
            return true;
        }

        bool pop(T& item)
        {
            const auto read = m_read.load(std::memory_order_relaxed);
            if (read == m_write.load(std::memory_order_acquire)) return false;
            item = m_data[read & (Capacity - 1)];
            m_read.store(read + 1, std::memory_order_release);
            return true;
        }

        size_t size() const
        {
            const auto read = m_read.load(std::memory_order_acquire);
            return m_write.load(std::memory_order_acquire) - read;
        }

        void reset()
        {
            m_read.store(0, std::memory_order_relaxed);
            m_write.store(0, std::memory_order_relaxed);
        }

    private:
        std::array<T, Capacity> m_data{};
        alignas(64) std::atomic<size_t> m_read{0};
        alignas(64) std::atomic<size_t> m_write{0};
    };
}
