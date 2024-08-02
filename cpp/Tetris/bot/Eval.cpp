#include "Eval.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

#include "engine/Board.hpp"
#include "engine/Game.hpp"
#include "Move.hpp"

using lut_int = uint64_t;
struct entry {
    size_t index;
    lut_int data;
};

using DyLUT_h_t = std::vector<entry>;
using DyLUT_v_t = std::vector<entry>;

namespace Eval {
    DyLUT_h_t LUT_h;
    DyLUT_v_t LUT_v;

    std::unordered_map<uint64_t, uint64_t> LUT_h_map;
    std::unordered_map<uint64_t, uint64_t> LUT_v_map;

    void init();
    bool needs_init = true;
}  // namespace Eval

static size_t hashh(size_t left, size_t middle, size_t right, size_t height) {
    return left | (middle << 9) | (right << 18) | (height << 27);
}

static size_t hashv(size_t bottom, size_t top, size_t height) {
    return bottom | (top << 9) | (height << 18);
}

static void deserializeDyLUTV(const char* filename) {
    std::ifstream file(filename, std::ios::binary);
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    Eval::LUT_v.resize(size / sizeof(entry));
    file.read(reinterpret_cast<char*>(Eval::LUT_v.data()), size);
}

static void deserializeDyLUTH(const char* filename) {
    std::ifstream file(filename, std::ios::binary);
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    Eval::LUT_h.resize(size / sizeof(entry));
    file.read(reinterpret_cast<char*>(Eval::LUT_h.data()), size);
}

void Eval::init() {
    deserializeDyLUTV("DLUTV.bin");
    deserializeDyLUTH("DLUTH.bin");

    for (size_t i = 0; i < LUT_h.size(); i++)
        LUT_h_map[LUT_h[i].index] = LUT_h[i].data;

    for (size_t i = 0; i < LUT_v.size(); i++)
        LUT_v_map[LUT_v[i].index] = LUT_v[i].data;

    LUT_h.clear();
    LUT_v.clear();
}

static double logfactorial(double n) {
    double sum = 0;
    for (int i = 1; i <= n; i++)
        sum += log(i);
    return sum;
}

