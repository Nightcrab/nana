import numba as nb
import numba.experimental as nbe
from numba import types
from numba.cpython import mathimpl
from numba.extending import intrinsic
import numpy as np
import raylib as rl
from numba import njit

from numba import int8, int16, int32, uint32, uint64

@njit
def _is_empty(array, index):
    # 0 only possible if we never set the last bit i.e empty
    if array[index][0] == 0:
        return True
    return False

@njit
def _get_key(array, index):
    # right shift key
    return array[index][0] >> 1

@njit
def _set_key(array, index, key):
    # left shift key and set last bit
    key = np.array(key, dtype="int32")
    shifted_key = key << 1 | 1
    array[index][0] = shifted_key

@njit
def _strip_bit(i):
    return (i << 1) >> 1

@njit(int32(int32[:,:], int32, int32, int32))
def _address(array, key, stride, size):
    index = key % size
    # search for either an empty address or one matching our key
    while not _is_empty(array, index) and _get_key(array, index) != _strip_bit(key):
        # linear addressing
        index = (index + stride) % size
        if index == key % size:
            # we're full
            break
    return index

@nb.experimental.jitclass([
    ('array', int32[:, :]),   
    ('size', int32),   
    ('stride', int32),   
    ('_keys', int32[:]),   
    ('key_count', int32),    
])
class UMap:

    def __init__(self, size=1000000):
        self.array = np.zeros((size, 2), dtype="int32")
        self.size = size
        self.stride = 1
        self._keys = np.zeros(size, dtype="int32")
        self.key_count = 0

    def is_empty(self, index):
        return _is_empty(self.array, index)

    def get_key(self, index):
        return _get_key(self.array, index)

    def set_key(self, index, key):
        return _set_key(self.array, index, key)

    def strip_bit(self, i):
        return _strip_bit(i)

    def address(self, key: int):
        return _address(self.array, key, self.stride, self.size)

    def put(self, key: int, value):
        if not self.has(key):
            self._keys[self.key_count] = key
            self.key_count += 1

        index = self.address(key)
        self.set_key(index, key)
        self.array[index][1] = value

    def get(self, key: int):
        index = self.address(key)
        return self.array[index][1]

    def __getitem__(self, key):
        return self.get(key)

    def has(self, key: int):
        index = self.address(key)
        if self.is_empty(index):
            return False
        return True

    def keys(self):
        return self._keys[:self.key_count]
