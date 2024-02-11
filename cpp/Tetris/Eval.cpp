#include "Eval.hpp"
#include "Board.hpp"

#include <fstream>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <iostream>

using lut_int = uint64_t;
struct entry {
	size_t index;
	lut_int data;
};

using DyLUT_h_t = std::vector<entry>;
using DyLUT_v_t = std::vector<entry>;

namespace Eval {
	DyLUT_h_t LUT_h;
	DyLUT_v_t LUT_v;

	std::unordered_map<uint64_t, uint64_t> LUT_h_map;
	std::unordered_map<uint64_t, uint64_t> LUT_v_map;

	void init();
	bool needs_init = true;
}

static size_t hashh(size_t left, size_t middle, size_t right, size_t height) {
	return left | (middle << 9) | (right << 18) | (height << 27);
}

static size_t hashv(size_t bottom, size_t top, size_t height) {
	return bottom | (top << 9) | (height << 18);
}

static void deserializeDyLUTV(const char* filename) {
	std::ifstream file(filename, std::ios::binary);
	file.seekg(0, std::ios::end);
	size_t size = file.tellg();
	file.seekg(0, std::ios::beg);
	Eval::LUT_v.resize(size / sizeof(entry));
	file.read(reinterpret_cast<char*>(Eval::LUT_v.data()), size);
}

static void deserializeDyLUTH(const char* filename) {
	std::ifstream file(filename, std::ios::binary);
	file.seekg(0, std::ios::end);
	size_t size = file.tellg();
	file.seekg(0, std::ios::beg);
	Eval::LUT_h.resize(size / sizeof(entry));
	file.read(reinterpret_cast<char*>(Eval::LUT_h.data()), size);
}

void Eval::init() {
	deserializeDyLUTV("DLUTV.bin");
	deserializeDyLUTH("DLUTH.bin");

	for (size_t i = 0; i < LUT_h.size(); i++)
		LUT_h_map[LUT_h[i].index] = LUT_h[i].data;

	for (size_t i = 0; i < LUT_v.size(); i++)
		LUT_v_map[LUT_v[i].index] = LUT_v[i].data;

	LUT_h.clear();
	LUT_v.clear();
}

static double logfactorial(double n) {
	double sum = 0;
	for (int i = 1; i <= n; i++)
		sum += log(i);
	return sum;
}

double Eval::eval(const Board& board) {
	return eval(board, false);
}

double Eval::eval(const Board& board, bool fast)
{
	if(needs_init)
	{
		init();
		needs_init = false;
	}
	auto get_3x3 = [](Board board, int x, int y) {
		uint16_t bits = 0;

		for (int wx = 0; wx < 3; ++wx)
			for (int wy = 0; wy < 3; ++wy) {
				bool filled = false;
				if ((x + wx) < 0 || (x + wx) >= BOARD_WIDTH)
					filled = true;
				else
					filled = board.get(x + wx, y - wy);

				if (filled)
					bits |= 1 << (wy + wx * 3);
			}
		return bits;
		};
	auto get_3x3s = [](Board board, size_t x, size_t y) {
		uint16_t bits = 0;

		for (size_t wx = 0; wx < 3; ++wx){
				uint32_t filled = 0;
				if ((x + wx) < 0 || (x + wx) >= BOARD_WIDTH)
					filled = 0b111;
				else
					filled = (board.get_column(static_cast<unsigned long long>(x) + wx) & ((size_t)0b111 << y + 2)) >> y;

				bits |= filled << ((3 - wx) * 3);
			}
		return bits;
	};

	double score = 1.0;
	struct fentry {
		size_t index;
		double data;
		bool operator<(const fentry& other)const
		{
			return index < other.index;
		}
	};

	struct Comp {
		bool operator()(const fentry& a, size_t b) const
		{
			return a.index < b;
		}
		bool operator()(size_t a, const fentry& b) const
		{
			return a < b.index;
		}
		bool operator()(const fentry& a, const fentry& b) const
		{
			return a.index < b.index;
		}
	};
	std::vector<fentry> freq_h; freq_h.reserve(90);
	std::vector<fentry> freq_v; freq_v.reserve(144);

	// note: can trade 2x speed for 30% quality drop by striding horizontal x by 2 and vertical y by 2

	for (size_t x = 1; x < BOARD_WIDTH - 4; x += 1)
		for (size_t y = 2; y < 20; y += 1)
		{
			size_t left = get_3x3(board, x - 3, y);
			size_t middle = get_3x3(board, x, y);
			size_t right = get_3x3(board, x + 3, y);

			size_t effective_y = y;

			// account for lack of data at the top
			if (y > 10) {
				effective_y -= 10;
			}

			bool exists = std::binary_search(freq_h.begin(), freq_h.end(), hashh(left, middle, right, effective_y), Comp{});
			if(!exists)
				freq_h.emplace_back(hashh(left, middle, right, effective_y), 1.0);
			else
			{
				auto it = std::lower_bound(freq_h.begin(), freq_h.end(), hashh(left, middle, right, effective_y), Comp{});
				it->data += 1.0;
			}
		}


	for (size_t x = 0; x < BOARD_WIDTH - 2; x += 1)
		for (size_t y = 2; y < 20; y += 2)
		{
			size_t bottom = get_3x3(board, x, y);
			size_t top = get_3x3(board, x, y + 3);

			size_t effective_y = y;

			// account for lack of data at the top
			if (y > 10) {
				effective_y -= 10;
			}

			bool exists = std::binary_search(freq_v.begin(), freq_v.end(), hashv(bottom, top, effective_y), Comp{});
			if (!exists)
				freq_v.emplace_back(hashv(bottom, top, effective_y), 1.0);
			else
			{
				auto it = std::lower_bound(freq_v.begin(), freq_v.end(), hashv(bottom, top, effective_y), Comp{});
				it->data += 1.0;
			}
		}


	for (auto& [key, f_k] : freq_h)
	{

		double frequency = LUT_h_map.find(key) != LUT_h_map.end() ? LUT_h_map[key] : 0.0;
		// clip at 3 to prevent negatives
		double LUT_K = std::max((double)frequency, 3.0);
	
		score += f_k * log(LUT_K) - logfactorial(f_k);
	}

	for (auto& [key, f_k] : freq_v)
	{
		double frequency = LUT_v_map.find(key) != LUT_v_map.end() ? LUT_v_map[key] : 0.0;
		// clip at 3 to prevent negatives
		double LUT_K = std::max((double)frequency, 3.0);
		score += f_k * log(LUT_K) - logfactorial(f_k);
	}

	return score;
}
