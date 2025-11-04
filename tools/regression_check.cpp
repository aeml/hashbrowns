#include <iostream>
#include <string>
#include <cstdlib>
#include "benchmark/regression_tester.h"

using namespace hashbrowns;

static void usage(const char* argv0) {
    std::cerr << "Usage: " << argv0 << " --current PATH --baseline PATH [--threshold PCT]\n";
}

int main(int argc, char** argv) {
    std::string current, baseline; double threshold = 10.0;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "--current" || arg == "-c") && i + 1 < argc) current = argv[++i];
        else if ((arg == "--baseline" || arg == "-b") && i + 1 < argc) baseline = argv[++i];
        else if ((arg == "--threshold" || arg == "-t") && i + 1 < argc) threshold = std::stod(argv[++i]);
        else if (arg == "--help" || arg == "-h") { usage(argv[0]); return 0; }
    }
    if (current.empty() || baseline.empty()) { usage(argv[0]); return 2; }
    auto res = compare_benchmarks(current, baseline, threshold);
    std::cout << res.summary << "\n";
    for (const auto& e : res.entries) {
        std::cout << "- " << e.structure
                  << " | insert=" << e.insert_delta.pct << "%"
                  << ", search=" << e.search_delta.pct << "%"
                  << ", remove=" << e.remove_delta.pct << "%"
                  << ", mem=" << e.memory_delta.pct << "%\n";
    }
    return res.passed ? 0 : 1;
}
