#include "NN.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <bitset>
#include "engine/Board.hpp"
#include "engine/Game.hpp"
#include "Move.hpp"

/*

012
3?4
567

012012012
3?43?43?4
567567567
012012012
3?43?43?4
567567567
012012012
3?43?43?4
567567567

*/
consteval auto something() {
	std::array<Image, 256> lut{};

	for (int iter = 0; iter < 256; ++iter) {
		const std::bitset<32> b(iter);
		std::array<std::bitset<32>, 3> patterns{};

		// pattern one 
		for (int i = 31; i >= 0; i -= 3) {
			auto& pattern = patterns[0];

			pattern[i - 0] = b[0];
			if (i - 1 >= 0)
				pattern[i - 1] = b[3];
			if (i - 2 >= 0)
				pattern[i - 2] = b[5];
		}
		// pattern two
		for (int i = 31; i >= 0; i -= 3) {
			auto& pattern = patterns[1];

			pattern[i - 0] = b[1];
			if (i - 2 >= 0)
				pattern[i - 2] = b[6];
		}
		// pattern three
		for (int i = 31; i >= 0; i -= 3) {
			auto& pattern = patterns[2];

			pattern[i - 0] = b[2];
			if (i - 1 >= 0)
				pattern[i - 1] = b[4];
			if (i - 2 >= 0)
				pattern[i - 2] = b[7];
		}

		// for every column in the lut [ iter ]

		for (int i = 0; i < 16; ++i) {
			lut[iter][i] = patterns[i % 3].to_ulong();
		}

	}

	return lut;
}


constexpr std::array<Image, 256> lut = something();


int dot_product(const Image& input, const Image& weight) {

	std::array<std::bitset<64>, 8> A = std::bit_cast<std::array<std::bitset<64>, 8>>(input);
	std::array<std::bitset<64>, 8> B = std::bit_cast<std::array<std::bitset<64>, 8>>(weight);

	int total{};

	for (int i = 0; i < 8; ++i) {
		total += (A[i] ^ ~B[i]).count();
	}

	return total;
}

Image hadamard(const Image& input, const Image& weight) {

	Image ret;

	for (int i = 0; i < 16; ++i) {
		ret[i] = input[i] ^ ~weight[i];
	}

	return ret;
}
