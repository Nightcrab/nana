import numba as nb
import numba.experimental as nbe
from numba import types
from numba.cpython import mathimpl
from numba.extending import intrinsic
import numpy as np
import raylib as rl
import os

from numba import int8, int16, int32, uint32, uint64, float32

from umap import UMap

import time

PPTRNG = 3

def GetRand(upperBound: int):
    global PPTRNG
    PPTRNG = PPTRNG * 0x5D588B65 + 0x269EC3
    PPTRNG = PPTRNG % (2**32)
    uVar1: int = PPTRNG >> 0x10

    if upperBound != 0:
        uVar1 = uVar1 * upperBound >> 0x10
    return uVar1



class PieceType:
    S = 0
    Z = 1
    J = 2
    L = 3
    T = 4
    O = 5
    I = 6
    Empty = 7
    N = 8


class RotationDirection:
    North = 0
    East = 1
    South = 2
    West = 3
    N = 4


class TurnDirection:
    Left = 0
    Right = 1


class piecedefinitions:

    pieces = [
        [-1, 0, 0, 0, 0, 1, 1, 1],  # S
        [-1, 1, 0, 1, 0, 0, 1, 0],  # Z
        [-1, 0, 0, 0, 1, 0, -1, 1],  # J
        [-1, 0, 0, 0, 1, 0, 1, 1],  # L
        [-1, 0, 0, 0, 1, 0, 0, 1],  # T
        [0, 0, 1, 0, 0, 1, 1, 1],  # O
        [-1, 0, 0, 0, 1, 0, 2, 0],  # I
        [0, 0, 0, 0, 0, 0, 0, 0],  # NULL
    ]


class Movement:
    Left = 0
    Right = 1
    RotateClockwise = 2
    RotateCounterClockwise = 3
    SonicDrop = 4
    HardDrop = 5


# number of kicks srs has, including for initial
srs_kicks = 5

