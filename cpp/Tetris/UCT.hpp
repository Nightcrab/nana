
#include <vector>
#include <algorithm>
#include <unordered_map>

class UCT {
public:
	int workers = 4;

	std::vector<std::unordered_map<int, UCTNode>> hash_tables;

	bool nodeExists(int workerID, int nodeID);

	UCTNode getNode(int workerID, int nodeID);

	UCTNode updateNode(int workerID, int nodeID, float R);

	void insertNode(int workerID, int nodeID, UCTNode node);
};

class UCTNode {
public:
	float R;
	int N;

	std::vector<UCTNode> children;
};