<p align="center">
<img class="only-light" height="150px" src="./logo.png"/>
</p>
<br>

# Nana

--------------------------

Nana is a parallel search program for two-player stacking games, such as Versus Tetris. 

The program is somewhat general, and designed for similar games that have any or all of the following properties:

* Zero-sum games
* Large branching factors
* Random chance
* Hidden information
* Efficient parallelisation
* Online any-time planning


## How It Works

Nana at its core is based on the combination of two search techniques; Monte Carlo Tree Search (MCTS)[[1]](#1), and Transposition Driven Scheduling (TDS)[[2]](#2).

MCTS operates on the premise that we can generate an analysis of the game's local search space by phrasing states and actions as individual Multi Arm Bandit problems, and repeatedly performing Upper Confidence Bound selection of the actions which provide the best balance of exploitation and exploration. This method is also known as Upper Confidence bound applied to Trees, or UCT search.[[1]](#1)

Transposition Driven Scheduling is a load balancing technique that allows a program to efficiently scale to many threads while searching for solutions, and has historically been applied to puzzle games. It works by having the program globally agree to allocate ownership of certain gamestates to certain threads, based on the state's hash. This means that any thread can calculate who the owner is of a state that has never been seen before. 

### Work Witholding

One of the challenges specifically of searching Tetris using TDS-MCTS is that computing legal actions is very costly and highly non-uniform. In some states, it is a trivial computation while in others, it can take a long time. This means that existing TDS-MCTS methods such as those by Yoshizoe et al.[[3]](#3)[[4]](#4) encounter significant load balancing and workload congestion issues when applied directly.

To address this, Nana uses a technique similar to Work Stealing that we call Work Witholding. Essentially, it allows threads who would otherwise exhaust their own job queue to 'steal' work that would otherwise be sent to neighbouring threads. This advances typical TDS-MCTS to an algorithm that has guaranteed progress both per-thread and system wide.


## Usage

To use, Nana needs to be built from its C++ source code using CMake, i.e.

```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
```

It supports a standard JSON-based interface for bots playing Tetris-like games, known as the Tetris Bot Protocol: https://github.com/tetris-bot-protocol. This version is compiled to a ``tbp`` executable.

There is also a standalone, graphical version of the bot that requires SDL2.

## References

1. https://en.wikipedia.org/wiki/Monte_Carlo_tree_search
2. https://en.wikipedia.org/wiki/Transposition-driven_scheduling
3. Scalable Distributed Monte-Carlo Tree Search, Yoshizoe et al. 2011 (https://www.graco.c.u-tokyo.ac.jp/~kaneko/papers/socs2011pmcts.pdf)
4. Practical Massively Parallel Monte-Carlo Tree Search Applied to Molecular Design, Yang et al. 2020 (https://arxiv.org/pdf/2006.10504)



