# CMakeList.txt : CMake project for cfr_tetris, include source and define
# project specific logic here.
#

set(TETRIS_SOURCES
	"Tetris/Board.hpp"
	"Tetris/Game.cpp"
	"Tetris/Game.hpp"
	"Tetris/Piece.cpp"
	"Tetris/Piece.hpp"
	"Tetris/rng.cpp"
	"Tetris/rng.hpp"
	"Tetris/TetrisConstants.hpp"
	"Tetris/VersusGame.cpp"
	"Tetris/Eval.cpp"
	"Tetris/CFR.cpp"
	"Tetris/Search.cpp"
	"Tetris/Search.hpp"
	"Tetris/Move.cpp"
	"Tetris/Move.hpp"
	"Tetris/EmulationGame.cpp"
	"Tetris/EmulationGame.hpp"
)

# Add source to this project's executable.
add_executable (cfr_tetris "cfr_tetris.cpp" "cfr_tetris.h" ${TETRIS_SOURCES})

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET cfr_tetris PROPERTY CXX_STANDARD 23)
endif()

# TODO: Add tests and install targets if needed.