double Eval::eval_LUT(const Board& board) {
    if (needs_init) {
        init();
        needs_init = false;
    }
    [[maybe_unused]] auto get_3x3s = [](Board board, int x, int y) {
        uint16_t bits = 0;

        for (int wx = 0; wx < 3; ++wx)
            for (int wy = 0; wy < 3; ++wy) {
                bool filled = false;
                if ((x + wx) < 0 || (x + wx) >= Board::width)
                    filled = true;
                else
                    filled = board.get(x + wx, y - wy);

                if (filled)
                    bits |= 1 << (wy + wx * 3);
            }
        return bits;
        };

    auto get_3x3 = [](Board board, size_t x, size_t y) {
        uint16_t bits = 0;

        for (size_t wx = 0; wx < 3; ++wx) {
            uint32_t filled = 0;
            if ((x + wx) < 0 || (x + wx) >= Board::width)
                filled = 0b111;
            else
                filled = board.get_column(x + wx);
            filled &= 0b111 << (y - 2);
            filled >>= (y - 2);

            if (filled == 0b001) {
                filled = 0b100;
            }
            else if (filled == 0b011) {
                filled = 0b110;
            }
            else if (filled == 0b100) {
                filled = 0b001;
            }
            else if (filled == 0b110) {
                filled = 0b011;
            }

            bits |= filled << (wx * 3);
        }
        return bits;
        };

    double score = 1.0;
    struct fentry {
        size_t index;
        double data;
        bool operator<(const fentry& other) const {
            return index < other.index;
        }
    };

    struct Comp {
        bool operator()(const fentry& a, size_t b) const {
            return a.index < b;
        }
        bool operator()(size_t a, const fentry& b) const {
            return a < b.index;
        }
        bool operator()(const fentry& a, const fentry& b) const {
            return a.index < b.index;
        }
    };
    std::vector<fentry> freq_h;
    freq_h.reserve(90);
    std::vector<fentry> freq_v;
    freq_v.reserve(144);

    // note: can trade 2x speed for 30% quality drop by striding horizontal x by 2 and vertical y by 2

    for (size_t x = 1; x < Board::width - 4; x += 1)
        for (size_t y = 2; y < 20; y += 1) {
            size_t left = get_3x3(board, x - 3, y);
            size_t middle = get_3x3(board, x, y);
            size_t right = get_3x3(board, x + 3, y);

            size_t effective_y = y;

            // account for lack of data at the top
            if (y > 10) {
                effective_y -= 10;
            }

            bool exists = std::binary_search(freq_h.begin(), freq_h.end(), hashh(left, middle, right, effective_y), Comp{});
            if (!exists)
                freq_h.push_back({hashh(left, middle, right, effective_y), 1.0});
            else {
                auto it = std::lower_bound(freq_h.begin(), freq_h.end(), hashh(left, middle, right, effective_y), Comp{});
                it->data += 1.0;
            }
        }

    for (size_t x = 0; x < Board::width - 2; x += 1)
        for (size_t y = 2; y < 20; y += 2) {
            size_t bottom = get_3x3(board, x, y);
            size_t top = get_3x3(board, x, y + 3);

            size_t effective_y = y;

            // account for lack of data at the top
            if (y > 10) {
                effective_y -= 10;
            }

            bool exists = std::binary_search(freq_v.begin(), freq_v.end(), hashv(bottom, top, effective_y), Comp{});
            if (!exists)
                freq_v.push_back({hashv(bottom, top, effective_y), 1.0});
            else {
                auto it = std::lower_bound(freq_v.begin(), freq_v.end(), hashv(bottom, top, effective_y), Comp{});
                it->data += 1.0;
            }
        }

    for (auto& [key, f_k] : freq_h) {
        double frequency = LUT_h_map.find(key) != LUT_h_map.end() ? LUT_h_map[key] : 0.0;
        // clip at 3 to prevent negatives
        double LUT_K = std::max((double)frequency, 3.0);

        score += f_k * log(LUT_K) - logfactorial(f_k);
    }

    for (auto& [key, f_k] : freq_v) {
        double frequency = LUT_v_map.find(key) != LUT_v_map.end() ? LUT_v_map[key] : 0.0;
        // clip at 3 to prevent negatives
        double LUT_K = std::max((double)frequency, 3.0);
        score += f_k * log(LUT_K) - logfactorial(f_k);
    }

    return score;
}

static bool Eval::is_top_quarter(const Board& board) {
    // do NOT touch anything this vectorizes
    constexpr uint32_t top_quarter_collider = ~((1 << 15) - 1);

    bool ret = false;

    for (size_t x = 0; x < Board::width; x++) {
        if (board.get_column(x) & top_quarter_collider)
            ret = true;
    }
    return ret;
}

static bool Eval::is_top_half(const Board& board) {
    // do NOT touch anything this vectorizes
    constexpr uint32_t top_half_collider = ~((1 << 10) - 1);

    bool ret = false;

    for (size_t x = 0; x < Board::width; x++) {
        if (board.get_column(x) & top_half_collider)
            ret = true;
    }
    return ret;
}


static bool is_low(const Board& board) {
    constexpr uint32_t high_collider = ~((1 << 4) - 1);

    bool ret = true;

    for (size_t x = 0; x < Board::width; x++) {
        if (board.get_column(x) & high_collider)
            ret = false;
    }
    return ret;
}

static bool is_perfect_clear(const Board& board) {

    bool ret = true;

    for (size_t x = 0; x < Board::width; x++) {
        if (board.get_column(x))
            ret = false;
    }
    return ret;
}

