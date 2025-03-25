#include <iomanip>
#include <iostream>
#include <random>
#include <thread>

/**
 * Philosopher's state.
 */
enum state { THINKING, HUNGRY, EATING };

/**
 * Returns a string to be displayed for a specific state.
 *
 * @param state state enum
 * @return text displayed for provided state
 */
std::string state_to_string(const state& state) {
    switch (state) {
        case HUNGRY:
            return "jest głodny";
        case EATING:
            return "je         ";
        default:
        case THINKING:
            return "myśli      ";
    }
}

/**
 * Struct representing a philosopher. Contains their id, state, and last state
 * change timestamp.
 */
struct philosopher {
    int id;

    state current_state = THINKING;
    std::chrono::time_point<std::chrono::system_clock> last_state_change;
};

/**
 * Struct representing a table. Contains philosophers, forks, number of seats
 * and mutex responsible for critical regions.
 */
struct table {
    int seats{};

    std::mutex critical_region_mtx{};

    std::vector<philosopher> philosophers{};
    std::vector<std::unique_ptr<std::binary_semaphore>> forks_available{};
};

/**
 * Initializes a table with philosophers and binary semaphores for forks.
 *
 * @param table table to initialize
 * @param seats number of seats
 */
void initialize(table& table, const int seats) {
    table.seats = seats;

    const auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < seats; i++) {
        table.philosophers.emplace_back(i, THINKING, start);
        table.forks_available.push_back(
            std::make_unique<std::binary_semaphore>(0));
    }
}

/**
 * Returns the left neighbour of the specified philosopher.
 *
 * @param table table of the philosophers
 * @param person philosopher
 * @return left neighbour of specified philosopher
 */
philosopher& left(table& table, const philosopher& person) {
    return table.philosophers[(person.id - 1 + table.seats) % table.seats];
}

/**
 * Returns the right neighbour of the specified philosopher.
 *
 * @param table table of the philosophers
 * @param person philosopher
 * @return right neighbour of specified philosopher
 */
philosopher& right(table& table, const philosopher& person) {
    return table.philosophers[(person.id + 1) % table.seats];
}

/**
 * Returns a random number from [min, max] range.
 *
 * @param min minimum
 * @param max maximum
 * @return random number from specified range
 */
int generate_random(const int min, const int max) {
    static auto random_engine = std::mt19937(std::random_device()());
    return std::uniform_int_distribution(min, max)(random_engine);
}

/**
 * Tests if two forks are available for the specified philosopher, and when
 * available changes state to eating and "locks" their forks down.
 *
 * @param table philosopher's table
 * @param philosopher target philosopher
 */
void test(table& table, philosopher& philosopher) {
    if (philosopher.current_state == HUNGRY &&
        left(table, philosopher).current_state != EATING &&
        right(table, philosopher).current_state != EATING) {
        philosopher.current_state = EATING;
        philosopher.last_state_change =
            std::chrono::high_resolution_clock::now();
        table.forks_available[philosopher.id]->release();
    }
}

/**
 * Responsible for philosopher thinking. Thinking is simulated
 * by putting the philosopher's thread to sleep for a random time (between 500
 * and 1500 ms)
 */
void think() {
    const size_t duration = generate_random(500, 1500);
    std::this_thread::sleep_for(std::chrono::milliseconds(duration));
}

/**
 * Responsible for philosopher taking the forks. Contains a critical section.
 *
 * @param table philosopher's table
 * @param philosopher target philosopher
 */
void take_forks(table& table, philosopher& philosopher) {
    {
        std::lock_guard lk{table.critical_region_mtx};
        philosopher.current_state = HUNGRY;
        philosopher.last_state_change =
            std::chrono::high_resolution_clock::now();
        test(table, philosopher);
    }

    table.forks_available[philosopher.id]->acquire();
}

/**
 * Responsible for philosopher eating. Eating is simulated
 * by putting the philosopher's thread to sleep for a random time (between 400
 * and 800 ms)
 */
void eat() {
    const size_t duration = generate_random(400, 800);
    std::this_thread::sleep_for(std::chrono::milliseconds(duration));
}

/**
 * Responsible for putting forks down logic. Contains a critical
 * section.
 *
 * @param table philosopher's table
 * @param philosopher target philosopher
 */
void put_forks(table& table, philosopher& philosopher) {
    std::lock_guard lk{table.critical_region_mtx};
    philosopher.current_state = THINKING;
    philosopher.last_state_change = std::chrono::high_resolution_clock::now();
    test(table, left(table, philosopher));
    test(table, right(table, philosopher));
}

/**
 * Main philosopher loop, responsible for thinking, taking forks,
 * eating, and putting the forks back.
 *
 * @param table philosopher's table
 * @param philosopher target philosopher
 */
[[noreturn]] void philosopher_loop(table& table, philosopher& philosopher) {
    while (true) {
        think();
        take_forks(table, philosopher);
        eat();
        put_forks(table, philosopher);
    }
}

int main(const int argc, char* argv[]) {
    int number_of_philosophers = 5;

    if (argc == 2) {
        number_of_philosophers = std::stoi(argv[1]);
    }

    if (number_of_philosophers <= 0) {
        std::cout << "[!] Ilość filozofów musi być >= 1" << std::endl;
        return 1;
    }

    table table;
    initialize(table, number_of_philosophers);

    std::thread display([&] {
        while (true) {
            std::cout << "\x1B[2J\x1B[H";
            std::cout << "[#] Stan filozofów: " << std::endl;
            for (int i = 0; i < number_of_philosophers; i++) {
                std::cout << "  " << i + 1 << ". filozof: "
                          << state_to_string(
                                 table.philosophers[i].current_state)
                          << " (ostatnia zmiana " << std::setprecision(2)
                          << static_cast<double>(
                                 std::chrono::duration_cast<
                                     std::chrono::milliseconds>(
                                     std::chrono::high_resolution_clock::now() -
                                     table.philosophers[i].last_state_change)
                                     .count()) /
                                 1000.0
                          << " sekund temu)" << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });

    auto* threads = new std::thread[number_of_philosophers];

    for (int i = 0; i < number_of_philosophers; i++) {
        std::thread philosopher_thread(
            [&table, i] { philosopher_loop(table, table.philosophers[i]); });
        threads[i] = std::move(philosopher_thread);
    }

    for (int i = 0; i < number_of_philosophers; i++) {
        threads[i].join();
    }

    display.join();

    return 0;
}