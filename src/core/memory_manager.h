#ifndef HASHBROWNS_MEMORY_MANAGER_H
#define HASHBROWNS_MEMORY_MANAGER_H

#include <cstddef>
#include <memory>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <string>
#include <vector>
#include <type_traits>

namespace hashbrowns {

/**
 * @brief Memory tracking and debugging utilities for the benchmarking suite
 * 
 * This class provides detailed memory allocation tracking, leak detection,
 * and performance analysis for custom data structures. It's designed to be
 * lightweight in production but comprehensive for development and testing.
 */
class MemoryTracker {
public:
    /**
     * @brief Statistics about memory allocation patterns
     */
    struct Stats {
        size_t total_allocated{0};    ///< Total bytes allocated
        size_t total_deallocated{0};  ///< Total bytes deallocated
        size_t current_usage{0};      ///< Current memory usage
        size_t peak_usage{0};         ///< Peak memory usage
        size_t allocation_count{0};   ///< Number of allocations
        size_t deallocation_count{0}; ///< Number of deallocations

        /**
         * @brief Get current memory leakage (allocated - deallocated)
         */
        size_t memory_leaked() const {
            return total_allocated - total_deallocated;
        }

        /**
         * @brief Get number of outstanding allocations
         */
        size_t outstanding_allocations() const {
            return allocation_count - deallocation_count;
        }
    };

    /**
     * @brief Get the singleton instance of the memory tracker
     */
    static MemoryTracker& instance();

    /**
     * @brief Record a memory allocation
     * @param ptr Pointer to allocated memory
     * @param size Size of allocation in bytes
     * @param file Source file where allocation occurred
     * @param line Line number where allocation occurred
     */
    void record_allocation(void* ptr, size_t size, const char* file = nullptr, int line = 0);

    /**
     * @brief Record a memory deallocation
     * @param ptr Pointer to memory being deallocated
     */
    void record_deallocation(void* ptr);

    /**
     * @brief Get current memory statistics
     */
    Stats get_stats() const { 
        std::lock_guard<std::mutex> lock(mutex_);
        return stats_; 
    }

    /**
     * @brief Reset all tracking statistics
     */
    void reset();

    /**
     * @brief Check for memory leaks and print report
     * @return true if no leaks detected, false otherwise
     */
    bool check_leaks() const;

    /**
     * @brief Enable or disable detailed tracking (affects performance)
     */
    void set_detailed_tracking(bool enabled) { detailed_tracking_ = enabled; }

private:
    MemoryTracker() = default;
    ~MemoryTracker() = default;
    MemoryTracker(const MemoryTracker&) = delete;
    MemoryTracker& operator=(const MemoryTracker&) = delete;

    mutable std::mutex mutex_;
    Stats stats_;
    bool detailed_tracking_ = false;
    std::unordered_map<void*, size_t> allocations_; ///< ptr -> size mapping for leak detection
};

/**
 * @brief Custom allocator that integrates with memory tracking
 * 
 * This allocator can be used with STL containers or custom data structures
 * to provide detailed allocation tracking and analysis.
 */
template<typename T>
class TrackedAllocator {
public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    TrackedAllocator() = default;
    
    template<typename U>
    TrackedAllocator(const TrackedAllocator<U>&) noexcept {}

    /**
     * @brief Allocate memory for n objects of type T
     */
    T* allocate(size_type n) {
        if (n > std::numeric_limits<size_type>::max() / sizeof(T)) {
            throw std::bad_alloc();
        }
        
        size_t bytes = n * sizeof(T);
        T* ptr = static_cast<T*>(std::malloc(bytes));
        
        if (!ptr) {
            throw std::bad_alloc();
        }
        
        MemoryTracker::instance().record_allocation(ptr, bytes);
        return ptr;
    }

    /**
     * @brief Deallocate memory previously allocated by this allocator
     */
    void deallocate(T* ptr, size_type) noexcept {
        if (ptr) {
            MemoryTracker::instance().record_deallocation(ptr);
            std::free(ptr);
        }
    }

    template<typename U>
    bool operator==(const TrackedAllocator<U>&) const noexcept { return true; }
    
    template<typename U>
    bool operator!=(const TrackedAllocator<U>&) const noexcept { return false; }
};

/**
 * @brief RAII wrapper for raw memory management
 * 
 * This class provides safe, automatic cleanup of raw memory allocations
 * while integrating with the memory tracking system.
 */
template<typename T>
class unique_array {
public:
    /**
     * @brief Construct from raw pointer (takes ownership)
     */
    explicit unique_array(T* ptr = nullptr, size_t size = 0) 
        : ptr_(ptr), size_(size) {
        if (ptr_ && size_ > 0) {
            MemoryTracker::instance().record_allocation(ptr_, size_ * sizeof(T));
        }
    }

