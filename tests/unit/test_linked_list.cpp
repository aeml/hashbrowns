#include "structures/linked_list.h"
#include "core/memory_manager.h"
#include <iostream>
#include <string>
#include <cassert>

using namespace hashbrowns;

static void test_singly_basic() {
    std::cout << "Testing SinglyLinkedList basic operations..." << std::endl;
    SinglyLinkedList<std::pair<int, std::string>> list;
    assert(list.size() == 0);
    assert(list.empty());

    list.insert(1, "one");
    list.insert(2, "two");
    list.insert(3, "three");

    assert(list.size() == 3);
    assert(!list.empty());

    std::string v;
    assert(list.search(1, v) && v == "one");
    assert(list.search(2, v) && v == "two");
    assert(list.search(3, v) && v == "three");
    assert(!list.search(4, v));

    std::cout << "✓ Singly basic passed" << std::endl;
}

static void test_singly_remove_edges() {
    std::cout << "Testing SinglyLinkedList remove edge cases..." << std::endl;
    SinglyLinkedList<std::pair<int, std::string>> list;
    list.insert(1, "one");
    list.insert(2, "two");
    list.insert(3, "three");
    list.insert(4, "four");

    std::string v;
    // remove head
    assert(list.remove(1));
    assert(!list.search(1, v));
    assert(list.size() == 3);

    // remove middle
    assert(list.remove(3));
    assert(!list.search(3, v));
    assert(list.size() == 2);

    // remove tail
    assert(list.remove(4));
    assert(!list.search(4, v));
    assert(list.size() == 1);

    // remove only remaining
    assert(list.remove(2));
    assert(list.size() == 0);
    assert(list.empty());

    // removing missing key returns false
    assert(!list.remove(42));

    // clear on empty is safe
    list.clear();
    assert(list.size() == 0);
    assert(list.empty());

    std::cout << "✓ Singly remove edges passed" << std::endl;
}

static void test_singly_copy_move() {
    std::cout << "Testing SinglyLinkedList copy/move..." << std::endl;
    SinglyLinkedList<std::pair<int, std::string>> a;
    a.insert(10, "ten");
    a.insert(20, "twenty");

    // copy
    SinglyLinkedList<std::pair<int, std::string>> b(a);
    assert(b.size() == a.size());
    std::string v;
    assert(b.search(10, v) && v == "ten");
    assert(b.search(20, v) && v == "twenty");

    // independence
    assert(a.remove(10));
    assert(!a.search(10, v));
    assert(b.search(10, v) && v == "ten");

    // move
    SinglyLinkedList<std::pair<int, std::string>> c(std::move(b));
    assert(c.size() == 2);
    assert(c.search(10, v) && v == "ten");
    assert(c.search(20, v) && v == "twenty");
    assert(b.size() == 0);
    assert(b.empty());

    std::cout << "✓ Singly copy/move passed" << std::endl;
}

static void test_doubly_basic() {
    std::cout << "Testing DoublyLinkedList basic operations..." << std::endl;
    DoublyLinkedList<std::pair<int, std::string>> list;
    assert(list.size() == 0);
    assert(list.empty());

    list.insert(1, "one");
    list.insert(2, "two");
    list.insert(3, "three");

    assert(list.size() == 3);
    assert(!list.empty());

    std::string v;
    assert(list.search(1, v) && v == "one");
    assert(list.search(2, v) && v == "two");
    assert(list.search(3, v) && v == "three");
    assert(!list.search(4, v));

    std::cout << "✓ Doubly basic passed" << std::endl;
}

static void test_doubly_remove_edges() {
    std::cout << "Testing DoublyLinkedList remove edge cases..." << std::endl;
    DoublyLinkedList<std::pair<int, std::string>> list;
    list.insert(1, "one");
    list.insert(2, "two");
    list.insert(3, "three");
    list.insert(4, "four");

    std::string v;
    // remove head
    assert(list.remove(1));
    assert(!list.search(1, v));
    assert(list.size() == 3);

    // remove middle
    assert(list.remove(3));
    assert(!list.search(3, v));
    assert(list.size() == 2);

    // remove tail
    assert(list.remove(4));
    assert(!list.search(4, v));
    assert(list.size() == 1);

    // remove only remaining
    assert(list.remove(2));
    assert(list.size() == 0);
    assert(list.empty());

    // removing missing key returns false
    assert(!list.remove(42));

    // clear on empty is safe
    list.clear();
    assert(list.size() == 0);
    assert(list.empty());

    std::cout << "✓ Doubly remove edges passed" << std::endl;
}

