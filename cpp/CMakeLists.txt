﻿# CMakeList.txt : CMake project for cfr_tetris, include source and define
# project specific logic here.
#

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Tetris)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Tetris/game)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Tetris/game/engine)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Tetris/bot)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Tetris/util)


set(TETRIS_SOURCES
	"Tetris/game/engine/Board.hpp"
    "Tetris/util/fasthash.h"
    "Tetris/util/fasthash.cpp"
	"Tetris/game/engine/Game.cpp"
	"Tetris/game/engine/Game.hpp"
	"Tetris/game/engine/Piece.cpp"
	"Tetris/game/engine/Piece.hpp"
	"Tetris/util/rng.cpp"
	"Tetris/util/rng.hpp"
	"Tetris/game/engine/TetrisConstants.hpp"
	"Tetris/game/VersusGame.cpp"
	"Tetris/game/VersusGame.hpp"
	"Tetris/bot/Eval.cpp"
	"Tetris/bot/Eval.hpp"
	"Tetris/game/Move.cpp"
	"Tetris/game/Move.hpp"
	"Tetris/game/EmulationGame.cpp"
	"Tetris/game/EmulationGame.hpp"
	"Tetris/game/Chance.hpp"
	"Tetris/bot/UCT.cpp"
	"Tetris/bot/UCT.hpp"
	"Tetris/bot/Search.cpp"
	"Tetris/bot/Search.hpp"
	"Tetris/util/Distribution.hpp"
	"Tetris/util/MPSC.hpp"
 "Tetris/util/Distribution.cpp")
 

if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    find_package(OpenGL REQUIRED)
    find_package(PNG REQUIRED)
endif()


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")

# Add source to this project's executable.
add_executable (cfr_tetris "cfr_tetris.cpp" "cfr_tetris.h" ${TETRIS_SOURCES})

if(UNIX AND NOT APPLE)
    target_include_directories(cfr_tetris PRIVATE ${X11_INCLUDE_DIR} ${OPENGL_INCLUDE_DIRS} ${PNG_INCLUDE_DIR})

    target_link_libraries(cfr_tetris ${X11_LIBRARIES} ${OPENGL_LIBRARIES} ${PNG_LIBRARY})
endif()

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET cfr_tetris PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
