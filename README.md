# DPHH

DPHH is a C++ implementation of streaming differentially private data stream heavy hitter algorithms, including counter-based methods and linear sketches.

The project uses CMake and targets modern C++ (C++20).

---

## Requirements

- **C++ compiler** with C++20 support  
  (e.g., `g++` ≥ 11, `clang++` ≥ 14)
- **CMake** ≥ 3.29

No special hardware features are required.

---

## Build Instructions

Clone the repository and build using an out-of-source CMake build:

```bash
git clone <repo-url>
cd DPHH

mkdir build
cd build
cmake ..
make

## Executable

This will create the executable DPHH

from the build directory run

./DPHH


