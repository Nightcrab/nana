#pragma once
#include <ranges>
#include <vector>

#include "rng.hpp"

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

namespace Distribution {

	std::vector<float> normalise(std::vector<float> vec) {
		float sum = 0;
		for (auto el : vec) {
			sum += el;
		}

		for (auto& el : vec) {
			el = el / sum;
		}
		return vec;
	}

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
	T sample(std::vector<Stochastic<T>> pdf, RNG rng) {
		std::ranges::sort(pdf, [](const Stochastic<T>& a, const Stochastic<T>& b)
		{
			return a.probability > b.probability;
		});
		float r = (float) rng.getRand(1 << 16) / (float) (1 << 16);
		for (auto& event : pdf) {
			r -= event.probability;
			if (r < 0) {
				return event.value;
			}
		}

		return pdf[0].value;
	}
}