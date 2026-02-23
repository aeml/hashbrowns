#include "core/memory_manager.h"
#include "structures/dynamic_array.h"

#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <string>

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

void test_additional_functions() {
    std::cout << "Testing additional functions for coverage..." << std::endl;

    // Test DataStructure interface methods
    DynamicArray<std::pair<int, std::string>> arr;

    // Test complexity methods
    std::string type_n   = arr.type_name();
    std::string insert_c = arr.insert_complexity();
    std::string search_c = arr.search_complexity();
    std::string remove_c = arr.remove_complexity();

    assert(!type_n.empty());
    assert(!insert_c.empty());
    assert(!search_c.empty());
    assert(!remove_c.empty());

    // Test copy and move assignment
    arr.insert(1, "one");
    arr.insert(2, "two");

    DynamicArray<std::pair<int, std::string>> arr2;
    arr2 = arr; // Copy assignment
    assert(arr2.size() == 2);

    DynamicArray<std::pair<int, std::string>> arr3;
    arr3 = std::move(arr2); // Move assignment
    assert(arr3.size() == 2);
    assert(arr2.size() == 0);

    // Test at() method with bounds checking
    arr3.insert(3, "three");
    try {
        auto& val = arr3.at(0);
        assert(val.first == 1 || val.first == 2 || val.first == 3);
    } catch (...) {
        assert(false && "at() should not throw for valid index");
    }

    try {
        arr3.at(100); // Should throw
        assert(false && "at() should throw for invalid index");
    } catch (const std::out_of_range&) {
        // Expected
    }

    // Test const at()
    const auto& const_arr = arr3;
    try {
        const auto& val = const_arr.at(0);
        (void)val; // Use the value
    } catch (...) {
        assert(false && "const at() should not throw for valid index");
    }

    // Test pop_back()
    DynamicArray<int> int_arr;
    int_arr.push_back(10);
    int_arr.push_back(20);
    int_arr.pop_back();
    assert(int_arr.size() == 1);
    assert(int_arr[0] == 10);

    try {
        int_arr.pop_back();
        int_arr.pop_back(); // Should throw on empty
        assert(false && "pop_back() on empty should throw");
    } catch (const std::out_of_range&) {
        // Expected
    }

    // Test front(), back()
    int_arr.push_back(30);
    int_arr.push_back(40);
    assert(int_arr.front() == 30);
    assert(int_arr.back() == 40);

    const auto& const_int_arr = int_arr;
    assert(const_int_arr.front() == 30);
    assert(const_int_arr.back() == 40);

    // Test data()
    auto* ptr = int_arr.data();
    assert(ptr != nullptr);
    const auto* const_ptr = const_int_arr.data();
    assert(const_ptr != nullptr);

    // Test resize()
    int_arr.resize(10);
    assert(int_arr.size() == 10);

    int_arr.resize(5, 99);
    assert(int_arr.size() == 5);

    // Test shrink_to_fit()
    int_arr.reserve(1000);
    size_t large_cap = int_arr.capacity();
    int_arr.shrink_to_fit();
    assert(int_arr.capacity() <= large_cap);

    // Test reverse iterators
    DynamicArray<int> rev_arr;
    for (int i = 1; i <= 5; ++i)
        rev_arr.push_back(i);

    int expected = 5;
    for (auto it = rev_arr.rbegin(); it != rev_arr.rend(); ++it) {
        assert(*it == expected--);
    }

    expected = 5;
    for (auto it = rev_arr.crbegin(); it != rev_arr.crend(); ++it) {
        assert(*it == expected--);
    }

    const auto& const_rev = rev_arr;
    expected              = 5;
    for (auto it = const_rev.rbegin(); it != const_rev.rend(); ++it) {
        assert(*it == expected--);
    }

    // Test cend() and other const iterators
    auto cit = const_rev.cend();
    --cit;
    assert(*cit == 5);

    // Test iterator comparisons
    auto it1 = rev_arr.begin();
    auto it2 = rev_arr.begin();
    assert(it1 == it2);
    ++it2;
    assert(it1 != it2);
    assert(it1 < it2);
    assert(it1 <= it2);
    assert(it2 > it1);
    assert(it2 >= it1);

    // Test iterator arithmetic
    auto it3 = it1 + 2;
    assert(*it3 == 3);
    auto it4 = it3 - 1;
    assert(*it4 == 2);

    auto diff = it3 - it1;
    assert(diff == 2);

    it3 += 1;
    assert(*it3 == 4);
    it3 -= 2;
    assert(*it3 == 2);

    // Test operator[]
    assert(rev_arr[2] == 3);
    const auto& c_rev = rev_arr;
    assert(c_rev[2] == 3);

    // Test iterator operator[]
    auto it_base = rev_arr.begin();
    assert(it_base[2] == 3);

    // Test iterator post-increment/decrement
    auto it5 = rev_arr.begin();
    auto it6 = it5++;
    assert(*it6 == 1);
    assert(*it5 == 2);

    it5 = rev_arr.end();
    --it5;
    it6 = it5--;
    assert(*it6 == 5);
    assert(*it5 == 4);

    // Test swap
    DynamicArray<int> swap1, swap2;
    swap1.push_back(1);
    swap2.push_back(2);
    swap1.swap(swap2);
    assert(swap1[0] == 2);
    assert(swap2[0] == 1);

    // Test growth_strategy() and set_growth_strategy()
    DynamicArray<int> strat_arr(GrowthStrategy::MULTIPLICATIVE_2_0);
    assert(strat_arr.growth_strategy() == GrowthStrategy::MULTIPLICATIVE_2_0);
    strat_arr.set_growth_strategy(GrowthStrategy::FIBONACCI);
    assert(strat_arr.growth_strategy() == GrowthStrategy::FIBONACCI);

    // Test comparison operators
    DynamicArray<int> cmp1, cmp2;
    cmp1.push_back(1);
    cmp1.push_back(2);
    cmp2.push_back(1);
    cmp2.push_back(2);
    assert(cmp1 == cmp2);
    assert(!(cmp1 != cmp2));

    cmp2.push_back(3);
    assert(cmp1 != cmp2);

    // Test initializer list constructor
    DynamicArray<int> init_arr({1, 2, 3, 4, 5});
    assert(init_arr.size() == 5);
    assert(init_arr[2] == 3);

    // Test constructor with initial capacity
    DynamicArray<int> cap_arr(50, GrowthStrategy::ADDITIVE);
    assert(cap_arr.capacity() >= 50);
    assert(cap_arr.size() == 0);

    std::cout << "✓ Additional functions tested" << std::endl;
}

