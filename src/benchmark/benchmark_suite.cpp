#include "benchmark_suite.h"
#include "stats_analyzer.h"
#include "core/timer.h"
#include "core/memory_manager.h"
#include "structures/dynamic_array.h"
#include "structures/linked_list.h"
#include "structures/hash_map.h"
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <algorithm>
#include <random>
#include <ctime>
#include <cstdio>

namespace hashbrowns {

static std::string read_cpu_governor() {
    std::ifstream f("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
    std::string g; if (f) std::getline(f, g); return g;
}

static std::string git_commit_sha() {
    FILE* pipe = popen("git rev-parse --short HEAD 2>/dev/null", "r");
    if (!pipe) return "unknown";
    char buf[64]; std::string out; if (fgets(buf, sizeof(buf), pipe)) out = buf; pclose(pipe);
    if (!out.empty() && out.back()=='\n') out.pop_back();
    return out.empty()?"unknown":out;
}

static DataStructurePtr make_structure(const std::string& name, const BenchmarkConfig& cfg) {
    if (name == "array" || name == "dynamic-array") {
        return std::make_unique< DynamicArray<std::pair<int,std::string>> >();
    } else if (name == "slist" || name == "list" || name == "singly-list") {
        return std::make_unique< SinglyLinkedList<std::pair<int,std::string>> >();
    } else if (name == "dlist" || name == "doubly-list") {
        return std::make_unique< DoublyLinkedList<std::pair<int,std::string>> >();
    } else if (name == "hashmap" || name == "hash-map") {
        std::size_t cap = cfg.hash_initial_capacity.value_or(16);
        auto hm = std::make_unique<HashMap>(cfg.hash_strategy, cap);
        if (cfg.hash_max_load_factor) hm->set_max_load_factor(*cfg.hash_max_load_factor);
        return hm;
    }
    return nullptr;
}

static void write_results_csv(const std::string& path,
                            const std::vector<BenchmarkResult>& results,
                            const BenchmarkConfig& cfg,
                            unsigned long long actual_seed) {
    std::ofstream out(path);
    if (!out) return;
    out << "structure,seed,insert_ms_mean,insert_ms_stddev,insert_ms_median,insert_ms_p95,insert_ci_low,insert_ci_high,";
    out << "search_ms_mean,search_ms_stddev,search_ms_median,search_ms_p95,search_ci_low,search_ci_high,";
    out << "remove_ms_mean,remove_ms_stddev,remove_ms_median,remove_ms_p95,remove_ci_low,remove_ci_high,";
    out << "memory_bytes,memory_insert_mean,memory_insert_stddev,memory_search_mean,memory_search_stddev,memory_remove_mean,memory_remove_stddev,";
    out << "insert_probes_mean,insert_probes_stddev,search_probes_mean,search_probes_stddev,remove_probes_mean,remove_probes_stddev\n";
    for (const auto& r : results) {
        out << r.structure << "," << actual_seed << ","
            << r.insert_ms_mean << "," << r.insert_ms_stddev << "," << r.insert_ms_median << "," << r.insert_ms_p95 << "," << r.insert_ci_low << "," << r.insert_ci_high << ","
            << r.search_ms_mean << "," << r.search_ms_stddev << "," << r.search_ms_median << "," << r.search_ms_p95 << "," << r.search_ci_low << "," << r.search_ci_high << ","
            << r.remove_ms_mean << "," << r.remove_ms_stddev << "," << r.remove_ms_median << "," << r.remove_ms_p95 << "," << r.remove_ci_low << "," << r.remove_ci_high << ","
            << r.memory_bytes << "," << r.memory_insert_bytes_mean << "," << r.memory_insert_bytes_stddev << "," << r.memory_search_bytes_mean << "," << r.memory_search_bytes_stddev << "," << r.memory_remove_bytes_mean << "," << r.memory_remove_bytes_stddev << ","
            << r.insert_probes_mean << "," << r.insert_probes_stddev << "," << r.search_probes_mean << "," << r.search_probes_stddev << "," << r.remove_probes_mean << "," << r.remove_probes_stddev << "\n";
    }
}

static const char* to_string(hashbrowns::BenchmarkConfig::Pattern p) {
    using P = hashbrowns::BenchmarkConfig::Pattern;
    switch (p) {
        case P::SEQUENTIAL: return "sequential";
        case P::RANDOM: return "random";
        case P::MIXED: return "mixed";
    }
    return "sequential";
}

static const char* to_string(hashbrowns::HashStrategy s) {
    switch (s) {
        case hashbrowns::HashStrategy::OPEN_ADDRESSING: return "open";
        case hashbrowns::HashStrategy::SEPARATE_CHAINING: return "chain";
    }
    return "open";
}

static void write_results_json(const std::string& path,
                               const std::vector<BenchmarkResult>& results,
                               const BenchmarkConfig& config,
                               unsigned long long actual_seed) {
    std::ofstream out(path);
    if (!out) return;
    out << "{\n";
    // metadata
    out << "  \"meta\": {\n";
    out << "    \"size\": " << config.size << ",\n";
    out << "    \"runs\": " << config.runs << ",\n";
    out << "    \"warmup_runs\": " << config.warmup_runs << ",\n";
    out << "    \"bootstrap_iters\": " << config.bootstrap_iters << ",\n";
    out << "    \"structures\": [";
    for (std::size_t i = 0; i < config.structures.size(); ++i) {
        out << "\"" << config.structures[i] << "\"";
        if (i + 1 < config.structures.size()) out << ",";
    }
    out << "],\n";
    out << "    \"pattern\": \"" << to_string(config.pattern) << "\",\n";
    out << "    \"seed\": " << actual_seed << ",\n";
    // environment snapshot
    out << "    \"timestamp\": \"";
    {
        std::time_t t = std::time(nullptr);
        char buf[64]; std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&t));
        out << buf;
    }
    out << "\",\n";
    out << "    \"cpu_governor\": \"" << read_cpu_governor() << "\",\n";
    out << "    \"git_commit\": \"" << git_commit_sha() << "\",\n";
    out << "    \"compiler\": \"" << __VERSION__ << "\",\n";
    out << "    \"hash_strategy\": \"" << to_string(config.hash_strategy) << "\"";
    if (config.hash_initial_capacity) out << ",\n    \"hash_capacity\": " << *config.hash_initial_capacity;
    if (config.hash_max_load_factor) out << ",\n    \"hash_load\": " << *config.hash_max_load_factor;
    out << "\n  },\n";
    // results
    out << "  \"results\": [\n";
    for (std::size_t i = 0; i < results.size(); ++i) {
        const auto& r = results[i];
        out << "    {"
            << "\"structure\": \"" << r.structure << "\"," 
            << "\"insert_ms_mean\": " << r.insert_ms_mean << ","
            << "\"insert_ms_stddev\": " << r.insert_ms_stddev << ","
            << "\"insert_ms_median\": " << r.insert_ms_median << ","
            << "\"insert_ms_p95\": " << r.insert_ms_p95 << ","
            << "\"insert_ci_low\": " << r.insert_ci_low << ","
            << "\"insert_ci_high\": " << r.insert_ci_high << ","
            << "\"search_ms_mean\": " << r.search_ms_mean << ","
            << "\"search_ms_stddev\": " << r.search_ms_stddev << ","
            << "\"search_ms_median\": " << r.search_ms_median << ","
            << "\"search_ms_p95\": " << r.search_ms_p95 << ","
            << "\"search_ci_low\": " << r.search_ci_low << ","
            << "\"search_ci_high\": " << r.search_ci_high << ","
            << "\"remove_ms_mean\": " << r.remove_ms_mean << ","
            << "\"remove_ms_stddev\": " << r.remove_ms_stddev << ","
            << "\"remove_ms_median\": " << r.remove_ms_median << ","
            << "\"remove_ms_p95\": " << r.remove_ms_p95 << ","
            << "\"remove_ci_low\": " << r.remove_ci_low << ","
            << "\"remove_ci_high\": " << r.remove_ci_high << ","
            << "\"memory_bytes\": " << r.memory_bytes << ","
            << "\"memory_insert_mean\": " << r.memory_insert_bytes_mean << ","
            << "\"memory_insert_stddev\": " << r.memory_insert_bytes_stddev << ","
            << "\"memory_search_mean\": " << r.memory_search_bytes_mean << ","
            << "\"memory_search_stddev\": " << r.memory_search_bytes_stddev << ","
        << "\"memory_remove_mean\": " << r.memory_remove_bytes_mean << ","
        << "\"memory_remove_stddev\": " << r.memory_remove_bytes_stddev << ","
        << "\"insert_probes_mean\": " << r.insert_probes_mean << ","
        << "\"insert_probes_stddev\": " << r.insert_probes_stddev << ","
        << "\"search_probes_mean\": " << r.search_probes_mean << ","
        << "\"search_probes_stddev\": " << r.search_probes_stddev << ","
        << "\"remove_probes_mean\": " << r.remove_probes_mean << ","
        << "\"remove_probes_stddev\": " << r.remove_probes_stddev
            << "}" << (i + 1 < results.size() ? "," : "") << "\n";
    }
    out << "  ]\n}\n";
}

