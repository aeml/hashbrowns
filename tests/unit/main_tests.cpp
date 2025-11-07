#include <iostream>

int run_dynamic_array_tests();
int run_linked_list_tests();
int run_hash_map_tests();
int run_json_output_tests();
// Integrated series JSON and stats tests (inlined to avoid linkage edge cases on some platforms)
#include "benchmark/benchmark_suite.h"
#include "benchmark/stats_analyzer.h"
#include <fstream>
#include <sstream>
#include <stdexcept>

using namespace hashbrowns;

static int run_series_json_tests() {
    std::cout << "Series JSON Test Suite\n";
    std::cout << "=======================\n\n";
    try {
        BenchmarkSuite suite;
        BenchmarkConfig cfg;
        cfg.structures = {"array","slist"};
        cfg.size = 16;
        cfg.runs = 2;
        cfg.pattern = BenchmarkConfig::Pattern::SEQUENTIAL;
        cfg.output_format = BenchmarkConfig::OutputFormat::JSON;
        std::vector<std::size_t> sizes = {8, 16};
        BenchmarkSuite::Series series;
        for (auto s : sizes) {
            cfg.size = s;
            auto res = suite.run(cfg);
            for (const auto& r : res) series.push_back({ s, r.structure, r.insert_ms_mean, r.search_ms_mean, r.remove_ms_mean });
        }
        const char* path = "series_test.json";
        suite.write_series_json(path, series, cfg);
        std::ifstream in(path);
        if (!in.good()) throw std::runtime_error("series_test.json not written");
        std::stringstream buffer; buffer << in.rdbuf();
        auto text = buffer.str();
        if (text.find("\"runs_per_size\": 2") == std::string::npos) throw std::runtime_error("runs_per_size missing");
        if (text.find("\"structures\": [\"array\",\"slist\"]") == std::string::npos) throw std::runtime_error("structures list missing");
        size_t count_sizes = 0; size_t pos = 0; while ((pos = text.find("\"size\":", pos)) != std::string::npos) { ++count_sizes; pos += 7; }
        if (count_sizes < 4) throw std::runtime_error("insufficient series points");
        std::cout << "\n✅ Series JSON tests passed!\n";
        return 0;
    } catch (const std::exception& e) {
        std::cout << "\n❌ Series JSON test failed: " << e.what() << "\n"; return 1;
    }
}

static int run_stats_tests() {
    std::cout << "Stats Analyzer Test Suite\n";
    std::cout << "=========================\n\n";
    try {
        {
            std::vector<double> v{1,2,3,4};
            auto s = summarize(v, 0);
            if (!(std::abs(s.mean - 2.5) < 1e-9)) throw std::runtime_error("mean");
            if (!(std::abs(s.median - 2.5) < 1e-9)) throw std::runtime_error("median");
            if (!(s.stddev > 0)) throw std::runtime_error("stddev");
            if (!(s.p95 >= 3.0 && s.p95 <= 4.0)) throw std::runtime_error("p95");
        }
        {
            std::vector<double> v(50, 10.0);
            auto s = summarize(v, 200);
            if (!(std::abs(s.ci_low - 10.0) < 1e-9)) throw std::runtime_error("ci_low");
            if (!(std::abs(s.ci_high - 10.0) < 1e-9)) throw std::runtime_error("ci_high");
        }
        std::cout << "\n✅ Stats tests passed!\n"; return 0;
    } catch (const std::exception& e) { std::cout << "\n❌ Stats test failed: " << e.what() << "\n"; return 1; }
}

int main() {
    int failures = 0;
    std::cout << "\n=== Running All Unit Test Suites ===\n";

    failures += run_dynamic_array_tests();
    std::cout << "\n------------------------------------\n";
    failures += run_linked_list_tests();
    std::cout << "\n------------------------------------\n";
    failures += run_hash_map_tests();
    std::cout << "\n------------------------------------\n";
    failures += run_json_output_tests();
    std::cout << "\n------------------------------------\n";
    failures += run_series_json_tests();
    std::cout << "\n------------------------------------\n";
    failures += run_stats_tests();

    if (failures == 0) {
        std::cout << "\n✅ All unit test suites passed!\n";
    } else {
        std::cout << "\n❌ " << failures << " suite(s) reported failures.\n";
    }
    return failures == 0 ? 0 : 1;
}
