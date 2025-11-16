#include "core/memory_manager.h"
#include <iostream>

using namespace hashbrowns;

int run_memory_tracker_tests() {
    int failures = 0;
    std::cout << "MemoryTracker Test Suite\n";
    std::cout << "========================\n\n";

    // Basic reset should zero stats
    {
        auto& tracker = MemoryTracker::instance();
        tracker.reset();
        auto stats = tracker.get_stats();
        if (stats.current_usage != 0 ||
            stats.total_allocated != 0 ||
            stats.total_deallocated != 0 ||
            stats.allocation_count != 0 ||
            stats.deallocation_count != 0) {
            std::cout << "❌ MemoryTracker reset did not zero stats\n";
            ++failures;
        }
    }

    // Allocation and deallocation via unique_array / make_unique_array
    {
        auto& tracker = MemoryTracker::instance();
        tracker.reset();

        {
            auto arr = make_unique_array<int>(16);
            auto stats = tracker.get_stats();
            if (stats.current_usage == 0 || stats.allocation_count == 0) {
                std::cout << "❌ Allocation not tracked correctly\n";
                ++failures;
            }
        } // arr goes out of scope and should deallocate

        bool no_leaks = MemoryTracker::instance().check_leaks();
        if (!no_leaks) {
            std::cout << "❌ Leak reported when all allocations freed\n";
            ++failures;
        }
    }

    // Optional: intentionally leak allocation and verify check_leaks() reports it
    {
        auto& tracker = MemoryTracker::instance();
        tracker.reset();

        // Use detailed tracking so the tracker can associate ptr->size
        tracker.set_detailed_tracking(true);
        {
            // Intentionally leak by releasing ownership without freeing
            auto arr = make_unique_array<int>(8);
            (void)arr.release();
        }

        bool no_leaks = tracker.check_leaks();
        if (no_leaks) {
            std::cout << "❌ check_leaks() failed to detect intentional leak\n";
            ++failures;
        }

        // Clean up state for other tests
        tracker.reset();
        tracker.set_detailed_tracking(false);
    }

    if (failures == 0) {
        std::cout << "\n✅ MemoryTracker tests passed!\n";
    } else {
        std::cout << "\n❌ " << failures << " MemoryTracker test(s) failed.\n";
    }

    return failures == 0 ? 0 : 1;
}