std::vector<BenchmarkResult> BenchmarkSuite::run(const BenchmarkConfig& config) {
    std::vector<BenchmarkResult> out_results;
    if (config.structures.empty()) return out_results;

    // RNG setup for RANDOM/MIXED patterns
    std::mt19937_64 rng;
    unsigned long long actual_seed = 0ULL;
    if (config.pattern != BenchmarkConfig::Pattern::SEQUENTIAL) {
        if (config.seed.has_value()) { actual_seed = *config.seed; rng.seed(actual_seed); }
        else { std::random_device rd; actual_seed = ((uint64_t)rd() << 32) ^ rd(); rng.seed(actual_seed); }
    }

    for (const auto& name : config.structures) {
        auto ds = make_structure(name, config);
        if (!ds) {
            std::cerr << "Unknown structure: " << name << "\n";
            continue;
        }

        std::vector<double> insert_ms, search_ms, remove_ms;
    std::vector<double> mem_ins, mem_sea, mem_rem;
    std::vector<double> probes_ins, probes_sea, probes_rem; // only populated for hashmap
        insert_ms.reserve(config.runs);
        search_ms.reserve(config.runs);
        remove_ms.reserve(config.runs);
        mem_ins.reserve(config.runs);
        mem_sea.reserve(config.runs);
        mem_rem.reserve(config.runs);

        // Warm-up runs (discard from aggregation)
        for (int w = 0; w < config.warmup_runs; ++w) {
            auto warm = make_structure(name, config);
            std::vector<int> keys(config.size);
            for (std::size_t i = 0; i < config.size; ++i) keys[i] = static_cast<int>(i);
            std::vector<int> ins_keys = keys, sea_keys = keys, rem_keys = keys;
            if (config.pattern == BenchmarkConfig::Pattern::RANDOM) {
                std::shuffle(ins_keys.begin(), ins_keys.end(), rng);
                sea_keys = ins_keys;
                std::shuffle(rem_keys.begin(), rem_keys.end(), rng);
            } else if (config.pattern == BenchmarkConfig::Pattern::MIXED) {
                std::shuffle(ins_keys.begin(), ins_keys.end(), rng);
                std::shuffle(rem_keys.begin(), rem_keys.end(), rng);
            }
            for (auto k : ins_keys) warm->insert(k, std::to_string(k));
            std::string v; for (auto k : sea_keys) { warm->search(k, v); }
            for (auto k : rem_keys) { warm->remove(k); }
        }

        for (int r = 0; r < config.runs; ++r) {
            // Fresh instance per run
            ds = make_structure(name, config);

            // Prepare data
            std::vector<int> keys(config.size);
            for (std::size_t i = 0; i < config.size; ++i) keys[i] = static_cast<int>(i);
            std::vector<int> ins_keys = keys;
            std::vector<int> sea_keys = keys;
            std::vector<int> rem_keys = keys;
            if (config.pattern == BenchmarkConfig::Pattern::RANDOM) {
                std::shuffle(ins_keys.begin(), ins_keys.end(), rng);
                sea_keys = ins_keys; // random lookup order
                std::shuffle(rem_keys.begin(), rem_keys.end(), rng);
            } else if (config.pattern == BenchmarkConfig::Pattern::MIXED) {
                std::shuffle(ins_keys.begin(), ins_keys.end(), rng);  // random inserts
                // keep sequential search to probe cache/iteration
                std::shuffle(rem_keys.begin(), rem_keys.end(), rng);   // random removals
            }

            // Insert
            MemoryTracker::instance().reset();
            auto mem_before = MemoryTracker::instance().get_stats().current_usage;
            Timer t; t.start();
            // reset hash probes if applicable
            if (name == "hashmap" || name == "hash-map") {
                if (auto hm = dynamic_cast<HashMap*>(ds.get())) hm->metrics_reset();
            }
            for (auto k : ins_keys) ds->insert(k, std::to_string(k));
            auto ins_us = t.stop().count();
            insert_ms.push_back(ins_us / 1000.0);
            auto mem_after_insert = MemoryTracker::instance().get_stats().current_usage;
            mem_ins.push_back(static_cast<double>(mem_after_insert - mem_before));
            if (name == "hashmap" || name == "hash-map") {
                if (auto hm = dynamic_cast<HashMap*>(ds.get())) probes_ins.push_back(hm->avg_insert_probes());
            }

            // Search
            t.start();
            std::string v;
            if (name == "hashmap" || name == "hash-map") {
                if (auto hm = dynamic_cast<HashMap*>(ds.get())) hm->metrics_reset();
            }
            for (auto k : sea_keys) {
                bool found = ds->search(k, v);
                if (!found) { /* ensure no UB in opt build */ }
            }
            auto sea_us = t.stop().count();
            search_ms.push_back(sea_us / 1000.0);
            auto mem_after_search = MemoryTracker::instance().get_stats().current_usage;
            mem_sea.push_back(static_cast<double>(mem_after_search - mem_after_insert));
            if (name == "hashmap" || name == "hash-map") {
                if (auto hm = dynamic_cast<HashMap*>(ds.get())) probes_sea.push_back(hm->avg_search_probes());
            }

            // Remove
            t.start();
            if (name == "hashmap" || name == "hash-map") {
                if (auto hm = dynamic_cast<HashMap*>(ds.get())) hm->metrics_reset();
            }
            for (auto k : rem_keys) { ds->remove(k); }
            auto rem_us = t.stop().count();
            remove_ms.push_back(rem_us / 1000.0);
            auto mem_after_remove = MemoryTracker::instance().get_stats().current_usage;
            mem_rem.push_back(static_cast<double>(mem_after_remove - mem_after_search));
            if (name == "hashmap" || name == "hash-map") {
                if (auto hm = dynamic_cast<HashMap*>(ds.get())) probes_rem.push_back(hm->avg_remove_probes());
            }
        }

        auto ins = summarize(insert_ms, config.bootstrap_iters);
        auto sea = summarize(search_ms, config.bootstrap_iters);
        auto rem = summarize(remove_ms, config.bootstrap_iters);
        auto mins = summarize(mem_ins);
        auto msea = summarize(mem_sea);
        auto mrem = summarize(mem_rem);

        // One more fresh instance to estimate memory footprint after inserts
    auto mem_ds = make_structure(name, config);
    for (std::size_t i = 0; i < config.size; ++i) mem_ds->insert(static_cast<int>(i), std::to_string(i));

        BenchmarkResult br;
        br.structure = name;
        br.insert_ms_mean = ins.mean; br.insert_ms_stddev = ins.stddev; br.insert_ms_median = ins.median; br.insert_ms_p95 = ins.p95; br.insert_ci_low = ins.ci_low; br.insert_ci_high = ins.ci_high;
        br.search_ms_mean = sea.mean; br.search_ms_stddev = sea.stddev; br.search_ms_median = sea.median; br.search_ms_p95 = sea.p95; br.search_ci_low = sea.ci_low; br.search_ci_high = sea.ci_high;
        br.remove_ms_mean = rem.mean; br.remove_ms_stddev = rem.stddev; br.remove_ms_median = rem.median; br.remove_ms_p95 = rem.p95; br.remove_ci_low = rem.ci_low; br.remove_ci_high = rem.ci_high;
        br.memory_bytes = mem_ds->memory_usage();
        br.memory_insert_bytes_mean = mins.mean; br.memory_insert_bytes_stddev = mins.stddev;
        br.memory_search_bytes_mean = msea.mean; br.memory_search_bytes_stddev = msea.stddev;
        br.memory_remove_bytes_mean = mrem.mean; br.memory_remove_bytes_stddev = mrem.stddev;
        // HashMap-specific probe summaries
        if (!probes_ins.empty()) {
            auto s = summarize(probes_ins);
            br.insert_probes_mean = s.mean; br.insert_probes_stddev = s.stddev;
        }
        if (!probes_sea.empty()) {
            auto s = summarize(probes_sea);
            br.search_probes_mean = s.mean; br.search_probes_stddev = s.stddev;
        }
        if (!probes_rem.empty()) {
            auto s = summarize(probes_rem);
            br.remove_probes_mean = s.mean; br.remove_probes_stddev = s.stddev;
        }
        out_results.push_back(br);
    }

    if (config.csv_output) {
        if (config.output_format == BenchmarkConfig::OutputFormat::CSV) write_results_csv(*config.csv_output, out_results, config, actual_seed);
        else write_results_json(*config.csv_output, out_results, config, actual_seed);
    }
    return out_results;
}

