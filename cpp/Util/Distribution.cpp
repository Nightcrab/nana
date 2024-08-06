#include "Distribution.hpp"

std::vector<float> Distribution::normalise(std::vector<float> &vec) {
	float sum = 0;
	for (auto el : vec) {
		sum += el;
	}

	for (auto& el : vec) {
		el = el / sum;
	}
	return vec;
}

float Distribution::sigmoid(float x) {
	return x / (1 + abs(x));
}