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


UCTNode UCTEdge::edgeSelect(RNG rng) {
	int rint = rng.getRand(children.size());
	UCTNode node = children[rint];
	return node;
}


bool UCT::nodeExists(int workerID, int nodeID) {
	return nodes[nodeID % workers].find(nodeID) != nodes[nodeID % workers].end();
};


UCTNode UCT::getNode(int workerID, int nodeID) {
	return nodes[nodeID % workers].at(nodeID);
};


void UCT::updateNode(int workerID, int edgeID, float R) {
	int ownerID = edgeID % workers;

	if (workerID != ownerID) {
		throw;
	}

	UCTNode edge = getNode(workerID, edgeID);

	edge.R += R;
	edge.N += 1;
};


void UCT::insertNode(int workerID, int nodeID, UCTNode node) {
	int ownerID = nodeID % workers;

	if (workerID != ownerID) {
		throw;
	}

	nodes[nodeID % workers].insert({ nodeID, node });
};