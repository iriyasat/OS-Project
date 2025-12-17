import java.util.Arrays;
import java.util.Random;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;


public class DiningPhilosophers {
    private static final int PHIL_COUNT = 5;

    private static class Philosopher implements Runnable {
        private final int id;
        private final Lock left;
        private final Lock right;
        private final Random rng;
        private final long maxThinkMillis;
        private final long maxEatMillis;
        private final long timeoutMillis;
        private final long starvationLimitMillis;
        private volatile String state = "thinking";
        private volatile int meals = 0;
        private volatile long lastMealAt;
        private volatile boolean starvingFlag = false;
        private volatile boolean running = true;

        Philosopher(int id, Lock left, Lock right, long maxThinkMillis, long maxEatMillis,
                    long timeoutMillis, long starvationLimitMillis) {
            this.id = id;
            this.left = left;
            this.right = right;
            this.maxThinkMillis = maxThinkMillis;
            this.maxEatMillis = maxEatMillis;
            this.timeoutMillis = timeoutMillis;
            this.starvationLimitMillis = starvationLimitMillis;
            this.rng = new Random(id);
            this.lastMealAt = System.currentTimeMillis();
        }

        @Override
        public void run() {
            while (running) {
                think();
                attemptToEat();
            }
        }

        private void think() {
            state = "thinking";
            sleepRandom(maxThinkMillis);
        }

        private void attemptToEat() {
            state = "hungry";
            long startWait = System.currentTimeMillis();
            boolean firstIsLeft = (id % 2 == 0);
            Lock first = firstIsLeft ? left : right;
            Lock second = firstIsLeft ? right : left;

            boolean gotFirst = false;
            boolean gotSecond = false;

            try {
                gotFirst = first.tryLock(timeoutMillis, TimeUnit.MILLISECONDS);
                if (gotFirst) {
                    long elapsed = System.currentTimeMillis() - startWait;
                    long remaining = Math.max(0L, timeoutMillis - elapsed);
                    gotSecond = second.tryLock(remaining, TimeUnit.MILLISECONDS);
                }

                if (gotFirst && gotSecond) {
                    eat();
                }
            } catch (InterruptedException ie) {
                Thread.currentThread().interrupt();
            } finally {
                if (gotSecond) {
                    second.unlock();
                }
                if (gotFirst) {
                    first.unlock();
                }
            }

            if (System.currentTimeMillis() - lastMealAt > starvationLimitMillis) {
                starvingFlag = true;
            }

            sleepRandom(50L);
        }

        private void eat() {
            state = "eating";
            meals++;
            lastMealAt = System.currentTimeMillis();
            sleepRandom(maxEatMillis);
            state = "thinking";
        }

        private void sleepRandom(long maxMillis) {
            if (maxMillis <= 0) {
                return;
            }
            long dur = 50 + Math.abs(rng.nextLong()) % maxMillis;
            try {
                Thread.sleep(dur);
            } catch (InterruptedException ie) {
                Thread.currentThread().interrupt();
            }
        }

        void stop() {
            running = false;
        }
    }

    private final long runTimeMillis;
    private final long maxThinkMillis;
    private final long maxEatMillis;
    private final long timeoutMillis;
    private final long starvationLimitMillis;

    public DiningPhilosophers(long runTimeMillis, long maxThinkMillis, long maxEatMillis,
                              long timeoutMillis, long starvationLimitMillis) {
        this.runTimeMillis = runTimeMillis;
        this.maxThinkMillis = maxThinkMillis;
        this.maxEatMillis = maxEatMillis;
        this.timeoutMillis = timeoutMillis;
        this.starvationLimitMillis = starvationLimitMillis;
    }

    public void runSimulation() {
        Lock[] chopsticks = new Lock[PHIL_COUNT];
        Arrays.setAll(chopsticks, i -> new ReentrantLock());

        Philosopher[] philosophers = new Philosopher[PHIL_COUNT];
        for (int i = 0; i < PHIL_COUNT; i++) {
            Lock left = chopsticks[i];
            Lock right = chopsticks[(i + 1) % PHIL_COUNT];
            philosophers[i] = new Philosopher(i, left, right, maxThinkMillis, maxEatMillis, timeoutMillis, starvationLimitMillis);
        }

        ExecutorService exec = Executors.newFixedThreadPool(PHIL_COUNT);
        for (Philosopher p : philosophers) {
            exec.submit(p);
        }

        sleep(runTimeMillis);
        for (Philosopher p : philosophers) {
            p.stop();
        }
        exec.shutdownNow();
        try {
            exec.awaitTermination(2, TimeUnit.SECONDS);
        } catch (InterruptedException ignored) {
            Thread.currentThread().interrupt();
        }

        report(philosophers);
    }

