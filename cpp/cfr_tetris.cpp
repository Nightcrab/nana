// cfr_tetris.cpp : Defines the entry point for the application.
//
#include "cfr_tetris.h"

#include "Board.hpp"
#include "Eval.hpp"
#include "Game.hpp"
#include "Piece.hpp"
#include "VersusGame.hpp"
#include "EmulationGame.hpp"
#include "Search.hpp"
#include "rng.hpp"
#include "Distribution.hpp"

#define OLC_PGE_APPLICATION
#include "OLC/olcPixelGameEngine.h"
#include <numeric>

const int SCREEN_WIDTH = 600;
const int SCREEN_HEIGHT = 25 * 32;

class Tetris : public olc::PixelGameEngine {
   public:
    Tetris() {
        sAppName = "Tetris";
    }

   protected:
    VersusGame game;
    RNG player_1_rng;
    RNG player_2_rng;

    void renderBoard(Game& game, int x_offset) {
        for (int i = 0; i < Board::width; i++) {
            for (int j = 0; j < 32; j++) {
                if (game.board.get(i, j)) {
                    const int size = 25;
                    FillRect(i * size + x_offset, (31 - j) * size, size, size, olc::WHITE);
                } else {
                    const int size = 25;
                    DrawRect(i * size + x_offset, (31 - j) * size, size, size, olc::BLACK);
                }
            }
        }
    }
    void renderPiece(Game& game, int x_offset) {
        for (int i = 0; i < 4; i++) {
            int x = game.current_piece.position.x + game.current_piece.minos[i].x;
            int y = game.current_piece.position.y + game.current_piece.minos[i].y;
            const int size = 25;
            FillRect(x * size + x_offset, (31 - y) * size, size, size, olc::RED);
        }
    }

    void renderHold(Game& game, int x_offset) {
        if (game.hold.has_value()) {
            for (int i = 0; i < 4; i++) {
                int x = game.hold.value().minos[i].x;
                int y = game.hold.value().minos[i].y;
                const int size = 10;
                FillRect(x * size + 250 + x_offset, (4 - y) * size, size, size, olc::RED);
            }
        }
    }
    void renderMeter(int meter, int x_offset) {
        FillRect(x_offset, SCREEN_HEIGHT - meter * 25, 25, 25 * meter, olc::RED);
    }

