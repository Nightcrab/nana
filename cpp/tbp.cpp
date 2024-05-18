#include <iostream>
#include <string>

#include "Search.hpp"
#include "Util/json.hpp"

auto type_to_json(PieceType type) -> nlohmann::json {
    switch (type) {
        case PieceType::S:
            return "S";
            break;
        case PieceType::Z:
            return "Z";
            break;
        case PieceType::J:
            return "J";
            break;
        case PieceType::L:
            return "L";
            break;
        case PieceType::T:
            return "T";
            break;
        case PieceType::O:
            return "O";
            break;
        case PieceType::I:
            return "I";
            break;
        case PieceType::Empty:
            return "I";
            break;
        default:
            return nullptr;
            break;
    }
};

auto json_to_type(nlohmann::json json) -> PieceType {
    std::string str = json.get<std::string>();
    if (str == "S") {
        return PieceType::S;
    } else if (str == "Z") {
        return PieceType::Z;
    } else if (str == "J") {
        return PieceType::J;
    } else if (str == "L") {
        return PieceType::L;
    } else if (str == "T") {
        return PieceType::T;
    } else if (str == "O") {
        return PieceType::O;
    } else if (str == "I") {
        return PieceType::I;
    } else if (str == "null") {
        return PieceType::Empty;
    } else {
        return PieceType::Empty;
    }
};

auto board_to_json(Board board) -> nlohmann::json {
    nlohmann::json json;
    std::array<std::array<std::optional<std::string>, 40>, 10> tbpBoard;

    for (int x = 0; x < Board::width; ++x)
        for (int y = 0; y < Board::visual_height; ++y) {
            if (board.get(x, y))
                tbpBoard[x][y] = "G";
            else
                tbpBoard[x][y] = std::nullopt;
        }

    json = nlohmann::json::array();

    for (int y = 0; y < Board::height; ++y) {
        nlohmann::json tmp = nlohmann::json::array();
        for (int x = 0; x < Board::width; ++x) {
            if (tbpBoard[x][y].has_value()) {
                tmp.push_back(tbpBoard[x][y].value());
            } else {
                tmp.push_back(nullptr);
            }
        }
        json.push_back(tmp);
    }
    return json;
};

auto json_to_board(nlohmann::json json) -> Board {
    Board board;
    for (int y = 0; y < Board::height; ++y) {
        for (int x = 0; x < Board::width; ++x) {
            if (!json[y][x].is_null()) {
                board.set(x, y);
            }
        }
    }
    return board;
};

auto json_to_queue(nlohmann::json json) -> std::vector<PieceType> {
    std::vector<PieceType> queue;
    for (auto &piece : json) {
        queue.push_back(json_to_type(piece));
    }
    return queue;
};
auto rotation_to_json(RotationDirection dir) -> nlohmann::json {
    if (dir == RotationDirection::North) {
        return "north";
    } else if (dir == RotationDirection::East) {
        return "east";
    } else if (dir == RotationDirection::South) {
        return "south";
    } else if (dir == RotationDirection::West) {
        return "west";
    }
    throw;
    return nullptr;
}
auto spin_to_json(spinType spin) -> nlohmann::json {
    if (spin == spinType::null) {
        return nullptr;
    } else if (spin == spinType::mini) {
        return "mini";
    } else if (spin == spinType::null) {
        return "none";
    } else if (spin == spinType::normal) {
        return "full";
    }
    throw;
    return nullptr;
}

auto move_to_json(Move &move) {
    nlohmann::json json;
    json["location"]["x"] = move.piece.position.x;
    json["location"]["y"] = move.piece.position.y;
    json["location"]["type"] = type_to_json(move.piece.type);
    json["location"]["orientation"] = rotation_to_json(move.piece.rotation);
    json["spin"] = spin_to_json(move.piece.spin);
    return json;
}

// TBP entry point
int main() {
    // send info state to the client
    std::vector<PieceType> queue;
    int queue_index = 0;
    EmulationGame game;

    nlohmann::json message;

    message["type"] = "info";
    /*
    name 	A string, the human-friendly name of the bot. Example: "Cold Clear"
    version 	A string version identifier. Example: "Gen 14 proto 8"
    author 	A string identifying the author of the bot. Example: "SoRA_X7"
    features 	A list of supported features.*/

    message["name"] = "Nana";
    message["version"] = "0.1";
    message["author"] = "Shakkar + Awyzza";

    // send just output to stdout
    std::cout << message.dump() << std::endl;

    // accept input from stdin
    std::string str_input;
    std::getline(std::cin, str_input);

    // parse input which should be rules
    message = nlohmann::json::parse(str_input);

    // currently there really is no use for this because both the client and spec doesnt really use this

    // send the ready message to the client
    message.clear();
    message["type"] = "ready";
    std::cout << message.dump() << std::endl;

    while (true) {
        std::cin >> str_input;
        message.clear();
        message = nlohmann::json::parse(str_input);
        std::string type = message["type"];
        if (type == "quit") {
            Search::endSearch();
            break;
        } else if (type == "start") {
            /*
            The start message tells the bot to begin calculating from the specified position. This message must be sent before asking the bot for a move.
                Attribute 	Description
                hold 	Either a pieces if hold is filled or null if hold is empty.
                queue 	A list of pieces. Example: ["S", "Z", "O"]
                combo 	The number of consecutive line clears that have been made.
                back_to_back 	A boolean indicating back to back status.
                board 	A list of 40 lists of 10 board cells.
            A board cell can be either null to indicate that it is empty, or a string indicating which piece was used to fill it, or "G" for garbage cells.*/
            Board board = json_to_board(message["board"]);
            queue = json_to_queue(message["queue"]);
            PieceType hold = json_to_type(message["hold"]);
            int combo = message["combo"];
            bool back_to_back = message["back_to_back"];

            game.game.board = board;
            game.game.current_piece = queue[0];
            for (int i = 1; i < queue.size() && i < game.game.queue.size(); ++i) {
                game.game.queue[i - 1] = queue[i];
            }
            game.game.hold = hold;
            game.combo = combo;
            game.game.b2b = back_to_back;

            Search::startSearch(game, 4);
        } else if (type == "new_piece") {
            PieceType piece = json_to_type(message["piece"]);
            queue.push_back(piece);
        } else if (type == "suggest") {
            // end search and give the best move
            Search::endSearch();
            Move best_move = Search::bestMove();
            // create suggestion message
            message.clear();
            message["type"] = "move";
            message["move"] = move_to_json(best_move);

            // play the move
            game.game.place_piece(best_move.piece);
            std::cout << message.dump() << std::endl;
        }
    }
    return 0;
}