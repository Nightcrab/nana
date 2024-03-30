# CMakeList.txt : CMake project for cfr_tetris, include source and define
# project specific logic here.
#

set(TETRIS_SOURCES
	"Tetris/Board.hpp"
    "Tetris/fasthash.h"
    "Tetris/fasthash.cpp"
	"Tetris/Game.cpp"
	"Tetris/Game.hpp"
	"Tetris/Piece.cpp"
	"Tetris/Piece.hpp"
	"Tetris/rng.cpp"
	"Tetris/rng.hpp"
	"Tetris/TetrisConstants.hpp"
	"Tetris/VersusGame.cpp"
	"Tetris/VersusGame.hpp"
	"Tetris/Eval.cpp"
	"Tetris/Eval.hpp"
	"Tetris/Move.cpp"
	"Tetris/Move.hpp"
	"Tetris/EmulationGame.cpp"
	"Tetris/EmulationGame.hpp"
	"Tetris/Chance.hpp"
	"Tetris/UCT.cpp"
	"Tetris/UCT.hpp"
	"Tetris/Search.cpp"
	"Tetris/Search.hpp"
	"Tetris/Distribution.hpp"
)


if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    find_package(OpenGL REQUIRED)
    find_package(PNG REQUIRED)
    if(CMAKE_CXX_FLAGS_RELEASE)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=x86-64-v3")
    endif()
endif()

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
