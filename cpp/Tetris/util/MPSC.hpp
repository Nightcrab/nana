#pragma once

#include "SPSC.hpp"
#include <thread>

template<class T>
class mpsc {
public:
    mpsc(size_t num_threads) {
        size = num_threads;
        for (int i = 0; i < num_threads; i++) {
            queues.push_back(std::make_unique<rigtorp::SPSCQueue<T>>(1024));
        }
    }

    // producer function
    void enqueue(const T& data, size_t id) noexcept {
        queues[id]->push(data);
    }

    // consumer function
    T* peek(size_t id) noexcept {
        T* front = queues[id]->front();
        return front;
    }

    // consumer function
    void pop(size_t id) noexcept {
        T* front = queues[id]->front();
        if (front != nullptr) {
            queues[id]->pop();
        }
    }

    // consumer function
    T dequeue() noexcept {
        T* front = nullptr;
        size_t i = 0;

        while(!(front = peek(i))) {
			++i;
            i %= size;
		}

        T ret = std::move(*front);
        pop(i);
        return std::move(ret);
    }
private:
    std::vector<std::unique_ptr<rigtorp::SPSCQueue<T>>> queues;
    size_t size;
};
