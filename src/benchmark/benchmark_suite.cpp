#include "benchmark_suite.h"
#include "stats_analyzer.h"
#include "core/timer.h"
#include "core/memory_manager.h"
#include "structures/dynamic_array.h"
#include "structures/linked_list.h"
#include "structures/hash_map.h"
#include <fstream>
#include <iostream>

namespace hashbrowns {

static DataStructurePtr make_structure(const std::string& name) {
    if (name == "array" || name == "dynamic-array") {
        return std::make_unique< DynamicArray<std::pair<int,std::string>> >();
    } else if (name == "slist" || name == "list" || name == "singly-list") {
        return std::make_unique< SinglyLinkedList<std::pair<int,std::string>> >();
    } else if (name == "dlist" || name == "doubly-list") {
        return std::make_unique< DoublyLinkedList<std::pair<int,std::string>> >();
    } else if (name == "hashmap" || name == "hash-map") {
        return std::make_unique<HashMap>(HashStrategy::OPEN_ADDRESSING);
    }
    return nullptr;
}

static void maybe_write_csv(const std::optional<std::string>& csv_path,
                            const std::vector<BenchmarkResult>& results) {
    if (!csv_path) return;
    std::ofstream out(*csv_path);
    if (!out) return;
    out << "structure,insert_ms_mean,insert_ms_stddev,search_ms_mean,search_ms_stddev,remove_ms_mean,remove_ms_stddev,memory_bytes\n";
    for (const auto& r : results) {
        out << r.structure << "," << r.insert_ms_mean << "," << r.insert_ms_stddev
            << "," << r.search_ms_mean << "," << r.search_ms_stddev
            << "," << r.remove_ms_mean << "," << r.remove_ms_stddev
            << "," << r.memory_bytes << "\n";
    }
}

std::vector<BenchmarkResult> BenchmarkSuite::run(const BenchmarkConfig& config) {
    std::vector<BenchmarkResult> out_results;
    if (config.structures.empty()) return out_results;

    for (const auto& name : config.structures) {
        auto ds = make_structure(name);
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
            ds = make_structure(name);

            // Prepare data
            std::vector<int> keys(config.size);
            for (std::size_t i = 0; i < config.size; ++i) keys[i] = static_cast<int>(i);

            // Insert
            Timer t; t.start();
            for (auto k : keys) ds->insert(k, std::to_string(k));
            auto ins_us = t.stop().count();
            insert_ms.push_back(ins_us / 1000.0);

            // Search
            t.start();
            std::string v;
            for (auto k : keys) {
                bool found = ds->search(k, v);
                if (!found) { /* ensure no UB in opt build */ }
            }
            auto sea_us = t.stop().count();
            search_ms.push_back(sea_us / 1000.0);

            // Remove
            t.start();
            for (auto k : keys) { ds->remove(k); }
            auto rem_us = t.stop().count();
            remove_ms.push_back(rem_us / 1000.0);
        }

        auto ins = summarize(insert_ms);
        auto sea = summarize(search_ms);
        auto rem = summarize(remove_ms);

        // One more fresh instance to estimate memory footprint after inserts
        auto mem_ds = make_structure(name);
        for (std::size_t i = 0; i < config.size; ++i) mem_ds->insert(static_cast<int>(i), std::to_string(i));

        BenchmarkResult br;
        br.structure = name;
        br.insert_ms_mean = ins.mean; br.insert_ms_stddev = ins.stddev;
        br.search_ms_mean = sea.mean; br.search_ms_stddev = sea.stddev;
        br.remove_ms_mean = rem.mean; br.remove_ms_stddev = rem.stddev;
        br.memory_bytes = mem_ds->memory_usage();
        out_results.push_back(br);
    }

    maybe_write_csv(config.csv_output, out_results);
    return out_results;
}

} // namespace hashbrowns