#include "regression_tester.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace hashbrowns {

static inline std::string trim(std::string s) {
    auto not_space = [](unsigned char c) { return !std::isspace(c); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
    s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
    return s;
}

// Split a CSV line into tokens (does not handle quoted fields with commas).
static std::vector<std::string> split_csv(const std::string& line) {
    std::vector<std::string> tokens;
    std::stringstream        ss(line);
    std::string              tok;
    while (std::getline(ss, tok, ',')) {
        tokens.push_back(trim(tok));
    }
    return tokens;
}

std::vector<BenchmarkRow> read_benchmark_csv(const std::string& path) {
    std::vector<BenchmarkRow> rows;
    std::ifstream             in(path);
    if (!in)
        return rows;

    // --- Parse header to build column-name → index map ---
    std::string header_line;
    if (!std::getline(in, header_line))
        return rows;
    auto col_names = split_csv(header_line);

    std::unordered_map<std::string, int> col_idx;
    for (int i = 0; i < static_cast<int>(col_names.size()); ++i) {
        col_idx[col_names[i]] = i;
    }

    // Required columns; the CSV schema may have grown but these must exist.
    auto has   = [&](const std::string& name) { return col_idx.count(name) > 0; };
    auto get_d = [&](const std::vector<std::string>& toks, const std::string& name) -> double {
        if (!has(name))
            return 0.0;
        int idx = col_idx[name];
        if (idx >= static_cast<int>(toks.size()))
            return 0.0;
        try {
            return std::stod(toks[idx]);
        } catch (...) {
            return 0.0;
        }
    };
    auto get_u = [&](const std::vector<std::string>& toks, const std::string& name) -> std::size_t {
        if (!has(name))
            return 0;
        int idx = col_idx[name];
        if (idx >= static_cast<int>(toks.size()))
            return 0;
        try {
            return static_cast<std::size_t>(std::stoull(toks[idx]));
        } catch (...) {
            return 0;
        }
    };

    if (!has("structure"))
        return rows; // unrecognisable schema

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty())
            continue;
        auto toks = split_csv(line);
        if (toks.empty())
            continue;
        BenchmarkRow r;
        r.structure        = toks[col_idx["structure"]];
        r.insert_ms_mean   = get_d(toks, "insert_ms_mean");
        r.insert_ms_stddev = get_d(toks, "insert_ms_stddev");
        r.search_ms_mean   = get_d(toks, "search_ms_mean");
        r.search_ms_stddev = get_d(toks, "search_ms_stddev");
        r.remove_ms_mean   = get_d(toks, "remove_ms_mean");
        r.remove_ms_stddev = get_d(toks, "remove_ms_stddev");
        r.memory_bytes     = get_u(toks, "memory_bytes");
        rows.push_back(r);
    }
    return rows;
}

static RegressionResult::Delta compute_delta(double current, double baseline) {
    RegressionResult::Delta d;
    d.abs = current - baseline;
    if (baseline != 0.0)
        d.pct = (current - baseline) / baseline * 100.0;
    else
        d.pct = (current == 0.0) ? 0.0 : 100.0;
    return d;
}

RegressionResult compare_benchmarks(const std::string& current_csv, const std::string& baseline_csv,
                                    double threshold_percent) {
    auto             cur  = read_benchmark_csv(current_csv);
    auto             base = read_benchmark_csv(baseline_csv);
    RegressionResult out;

    std::unordered_map<std::string, BenchmarkRow> base_by_name;
    for (const auto& r : base)
        base_by_name[r.structure] = r;

    for (const auto& c : cur) {
        auto it = base_by_name.find(c.structure);
        if (it == base_by_name.end())
            continue; // skip structures absent in baseline
        const auto&             b = it->second;
        RegressionResult::Entry e;
        e.structure    = c.structure;
        e.insert_delta = compute_delta(c.insert_ms_mean, b.insert_ms_mean);
        e.search_delta = compute_delta(c.search_ms_mean, b.search_ms_mean);
        e.remove_delta = compute_delta(c.remove_ms_mean, b.remove_ms_mean);
        e.memory_delta = compute_delta(static_cast<double>(c.memory_bytes), static_cast<double>(b.memory_bytes));
        // For time metrics, a positive increase above threshold is a regression
        if (e.insert_delta.pct > threshold_percent || e.search_delta.pct > threshold_percent ||
            e.remove_delta.pct > threshold_percent) {
            out.passed = false;
        }
        out.entries.push_back(e);
    }

    // Build a compact summary
    int regressions = 0;
    for (const auto& e : out.entries) {
        if (e.insert_delta.pct > threshold_percent || e.search_delta.pct > threshold_percent ||
            e.remove_delta.pct > threshold_percent) {
            ++regressions;
        }
    }
    std::ostringstream oss;
    oss << (out.passed ? "PASS" : "FAIL") << ": compared " << out.entries.size() << " structures with threshold "
        << threshold_percent << "% (time metrics)";
    if (!out.passed)
        oss << "; regressions=" << regressions;
    out.summary = oss.str();
    return out;
}

} // namespace hashbrowns
