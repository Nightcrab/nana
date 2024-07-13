#pragma once
#include "SPSC.hpp"

template<class T>
class mpsc {
public:
    mpsc(std::uint16_t _num_threads) {
        size = _num_threads;
        for (int i = 0; i < _num_threads; i++) {
            queues.push_back(std::make_unique<rigtorp::SPSCQueue<T>>(256));
        }
    }

    void enqueue(T _data, std::uint16_t _t_id) noexcept {
        queues[_t_id]->push(_data);
    }

    T* peek() noexcept {
        for (int id = 0; id < size; id++) {
            T* front = queues[id]->front();
            if (front != nullptr) {
                return front;
            }
        }

        return nullptr;
    }

    void pop() noexcept {
        for (int id = 0; id < size; id++) {
            T* front = queues[id]->front();
            if (front != nullptr) {
                queues[id]->pop();
            }
        }
    }

    T dequeue() noexcept {
        while (true) {
            if (peek() != nullptr) {
                break;
            }
        }
        T ret = *peek();
        pop();
        return ret;
    }

    std::vector< std::unique_ptr<rigtorp::SPSCQueue<T>>> queues;
    int size;
};