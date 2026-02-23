#ifndef HASHBROWNS_CLI_ARGS_H
#define HASHBROWNS_CLI_ARGS_H

#include "benchmark/benchmark_suite.h" // BenchmarkConfig, BaselineConfig, HashStrategy

#include <optional>
#include <string>
#include <vector>

namespace hashbrowns {
namespace cli {

/**
 * @brief All values parsed from the command-line arguments.
 *
 * Populated by parse_args().  None of the fields represent application
 * logic; they are plain data that main() and the command handlers act on.
 */
struct CliArgs {
    // Early-exit flags (checked before anything else)
    bool no_banner{false};
    bool quiet{false};
    bool version_only{false};

    // Primary mode flags
    bool show_help{false};
    bool demo_mode{true}; ///< True when no meaningful flag was provided
    bool wizard_mode{false};
    bool opt_op_tests{false};
    bool opt_memory_tracking{false};
    bool opt_crossover{false};

    // Benchmark parameters
    std::size_t                opt_size{10000};
    int                        opt_runs{10};
    int                        opt_warmup{0};
    int                        opt_bootstrap{0};
    int                        opt_series_count{0};
    int                        opt_series_runs{-1}; ///< <0 means "use default"
    std::optional<std::string> opt_series_out;
    std::vector<std::size_t>   opt_series_sizes;
    std::vector<std::string>   opt_structures;
    std::optional<std::string> opt_output;

    // Hardware-affinity / reproducibility
    bool opt_pin_cpu{false};
    int  opt_cpu_index{0};
    bool opt_no_turbo{false};

    // Data pattern / seed
    BenchmarkConfig::Pattern          opt_pattern{BenchmarkConfig::Pattern::SEQUENTIAL};
    std::optional<unsigned long long> opt_seed;

    // Output format
    BenchmarkConfig::OutputFormat opt_out_fmt{BenchmarkConfig::OutputFormat::CSV};

    // Crossover sweep
    std::size_t           opt_max_size{100000};
    std::optional<double> opt_max_seconds;

    // HashMap tuning
    HashStrategy               opt_hash_strategy{HashStrategy::OPEN_ADDRESSING};
    std::optional<std::size_t> opt_hash_capacity;
    std::optional<double>      opt_hash_load;

    // Baseline comparison
    std::optional<std::string>  opt_baseline_path;
    double                      opt_baseline_threshold{20.0};
    double                      opt_baseline_noise{1.0};
    BaselineConfig::MetricScope opt_baseline_scope{BaselineConfig::MetricScope::MEAN};
};

/**
 * @brief Parse command-line arguments into a CliArgs struct.
 *
 * Does not print anything and does not call exit().  The caller is
 * responsible for acting on special flags such as show_help and
 * version_only.
 */
CliArgs parse_args(int argc, char* argv[]);

} // namespace cli
} // namespace hashbrowns

#endif // HASHBROWNS_CLI_ARGS_H
