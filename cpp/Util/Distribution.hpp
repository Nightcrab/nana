#pragma once
#include <ranges>
#include <vector>

#include "util/rng.hpp"

template <typename T>
class Stochastic {
public:
	Stochastic(T value, float probability) {
		this->value = value;
		this->probability = probability;
	}

	T value;
	float probability = 0;
};

#ifndef __DIST_HPP
#define __DIST_HPP
namespace Distribution {

	std::vector<float> normalise(std::vector<float> vec);

	float sigmoid(float x);

	template <typename T>
	std::vector<Stochastic<T>> normalise(std::vector<Stochastic<T>> vec) {
		float sum = 0;
		for (auto el : vec) {
			sum += el.probability;
		}

		for (auto& el : vec) {
			el.probability = el.probability / sum;
		}
		return vec;
	}

	template <typename T>
	float expectation(std::vector<Stochastic<T>> vec) {
		float sum = 0;
		for (auto el : vec) {
			sum += el.probability * el.value;
		}
		return sum;
	}

	template <typename T>
	float max_value(std::vector<Stochastic<T>> vec) {
		float mx = 0;
		for (auto el : vec) {
			mx = std::max(mx, el.value);
		}
		return mx;
	}

	template <typename T>
	T sample(std::vector<Stochastic<T>> pdf, RNG &rng) {
		std::ranges::sort(pdf, [](const Stochastic<T>& a, const Stochastic<T>& b)
			{
				return a.probability > b.probability;
		});

		uint32_t ri = rng.getRand(256) % 256;
		float r = 1.0 * ri;
		r /= 1.0 * 256;
		for (auto& event : pdf) {
			r -= event.probability;
			if (r < 0) {
				return event.value;
			}
		}

		return pdf[0].value;
	}

	template <typename T>
    // sort based off of probability
	void sort_des(std::vector<Stochastic<T>> &pdf) {
		std::ranges::sort(pdf, [](const Stochastic<T>& a, const Stochastic<T>& b) {
			return a.probability > b.probability;
		});
	}


}
#endif