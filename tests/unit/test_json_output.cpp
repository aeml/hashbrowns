// Minimal unit test using simple asserts, consistent with existing tests
#include "benchmark/benchmark_suite.h"
#include <fstream>
#include <iostream>
#include <string>

using namespace hashbrowns;

static void test_results_json_has_meta_and_results() {
    BenchmarkConfig cfg;
    cfg.size = 64;
    cfg.runs = 1;
    cfg.warmup_runs = 1;
    cfg.bootstrap_iters = 10;
    cfg.structures = {"hashmap"};
    cfg.output_format = BenchmarkConfig::OutputFormat::JSON;
    cfg.csv_output = std::string("json_test_output.json");
    cfg.pattern = BenchmarkConfig::Pattern::SEQUENTIAL;
    cfg.hash_strategy = HashStrategy::OPEN_ADDRESSING;

    BenchmarkSuite suite;
    auto res = suite.run(cfg);
    if (res.empty()) throw std::runtime_error("No results produced for JSON test");

    std::ifstream in("json_test_output.json");
    if (!in.good()) throw std::runtime_error("json_test_output.json not written");
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (content.find("\"meta\"") == std::string::npos) throw std::runtime_error("JSON missing meta section");
    if (content.find("\"results\"") == std::string::npos) throw std::runtime_error("JSON missing results section");
    // meta keys
    if (content.find("\"size\"") == std::string::npos) throw std::runtime_error("JSON meta missing size");
    if (content.find("\"runs\"") == std::string::npos) throw std::runtime_error("JSON meta missing runs");
    if (content.find("\"warmup_runs\"") == std::string::npos) throw std::runtime_error("JSON meta missing warmup_runs");
    if (content.find("\"bootstrap_iters\"") == std::string::npos) throw std::runtime_error("JSON meta missing bootstrap_iters");
    if (content.find("\"structures\"") == std::string::npos) throw std::runtime_error("JSON meta missing structures");
    if (content.find("\"pattern\"") == std::string::npos) throw std::runtime_error("JSON meta missing pattern");
    if (content.find("\"hash_strategy\"") == std::string::npos) throw std::runtime_error("JSON meta missing hash_strategy");
    if (content.find("\"timestamp\"") == std::string::npos) throw std::runtime_error("JSON meta missing timestamp");
    if (content.find("\"cpu_governor\"") == std::string::npos) throw std::runtime_error("JSON meta missing cpu_governor");
    if (content.find("\"git_commit\"") == std::string::npos) throw std::runtime_error("JSON meta missing git_commit");
    if (content.find("\"compiler\"") == std::string::npos) throw std::runtime_error("JSON meta missing compiler");
    if (content.find("\"cpp_standard\"") == std::string::npos) throw std::runtime_error("JSON meta missing cpp_standard");
    if (content.find("\"build_type\"") == std::string::npos) throw std::runtime_error("JSON meta missing build_type");
    if (content.find("\"cpu_model\"") == std::string::npos) throw std::runtime_error("JSON meta missing cpu_model");
    if (content.find("\"cores\"") == std::string::npos) throw std::runtime_error("JSON meta missing cores");
    if (content.find("\"total_ram_bytes\"") == std::string::npos) throw std::runtime_error("JSON meta missing total_ram_bytes");
    if (content.find("\"kernel\"") == std::string::npos) throw std::runtime_error("JSON meta missing kernel");

    // result stats keys
    if (content.find("\"insert_ms_median\"") == std::string::npos) throw std::runtime_error("JSON results missing insert_ms_median");
    if (content.find("\"insert_ms_p95\"") == std::string::npos) throw std::runtime_error("JSON results missing insert_ms_p95");
    if (content.find("\"insert_ci_low\"") == std::string::npos) throw std::runtime_error("JSON results missing insert_ci_low");
    if (content.find("\"insert_ci_high\"") == std::string::npos) throw std::runtime_error("JSON results missing insert_ci_high");
    if (content.find("\"search_ms_median\"") == std::string::npos) throw std::runtime_error("JSON results missing search_ms_median");
    if (content.find("\"search_ms_p95\"") == std::string::npos) throw std::runtime_error("JSON results missing search_ms_p95");
    if (content.find("\"remove_ms_median\"") == std::string::npos) throw std::runtime_error("JSON results missing remove_ms_median");
    if (content.find("\"remove_ms_p95\"") == std::string::npos) throw std::runtime_error("JSON results missing remove_ms_p95");

    // memory delta keys
    if (content.find("\"memory_insert_mean\"") == std::string::npos) throw std::runtime_error("JSON results missing memory_insert_mean");
    if (content.find("\"memory_insert_stddev\"") == std::string::npos) throw std::runtime_error("JSON results missing memory_insert_stddev");
    if (content.find("\"memory_search_mean\"") == std::string::npos) throw std::runtime_error("JSON results missing memory_search_mean");
    if (content.find("\"memory_search_stddev\"") == std::string::npos) throw std::runtime_error("JSON results missing memory_search_stddev");
    if (content.find("\"memory_remove_mean\"") == std::string::npos) throw std::runtime_error("JSON results missing memory_remove_mean");
    if (content.find("\"memory_remove_stddev\"") == std::string::npos) throw std::runtime_error("JSON results missing memory_remove_stddev");
}

