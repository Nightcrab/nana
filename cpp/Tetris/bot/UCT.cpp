#include "UCT.hpp"
#include "MPSC.hpp"


UCTNode::UCTNode(EmulationGame state) {
	std::vector<Piece> raw_actions = state.game.get_possible_piece_placements();

	std::vector<Action> actions;
	actions.reserve(raw_actions.size() * 2);

	for (auto& raw_action : raw_actions) {
		actions.push_back(Action(Move(raw_action, true)));
		actions.push_back(Action(Move(raw_action, false)));
	}

	this->actions = actions;

	this->ID = state.hash();

	this->N = 1;
};

Action UCTNode::select() {
	Action& best_action = actions[0];
	float highest_priority = -1.0;
	constexpr float c = 1.41421356237;

	for (Action& edge : actions) {
		float priority = edge.R / edge.N + c * sqrt(log(N) / edge.N);
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


UCTNode UCT::getNode(uint32_t nodeID) {
	return nodes[nodeID % workers].at(nodeID);
};

void UCT::insertNode(UCTNode node) {
	nodes[node.ID % workers].insert({ node.ID, node });
};

uint32_t UCT::getOwner(uint32_t hash) {
	return hash % workers;
}