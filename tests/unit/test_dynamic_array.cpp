#include "structures/dynamic_array.h"
#include "core/memory_manager.h"
#include <iostream>
#include <string>
#include <cassert>
#include <algorithm>
#include <functional>

using namespace hashbrowns;

void test_basic() {
    std::cout << "Testing basic operations..." << std::endl;
    
    // Basic construction and push_back
    DynamicArray<int> arr;
    assert(arr.size() == 0);
    assert(arr.empty());
    
    arr.push_back(1);
    arr.push_back(2);
    arr.push_back(3);
    
    assert(arr.size() == 3);
    assert(!arr.empty());
    assert(arr[0] == 1);
    assert(arr[1] == 2);
    assert(arr[2] == 3);
    
    std::cout << "✓ Basic operations passed" << std::endl;
}

void test_growth_strategies() {
    std::cout << "Testing growth strategies..." << std::endl;
    
    DynamicArray<int> arr(GrowthStrategy::MULTIPLICATIVE_2_0);
    for (int i = 0; i < 100; ++i) {
        arr.push_back(i);
    }
    
    assert(arr.size() == 100);
    for (int i = 0; i < 100; ++i) {
        assert(arr[i] == i);
    }
    
    std::cout << "✓ Growth strategies passed" << std::endl;
}

void test_iterators() {
    std::cout << "Testing iterators..." << std::endl;
    
    std::cout << "  Creating DynamicArray and adding elements manually..." << std::endl;
    DynamicArray<int, std::allocator<int>> arr;
    arr.push_back(1);
    arr.push_back(2);
    arr.push_back(3);
    arr.push_back(4);
    arr.push_back(5);
    
    std::cout << "  Testing forward iteration..." << std::endl;
    int expected = 1;
    for (auto it = arr.begin(); it != arr.end(); ++it) {
        assert(*it == expected++);
    }
    
    std::cout << "  Testing range-based for loop..." << std::endl;
    expected = 1;
    for (int value : arr) {
        assert(value == expected++);
    }
    
    std::cout << "  About to destroy DynamicArray..." << std::endl;
    
    std::cout << "✓ Iterators passed" << std::endl;
}

void test_memory_management() {
    std::cout << "Testing memory management..." << std::endl;
    
    DynamicArray<std::string> arr;
    arr.reserve(100);
    
    assert(arr.capacity() >= 100);
    assert(arr.size() == 0);
    
    arr.push_back("hello");
    arr.push_back("world");
    
    assert(arr.size() == 2);
    assert(arr[0] == "hello");
    assert(arr[1] == "world");
    
    arr.clear();
    assert(arr.size() == 0);
    assert(arr.empty());
    
    std::cout << "✓ Memory management passed" << std::endl;
}

void test_datastructure_interface() {
    std::cout << "Testing DataStructure interface..." << std::endl;
    
    DynamicArray<std::pair<int, std::string>> arr;
    
    arr.insert(1, "first");
    arr.insert(2, "second");
    arr.insert(3, "third");
    
    assert(arr.size() == 3);
    
    std::string value;
    assert(arr.search(2, value));
    assert(value == "second");
    
    assert(arr.remove(2));
    assert(arr.size() == 2);
    
    assert(!arr.search(2, value));
    
    std::cout << "✓ DataStructure interface passed" << std::endl;
}

void test_comprehensive() {
    std::cout << "Testing comprehensive DynamicArray functionality..." << std::endl;
    
    // Test all growth strategies
    for (auto strategy : {GrowthStrategy::MULTIPLICATIVE_2_0, 
                         GrowthStrategy::MULTIPLICATIVE_1_5,
                         GrowthStrategy::FIBONACCI, 
                         GrowthStrategy::ADDITIVE}) {
        // Use std::allocator in this heavy algorithmic test to isolate from tracking side-effects
        DynamicArray<int, std::allocator<int>> arr(strategy);
        
        // Test basic operations
        for (int i = 0; i < 100; ++i) {
            arr.push_back(i);
        }
        assert(arr.size() == 100);
        
        // Test access
        for (int i = 0; i < 100; ++i) {
            assert(arr[i] == i);
        }
        
        // Test copy constructor
    DynamicArray<int, std::allocator<int>> copy(arr);
        assert(copy.size() == arr.size());
        for (size_t i = 0; i < copy.size(); ++i) {
            assert(copy[i] == arr[i]);
        }
        
        // Test move constructor
        size_t original_size = arr.size();
    DynamicArray<int, std::allocator<int>> moved(std::move(arr));
        assert(moved.size() == original_size);
        assert(arr.size() == 0);
        
        // Test STL compatibility
        std::sort(moved.begin(), moved.end(), std::greater<int>());
        assert(moved[0] == 99);
        assert(moved[99] == 0);
    }
    
    std::cout << "✓ Comprehensive tests passed" << std::endl;
}

int main() {
    std::cout << "DynamicArray Test Suite\n";
    std::cout << "======================\n\n";
    
    // Reset memory tracker for clean testing
    auto& tracker = MemoryTracker::instance();
    tracker.set_detailed_tracking(true);  // Enable detailed tracking to debug
    tracker.reset();
    
    try {
        test_basic();
        test_growth_strategies();
        test_iterators();
        test_memory_management();
    test_datastructure_interface();
    test_comprehensive();
        
        std::cout << "\n✅ All tests passed!\n";
        
        // Check for memory leaks with detailed reporting
        auto final_stats = tracker.get_stats();
        if (final_stats.memory_leaked() > 0) {
            std::cout << "\n⚠️  Warning: Memory leaks detected!\n";
            tracker.check_leaks();
            return 1;
        }
        
        return 0;
    } catch (const std::exception& e) {
        std::cout << "\n❌ Test failed with exception: " << e.what() << "\n";
        return 1;
    }
}