static void test_memory_deltas_nonnegative() {
    BenchmarkConfig cfg;
    cfg.size = 64;
    cfg.runs = 2;
    cfg.structures = {"hashmap"};
    cfg.pattern = BenchmarkConfig::Pattern::SEQUENTIAL;

    BenchmarkSuite suite;
    auto res = suite.run(cfg);
    if (res.empty()) throw std::runtime_error("No results produced for memory delta test");
    const auto& r = res.front();
    if (r.memory_insert_bytes_mean < 0.0) throw std::runtime_error("memory_insert_bytes_mean negative");
    if (r.memory_search_bytes_mean < 0.0) throw std::runtime_error("memory_search_bytes_mean negative");
    if (r.memory_remove_bytes_mean < 0.0) throw std::runtime_error("memory_remove_bytes_mean negative");
}

static void test_series_json_has_meta_and_series() {
    BenchmarkConfig cfg;
    cfg.size = 64;
    cfg.runs = 2;
    cfg.structures = {"array","hashmap"};
    cfg.pattern = BenchmarkConfig::Pattern::SEQUENTIAL;
    BenchmarkSuite suite;

    // Build a simple series of two sizes
    std::vector<std::size_t> sizes = {32, 64};
    auto series = suite.run_series(cfg, sizes);
    if (series.empty()) throw std::runtime_error("Series run produced no data");

    const std::string out_path = "series_schema_test_output.json";
    suite.write_series_json(out_path, series, cfg);

    std::ifstream in(out_path);
    if (!in.good()) throw std::runtime_error("series_schema_test_output.json not written");
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    if (content.find("\"meta\"") == std::string::npos) throw std::runtime_error("Series JSON missing meta section");
    if (content.find("\"series\"") == std::string::npos) throw std::runtime_error("Series JSON missing series section");
    if (content.find("\"runs_per_size\"") == std::string::npos) throw std::runtime_error("Series JSON meta missing runs_per_size");
    if (content.find("\"pattern\"") == std::string::npos) throw std::runtime_error("Series JSON meta missing pattern");
    if (content.find("\"structures\"") == std::string::npos) throw std::runtime_error("Series JSON meta missing structures");

    // Check at least one series entry keys
    if (content.find("\"size\"") == std::string::npos) throw std::runtime_error("Series JSON missing size key in entries");
    if (content.find("\"insert_ms\"") == std::string::npos) throw std::runtime_error("Series JSON missing insert_ms key in entries");
    if (content.find("\"search_ms\"") == std::string::npos) throw std::runtime_error("Series JSON missing search_ms key in entries");
    if (content.find("\"remove_ms\"") == std::string::npos) throw std::runtime_error("Series JSON missing remove_ms key in entries");
}

int run_json_output_tests() {
    std::cout << "JSON Output Test Suite\n";
    std::cout << "=======================\n\n";
    try {
        test_results_json_has_meta_and_results();
    test_memory_deltas_nonnegative();
        test_series_json_has_meta_and_series();
        std::cout << "\n✅ JSON output tests passed!\n";
        return 0;
    } catch (const std::exception& e) {
        std::cout << "\n❌ JSON output test failed: " << e.what() << "\n";
        return 1;
    }
}
