// SecretEngine
// Module: core
// Responsibility: Lock-free job system implementation
// Dependencies: JobSystem.h

#include <SecretEngine/JobSystem.h>
#include <chrono>

namespace SecretEngine {

JobSystem::JobSystem() {
    // Constructor - initialization happens in Initialize()
}

JobSystem::~JobSystem() {
    Shutdown();
}

void JobSystem::Initialize(uint32_t numThreads) {
    if (m_isRunning.load(std::memory_order_relaxed)) {
        return; // Already initialized
    }
    
    // Auto-detect thread count
    if (numThreads == 0) {
        numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) numThreads = 4; // Fallback
        
        // Reserve one thread for main thread
        if (numThreads > 1) {
            numThreads -= 1;
        }
    }
    
    m_numThreads = numThreads;
    m_isRunning.store(true, std::memory_order_release);
    
    // Spawn worker threads
    m_workers.reserve(numThreads);
    for (uint32_t i = 0; i < numThreads; ++i) {
        m_workers.emplace_back(&JobSystem::WorkerThread, this, i);
    }
}

void JobSystem::Shutdown() {
    if (!m_isRunning.load(std::memory_order_relaxed)) {
        return; // Not running
    }
    
    // Signal shutdown
    m_isRunning.store(false, std::memory_order_release);
    
    // Wait for all workers to finish
    for (auto& worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    m_workers.clear();
    m_numThreads = 0;
}

void JobSystem::Execute(JobFunction&& function, std::atomic<uint32_t>* counter) {
    if (!m_isRunning.load(std::memory_order_relaxed)) {
        // System not running, execute immediately on calling thread
        function();
        if (counter) {
            counter->fetch_sub(1, std::memory_order_release);
        }
        return;
    }
    
    // Get next write position
    uint32_t head = m_jobHead.load(std::memory_order_relaxed);
    uint32_t nextHead = (head + 1) % MAX_JOBS;
    
    // Check if queue is full
    uint32_t tail = m_jobTail.load(std::memory_order_acquire);
    if (nextHead == tail) {
        // Queue full, execute on calling thread
        function();
        if (counter) {
            counter->fetch_sub(1, std::memory_order_release);
        }
        return;
    }
    
    // Write job
    m_jobs[head].function = std::move(function);
    m_jobs[head].counter = counter;
    
    // Commit write
    m_jobHead.store(nextHead, std::memory_order_release);
}

void JobSystem::WaitForCounter(std::atomic<uint32_t>* counter) {
    if (!counter) return;
    
    // Spin-wait with yield (efficient for short waits)
    while (counter->load(std::memory_order_acquire) > 0) {
        // Help process jobs while waiting
        Job job;
        if (GetJob(job)) {
            job.function();
            if (job.counter) {
                job.counter->fetch_sub(1, std::memory_order_release);
            }
        } else {
            // No jobs available, yield to other threads
            std::this_thread::yield();
        }
    }
}

void JobSystem::WorkerThread(uint32_t threadIndex) {
    (void)threadIndex; // Unused for now, could be used for thread-local storage
    
    while (m_isRunning.load(std::memory_order_relaxed)) {
        Job job;
        if (GetJob(job)) {
            // Execute job
            job.function();
            
            // Decrement counter if present
            if (job.counter) {
                job.counter->fetch_sub(1, std::memory_order_release);
            }
        } else {
            // No jobs available, sleep briefly
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }
}

bool JobSystem::GetJob(Job& outJob) {
    uint32_t tail = m_jobTail.load(std::memory_order_relaxed);
    uint32_t head = m_jobHead.load(std::memory_order_acquire);
    
    // Check if queue is empty
    if (tail == head) {
        return false;
    }
    
    // Read job
    outJob = m_jobs[tail];
    
    // Commit read
    uint32_t nextTail = (tail + 1) % MAX_JOBS;
    m_jobTail.store(nextTail, std::memory_order_release);
    
    return true;
}

} // namespace SecretEngine
