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

namespace hashbrowns {

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
                            const std::vector<BenchmarkResult>& results) {
    std::ofstream out(path);
    if (!out) return;
    out << "structure,insert_ms_mean,insert_ms_stddev,search_ms_mean,search_ms_stddev,remove_ms_mean,remove_ms_stddev,memory_bytes\n";
    for (const auto& r : results) {
        out << r.structure << "," << r.insert_ms_mean << "," << r.insert_ms_stddev
            << "," << r.search_ms_mean << "," << r.search_ms_stddev
            << "," << r.remove_ms_mean << "," << r.remove_ms_stddev
            << "," << r.memory_bytes << "\n";
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
                               const BenchmarkConfig& config) {
    std::ofstream out(path);
    if (!out) return;
    out << "{\n";
    // metadata
    out << "  \"meta\": {\n";
    out << "    \"size\": " << config.size << ",\n";
    out << "    \"runs\": " << config.runs << ",\n";
    out << "    \"structures\": [";
    for (std::size_t i = 0; i < config.structures.size(); ++i) {
        out << "\"" << config.structures[i] << "\"";
        if (i + 1 < config.structures.size()) out << ",";
    }
    out << "],\n";
    out << "    \"pattern\": \"" << to_string(config.pattern) << "\",\n";
    if (config.seed.has_value()) out << "    \"seed\": " << *config.seed << ",\n";
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
            << "\"search_ms_mean\": " << r.search_ms_mean << ","
            << "\"search_ms_stddev\": " << r.search_ms_stddev << ","
            << "\"remove_ms_mean\": " << r.remove_ms_mean << ","
            << "\"remove_ms_stddev\": " << r.remove_ms_stddev << ","
            << "\"memory_bytes\": " << r.memory_bytes
            << "}" << (i + 1 < results.size() ? "," : "") << "\n";
    }
    out << "  ]\n}\n";
}

std::vector<BenchmarkResult> BenchmarkSuite::run(const BenchmarkConfig& config) {
    std::vector<BenchmarkResult> out_results;
    if (config.structures.empty()) return out_results;

    // RNG setup for RANDOM/MIXED patterns
    std::mt19937_64 rng;
    if (config.pattern != BenchmarkConfig::Pattern::SEQUENTIAL) {
        if (config.seed.has_value()) rng.seed(*config.seed);
        else {
            std::random_device rd;
            rng.seed(((uint64_t)rd() << 32) ^ rd());
        }
    }

    for (const auto& name : config.structures) {
        auto ds = make_structure(name, config);
        if (!ds) {
            std::cerr << "Unknown structure: " << name << "\n";
            continue;
        }

        std::vector<double> insert_ms, search_ms, remove_ms;
        insert_ms.reserve(config.runs);
        search_ms.reserve(config.runs);
        remove_ms.reserve(config.runs);

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
            Timer t; t.start();
            for (auto k : ins_keys) ds->insert(k, std::to_string(k));
            auto ins_us = t.stop().count();
            insert_ms.push_back(ins_us / 1000.0);

            // Search
            t.start();
            std::string v;
            for (auto k : sea_keys) {
                bool found = ds->search(k, v);
                if (!found) { /* ensure no UB in opt build */ }
            }
            auto sea_us = t.stop().count();
            search_ms.push_back(sea_us / 1000.0);

            // Remove
            t.start();
            for (auto k : rem_keys) { ds->remove(k); }
            auto rem_us = t.stop().count();
            remove_ms.push_back(rem_us / 1000.0);
        }

        auto ins = summarize(insert_ms);
        auto sea = summarize(search_ms);
        auto rem = summarize(remove_ms);

        // One more fresh instance to estimate memory footprint after inserts
    auto mem_ds = make_structure(name, config);
    for (std::size_t i = 0; i < config.size; ++i) mem_ds->insert(static_cast<int>(i), std::to_string(i));

        BenchmarkResult br;
        br.structure = name;
        br.insert_ms_mean = ins.mean; br.insert_ms_stddev = ins.stddev;
        br.search_ms_mean = sea.mean; br.search_ms_stddev = sea.stddev;
        br.remove_ms_mean = rem.mean; br.remove_ms_stddev = rem.stddev;
        br.memory_bytes = mem_ds->memory_usage();
        out_results.push_back(br);
    }

    if (config.csv_output) {
        if (config.output_format == BenchmarkConfig::OutputFormat::CSV) write_results_csv(*config.csv_output, out_results);
        else write_results_json(*config.csv_output, out_results, config);
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

} // namespace hashbrowns