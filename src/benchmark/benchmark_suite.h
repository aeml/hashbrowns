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
    std::string profile_name{"custom"}; // canonical named profile or "custom"
    std::vector<std::string> profile_applied_defaults;  // knobs supplied by the selected profile
    std::vector<std::string> profile_explicit_overrides; // knobs the caller overrode explicitly
};

// Configuration for comparing current benchmark results against a stored baseline.
struct BaselineConfig {
    std::string baseline_path;           // path to baseline benchmark_results.json
    double threshold_pct = 20.0;         // max allowed slowdown (%) per metric
    double noise_floor_pct = 1.0;        // ignore deltas within this band as noise
    // Which metrics to consider when evaluating regressions.
    enum class MetricScope { MEAN, P95, CI_HIGH, ANY };
    MetricScope scope = MetricScope::MEAN;
    bool strict_profile_intent{false};   // require matching profile manifest intent, not just matching coarse metadata
};

struct BaselineComparison {
    struct Entry {
        std::string structure;
        double insert_delta_pct{0.0};
        double search_delta_pct{0.0};
        double remove_delta_pct{0.0};
        bool insert_ok{true};
        bool search_ok{true};
        bool remove_ok{true};
        std::string insert_basis{"mean"};
        std::string search_basis{"mean"};
        std::string remove_basis{"mean"};
        double insert_mean_delta_pct{0.0};
        double insert_p95_delta_pct{0.0};
        double insert_ci_high_delta_pct{0.0};
        bool insert_mean_ok{true};
        bool insert_p95_ok{true};
        bool insert_ci_high_ok{true};
        double search_mean_delta_pct{0.0};
        double search_p95_delta_pct{0.0};
        double search_ci_high_delta_pct{0.0};
        bool search_mean_ok{true};
        bool search_p95_ok{true};
        bool search_ci_high_ok{true};
        double remove_mean_delta_pct{0.0};
        double remove_p95_delta_pct{0.0};
        double remove_ci_high_delta_pct{0.0};
        bool remove_mean_ok{true};
        bool remove_p95_ok{true};
        bool remove_ci_high_ok{true};
    };
    std::vector<Entry> entries;
    bool all_ok{true};
    std::string scope{"mean"};
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

struct BenchmarkMeta {
    int                               schema_version{0};
    std::size_t                       size{0};
    int                               runs{0};
    int                               warmup_runs{0};
    int                               bootstrap_iters{0};
    std::vector<std::string>          structures;
    std::string                       pattern{"unknown"};
    std::optional<unsigned long long> seed;
    std::string                       timestamp{"unknown"};
    std::string                       cpu_governor{"unknown"};
    std::string                       git_commit{"unknown"};
    std::string                       compiler{"unknown"};
    std::string                       cpp_standard{"unknown"};
    std::string                       build_type{"unknown"};
    std::string                       cpu_model{"unknown"};
    std::string                       profile{"custom"};
    std::string                       profile_selected{"custom"};
    std::vector<std::string>          profile_applied_defaults;
    std::vector<std::string>          profile_explicit_overrides;
    unsigned int                      cores{0};
    unsigned long long                total_ram_bytes{0};
    std::string                       kernel{"unknown"};
    std::string                       hash_strategy{"unknown"};
    std::optional<std::size_t>        hash_capacity;
    std::optional<double>             hash_load;
    int                               pinned_cpu{-1};
    bool                              turbo_disabled{false};
};

struct BenchmarkData {
    BenchmarkMeta                meta;
    std::vector<BenchmarkResult> results;
};

struct BaselineMetadataReport {
    bool                     ok{true};
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
};

struct BaselineReport {
    BaselineMetadataReport metadata;
    BaselineComparison     comparison;
    double                 threshold_pct{0.0};
    double                 noise_floor_pct{0.0};
    std::string            baseline_path;
    std::string            scope{"mean"};
    bool                   strict_profile_intent{false};
    int                    exit_code{0};
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

// Load a benchmark_results.json file and extract metadata plus per-structure results.
// Returns default-initialized BenchmarkData on error.
BenchmarkData load_benchmark_data_json(const std::string& path);

// Backward-compatible helper returning only per-structure results.
std::vector<BenchmarkResult> load_benchmark_results_json(const std::string& path);

// Compare baseline vs current benchmark metadata for reproducibility guardrails.
BaselineMetadataReport compare_benchmark_metadata(const BenchmarkMeta& baseline, const BenchmarkMeta& current,
                                                  const BaselineConfig& cfg);

// Compare current results against a baseline using the provided configuration.
BaselineComparison compare_against_baseline(const std::vector<BenchmarkResult>& baseline,
                                            const std::vector<BenchmarkResult>& current,
                                            const BaselineConfig& cfg);

// Pretty-print a comparison summary to stdout.
void print_baseline_report(const BaselineComparison& report, double threshold_pct, double noise_floor_pct);

// Pretty-print metadata compatibility findings to stdout.
void print_baseline_metadata_report(const BaselineMetadataReport& report);

// Write a machine-readable baseline comparison report to JSON.
void write_baseline_report_json(const std::string& path, const BaselineReport& report);

} // namespace hashbrowns

#endif // HASHBROWNS_BENCHMARK_SUITE_H