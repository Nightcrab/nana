#include "rng.hpp"
#include "Board.hpp"

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

	Piece p1_next_piece = Piece(PieceType::Empty);
	Piece p2_next_piece = Piece(PieceType::Empty);

	pptRNG rng1;
	pptRNG rng2;

	void new_move() {
		p1_next_piece = rng1.getPiece();
		p2_next_piece = rng2.getPiece();
		p1_garbage_column = rng1.GetRand(Board::width);
		p2_garbage_column = rng2.GetRand(Board::width);
	};
};