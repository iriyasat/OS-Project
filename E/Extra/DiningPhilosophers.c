#define _GNU_SOURCE
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define PHIL_COUNT 5

typedef struct Philosopher {
    int id;
    char state[16];
    int meals;
    double last_meal_at;
} Philosopher;

typedef struct Config {
    double run_time_sec;
    double max_think_sec;
    double max_eat_sec;
    double timeout_sec;
    double starvation_limit_sec;
} Config;

static volatile int running = 1;

static double now_monotonic_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static void sleep_sec(double sec) {
    if (sec <= 0) return;
    struct timespec ts;
    ts.tv_sec = (time_t)sec;
    ts.tv_nsec = (long)((sec - (double)ts.tv_sec) * 1e9);
    while (nanosleep(&ts, &ts) == -1 && errno == EINTR) {
    }
}

static double rand_range(unsigned int *seed, double a, double b) {
    if (b < a) { double t = a; a = b; b = t; }
    double r = (double)rand_r(seed) / (double)RAND_MAX;
    return a + r * (b - a);
}

static int mutex_timedlock_sec(pthread_mutex_t *mtx, double timeout_sec) {
    struct timespec abstime;
    clock_gettime(CLOCK_REALTIME, &abstime);
    long nsec_add = (long)((timeout_sec - (long)timeout_sec) * 1e9);
    abstime.tv_sec += (time_t)timeout_sec;
    abstime.tv_nsec += nsec_add;
    if (abstime.tv_nsec >= 1000000000L) {
        abstime.tv_sec += 1;
        abstime.tv_nsec -= 1000000000L;
    }
    return pthread_mutex_timedlock(mtx, &abstime);
}

static inline int left_idx(int i) { return i; }
static inline int right_idx(int i) { return (i + 1) % PHIL_COUNT; }

typedef struct ThreadArgs {
    Philosopher *ph;
    pthread_mutex_t *chopsticks;
    Config *cfg;
} ThreadArgs;

static void *philosopher_thread(void *arg) {
    ThreadArgs *ta = (ThreadArgs *)arg;
    Philosopher *ph = ta->ph;
    pthread_mutex_t *chop = ta->chopsticks;
    Config *cfg = ta->cfg;

    unsigned int seed = (unsigned int)(time(NULL) ^ (ph->id * 2654435761u));

    while (running) {
        strncpy(ph->state, "thinking", sizeof(ph->state) - 1);
        ph->state[sizeof(ph->state) - 1] = '\0';
        sleep_sec(rand_range(&seed, 0.1, cfg->max_think_sec));

        strncpy(ph->state, "hungry", sizeof(ph->state) - 1);
        ph->state[sizeof(ph->state) - 1] = '\0';

        double start_wait = now_monotonic_sec();
        int first_is_left = (ph->id % 2 == 0);
        int first = first_is_left ? left_idx(ph->id) : right_idx(ph->id);
        int second = first_is_left ? right_idx(ph->id) : left_idx(ph->id);

        int got_first = 0, got_second = 0;
        // Acquire first with timeout
        if (mutex_timedlock_sec(&chop[first], cfg->timeout_sec) == 0) {
            got_first = 1;
            double elapsed = now_monotonic_sec() - start_wait;
            double remaining = cfg->timeout_sec - elapsed;
            if (remaining < 0) remaining = 0;
            if (mutex_timedlock_sec(&chop[second], remaining) == 0) {
                got_second = 1;
            } else {
                pthread_mutex_unlock(&chop[first]);
                got_first = 0;
            }
        }

        if (!(got_first && got_second)) {
            sleep_sec(rand_range(&seed, 0.01, 0.05));
            continue;
        }

        strncpy(ph->state, "eating", sizeof(ph->state) - 1);
        ph->state[sizeof(ph->state) - 1] = '\0';
        ph->meals += 1;
        ph->last_meal_at = now_monotonic_sec();
        sleep_sec(rand_range(&seed, 0.1, cfg->max_eat_sec));

        pthread_mutex_unlock(&chop[left_idx(ph->id)]);
        pthread_mutex_unlock(&chop[right_idx(ph->id)]);
        strncpy(ph->state, "thinking", sizeof(ph->state) - 1);
        ph->state[sizeof(ph->state) - 1] = '\0';
    }

    return NULL;
}