    private void report(Philosopher[] philosophers) {
        System.out.println("Dining Philosophers Simulation Results (Java)");
        System.out.printf("Run duration: %.2fs | Max think: %.2fs | Max eat: %.2fs | Timeout: %.2fs%n",
                runTimeMillis / 1000.0, maxThinkMillis / 1000.0, maxEatMillis / 1000.0, timeoutMillis / 1000.0);
        System.out.println("Asymmetric pickup (even: left->right, odd: right->left) with timeout-based release to avoid deadlock.\n");

        boolean deadlock = true;
        boolean starvation = false;
        long now = System.currentTimeMillis();

        System.out.printf("%-6s %-8s %-18s %-10s %-10s%n", "Phil", "Meals", "Since Last Meal (s)", "State", "Starving?");
        for (Philosopher p : philosophers) {
            double since = (now - p.lastMealAt) / 1000.0;
            deadlock &= (p.meals == 0);
            starvation |= p.starvingFlag;
            System.out.printf("%-6d %-8d %-18.2f %-10s %-10s%n",
                    p.id, p.meals, since, p.state, p.starvingFlag ? "YES" : "NO");
        }

        System.out.println();
        if (deadlock) {
            System.out.println("Deadlock detected: no meals were completed.");
        } else {
            System.out.println("No deadlock observed (meals were completed).");
        }

        if (starvation) {
            System.out.println("Starvation detected: at least one philosopher exceeded the starvation limit.");
        } else {
            System.out.println("No starvation detected within the run window.");
        }
    }

    private static void sleep(long millis) {
        try {
            Thread.sleep(millis);
        } catch (InterruptedException ie) {
            Thread.currentThread().interrupt();
        }
    }

    private static long parseMillis(double seconds, String name) {
        if (seconds <= 0) throw new IllegalArgumentException(name + " must be positive");
        return (long) (seconds * 1000);
    }

    public static void main(String[] args) {
        double runTimeSec = 10.0;
        double maxThinkSec = 1.5;
        double maxEatSec = 1.0;
        double timeoutSec = 1.0;
        double starvationLimitSec = 5.0;

        for (String arg : args) {
            if (arg.startsWith("--run-time=")) {
                runTimeSec = Double.parseDouble(arg.substring("--run-time=".length()));
            } else if (arg.startsWith("--max-think=")) {
                maxThinkSec = Double.parseDouble(arg.substring("--max-think=".length()));
            } else if (arg.startsWith("--max-eat=")) {
                maxEatSec = Double.parseDouble(arg.substring("--max-eat=".length()));
            } else if (arg.startsWith("--timeout=")) {
                timeoutSec = Double.parseDouble(arg.substring("--timeout=".length()));
            } else if (arg.startsWith("--starvation-limit=")) {
                starvationLimitSec = Double.parseDouble(arg.substring("--starvation-limit=".length()));
            } else if (arg.equals("--help") || arg.equals("-h")) {
                printUsage();
                return;
            } else {
                System.err.println("Unknown argument: " + arg);
                printUsage();
                return;
            }
        }

        long runMillis = parseMillis(runTimeSec, "--run-time");
        long thinkMillis = parseMillis(maxThinkSec, "--max-think");
        long eatMillis = parseMillis(maxEatSec, "--max-eat");
        long timeoutMillis = parseMillis(timeoutSec, "--timeout");
        long starvationMillis = parseMillis(starvationLimitSec, "--starvation-limit");

        DiningPhilosophers sim = new DiningPhilosophers(runMillis, thinkMillis, eatMillis, timeoutMillis, starvationMillis);
        sim.runSimulation();
    }

    private static void printUsage() {
        System.out.println("Usage: java DiningPhilosophers [--run-time=SECONDS] [--max-think=SECONDS] [--max-eat=SECONDS] " +
                "[--timeout=SECONDS] [--starvation-limit=SECONDS]");
    }
}
