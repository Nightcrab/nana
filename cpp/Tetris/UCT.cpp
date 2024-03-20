#include "UCT.hpp"

UCTNode UCTNode::nodeSelect() {
	UCTNode& best_edge = children[0];
	float highest_priority = -1.0;
	constexpr float c = 1.41421356237;

	for (UCTNode& edge : children) {
		float priority = edge.R / edge.N + c * sqrt(log(N) / edge.N);
		if (priority > highest_priority) {
			best_edge = edge;
			highest_priority = priority;
		}
	}

	return best_edge;
}

UCTNode UCTNode::edgeSelect(RNG rng) {
	int rint = rng.getRand(children.size());
	UCTNode node = children[rint];
	return node;
}