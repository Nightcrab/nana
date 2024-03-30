/*
 * [....... [..[..[.. [..
 *        [..  [..[.    [..
 *       [..   [..[.     [..
 *     [..     [..[... [.
 *    [..      [..[.     [..
 *  [..        [..[.      [.
 * [...........[..[.... [..
 *
 *
 * MIT License
 *
 * Copyright (c) 2021 Donald-Rupin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 *
 *  @file wait_mpsc_queue.hpp
 *
 */

#ifndef ZIB_WAIT_MPSC_QUEUE_HPP_
#define ZIB_WAIT_MPSC_QUEUE_HPP_

#include <assert.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace zib {

namespace wait_details {

/* Shamelssly takend from
 * https://en.cppreference.com/w/cpp/thread/hardware_destructive_interference_size
 * as in some c++ libraries it doesn't exists
 */
#ifdef __cpp_lib_hardware_interference_size
using std::hardware_constructive_interference_size;
using std::hardware_destructive_interference_size;
#else
// 64 bytes on x86-64 │ L1_CACHE_BYTES │ L1_CACHE_SHIFT │ __cacheline_aligned │
// ...
constexpr std::size_t hardware_constructive_interference_size =
    2 * sizeof(std::max_align_t);
constexpr std::size_t hardware_destructive_interference_size = 2 * sizeof(std::max_align_t);
#endif

template <typename Dec, typename F>
concept Deconstructor = requires(const Dec _dec, F* _ptr) {
    {
        _dec(_ptr)
    } noexcept -> std::same_as<void>;
    {
        Dec{}
    } noexcept -> std::same_as<Dec>;
};

template <typename T>
struct deconstruct_noop {
    void
    operator()(T*) const noexcept {};
};

static constexpr std::size_t kDefaultMPSCSize = 4096;
static constexpr std::size_t kDefaultMPSCAllocationBufferSize = 16;

}  // namespace wait_details

template <
    typename T,
    wait_details::Deconstructor<T> F = wait_details::deconstruct_noop<T>,
    std::size_t BufferSize = wait_details::kDefaultMPSCSize,
    std::size_t AllocationSize = wait_details::kDefaultMPSCAllocationBufferSize>
class wait_mpsc_queue {
   private:
    static constexpr auto kEmpty = std::numeric_limits<std::size_t>::max();

    static constexpr auto kAlignment = wait_details::hardware_destructive_interference_size;

    struct alignas(kAlignment) node {
        node() : count_(kEmpty) {}

        T data_;

        std::atomic<std::uint64_t> count_;
    };

    struct alignas(kAlignment) node_buffer {
        node_buffer() : read_head_(0), next_(nullptr), elements_{}, write_head_(0) {}

        std::size_t read_head_ alignas(kAlignment);

        node_buffer* next_ alignas(kAlignment);

        node elements_[BufferSize];

        std::size_t write_head_ alignas(kAlignment);
    };

    struct alignas(kAlignment) allocation_pool {
        std::atomic<std::uint64_t> read_count_ alignas(kAlignment);

        std::atomic<std::uint64_t> write_count_ alignas(kAlignment);

        struct alignas(kAlignment) aligned_ptr {
            node_buffer* ptr_;
        };

        aligned_ptr items_[AllocationSize];

        void
        push(node_buffer* _ptr) {
            auto write_idx = write_count_.load(std::memory_order_relaxed);

            if (write_idx + 1 == read_count_.load(std::memory_order_relaxed)) {
                delete _ptr;
                return;
            }

            _ptr->~node_buffer();
            new (_ptr) node_buffer();

            items_[write_idx].ptr_ = _ptr;

            if (write_idx + 1 != AllocationSize) {
                write_count_.store(write_idx + 1, std::memory_order_release);
            } else {
                write_count_.store(0, std::memory_order_release);
            }
        }

        node_buffer*
        pop() {
            auto read_idx = read_count_.load(std::memory_order_relaxed);
            if (read_idx == write_count_.load(std::memory_order_acquire)) {
                return new node_buffer;
            }

            auto tmp = items_[read_idx].ptr_;

            if (read_idx + 1 != AllocationSize) {
                read_count_.store(read_idx + 1, std::memory_order_release);
            } else {
                read_count_.store(0, std::memory_order_release);
            }

            return tmp;
        }