BenchmarkSuite::Series BenchmarkSuite::run_series(const BenchmarkConfig& baseConfig, const std::vector<std::size_t>& sizes) {
    Series all;
    for (auto s : sizes) {
        BenchmarkConfig cfg = baseConfig;
        cfg.size = s;
        auto results = run(cfg);
        for (const auto& r : results) {
            all.push_back(SeriesPoint{ s, r.structure, r.insert_ms_mean, r.search_ms_mean, r.remove_ms_mean });
        }
    }
    return all;
}

static std::optional<std::size_t> find_crossover_1d(const std::vector<std::pair<std::size_t,double>>& a,
                                                     const std::vector<std::pair<std::size_t,double>>& b) {
    // assumes same size sequence; scan for sign flip in diff and linearly interpolate
    for (std::size_t i = 1; i < a.size(); ++i) {
        double d0 = a[i-1].second - b[i-1].second;
        double d1 = a[i].second - b[i].second;
        if ((d0 <= 0 && d1 >= 0) || (d0 >= 0 && d1 <= 0)) {
            // linear interpolation between sizes
            double x0 = static_cast<double>(a[i-1].first);
            double x1 = static_cast<double>(a[i].first);
            if (std::fabs(d1 - d0) < 1e-9) return static_cast<std::size_t>((x0 + x1) / 2);
            double t = (-d0) / (d1 - d0);
            double xc = x0 + t * (x1 - x0);
            if (xc < x0) { xc = x0; }
            if (xc > x1) { xc = x1; }
            return static_cast<std::size_t>(xc);
        }
    }
    return std::nullopt;
}

