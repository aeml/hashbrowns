#include "../src/structures/dynamic_array.h"
#include "../src/core/memory_manager.h"
#include <iostream>

using namespace hashbrowns;

int main() {
    std::cout << "Testing minimal DynamicArray creation and destruction..." << std::endl;
    
    // Reset memory tracker for clean testing
    auto& tracker = MemoryTracker::instance();
    tracker.set_detailed_tracking(true);
    tracker.reset();
    
    {
        std::cout << "Creating DynamicArray..." << std::endl;
        DynamicArray<int> arr;
        std::cout << "DynamicArray created, adding one element..." << std::endl;
        arr.push_back(42);
        std::cout << "Element added, array will be destroyed at end of scope..." << std::endl;
    }
    
    std::cout << "DynamicArray destroyed, checking memory..." << std::endl;
    
    auto stats = tracker.get_stats();
    std::cout << "Memory stats:" << std::endl;
    std::cout << "  Allocated: " << stats.total_allocated << " bytes" << std::endl;
    std::cout << "  Deallocated: " << stats.total_deallocated << " bytes" << std::endl;
    std::cout << "  Current usage: " << stats.current_usage << " bytes" << std::endl;
    std::cout << "  Alloc count: " << stats.allocation_count << std::endl;
    std::cout << "  Dealloc count: " << stats.deallocation_count << std::endl;
    
    if (stats.memory_leaked() > 0) {
        std::cout << "Memory leak detected!" << std::endl;
        tracker.check_leaks();
        return 1;
    } else {
        std::cout << "No memory leaks!" << std::endl;
        return 0;
    }
}