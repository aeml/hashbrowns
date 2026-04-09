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

    // Test explicit flag tracking even when the user passes parser-default values.
    {
        std::cout << "\nTesting explicit benchmark flag tracking:\n";
        const char* argv[] = {"hashbrowns", "--profile", "ci", "--size", "10000", "--runs", "10", "--out-format", "json"};
        auto        parsed = hashbrowns::cli::parse_args(9, const_cast<char**>(argv));
        if (!parsed.opt_size_explicit || !parsed.opt_runs_explicit || !parsed.opt_out_format_explicit) {
            std::cout << "❌ CLI lost explicit flags when values matched parser defaults\n";
            ++failures;
        } else {
            std::cout << "✅ CLI preserves explicit flags even when values match parser defaults\n";
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
        base_config.threshold_pct          = 10.0;
        base_config.noise_floor_pct        = 2.0;
        base_config.scope                  = BaselineConfig::MetricScope::MEAN;
        base_config.strict_profile_intent  = true;

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
        if (comparison.scope != "mean") {
            std::cout << "❌ Baseline comparison should record selected scope\n";
            ++failures;
        }
        if (comparison.health != "clean") {
            std::cout << "❌ Fully comparable non-duplicated baseline comparison should be classified as clean\n";
            ++failures;
        }
        if (comparison.actionability != "fully_actionable") {
            std::cout << "❌ Clean comparison should be fully actionable\n";
            ++failures;
        }
        if (comparison.entries.front().insert_basis != "mean" ||
            comparison.entries.front().search_basis != "mean" ||
            comparison.entries.front().remove_basis != "mean") {
            std::cout << "❌ Baseline comparison should record metric basis for each operation\n";
            ++failures;
        }

        BaselineConfig any_scope_config = base_config;
        any_scope_config.scope = BaselineConfig::MetricScope::ANY;
        auto any_scope_comparison = compare_against_baseline(baseline, current, any_scope_config);
        if (any_scope_comparison.scope != "any") {
            std::cout << "❌ Baseline comparison should record any scope\n";
            ++failures;
        }
        if (any_scope_comparison.entries.empty()) {
            std::cout << "❌ Any-scope baseline comparison returned no entries\n";
            ++failures;
        } else {
            const auto& any_entry = any_scope_comparison.entries.front();
            if (any_entry.insert_basis.find("any(") == std::string::npos ||
                any_entry.search_basis.find("any(") == std::string::npos ||
                any_entry.remove_basis.find("any(") == std::string::npos) {
                std::cout << "❌ Any-scope comparison should explain pass/fail basis per operation\n";
                ++failures;
            }
            if (any_entry.insert_mean_delta_pct == 0.0 || any_entry.insert_p95_delta_pct == 0.0 || any_entry.insert_ci_high_delta_pct == 0.0 ||
                any_entry.search_mean_delta_pct == 0.0 || any_entry.search_p95_delta_pct == 0.0 || any_entry.search_ci_high_delta_pct == 0.0 ||
                any_entry.remove_mean_delta_pct == 0.0 || any_entry.remove_p95_delta_pct == 0.0 || any_entry.remove_ci_high_delta_pct == 0.0) {
                std::cout << "❌ Any-scope comparison should expose exact per-metric deltas\n";
                ++failures;
            }
        }

        auto current_any_fail = current;
        current_any_fail.front().insert_ms_mean = 1.30;
        current_any_fail.front().insert_ms_p95 = 1.36;
        current_any_fail.front().insert_ci_high = 1.45;
        auto any_scope_failure_comparison = compare_against_baseline(baseline, current_any_fail, any_scope_config);
        if (any_scope_failure_comparison.failures.empty()) {
            std::cout << "❌ Any-scope comparison should summarize failing coarse checks\n";
            ++failures;
        } else {
            const auto& failure = any_scope_failure_comparison.failures.front();
            if (failure.structure.empty() || failure.operation != "insert" || failure.chosen_basis.empty() ||
                failure.failed_metric_families.empty()) {
                std::cout << "❌ Coarse failure summary should capture structure, operation, chosen basis, and failed metric families\n";
                ++failures;
            }
        }

        auto coverage_baseline = baseline;
        auto coverage_current  = current;
        BenchmarkResult coverage_baseline_only;
        coverage_baseline_only.structure      = "hashmap";
        coverage_baseline_only.insert_ms_mean = 0.9;
        coverage_baseline_only.search_ms_mean = 0.4;
        coverage_baseline_only.remove_ms_mean = 0.7;
        coverage_baseline_only.insert_ms_p95  = 1.0;
        coverage_baseline_only.search_ms_p95  = 0.5;
        coverage_baseline_only.remove_ms_p95  = 0.8;
        coverage_baseline_only.insert_ci_high = 1.1;
        coverage_baseline_only.search_ci_high = 0.6;
        coverage_baseline_only.remove_ci_high = 0.9;
        coverage_baseline.push_back(coverage_baseline_only);
        BenchmarkResult coverage_current_only = coverage_baseline_only;
        coverage_current_only.structure = "linked_list";
        coverage_current.push_back(coverage_current_only);
        auto coverage_comparison = compare_against_baseline(coverage_baseline, coverage_current, base_config);
        if (coverage_comparison.coverage.baseline_structure_count != 2 ||
            coverage_comparison.coverage.current_structure_count != 2 ||
            coverage_comparison.coverage.comparable_structure_count != 1 ||
            coverage_comparison.coverage.baseline_only_structures.size() != 1 ||
            coverage_comparison.coverage.current_only_structures.size() != 1) {
            std::cout << "❌ Baseline comparison should report comparable and missing structures honestly\n";
            ++failures;
        }
        if (coverage_comparison.health != "partial_coverage") {
            std::cout << "❌ Missing structures should classify comparison health as partial_coverage\n";
            ++failures;
        }
        if (coverage_comparison.actionability != "actionable_with_hygiene_warnings") {
            std::cout << "❌ Partial coverage should remain actionable but with hygiene warnings\n";
            ++failures;
        }

        auto duplicate_baseline = baseline;
        duplicate_baseline.push_back(br1);
        auto duplicate_current = current;
        duplicate_current.push_back(br2);
        auto duplicate_comparison = compare_against_baseline(duplicate_baseline, duplicate_current, base_config);
        if (duplicate_comparison.coverage.duplicate_baseline_structures.size() != 1 ||
            duplicate_comparison.coverage.duplicate_current_structures.size() != 1) {
            std::cout << "❌ Baseline comparison should report duplicate structures instead of silently collapsing them\n";
            ++failures;
        }
        if (duplicate_comparison.health != "duplicate_inputs") {
            std::cout << "❌ Duplicate structures without coverage drift should classify comparison health as duplicate_inputs\n";
            ++failures;
        }
        if (duplicate_comparison.actionability != "not_actionable") {
            std::cout << "❌ Duplicate inputs should classify comparison actionability as not_actionable\n";
            ++failures;
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
        baseline_meta.profile_selected = "ci";
        baseline_meta.profile_applied_defaults = {"size", "runs", "structures", "seed"};
        baseline_meta.profile_explicit_overrides = {"output"};

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

        current_meta = baseline_meta;
        current_meta.profile_explicit_overrides = {"runs", "output", "seed"};
        auto meta_manifest_bad = compare_benchmark_metadata(baseline_meta, current_meta, base_config);
        bool saw_profile_manifest_mismatch = false;
        for (const auto& msg : meta_manifest_bad.errors) {
            if (msg.find("profile_explicit_overrides") != std::string::npos)
                saw_profile_manifest_mismatch = true;
        }
        if (meta_manifest_bad.ok || !saw_profile_manifest_mismatch) {
            std::cout << "❌ Strict baseline intent should fail when profile overrides drift\n";
            ++failures;
        } else {
            std::cout << "✅ Strict baseline intent rejects profile override drift\n";
        }

        // Test print helpers
        print_baseline_report(comparison, base_config.threshold_pct, base_config.noise_floor_pct);
        print_baseline_metadata_report(meta_bad);

        auto duplicate_partial_baseline = coverage_baseline;
        duplicate_partial_baseline.push_back(coverage_baseline_only);
        auto duplicate_partial_current = coverage_current;
        duplicate_partial_current.push_back(coverage_current_only);
        auto duplicate_partial_comparison = compare_against_baseline(duplicate_partial_baseline, duplicate_partial_current, base_config);
        if (duplicate_partial_comparison.health != "partial_coverage_with_duplicates") {
            std::cout << "❌ Partial coverage plus duplicates should classify comparison health as partial_coverage_with_duplicates\n";
            ++failures;
        }
        if (duplicate_partial_comparison.actionability != "not_actionable") {
            std::cout << "❌ Partial coverage plus duplicates should classify comparison actionability as not_actionable\n";
            ++failures;
        }

        BaselineReport report_json;
        report_json.metadata              = meta_ok;
        report_json.comparison            = duplicate_partial_comparison;
        report_json.threshold_pct         = base_config.threshold_pct;
        report_json.noise_floor_pct       = base_config.noise_floor_pct;
        report_json.baseline_path         = "perf_baselines/baseline.json";
        report_json.scope                 = "mean";
        report_json.strict_profile_intent = true;
        report_json.exit_code             = 0;
        write_baseline_report_json("baseline_report_test.json", report_json);
        std::ifstream baseline_report_in("baseline_report_test.json");
        if (!baseline_report_in.good()) {
            std::cout << "❌ Baseline report JSON was not written\n";
            ++failures;
        } else {
            std::string baseline_report_content((std::istreambuf_iterator<char>(baseline_report_in)), std::istreambuf_iterator<char>());
            if (baseline_report_content.find("\"metadata\"") == std::string::npos ||
                baseline_report_content.find("\"comparison\"") == std::string::npos ||
                baseline_report_content.find("\"baseline_path\": \"perf_baselines/baseline.json\"") == std::string::npos ||
                baseline_report_content.find("\"scope\": \"mean\"") == std::string::npos ||
                baseline_report_content.find("\"decision_basis\": \"mean\"") == std::string::npos ||
                baseline_report_content.find("\"health\": \"partial_coverage_with_duplicates\"") == std::string::npos ||
                baseline_report_content.find("\"actionability\": \"not_actionable\"") == std::string::npos ||
                baseline_report_content.find("\"insert_basis\": \"mean\"") == std::string::npos ||
                baseline_report_content.find("\"insert_mean_delta_pct\"") == std::string::npos ||
                baseline_report_content.find("\"insert_p95_delta_pct\"") == std::string::npos ||
                baseline_report_content.find("\"insert_ci_high_delta_pct\"") == std::string::npos ||
                baseline_report_content.find("\"insert_mean_ok\"") == std::string::npos ||
                baseline_report_content.find("\"failures\"") == std::string::npos ||
                baseline_report_content.find("\"coverage\"") == std::string::npos ||
                baseline_report_content.find("\"baseline_structure_count\"") == std::string::npos ||
                baseline_report_content.find("\"comparable_structure_count\"") == std::string::npos ||
                baseline_report_content.find("\"duplicate_baseline_structures\"") == std::string::npos ||
                baseline_report_content.find("\"duplicate_current_structures\"") == std::string::npos ||
                baseline_report_content.find("\"exit_code\": 0") == std::string::npos ||
                baseline_report_content.find("\"strict_profile_intent\": true") == std::string::npos ||
                baseline_report_content.find("\"entries\"") == std::string::npos) {
                std::cout << "❌ Baseline report JSON missing expected structure\n";
                ++failures;
            }
        }

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
        out << "    \"profile\": \"ci\",\n";
        out << "    \"profile_manifest\": {\n";
        out << "      \"selected_profile\": \"ci\",\n";
        out << "      \"applied_defaults\": [\"size\",\"runs\",\"structures\",\"seed\"],\n";
        out << "      \"explicit_overrides\": [\"output\"]\n";
        out << "    },\n";
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
                   *loaded_data.meta.seed != 12345ULL || loaded_data.meta.hash_strategy != "open" ||
                   loaded_data.meta.profile_selected != "ci" || loaded_data.meta.profile_explicit_overrides != std::vector<std::string>{"output"}) {
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