piece_offsets_JLSTZ = [
    [0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
    [0, 0, 1, 0, 1, -1, 0, 2, 1, 2],
    [0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
    [0, 0, -1, 0, -1, -1, 0, 2, -1, 2],
]

piece_offsets_I = [
    [0, 0, -1, 0, 2, 0, -1, 0, 2, 0],
    [-1, 0, 0, 0, 0, 0, 0, 1, 0, -2],
    [-1, 1, 1, 1, -2, 1, 1, 0, -2, 0],
    [0, 1, 0, 1, 0, 1, 0, -1, 0, 2],
]

def jitmethod(fn):
    return staticmethod(nb.njit(fn))

@nb.njit([
    int16(int16, int16),
    int32(int32, int32)
])
def xor_hash(a: int, b: int):
    return a ^ (b + 0x9e3779b9 + (a << 6) + (a >> 2))


def tuple_hash(t: tuple):
    out = t[0]
    for i in range(len(tuple) - 1):
        out = xor_hash(out, out[i + 1])        
    return out

@nb.njit(int32(int32[:]))
def array_hash_1d(array):
    out = np.zeros(1, dtype="int32")
    for i in range(array.shape[0]):
        out[0] = xor_hash(out[0], array[i])
    return out[0]


class Piece:

    def __init__(self, type: int, as_empty=False):

        self.type = type

        if as_empty:
            return

        self.pos = np.zeros(2, dtype="int32")
        self.pos[0] = 4
        self.pos[1] = 18
        self.rotation = RotationDirection.North
        self.minos: np.array(8, dtype="int32") = np.copy(piecedefinitions.pieces[type])

    @jitmethod
    def _rotate_right(rotation, minos):
        rotation = (rotation + 1) % 4
        for i in range(4):
            mino = np.array((minos[i * 2], minos[i * 2 + 1]))
            temp_mino = mino
            temp_mino[0] *= -1
            mino = np.array((temp_mino[1], temp_mino[0]))
            minos[i * 2] = mino[0]
            minos[i * 2 + 1] = mino[1]
        return rotation

    def rotate_right(self):
        self.rotation = Piece._rotate_right(self.rotation, self.minos)

    @jitmethod
    def _rotate_left(rotation, minos):
        rotation = (rotation + 3) % 4
        for i in range(4):
            mino = np.array((minos[i * 2], minos[i * 2 + 1]))
            temp_mino = mino
            temp_mino[1] *= -1
            mino = np.array((temp_mino[1], temp_mino[0]))
            minos[i * 2] = mino[0]
            minos[i * 2 + 1] = mino[1]
        return rotation

    def rotate_left(self):
        self.rotation = Piece._rotate_left(self.rotation, self.minos)

    def hash(self):
        return (
            (self.type << 24)
            | (self.pos[0] << 16)
            | (self.pos[1] << 8)
            | (self.rotation)
        )

    def as_tuple(self):
        return (self.type, self.pos[0], self.pos[1], self.rotation)

    def copy(self):
        new_piece = Piece(self.type, as_empty=True)
        new_piece.pos = np.copy(self.pos)
        new_piece.rotation = self.rotation
        new_piece.minos = np.copy(self.minos)
        return new_piece

    def __repr__(self):
        return repr(self.as_tuple())


@intrinsic
def popcnt(typingctx, src):
    sig = types.uint32(types.uint32)

    def codegen(context, builder, signature, args):
        return mathimpl.call_fp_intrinsic(builder, "llvm.ctpop.i32", args)

    return sig, codegen


@intrinsic
def _ctlz(typingctx, src):
    sig = types.uint32(types.uint32)

    def codegen(context, builder, signature, args):
        return mathimpl.call_fp_intrinsic(builder, "llvm.ctlz.i32", args)

    return sig, codegen


@nb.njit(nb.uint32(nb.uint32))
def popcount(x):
    return popcnt(x)


@nb.njit(nb.uint32(nb.uint32))
def ctlz(x):
    return _ctlz(x)


@nb.njit(nb.uint32(nb.int32, nb.int32))
def pext(x, mask):
    DEST = 0
    m = 0
    k = 0
    while m < 32:
        if mask & (1 << m) != 0:
            DEST |= (x & (1 << m)) >> (m - k)
            k = k + 1
        m = m + 1
    return DEST


class Board:

    def __init__(self):
        # fill board with 0s
        self.board = np.zeros(10, dtype="uint32")

    @jitmethod
    def _clear_lines(board):
        mask = 0xFFFFFFFF

        for col in board:
            mask &= col

        lines_cleared = popcount(mask)

        mask = ~mask

        for i in range(10):
            board[i] = pext(board[i], mask)

        return lines_cleared

    def clear_lines(self):
        return Board._clear_lines(self.board)

    @jitmethod
    def _set(board, pos, minos):
        for i in range(4):
            x = pos[0] + minos[i * 2]
            y = pos[1] + minos[i * 2 + 1]
            board[x] |= 1 << y

    def set(self, piece: Piece):
        return Board._set(self.board, piece.pos, piece.minos)

    def copy(self):
        out = Board()
        out.board = np.copy(self.board)
        return out

    @jitmethod
    def hash(board):
        out = 0
        for i in range(10):
            out = xor_hash(out, board[i])
        return out


def factorial(x):
    if x == 1:
        return 1
    if x < 1:
        raise Exception("tried to calculate x! where x <= 0")
    return x * factorial(x - 1)


@nb.njit(float32(int32))
def logfactorial(x):
    out = 0
    for i in range(x):
        out += np.log(i + 1)
    return out


@nb.njit(float32(float32, float32))
def logscore(f_k, LUT_k):
    return f_k * np.log(LUT_k) - logfactorial(f_k)


class Eval:
    BOARD_WIDTH = 10
    VISUAL_BOARD_HEIGHT = 20

    def __init__(self, h_lut, v_lut):
        self.h_lut = h_lut
        self.v_lut = v_lut

    @jitmethod
    def _get3x3(board: uint32[:], BOARD_WIDTH, x, y):
        out = 0
        for i in range(3):
            for j in range(3):

                if x + i < 0 or x + i > BOARD_WIDTH:
                    out = (out << 1) | 1
                    continue

                out = (out << 1) | board_get(board, x + i, y - j)

        return out

    def get3x3(self, board, x, y):
        return Eval._get3x3(board.board, Eval.BOARD_WIDTH, x, y)

    def eval_board_v(self, board):
        # height specific
        f = {}

        x = 0
        while (x < Eval.BOARD_WIDTH - 2):
            y = 2
            while (y < Eval.VISUAL_BOARD_HEIGHT):

                bottom = self.get3x3(board, x, y)
                top = self.get3x3(board, x, y + 3)

                index = (bottom, top, y - 2)

                if index in f:
                    f[index] += 1.0
                else:
                    f[index] = 1.0

                y += 1
            x += 1

        log_score = 1

        for index in f.keys():
            f_k = f[index]
            # clip at 3 to prevent negatives
            LUT_k = max(self.v_lut.get(*index), 3.0)
            log_score += logscore(f_k, LUT_k)

        return log_score

    def eval_board1(self, board):
        # height specific
        f = {}

        x = 2
        while (x < Eval.BOARD_WIDTH - 5):
            y = 2
            while (y < Eval.VISUAL_BOARD_HEIGHT):

                left = self.get3x3(board, x - 3, y)
                middle = self.get3x3(board, x, y)
                right = self.get3x3(board, x + 3, y)

                index = (left, middle, right, y - 2)

                if index in f:
                    f[index] += 1.0
                else:
                    f[index] = 1.0

                y += 1
            x += 1

        log_score = 1

        for index in f.keys():
            f_k = f[index]
            # clip at 3 to prevent negatives
            LUT_k = max(self.h_lut.get(*index), 3.0)
            log_score += f_k * np.log(float(LUT_k)) - np.log(factorial(f_k))

        return log_score

    def eval_board(self, board):
        return self.eval_board1(board) + self.eval_board_v(board)


@nb.njit(int8(uint32[:], int8, int8))
def board_get(board, x, y):
    return board[x] & (1 << y) != 0


@nb.njit(int8(uint32[:], int8, int8))
def board_get_index(board, x, y):
    bits: int = 0

    for wx in range(3):
        for wy in range(3):
            filled: bool = False
            if (x + wx) < 0 or (x + wx) > 9:
                filled = True
            else:
                filled = board_get(board, x + wx, y - wy)
            bits |= (board[(x + wx)] & (1 << (y + wy))) != 0
    return bits


class Lut_H:
    def __init__(self, filepath):
        file = open(filepath, "rb").read()
        lut = np.frombuffer(file, dtype="uint64")
        size_bytes = os.path.getsize(filepath)
        self.map = UMap(size=10000000)

        for l in range(lut.shape[0] // 2):
            index = lut[l * 2]
            freq = lut[l * 2 + 1]
            self.map.put(index, freq)
                
    def get(self, left: int, middle: int, right: int, height: int):
        return self.map[left | (middle << 9) | (right << 18) | (height << 27)]


class Lut_V:
    def __init__(self, filepath):
        file = open(filepath, "rb").read()
        lut = np.frombuffer(file, dtype="uint64")
        self.map = UMap(size=10000000)

        for l in range(lut.shape[0] // 2):
            index = lut[l * 2]
            freq = lut[l * 2 + 1]
            self.map.put(index, freq)
                
    def get(self, bottom: int, top: int, height: int):
        return self.map[bottom | (top << 9) | (height << 18)]


QUEUE_SIZE = 5


class Game:

    def __init__(self):

        self.bag = [0] * 7
        self.bagiterator = 7

        self.board: Board = Board()
        self.current_piece: Piece = Piece(self.getPiece())
        self.hold: Piece | None = None
        self.held: bool = False
        self.queue = np.zeros(QUEUE_SIZE, "int8")


        for i in range(QUEUE_SIZE):
            self.queue[i] = self.getPiece()

        self.garbage_meter: int = 0
        self.b2b: int = 0
        self.combo: int = 0

        # tetrio stuff
        self.currentcombopower: int = 0
        self.currentbtbchainpower: int = 0
        self.b2bchaining: bool = True
        self.garbagemultiplier: int = 1

        
    def makebag(self):
        self.bagiterator = 0
        self.buffer = 0

        pieces = [0, 1, 2, 3, 4, 5, 6]
        i = 6
        while i >= 0:
            self.buffer = GetRand(i + 1)
            self.bag[i] = pieces[self.buffer]
            temp = pieces[self.buffer]
            pieces[self.buffer] = pieces[i]
            pieces[i] = temp
            i -= 1


    def getPiece(self):
        if self.bagiterator == 7:
            self.makebag()
        self.bagiterator += 1
        return self.bag[self.bagiterator - 1]


    def movegen(self, piece_type: int):

        t = time.time()
        initial_piece = Piece(piece_type)

        open_nodes = []
        next_nodes = []
        visited = {}

        valid_moves = []

        # root node
        open_nodes.append(initial_piece)

        while len(open_nodes) > 0:

            # expand edges
            for piece in open_nodes:
                h = piece.hash()
                if h in visited:
                    continue

                # mark node as visited
                visited[h] = True

                # try all movements
                new_piece = piece.copy()
                self.process_movement(Movement.RotateCounterClockwise, piece=new_piece)
                next_nodes.append(new_piece)

                new_piece = piece.copy()
                self.process_movement(Movement.RotateClockwise, piece=new_piece)
                next_nodes.append(new_piece)

                new_piece = piece.copy()
                self.process_movement(Movement.Left, piece=new_piece)
                next_nodes.append(new_piece)

                new_piece = piece.copy()
                self.process_movement(Movement.Right, piece=new_piece)
                next_nodes.append(new_piece)

                new_piece = piece.copy()
                self.process_movement(Movement.SonicDrop, piece=new_piece)
                next_nodes.append(new_piece)

                if self.grounded(piece):
                    valid_moves.append(piece)

            open_nodes = next_nodes
            next_nodes = []

        #print("movegen took", time.time() - t)

        return valid_moves

    def place_random_valid_piece(self, i: int):
        valid_pieces = self.movegen(self.current_piece.type)

        if len(valid_pieces) == 0:
            return

        if i < 0:
            i = len(valid_pieces) - 1
        if i >= len(valid_pieces):
            i = 0
        self.current_piece = valid_pieces[i]

        return i

    def place_best_piece(self, eval: Eval):
        self.current_piece = self.get_best_piece(eval)

    def get_best_piece(self, eval):
        valid_pieces = self.movegen(self.current_piece.type)

        if self.hold is not None:
            valid_pieces = [*valid_pieces, *self.movegen(self.hold.type)]
        else:
            valid_pieces = [*valid_pieces, *self.movegen(self.queue[0])]

        best_piece = None
        best_score = 0

        for piece in valid_pieces:
            board = self.board.copy()
            board.set(piece)
            board.clear_lines()
            lscore = eval.eval_board(board)
            if lscore > best_score:
                best_score = lscore
                best_piece = piece

        if best_piece.type != self.current_piece.type:
            self.hold_piece()

        return best_piece

    def grounded(self, piece):

        if self.collides(self.board, piece):
            return False

        piece = piece.copy()
        piece.pos[1] -= 1

        if self.collides(self.board, piece):
            return True

    def place_piece(self, piece: Piece = None):
        global bag

        if not piece:
            piece = self.current_piece

        self.board.set(piece)

        cleared = self.board.clear_lines()

        self.current_piece = Piece(self.queue[0])

        self.queue = np.roll(self.queue, -1)

        self.queue[QUEUE_SIZE - 1] = self.getPiece()

        self.held = False

        return cleared

    @jitmethod
    def _collides(minos, pos, board) -> bool:
        for i in range(4):
            x_pos = minos[i * 2 + 0] + pos[0]
            if x_pos < 0 or x_pos >= 10:
                return True

            y_pos = minos[i * 2 + 1] + pos[1]
            if y_pos < 0 or y_pos >= 32:
                return True
            if board_get(board, x_pos, y_pos):
                return True

        return False

    def collides(self, board: Board, piece: Piece) -> bool:
        return Game._collides(piece.minos, piece.pos, board.board)

    def rotate(self, dir: int, piece: Piece):
        if piece.type == PieceType.O:
            return  # O piece doesn't rotate

        prev_rot = piece.rotation

        (piece.rotate_left() if dir == TurnDirection.Left else piece.rotate_right())

        if piece.type == PieceType.I:
            prev_offsets = np.copy(piece_offsets_I[prev_rot])
            offsets = np.copy(piece_offsets_I[piece.rotation])
        else:
            prev_offsets = np.copy(piece_offsets_JLSTZ[prev_rot])
            offsets = np.copy(piece_offsets_JLSTZ[piece.rotation])

        x = piece.pos[0]
        y = piece.pos[1]

        kicks = 5

        if piece.type == PieceType.O:
            kicks = 1

        for i in range(kicks):
            piece.pos = (
                x + prev_offsets[i * 2] - offsets[i * 2],
                y + prev_offsets[i * 2 + 1] - offsets[i * 2 + 1],
            )
            if not self.collides(self.board, piece):
                return

        piece.pos = (x, y)

        (piece.rotate_right() if dir == TurnDirection.Left else piece.rotate_left())

    def shift(self, dir: int, piece: Piece):
        piece.pos = (
            piece.pos[0] + dir,
            piece.pos[1],
        )

        if self.collides(self.board, piece):
            piece.pos = (
                piece.pos[0] - dir,
                piece.pos[1],
            )

    def sonic_drop(self, piece: Piece):
        while not self.collides(self.board, piece):
            piece.pos = (
                piece.pos[0],
                piece.pos[1] - 1,
            )
        piece.pos = (
            piece.pos[0],
            piece.pos[1] + 1,
        )

    def add_garbage(self, lines: int, loc: int):
        assert loc >= 0 and loc < 10

        for i in range(10):
            self.board.board[i] <<= lines

            if loc != i:
                self.board.board[i] |= (1 << lines) - 1


    def calc_damage(self, lines_cleared, tspin=False, mini=False, combo=0):
        return lines_cleared


    def process_movement(self, movement: int, piece: Piece = None):
        if not piece:
            piece = self.current_piece
        match (movement):
            case Movement.Left:
                self.shift(-1, piece)
            case Movement.Right:
                self.shift(1, piece)
            case Movement.RotateClockwise:
                self.rotate(TurnDirection.Right, piece)
            case Movement.RotateCounterClockwise:
                self.rotate(TurnDirection.Left, piece)
            case Movement.SonicDrop:
                self.sonic_drop(piece)
            case Movement.HardDrop:
                self.sonic_drop(piece)
                self.place_piece()
                lines_cleared = self.board.clear_lines()
                # int damage = damage_sent(lines_cleared);

    def hold_piece(self):
        if self.held:
            return

        if self.hold is None:
            self.hold = self.current_piece
            self.current_piece = Piece(self.queue[0])
            self.queue = np.roll(self.queue, -1)
            self.queue[QUEUE_SIZE - 1] = self.getPiece()
        else:
            temp = self.hold
            self.hold = self.current_piece
            self.current_piece = temp

        self.held = True

    def evaluate(self, board=None):
        if not board:
            board = self.board
        # find well
        well_index = 0
        well_score = 0
        for i in range(10):
            col = board.board[i]
            if well_score < ctlz(col):
                well_score = ctlz(col)
                well_index = i

        # height variance
        hight_variance_eval = 0
        diff = np.zeros(11, dtype="int32")
        diff[0] = -32
        diff[10] = 32

        for i in range(1, 10, 1):

            prev = ctlz(board.board[i - 1])
            cur = ctlz(board.board[i])

            diff[i] = prev - cur

            if i < well_index:
                hight_variance_eval += np.clip(cur - prev, -1, 1)
            elif i > well_index + 1:
                hight_variance_eval += np.clip(prev - cur, -1, 1)

        dependency_eval = 0
        for i in range(10):
            prev = diff[i]
            cur = diff[i + 1]
            if prev < cur:
                if prev <= -2 and cur >= 2:
                    dependency_eval += 1

        return dependency_eval

class Move:
    def __init__(self, piece = None):
        self.null = False
        if piece is None:
            self.null = True
        self.piece = piece
    
    def __repr__(self):
        return repr(self.piece)

class VersusGame:

    def __init__(self):
        self.games = (Game(), Game())

        self.moves = [None, None]

        self.damage = [0, 0]

    def set_move(self, id, move: Move):
        if id == 0:
            self.moves[0] = move
        if id == 1:
            self.moves[1] = move
    
    def play_moves(self):
        if self.moves[0] is None or self.moves[1] is None:
            # not all players have moved yet
            return

        if self.games[0].collides(self.games[0].board, self.games[0].current_piece):
            return

        if self.games[1].collides(self.games[1].board, self.games[1].current_piece):
            return

        cleared = [0, 0]

        if not self.moves[0].null:
            cleared[0] = self.games[0].place_piece(self.moves[0].piece)

        if not self.moves[1].null:
            cleared[1] = self.games[1].place_piece(self.moves[1].piece)

        if cleared[0] == 0:
            self.games[0].add_garbage(self.damage[1], GetRand(10))
        if cleared[1] == 0:
            self.games[1].add_garbage(self.damage[0], GetRand(10))

        self.damage[0] = self.games[0].calc_damage(cleared[0])
        self.damage[1] = self.games[1].calc_damage(cleared[1])

        if self.damage[1] > self.damage[0]:
            self.damage[1] = self.damage[1] - self.damage[0]
            self.damage[0] = 0
        else:
            self.damage[0] = self.damage[0] - self.damage[1]
            self.damage[1] = 0



    def get_moves(self, id):
        if id == 0:
            return [Move(), *[Move(p) for p in self.games[0].movegen()]]
        if id == 1:
            return [Move(), *[Move(p) for p in self.games[1].movegen()]]



def renderBoard(game: Game, offx=0, offy=0):
    for i in range(10):
        for j in range(32):
            if board_get(game.board.board, i, j):
                size = 25
                rl.DrawRectangle(i * size + offx, (31 - j) * size + offy, size, size, rl.BLACK)
            else:
                size = 25
                rl.DrawRectangle(i * size + offx, (31 - j) * size + offy, size, size, rl.WHITE)


def renderCurrentPiece(game: Game, offx=0, offy=0):
    for i in range(4):
        x = game.current_piece.pos[0] + game.current_piece.minos[i * 2]
        y = game.current_piece.pos[1] + game.current_piece.minos[i * 2 + 1]
        size = 25
        rl.DrawRectangle(x * size + offx, (31 - y) * size + offy, size, size, rl.RED)



def main():
    luth: Lut_H = Lut_H(
        "./DLUTH.bin"
    )
    lutv: Lut_V = Lut_V(
        "./DLUTV.bin"
    )

    lut_eval = Eval(luth, lutv)

    index = 0
    vgame = VersusGame()
    game = vgame.games[0]
    game2 = vgame.games[1]

    SCREEN_WIDTH = 800
    SCREEN_HEIGHT = 25 * 32
    rl.InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, b"Tetris")

    rl.SetTargetFPS(60)
    downed = False
    accumulatedTime = 0

    index = 0

    e = lut_eval.eval_board(game.board)

    while not rl.WindowShouldClose():
        rl.BeginDrawing()
        accumulatedTime += rl.GetFrameTime()
        rl.ClearBackground(rl.BLUE)
        renderBoard(game)
        renderCurrentPiece(game)
        renderBoard(game2, 500)
        renderCurrentPiece(game2, 500)
        if accumulatedTime > 0.5:
            accumulatedTime = 0
            downed = False

        if rl.IsKeyPressed(263):
            game.process_movement(Movement.Left)
        if rl.IsKeyPressed(262):
            game.process_movement(Movement.Right)

        if rl.IsKeyPressed(ord("V")):
            game.process_movement(Movement.SonicDrop)
            vgame.set_move(0, Move(game.current_piece))
            vgame.set_move(1, Move(vgame.games[1].get_best_piece(lut_eval)))
            vgame.play_moves()
            e = lut_eval.eval_board(game.board)

        if rl.IsKeyPressed(ord("E")):
            game.process_movement(Movement.SonicDrop)

        # detect right arrow key
        if rl.IsKeyPressed(ord("D")):
            game.process_movement(Movement.RotateClockwise)

        # detect left arrow key
        if rl.IsKeyPressed(ord("S")):
            game.process_movement(Movement.RotateCounterClockwise)

        # up key
        if rl.IsKeyPressed(265):
            game.hold_piece()

        if rl.IsKeyPressed(ord("G")):
            game.add_garbage(GetRand(4) + 1, GetRand(10))

        if rl.IsKeyPressed(ord("P")):
            game.place_random_valid_piece(index)

        if rl.IsKeyPressed(ord("B")):
            game.place_best_piece(lut_eval)

        if rl.IsKeyPressed(ord("R")):                   
            vgame = VersusGame()
            game = vgame.games[0]
            game2 = vgame.games[1]

        if rl.IsKeyPressed(ord("Q")):
            vgame.set_move(0, Move(vgame.games[0].get_best_piece(lut_eval)))
            vgame.set_move(1, Move(vgame.games[1].get_best_piece(lut_eval)))
            vgame.play_moves()


        text = f"Eval 1: {e}"
        text = text.encode("utf-8")
        rl.DrawText(text, 260, 10, 20, rl.BLACK)

        text = f"Eval 2: {lut_eval.eval_board(game2.board)}"
        text = text.encode("utf-8")
        rl.DrawText(text, 260, 50, 20, rl.BLACK)

        rl.EndDrawing()

if __name__ == "__main__":
    main()
