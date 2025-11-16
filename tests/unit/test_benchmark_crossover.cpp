#include "benchmark/benchmark_suite.h"
#include <iostream>
#include <fstream>

using namespace hashbrowns;

int run_benchmark_crossover_tests() {
    int failures = 0;
    std::cout << "BenchmarkSuite Crossover Test Suite\n";
    std::cout << "===================================\n\n";

    BenchmarkSuite suite;

    // Construct a tiny synthetic series with obvious crossover for "insert".
    BenchmarkSuite::Series series;
    // size, structure, insert_ms, search_ms, remove_ms
    // At small size, A slower than B; at large size, A faster.
    series.push_back({10, "A", 5.0, 1.0, 1.0});
    series.push_back({10, "B", 3.0, 1.0, 1.0});
    series.push_back({100, "A", 4.0, 1.0, 1.0});
    series.push_back({100, "B", 6.0, 1.0, 1.0});

    auto crossovers = suite.compute_crossovers(series);
    if (crossovers.empty()) {
        std::cout << "❌ compute_crossovers returned empty list for obvious pattern\n";
        ++failures;
    } else {
        bool found_insert = false;
        for (const auto& c : crossovers) {
            if (c.operation == "insert" && ((c.a == "A" && c.b == "B") || (c.a == "B" && c.b == "A"))) {
                found_insert = true;
                break;
            }
        }
        if (!found_insert) {
            std::cout << "❌ compute_crossovers did not report expected insert crossover between A and B\n";
            ++failures;
        }
    }

    // Test CSV writing and header
    {
        const char* path = "crossovers_test.csv";
        suite.write_crossover_csv(path, crossovers);
        std::ifstream in(path);
        if (!in.good()) {
            std::cout << "❌ crossovers_test.csv was not written\n";
            ++failures;
        } else {
            std::string header;
            std::getline(in, header);
            if (header.find("operation") == std::string::npos ||
                header.find("a") == std::string::npos ||
                header.find("b") == std::string::npos ||
                header.find("size_at_crossover") == std::string::npos) {
                std::cout << "❌ CSV header missing expected columns\n";
                ++failures;
            }
        }
    }

    if (failures == 0) {
        std::cout << "\n✅ BenchmarkSuite crossover tests passed!\n";
    } else {
        std::cout << "\n❌ " << failures << " BenchmarkSuite crossover test(s) failed.\n";
    }

    return failures == 0 ? 0 : 1;
}
