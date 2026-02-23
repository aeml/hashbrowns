#include "memory_manager.h"

#include <algorithm>
#include <iomanip>
#include <iostream>

namespace hashbrowns {

MemoryTracker& MemoryTracker::instance() {
    static MemoryTracker instance;
    return instance;
}

void MemoryTracker::record_allocation(void* ptr, size_t size, const char* file, int line) {
    if (!ptr)
        return;

    std::lock_guard<std::mutex> lock(mutex_);

    // Update statistics
    stats_.total_allocated += size;
    stats_.current_usage += size;
    stats_.allocation_count += 1;

    // Update peak usage
    if (stats_.current_usage > stats_.peak_usage) {
        stats_.peak_usage = stats_.current_usage;
    }

    // Always record ptr->size mapping so deallocation can reverse the stats.
    // Verbose logging is gated on detailed_tracking_.
    allocations_[ptr] = size;

    if (detailed_tracking_ && file && line > 0) {
        std::cout << "[ALLOC] " << size << " bytes at " << ptr << " (" << file << ":" << line << ")\n";
    }
}

void MemoryTracker::record_deallocation(void* ptr) {
    if (!ptr)
        return;

    std::lock_guard<std::mutex> lock(mutex_);

    size_t size = 0;

    // Always look up the size so stats stay correct regardless of tracking mode.
    auto it = allocations_.find(ptr);
    if (it != allocations_.end()) {
        size = it->second;
        allocations_.erase(it);
        if (detailed_tracking_) {
            std::cout << "[DEALLOC] " << size << " bytes at " << ptr << "\n";
        }
    } else {
        if (detailed_tracking_) {
            std::cout << "[DEALLOC] Unknown allocation at " << ptr << "\n";
        }
        stats_.deallocation_count += 1; // Still count the deallocation
        return;
    }

    // Update statistics
    if (size > 0) {
        stats_.total_deallocated += size;
        stats_.current_usage -= size;
    }
    stats_.deallocation_count += 1;
}

void MemoryTracker::reset() {
    std::lock_guard<std::mutex> lock(mutex_);

    stats_.total_allocated    = 0;
    stats_.total_deallocated  = 0;
    stats_.current_usage      = 0;
    stats_.peak_usage         = 0;
    stats_.allocation_count   = 0;
    stats_.deallocation_count = 0;

    allocations_.clear();
}

bool MemoryTracker::check_leaks() const {
    std::lock_guard<std::mutex> lock(mutex_);

    Stats current_stats = stats_;

    std::cout << "\n=== Memory Leak Report ===\n";
    std::cout << std::fixed << std::setprecision(2);

    // Summary statistics
    std::cout << "Total allocated:     " << current_stats.total_allocated << " bytes\n";
    std::cout << "Total deallocated:   " << current_stats.total_deallocated << " bytes\n";
    std::cout << "Current usage:       " << current_stats.current_usage << " bytes\n";
    std::cout << "Peak usage:          " << current_stats.peak_usage << " bytes\n";
    std::cout << "Allocation count:    " << current_stats.allocation_count << "\n";
    std::cout << "Deallocation count:  " << current_stats.deallocation_count << "\n";

    // Memory efficiency metrics
    if (current_stats.peak_usage > 0) {
        double efficiency = static_cast<double>(current_stats.total_allocated) / current_stats.peak_usage;
        std::cout << "Memory efficiency:   " << efficiency << "x (allocated/peak)\n";
    }

    // Leak detection
    size_t leaked_bytes       = current_stats.memory_leaked();
    size_t outstanding_allocs = current_stats.outstanding_allocations();

    std::cout << "\n--- Leak Analysis ---\n";
    std::cout << "Leaked bytes:        " << leaked_bytes << "\n";
    std::cout << "Outstanding allocs:  " << outstanding_allocs << "\n";

    bool has_leaks = false;

    if (detailed_tracking_ && !allocations_.empty()) {
        has_leaks = true;
        std::cout << "\n--- Unfreed Allocations ---\n";

        size_t total_unfreed = 0;
        for (const auto& [ptr, size] : allocations_) {
            std::cout << "  " << ptr << ": " << size << " bytes\n";
            total_unfreed += size;
        }
        std::cout << "Total unfreed: " << total_unfreed << " bytes\n";
    } else if (leaked_bytes > 0 || outstanding_allocs > 0) {
        has_leaks = true;
        std::cout << "WARNING: Potential memory leaks detected!\n";
        std::cout << "Enable detailed tracking for more information.\n";
    }

    if (!has_leaks) {
        std::cout << "✓ No memory leaks detected!\n";
    }

    std::cout << "=========================\n\n";

    return !has_leaks;
}

} // namespace hashbrowns