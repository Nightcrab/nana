#pragma once

#include "SPSC.hpp"
#include <thread>

template<class T>
class mpsc {
public:
    explicit mpsc(size_t num_threads) {
        size = num_threads;
        for (int i = 0; i < num_threads; i++) {
            queues.push_back(std::make_unique<rigtorp::SPSCQueue<T>>(1024));
        }
    }
    ~mpsc() = default;
    // non copyable or movable
    mpsc(const mpsc&) = delete;
    mpsc& operator=(const mpsc&) = delete;

    // producer function
    inline void enqueue(const T& data, size_t id) noexcept {
        queues[id]->push(data);
    }

    // consumer function
    inline T* single_peek(size_t id) noexcept {
        T* front = queues[id]->front();
        return front;
    }

    inline T* peek() noexcept {
        T* front = nullptr;

        for (int i = 0; i < size; i++) {
            front = queues[i]->front();
            if (front) {
                break;
            }
        }

        return front;
    }

    inline bool isempty() noexcept {
        return peek() == nullptr;
    }

    // consumer function
    inline void pop(size_t id) noexcept {
        T* front = queues[id]->front();
        if (front != nullptr) {
            queues[id]->pop();
        }
    }

    // consumer function
    inline T dequeue() noexcept {
        T* front = nullptr;
        size_t i = 0;
        int j = 0;

        while(!(front = single_peek(i))) {
			++i;
            ++j;
            i %= size;
		}

        T ret = std::move(*front);
        pop(i);
        return std::move(ret);
    }
    size_t size;
private:
    std::vector<std::unique_ptr<rigtorp::SPSCQueue<T>>> queues;
};
