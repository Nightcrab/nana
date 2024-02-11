#include "Game.hpp"
#include "Eval.hpp"

#include <iostream>

#include <map>
#include <tuple>

void Game::place_piece() {
	board.set(current_piece);

	current_piece = queue.front();

	std::ranges::shift_left(queue, 1);

	queue.back() = rng.getPiece();
}

bool Game::collides(const Board& board, const Piece& piece) const {
	for (auto& mino : piece.minos)
	{
		int x_pos = mino.x + piece.position.x;
		if (x_pos < 0 || x_pos >= BOARD_WIDTH)
			return true;

		int y_pos = mino.y + piece.position.y;
		if (y_pos < 0 || y_pos >= BOARD_HEIGHT)
			return true;
		if (board.get(x_pos, y_pos))
			return true;
	}

	return false;
}

void Game::rotate(Piece& piece, TurnDirection dir) const {
	const RotationDirection prev_rot = piece.rotation;

	piece.rotate(dir);

	const std::array<Coord, srs_kicks>* offsets = &piece_offsets_JLSTZ[piece.rotation];
	const std::array<Coord, srs_kicks>* prev_offsets = &piece_offsets_JLSTZ[prev_rot];

	if (piece.type == PieceType::I)
	{
		prev_offsets = &piece_offsets_I[prev_rot];
		offsets = &piece_offsets_I[piece.rotation];
	}
	else if (piece.type == PieceType::O)
	{
		prev_offsets = &piece_offsets_O[prev_rot];
		offsets = &piece_offsets_O[piece.rotation];
	}

	auto x = piece.position.x;
	auto y = piece.position.y;

	for (int i = 0; i < srs_kicks; i++)
	{
		piece.position.x = x + (*prev_offsets)[i].x - (*offsets)[i].x;
		piece.position.y = y + (*prev_offsets)[i].y - (*offsets)[i].y;
		if (!collides(board, piece))
		{
			if (piece.type == PieceType::T)
			{
				constexpr std::array < std::array<Coord, 4>, 4 > corners = { {
						//a       b       c        d
					{ { {-1, 1}, { 1, 1}, { 1,-1}, {-1,-1}} }, // North
					{ { { 1, 1}, { 1,-1}, {-1,-1}, {-1, 1}} }, // East
					{ { { 1,-1}, {-1,-1}, {-1, 1}, { 1, 1}} }, // South
					{ { {-1,-1}, {-1, 1}, { 1, 1}, { 1,-1} } }, // West
					} };

				bool filled[4] = { true,true,true,true };

				for (int u = 0; u < 4; u++) {
					Coord c = corners[piece.rotation][u];
					c.x += piece.position.x;
					c.y += piece.position.y;
					if (c.x >= 0 && c.x < BOARD_WIDTH)
						filled[u] = board.get(c.x, c.y);
				}



				if (filled[0] && filled[1] && (filled[2] || filled[3]))
					piece.spin = spinType::normal;
				else if ((filled[0] || filled[1]) && filled[2] && filled[3])
				{
					if (i >= (srs_kicks - 1)) {
						piece.spin = spinType::normal;
					}
					else
						piece.spin = spinType::mini;
				}
				else
					piece.spin = spinType::null;
			}
			return;
		}
	}

	piece.position.x = x;
	piece.position.y = y;

	piece.rotate(dir == TurnDirection::Left ? TurnDirection::Right : TurnDirection::Left);
}

void Game::shift(Piece& piece, int dir) const
{
	piece.position.x += dir;

	if (collides(board, piece))
		piece.position.x -= dir;
	else
		piece.spin = spinType::null;
}

void Game::sonic_drop(const Board board, Piece& piece) const
{
	while (!collides(board, piece))
	{
		piece.position.y--;
	}
	piece.position.y++;
}

void Game::add_garbage(int lines, int loc) {
	for (int i = 0; i < BOARD_WIDTH; ++i) {
		auto& column = board.board[i];
		column <<= lines;

		if (loc != i) {
			column |= (1 << lines) - 1;
		}
	}
}

