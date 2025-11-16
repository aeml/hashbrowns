#include "structures/hash_map.h"
#include "core/memory_manager.h"
#include <cassert>
#include <iostream>
#include <string>

using namespace hashbrowns;

static void test_basic_open_addressing() {
    std::cout << "Testing HashMap (open addressing) basic ops..." << std::endl;
    HashMap map(HashStrategy::OPEN_ADDRESSING, 16);

    assert(map.size() == 0);
    assert(map.empty());

    map.insert(1, "one");
    map.insert(2, "two");
    map.insert(3, "three");
    assert(map.size() == 3);
    assert(!map.empty());

    std::string v;
    assert(map.search(1, v) && v == "one");
    assert(map.search(2, v) && v == "two");
    assert(map.search(3, v) && v == "three");
    assert(!map.search(4, v));

    // update
    map.insert(2, "two2");
    assert(map.search(2, v) && v == "two2");

    // remove
    assert(map.remove(2));
    assert(!map.search(2, v));
    assert(map.size() == 2);
    assert(!map.remove(42));

    std::cout << "✓ OA basic ops passed" << std::endl;
}

static void test_open_addressing_growth_and_tombstones() {
    std::cout << "Testing HashMap (open addressing) growth & tombstones..." << std::endl;
    HashMap map(HashStrategy::OPEN_ADDRESSING, 8);

    // Insert enough items to trigger growth (> 70% of capacity) and exercise probing
    for (int i = 0; i < 100; ++i) {
        map.insert(i, std::to_string(i));
    }
    std::string v;
    for (int i = 0; i < 100; ++i) {
        assert(map.search(i, v) && v == std::to_string(i));
    }

    // Create tombstones
    for (int i = 0; i < 50; ++i) {
        assert(map.remove(i));
    }
    for (int i = 0; i < 50; ++i) {
        assert(!map.search(i, v));
    }
    for (int i = 50; i < 100; ++i) {
        assert(map.search(i, v));
    }
    std::cout << "✓ OA growth & tombstones passed" << std::endl;
}

static void test_basic_separate_chaining() {
    std::cout << "Testing HashMap (separate chaining) basic ops..." << std::endl;
    HashMap map(HashStrategy::SEPARATE_CHAINING, 8);

    assert(map.size() == 0);
    assert(map.empty());

    map.insert(10, "ten");
    map.insert(20, "twenty");
    map.insert(30, "thirty");
    assert(map.size() == 3);
    assert(!map.empty());

    std::string v;
    assert(map.search(10, v) && v == "ten");
    assert(map.search(20, v) && v == "twenty");
    assert(map.search(30, v) && v == "thirty");
    assert(!map.search(40, v));

    // update
    map.insert(20, "twenty2");
    assert(map.search(20, v) && v == "twenty2");

    // remove
    assert(map.remove(20));
    assert(!map.search(20, v));
    assert(map.size() == 2);
    assert(!map.remove(42));

    std::cout << "✓ SC basic ops passed" << std::endl;
}

static void test_separate_chaining_growth_and_rehash() {
    std::cout << "Testing HashMap (separate chaining) growth & rehash..." << std::endl;
    HashMap map(HashStrategy::SEPARATE_CHAINING, 8);
    for (int i = 0; i < 100; ++i) {
        map.insert(i, std::to_string(i));
    }
    std::string v;
    for (int i = 0; i < 100; ++i) {
        assert(map.search(i, v) && v == std::to_string(i));
    }
    for (int i = 0; i < 50; ++i) {
        assert(map.remove(i));
    }
    for (int i = 0; i < 50; ++i) {
        assert(!map.search(i, v));
    }
    for (int i = 50; i < 100; ++i) {
        assert(map.search(i, v));
    }
    std::cout << "✓ SC growth & rehash passed" << std::endl;
}

static void test_additional_coverage() {
    std::cout << "Testing additional HashMap functions for coverage..." << std::endl;
    
    // Test DataStructure interface methods for OPEN_ADDRESSING
    HashMap map_oa(HashStrategy::OPEN_ADDRESSING, 16);
    std::string type_n = map_oa.type_name();
    std::string insert_c = map_oa.insert_complexity();
    std::string search_c = map_oa.search_complexity();
    std::string remove_c = map_oa.remove_complexity();
    assert(!type_n.empty());
    assert(!insert_c.empty());
    assert(!search_c.empty());
    assert(!remove_c.empty());
    
    // Test strategy() getter
    assert(map_oa.strategy() == HashStrategy::OPEN_ADDRESSING);
    
    // Test max_load_factor() getter
    float load_factor_oa = map_oa.max_load_factor();
    assert(load_factor_oa > 0.0f);
    
    // Test DataStructure interface methods for SEPARATE_CHAINING
    HashMap map_sc(HashStrategy::SEPARATE_CHAINING, 16);
    type_n = map_sc.type_name();
    insert_c = map_sc.insert_complexity();
    search_c = map_sc.search_complexity();
    remove_c = map_sc.remove_complexity();
    assert(!type_n.empty());
    assert(!insert_c.empty());
    assert(!search_c.empty());
    assert(!remove_c.empty());
    
    // Test strategy() getter
    assert(map_sc.strategy() == HashStrategy::SEPARATE_CHAINING);
    
    // Test max_load_factor() getter
    float load_factor_sc = map_sc.max_load_factor();
    assert(load_factor_sc > 0.0f);
    
    // Test set_strategy() on empty map
    HashMap map_empty(HashStrategy::OPEN_ADDRESSING, 8);
    map_empty.set_strategy(HashStrategy::SEPARATE_CHAINING);
    assert(map_empty.strategy() == HashStrategy::SEPARATE_CHAINING);
    
    // Test set_max_load_factor() for separate chaining
    HashMap map_sc2(HashStrategy::SEPARATE_CHAINING, 8);
    map_sc2.set_max_load_factor(2.0f);
    float new_load = map_sc2.max_load_factor();
    assert(new_load == 2.0f);
    
    // Test memory_usage() for separate chaining
    HashMap map_mem(HashStrategy::SEPARATE_CHAINING, 8);
    map_mem.insert(1, "one");
    map_mem.insert(2, "two");
    size_t mem_usage = map_mem.memory_usage();
    assert(mem_usage > 0);
    
    std::cout << "✓ Additional coverage passed" << std::endl;
}

int run_hash_map_tests() {
    std::cout << "HashMap Test Suite\n";
    std::cout << "==================\n\n";

    auto& tracker = MemoryTracker::instance();
    tracker.set_detailed_tracking(true);
    tracker.reset();

    try {
        test_basic_open_addressing();
        test_open_addressing_growth_and_tombstones();
        test_basic_separate_chaining();
        test_separate_chaining_growth_and_rehash();
        test_additional_coverage();

        std::cout << "\n✅ All HashMap tests passed!\n";
        auto stats = tracker.get_stats();
        if (stats.memory_leaked() > 0) {
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
