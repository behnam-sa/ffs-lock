#include <cstddef>
#include <atomic>
#include "spin-lock.hpp"

/**
 * This code implements a fast, fair, and scalable lock (FFS lock) that allows multiple readers and exclusive writers.
 * The lock is implemented using a doubly linked list of ListElement structures, which represent threads that are waiting for the lock.
 */
class FFSLock
{
public:
    enum class State // List element state
    {
        READER,
        WRITER,
        ACTIVE_READER
    };

    struct ListElement // Implementation for Lelem record in the paper
    {
        State state;              // State of the element
        size_t spin;              // Local spin variable
        ListElement *next, *prev; // Pointers to the next and previous elements
        SpinLock exclusive_lock;  // Lock used for exclusive access in reader unlock
    };

private:
    std::atomic<ListElement *> tail; // Atomic pointer to the tail of the list

public:
    FFSLock() : tail(nullptr) {}

    /**
     * Locks the FFSLock for a writer element. (Implementation of writerLock)
     * 
     * @param element A pointer to the ListElement that represents the writer.
     */
    void lock_write(ListElement *element)
    {
        // Set the element's state to writer and initialize spin and next.
        element->state = State::WRITER;
        element->spin = 1;
        element->next = nullptr;

        // Atomically exchange the tail with the element and get the previous tail.
        auto pred = tail.exchange(element);

        // If the previous tail is not null, set its next to element and wait for element's spin to be 0.
        if (pred != nullptr)
        {
            pred->next = element;
            while (element->spin != 0)
                ; // busy-wait for spin to be zero
        }
    }

    /**
     * Unlocks a writer element from the queue and notifies the next element in the queue if there is one. (Implementation of writerUnlock)
     * 
     * @param element A pointer to the ListElement that represents the writer.
     */
    void unlock_write(ListElement *element)
    {
        // If the element is at the end of the queue and there are no other elements, just set the tail to null
        auto expected = element;
        if (element->next == nullptr && tail.compare_exchange_strong(expected, nullptr))
            return;

        // If there are other elements in the queue, wait until the next element has been set
        while (element->next == nullptr)
            ;

        // Set the previous pointer of the next element to null, indicating that this element has been removed from the queue
        element->next->prev = nullptr;

        // Release the spin lock of the next element to allow it to continue execution
        element->next->spin = 0;
    }

    /**
     * Locks the FFSLock for a writer element. (Implementation of readerLock)
     * 
     * @param element A pointer to the ListElement that represents the reader.
     */
    void lock_read(ListElement *element)
    {
        element->state = State::READER;
        element->spin = 1;
        element->next = element->prev = nullptr;

        // Try to acquire the lock atomically, by exchanging the current tail with the new element.
        auto pred = tail.exchange(element);

        // If the tail was not null, this element must be added to the list and wait until the previous not ACTIVE_READER element releases its lock.
        if (pred != nullptr)
        {
            // Set the previous element as the predecessor of this element.
            element->prev = pred;
            // Link this element to the previous element.
            pred->next = element;

            // If the previous element is not an active reader, spin until it releases its lock.
            if (pred->state != State::ACTIVE_READER)
            {
                while (element->spin != 0)
                    ;
            }
        }

        // If the next element is a reader, release its spinlock.
        if (element->next != nullptr && element->next->state == State::READER)
            element->next->spin = 0;

        // Set the state of this element to active reader.
        element->state = State::ACTIVE_READER;
    }

    /**
     * Unlocks the read lock for the given ListElement. (Implementation of readerUnlock)
     *
     * @param element A pointer to the ListElement that represents the reader.
     */
    void unlock_read(ListElement *element)
    {
        auto prev = element->prev;
        if (prev != nullptr)
        {
            // If the previous element is not null, lock its exclusive lock
            prev->exclusive_lock.lock();

            // Keep looping until we have the correct previous element
            while (prev != element->prev)
            {
                // Unlock the previous element's exclusive lock and lock the new one's
                prev->exclusive_lock.unlock();
                prev = element->prev;
                if (prev == nullptr)
                    break;
                prev->exclusive_lock.lock();
            }

            if (prev != nullptr)
            {
                element->exclusive_lock.lock();
                prev->next = nullptr;

                // Try to set the tail to the previous element, if the next pointer of the current element is null
                auto expected = element;
                if (element->next == nullptr && !tail.compare_exchange_strong(expected, element->prev))
                {
                    // Wait until the next pointer of the current element is not null
                    while (element->next == nullptr)
                        ;
                }

                // If the current element has a next element, update its previous and next pointers
                if (element->next != nullptr)
                {
                    element->next->prev = element->prev;
                    element->prev->next = element->next;
                }

                // Unlock the current and previous elements
                element->exclusive_lock.unlock();
                prev->exclusive_lock.unlock();
                return;
            }
        }

        // Lock the current element's exclusive lock
        element->exclusive_lock.lock();

        // Try to set the tail to null if it is currently the current element
        auto expected = element;
        if (element->next == nullptr && !tail.compare_exchange_strong(expected, nullptr))
        {
            // Wait until the next pointer of the current element is not null
            while (element->next == nullptr)
                ;
        }

        // If the current element has a next element, set its spin to 0 and update its previous pointer
        if (element->next != nullptr)
        {
            element->next->spin = 0;
            element->next->prev = nullptr; // The bug in the pseudoscope is fixed here
        }

        // Unlock the current element's exclusive lock
        element->exclusive_lock.unlock();
    }
};
