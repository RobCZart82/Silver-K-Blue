#include "kstationQueue.h"

#include <iostream>
#include <stdexcept>
#include <thread>

namespace
{
    void require(bool value) { if (!value) throw std::runtime_error("Queue integrity failure"); }
}

int main()
{
    kstation::Queue<uint64_t, 8> queue;
    uint64_t value = 0;
    require(!queue.pop(value));
    const uint64_t first[]{10, 11, 12, 13, 14, 15, 16};
    require(queue.push(first, 7));
    const uint64_t rejected[]{20, 21};
    require(!queue.push(rejected, 2));
    for (auto expected : first) { require(queue.pop(value)); require(value == expected); }
    require(!queue.pop(value));

    constexpr uint64_t count = 1000000;
    kstation::Queue<uint64_t, 1024> concurrent;
    std::atomic<bool> valid{true};
    std::thread producer([&]
    {
        for (uint64_t i = 0; i < count; ++i)
            while (!concurrent.push(i)) std::this_thread::yield();
    });
    for (uint64_t i = 0; i < count; ++i)
    {
        while (!concurrent.pop(value)) std::this_thread::yield();
        if (value != i) valid.store(false);
    }
    producer.join();
    require(valid.load() && concurrent.size() == 0);
    std::cout << "Queue: whole-batch rejection, wraparound and one million concurrent items passed\n";
}
