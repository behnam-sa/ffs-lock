#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <vector>
#include "ffs-lock.hpp"

void wait_randomly()
{
    std::mt19937_64 eng{std::random_device{}()};
    std::uniform_int_distribution<> dist{1, 100};
    std::this_thread::sleep_for(std::chrono::nanoseconds{dist(eng)});
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

void writer(int start, int count, FFSLock::ListElement *element)
{
    for (int i = start; i < start + count; i++)
    {
        lock.lock_write(element);
        value = i;
        wait_randomly();
        lock.unlock_write(element);

        wait_randomly();
    }
}

void test1()
{
    std::vector<std::thread> reader_threads;
    for (size_t i = 0; i < 20; i++)
    {
        auto element = new FFSLock::ListElement();
        reader_threads.push_back(std::thread(reader, element));
    }

    std::vector<std::thread> writer_threads;
    for (size_t i = 0; i < 5; i++)
    {
        auto element = new FFSLock::ListElement();
        writer_threads.push_back(std::thread(writer, i * 10, 10, element));
    }

    for (auto &thread : writer_threads)
        thread.join();

    done = true;

    for (auto &thread : reader_threads)
        thread.join();
}