std::vector<BenchmarkSuite::CrossoverInfo> BenchmarkSuite::compute_crossovers(const Series& series) {
    std::vector<CrossoverInfo> out;
    // group by structure
    std::map<std::string, std::vector<std::pair<std::size_t,double>>> ins, sea, rem;
    for (const auto& p : series) {
        ins[p.structure].push_back({p.size, p.insert_ms});
        sea[p.structure].push_back({p.size, p.search_ms});
        rem[p.structure].push_back({p.size, p.remove_ms});
    }
    // ensure each vector is sorted by size
    auto sort_by_size = [](auto& m){ for (auto& kv : m) std::sort(kv.second.begin(), kv.second.end(), [](auto& a, auto& b){return a.first<b.first;}); };
    sort_by_size(ins); sort_by_size(sea); sort_by_size(rem);
    // consider pairs of structures
    std::vector<std::string> names; names.reserve(ins.size());
    for (auto& kv : ins) names.push_back(kv.first);
    for (std::size_t i = 0; i < names.size(); ++i) {
        for (std::size_t j = i+1; j < names.size(); ++j) {
            const auto& A = names[i];
            const auto& B = names[j];
            if (ins[A].size() == ins[B].size() && !ins[A].empty()) {
                if (auto cx = find_crossover_1d(ins[A], ins[B])) out.push_back(CrossoverInfo{"insert", A, B, *cx});
            }
            if (sea[A].size() == sea[B].size() && !sea[A].empty()) {
                if (auto cx = find_crossover_1d(sea[A], sea[B])) out.push_back(CrossoverInfo{"search", A, B, *cx});
            }
            if (rem[A].size() == rem[B].size() && !rem[A].empty()) {
                if (auto cx = find_crossover_1d(rem[A], rem[B])) out.push_back(CrossoverInfo{"remove", A, B, *cx});
            }
        }
    }
    return out;
}

