#pragma once

#include "SPSC.hpp"
#include <thread>

template<class T>
class mpsc {
public:
    mpsc(size_t _num_threads) {
        size = _num_threads;
        for (int i = 0; i < _num_threads; i++) {
            queues.push_back(std::make_unique<rigtorp::SPSCQueue<T>>(256));
        }
    }

    void enqueue(const T& _data, size_t _t_id) noexcept {
        queues[_t_id]->push(_data);
    }

    T* peek(size_t id) noexcept {
        T* front = queues[id]->front();
        return front;
    }

    void pop(size_t id) noexcept {
        T* front = queues[id]->front();
        if (front != nullptr) {
            queues[id]->pop();
        }
    }

    T dequeue() noexcept {
        T* front = nullptr;
        size_t i = 0;

        while(!(front = peek(i))) {
			++i;
            i %= size;
		}

        T ret = std::move(*front);
        pop(i);
        return ret;
    }
private:
    std::vector<std::unique_ptr<rigtorp::SPSCQueue<T>>> queues;
    size_t size;
};
