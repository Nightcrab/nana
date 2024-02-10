from run import *

def test_map():
    map = UMap(5)
    map.put(1, 1)
    print(map.array)
    print("1, 1:", map.get(1))
    map.put(1, 2)
    print(map.array)
    print("1, 2:", map.get(1))
    map.put(2, 45)
    print(map.array)
    print("2, 45:", map.get(2))
    map.put(0, 55)
    print(map.array)
    print("0, 55:", map.get(0))
    print("has 0", map.has(0))
    print("has 1", map.has(1))
    print("has 2", map.has(2))
    print("has 3", map.has(3))
    print("has 4", map.has(4))

test_map()

def bench_umap():
    map = UMap(size=1000000)

    for i in range(10000):
        map.put(GetRand(100000000), i)

    #warmup

    map.get(1)

    t = time.time()
    for i in range(1000000):
        map.get(i)

    print("time to get 1,000,000 elements", time.time() - t)

bench_umap()