    /**
     * @brief Destructor - automatically deallocates memory
     */
    ~unique_array() {
        reset();
    }

    // Move semantics
    unique_array(unique_array&& other) noexcept 
        : ptr_(other.ptr_), size_(other.size_) {
        other.ptr_ = nullptr;
        other.size_ = 0;
    }

    unique_array& operator=(unique_array&& other) noexcept {
        if (this != &other) {
            reset();
            ptr_ = other.ptr_;
            size_ = other.size_;
            other.ptr_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    // Disable copying
    unique_array(const unique_array&) = delete;
    unique_array& operator=(const unique_array&) = delete;

    /**
     * @brief Access element at index
     */
    T& operator[](size_t index) {
        return ptr_[index];
    }

    const T& operator[](size_t index) const {
        return ptr_[index];
    }

    /**
     * @brief Get raw pointer
     */
    T* get() const noexcept { return ptr_; }

    /**
     * @brief Get array size
     */
    size_t size() const noexcept { return size_; }

    /**
     * @brief Release ownership and return raw pointer
     */
    T* release() noexcept {
        T* result = ptr_;
        ptr_ = nullptr;
        size_ = 0;
        return result;
    }

    /**
     * @brief Reset to new pointer (deallocates current)
     */
    void reset(T* ptr = nullptr, size_t size = 0) {
        if (ptr_ && size_ > 0) {
            MemoryTracker::instance().record_deallocation(ptr_);
            std::free(ptr_);
        }
        ptr_ = ptr;
        size_ = size;
        if (ptr_ && size_ > 0) {
            MemoryTracker::instance().record_allocation(ptr_, size_ * sizeof(T));
        }
    }

    /**
     * @brief Check if pointer is valid
     */
    explicit operator bool() const noexcept {
        return ptr_ != nullptr;
    }

private:
    T* ptr_;
    size_t size_;
};

/**
 * @brief Factory function to create tracked arrays
 */
template<typename T>
unique_array<T> make_unique_array(size_t count) {
    if (count == 0) {
        return unique_array<T>(nullptr, 0);
    }
    
    T* ptr = static_cast<T*>(std::malloc(count * sizeof(T)));
    if (!ptr) {
        throw std::bad_alloc();
    }
    
    return unique_array<T>(ptr, count);
}

/**
 * @brief Memory pool for efficient allocation of fixed-size objects
 * 
 * This pool reduces allocation overhead and fragmentation for data structures
 * that frequently allocate/deallocate objects of the same size (e.g., list nodes).
 */
template<typename T, size_t ChunkSize = 1024>
class MemoryPool {
public:
    MemoryPool() : free_list_(nullptr) {
        add_chunk();
    }

    /**
     * @brief Allocate one object from the pool (grows in chunks on demand)
     */
    T* allocate() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!free_list_) {
            add_chunk();
        }
        // Pop from free list
        Slot* slot = free_list_;
        free_list_ = free_list_->next;
        return reinterpret_cast<T*>(slot);
    }

    /**
     * @brief Return an object to the pool
     */
    void deallocate(T* ptr) {
        if (!ptr) return;
        std::lock_guard<std::mutex> lock(mutex_);
        Slot* slot = reinterpret_cast<Slot*>(ptr);
        slot->next = free_list_;
        free_list_ = slot;
    }

private:
    // Internal slot used for free list linking; shares storage with raw storage for T
    struct Slot { Slot* next; };
    using Storage = typename std::aligned_storage<sizeof(T), alignof(T)>::type;

    void add_chunk() {
        unique_array<Storage> chunk = make_unique_array<Storage>(ChunkSize);
        // Link all new slots into the free list
        // We push in forward order; order doesn't matter
        for (size_t i = 0; i < ChunkSize; ++i) {
            Storage* cell = &chunk[i];
            Slot* slot = reinterpret_cast<Slot*>(cell);
            slot->next = free_list_;
            free_list_ = slot;
        }
        chunks_.push_back(std::move(chunk));
    }

    std::vector< unique_array<Storage> > chunks_;
    Slot* free_list_;
    std::mutex mutex_;
};

} // namespace hashbrowns

// Convenience macros for tracked allocation (optional, for debugging)
#ifdef HASHBROWNS_TRACK_ALLOCATIONS
#define TRACKED_NEW(size) hashbrowns::MemoryTracker::instance().record_allocation(std::malloc(size), size, __FILE__, __LINE__)
#define TRACKED_DELETE(ptr) do { hashbrowns::MemoryTracker::instance().record_deallocation(ptr); std::free(ptr); } while(0)
#else
#define TRACKED_NEW(size) std::malloc(size)
#define TRACKED_DELETE(ptr) std::free(ptr)
#endif

#endif // HASHBROWNS_MEMORY_MANAGER_H