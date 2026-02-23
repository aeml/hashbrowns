// benchmark_baseline.cpp
// Baseline loading, comparison, and reporting helpers for BenchmarkSuite.
// Extracted from benchmark_suite.cpp to isolate schema-aware JSON parsing
// and regression-detection logic from the timing run loop.

#include "benchmark_suite.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace hashbrowns {

// ---------------------------------------------------------------------------
// Internal helpers (file-local)
// ---------------------------------------------------------------------------

static std::string trim_ws(const std::string& s) {
    std::size_t start = 0, end = s.size();
    while (start < end && std::isspace(static_cast<unsigned char>(s[start])))
        ++start;
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])))
        --end;
    return s.substr(start, end - start);
}

static double pct_delta(double baseline, double current) {
    if (baseline == 0.0)
        return 0.0;
    return (current - baseline) * 100.0 / baseline;
}

// ---------------------------------------------------------------------------
// load_benchmark_results_json
// ---------------------------------------------------------------------------

// Very small, schema-aware JSON reader tailored to benchmark_results.json.
// It assumes the existing write_results_json layout and extracts only fields
// needed for baseline comparison.

std::vector<BenchmarkResult> load_benchmark_results_json(const std::string& path) {
    std::ifstream in(path);
    if (!in)
        return {};
    std::string                  json((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    std::vector<BenchmarkResult> out;

    std::size_t pos = json.find("\"results\"");
    if (pos == std::string::npos)
        return {};
    pos = json.find('[', pos);
    if (pos == std::string::npos)
        return {};
    ++pos;

    while (pos < json.size()) {
        pos = json.find('{', pos);
        if (pos == std::string::npos)
            break;
        auto end = json.find('}', pos);
        if (end == std::string::npos)
            break;
        std::string obj = json.substr(pos + 1, end - pos - 1);
        pos             = end + 1;

        BenchmarkResult r;

        auto extract_double = [&obj](const char* key, double& target) {
            std::string pattern = std::string("\"") + key + "\"";
            auto        kpos    = obj.find(pattern);
            if (kpos == std::string::npos)
                return;
            kpos = obj.find(':', kpos);
            if (kpos == std::string::npos)
                return;
            ++kpos;
            std::size_t endv = obj.find_first_of(",}\n", kpos);
            std::string val  = trim_ws(obj.substr(kpos, endv - kpos));
            try {
                target = std::stod(val);
            } catch (...) {
            }
        };

        auto extract_size_t = [&obj](const char* key, std::size_t& target) {
            std::string pattern = std::string("\"") + key + "\"";
            auto        kpos    = obj.find(pattern);
            if (kpos == std::string::npos)
                return;
            kpos = obj.find(':', kpos);
            if (kpos == std::string::npos)
                return;
            ++kpos;
            std::size_t endv = obj.find_first_of(",}\n", kpos);
            std::string val  = trim_ws(obj.substr(kpos, endv - kpos));
            try {
                target = static_cast<std::size_t>(std::stoull(val));
            } catch (...) {
            }
        };

        // structure name
        {
            std::string pattern = "\"structure\"";
            auto        kpos    = obj.find(pattern);
            if (kpos != std::string::npos) {
                kpos = obj.find(':', kpos);
                if (kpos != std::string::npos) {
                    kpos = obj.find('"', kpos);
                    if (kpos != std::string::npos) {
                        auto endv = obj.find('"', kpos + 1);
                        if (endv != std::string::npos) {
                            r.structure = obj.substr(kpos + 1, endv - kpos - 1);
                        }
                    }
                }
            }
        }

        extract_double("insert_ms_mean", r.insert_ms_mean);
        extract_double("search_ms_mean", r.search_ms_mean);
        extract_double("remove_ms_mean", r.remove_ms_mean);
        extract_double("insert_ms_p95", r.insert_ms_p95);
        extract_double("search_ms_p95", r.search_ms_p95);
        extract_double("remove_ms_p95", r.remove_ms_p95);
        extract_double("insert_ci_high", r.insert_ci_high);
        extract_double("search_ci_high", r.search_ci_high);
        extract_double("remove_ci_high", r.remove_ci_high);
        extract_size_t("memory_bytes", r.memory_bytes);

        if (!r.structure.empty()) {
            out.push_back(r);
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// compare_against_baseline
// ---------------------------------------------------------------------------

BaselineComparison compare_against_baseline(const std::vector<BenchmarkResult>& baseline,
                                            const std::vector<BenchmarkResult>& current, const BaselineConfig& cfg) {
    BaselineComparison out;
    if (baseline.empty() || current.empty())
        return out;

    std::map<std::string, BenchmarkResult> base_map;
    for (const auto& b : baseline)
        base_map[b.structure] = b;

    for (const auto& cur : current) {
        auto it = base_map.find(cur.structure);
        if (it == base_map.end())
            continue;
        const auto&               b = it->second;
        BaselineComparison::Entry e;
        e.structure = cur.structure;

        auto pick = [&cfg](double mean, double p95, double ci_high) {
            switch (cfg.scope) {
            case BaselineConfig::MetricScope::MEAN:
                return mean;
            case BaselineConfig::MetricScope::P95:
                return p95;
            case BaselineConfig::MetricScope::CI_HIGH:
                return ci_high;
            case BaselineConfig::MetricScope::ANY:
            default:
                return mean;
            }
        };

        double b_ins = pick(b.insert_ms_mean, b.insert_ms_p95, b.insert_ci_high);
        double b_sea = pick(b.search_ms_mean, b.search_ms_p95, b.search_ci_high);
        double b_rem = pick(b.remove_ms_mean, b.remove_ms_p95, b.remove_ci_high);
        double c_ins = pick(cur.insert_ms_mean, cur.insert_ms_p95, cur.insert_ci_high);
        double c_sea = pick(cur.search_ms_mean, cur.search_ms_p95, cur.search_ci_high);
        double c_rem = pick(cur.remove_ms_mean, cur.remove_ms_p95, cur.remove_ci_high);

        e.insert_delta_pct = pct_delta(b_ins, c_ins);
        e.search_delta_pct = pct_delta(b_sea, c_sea);
        e.remove_delta_pct = pct_delta(b_rem, c_rem);

        auto within = [&cfg](double delta) {
            double absd = std::fabs(delta);
            if (absd <= cfg.noise_floor_pct)
                return true;
            return delta <= cfg.threshold_pct;
        };

        e.insert_ok = within(e.insert_delta_pct);
        e.search_ok = within(e.search_delta_pct);
        e.remove_ok = within(e.remove_delta_pct);

        if (!e.insert_ok || !e.search_ok || !e.remove_ok)
            out.all_ok = false;
        out.entries.push_back(e);
    }
    return out;
}

// ---------------------------------------------------------------------------
// print_baseline_report
// ---------------------------------------------------------------------------

void print_baseline_report(const BaselineComparison& report, double threshold_pct, double noise_floor_pct) {
    if (report.entries.empty()) {
        std::cout << "[baseline] No comparable structures between baseline and current results.\n";
        return;
    }
    std::cout << "[baseline] Threshold=" << threshold_pct << "% (noise floor=" << noise_floor_pct << "%)\n";
    for (const auto& e : report.entries) {
        auto status = (e.insert_ok && e.search_ok && e.remove_ok) ? "OK" : "FAIL";
        std::cout << "  " << status << "  " << e.structure << "  insert=" << e.insert_delta_pct << "%"
                  << "  search=" << e.search_delta_pct << "%"
                  << "  remove=" << e.remove_delta_pct << "%"
                  << "\n";
    }
    if (report.all_ok) {
        std::cout << "[baseline] All metrics within tolerance." << std::endl;
    } else {
        std::cout << "[baseline] Performance regression detected." << std::endl;
    }
}

} // namespace hashbrowns