    bool OnUserCreate() override {
        // give both players a bag
        game.p1_game.current_piece = player_1_rng.getPiece();
        for (auto& piece_type : game.p1_game.queue) {
            piece_type = player_1_rng.getPiece();
        }

        game.p2_game.current_piece = player_2_rng.getPiece();
        for (auto& piece_type : game.p2_game.queue) {
            piece_type = player_2_rng.getPiece();
        }

        auto board = Board();
        Eval::eval_LUT(board);
        return true;
    }
    bool OnUserUpdate(float fElapsedTime) override {
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

        if (move_left)
            game.p1_game.shift(game.p1_game.current_piece, -1);

        if (move_right)
            game.p1_game.shift(game.p1_game.current_piece, 1);

        if (rotate_right)
            game.p1_game.rotate(game.p1_game.current_piece, TurnDirection::Right);

        if (rotate_left)
            game.p1_game.rotate(game.p1_game.current_piece, TurnDirection::Left);

        if (hold) {
            if (game.p1_game.hold) {
                std::swap(game.p1_game.hold.value(), game.p1_game.current_piece);
            } else {
                game.p1_game.hold = game.p1_game.current_piece;

                // shift queue
                game.p1_game.current_piece = game.p1_game.queue.front();

                std::shift_left(game.p1_game.queue.begin(), game.p1_game.queue.end(), 1);

                game.p1_game.queue.back() = player_1_rng.getPiece();
            }
        }

        if (sonic_drop) {
            game.p1_game.sonic_drop(game.p1_game.board, game.p1_game.current_piece);
            game.p1_move = {game.p1_game.current_piece, false};
        }

        if (hard_drop) {
            game.p1_game.sonic_drop(game.p1_game.board, game.p1_game.current_piece);
            game.p2_game.sonic_drop(game.p2_game.board, game.p2_game.current_piece);
            game.p1_move = {game.p1_game.current_piece, false};
            game.p2_move = {game.p2_game.current_piece, false};

            // game.p2_move = Search::monte_carlo_best_move(game, 12, 120, 7, 1);
            game.play_moves();
        }

        if (GetKey(olc::Key::Q).bPressed) {
            game.p1_game.sonic_drop(game.p1_game.board, game.p1_game.current_piece);
            game.p1_move = {game.p1_game.current_piece, true};

            game.play_moves();
        }
        if (GetKey(olc::Key::P).bHeld) {
            // t = time.time()
            std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
            game.play_moves();
            std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

            // std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "[us]" << std::endl;
        }

        if (GetKey(olc::Key::R).bPressed) {
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

class Firetris : public Tetris {
public:
    Firetris() {
        sAppName = "Firetris";
    }
private:
    EmulationGame game;

    bool OnUserCreate() override {
        game = EmulationGame();

        game.game.current_piece = game.chance.rng.getPiece();

        for (auto& piece_type : game.game.queue) {
            piece_type = game.chance.rng.getPiece();
        }
        return true;
    }
    bool OnUserUpdate(float fElapsedTime) override {
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

        if (move_left)
            game.game.process_movement(game.game.current_piece, Movement::Left);

        if (move_right)
            game.game.process_movement(game.game.current_piece, Movement::Right);

        if (rotate_right)
            game.game.process_movement(game.game.current_piece, Movement::RotateClockwise);

        if (rotate_left)
            game.game.process_movement(game.game.current_piece, Movement::RotateCounterClockwise);

        if (hold) {
            game.game.do_hold();
        }

        if (sonic_drop) {
            game.game.process_movement(game.game.current_piece, Movement::SonicDrop);
        }

        if (hard_drop) {
            game.game.process_movement(game.game.current_piece, Movement::SonicDrop);
            game.set_move(Move(game.game.current_piece, false));

            game.play_moves();

            std::cout << "APP:" << game.app() << std::endl;
        }

        if (GetKey(olc::Key::P).bPressed) {
            if (Search::searching) {
                Search::endSearch();
            }
            else {
                Search::startSearch(game, 4);
            }
            
        }

        if (GetKey(olc::Key::O).bPressed) {
            std::vector<Move> moves = game.legal_moves();

            std::vector<Stochastic<Move>> policy;
            std::vector<Stochastic<Move>> SoR_policy;
            std::vector<Stochastic<float>> cc_dist;

            policy.reserve(moves.size());

            for (auto& move : moves) {
                // raw scores
                policy.push_back(Stochastic<Move>(move, Eval::eval_CC(game.game, move)));
            }

            // sort in descending order
            std::ranges::sort(policy, [](const Stochastic<Move>& a, const Stochastic<Move>& b) {
                return a.probability > b.probability;
                });

            // square of rank

            for (int rank = 1; rank <= policy.size(); rank++) {
                float prob = 1.0 / (rank * rank);
                SoR_policy.push_back(Stochastic<Move>(policy[rank - 1].value, prob));
                cc_dist.push_back(Stochastic<float>(policy[rank - 1].probability, prob));
            }

            std::cout << "expectation of this state was: " << Distribution::expectation(cc_dist) << std::endl;

            Distribution::normalise(SoR_policy);

            Move sample = Distribution::sample(SoR_policy, game.chance.rng);

            game.set_move(sample);
            game.play_moves();
        }

        if (GetKey(olc::Key::Q).bPressed && Search::searching) {
            Search::endSearch();
            game.set_move(Search::bestMove());

            game.play_moves();

            Search::printStatistics();

            Search::continueSearch(game);

            std::cout << "APP:" << game.app() << std::endl;
        }
        
        if (GetKey(olc::Key::R).bPressed) {
            game = EmulationGame();

            game.game.current_piece = game.chance.rng.getPiece();

            for (auto& piece_type : game.game.queue) {
                piece_type = game.chance.rng.getPiece();
            }
        }

        renderPiece(game.game, 0);
        renderBoard(game.game, 0);
        renderHold(game.game, 0);
        renderMeter(std::accumulate(game.garbage_meter.begin(), game.garbage_meter.end(), 0), 25 * 10);

        return true;
    }
};

int main() {
    /*
    Tetris gamer;
    if (gamer.Construct(SCREEN_WIDTH, SCREEN_HEIGHT, 1, 1))
        gamer.Start();
    */

    Firetris gamer;
    if (gamer.Construct(SCREEN_WIDTH, SCREEN_HEIGHT, 1, 1))
        gamer.Start();

    return 0;
    VersusGame game;
    const int n_games = 100;
    int games_played = 0;

    while (true) {
        game.play_moves();

        if (game.p1_game.collides(game.p1_game.board, game.p1_game.current_piece) ||
            game.p2_game.collides(game.p2_game.board, game.p2_game.current_piece)) {
            games_played++;
            game = VersusGame();
        }

        if (games_played == n_games)
            break;
    }

    return 0;
}
