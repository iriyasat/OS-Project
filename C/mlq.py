#!/usr/bin/env python3
import argparse
import math
import os
from dataclasses import dataclass, field
from typing import List, Optional

MAX_PROCESSES = 128

@dataclass
class Process:
    pid: str
    arrival: int
    burst: int
    queue: int
    remaining: int = field(init=False)
    start_time: int = -1
    completion_time: int = 0
    response_time: int = 0
    waiting_time: int = 0
    turnaround_time: int = 0
    finished: bool = False
    enqueued: bool = False

    def __post_init__(self) -> None:
        self.remaining = self.burst


def parse_process_file(path: str) -> List[Process]:
    procs: List[Process] = []
    if not os.path.exists(path):
        raise FileNotFoundError(f"Could not open {path}")

    with open(path, "r", encoding="utf-8") as f:
        for line in f:
            stripped = line.strip()
            if not stripped or stripped.startswith("#"):
                continue

            parts = stripped.split()
            if len(parts) != 4:
                print(f"Skipping invalid line: {stripped}")
                continue

            pid, arrival_s, burst_s, queue_s = parts
            try:
                arrival = int(arrival_s)
                burst = int(burst_s)
                queue = int(queue_s)
            except ValueError:
                print(f"Skipping invalid numeric values: {stripped}")
                continue

            if arrival < 0 or burst <= 0:
                print(f"Skipping line with invalid times: {stripped}")
                continue

            queue = max(0, min(2, queue))
            procs.append(Process(pid=pid[:31], arrival=arrival, burst=burst, queue=queue))

            if len(procs) >= MAX_PROCESSES:
                print(f"Reached max process limit ({MAX_PROCESSES}). Extra lines ignored.")
                break

    if not procs:
        raise ValueError("No valid processes loaded.")
    return procs


def next_arrival_after(procs: List[Process], current_time: int) -> Optional[int]:
    candidates = [p.arrival for p in procs if not p.finished and p.arrival > current_time]
    if not candidates:
        return None
    return min(candidates)


def add_arrivals(procs: List[Process], time: int, ready: List[List[int]], current_idx: Optional[int]) -> None:
    for idx, p in enumerate(procs):
        if p.finished or p.enqueued or p.arrival > time or idx == current_idx:
            continue
        ready[p.queue].append(idx)
        p.enqueued = True


def min_remaining_pos(ready_q: List[int], procs: List[Process]) -> int:
    if not ready_q:
        return -1
    best_pos = 0
    best_remaining = procs[ready_q[0]].remaining
    for pos, idx in enumerate(ready_q[1:], start=1):
        if procs[idx].remaining < best_remaining:
            best_remaining = procs[idx].remaining
            best_pos = pos
    return best_pos


def simulate(procs: List[Process], quantum: int, preemptive: bool) -> None:
    ready: List[List[int]] = [[], [], []]
    time = 0
    completed = 0
    busy_time = 0
    current: Optional[int] = None
    current_q = -1
    slice_left = quantum

    n = len(procs)

    while completed < n:
        add_arrivals(procs, time, ready, current)

        any_ready = sum(len(q) for q in ready)
        if current is None and any_ready == 0:
            next_time = next_arrival_after(procs, time)
            if next_time is None:
                break
            time = next_time
            continue

        rr_expired = current_q == 0 and current is not None and slice_left <= 0 and procs[current].remaining > 0

        higher_ready = False
        if preemptive and current is not None:
            for q in range(0, current_q):
                if ready[q]:
                    higher_ready = True
                    break

        srtf_better = False
        if preemptive and current is not None and current_q == 1 and ready[1]:
            pos = min_remaining_pos(ready[1], procs)
            cand_idx = ready[1][pos]
            if procs[cand_idx].remaining < procs[current].remaining:
                srtf_better = True

        need_switch = (
            current is None
            or rr_expired
            or procs[current].remaining == 0
            or (preemptive and (higher_ready or srtf_better))
        )

        if need_switch:
            if current is not None and procs[current].remaining > 0:
                ready[current_q].append(current)
                procs[current].enqueued = True

            current = None
            current_q = -1
            slice_left = quantum

            if ready[0]:
                current = ready[0].pop(0)
                current_q = 0
                slice_left = quantum
                procs[current].enqueued = False
            elif ready[1]:
                pos = min_remaining_pos(ready[1], procs)
                current = ready[1].pop(pos)
                current_q = 1
                procs[current].enqueued = False
            elif ready[2]:
                current = ready[2].pop(0)
                current_q = 2
                procs[current].enqueued = False

        if current is None:
            time += 1
            continue

        p = procs[current]
        if p.start_time == -1:
            p.start_time = time
            p.response_time = p.start_time - p.arrival

        p.remaining -= 1
        busy_time += 1
        if current_q == 0:
            slice_left -= 1

        if p.remaining == 0:
            p.completion_time = time + 1
            p.turnaround_time = p.completion_time - p.arrival
            p.waiting_time = p.turnaround_time - p.burst
            p.finished = True
            p.enqueued = False
            completed += 1
            current = None
            current_q = -1
            slice_left = quantum

        time += 1

    last_completion = max((p.completion_time for p in procs), default=0)
    avg_tat = sum(p.turnaround_time for p in procs) / n
    avg_wait = sum(p.waiting_time for p in procs) / n
    avg_resp = sum(p.response_time for p in procs) / n

    cpu_util = 100.0 * busy_time / last_completion if last_completion else 0.0
    throughput = n / last_completion if last_completion else 0.0

    mode = "preemptive" if preemptive else "non-preemptive"
    print(f"Multi-level queue scheduling ({mode}), quantum={quantum}")
    print("Queue 0: Round Robin | Queue 1: Shortest Remaining Time First | Queue 2: First-Come, First-Served\n")

    header = f"{'PID':<10} {'Arr':<6} {'Burst':<6} {'Queue':<6} {'Start':<8} {'Complete':<11} {'Turnaround':<11} {'Waiting':<9} {'Response':<9}"
    print(header)
    for p in procs:
        print(f"{p.pid:<10} {p.arrival:<6} {p.burst:<6} {p.queue:<6} {p.start_time:<8} {p.completion_time:<11} {p.turnaround_time:<11} {p.waiting_time:<9} {p.response_time:<9}")

    print("\nCPU Utilization: {:.2f}%".format(cpu_util))
    print("Throughput: {:.3f} processes/unit time".format(throughput))
    print("Average Turnaround: {:.2f}".format(avg_tat))
    print("Average Waiting: {:.2f}".format(avg_wait))
    print("Average Response: {:.2f}".format(avg_resp))
    print(f"Total Time: {last_completion} units")


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Multi-level queue CPU scheduler simulation (Queue0=RR, Queue1=SRTF, Queue2=FCFS)",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument("process_file", help="Path to process definition file (PID ARRIVAL BURST QUEUE)")
    parser.add_argument("--quantum", type=int, default=2, help="Time quantum for Round Robin queue (queue 0)")
    parser.add_argument("--preemptive", dest="preemptive", action="store_true", help="Allow higher-priority preemption (default)")
    parser.add_argument("--non-preemptive", dest="preemptive", action="store_false", help="Disable preemption across queues")
    parser.set_defaults(preemptive=True)
    return parser


def main() -> None:
    parser = build_arg_parser()
    args = parser.parse_args()

    if args.quantum <= 0:
        parser.error("--quantum must be positive")

    procs = parse_process_file(args.process_file)
    simulate(procs, quantum=args.quantum, preemptive=args.preemptive)


if __name__ == "__main__":
    main()
