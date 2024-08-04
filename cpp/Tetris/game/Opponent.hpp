#pragma once
#include "engine/Board.hpp"
#include "util/rng.hpp"

#include <vector>

enum LayerType {
    SPIN,
    COMBO,
    MESSY,
    CLEAN,
};

enum TacticState {
    BUILD_SPIN,
    BUILD_COMBO,
    BUILD_CLEAN,
    ATTACK,
};

class StackLayer {
public:
    LayerType type;
    double height;
    // whether clean below can be accessed
    bool open = false;

    StackLayer(LayerType type, int height) : type(type), height(height) {}
    StackLayer(LayerType type, int height, bool open) : type(type), height(height), open(open) {}
};

constexpr std::array<int> comboTable = { 0, 1, 1, 2, 2, 3, 3, 3, 4, 5 };

class Opponent {
public:
    Opponent() : combo(0) {
    }

    RNG rng;

    // Probability of layers behaving as MESSY during attack, expressed as percentages.
    const MISTAKE_PROB_CLEAN = 5;
    const MISTAKE_PROB_COMBO = 5;
    const MISTAKE_PROB_SPIN = 10;

    // Probability of a spin exposing the lower garbage well
    const OPEN_SPIN_PROB = 20;

    // Abstraction of the board.
    std::vector<StackLayer> stack;

    // Represents the incoming garbage meter.
    std::vector<StackLayer> garbage;

    bool hasI = false;
    bool hasSpinPiece = false;

    int nextI;
    int nextSpinPiece;

    int combo = 0;
    TacticState state;

    void reset_rng() {
        rng.new_seed();
    }

    void nextBuild() {
        state = (TacticState)rng.getRand(4);
    }

    void nextState() {
        // Markovian state transitions
        if (rng.getRand(5) == 0)) {
            if (state != ATTACK) {
                state = ATTACK;
            }
            else {
                double height = stack.back().height;
                switch (stack.back().type) {
                case SPIN:
                    if (height >= 2) {
                        nextBuild();
                    }
                case CLEAN:
                    if (height >= 4) {
                        nextBuild();
                    }
                case COMBO:
                    if (rng.getRand(5) == 0) {
                        nextBuild();
                    }
                }
                default:
                    nextBuild();
            }
        }
    }

    void build() {
        switch (state) {
        case BUILD_COMBO:
            buildCombo(); break;
        case BUILD_SPIN:
            buildSpin(); break;
        case BUILD_CLEAN:
            buildClean(); break;
        default:
            buildClean();
        }
    }

    void buildSpin() {

        if (stack.back().type == SPIN) {
            // Continue current build
            stack.back().height += 0.45;
        }
        else {
            // Round off the current layer
            stack.back().height = std::ceil(stack.back().height);

            // Whether this spin exposes clean below it
            bool open = rng.getRand(100) < OPEN_SPIN_PROB;

            // Start building spin
            stack.push_back({ SPIN, 0.45, open });
        }

    }

    void buildCombo() {

        if (stack.back().type == COMBO) {
            // Continue current build
            stack.back().height += 0.5;
        }
        else {

            double height = stack.back().height;

            // Combo stack can borrow excess minos from the layer underneath
            double leftover = height - (int)height;

            // Round off the current layer
            stack.back().height = std::ceil(stack.back().height);

            // Start building combo
            stack.push_back({ COMBO, leftover + 0.5 });
        }
    }

    void buildClean() {
        if (stack.back().type == CLEAN) {
            // Continue current build
            stack.back().height += 0.4;
        }
        else {
            // Round off the current layer
            stack.back().height = std::ceil(stack.back().height);

            // Start building combo
            stack.push_back({ COMBO, 0.5 });
        }
    }

    double attack() {
        if (stack.empty()) return 0;

        StackLayer& topLayer = stack.back();

        int damage = 0;

        switch (topLayer.type) {
        case SPIN:
            damage = 4;
            reduceLayerHeight(topLayer, 2);
            break;
        case CLEAN:
            damage = 4;
            reduceLayerHeight(topLayer, 4);
            break;
        case MESSY:
            damage = 0;
            if (topLayer.height > 4) {
                reduceLayerHeight(topLayer, 0.35);
            }
            reduceLayerHeight(topLayer, 0.7);
            break;
        case COMBO:
            if (combo >= comboTable.size()) {
                damage = 5;
            }
            else {
                damage = comboTable[combo];
            }
            reduceLayerHeight(topLayer, 1);
            combo++;
            break;
        default:
            break;
        }

        return damage;
    }

    void reduceLayerHeight(StackLayer& layer, double amount) {
        layer.height -= amount;
        if (layer.height <= 0) {
            stack.pop_back();
        }
    }

    void convertGarbage() {
        while (!garbage.empty()) {
            StackLayer& garbageLayer = garbage.back();
            stack.insert(stack.begin(), garbageLayer); // Add garbage layer to the bottom of the stack
            garbage.pop_back();
        }
    }

    void receiveAttack(int amount) {
        // Add garbage layers to the garbage meter
        if (amount < 4) {
            garbage.push_back({ MESSY, amount });
        }
        else {
            garbage.push_back({ CLEAN, amount });
        }
    }

    // Advances internal state and returns outgoing damage this turn
    double playMove() {
        nextState();
        build();
        if (state == ATTACK) {
            return attack();
        }
    }
};
