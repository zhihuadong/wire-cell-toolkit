#include <mutex>
#include <atomic>
#include <condition_variable>
#include <cassert>
#include <chrono>

// https://vorbrodt.blog/2019/02/05/fast-semaphore/
//
// The "fast" version is indeed faster but both spin at MHz+ on
// i7-9750.
class semaphore
{
public:
    semaphore(int count=0) noexcept
    : m_count(count) { assert(count > -1); }

    void notify() noexcept
    {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            ++m_count;
        }
        m_cv.notify_one();
    }

    void wait() noexcept
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [&]() { return m_count != 0; });
        --m_count;
    }

private:
    int m_count;
    std::mutex m_mutex;
    std::condition_variable m_cv;
};

class fast_semaphore
{
public:
    fast_semaphore(int count) noexcept
    : m_count(count), m_semaphore(0) {}
 
    void notify()
    {
        std::atomic_thread_fence(std::memory_order_release);
        int count = m_count.fetch_add(1, std::memory_order_relaxed);
        if (count < 0)
            m_semaphore.notify();
    }
 
    void wait()
    {
        int count = m_count.fetch_sub(1, std::memory_order_relaxed);
        if (count < 1)
            m_semaphore.wait();
        std::atomic_thread_fence(std::memory_order_acquire);
    }
 
private:
    std::atomic<int> m_count;
    semaphore m_semaphore;
};


#include <iostream>
#include <thread>
#include <vector>

const int delay_ms = 0;
std::atomic<int> counter;

void run(fast_semaphore* sem, int threadIdx, int nfor) 
{
    while (nfor-- > 0) {
        sem->wait();
//        std::cout << "Thread " << threadIdx << " enter critical section" << std::endl;
        counter++;
//        std::cout << "Thread " << threadIdx << " incresed counter to " << counter << std::endl;

        // Do work;
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));

//        std::cout << "Thread " << threadIdx << " leave critical section" << std::endl;
        sem->notify();
    }
}
int main(int argc, char* argv[]) {
    int nsem = 0;
    if (argc > 1) {
        nsem = atoi(argv[1]);
    }

    //semaphore sem(nsem);
    fast_semaphore sem(nsem);

    const int nthreads = 15;
    std::vector<std::thread> threads;

    auto start = std::chrono::steady_clock::now();

    const int nloops = 1000000;
    for(int ind = 0; ind < nthreads; ++ind) {
        threads.push_back(std::thread(run, &sem, ind, nloops));
    }

    sem.notify();

    for(auto& t : threads) {
        t.join();
    }

    auto stop = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = stop - start;

    double duration = elapsed_seconds.count();
    double rate = counter/duration;

    std::cout
        << "nsem: " << nsem << "\n"
        << "nthreads: " << nthreads << "\n"
        << "nloops: " << nloops << "\n"
        << "delay: " << delay_ms << " ms\n"
        << "count: " << counter << "\n"
        << "time: " << duration << " s\n"
        // << "slept: " << slept << " s\n"
        // << "worked: " << worked << " s\n"
        << "rate: " << rate << " Hz\n";

    return 0;
}
