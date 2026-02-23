#include "cli_args.h"

#include <algorithm>
#include <cctype>

namespace hashbrowns {
namespace cli {

CliArgs parse_args(int argc, char* argv[]) {
    CliArgs a;

    // --- Pre-scan for early-exit flags ---
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--no-banner")
            a.no_banner = true;
        if (arg == "--quiet") {
            a.quiet     = true;
            a.no_banner = true;
        }
        if (arg == "--version") {
            a.version_only = true;
            a.no_banner    = true;
        }
    }

    if (a.version_only)
        return a;

    // --- Full parse ---
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            a.show_help = true;
            a.demo_mode = false;
        } else if (arg == "--version") {
            a.demo_mode = false;
        } else if (arg == "--wizard" || arg == "-wizard") {
            a.wizard_mode = true;
            a.demo_mode   = false;
        } else if (arg == "--size" && i + 1 < argc) {
            a.opt_size  = static_cast<std::size_t>(std::stoull(argv[++i]));
            a.demo_mode = false;
        } else if (arg == "--runs" && i + 1 < argc) {
            a.opt_runs  = std::stoi(argv[++i]);
            a.demo_mode = false;
        } else if (arg == "--warmup" && i + 1 < argc) {
            a.opt_warmup = std::stoi(argv[++i]);
            a.demo_mode  = false;
        } else if (arg == "--bootstrap" && i + 1 < argc) {
            a.opt_bootstrap = std::stoi(argv[++i]);
            a.demo_mode     = false;
        } else if (arg == "--series-count" && i + 1 < argc) {
            a.opt_series_count = std::stoi(argv[++i]);
            a.demo_mode        = false;
        } else if (arg == "--series-out" && i + 1 < argc) {
            a.opt_series_out = std::string(argv[++i]);
            a.demo_mode      = false;
        } else if (arg == "--series-sizes" && i + 1 < argc) {
            std::string list  = argv[++i];
            std::size_t start = 0, pos = 0;
            while ((pos = list.find(',', start)) != std::string::npos) {
                auto tok = list.substr(start, pos - start);
                if (!tok.empty())
                    a.opt_series_sizes.push_back(static_cast<std::size_t>(std::stoull(tok)));
                start = pos + 1;
            }
            if (start < list.size())
                a.opt_series_sizes.push_back(static_cast<std::size_t>(std::stoull(list.substr(start))));
            a.demo_mode = false;
        } else if (arg == "--series-runs" && i + 1 < argc) {
            a.opt_series_runs = std::stoi(argv[++i]);
            a.demo_mode       = false;
        } else if (arg == "--structures" && i + 1 < argc) {
            std::string list  = argv[++i];
            std::size_t start = 0, pos = 0;
            while ((pos = list.find(',', start)) != std::string::npos) {
                a.opt_structures.push_back(list.substr(start, pos - start));
                start = pos + 1;
            }
            if (start < list.size())
                a.opt_structures.push_back(list.substr(start));
            a.demo_mode = false;
        } else if (arg == "--output" && i + 1 < argc) {
            a.opt_output = std::string(argv[++i]);
            a.demo_mode  = false;
        } else if (arg == "--memory-tracking") {
            a.opt_memory_tracking = true;
            a.demo_mode           = false;
        } else if (arg == "--crossover-analysis") {
            a.opt_crossover = true;
            a.demo_mode     = false;
        } else if (arg == "--max-size" && i + 1 < argc) {
            a.opt_max_size = static_cast<std::size_t>(std::stoull(argv[++i]));
            a.demo_mode    = false;
        } else if (arg == "--pattern" && i + 1 < argc) {
            std::string p = argv[++i];
            if (p == "sequential")
                a.opt_pattern = BenchmarkConfig::Pattern::SEQUENTIAL;
            else if (p == "random")
                a.opt_pattern = BenchmarkConfig::Pattern::RANDOM;
            else if (p == "mixed")
                a.opt_pattern = BenchmarkConfig::Pattern::MIXED;
            a.demo_mode = false;
        } else if (arg == "--seed" && i + 1 < argc) {
            a.opt_seed  = static_cast<unsigned long long>(std::stoull(argv[++i]));
            a.demo_mode = false;
        } else if (arg == "--pin-cpu") {
            a.opt_pin_cpu = true;
            if (i + 1 < argc) {
                std::string maybe = argv[i + 1];
                if (!maybe.empty() && std::all_of(maybe.begin(), maybe.end(), ::isdigit)) {
                    ++i;
                    a.opt_cpu_index = std::stoi(maybe);
                }
            }
            a.demo_mode = false;
        } else if (arg == "--no-turbo") {
            a.opt_no_turbo = true;
            a.demo_mode    = false;
        } else if (arg == "--max-seconds" && i + 1 < argc) {
            a.opt_max_seconds = std::stod(argv[++i]);
            a.demo_mode       = false;
        } else if (arg == "--out-format" && i + 1 < argc) {
            std::string f = argv[++i];
            a.opt_out_fmt = (f == "json") ? BenchmarkConfig::OutputFormat::JSON : BenchmarkConfig::OutputFormat::CSV;
            a.demo_mode   = false;
        } else if (arg == "--hash-strategy" && i + 1 < argc) {
            std::string s = argv[++i];
            if (s == "open")
                a.opt_hash_strategy = HashStrategy::OPEN_ADDRESSING;
            else if (s == "chain")
                a.opt_hash_strategy = HashStrategy::SEPARATE_CHAINING;
            a.demo_mode = false;
        } else if (arg == "--hash-capacity" && i + 1 < argc) {
            a.opt_hash_capacity = static_cast<std::size_t>(std::stoull(argv[++i]));
            a.demo_mode         = false;
        } else if (arg == "--hash-load" && i + 1 < argc) {
            a.opt_hash_load = std::stod(argv[++i]);
            a.demo_mode     = false;
        } else if (arg == "--baseline" && i + 1 < argc) {
            a.opt_baseline_path = std::string(argv[++i]);
            a.demo_mode         = false;
        } else if (arg == "--baseline-threshold" && i + 1 < argc) {
            a.opt_baseline_threshold = std::stod(argv[++i]);
            a.demo_mode              = false;
        } else if (arg == "--baseline-noise" && i + 1 < argc) {
            a.opt_baseline_noise = std::stod(argv[++i]);
            a.demo_mode          = false;
        } else if (arg == "--baseline-scope" && i + 1 < argc) {
            std::string s = argv[++i];
            if (s == "mean")
                a.opt_baseline_scope = BaselineConfig::MetricScope::MEAN;
            else if (s == "p95")
                a.opt_baseline_scope = BaselineConfig::MetricScope::P95;
            else if (s == "ci_high")
                a.opt_baseline_scope = BaselineConfig::MetricScope::CI_HIGH;
            else if (s == "any")
                a.opt_baseline_scope = BaselineConfig::MetricScope::ANY;
            a.demo_mode = false;
        } else if (arg == "--op-tests") {
            a.opt_op_tests = true;
            a.demo_mode    = false;
        } else if (arg == "--no-banner" || arg == "--quiet") {
            a.demo_mode = false;
        } else if (arg.rfind("--", 0) == 0) {
            a.demo_mode = false;
        }
    }

    return a;
}

} // namespace cli
} // namespace hashbrowns