        node_buffer*
        drain() {
            auto read_idx = read_count_.load(std::memory_order_relaxed);
            if (read_idx == write_count_.load(std::memory_order_relaxed)) {
                return nullptr;
            }

            auto tmp = items_[read_idx].ptr_;

            if (read_idx + 1 != AllocationSize) {
                read_count_.store(read_idx + 1, std::memory_order_relaxed);
            } else {
                read_count_.store(0, std::memory_order_relaxed);
            }

            return tmp;
        }
    };

   public:
    using value_type = T;
    using deconstructor_type = F;

    wait_mpsc_queue(std::uint64_t _num_threads)
        : heads_(_num_threads), lowest_seen_(0), sleeping_(false), tails_(_num_threads), up_to_(0), buffers_(_num_threads) {
        for (std::size_t i = 0; i < _num_threads; ++i) {
            auto* buf = new node_buffer;
            heads_[i] = buf;
            tails_[i] = buf;
        }
    }

    ~wait_mpsc_queue() {
        deconstructor_type t;

        for (auto h : heads_) {
            while (h) {
                for (std::size_t i = h->read_head_; i < BufferSize; ++i) {
                    if (h->elements_[i].count_.load() != kEmpty) {
                        t(&h->elements_[i].data_);
                    } else {
                        break;
                    }
                }

                auto tmp = h->next_;
                delete h;
                h = tmp;
            }
        }

        for (auto& q : buffers_) {
            node_buffer* to_delete = nullptr;
            while ((to_delete = q.drain())) {
                delete to_delete;
            }
        }
    }

    void
    enqueue(T _data, std::uint16_t _t_id) noexcept {
        auto* buffer = tails_[_t_id];
        if (buffer->write_head_ == BufferSize - 1) {
            tails_[_t_id] = buffers_[_t_id].pop();
            buffer->next_ = tails_[_t_id];
            assert(tails_[_t_id]);
        }

        auto cur = up_to_.fetch_add(1, std::memory_order_release);

        buffer->elements_[buffer->write_head_].data_ = _data;

        buffer->elements_[buffer->write_head_++].count_.store(
            cur,
            std::memory_order_release);

        if (sleeping_.load(std::memory_order_acquire) == true) {
            up_to_.notify_one();
        }
    }

    T dequeue() noexcept {
        while (true) {
            std::int64_t prev_index = -2;
            while (true) {
                auto min_count = kEmpty;
                std::int64_t min_index = -1;

                for (std::uint64_t i = 0; i < heads_.size(); ++i) {
                    assert(heads_[i]->read_head_ < BufferSize);

                    auto count = heads_[i]->elements_[heads_[i]->read_head_].count_.load(
                        std::memory_order_acquire);

                    if (count < min_count) {
                        min_count = count;
                        min_index = i;
                        if (min_count == lowest_seen_) {
                            prev_index = i;
                            break;
                        }
                    }
                }

                if (min_index == -1 && prev_index == min_index) {
                    if (up_to_.load(std::memory_order_relaxed) == lowest_seen_) {
                        sleeping_.store(true, std::memory_order_release);
                        up_to_.wait(lowest_seen_, std::memory_order_acquire);
                        sleeping_.store(false, std::memory_order_relaxed);
                    }
                    break;
                }

                if (prev_index == min_index) {
                    auto data =
                        heads_[min_index]->elements_[heads_[min_index]->read_head_++].data_;

                    if (heads_[min_index]->read_head_ == BufferSize) {
                        auto tmp = heads_[min_index];
                        heads_[min_index] = tmp->next_;

                        assert(heads_[min_index]);

                        buffers_[min_index].push(tmp);
                    }

                    if (lowest_seen_ == min_count) {
                        lowest_seen_++;
                    }

                    return data;
                }

                prev_index = min_index;
            }
        }
    }

   private:
    std::vector<node_buffer*> heads_ alignas(kAlignment);
    std::size_t lowest_seen_;

    std::atomic<bool> sleeping_ alignas(kAlignment);

    std::vector<node_buffer*> tails_ alignas(kAlignment);
    std::atomic<std::uint64_t> up_to_ alignas(kAlignment);

    std::vector<allocation_pool> buffers_ alignas(kAlignment);
    char padding_[kAlignment - sizeof(buffers_)];
};

}  // namespace zib

#endif /* ZIB_WAIT_MPSC_QUEUE_HPP_ */