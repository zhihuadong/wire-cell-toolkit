/** A thread semaphore.

    At least until we get C++20.

    Use this to limit the number of active threads in a code context,
    eg in a service method.

*/
#ifndef WIRECELL_SEMAPHORE
#define WIRECELL_SEMAPHORE

#include <mutex>
#include <atomic>
#include <condition_variable>

namespace WireCell {

    // https://vorbrodt.blog/2019/02/05/fast-semaphore/
    //
    // The "fast" version below is indeed faster but both spin at MHz+
    // on i7-9750.
    class semaphore
    {
      public:
        semaphore(int count=0) noexcept
            : m_count(count) { assert(count > -1); }

        // Call before any use of acquire() or release().  Note, this
        // is number of *additional* threads so count=0 allows only
        // one thread.
        void set_count(int c)
        {
            m_count = c;
        }

        // Grab the semaphore, blocks until space is available.
        void acquire() noexcept
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [&]() { return m_count != 0; });
            --m_count;
        }

        // Remove hold on the semaphore.
        void release() noexcept
        {
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                ++m_count;
            }
            m_cv.notify_one();
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
 
        // Call before any use of acquire() or release().  Note, this
        // is number of *additional* threads so count=0 allows only
        // one thread.
        void set_count(int c)
        {
            m_count = c;
        }

        void acquire()
        {
            int count = m_count.fetch_sub(1, std::memory_order_relaxed);
            if (count < 1) {
                m_semaphore.acquire();
            }
            std::atomic_thread_fence(std::memory_order_acquire);
        }

        void release()
        {
            std::atomic_thread_fence(std::memory_order_release);
            int count = m_count.fetch_add(1, std::memory_order_relaxed);
            if (count < 0) {
                m_semaphore.release();
            }
        }
 
      private:
        std::atomic<int> m_count;
        semaphore m_semaphore;
    };
}


#endif
