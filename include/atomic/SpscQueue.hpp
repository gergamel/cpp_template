#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <optional>
#include <vector>

namespace atomic {

template <typename T, size_t Capacity>
class SpscQueue {
public:
    SpscQueue() noexcept;
    bool push(T const& value) noexcept;
    std::optional<T> pop() noexcept;
    bool empty() const noexcept;

private:
    alignas(64) std::array<T, Capacity> buffer_{};
    alignas(64) std::atomic<size_t> head_{0};
    alignas(64) std::atomic<size_t> tail_{0};
};

} // namespace atomic

#include "SpscQueue.inl"