std::pair<int, int> Eval::cavities_overhangs(const Board& board) {
    int cavities = 0;
    int overhangs = 0;

    for (int i = 0; i < Board::width; ++i) {
        auto col = board.board[i];
        if (col == 0) {
            continue;
        }
        col = ~col;
        col = col << std::countl_one(col);
        cavities += std::popcount(col);
    }

    for (int i = 1; i < Board::width; ++i) {
        auto col1 = board.board[i - 1];
        auto col2 = board.board[i];
        col1 = col1 >> (32 - std::countl_zero(col2));
        if (col1 == 0) {
            continue;
        }
        col1 = ~col1;
        col1 = col1 << std::countl_one(col1);
        overhangs += std::popcount(col1);
    }

    return { cavities, overhangs };
}

int Eval::well_position(const Board& board) {

    int max_air = -1;
    int well_position = 0;

    for (int i = 0; i < Board::width; ++i) {
        auto& col = board.board[i];
        int air = std::countl_zero(col);
        if (air > max_air) {
            max_air = air;
            well_position = i;
        }
    }

    return well_position;
}

// lowest height, highest height
std::pair<int, int> Eval::height_features(const Board& board) {

    // air is the complement of height

    int max_air = -1;
    int min_air = 1 << 30;

    for (int i = 0; i < Board::width; ++i) {
        auto& col = board.board[i];
        int air = std::countl_zero(col);
        max_air = std::max(air, max_air);
        min_air = std::min(air, min_air);
    }

    return { 32 - max_air, 32 - min_air };
}

std::pair<int, int> Eval::n_covered_cells(Board board) {
    int covered = 0;
    int covered_sq = 0;

    for (int i = 0; i < Board::width; ++i) {
        auto col = board.board[i];
        int col_covered = 0;
        int zeros = std::countl_zero(col);
        col <<= zeros; // 00011101 -> 11101
        int ones = std::countl_one(col);
        if (zeros + ones != 32) {
            col_covered += col_covered + ones; 
        }
        covered += col_covered;
        covered_sq += col_covered * col_covered;
    }

    return { covered, covered_sq };
}

static auto abss(auto num) {
    if (num < 0)
        return -num;
    else
        return num;

}

static std::pair<int, int> Eval::get_bumpiness(const Board& board) {
    int bumpiness = 0;
    int bumpiness_sq = 0;

    std::array<int, Board::width> air;

    for (int i = 0; i < Board::width; ++i) {
        air[i] = std::countl_zero(board.board[i]);
    }

    std::array<int, Board::width> bump;


    for (int i = 1; i < Board::width; ++i) {
        bump[i] = abss(air[i] - air[i - 1]);
    }

    for (int i = 1; i < Board::width; ++i) {
        bumpiness += bump[i];
        bumpiness_sq += bump[i] * bump[i];

    }
    return { bumpiness, bumpiness_sq };
}

bool has_v(const Board& board, int min_height, int max_height) {
    uint32_t col1 = 0b001;
    uint32_t col2 = 0b000;
    uint32_t col3 = 0b001;

    uint32_t mask = 0b111;

    bool ret = false;

    for (int m = 0; m < 2; m++) {
        for (int y = min_height; y < max_height - 3; y++) {
            for (int i = 0; i < Board::width - 3; ++i) {
                auto& b_col1 = board.board[i];
                auto& b_col2 = board.board[i + 1];
                auto& b_col3 = board.board[i + 2];
                bool tsd = true;
                tsd = tsd && (((b_col2 & (mask << y)) >> y) == col2);
                tsd = tsd && (((b_col1 & (mask << y)) >> y) == col1);
                tsd = tsd && (((b_col3 & (mask << y)) >> y) == col3);
                if (tsd) {
                    return true;
                }
            }
        }
        std::swap(col1, col3);
    }

    return ret;
}

bool Eval::has_tsd(const Board& board, int min_height, int max_height) {
    uint32_t col1 = 0b101;
    uint32_t col2 = 0b000;
    uint32_t col3 = 0b001;

    uint32_t mask = 0b111;

    bool ret = false;

    for (int m = 0; m < 2; m++) {
        for (int y = min_height; y < max_height - 3; y++) {
            for (int i = 0; i < Board::width - 3; ++i) {
                auto& b_col1 = board.board[i];
                auto& b_col2 = board.board[i + 1];
                auto& b_col3 = board.board[i + 2];
                bool tsd = true;
                tsd = tsd && (((b_col2 & (mask << y)) >> y) == col2);
                tsd = tsd && (((b_col1 & (mask << y)) >> y) == col1);
                tsd = tsd && (((b_col3 & (mask << y)) >> y) == col3);
                if (tsd) {
                    return true;
                }
            }
        }
        std::swap(col1, col3);
    }

    return ret;
}

