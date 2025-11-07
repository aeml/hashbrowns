#include "benchmark/benchmark_suite.h"
#include "tests/unit/test_framework.h"
#include <fstream>
#include <sstream>

using namespace hashbrowns;

TEST_CASE("series_json_writes_meta_and_points") {
    BenchmarkSuite suite;
    BenchmarkConfig cfg;
    cfg.structures = {"array","slist"};
    cfg.size = 16; // will be overridden per size
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
    REQUIRE(in.good());
    std::stringstream buffer; buffer << in.rdbuf();
    auto text = buffer.str();

    // Basic sanity checks without a JSON parser
    CHECK(text.find("\"runs_per_size\": 2") != std::string::npos);
    CHECK(text.find("\"structures\": [\"array\",\"slist\"]") != std::string::npos);
    // Expect 2 sizes * 2 structures = 4 series entries
    size_t count_sizes = 0;
    size_t pos = 0;
    while ((pos = text.find("\"size\":", pos)) != std::string::npos) { ++count_sizes; pos += 7; }
    CHECK(count_sizes >= 4); // at least 4 entries
}
