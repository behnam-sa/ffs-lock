#include <atomic>

/**
 * This is an implementation of a spin lock using std::atomic_flag. The lock
 * function acquires the lock by repeatedly attempting to set the atomic flag
 * until it becomes available, while the unlock function releases the lock by
 * clearing the atomic flag.
 */
class SpinLock
{
    // An atomic flag to indicate whether the lock is held or not
    std::atomic_flag locked = ATOMIC_FLAG_INIT;

public:
    /**
     * Acquire the lock
     */
    void lock()
    {
        // Keep trying to set the lock until it becomes available
        while (locked.test_and_set(std::memory_order_acquire))
            ;
    }

    /**
     * Release the lock
     */
    void unlock()
    {
        // Clear the lock flag to indicate that the lock is released
        locked.clear(std::memory_order_release);
    }
};
