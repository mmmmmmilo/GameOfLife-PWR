# Multithreaded Game of Life

A high-performance, terminal-based simulation of Conway's Game of Life written in **C++20**. This project serves as a practical exploration of concurrent programming, spatial partitioning, and thread synchronization. It visualizes not only the cellular automaton itself but also the underlying multithreading mechanics, including real-time chunk updates and intentional deadlock states.

## Description

Instead of processing the grid sequentially, this simulation divides the world into an independent grid of "chunks" (e.g., 9x5). Each chunk is assigned its own dedicated `std::thread` and lifecycle. To calculate the next generation, a chunk must safely read the borders of its 8 neighboring chunks, requiring strict synchronization protocols to avoid data races and deadlocks.

## Highlights & Implementation Details

* **Spatial Partitioning (Chunking):** The world is divided into smaller 7x7 cell chunks. Each chunk runs an independent `worker_routine` loop, simulating asynchronous computation with randomized jitter.
* **Advanced Concurrency & Synchronization:** * Utilizes `std::thread`, `std::mutex`, and `std::atomic` to manage state across chunk boundaries.
  * **Deadlock Prevention:** Implements a strict lock hierarchy algorithm. Before a chunk updates, it gathers its own mutex and the mutexes of its neighbors, sorting their memory addresses (`std::sort`) to guarantee a globally consistent locking order, effectively preventing circular wait conditions.
* **Deadlock Simulation Mode:** Includes an interactive toggle to intentionally disable lock ordering and inject artificial delays. This forces the engine into a classic deadlock state (visualized in red) to demonstrate the consequences of poor synchronization.
* **ANSI Terminal Rendering:** Bypasses standard graphical libraries in favor of raw ANSI escape codes (`\033[44;34m`), providing a fast terminal UI with distinct color representations for chunk boundaries, thread activity, and locked states.

## Compilation and Execution

### Prerequisites
* C++20 compatible compiler (GCC, Clang)
* CMake 3.15+
* Linux/Unix terminal environment (for correct ANSI color rendering)

### Build Instructions
1. Generate build files and compile:
   ```bash
   mkdir build && cd build
   cmake -DCMAKE_BUILD_TYPE=Release ..
   cmake --build .

```

2. Run the executable:
```bash
./Game_Of_Life

```



### Interactive Settings

Upon launch, the CLI will prompt for the following visual and mechanical settings:

* `Enable deadlock? [y/n]`: Injects delays to purposefully crash the threads into a circular wait.
* `Visualize chunks? [y/n]`: Renders a checkerboard pattern showing thread boundaries.
* `Visualize updating chunks? [y/n]`: Highlights chunks in real-time as their threads lock memory and calculate the next generation.

## Showcase
<img width="80%" alt="game_of_life" src="https://github.com/user-attachments/assets/4c094438-2078-44ce-b87b-40417b086763" />