// ported from
// https://github.com/emmachase/tetrio-combo
int Game::damage_sent(int linesCleared, spinType spinType, bool pc) {

	auto maintainsB2B = false;
	if (linesCleared) {
		combo++;
		if (4 == linesCleared || spinType != spinType::null) {
			maintainsB2B = true;
		}

		if (maintainsB2B) {
			b2b++;
		}
		else {
			b2b = 0;
		}
	}
	else {
		combo = 0;
		currentcombopower = 0;
	}

	int garbage = 0;

	switch (linesCleared) {
	case 0:
		if (spinType::mini == spinType) {
			garbage = GarbageValues::TSPIN_MINI;
		}
		else if (spinType::normal == spinType) {
			garbage = GarbageValues::TSPIN;
		}
		break;
	case 1:
		if (spinType::mini == spinType) {
			garbage = GarbageValues::TSPIN_MINI_SINGLE;
		}
		else if (spinType::normal == spinType) {
			garbage = GarbageValues::TSPIN_SINGLE;
		}
		else {
			garbage = GarbageValues::SINGLE;
		}
		break;

	case 2:
		if (spinType::mini == spinType) {
			garbage = GarbageValues::TSPIN_MINI_DOUBLE;
		}
		else if (spinType::normal == spinType) {
			garbage = GarbageValues::TSPIN_DOUBLE;
		}
		else {
			garbage = GarbageValues::DOUBLE;
		}
		break;

	case 3:
		if (spinType != spinType::null) {
			garbage = GarbageValues::TSPIN_TRIPLE;
		}
		else {
			garbage = GarbageValues::TRIPLE;
		}
		break;

	case 4:
		if (spinType != spinType::null) {
			garbage = GarbageValues::TSPIN_QUAD;
		}
		else {
			garbage = GarbageValues::QUAD;
		}
		break;
	}

	if (linesCleared) {
		if (b2b > 1) {

			if (options.b2bchaining) {
				const int b2bGarbage = GarbageValues::BACKTOBACK_BONUS *
					(
						1 + std::log1p((b2b - 1) * GarbageValues::BACKTOBACK_BONUS_LOG)
						+ (b2b - 1 <= 1
							? 0
							: (1 +
								std::log1p((b2b - 1) * GarbageValues::BACKTOBACK_BONUS_LOG) -
								(int)std::log1p((b2b - 1) * GarbageValues::BACKTOBACK_BONUS_LOG)) / 3
							)
						);

				garbage += b2bGarbage;

				if (b2bGarbage > currentbtbchainpower) {
					currentbtbchainpower = b2bGarbage;
				}
			}
			else {
				garbage += GarbageValues::BACKTOBACK_BONUS;
			}
		}
		else {
			currentbtbchainpower = 0;
		}
	}

	if (combo > 1) {
		garbage *= 1 + GarbageValues::COMBO_BONUS * (combo - 1); // Fucking broken ass multiplier :)
	}

	if (combo > 2) {
		garbage = std::max((int)std::log1p(GarbageValues::COMBO_MINIFIER * (combo - 1) * GarbageValues::COMBO_MINIFIER_LOG), garbage);
	}

	const int finalGarbage = garbage * options.garbagemultiplier;
	if (combo > 2) {
		currentcombopower = std::max(currentcombopower, finalGarbage);
	}

	const auto combinedGarbage = finalGarbage + (pc ? GarbageValues::ALL_CLEAR : 0);

	return combinedGarbage;
}

void Game::process_movement(Piece& piece, Movement movement) const {
	switch (movement)
	{
	case Movement::Left:
		shift(piece, -1);
		break;
	case Movement::Right:
		shift(piece, 1);
		break;
	case Movement::RotateClockwise:
		rotate(piece, TurnDirection::Right);
		break;
	case Movement::RotateCounterClockwise:
		rotate(piece, TurnDirection::Left);
		break;
	case Movement::SonicDrop:
		sonic_drop(board, piece);
		break;
	default:
		std::unreachable();
	}
}


