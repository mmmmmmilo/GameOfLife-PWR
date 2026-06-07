#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include <random>
#include <atomic>
#include <algorithm>

using namespace std;

const int chunk_x = 7;
const int chunk_y = 7;
const int world_chunks_x = 9;
const int world_chunks_y = 5;
const float density = 0.15;
const int frequency = 240;

bool visualize_updates = false;
bool visualize_chunks = false;
bool deadlock_enabled = false;

const string color_1 = "\033[44;34m";
const string color_1_no_bg = "\033[2;34m";
const string color_2 = "\033[47;37m";
const string color_2_no_bg = "\033[2;37m";
const string color_update = "\033[45;35m";
const string color_update_no_bg = "\033[2;35m";
const string color_deadlock = "\033[41;31m";
const string color_deadlock_no_bg = "\033[2;31m";
const string color_reset = "\033[0m";

struct Chunk {
    int id;
    int x_idx, y_idx;
    bool cells[chunk_y][chunk_x];
    bool next_gen[chunk_y][chunk_x];

    mutex chunk_mutex;
    vector<Chunk*> neighbors;

    thread worker;
    atomic<bool> running{true};
    atomic<bool> is_updating{false};
    atomic<long long> last_update_time_ms{0};

    Chunk(int id, int x, int y) : id(id), x_idx(x), y_idx(y) {
        default_random_engine generator(random_device{}());
        bernoulli_distribution distribution(density);

        for (int i = 0; i < chunk_y; i++) {
            for (int j = 0; j < chunk_x; j++) {
                cells[i][j] = distribution(generator);
            }
        }

        auto now = chrono::system_clock::now().time_since_epoch();
        last_update_time_ms = chrono::duration_cast<chrono::milliseconds>(now).count();
    }

    ~Chunk() {
        running = false;
        if (worker.joinable()) worker.join();
    }
};

class GameWorld {
public:
    vector<Chunk*> linear_chunks;
    vector<vector<Chunk*>> grid_chunks;