// Identify clean count to 4
static bool Eval::ct4(const Board& board) {

    int garbage_height = height_features(board).first;

    bool quad_ready = false;

    for (int i = 0; i < Board::width; ++i) {
        auto& col = board.board[i];
        quad_ready = quad_ready && ((((col >> garbage_height) | 0b1111) & 0b1111) == 0b1111);
    }

    if (!quad_ready) {
        return false;
    }

    if (garbage_height == 0) {
        return true;
    }

    for (int i = 0; i < Board::width; ++i) {
        if (!board.get(i, garbage_height - 1)) {
            // check we have exactly 4 rows filled above this hole
            if (std::countl_zero(board.board[i]) != 28 - garbage_height) {
                return false;
            }
        }
    }

    return true;
}

double Eval::eval_CC(const Board& board, int lines, bool tspin, bool waste_t) {
    constexpr auto top_half = -0.0;
    constexpr auto top_quarter = -120.0;
    constexpr auto low = -5.0;
    constexpr auto cavity_cells = -173.0;
    constexpr auto cavity_cells_sq = -3.0;
    constexpr auto overhangs = -47.0;
    constexpr auto overhangs_sq = -9.0;
    constexpr auto covered_cells = -17.0;
    constexpr auto covered_cells_sq = -1.0;
    constexpr auto bumpiness = -24.0;
    constexpr auto bumpiness_sq = -7.0;
    constexpr auto height = -37.0;
    constexpr float well_columns[10] = { 20, 23, 20, 50, 59, 21, 59, 10, -10, 24 };
    constexpr float clears[5] = { 40, -160, -140, -170, 490 };
    constexpr float tspins[4] = { 0, 231, 520, 728 };
    constexpr float perfect_clear = 300.0;
    constexpr float wasted_t = -152.0;
    constexpr float tsd_shape = 220.0;
    constexpr float v_shape = 70.0;
    constexpr float counting = 80.0;

    double score = 0.0;

    std::pair<int, int> values;

    if (is_top_half(board))
        score += top_half;

    if (is_top_quarter(board))
        score += top_quarter;

    if (is_low(board))
        score += low;

    if (is_perfect_clear(board))
        score += perfect_clear;


    values = cavities_overhangs(board);

    score += values.first * cavity_cells;

    score += values.first * values.first * cavity_cells_sq;

    score += values.second * overhangs;

    score += values.second * values.second * overhangs_sq;

    values = height_features(board);

    score += values.second * height;

    if (has_tsd(board, values.first, values.second)) {
        score += tsd_shape;
    }
    else if (has_v(board, values.first, values.second)) {
        score += v_shape;
    }

    if (ct4(board)) {
        score += counting;
    }

    values = n_covered_cells(board);

    score += values.first * covered_cells;
    score += values.second * covered_cells_sq;

    values = get_bumpiness(board);

    score += values.first * bumpiness;
    score += values.second * bumpiness_sq;

    score += well_columns[well_position(board)];
    score += clears[lines];

    if (tspin) {
        score += tspins[lines];
    }

    if (waste_t) {
        score += wasted_t;
    }

    return (score + 10000) / 10000;
}

double Eval::eval_CC(Game game, Move move) {
    game.place_piece(move.piece);
    int lines = game.board.clearLines();
    spinType spin = move.piece.spin;
    bool tspin = false;
    if (spin == spinType::normal) {
        tspin = true;
    }

    bool waste_t = false;

    if (move.piece.type == PieceType::T) {
        if (!tspin) {
            waste_t = true;
        }
    }

    return eval_CC(game.board, lines, tspin, waste_t);
}