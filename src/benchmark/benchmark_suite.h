#ifndef HASHBROWNS_BENCHMARK_SUITE_H
#define HASHBROWNS_BENCHMARK_SUITE_H

#include <string>
#include <vector>
#include <optional>
#include <memory>

#include "core/data_structure.h"

namespace hashbrowns {

struct BenchmarkConfig {
    std::size_t size = 10000;
    int runs = 10;
    bool verbose = false;
    std::optional<std::string> csv_output;
    std::vector<std::string> structures; // e.g., {"array","slist","dlist","hashmap"}
};

struct BenchmarkResult {
    std::string structure;
    double insert_ms_mean{0.0};
    double search_ms_mean{0.0};
    double remove_ms_mean{0.0};
    double insert_ms_stddev{0.0};
    double search_ms_stddev{0.0};
    double remove_ms_stddev{0.0};
    std::size_t memory_bytes{0};
};

class BenchmarkSuite {
public:
    std::vector<BenchmarkResult> run(const BenchmarkConfig& config);
};

} // namespace hashbrowns

#endif // HASHBROWNS_BENCHMARK_SUITE_H