static void print_usage(void) {
    fprintf(stderr, "Usage: ./dining_philosophers_c [--run-time=SEC] [--max-think=SEC] [--max-eat=SEC] [--timeout=SEC] [--starvation-limit=SEC]\n");
}

static Config parse_args(int argc, char **argv) {
    Config cfg = {10.0, 1.5, 1.0, 1.0, 5.0};
    for (int i = 1; i < argc; ++i) {
        const char *arg = argv[i];
        if (strncmp(arg, "--run-time=", 11) == 0) cfg.run_time_sec = atof(arg + 11);
        else if (strncmp(arg, "--max-think=", 12) == 0) cfg.max_think_sec = atof(arg + 12);
        else if (strncmp(arg, "--max-eat=", 11) == 0) cfg.max_eat_sec = atof(arg + 11);
        else if (strncmp(arg, "--timeout=", 10) == 0) cfg.timeout_sec = atof(arg + 10);
        else if (strncmp(arg, "--starvation-limit=", 20) == 0) cfg.starvation_limit_sec = atof(arg + 20);
        else if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) { print_usage(); exit(0); }
        else { fprintf(stderr, "Unknown arg: %s\n", arg); print_usage(); exit(1); }
    }
    if (cfg.run_time_sec <= 0 || cfg.max_think_sec <= 0 || cfg.max_eat_sec <= 0 || cfg.timeout_sec <= 0 || cfg.starvation_limit_sec <= 0) {
        fprintf(stderr, "All time parameters must be positive.\n");
        exit(1);
    }
    return cfg;
}

int main(int argc, char **argv) {
    Config cfg = parse_args(argc, argv);

    pthread_mutex_t chopsticks[PHIL_COUNT];
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
#ifdef __linux__
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
#endif
    for (int i = 0; i < PHIL_COUNT; ++i) {
        pthread_mutex_init(&chopsticks[i], &attr);
    }
    pthread_mutexattr_destroy(&attr);

    Philosopher philosophers[PHIL_COUNT];
    pthread_t threads[PHIL_COUNT];

    double now = now_monotonic_sec();
    for (int i = 0; i < PHIL_COUNT; ++i) {
        philosophers[i].id = i;
        strncpy(philosophers[i].state, "thinking", sizeof(philosophers[i].state) - 1);
        philosophers[i].state[sizeof(philosophers[i].state) - 1] = '\0';
        philosophers[i].meals = 0;
        philosophers[i].last_meal_at = now;
        ThreadArgs *ta = (ThreadArgs *)malloc(sizeof(ThreadArgs));
        ta->ph = &philosophers[i];
        ta->chopsticks = chopsticks;
        ta->cfg = &cfg;
        pthread_create(&threads[i], NULL, philosopher_thread, ta);
    }

    sleep_sec(cfg.run_time_sec);
    running = 0;

    for (int i = 0; i < PHIL_COUNT; ++i) {
        pthread_join(threads[i], NULL);
    }

    printf("Dining Philosophers (C)\n");
    printf("Run: %.2fs | Max think: %.2fs | Max eat: %.2fs | Timeout: %.2fs\n",
           cfg.run_time_sec, cfg.max_think_sec, cfg.max_eat_sec, cfg.timeout_sec);
    printf("Asymmetric pickup with timeout-based release to avoid deadlock.\n\n");

    printf("%-6s %-8s %-18s %-10s %-10s\n", "Phil", "Meals", "Since Last (s)", "State", "Starving?");
    int deadlock = 1;
    int starvation = 0;
    double end_now = now_monotonic_sec();
    for (int i = 0; i < PHIL_COUNT; ++i) {
        double since = end_now - philosophers[i].last_meal_at;
        deadlock = deadlock && (philosophers[i].meals == 0);
        int starv = since > cfg.starvation_limit_sec;
        if (starv) starvation = 1;
        printf("%-6d %-8d %-18.2f %-10s %-10s\n", philosophers[i].id, philosophers[i].meals,
               since, philosophers[i].state, starv ? "YES" : "NO");
    }

    printf("\n%s\n", deadlock ? "Deadlock detected." : "No deadlock observed.");
    printf("%s\n", starvation ? "Starvation detected." : "No starvation detected.");

    for (int i = 0; i < PHIL_COUNT; ++i) {
        pthread_mutex_destroy(&chopsticks[i]);
    }

    return 0;
}