std::vector<Piece> Game::movegen(PieceType piece_type) const {

	Piece initial_piece = Piece(piece_type);

	std::vector<Piece> open_nodes; open_nodes.reserve(150);
	std::vector<Piece> next_nodes; next_nodes.reserve(150);
	std::map<u32, bool> visited;

	std::vector<Piece> valid_moves; valid_moves.reserve(100);

	// root node
	open_nodes.emplace_back(initial_piece);

	while (open_nodes.size() > 0)
	{
		// expand edges
		for (auto& piece : open_nodes)
		{
			auto h = piece.hash();
			if (visited[h])
				continue;
			// mark node as visited
			visited[h] = true;


			// try all movements
			Piece new_piece = piece;
			process_movement(new_piece, Movement::RotateCounterClockwise);
			next_nodes.emplace_back(new_piece);

			new_piece = piece;
			process_movement(new_piece, Movement::RotateClockwise);
			next_nodes.emplace_back(new_piece);

			new_piece = piece;
			process_movement(new_piece, Movement::Left);
			next_nodes.emplace_back(new_piece);

			new_piece = piece;
			process_movement(new_piece, Movement::Right);
			next_nodes.emplace_back(new_piece);

			new_piece = piece;
			process_movement(new_piece, Movement::SonicDrop);
			next_nodes.emplace_back(new_piece);

			// check if the piece is grounded and therefore valid

			piece.position.y--;

			if (collides(board, piece))
			{
				piece.position.y++;
				valid_moves.emplace_back(piece);
			}

		}
		open_nodes = next_nodes;
		next_nodes.clear();
	}

	return valid_moves;
}

std::pair<Piece, bool> Game::get_best_piece() const {
	std::vector<Piece> valid_pieces = movegen(current_piece.type);
	PieceType holdType = hold.has_value() ? hold->type : queue.front();

	std::vector<Piece> hold_pieces = movegen(holdType);
	valid_pieces.reserve(valid_pieces.size() + hold_pieces.size());
	for (auto& piece : hold_pieces)
	{
		valid_pieces.emplace_back(piece);
	}

	std::optional<Piece> best_piece;
	std::vector<std::pair<double, std::optional<Piece>>> moves;

	double total_score = 0;
	double max_score = 0;

	for (auto& piece : valid_pieces)
	{
		Board temp_board = board;
		temp_board.set(piece);
		double score = Eval::eval(temp_board);

		total_score += score;
		max_score = std::max(max_score, score);
		moves.emplace_back(score, piece);
	}

	// try the null move
	const double null_score = Eval::eval(board);
	{
		total_score += null_score;
		max_score = std::max(max_score, null_score);
		moves.emplace_back(null_score, std::nullopt);
	}
	// sort the moves by score
	std::sort(moves.begin(), moves.end(), [](auto& a, auto& b) { return a.first > b.first; });


	// pick a random piece based on the score as a probability between 0 and 1
	std::mt19937 gen;
	gen.seed(std::random_device()());
	std::uniform_real_distribution<double> dis(0, 1);
	double r = dis(gen);
	auto softmax = [](double x, double total_score) { return exp(x) / total_score; };

	double sum = 0;

	for (auto& [score, piece] : moves)
	{
		score -= max_score;
	}
	total_score = 0;
	for (auto& [score, piece] : moves)
	{
		total_score += exp(score);
	}
	bool null_move = false;
	for (auto& [score, piece] : moves)
	{
		sum += softmax(score, total_score);
		if (r < sum)
		{
			best_piece = piece;

			if (!best_piece.has_value())
				null_move = true;

			break;
		}
	}

	if (null_move)
	{
		r = dis(gen);

		auto it = std::find(moves.begin(), moves.end(), std::make_pair(null_score - max_score, std::nullopt));
		if (it != moves.end())
		{
			// take out the nullopt
			moves.erase(it);
		}

		sum = 0;

		// recompute
		for (auto& [score, piece] : moves)
		{
			max_score = std::min(max_score, score);
		}

		for (auto& [score, piece] : moves)
		{
			score -= max_score;
		}

		total_score = 0;
		for (auto& [score, piece] : moves)
		{
			total_score += exp(score);
		}

		for (auto& [score, piece] : moves)
		{
			sum += softmax(score, total_score);
			if (r < sum)
			{
				best_piece = piece;
				break;
			}
		}
	}

	// if null_move is true, we attempt null move
	return std::make_pair(*best_piece, null_move);
}

