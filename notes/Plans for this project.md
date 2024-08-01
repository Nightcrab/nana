## Short-term goals

- Replace reward function with AI
- Implement Work Stealing for threads that do not currently have a Job
- Remove the [multi-producer-single-consumer](https://github.com/Donald-Rupin/mpsc_zib) queue and replace it with a [single-producer-single-consumer](https://github.com/rigtorp/SPSCQueue)


## Long-term goals

- Implement continuous time into our game tree
- Implement Deep [CFR](https://arxiv.org/pdf/1811.00164) for training purposes
- Generalize into other tetrominoes game variants
- Achieve Nash Equilibrium


## TODOs
- Have jobs pass references of states instead of copying, and copy only at the root