// cfr_tetris.cpp : Defines the entry point for the application.
//
#include "cfr_tetris.h"
#include "Tetris/Game.hpp"
#include "Tetris/VersusGame.hpp"
#include "Tetris/Board.hpp"
#include "Tetris/Piece.hpp"
#include "Tetris/rng.hpp"
#include "Tetris/Eval.hpp"
#include "Tetris/Search.hpp"

#define OLC_PGE_APPLICATION
#include "OLC/olcPixelGameEngine.h"


const int SCREEN_WIDTH = 600;
const int SCREEN_HEIGHT = 25 * 32;

class Tetris : public olc::PixelGameEngine
{

public:
	Tetris()
	{
		sAppName = "Tetris";
	}
private:
	VersusGame game;

	void renderBoard(Game& game, int x_offset)
	{
		for (int i = 0; i < BOARD_WIDTH; i++)
		{
			for (int j = 0; j < 32; j++)
			{
				if (game.board.get(i, j))
				{
					const int size = 25;
					FillRect(i * size + x_offset, (31 - j) * size, size, size, olc::WHITE);
				}
				else
				{
					const int size = 25;
					DrawRect(i * size + x_offset, (31 - j) * size, size, size, olc::BLACK);
				}
			}
		}
	}
	void renderPiece(Game& game, int x_offset)
	{
		for (int i = 0; i < 4; i++)
		{
			int x = game.current_piece.position.x + game.current_piece.minos[i].x;
			int y = game.current_piece.position.y + game.current_piece.minos[i].y;
			const int size = 25;
			FillRect(x * size + x_offset, (31 - y) * size, size, size, olc::RED);
		}
	}

	void renderHold(Game& game, int x_offset)
	{
		if (game.hold.has_value())
		{
			for (int i = 0; i < 4; i++)
			{
				int x = game.hold.value().minos[i].x;
				int y = game.hold.value().minos[i].y;
				const int size = 10;
				FillRect(x * size + 250 + x_offset, (4-y) * size, size, size, olc::RED);
			}
		}
	}
	void renderMeter(int meter, int x_offset)
	{
		FillRect(x_offset, SCREEN_HEIGHT - meter * 25, 25, 25 * meter, olc::RED);
	}

	bool OnUserCreate() override
	{
		auto board = Board();
		Eval::eval(board);
		return true;
	}
	bool OnUserUpdate(float fElapsedTime) override
	{
		// fill the screen with black
		Clear(olc::BLUE);
		// for manually playing as player 1
		bool hard_drop = GetKey(olc::Key::V).bPressed;
		bool move_left = GetKey(olc::Key::LEFT).bPressed;
		bool sonic_drop = GetKey(olc::Key::DOWN).bPressed;
		bool move_right = GetKey(olc::Key::RIGHT).bPressed;
		bool rotate_right = GetKey(olc::Key::D).bPressed;
		bool rotate_left = GetKey(olc::Key::S).bPressed;
		bool hold = GetKey(olc::Key::UP).bPressed;

		if(move_left)
			game.p1_game.shift(game.p1_game.current_piece, -1);

		if(move_right)
			game.p1_game.shift(game.p1_game.current_piece, 1);

		if(rotate_right)
			game.p1_game.rotate(game.p1_game.current_piece, TurnDirection::Right);

		if(rotate_left)
			game.p1_game.rotate(game.p1_game.current_piece, TurnDirection::Left);

		if (hold)
		{
			if (game.p1_game.hold)
			{
				std::swap(game.p1_game.hold.value(), game.p1_game.current_piece);
			}
			else
			{
				game.p1_game.hold = game.p1_game.current_piece;
				// shift queue
				game.p1_game.current_piece = game.p1_game.queue.front();

				std::ranges::shift_left(game.p1_game.queue, 1);

				game.p1_game.queue.back() = game.p1_game.rng.getPiece();
			}
		}

		if (sonic_drop)
		{
			game.p1_game.sonic_drop(game.p1_game.board, game.p1_game.current_piece);
			game.p1_move = { game.p1_game.current_piece, false };
		}

		if (hard_drop)
		{
			game.p1_game.sonic_drop(game.p1_game.board, game.p1_game.current_piece);
			game.p1_move = { game.p1_game.current_piece , false};

			game.p2_move = game.p2_game.get_best_piece();
			game.play_moves();
		}


		if (GetKey(olc::Key::P).bHeld)
		{
			// t = time.time()
			std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
			//game.p1_move = game.p1_game.get_best_piece();
			game.p1_move = Search::monte_carlo_best_move(game, 5000,7, 0).as_pair();
			//game.p2_move = Search::monte_carlo_best_move(game, 5,5, 1).as_pair();
			game.p2_move = game.p2_game.get_best_piece();
			game.p2_move.second = false;

			game.play_moves();
			std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

			std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "[us]" << std::endl;
		}


		if (GetKey(olc::Key::R).bPressed)
		{
			game = VersusGame();
		}

		renderPiece(game.p1_game, 0);
		renderBoard(game.p1_game, 0);
		renderHold(game.p1_game, 0);
		renderMeter(game.p1_meter, 25 * 10);


		renderPiece(game.p2_game, SCREEN_WIDTH - 25 * 10);
		renderBoard(game.p2_game, SCREEN_WIDTH - 25 * 10);
		renderHold(game.p2_game, 40);
		renderMeter(game.p2_meter, SCREEN_WIDTH - 25 * 10 - 25);

		return true;
	}
};


int main()
{
	Tetris gamer;
	if (gamer.Construct(SCREEN_WIDTH, SCREEN_HEIGHT, 1, 1))
		gamer.Start();



	return 0;
	VersusGame game;
	const int n_games = 100;
	int games_played = 0;

	while (true) {
		game.p1_move = game.p1_game.get_best_piece();
		game.p2_move = game.p2_game.get_best_piece();

		game.play_moves();

		if(game.p1_game.collides(game.p1_game.board, game.p1_game.current_piece) || 
			game.p2_game.collides(game.p2_game.board, game.p2_game.current_piece))
		{
			games_played++;
			game = VersusGame();
		}

		if (games_played == n_games)
			break;
	}

	return 0;
}
