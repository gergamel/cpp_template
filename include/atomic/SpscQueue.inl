#pragma once

#include <array>

namespace atomic {

template <typename T, size_t Capacity>
SpscQueue<T, Capacity>::SpscQueue() noexcept = default;

template <typename T, size_t Capacity>
bool SpscQueue<T, Capacity>::push(T const& value) noexcept
{
    const size_t tail = tail_.load(std::memory_order_relaxed);
    const size_t nextTail = (tail + 1) % Capacity;
    if (nextTail == head_.load(std::memory_order_acquire)) {
        return false;
    }
    buffer_[tail] = value;
    tail_.store(nextTail, std::memory_order_release);
    return true;
}

template <typename T, size_t Capacity>
std::optional<T> SpscQueue<T, Capacity>::pop() noexcept
{
    const size_t head = head_.load(std::memory_order_relaxed);
    if (head == tail_.load(std::memory_order_acquire)) {
        return std::nullopt;
    }
    T value = buffer_[head];
    head_.store((head + 1) % Capacity, std::memory_order_release);
    return value;
}

template <typename T, size_t Capacity>
bool SpscQueue<T, Capacity>::empty() const noexcept
{
    return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
}

} // namespace atomic
