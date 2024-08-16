#pragma once

#include <array>

// 16x32 binary matrix
using Image = std::array<uint32_t, 16>;

// Channel-Height-Width
template <std::size_t C, std::size_t H, std::size_t W>
using Tensor = std::array<std::array<std::bitset<W>, H>, C>;

// u32 columnar bitboard
template <std::size_t W>
using Bitboard = std::array<uint32_t, W>;

// Linear product using XNOR-Popcount 
class Linear {
	template<M, N, P>
	Tensor<1, M, P> forward(Tensor<1, M, N> input, Tensor<1, N, P> weight);
};

// Depthwise-separable binary convolutions
class Convolution {
	// 3x3
	template<C H, W>
	Tensor<C, H, W> depthwise(Tensor<1, H, W> input, Tensor<C, 3, 3> weights);

	// 1x1
	template<C H, W>
	Tensor<C, H, W> pointwise(Tensor<C, H, W> input, Tensor<C, 1, 1> weights);
};

// Binary activation using Sign()
class Activation {
	template<C, H, W>
	Tensor<C, H, W> forward(Tensor<C, H, W> input);
};

// Stores operations, weights and their order of execution
class NN {
	template<W>
	Tensor<1, 32, W> to_tensor(Bitboard<W> input);
};