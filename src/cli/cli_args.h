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
    bool opt_memory_tracking_explicit{false};
    bool opt_crossover{false};
    bool opt_crossover_explicit{false};

    // Benchmark parameters
    std::size_t                opt_size{10000};
    bool                       opt_size_explicit{false};
    int                        opt_runs{10};
    bool                       opt_runs_explicit{false};
    int                        opt_warmup{0};
    bool                       opt_warmup_explicit{false};
    int                        opt_bootstrap{0};
    bool                       opt_bootstrap_explicit{false};
    int                        opt_series_count{0};
    bool                       opt_series_count_explicit{false};
    int                        opt_series_runs{-1}; ///< <0 means "use default"
    bool                       opt_series_runs_explicit{false};
    std::optional<std::string> opt_series_out;
    bool                       opt_series_out_explicit{false};
    std::vector<std::size_t>   opt_series_sizes;
    bool                       opt_series_sizes_explicit{false};
    std::vector<std::string>   opt_structures;
    bool                       opt_structures_explicit{false};
    std::optional<std::string> opt_output;
    bool                       opt_output_explicit{false};
    std::optional<std::string> opt_profile;
    bool                       opt_profile_valid{true};

    // Hardware-affinity / reproducibility
    bool opt_pin_cpu{false};
    bool opt_pin_cpu_explicit{false};
    int  opt_cpu_index{0};
    bool opt_cpu_index_explicit{false};
    bool opt_no_turbo{false};
    bool opt_no_turbo_explicit{false};

    // Data pattern / seed
    BenchmarkConfig::Pattern          opt_pattern{BenchmarkConfig::Pattern::SEQUENTIAL};
    bool                              opt_pattern_explicit{false};
    std::optional<unsigned long long> opt_seed;
    bool                              opt_seed_explicit{false};

    // Output format
    BenchmarkConfig::OutputFormat opt_out_fmt{BenchmarkConfig::OutputFormat::CSV};
    bool                          opt_out_format_explicit{false};

    // Crossover sweep
    std::size_t           opt_max_size{100000};
    bool                  opt_max_size_explicit{false};
    std::optional<double> opt_max_seconds;
    bool                  opt_max_seconds_explicit{false};

    // HashMap tuning
    HashStrategy               opt_hash_strategy{HashStrategy::OPEN_ADDRESSING};
    bool                       opt_hash_strategy_explicit{false};
    std::optional<std::size_t> opt_hash_capacity;
    bool                       opt_hash_capacity_explicit{false};
    std::optional<double>      opt_hash_load;
    bool                       opt_hash_load_explicit{false};

    // Baseline comparison
    std::optional<std::string>  opt_baseline_path;
    std::optional<std::string>  opt_baseline_report_json;
    double                      opt_baseline_threshold{20.0};
    double                      opt_baseline_noise{1.0};
    BaselineConfig::MetricScope opt_baseline_scope{BaselineConfig::MetricScope::MEAN};
    bool                        opt_baseline_strict_profile_intent{false};
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
