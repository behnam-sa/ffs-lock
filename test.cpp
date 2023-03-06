// The following code defines a test function that creates and manages multiple
// reader and writer threads, which access a global variable named value using
// an FFSLock object. The test function creates 20 reader threads and 5 writer
// threads. Each writer thread writes 10 consecutive integers to value, while
// each reader thread reads the current value of value at random intervals.
// The test function waits for all writer threads to finish before setting a
// done flag to stop the reader threads and waits for them to finish.

#include "test.hpp"

#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <vector>
#include "ffs-lock.hpp"

// Function to wait for a random amount of time
void wait_randomly()
{
    // Initializing a random number generator with a seed from a hardware device
    std::mt19937_64 eng{std::random_device{}()};
    // Creating a uniform distribution in the range of [1, 100]
    std::uniform_int_distribution<> dist{1, 100};
    // Sleeping for a random amount of time in the range of [1, 100] nanoseconds
    std::this_thread::sleep_for(std::chrono::nanoseconds{dist(eng)});
}

// Initializing global variables
FFSLock lock;
int value;
bool done = false;

// Function to be executed by reader threads
void reader(FFSLock::ListElement *element)
{
    // Loop until done is true
    while (!done)
    {
        // Acquiring a read lock
        lock.lock_read(element);
        // Reading the value
        std::cout << value << std::endl;
        // Releasing the read lock
        lock.unlock_read(element);

        // Waiting for a random amount of time
        wait_randomly();
    }
}

// Function to be executed by writer threads
void writer(int start, int count, FFSLock::ListElement *element)
{
    // Looping through the specified range of values
    for (int i = start; i < start + count; i++)
    {
        // Acquiring a write lock
        lock.lock_write(element);
        // Updating the global value
        value = i;
        // Waiting for a random amount of time
        wait_randomly();
        // Releasing the write lock
        lock.unlock_write(element);

        // Waiting for a random amount of time
        wait_randomly();
    }
}

// Test function that creates and manages threads
void test()
{
    // Creating 20 reader threads
    std::vector<std::thread> reader_threads;
    for (size_t i = 0; i < 20; i++)
    {
        auto element = new FFSLock::ListElement();
        reader_threads.push_back(std::thread(reader, element));
    }

    // Creating 5 writer threads
    std::vector<std::thread> writer_threads;
    for (size_t i = 0; i < 5; i++)
    {
        auto element = new FFSLock::ListElement();
        writer_threads.push_back(std::thread(writer, i * 10, 10, element));
    }

    // Waiting for all writer threads to finish
    for (auto &thread : writer_threads)
        thread.join();

    // Setting the done flag to true to stop reader threads
    done = true;

    // Waiting for all reader threads to finish
    for (auto &thread : reader_threads)
        thread.join();
}
