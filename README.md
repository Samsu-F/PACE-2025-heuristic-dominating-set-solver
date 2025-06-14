# PACE 2025 - Heuristic Dominating Set Solver
Submission for the [PACE Challenge 2025 - Dominating Set Heuristic Track](https://pacechallenge.org/2025/ds/) by Samuel Füßinger.

## Installation guide
Install Make and GCC if not already present.
Then simply run
```sh
make release
```
This will produce a binary executable which can be found at `build/release/heuristic_solver`.
For anyone interested in understanding or working on the source code: run `make help` for a list of additional targets.

## Dependencies
- The C standard library
- The C POSIX library

## Usage of the executable
The executable will read an input graph from stdin. It will then try to solve it as well as possible until it receives a SIGTERM signal, after which it will output its solution to stdout.
Note that it may stop delayed or may not stop at all if it receives the SIGTERM signal within the first 25 seconds of execution.
