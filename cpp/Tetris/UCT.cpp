#include "UCT.hpp"
#include "MPSC.hpp"


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

bool UCT::nodeExists(int nodeID) {
	return nodes[nodeID % workers].find(nodeID) != nodes[nodeID % workers].end();
};


UCTNode UCT::getNode(int nodeID) {
	return nodes[nodeID % workers].at(nodeID);
};


void UCT::insertNode(int nodeID, UCTNode node) {
	int ownerID = nodeID % workers;

	nodes[nodeID % workers].insert({ nodeID, node });
};

