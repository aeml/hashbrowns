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
    int warmup_runs = 0; // number of warm-up runs discarded
    int bootstrap_iters = 0; // bootstrap iterations for CI (0=disabled)
    bool verbose = false;
    std::optional<std::string> csv_output;
    enum class OutputFormat { CSV, JSON };
    OutputFormat output_format = OutputFormat::CSV;
    std::vector<std::string> structures; // e.g., {"array","slist","dlist","hashmap"}
    // Data pattern configuration
    enum class Pattern { SEQUENTIAL, RANDOM, MIXED };
    Pattern pattern = Pattern::SEQUENTIAL;
    std::optional<unsigned long long> seed; // RNG seed for RANDOM/MIXED; random_device if not provided
    bool seed_was_generated = false; // set true if we generated a seed internally

    // HashMap tuning
    HashStrategy hash_strategy = HashStrategy::OPEN_ADDRESSING;
    std::optional<std::size_t> hash_initial_capacity; // default 16
    std::optional<double> hash_max_load_factor;       // applies to both strategies if provided

    // Repro/affinity flags
    bool pin_cpu = false;            // if true attempt to pin
    int pin_cpu_index = 0;           // CPU index to pin to (default 0)
    bool disable_turbo = false;      // attempt to disable turbo boost (Linux-only, may require privileges)
};

struct BenchmarkResult {
    std::string structure;
    double insert_ms_mean{0.0};
    double search_ms_mean{0.0};
    double remove_ms_mean{0.0};
    double insert_ms_stddev{0.0};
    double search_ms_stddev{0.0};
    double remove_ms_stddev{0.0};
    double insert_ms_median{0.0};
    double search_ms_median{0.0};
    double remove_ms_median{0.0};
    double insert_ms_p95{0.0};
    double search_ms_p95{0.0};
    double remove_ms_p95{0.0};
    double insert_ci_low{0.0};
    double insert_ci_high{0.0};
    double search_ci_low{0.0};
    double search_ci_high{0.0};
    double remove_ci_low{0.0};
    double remove_ci_high{0.0};
    std::size_t memory_bytes{0};
    double memory_insert_bytes_mean{0.0};
    double memory_insert_bytes_stddev{0.0};
    double memory_search_bytes_mean{0.0};
    double memory_search_bytes_stddev{0.0};
    double memory_remove_bytes_mean{0.0};
    double memory_remove_bytes_stddev{0.0};
    // HashMap-specific metrics (probes per operation average across runs)
    double insert_probes_mean{0.0};
    double insert_probes_stddev{0.0};
    double search_probes_mean{0.0};
    double search_probes_stddev{0.0};
    double remove_probes_mean{0.0};
    double remove_probes_stddev{0.0};
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
    void write_crossover_json(const std::string& path, const std::vector<CrossoverInfo>& info, const BenchmarkConfig& config);

    // Linear series output helpers (size/structure -> per-op means)
    void write_series_csv(const std::string& path, const Series& series);
    void write_series_json(const std::string& path, const Series& series, const BenchmarkConfig& config);
};

} // namespace hashbrowns

#endif // HASHBROWNS_BENCHMARK_SUITE_H