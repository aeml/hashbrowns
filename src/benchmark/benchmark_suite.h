#ifndef HASHBROWNS_BENCHMARK_SUITE_H
#define HASHBROWNS_BENCHMARK_SUITE_H

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include "structures/hash_map.h"

#include "core/data_structure.h"

namespace hashbrowns {

struct BenchmarkConfig {
    std::size_t size = 10000;
    int runs = 10;
    bool verbose = false;
    std::optional<std::string> csv_output;
    enum class OutputFormat { CSV, JSON };
    OutputFormat output_format = OutputFormat::CSV;
    std::vector<std::string> structures; // e.g., {"array","slist","dlist","hashmap"}
    // Data pattern configuration
    enum class Pattern { SEQUENTIAL, RANDOM, MIXED };
    Pattern pattern = Pattern::SEQUENTIAL;
    std::optional<unsigned long long> seed; // RNG seed for RANDOM/MIXED; random_device if not provided

    // HashMap tuning
    HashStrategy hash_strategy = HashStrategy::OPEN_ADDRESSING;
    std::optional<std::size_t> hash_initial_capacity; // default 16
    std::optional<double> hash_max_load_factor;       // applies to both strategies if provided
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

    struct SeriesPoint { std::size_t size; std::string structure; double insert_ms; double search_ms; double remove_ms; };
    using Series = std::vector<SeriesPoint>;
    Series run_series(const BenchmarkConfig& baseConfig, const std::vector<std::size_t>& sizes);

    struct CrossoverInfo { std::string operation; std::string a; std::string b; std::size_t size_at_crossover; };
    std::vector<CrossoverInfo> compute_crossovers(const Series& series);
    void write_crossover_csv(const std::string& path, const std::vector<CrossoverInfo>& info);
    void write_crossover_json(const std::string& path, const std::vector<CrossoverInfo>& info);
};

} // namespace hashbrowns

#endif // HASHBROWNS_BENCHMARK_SUITE_H