#include "benchmark/benchmark_suite.h"
#include "cli/cli_args.h"
#include "core/memory_manager.h"

#include <fstream>
#include <iostream>

using namespace hashbrowns;

int run_benchmark_crossover_tests() {
    int failures = 0;
    std::cout << "BenchmarkSuite Crossover Test Suite\n";
    std::cout << "===================================\n\n";
    // Reset tracker so cross-suite allocations don't produce spurious log noise.
    {
        auto& tracker = MemoryTracker::instance();
        tracker.set_detailed_tracking(false);
        tracker.reset();
    }

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
            if (header.find("operation") == std::string::npos || header.find("a") == std::string::npos ||
                header.find("b") == std::string::npos || header.find("size_at_crossover") == std::string::npos) {
                std::cout << "❌ CSV header missing expected columns\n";
                ++failures;
            }
        }
    }

    // Test JSON crossover writing
    {
        const char*     path = "crossovers_test.json";
        BenchmarkConfig config;
        config.structures   = {"A", "B"};
        config.runs         = 3;
        config.pattern      = BenchmarkConfig::Pattern::SEQUENTIAL;
        config.profile_name = "crossover";

        suite.write_crossover_json(path, crossovers, config);
        std::ifstream in(path);
        if (!in.good()) {
            std::cout << "❌ crossovers_test.json was not written\n";
            ++failures;
        } else {
            std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            if (content.find("\"crossovers\"") == std::string::npos || content.find("\"meta\"") == std::string::npos) {
                std::cout << "❌ JSON missing expected structure\n";
                ++failures;
            } else if (content.find("\"profile\": \"crossover\"") == std::string::npos) {
                std::cout << "❌ Crossover JSON missing expected profile\n";
                ++failures;
            } else if (content.find("\"profile_manifest\"") == std::string::npos ||
                       content.find("\"selected_profile\": \"crossover\"") == std::string::npos) {
                std::cout << "❌ Crossover JSON missing profile manifest\n";
                ++failures;
            }
        }
    }

    // Test series CSV writing
    {
        const char* path = "series_test.csv";
        suite.write_series_csv(path, series);
        std::ifstream in(path);
        if (!in.good()) {
            std::cout << "❌ series_test.csv was not written\n";
            ++failures;
        } else {
            std::string header;
            std::getline(in, header);
            if (header.find("size") == std::string::npos || header.find("structure") == std::string::npos ||
                header.find("insert_ms") == std::string::npos) {
                std::cout << "❌ Series CSV header missing expected columns\n";
                ++failures;
            }
        }
    }

    // Test RANDOM pattern
    {
        std::cout << "\nTesting RANDOM pattern benchmark:\n";
        BenchmarkConfig config;
        config.structures  = {"array", "slist"};
        config.size        = 50;
        config.runs        = 2;
        config.warmup_runs = 1;
        config.pattern     = BenchmarkConfig::Pattern::RANDOM;
        config.seed        = 12345; // Fixed seed for reproducibility

        auto results = suite.run(config);
        if (results.empty()) {
            std::cout << "❌ RANDOM pattern benchmark returned no results\n";
            ++failures;
        } else {
            std::cout << "✅ RANDOM pattern benchmark completed with " << results.size() << " results\n";
        }
    }

    // Test MIXED pattern
    {
        std::cout << "\nTesting MIXED pattern benchmark:\n";
        BenchmarkConfig config;
        config.structures  = {"array"};
        config.size        = 30;
        config.runs        = 2;
        config.warmup_runs = 1;
        config.pattern     = BenchmarkConfig::Pattern::MIXED;
        config.seed        = 54321; // Fixed seed

        auto results = suite.run(config);
        if (results.empty()) {
            std::cout << "❌ MIXED pattern benchmark returned no results\n";
            ++failures;
        } else {
            std::cout << "✅ MIXED pattern benchmark completed with " << results.size() << " results\n";
        }
    }

    // Test CSV output format
    {
        std::cout << "\nTesting CSV output format:\n";
        BenchmarkConfig config;
        config.structures    = {"array"};
        config.size          = 20;
        config.runs          = 2;
        config.warmup_runs   = 0;
        config.pattern       = BenchmarkConfig::Pattern::SEQUENTIAL;
        config.output_format = BenchmarkConfig::OutputFormat::CSV;
        config.csv_output    = "test_results.csv";

        auto          results = suite.run(config);
        std::ifstream check("test_results.csv");
        if (!check.good()) {
            std::cout << "❌ CSV output file was not created\n";
            ++failures;
        } else {
            std::cout << "✅ CSV output file created successfully\n";
        }
    }

    // Test unknown structure handling
    {
        std::cout << "\nTesting unknown structure handling:\n";
        BenchmarkConfig config;
        config.structures = {"unknown_structure"};
        config.size       = 10;
        config.runs       = 1;

        auto results = suite.run(config);
        if (!results.empty()) {
            std::cout << "❌ Unknown structure should return empty results\n";
            ++failures;
        } else {
            std::cout << "✅ Unknown structure handled correctly\n";
        }
    }

    // Test doubly linked list structure
    {
        std::cout << "\nTesting doubly linked list structure:\n";
        BenchmarkConfig config;
        config.structures  = {"dlist"};
        config.size        = 20;
        config.runs        = 1;
        config.warmup_runs = 0;

        auto results = suite.run(config);
        if (results.empty()) {
            std::cout << "❌ Doubly linked list benchmark failed\n";
            ++failures;
        } else {
            std::cout << "✅ Doubly linked list benchmark completed\n";
        }
    }

    // Test hashmap with custom load factor and capacity
    {
        std::cout << "\nTesting hashmap with custom parameters:\n";
        BenchmarkConfig config;
        config.structures            = {"hashmap"};
        config.size                  = 30;
        config.runs                  = 2;
        config.warmup_runs           = 0;
        config.hash_strategy         = HashStrategy::OPEN_ADDRESSING;
        config.hash_initial_capacity = 64;
        config.hash_max_load_factor  = 0.7;

        auto results = suite.run(config);
        if (results.empty()) {
            std::cout << "❌ Hashmap with custom parameters failed\n";
            ++failures;
        } else {
            std::cout << "✅ Hashmap with custom parameters completed\n";
        }
    }

    // Test benchmark profiles via CLI parsing
    {
        std::cout << "\nTesting benchmark profile parsing:\n";
        const char* argv[] = {"hashbrowns", "--profile", "ci"};
        auto        parsed = hashbrowns::cli::parse_args(3, const_cast<char**>(argv));
        if (!parsed.opt_profile.has_value() || *parsed.opt_profile != "ci") {
            std::cout << "❌ CLI did not capture --profile ci\n";
            ++failures;
        } else {
            std::cout << "✅ CLI captured named benchmark profile\n";
        }
    }

    // Test baseline comparison functionality
    {
        std::cout << "\nTesting baseline comparison:\n";

        // Create baseline results
        std::vector<BenchmarkResult> baseline;
        BenchmarkResult              br1;
        br1.structure      = "array";
        br1.insert_ms_mean = 1.0;
        br1.search_ms_mean = 0.5;
        br1.remove_ms_mean = 0.8;
        br1.insert_ms_p95  = 1.2;
        br1.search_ms_p95  = 0.6;
        br1.remove_ms_p95  = 1.0;
        br1.insert_ci_high = 1.3;
        br1.search_ci_high = 0.7;
        br1.remove_ci_high = 1.1;
        br1.memory_bytes   = 1000;
        baseline.push_back(br1);

        // Create current results (slightly slower)
        std::vector<BenchmarkResult> current;
        BenchmarkResult              br2;
        br2.structure      = "array";
        br2.insert_ms_mean = 1.05; // 5% slower
        br2.search_ms_mean = 0.52; // 4% slower
        br2.remove_ms_mean = 0.85; // 6.25% slower
        br2.insert_ms_p95  = 1.26;
        br2.search_ms_p95  = 0.63;
        br2.remove_ms_p95  = 1.06;
        br2.insert_ci_high = 1.37;
        br2.search_ci_high = 0.74;
        br2.remove_ci_high = 1.16;
        br2.memory_bytes   = 1000;
        current.push_back(br2);

        BaselineConfig base_config;
        base_config.threshold_pct   = 10.0;
        base_config.noise_floor_pct = 2.0;
        base_config.scope           = BaselineConfig::MetricScope::MEAN;

        auto comparison = compare_against_baseline(baseline, current, base_config);
        if (comparison.entries.empty()) {
            std::cout << "❌ Baseline comparison returned no entries\n";
            ++failures;
        } else if (!comparison.all_ok) {
            std::cout << "❌ Baseline comparison failed when it should pass (within 10% threshold)\n";
            ++failures;
        } else {
            std::cout << "✅ Baseline comparison completed successfully\n";
        }

        BenchmarkMeta baseline_meta;
        baseline_meta.size            = 20000;
        baseline_meta.runs            = 5;
        baseline_meta.warmup_runs     = 1;
        baseline_meta.bootstrap_iters = 200;
        baseline_meta.structures      = {"array", "hashmap"};
        baseline_meta.pattern         = "random";
        baseline_meta.seed            = 12345ULL;
        baseline_meta.build_type      = "Release";
        baseline_meta.profile         = "ci";
        baseline_meta.hash_strategy   = "open";
        baseline_meta.hash_capacity   = 1024;
        baseline_meta.hash_load       = 0.7;
        baseline_meta.pinned_cpu      = 0;
        baseline_meta.turbo_disabled  = true;
        baseline_meta.cpu_model       = "Baseline CPU";
        baseline_meta.compiler        = "GCC 13";

        BenchmarkMeta current_meta = baseline_meta;
        auto          meta_ok      = compare_benchmark_metadata(baseline_meta, current_meta, base_config);
        if (!meta_ok.ok || !meta_ok.errors.empty() || !meta_ok.warnings.empty()) {
            std::cout << "❌ Matching metadata should not produce errors or warnings\n";
            ++failures;
        } else {
            std::cout << "✅ Matching baseline metadata accepted\n";
        }

        current_meta.pattern = "sequential";
        current_meta.profile = "custom";
        current_meta.seed    = std::nullopt;
        auto meta_bad        = compare_benchmark_metadata(baseline_meta, current_meta, base_config);
        if (meta_bad.ok) {
            std::cout << "❌ Metadata mismatch should fail guardrails\n";
            ++failures;
        }
        bool saw_pattern_mismatch = false;
        bool saw_seed_mismatch    = false;
        bool saw_profile_mismatch = false;
        for (const auto& msg : meta_bad.errors) {
            if (msg.find("pattern") != std::string::npos)
                saw_pattern_mismatch = true;
            if (msg.find("seed") != std::string::npos)
                saw_seed_mismatch = true;
            if (msg.find("profile") != std::string::npos)
                saw_profile_mismatch = true;
        }
        if (!saw_pattern_mismatch || !saw_seed_mismatch || !saw_profile_mismatch) {
            std::cout << "❌ Metadata mismatch errors should mention pattern, profile, and seed\n";
            ++failures;
        } else {
            std::cout << "✅ Metadata guardrails catch invalid workload changes\n";
        }

        current_meta = baseline_meta;
        current_meta.cpu_model = "Different CPU";
        current_meta.compiler  = "Clang 18";
        auto meta_warn         = compare_benchmark_metadata(baseline_meta, current_meta, base_config);
        if (!meta_warn.ok || meta_warn.warnings.size() < 2) {
            std::cout << "❌ Environment drift should warn without failing when workload matches\n";
            ++failures;
        } else {
            std::cout << "✅ Environment drift produces warnings without invalidating comparison\n";
        }

        // Test print helpers
        print_baseline_report(comparison, base_config.threshold_pct, base_config.noise_floor_pct);
        print_baseline_metadata_report(meta_bad);

        // Test with empty baseline
        auto empty_comparison = compare_against_baseline({}, current, base_config);
        if (!empty_comparison.entries.empty()) {
            std::cout << "❌ Empty baseline should return empty comparison\n";
            ++failures;
        }
    }

    // Test load_benchmark_results_json / load_benchmark_data_json
    {
        std::cout << "\nTesting load_benchmark_results_json:\n";

        // Create a minimal JSON file
        std::ofstream out("test_baseline.json");
        out << "{\n";
        out << "  \"meta\": {\n";
        out << "    \"schema_version\": 1,\n";
        out << "    \"size\": 4096,\n";
        out << "    \"runs\": 5,\n";
        out << "    \"warmup_runs\": 1,\n";
        out << "    \"bootstrap_iters\": 200,\n";
        out << "    \"structures\": [\"array\"],\n";
        out << "    \"pattern\": \"random\",\n";
        out << "    \"seed\": 12345,\n";
        out << "    \"timestamp\": \"2026-04-08T00:00:00Z\",\n";
        out << "    \"cpu_governor\": \"performance\",\n";
        out << "    \"git_commit\": \"abc123\",\n";
        out << "    \"compiler\": \"GCC 13\",\n";
        out << "    \"cpp_standard\": \"C++17\",\n";
        out << "    \"build_type\": \"Release\",\n";
        out << "    \"cpu_model\": \"Unit Test CPU\",\n";
        out << "    \"cores\": 8,\n";
        out << "    \"total_ram_bytes\": 17179869184,\n";
        out << "    \"kernel\": \"Linux 6.x\",\n";
        out << "    \"hash_strategy\": \"open\",\n";
        out << "    \"hash_capacity\": 1024,\n";
        out << "    \"hash_load\": 0.7,\n";
        out << "    \"pinned_cpu\": 0,\n";
        out << "    \"turbo_disabled\": 1\n";
        out << "  },\n";
        out << "  \"results\": [\n";
        out << "    {\n";
        out << "      \"structure\": \"array\",\n";
        out << "      \"insert_ms_mean\": 1.5,\n";
        out << "      \"search_ms_mean\": 0.8,\n";
        out << "      \"remove_ms_mean\": 1.2,\n";
        out << "      \"insert_ms_p95\": 1.8,\n";
        out << "      \"search_ms_p95\": 1.0,\n";
        out << "      \"remove_ms_p95\": 1.5,\n";
        out << "      \"insert_ci_high\": 2.0,\n";
        out << "      \"search_ci_high\": 1.1,\n";
        out << "      \"remove_ci_high\": 1.6,\n";
        out << "      \"memory_bytes\": 2000\n";
        out << "    }\n";
        out << "  ]\n";
        out << "}\n";
        out.close();

        auto loaded_data = load_benchmark_data_json("test_baseline.json");
        if (loaded_data.results.empty()) {
            std::cout << "❌ Failed to load benchmark results from JSON\n";
            ++failures;
        } else if (loaded_data.results[0].structure != "array") {
            std::cout << "❌ Loaded incorrect structure name\n";
            ++failures;
        } else if (loaded_data.results[0].insert_ms_mean != 1.5) {
            std::cout << "❌ Loaded incorrect insert_ms_mean\n";
            ++failures;
        } else if (loaded_data.meta.pattern != "random" || !loaded_data.meta.seed.has_value() ||
                   *loaded_data.meta.seed != 12345ULL || loaded_data.meta.hash_strategy != "open") {
            std::cout << "❌ Loaded incorrect benchmark metadata\n";
            ++failures;
        } else {
            std::cout << "✅ Successfully loaded benchmark results and metadata from JSON\n";
        }

        auto loaded = load_benchmark_results_json("test_baseline.json");
        if (loaded.size() != 1 || loaded[0].structure != "array") {
            std::cout << "❌ Backward-compatible result loader regressed\n";
            ++failures;
        }

        // Test loading non-existent file
        auto empty_load = load_benchmark_results_json("nonexistent.json");
        if (!empty_load.empty()) {
            std::cout << "❌ Loading non-existent file should return empty vector\n";
            ++failures;
        }
        auto empty_data = load_benchmark_data_json("nonexistent.json");
        if (!empty_data.results.empty()) {
            std::cout << "❌ Loading non-existent file should return empty benchmark data\n";
            ++failures;
        }
    }

    if (failures == 0) {
        std::cout << "\n✅ BenchmarkSuite crossover tests passed!\n";
    } else {
        std::cout << "\n❌ " << failures << " BenchmarkSuite crossover test(s) failed.\n";
    }

    return failures == 0 ? 0 : 1;
}
