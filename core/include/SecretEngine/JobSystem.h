// SecretEngine
// Module: core
// Responsibility: Lock-free job system for multithreading
// Dependencies: <atomic>, <thread>, <functional>

#pragma once
#include <atomic>
#include <thread>
#include <functional>
#include <vector>
#include <cstdint>

namespace SecretEngine {

// ============================================================================
// JOB SYSTEM - Lock-Free Work Stealing Task Scheduler
// ============================================================================

using JobFunction = std::function<void()>;

struct Job {
    JobFunction function;
    std::atomic<uint32_t>* counter; // Optional: for job completion tracking
};

class JobSystem {
public:
    static JobSystem& Instance() {
        static JobSystem instance;
        return instance;
    }
    
    // Initialize with worker thread count (call once at startup)
    void Initialize(uint32_t numThreads = 0); // 0 = auto-detect
    
    // Shutdown all worker threads (call at engine shutdown)
    void Shutdown();
    
    // Submit a job to the queue
    void Execute(JobFunction&& function, std::atomic<uint32_t>* counter = nullptr);
    
    // Wait for all jobs with this counter to complete
    void WaitForCounter(std::atomic<uint32_t>* counter);
    
    // Get number of worker threads
    uint32_t GetThreadCount() const { return m_numThreads; }
    
    // Check if system is running
    bool IsRunning() const { return m_isRunning.load(std::memory_order_relaxed); }
    
private:
    JobSystem();
    ~JobSystem();
    JobSystem(const JobSystem&) = delete;
    JobSystem& operator=(const JobSystem&) = delete;
    
    void WorkerThread(uint32_t threadIndex);
    bool GetJob(Job& outJob);
    
    // Lock-free ring buffer for jobs
    static constexpr uint32_t MAX_JOBS = 4096;
    Job m_jobs[MAX_JOBS];
    std::atomic<uint32_t> m_jobHead{0};  // Write position
    std::atomic<uint32_t> m_jobTail{0};  // Read position
    
    // Worker threads
    std::vector<std::thread> m_workers;
    uint32_t m_numThreads{0};
    std::atomic<bool> m_isRunning{false};
};

// ============================================================================
// PARALLEL FOR - Execute function for each index in parallel
// ============================================================================

template<typename Func>
void ParallelFor(uint32_t count, Func&& func) {
    if (count == 0) return;
    
    JobSystem& jobs = JobSystem::Instance();
    uint32_t numThreads = jobs.GetThreadCount();
    
    if (numThreads == 0 || count < numThreads * 2) {
        // Not enough work to parallelize, run sequentially
        for (uint32_t i = 0; i < count; ++i) {
            func(i);
        }
        return;
    }
    
    // Split work across threads
    std::atomic<uint32_t> counter{numThreads};
    uint32_t batchSize = (count + numThreads - 1) / numThreads;
    
    for (uint32_t t = 0; t < numThreads; ++t) {
        uint32_t start = t * batchSize;
        uint32_t end = std::min(start + batchSize, count);
        
        if (start >= count) break;
        
        jobs.Execute([start, end, &func, &counter]() {
            for (uint32_t i = start; i < end; ++i) {
                func(i);
            }
            counter.fetch_sub(1, std::memory_order_release);
        }, &counter);
    }
    
    // Wait for completion
    jobs.WaitForCounter(&counter);
}

} // namespace SecretEngine
