# Game of Life

A terminal-based simulation of Conway's Game of Life, implemented in C++20. This project demonstrates parallel processing and concurrency control mechanisms, including optional deadlock scenarios for visualization purposes.

## Description

This implementation of Conway's Game of Life divides the simulation grid into independent chunks, each processed by its own thread. It showcases synchronization techniques using mutexes to manage state updates across chunk boundaries in a concurrent environment.

## Technologies

*   **Language:** C++20
*   **Build System:** CMake
*   **Libraries:** Standard Library (`<thread>`, `<mutex>`, `<atomic>`, `<vector>`, etc.)

## Compilation and Execution

### Prerequisites

*   A C++ compiler supporting C++20 (e.g., GCC 10+, Clang 10+)
*   CMake (version 4.0 or higher)

### Build and Run

1.  Clone the repository and navigate to the project directory.
2.  Create a build directory and run the following commands:

```bash
mkdir build
cd build
cmake ..
make
./Game_Of_Life
```

Follow the on-screen prompts to configure the simulation settings (deadlock simulation, chunk visualization, etc.).

## Showcase

<img src="assets/placeholder.png" width="48%" />
