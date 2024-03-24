#pragma once
#include <vector>
#include <ranges>
#include "RNG.hpp"

template <typename T>
class Stochastic {
public:
	float probability = 0;
	T value;
};

namespace Distribution {
	std::vector<float> normalise(std::vector<float> vec) {
		float sum = 0;
		std::vector<float> out;
		out.reserve(vec.size());
		for (auto el : vec) {
			sum += el;
		}

		for (auto el : vec) {
			out.push_back(el / sum);
		}
		return out;
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
	}
}