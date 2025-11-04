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
    if (content.find("\"size\"") == std::string::npos) throw std::runtime_error("JSON meta missing size");
    if (content.find("\"runs\"") == std::string::npos) throw std::runtime_error("JSON meta missing runs");
    if (content.find("\"structures\"") == std::string::npos) throw std::runtime_error("JSON meta missing structures");
    if (content.find("\"hash_strategy\"") == std::string::npos) throw std::runtime_error("JSON meta missing hash_strategy");
}

int run_json_output_tests() {
    std::cout << "JSON Output Test Suite\n";
    std::cout << "=======================\n\n";
    try {
        test_results_json_has_meta_and_results();
        std::cout << "\n✅ JSON output tests passed!\n";
        return 0;
    } catch (const std::exception& e) {
        std::cout << "\n❌ JSON output test failed: " << e.what() << "\n";
        return 1;
    }
}
