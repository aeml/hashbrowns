#include "benchmark/stats_analyzer.h"
#include "test_framework.h"
#include <vector>

using namespace hashbrowns;

TEST_CASE("summarize_extended_basic") {
    std::vector<double> v{1,2,3,4};
    auto s = summarize(v, /*bootstrap_iters=*/0);
    CHECK(std::abs(s.mean - 2.5) < 1e-9);
    CHECK(std::abs(s.median - 2.5) < 1e-9);
    CHECK(s.stddev > 0);
    CHECK(s.p95 >= 3.0 && s.p95 <= 4.0);
}

TEST_CASE("summarize_bootstrap_ci") {
    std::vector<double> v;
    for (int i=0;i<50;++i) v.push_back(10.0);
    auto s = summarize(v, /*bootstrap_iters=*/200);
    // With identical samples, CI should collapse to the mean
    CHECK(std::abs(s.ci_low - 10.0) < 1e-9);
    CHECK(std::abs(s.ci_high - 10.0) < 1e-9);
}
