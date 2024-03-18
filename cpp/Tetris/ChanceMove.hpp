class ChanceMove {
public:
	ChanceMove() {
		p1_garbage_column = 0;
		p2_garbage_column = 0;
	}
	ChanceMove(int column1, int column2) {
		p1_garbage_column = column1;
		p2_garbage_column = column2;
	}

	int p1_garbage_column;
	int p2_garbage_column;

};