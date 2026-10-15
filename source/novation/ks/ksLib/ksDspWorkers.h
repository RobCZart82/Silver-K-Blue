#pragma once

#include <array>
#include <cstdint>
#include <memory>

namespace ks {
// Two persistent DSP executors. One coordinator submits at most one slice and
// joins BOTH results before touching H8, host ports, serial IO or snapshots.
class DspWorkers final {
public:
    using Execute = void(*)(void*, unsigned, int64_t, bool);
    DspWorkers(void* context, Execute execute, int rate, unsigned block);
    ~DspWorkers();
    DspWorkers(const DspWorkers&) = delete;
    DspWorkers& operator=(const DspWorkers&) = delete;
    void configure(int rate, unsigned block) noexcept;
    void run(const std::array<int64_t,2>& targets, bool sampled);
private:
    struct Worker;
    std::array<std::unique_ptr<Worker>,2> workers;
    int rate;
    unsigned block;
};
}
