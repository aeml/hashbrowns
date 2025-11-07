// Plain function-style tests to avoid macro portability issues
#include "benchmark/benchmark_suite.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>

using namespace hashbrowns;

int run_series_json_tests() {
    std::cout << "Series JSON Test Suite\n";
    std::cout << "=======================\n\n";
    try {
        BenchmarkSuite suite;
        BenchmarkConfig cfg;
        cfg.structures = {"array","slist"};
        cfg.size = 16; // overridden per size
        cfg.runs = 2;  // small runs
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

        if (text.find("\"runs_per_size\": 2") == std::string::npos)
            throw std::runtime_error("Series JSON missing runs_per_size");
        if (text.find("\"structures\": [\"array\",\"slist\"]") == std::string::npos)
            throw std::runtime_error("Series JSON missing structures list");
        // Expect 2 sizes * 2 structures = 4 series entries
        size_t count_sizes = 0;
        size_t pos = 0;
        while ((pos = text.find("\"size\":", pos)) != std::string::npos) { ++count_sizes; pos += 7; }
        if (count_sizes < 4)
            throw std::runtime_error("Series JSON has fewer points than expected");

        std::cout << "\n✅ Series JSON tests passed!\n";
    return 0;
    } catch (const std::exception& e) {
        std::cout << "\n❌ Series JSON test failed: " << e.what() << "\n";
        return 1;
    }
}