std::pair<Piece, bool> Game::get_bestish_piece() const {
	std::vector<Piece> valid_pieces = movegen(current_piece.type);
	PieceType holdType = hold.has_value() ? hold->type : queue.front();

	std::vector<Piece> hold_pieces = movegen(holdType);
	valid_pieces.reserve(valid_pieces.size() + hold_pieces.size());
	for (auto& piece : hold_pieces)
	{
		valid_pieces.emplace_back(piece);
	}

	std::vector<std::pair<double, std::optional<Piece>>> moves;

	double total_score = 0;
	double max_score = 0;

	for (auto& piece : valid_pieces)
	{
		Board temp_board = board;
		temp_board.set(piece);
		double score = Eval::eval(temp_board);

		total_score += score;
		max_score = std::max(max_score, score);
		moves.emplace_back(score, piece);
	}

	// try the null move
	const double null_score = Eval::eval(board);
	{
		total_score += null_score;
		max_score = std::max(max_score, null_score);
		moves.emplace_back(null_score, std::nullopt);
	}
	// sort the moves by score
	std::sort(moves.begin(), moves.end(), [](auto& a, auto& b) { return a.first > b.first; });


	// pick a random piece based on the score as a probability between 0 and 1
	std::mt19937 gen;
	gen.seed(std::random_device()());
	std::uniform_real_distribution<double> dis(0, 1);
	double r = dis(gen);
	auto softmax = [](double x, double total_score) { return exp(x) / total_score; };

	double Eexn = 0;

	for (int i = 0; i < moves.size(); i++)
	{
		//Eexn += exp(moves[i].first) / moves.size();
	}

	auto softmax2 = [Eexn](double x) { return exp(x) + Eexn; };
	auto return_best_piece = [](std::vector<std::pair<double, std::optional<Piece>>>& moves,
		double Eexn, double r
		) {
			//r = (r - 1) * (r - 1);
			double max_score = 0;
			for (auto& [score, piece] : moves)
			{
				max_score = std::max(max_score, score);
			}

			for (auto& [score, piece] : moves)
			{
				score -= max_score;
			}
			double total_score = 0;
			for (auto& [score, piece] : moves)
			{
				total_score += exp(score / 10) + Eexn;
			}
			double sum = 0;


			for (auto& [score, piece] : moves)
			{
				sum += (exp(score / 10)) / total_score;
				if (r < sum)
				{
					return piece;
				}
			}

			// no return
			std::unreachable();
		};

	bool null_move = false;

	auto best_piece = return_best_piece(moves, Eexn, r);

	if (!best_piece.has_value())
		null_move = true;

	if (null_move)
	{
		r = dis(gen);

		auto it = std::find(moves.begin(), moves.end(), std::make_pair(null_score - max_score, std::nullopt));
		if (it != moves.end())
		{
			// take out the nullopt
			moves.erase(it);
		}

		// recompute
		//Eexn = (Eexn * (moves.size() + 1) - exp(null_score - max_score)) / moves.size();

		best_piece = return_best_piece(moves, Eexn, r);
	}

	// if null_move is true, we attempt null move
	return std::make_pair(*best_piece, null_move);
}
