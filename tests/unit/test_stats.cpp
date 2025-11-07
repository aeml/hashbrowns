// Plain function-style tests to avoid macro portability issues
#include "benchmark/stats_analyzer.h"
#include <vector>
#include <iostream>
#include <stdexcept>
#include <cmath>

using namespace hashbrowns;

static void stats_basic() {
    std::vector<double> v{1,2,3,4};
    auto s = summarize(v, /*bootstrap_iters=*/0);
    if (!(std::abs(s.mean - 2.5) < 1e-9)) throw std::runtime_error("mean");
    if (!(std::abs(s.median - 2.5) < 1e-9)) throw std::runtime_error("median");
    if (!(s.stddev > 0)) throw std::runtime_error("stddev");
    if (!(s.p95 >= 3.0 && s.p95 <= 4.0)) throw std::runtime_error("p95");
}

static void stats_bootstrap_ci() {
    std::vector<double> v(50, 10.0);
    auto s = summarize(v, /*bootstrap_iters=*/200);
    if (!(std::abs(s.ci_low - 10.0) < 1e-9)) throw std::runtime_error("ci_low");
    if (!(std::abs(s.ci_high - 10.0) < 1e-9)) throw std::runtime_error("ci_high");
}

int run_stats_tests() {
    std::cout << "Stats Analyzer Test Suite\n";
    std::cout << "=========================\n\n";
    try {
        stats_basic();
        stats_bootstrap_ci();
        std::cout << "\n✅ Stats tests passed!\n";
        return 0;
    } catch (const std::exception& e) {
        std::cout << "\n❌ Stats test failed: " << e.what() << "\n";
        return 1;
    }
}
