#include <iomanip>
#include <iostream>
#include <random>
#include <thread>

enum state {
    THINKING,
    HUNGRY,
    EATING
};

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

struct philosopher {
    int id;

    state current_state = THINKING;
    std::chrono::time_point<std::chrono::system_clock> last_state_change;
};

struct table {
    int seats{};

    std::mutex critical_region_mtx;

    std::vector<philosopher> philosophers;
    std::vector<std::unique_ptr<std::binary_semaphore>> forks_available;
};

void initialize(table& table, int seats) {
    table.seats = seats;

    const auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < seats; i++) {
        table.philosophers.emplace_back(i, THINKING, start);
        table.forks_available.push_back(
            std::make_unique<std::binary_semaphore>(0));
    }
}

philosopher& left(table& table, const philosopher& person) {
    return table.philosophers[(person.id - 1 + table.seats) % table.seats];
}

philosopher& right(table& table, const philosopher& person) {
    return table.philosophers[(person.id + 1) % table.seats];
}

int generate_random(int min, int max) {
    static std::mt19937 random_engine = std::mt19937(std::random_device()());
    return std::uniform_int_distribution(min, max)(random_engine);
}

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

void think() {
    size_t duration = generate_random(500, 1500);
    std::this_thread::sleep_for(std::chrono::milliseconds(duration));
}

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

void eat() {
    size_t duration = generate_random(400, 800);
    std::this_thread::sleep_for(std::chrono::milliseconds(duration));
}

void put_forks(table& table, philosopher& philosopher) {
    std::lock_guard lk{table.critical_region_mtx};
    philosopher.current_state = THINKING;
    philosopher.last_state_change = std::chrono::high_resolution_clock::now();
    test(table, left(table, philosopher));
    test(table, right(table, philosopher));
}

[[noreturn]] void philosopher_loop(table& table, philosopher& philosopher) {
    while (true) {
        think();
        take_forks(table, philosopher);
        eat();
        put_forks(table, philosopher);
    }
}

int main(int argc, char* argv[]) {
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
                std::cout << "  " << i + 1 << ". filozof: " << state_to_string(
                        table.philosophers[i].current_state)
                    << " (ostatnia zmiana " << std::setprecision(2)
                    << std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::high_resolution_clock::now() - table.
                        philosophers
                        [i].last_state_change).count() / 1000.0 <<
                    " sekund temu)"
                    << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });

    std::thread* threads = new std::thread[number_of_philosophers];

    for (int i = 0; i < number_of_philosophers; i++) {
        std::thread philosopher_thread([&table, i] {
            philosopher_loop(table, table.philosophers[i]);
        });
        threads[i] = std::move(philosopher_thread);
    }

    for (int i = 0; i < number_of_philosophers; i++) {
        threads[i].join();
    }

    display.join();

    return 0;
}