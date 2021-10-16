#include <atomic>

#include <mutex>
#include <atomic>
#include <condition_variable>
#include <cassert>

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



#include <iostream>
#include <thread>
#include <vector>

semaphore sem(1);
int counter;

void run(int threadIdx) {

    sem.wait();
    std::cout << "Thread " << threadIdx << " enter critical section" << std::endl;
    counter++;
    std::cout << "Thread " << threadIdx << " incresed counter to " << counter << std::endl;

    // Do work;
    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    std::cout << "Thread " << threadIdx << " leave critical section" << std::endl;
    sem.notify();
}
int main() {
    std::vector<std::thread> threads;
    for(int i = 0; i < 15; i++) {
        threads.push_back(std::thread(run, i));
    }

    sem.notify();

    for(auto& t : threads) {
        t.join();
    }

    std::cout << "Terminate main." << std::endl;
    return 0;
}
