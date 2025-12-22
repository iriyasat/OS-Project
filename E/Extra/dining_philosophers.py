#!/usr/bin/env python3
import argparse
import random
import threading
import time
from dataclasses import dataclass, field
from typing import List

PHIL_COUNT = 5
DEFAULT_RUN_TIME = 10.0  # seconds
DEFAULT_MAX_THINK = 1.5
DEFAULT_MAX_EAT = 1.0
DEFAULT_TIMEOUT = 1.0

@dataclass
class Philosopher:
    idx: int
    state: str = "thinking"
    meals: int = 0
    last_meal_time: float = field(default_factory=time.time)


def left(idx: int) -> int:
    return idx

def right(idx: int) -> int:
    return (idx + 1) % PHIL_COUNT


def philosopher_thread(phil: Philosopher, chopsticks: List[threading.Lock], stop_evt: threading.Event,
                        max_think: float, max_eat: float, timeout: float, starvation_limit: float,
                        starvation_flags: List[bool]) -> None:
    rng = random.Random(phil.idx)
    while not stop_evt.is_set():
        phil.state = "thinking"
        time.sleep(rng.uniform(0.1, max_think))

        phil.state = "hungry"
        start_wait = time.time()
        acquired_left = acquired_right = False
        first = left(phil.idx) if phil.idx % 2 == 0 else right(phil.idx)
        second = right(phil.idx) if phil.idx % 2 == 0 else left(phil.idx)

        acquired_first = chopsticks[first].acquire(timeout=timeout)
        if acquired_first:
            if first == left(phil.idx):
                acquired_left = True
            else:
                acquired_right = True

            elapsed = time.time() - start_wait
            remaining = max(0.0, timeout - elapsed)
            acquired_second = chopsticks[second].acquire(timeout=remaining)
            if acquired_second:
                if second == left(phil.idx):
                    acquired_left = True
                else:
                    acquired_right = True
            else:
                if acquired_left:
                    chopsticks[left(phil.idx)].release()
                    acquired_left = False
                if acquired_right:
                    chopsticks[right(phil.idx)].release()
                    acquired_right = False
        if not (acquired_left and acquired_right):
            time.sleep(rng.uniform(0.01, 0.05))
            continue

        phil.state = "eating"
        phil.meals += 1
        phil.last_meal_time = time.time()
        time.sleep(rng.uniform(0.1, max_eat))

        chopsticks[left(phil.idx)].release()
        chopsticks[right(phil.idx)].release()
        phil.state = "thinking"

        if time.time() - phil.last_meal_time > starvation_limit:
            starvation_flags[phil.idx] = True


def run_sim(run_time: float, max_think: float, max_eat: float, timeout: float, starvation_limit: float) -> None:
    chopsticks = [threading.Lock() for _ in range(PHIL_COUNT)]
    philosophers = [Philosopher(idx=i) for i in range(PHIL_COUNT)]
    stop_evt = threading.Event()
    starvation_flags = [False] * PHIL_COUNT

    threads = [threading.Thread(target=philosopher_thread,
                                args=(ph, chopsticks, stop_evt, max_think, max_eat, timeout, starvation_limit, starvation_flags),
                                daemon=True)
               for ph in philosophers]

    for t in threads:
        t.start()

    time.sleep(run_time)
    stop_evt.set()

    for t in threads:
        t.join(timeout=1.0)

    print("Dining Philosophers (Python)")
    print(f"Run: {run_time:.2f}s | Max think: {max_think:.2f}s | Max eat: {max_eat:.2f}s | Timeout: {timeout:.2f}s")
    print("Asymmetric pickup with timeout-based release to avoid deadlock.\n")
    print(f"{'Phil':<6}{'Meals':<8}{'Since Last (s)':<16}{'State':<10}{'Starving?':<10}")
    now = time.time()
    deadlock = True
    for ph in philosophers:
        since = now - ph.last_meal_time
        starving = since > starvation_limit
        starvation_flags[ph.idx] = starvation_flags[ph.idx] or starving
        deadlock = deadlock and ph.meals == 0
        print(f"{ph.idx:<6}{ph.meals:<8}{since:<16.2f}{ph.state:<10}{'YES' if starvation_flags[ph.idx] else 'NO':<10}")

    print()
    print("Deadlock detected:" if deadlock else "No deadlock observed.")
    print("Starvation detected." if any(starvation_flags) else "No starvation detected.")


def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(description="Dining Philosophers (Python) with deadlock/starvation checks")
    p.add_argument("--run-time", type=float, default=DEFAULT_RUN_TIME)
    p.add_argument("--max-think", type=float, default=DEFAULT_MAX_THINK)
    p.add_argument("--max-eat", type=float, default=DEFAULT_MAX_EAT)
    p.add_argument("--timeout", type=float, default=DEFAULT_TIMEOUT)
    p.add_argument("--starvation-limit", type=float, default=DEFAULT_RUN_TIME/2)
    return p


def main() -> None:
    args = build_parser().parse_args()
    if args.run_time <= 0 or args.max_think <= 0 or args.max_eat <= 0 or args.timeout <= 0 or args.starvation_limit <= 0:
        raise SystemExit("All time parameters must be positive.")
    run_sim(args.run_time, args.max_think, args.max_eat, args.timeout, args.starvation_limit)

if __name__ == "__main__":
    main()
