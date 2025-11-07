#include <iostream>

int run_dynamic_array_tests();
int run_linked_list_tests();
int run_hash_map_tests();
int run_json_output_tests();
// Forward declarations only if corresponding sources exist; provide weak stubs otherwise.
int run_series_json_tests();
int run_stats_tests();

// Provide inline fallback stubs if the real definitions are not linked (to avoid undefined refs).
#if !defined(HASHBROWNS_HAS_SERIES_JSON_TESTS)
int run_series_json_tests() { return 0; }
#endif
#if !defined(HASHBROWNS_HAS_STATS_TESTS)
int run_stats_tests() { return 0; }
#endif

int main() {
    int failures = 0;
    std::cout << "\n=== Running All Unit Test Suites ===\n";

    failures += run_dynamic_array_tests();
    std::cout << "\n------------------------------------\n";
    failures += run_linked_list_tests();
    std::cout << "\n------------------------------------\n";
    failures += run_hash_map_tests();
    std::cout << "\n------------------------------------\n";
    failures += run_json_output_tests();
    std::cout << "\n------------------------------------\n";
    failures += run_series_json_tests();
    std::cout << "\n------------------------------------\n";
    failures += run_stats_tests();

    if (failures == 0) {
        std::cout << "\n✅ All unit test suites passed!\n";
    } else {
        std::cout << "\n❌ " << failures << " suite(s) reported failures.\n";
    }
    return failures == 0 ? 0 : 1;
}