static void test_doubly_copy_move() {
    std::cout << "Testing DoublyLinkedList copy/move..." << std::endl;
    DoublyLinkedList<std::pair<int, std::string>> a;
    a.insert(10, "ten");
    a.insert(20, "twenty");

    // copy
    DoublyLinkedList<std::pair<int, std::string>> b(a);
    assert(b.size() == a.size());
    std::string v;
    assert(b.search(10, v) && v == "ten");
    assert(b.search(20, v) && v == "twenty");

    // independence
    assert(a.remove(10));
    assert(!a.search(10, v));
    assert(b.search(10, v) && v == "ten");

    // move
    DoublyLinkedList<std::pair<int, std::string>> c(std::move(b));
    assert(c.size() == 2);
    assert(c.search(10, v) && v == "ten");
    assert(c.search(20, v) && v == "twenty");
    assert(b.size() == 0);
    assert(b.empty());

    std::cout << "✓ Doubly copy/move passed" << std::endl;
}

static void test_additional_coverage() {
    std::cout << "Testing additional functions for coverage..." << std::endl;
    
    // Test SinglyLinkedList DataStructure interface methods
    SinglyLinkedList<std::pair<int, std::string>> slist;
    std::string type_n = slist.type_name();
    std::string insert_c = slist.insert_complexity();
    std::string search_c = slist.search_complexity();
    std::string remove_c = slist.remove_complexity();
    assert(!type_n.empty());
    assert(!insert_c.empty());
    assert(!search_c.empty());
    assert(!remove_c.empty());
    
    // Test copy assignment
    slist.insert(1, "one");
    slist.insert(2, "two");
    SinglyLinkedList<std::pair<int, std::string>> slist2;
    slist2 = slist;
    assert(slist2.size() == 2);
    std::string v;
    assert(slist2.search(1, v) && v == "one");
    
    // Test move assignment
    SinglyLinkedList<std::pair<int, std::string>> slist3;
    slist3 = std::move(slist2);
    assert(slist3.size() == 2);
    assert(slist2.size() == 0);
    
    // Test DoublyLinkedList DataStructure interface methods
    DoublyLinkedList<std::pair<int, std::string>> dlist;
    type_n = dlist.type_name();
    insert_c = dlist.insert_complexity();
    search_c = dlist.search_complexity();
    remove_c = dlist.remove_complexity();
    assert(!type_n.empty());
    assert(!insert_c.empty());
    assert(!search_c.empty());
    assert(!remove_c.empty());
    
    // Test copy assignment
    dlist.insert(1, "one");
    dlist.insert(2, "two");
    DoublyLinkedList<std::pair<int, std::string>> dlist2;
    dlist2 = dlist;
    assert(dlist2.size() == 2);
    assert(dlist2.search(1, v) && v == "one");
    
    // Test move assignment
    DoublyLinkedList<std::pair<int, std::string>> dlist3;
    dlist3 = std::move(dlist2);
    assert(dlist3.size() == 2);
    assert(dlist2.size() == 0);
    
    std::cout << "✓ Additional coverage passed" << std::endl;
}

int run_linked_list_tests() {
    std::cout << "LinkedList Test Suite\n";
    std::cout << "=====================\n\n";

    auto& tracker = MemoryTracker::instance();
    tracker.set_detailed_tracking(true);
    tracker.reset();

    try {
        test_singly_basic();
        test_singly_remove_edges();
        test_singly_copy_move();

        test_doubly_basic();
        test_doubly_remove_edges();
        test_doubly_copy_move();
        test_additional_coverage();

        std::cout << "\n✅ All LinkedList tests passed!\n";

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