void BenchmarkSuite::write_crossover_csv(const std::string& path, const std::vector<CrossoverInfo>& info) {
    std::ofstream out(path);
    if (!out) return;
    out << "operation,a,b,size_at_crossover\n";
    for (const auto& c : info) {
        out << c.operation << "," << c.a << "," << c.b << "," << c.size_at_crossover << "\n";
    }
}

void BenchmarkSuite::write_crossover_json(const std::string& path, const std::vector<CrossoverInfo>& info, const BenchmarkConfig& config) {
    std::ofstream out(path);
    if (!out) return;
    out << "{\n";
    // metadata
    out << "  \"meta\": {\n";
    out << "    \"runs\": " << config.runs << ",\n";
    out << "    \"structures\": [";
    for (std::size_t i = 0; i < config.structures.size(); ++i) {
        out << "\"" << config.structures[i] << "\"";
        if (i + 1 < config.structures.size()) out << ",";
    }
    out << "],\n";
    out << "    \"pattern\": \"" << to_string(config.pattern) << "\"";
    if (config.seed.has_value()) out << ",\n    \"seed\": " << *config.seed;
    out << "\n  },\n";
    // crossovers
    out << "  \"crossovers\": [\n";
    for (std::size_t i = 0; i < info.size(); ++i) {
        const auto& c = info[i];
        out << "    {"
            << "\"operation\": \"" << c.operation << "\","
            << "\"a\": \"" << c.a << "\","
            << "\"b\": \"" << c.b << "\","
            << "\"size_at_crossover\": " << c.size_at_crossover
            << "}" << (i + 1 < info.size() ? "," : "") << "\n";
    }
    out << "  ]\n}\n";
}

