#include "regression_tester.h"

#include <fstream>
#include <sstream>
#include <unordered_map>
#include <algorithm>
#include <cctype>

namespace hashbrowns {

static inline std::string trim(std::string s) {
    auto not_space = [](unsigned char c){ return !std::isspace(c); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
    s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
    return s;
}

std::vector<BenchmarkRow> read_benchmark_csv(const std::string& path) {
    std::vector<BenchmarkRow> rows;
    std::ifstream in(path);
    if (!in) return rows;

    std::string header;
    if (!std::getline(in, header)) return rows;

    // Expected columns (order):
    // structure,insert_ms_mean,insert_ms_stddev,search_ms_mean,search_ms_stddev,remove_ms_mean,remove_ms_stddev,memory_bytes
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string token;
        BenchmarkRow r;
        try {
            std::getline(ss, token, ','); r.structure = trim(token);
            std::getline(ss, token, ','); r.insert_ms_mean = std::stod(token);
            std::getline(ss, token, ','); r.insert_ms_stddev = std::stod(token);
            std::getline(ss, token, ','); r.search_ms_mean = std::stod(token);
            std::getline(ss, token, ','); r.search_ms_stddev = std::stod(token);
            std::getline(ss, token, ','); r.remove_ms_mean = std::stod(token);
            std::getline(ss, token, ','); r.remove_ms_stddev = std::stod(token);
            std::getline(ss, token, ','); r.memory_bytes = static_cast<std::size_t>(std::stoull(token));
            rows.push_back(r);
        } catch (...) {
            // ignore malformed rows
        }
    }
    return rows;
}

static RegressionResult::Delta compute_delta(double current, double baseline) {
    RegressionResult::Delta d;
    d.abs = current - baseline;
    if (baseline != 0.0) d.pct = (current - baseline) / baseline * 100.0;
    else d.pct = (current == 0.0) ? 0.0 : 100.0;
    return d;
}

RegressionResult compare_benchmarks(const std::string& current_csv,
                                    const std::string& baseline_csv,
                                    double threshold_percent) {
    auto cur = read_benchmark_csv(current_csv);
    auto base = read_benchmark_csv(baseline_csv);
    RegressionResult out;

    std::unordered_map<std::string, BenchmarkRow> base_by_name;
    for (const auto& r : base) base_by_name[r.structure] = r;

    for (const auto& c : cur) {
        auto it = base_by_name.find(c.structure);
        if (it == base_by_name.end()) continue; // skip structures absent in baseline
        const auto& b = it->second;
        RegressionResult::Entry e;
        e.structure = c.structure;
        e.insert_delta = compute_delta(c.insert_ms_mean, b.insert_ms_mean);
        e.search_delta = compute_delta(c.search_ms_mean, b.search_ms_mean);
        e.remove_delta = compute_delta(c.remove_ms_mean, b.remove_ms_mean);
        e.memory_delta = compute_delta(static_cast<double>(c.memory_bytes), static_cast<double>(b.memory_bytes));
        // For time metrics, a positive increase above threshold is a regression
        if (e.insert_delta.pct > threshold_percent || e.search_delta.pct > threshold_percent || e.remove_delta.pct > threshold_percent) {
            out.passed = false;
        }
        out.entries.push_back(e);
    }

    // Build a compact summary
    int regressions = 0;
    for (const auto& e : out.entries) {
        if (e.insert_delta.pct > threshold_percent || e.search_delta.pct > threshold_percent || e.remove_delta.pct > threshold_percent) {
            ++regressions;
        }
    }
    std::ostringstream oss;
    oss << (out.passed ? "PASS" : "FAIL") << ": compared " << out.entries.size() << " structures with threshold " << threshold_percent << "% (time metrics)";
    if (!out.passed) oss << "; regressions=" << regressions;
    out.summary = oss.str();
    return out;
}

} // namespace hashbrowns
