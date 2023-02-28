#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <vector>
#include "ffs-lock.hpp"

void wait_randomly()
{
    std::mt19937_64 eng{std::random_device{}()};
    std::uniform_int_distribution<> dist{1, 10};
    std::this_thread::sleep_for(std::chrono::microseconds{dist(eng)});
}

FFSLock lock;
int value;
bool done = false;

void reader(FFSLock::ListElement *element)
{
    while (!done)
    {
        lock.lock_read(element);
        std::cout << value << std::endl;
        lock.unlock_read(element);

        wait_randomly();
    }
}

void writer(int start, int end, FFSLock::ListElement *element, bool wait = false)
{
    for (int i = start; i < end; i++)
    {
        lock.lock_write(element);
        value = i;
        wait_randomly();
        lock.unlock_write(element);

        //if (wait)
        wait_randomly();
    }
}

void test1()
{
    std::vector<std::thread> reader_threads;
    for (size_t i = 0; i < 2; i++)
    {
        auto element = new FFSLock::ListElement();
        reader_threads.push_back(std::thread(reader, element));
    }

    std::vector<std::thread> writer_threads;
    for (size_t i = 0; i < 1; i++)
    {
        auto element = new FFSLock::ListElement();
        writer_threads.push_back(std::thread(writer, i * 10, (i + 1) * 10, element, true));
    }

    for (auto &thread : writer_threads)
        thread.join();

    done = true;

    for (auto &thread : reader_threads)
        thread.join();
}
