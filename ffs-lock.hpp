#include <cstddef>
#include <atomic>
#include "spin-lock.hpp"

class FFSLock
{
public:
    enum class State
    {
        READER,
        WRITER,
        ACTIVE_READER
    };

    struct ListElement
    {
        State state;
        size_t spin;
        ListElement *next, *prev;
        SpinLock exclusive_lock;
    };

private:
    std::atomic<ListElement *> tail;

public:
    FFSLock() : tail(nullptr) {}

    void lock_write(ListElement *element)
    {
        element->state = State::WRITER;
        element->spin = 1;
        element->next = nullptr;

        auto pred = tail.exchange(element);
        if (pred != nullptr)
        {
            pred->next = element;
            while (element->spin != 0)
                ;
        }
    }

    void unlock_write(ListElement *element)
    {
        auto expected = element;
        if (element->next == nullptr && tail.compare_exchange_strong(expected, nullptr))
            return;

        while (element->next == nullptr)
            ;
        element->next->prev = nullptr;
        element->next->spin = 0;
    }

    void lock_read(ListElement *element)
    {
        element->state = State::READER;
        element->spin = 1;
        element->next = element->prev = nullptr;

        auto pred = tail.exchange(element);
        if (pred != nullptr)
        {
            element->prev = pred;
            pred->next = element;
            if (pred->state != State::ACTIVE_READER)
            {
                while (element->spin != 0)
                    ;
            }
        }

        if (element->next != nullptr && element->next->state == State::READER)
            element->next->spin = 0;

        element->state = State::ACTIVE_READER;
    }

    void unlock_read(ListElement *element)
    {
        auto prev = element->prev;
        if (prev != nullptr)
        {
            prev->exclusive_lock.lock();

            while (prev != element->prev)
            {
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

                auto expected = element;
                if (element->next == nullptr && !tail.compare_exchange_strong(expected, element->prev))
                {
                    while (element->next == nullptr)
                        ;
                }

                if (element->next != nullptr)
                {
                    element->next->prev = element->prev;
                    element->prev->next = element->next;
                }

                element->exclusive_lock.unlock();
                prev->exclusive_lock.unlock();
                return;
            }
        }

        element->exclusive_lock.lock();
        auto expected = element;
        if (element->next == nullptr && !tail.compare_exchange_strong(expected, nullptr))
        {
            while (element->next == nullptr)
                ;
        }
        if (element->next != nullptr)
        {
            element->next->spin = 0;
            element->next->prev = nullptr; // Seems like the bug was here!
        }
        element->exclusive_lock.unlock();
    }
};
