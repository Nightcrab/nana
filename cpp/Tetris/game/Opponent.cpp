#include "Opponent.hpp"
#include <bit>
#include <ostream>

int get_row_transitions(uint32_t row) {
    // todo
    // use row transitions to detect messy layer
}

Opponent::Opponent (const Game& game) {

    // scan through the rows of the board and create a matching Opponent model

    stack.clear();

    uint32_t rows[20] = { 0 };
    for (int y = 0; y < 20; y++) {
        for (int x = 0; x < 10; x++) {
            rows[y] = rows[y] | (game.board.get(x, y) << x);
        }
        if (rows[y] == 0) {
            break;
        }
    }

    StackLayer currentLayer = { COMBO, 0 };
    for (int i = 0; i < 16;i++) {
        if (rows[i] == 0) {
            break;
        }
        else if ((rows[i + 1] & rows[i + 2] & rows[i + 3]) == rows[i]) {
            stack.push_back(currentLayer);
            currentLayer = StackLayer();
            currentLayer.type = CLEAN;
            currentLayer.height += 4;
            i += 3;
        }
        else if ((rows[i + 1] & rows[i + 2]) == rows[i]) {
            stack.push_back(currentLayer);
            currentLayer = StackLayer();
            currentLayer.type = CLEAN;
            currentLayer.height += 3;
            i += 2;
        }
        else if (std::popcount(rows[i]) == 9) {
            currentLayer.height += 1;
        }
        else if (std::popcount(rows[i]) < 7) {
            if (currentLayer.type != COMBO) {
                stack.push_back(currentLayer);
                currentLayer = StackLayer({ MESSY, 0 });
            }
            currentLayer.type = COMBO;
            currentLayer.height += 1;
        }
        else if (std::popcount(rows[i]) <= 8 || std::popcount(rows[i + 1]) <= 8) {
            if (currentLayer.type != SPIN) {
                stack.push_back(currentLayer);
                currentLayer = StackLayer({ MESSY, 0 });
            }
            currentLayer.type = SPIN;
            currentLayer.height += 1;
        }
        else {
            stack.push_back(currentLayer);
        }
    }

    if (stack.empty()) {
        state = BUILD_SPIN;
    }
    else {
        state = (TacticState)stack.back().type;
    }
}
