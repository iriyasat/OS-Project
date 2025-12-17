# OS Project

Compact set of OS-focused exercises covering CPU scheduling, synchronization, hashing, and Windows clipboard handling.

## Repo Layout
- `A/` Collatz sequence driver in C (iterative with overflow-safe path lengths)
- `B/` MD5 directory hasher in C (recursive traversal, `-pthread`)
- `C/` Multi-level queue CPU scheduler in Python (RR/SRTF/FCFS with stats)
- `D/` Windows clipboard jammer + fix in C (Win32 API, event-driven)
- `E/` Dining Philosophers in Java, Python, and C (deadlock avoidance + starvation checks)

## Quickstart
Prereqs: `gcc` (C11), `python3`, `javac`/`java`. For Windows binaries on Linux, install `mingw-w64`; for native Windows, use MSVC or MinGW.

### A — Collatz (C)
```sh
cd /home/ihriyasat/Documents/OS/A && gcc -std=c11 -Wall -Wextra -O2 -o collatz collatz.c
./collatz 8
./collatz 35
```

### B — MD5 Hasher (C)
```sh
cd /home/ihriyasat/Documents/OS/B && gcc -std=c11 -Wall -Wextra -O2 -pthread -o md5hash md5Hash.c
./md5hash .
./md5hash ~/Downloads
```

### C — MLQ Scheduler (Python)
```sh
cd /home/ihriyasat/Documents/OS/C
python3 mlq.py sample_processes.txt
python3 mlq.py sample_processes.txt --quantum=4
python3 mlq.py sample_processes.txt --non-preemptive
```

### D — Clipboard Jammer/Fix (Windows-only C)
```sh
# MinGW-w64 cross (Linux -> Windows)
sudo apt install mingw-w64
cd /home/ihriyasat/Documents/OS/D
x86_64-w64-mingw32-gcc -O2 -municode -o clipboard_jammer.exe clipboard_jammer.c -luser32
x86_64-w64-mingw32-gcc -O2 -municode -o clipboard_jammer_fix.exe Reverse/clipboard_jammer_fix.c -luser32 -lkernel32 -ladvapi32

# MSVC (Developer Command Prompt)
cl /W4 /O2 clipboard_jammer.c user32.lib
cl /W4 /O2 Reverse\clipboard_jammer_fix.c user32.lib kernel32.lib advapi32.lib
```

### E — Dining Philosophers (Java/Python/C)
```sh
# Java
cd /home/ihriyasat/Documents/OS/E && javac DiningPhilosophers.java
java -cp /home/ihriyasat/Documents/OS/E DiningPhilosophers
java -cp /home/ihriyasat/Documents/OS/E DiningPhilosophers --run-time=15 --max-think=2 --max-eat=1.5 --timeout=1.0 --starvation-limit=6

# Python
python3 /home/ihriyasat/Documents/OS/E/dining_philosophers.py --run-time=15

# C
cd /home/ihriyasat/Documents/OS/E && gcc -std=c11 -O2 -pthread -o dining_philosophers_c DiningPhilosophers.c
./dining_philosophers_c --run-time=15
```

## Notes
- Folder `D` targets Windows; others run on Linux/macOS.
- Inputs are included: `C/sample_processes.txt` for the scheduler. Adjust paths as needed.
- See each `explain.txt` for design notes and viva Q&A.