void test_comprehensive() {
    std::cout << "Testing comprehensive DynamicArray functionality..." << std::endl;

    // Test all growth strategies
    for (auto strategy : {GrowthStrategy::MULTIPLICATIVE_2_0, GrowthStrategy::MULTIPLICATIVE_1_5,
                          GrowthStrategy::FIBONACCI, GrowthStrategy::ADDITIVE}) {
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
        size_t                                 original_size = arr.size();
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

int run_dynamic_array_tests() {
    std::cout << "DynamicArray Test Suite\n";
    std::cout << "======================\n\n";

    // Reset memory tracker for clean testing
    auto& tracker = MemoryTracker::instance();
    tracker.set_detailed_tracking(true); // Enable detailed tracking to debug
    tracker.reset();

    try {
        test_basic();
        test_growth_strategies();
        test_iterators();
        test_memory_management();
        test_datastructure_interface();
        test_comprehensive();
        test_additional_functions();

        std::cout << "\n✅ All tests passed!\n";

        // Check for memory leaks with detailed reporting
        auto final_stats = tracker.get_stats();
        if (final_stats.memory_leaked() > 0) {
            std::cout << "\n⚠️  Warning: Memory leaks detected!\n";
            tracker.check_leaks();
            tracker.set_detailed_tracking(false);
            return 1;
        }

        tracker.set_detailed_tracking(false);
        return 0;
    } catch (const std::exception& e) {
        std::cout << "\n❌ Test failed with exception: " << e.what() << "\n";
        tracker.set_detailed_tracking(false);
        return 1;
    }
}