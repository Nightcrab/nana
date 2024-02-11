#pragma once

#include "Board.hpp"
#include "VersusGame.hpp"

#include <map>
#include <optional>
#include <array>

class InfoState{
	
	public:
	std::map<std::optional<Piece>, double> children;

	double sample_amount;
	double sample_sum;
};

class CFR {
	public:
	CFR() = default;
	~CFR() = default;
	CFR(const CFR& other) = default;
	CFR(CFR&& other) noexcept = default;
	CFR& operator=(const CFR& other) = default;
	
	void sample_values(VersusGame game);
};