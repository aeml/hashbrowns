#include <iostream>

int run_dynamic_array_tests();
int run_linked_list_tests();
int run_hash_map_tests();

int main() {
    int failures = 0;
    std::cout << "\n=== Running All Unit Test Suites ===\n";

    failures += run_dynamic_array_tests();
    std::cout << "\n------------------------------------\n";
    failures += run_linked_list_tests();
    std::cout << "\n------------------------------------\n";
    failures += run_hash_map_tests();

    if (failures == 0) {
        std::cout << "\n✅ All unit test suites passed!\n";
    } else {
        std::cout << "\n❌ " << failures << " suite(s) reported failures.\n";
    }
    return failures == 0 ? 0 : 1;
}