void BenchmarkSuite::write_series_csv(const std::string& path, const Series& series) {
    std::ofstream out(path);
    if (!out) return;
    out << "size,structure,insert_ms,search_ms,remove_ms\n";
    for (const auto& p : series) {
        out << p.size << "," << p.structure << ","
            << p.insert_ms << "," << p.search_ms << "," << p.remove_ms << "\n";
    }
}

void BenchmarkSuite::write_series_json(const std::string& path, const Series& series, const BenchmarkConfig& config) {
    std::ofstream out(path);
    if (!out) return;
    out << "{\n";
    // metadata similar to results/crossover
    out << "  \"meta\": {\n";
    out << "    \"runs_per_size\": " << config.runs << ",\n";
    out << "    \"structures\": [";
    for (std::size_t i = 0; i < config.structures.size(); ++i) {
        out << "\"" << config.structures[i] << "\"";
        if (i + 1 < config.structures.size()) out << ",";
    }
    out << "],\n";
    out << "    \"pattern\": \"" << to_string(config.pattern) << "\"";
    if (config.seed.has_value()) out << ",\n    \"seed\": " << *config.seed;
    out << "\n  },\n";
    out << "  \"series\": [\n";
    for (std::size_t i = 0; i < series.size(); ++i) {
        const auto& p = series[i];
        out << "    {\"size\": " << p.size
            << ", \"structure\": \"" << p.structure << "\""
            << ", \"insert_ms\": " << p.insert_ms
            << ", \"search_ms\": " << p.search_ms
            << ", \"remove_ms\": " << p.remove_ms
            << "}" << (i + 1 < series.size() ? "," : "") << "\n";
    }
    out << "  ]\n}\n";
}

} // namespace hashbrowns