    GameWorld() {
        int id_counter = 0;
        for (int y = 0; y < world_chunks_y; y++) {
            vector<Chunk*> row;
            for (int x = 0; x < world_chunks_x; x++) {
                Chunk* c = new Chunk(id_counter++, x, y);
                row.push_back(c);
                linear_chunks.push_back(c);
            }
            grid_chunks.push_back(row);
        }

        for (int y = 0; y < world_chunks_y; y++) {
            for (int x = 0; x < world_chunks_x; x++) {
                Chunk* current = grid_chunks[y][x];

                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        if (dx == 0 && dy == 0) continue;

                        int nx = (x + dx + world_chunks_x) % world_chunks_x;
                        int ny = (y + dy + world_chunks_y) % world_chunks_y;

                        current->neighbors.push_back(grid_chunks[ny][nx]);
                    }
                }
            }
        }

        for (auto* c : linear_chunks) {
            c->worker = thread(&GameWorld::worker_routine, this, c);
        }
    }

    ~GameWorld() {
        for (auto* c : linear_chunks) delete c;
    }

    void worker_routine(Chunk* self) {
        default_random_engine gen(random_device{}() + self->id);
        uniform_int_distribution<int> jitter(0, 40);

        while (self->running) {
            this_thread::sleep_for(chrono::milliseconds(frequency + jitter(gen)));

            auto now = chrono::system_clock::now().time_since_epoch();
            self->last_update_time_ms = chrono::duration_cast<chrono::milliseconds>(now).count();
            self->is_updating = true;

            if (deadlock_enabled) {
                self->chunk_mutex.lock();
                this_thread::sleep_for(chrono::milliseconds(40));
                for (Chunk* neighbor : self->neighbors) {
                    neighbor->chunk_mutex.lock();
                    this_thread::sleep_for(chrono::milliseconds(2));
                }
            }
            else {
                vector<mutex*> mutexes_to_lock;
                mutexes_to_lock.push_back(&self->chunk_mutex);
                for (Chunk* neighbor : self->neighbors) {
                    mutexes_to_lock.push_back(&neighbor->chunk_mutex);
                }
                sort(mutexes_to_lock.begin(), mutexes_to_lock.end());
                for (mutex* m : mutexes_to_lock) {
                    m->lock();
                    this_thread::sleep_for(chrono::milliseconds(2));
                }
            }

            calculate_next_gen(self);

            for (Chunk* neighbor : self->neighbors) {
                neighbor->chunk_mutex.unlock();
            }
            self->chunk_mutex.unlock();
            self->is_updating = false;
        }
    }

    void calculate_next_gen(Chunk* self) {
        for (int y = 0; y < chunk_y; y++) {
            for (int x = 0; x < chunk_x; x++) {
                int alive_neighbors = count_alive_neighbors(self, x, y);

                bool current = self->cells[y][x];
                bool next_state = current;

                if (current && (alive_neighbors < 2 || alive_neighbors > 3))
                    next_state = false;
                else if (!current && alive_neighbors == 3)
                    next_state = true;

                self->next_gen[y][x] = next_state;
            }
        }

        for (int y = 0; y < chunk_y; y++)
            for (int x = 0; x < chunk_x; x++)
                self->cells[y][x] = self->next_gen[y][x];
    }

    int count_alive_neighbors(Chunk* center_chunk, int local_x, int local_y) {
        int count = 0;

        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0) continue;

                int nx = local_x + dx;
                int ny = local_y + dy;
                Chunk* target = center_chunk;

                if (nx < 0) { nx = chunk_x - 1; target = get_neighbor(center_chunk, -1, 0); }
                else if (nx >= chunk_x) { nx = 0; target = get_neighbor(center_chunk, 1, 0); }

                if (ny < 0) { ny = chunk_y - 1; target = get_neighbor(target, 0, -1); }
                else if (ny >= chunk_y) { ny = 0; target = get_neighbor(target, 0, 1); }

                if (target->cells[ny][nx]) count++;
            }
        }
        return count;
    }

    Chunk* get_neighbor(Chunk* c, int dx_chunk, int dy_chunk) {
        int tx = (c->x_idx + dx_chunk + world_chunks_x) % world_chunks_x;
        int ty = (c->y_idx + dy_chunk + world_chunks_y) % world_chunks_y;
        return grid_chunks[ty][tx];
    }

    void display() {
        cout << "\033[H";

        for (int y = 0; y < world_chunks_y * chunk_y; y++) {
            for (int x = 0; x < world_chunks_x * chunk_x; x++) {
                int cx = x / chunk_x;
                int cy = y / chunk_y;
                Chunk* c = grid_chunks[cy][cx];

                bool is_purple_chunk = (cx + cy) % 2 == 0;
                bool is_updating = c->is_updating;
                long long last_update_time_ms = c->last_update_time_ms;

                auto now = chrono::system_clock::now().time_since_epoch();
                long long current_ms = chrono::duration_cast<chrono::milliseconds>(now).count();

                string alive_col, dead_col;
                if (current_ms - last_update_time_ms > 2048) {
                    alive_col = color_deadlock;
                    dead_col = color_deadlock_no_bg;
                } else if (is_updating && visualize_updates) {
                    alive_col = color_update;
                    dead_col = color_update_no_bg;
                } else {
                    if (is_purple_chunk && visualize_chunks) {
                        alive_col = color_1;
                        dead_col = color_1_no_bg;
                    } else {
                        alive_col = color_2;
                        dead_col = color_2_no_bg;
                    }
                }

                bool cell_alive = c->cells[y % chunk_y][x % chunk_x];
                if (cell_alive) {
                    cout << alive_col << "  " << color_reset;
                }
                else {
                    cout << dead_col << "[]" << color_reset;
                }
            }
            cout << "\n";
        }
    }
};

void settings() {
    string answer;
    while (answer != "y" && answer != "n") {
        cout << "Enable deadlock? [y/n]: ";
        cin >> answer;
    }
    if (answer == "y") deadlock_enabled = true;

    answer = "";
    while (answer != "y" && answer != "n") {
        cout << "Visualize chunks? [y/n]: ";
        cin >> answer;
    }
    if (answer == "y") visualize_chunks = true;

    answer = "";
    while (answer != "y" && answer != "n") {
        cout << "Visualize updating chunks? [y/n]: ";
        cin >> answer;
    }
    if (answer == "y") visualize_updates = true;
}

int main() {
    settings();
    GameWorld world;
    system("clear || cls");

    while (true) {
        world.display();
        this_thread::sleep_for(chrono::milliseconds(20));
    }

    return 0;
}