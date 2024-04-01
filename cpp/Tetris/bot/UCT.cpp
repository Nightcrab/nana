#include "UCT.hpp"
#include "MPSC.hpp"
#include <iostream>


UCTNode::UCTNode(EmulationGame state) {
	std::vector<Piece> raw_actions = state.game.get_possible_piece_placements();

	std::vector<Action> actions;
	actions.reserve(raw_actions.size() * 2);

	int idx = 0;

	for (auto& raw_action : raw_actions) {
		actions.push_back(Action(Move(raw_action, false), idx));
		idx++;
	}

	this->actions = actions;

	this->ID = state.hash();

	this->N = 1;
}; 

float ln(float x) {
	unsigned int bx = *(unsigned int*)(&x);
	unsigned int ex = bx >> 23;
	signed int t = (signed int)ex - (signed int)127;
	unsigned int s = (t < 0) ? (-t) : t;
	bx = 1065353216 | (bx & 8388607);
	x = *(float*)(&bx);
	return -1.49278 + (2.11263 + (-0.729104 + 0.10969 * x) * x) * x + 0.6931471806 * t;
}

float quick_sqrt(const float x)
{
	const float xhalf = 0.5f * x;

	union // get bits for floating value
	{
		float x;
		int i;
	} u;
	u.x = x;
	u.i = 0x5f3759df - (u.i >> 1);  // gives initial guess y0
	return x * u.x * (1.5f - xhalf * u.x * u.x);// Newton step, repeating increases accuracy 
}

Action& UCTNode::select() {
	Action& best_action = actions[0];
	float highest_priority = -1.0;
	constexpr float c = 1.41421356237;

	//std::cout << "action count: " << actions.size() << std::endl;

	//for (Action& edge : actions) {
		//std::cout << edge.N << "," << edge.R << " ";
	//}
	//std::cout << std::endl;

	for (Action& edge : actions) {
		if (edge.N == 0) {
			return edge;
		}
		float priority = edge.R / edge.N + c * quick_sqrt(ln(N) / edge.N);
		if (priority > highest_priority) {
			best_action = edge;
			highest_priority = priority;
		}
	}

	return best_action;
}

bool UCT::nodeExists(uint32_t nodeID) {
	return nodes[nodeID % workers].find(nodeID) != nodes[nodeID % workers].end();
};


UCTNode& UCT::getNode(uint32_t nodeID) {
	return nodes[nodeID % workers].at(nodeID);
};

void UCT::insertNode(UCTNode node) {
	size++;
	nodes[node.ID % workers].insert({ node.ID, node });
};

uint32_t UCT::getOwner(uint32_t hash) {
	return hash % workers;
}