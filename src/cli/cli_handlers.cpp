#include "cli_handlers.h"

#include "benchmark/benchmark_suite.h"
#include "core/timer.h"
#include "structures/dynamic_array.h"
#include "structures/hash_map.h"
#include "structures/linked_list.h"

#include <cctype>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace hashbrowns {
namespace cli {

// ---------------------------------------------------------------------------
// Internal helpers (file-local)
// ---------------------------------------------------------------------------

static std::string prompt_line(const std::string& question, const std::string& def = "") {
    std::string ans;
    std::cout << question;
    if (!def.empty())
        std::cout << " [" << def << "]";
    std::cout << ": ";
    std::getline(std::cin, ans);
    if (ans.empty())
        return def;
    return ans;
}

static bool prompt_yesno(const std::string& question, bool def = false) {
    std::string defStr = def ? "Y/n" : "y/N";
    while (true) {
        std::string a = prompt_line(question + " (" + defStr + ")");
        if (a.empty())
            return def;
        for (auto& c : a)
            c = static_cast<char>(::tolower(c));
        if (a == "y" || a == "yes")
            return true;
        if (a == "n" || a == "no")
            return false;
        std::cout << "Please answer 'y' or 'n'.\n";
    }
}

static std::vector<std::string> split_list(const std::string& s) {
    std::vector<std::string> out;
    std::size_t              start = 0, pos = 0;
    while ((pos = s.find(',', start)) != std::string::npos) {
        auto t = s.substr(start, pos - start);
        if (!t.empty())
            out.push_back(t);
        start = pos + 1;
    }
    if (start < s.size())
        out.push_back(s.substr(start));
    return out;
}

// ---------------------------------------------------------------------------
// run_wizard
// ---------------------------------------------------------------------------

int run_wizard() {
    using std::cout;
    using std::string;
    using std::vector;
    cout << "\n=== Wizard Mode ===\n";
    cout << "Answer with values or press Enter for defaults.\n\n";

    // Mode selection
    string mode = prompt_line("Mode [benchmark|crossover]", "benchmark");
    for (auto& c : mode)
        c = static_cast<char>(::tolower(c));
    bool crossover = (mode == "crossover" || mode == "sweep");

    // Structures
    string         structs = prompt_line("Structures (comma list or 'all')", "all");
    vector<string> structures;
    if (structs == "all") {
        structures = {"array", "slist", "dlist", "hashmap"};
    } else {
        structures = split_list(structs);
        if (structures.empty())
            structures = {"array", "slist", "dlist", "hashmap"};
    }

    // Basics (interpret size as max if multi-size requested)
    string      size_s        = prompt_line("Max size", "10000");
    std::size_t max_size      = static_cast<std::size_t>(std::stoull(size_s));
    string      runs_s        = prompt_line("Sizes count (number of distinct sizes)", "10");
    int         size_count    = std::stoi(runs_s);
    string      reps_s        = prompt_line("Runs per size", "10");
    int         runs_per_size = std::stoi(reps_s);

    // Pattern and seed
    string                   pattern = prompt_line("Pattern [sequential|random|mixed]", "sequential");
    BenchmarkConfig::Pattern pat     = BenchmarkConfig::Pattern::SEQUENTIAL;
    if (pattern == "random")
        pat = BenchmarkConfig::Pattern::RANDOM;
    else if (pattern == "mixed")
        pat = BenchmarkConfig::Pattern::MIXED;
    string                            seed_s = prompt_line("Seed (blank=random)", "");
    std::optional<unsigned long long> seed;
    if (!seed_s.empty())
        seed = static_cast<unsigned long long>(std::stoull(seed_s));

    // Output
    string fmt = prompt_line("Output format [csv|json]", "csv");
    for (auto& c : fmt)
        c = static_cast<char>(::tolower(c));
    BenchmarkConfig::OutputFormat outfmt =
        (fmt == "json" ? BenchmarkConfig::OutputFormat::JSON : BenchmarkConfig::OutputFormat::CSV);
    // default series/bench outputs under results/csvs
    string def_out  = (outfmt == BenchmarkConfig::OutputFormat::CSV ? "results/csvs/benchmark_results.csv"
                                                                    : "results/csvs/benchmark_results.json");
    string out_path = prompt_line(std::string("Output file (blank=skip, default=") + def_out + ")", def_out);
    if (out_path == "skip" || out_path == "none")
        out_path.clear();

    // HashMap tuning
    string       hs     = prompt_line("Hash strategy [open|chain]", "open");
    HashStrategy hstrat = (hs == "chain" ? HashStrategy::SEPARATE_CHAINING : HashStrategy::OPEN_ADDRESSING);
    string       hcap   = prompt_line("Hash initial capacity (blank=default)", "");
    std::optional<std::size_t> hash_cap;
    if (!hcap.empty())
        hash_cap = static_cast<std::size_t>(std::stoull(hcap));
    string                hload = prompt_line("Hash max load factor (blank=default)", "");
    std::optional<double> hash_load;
    if (!hload.empty())
        hash_load = std::stod(hload);

    BenchmarkSuite  suite;
    BenchmarkConfig cfg;
    cfg.size                  = max_size; // updated per iteration if multi-size
    cfg.runs                  = runs_per_size;
    cfg.structures            = structures;
    cfg.pattern               = pat;
    cfg.seed                  = seed;
    cfg.output_format         = outfmt;
    cfg.hash_strategy         = hstrat;
    cfg.hash_initial_capacity = hash_cap;
    cfg.hash_max_load_factor  = hash_load;
    if (!out_path.empty())
        cfg.csv_output = out_path;

    if (!crossover) {
        if (size_count <= 1) {
            auto results = suite.run(cfg);
            cout << "\n=== Benchmark Results (avg ms over " << runs_per_size << ", size=" << max_size << ") ===\n";
            for (const auto& r : results) {
                cout << "- " << r.structure << ": insert=" << r.insert_ms_mean << ", search=" << r.search_ms_mean
                     << ", remove=" << r.remove_ms_mean << ", mem=" << r.memory_bytes << " bytes\n";
            }
            if (cfg.csv_output) {
                cout << "\nSaved " << (outfmt == BenchmarkConfig::OutputFormat::CSV ? "CSV" : "JSON")
                     << " to: " << *cfg.csv_output << "\n";
            }
            return results.empty() ? 1 : 0;
        } else {
            // Generate linearly spaced sizes: e.g. count=4, max=10000 -> 2500,5000,7500,10000
            std::vector<std::size_t> sizes;
            double                   step = static_cast<double>(max_size) / size_count;
            for (int i = 1; i <= size_count; ++i)
                sizes.push_back(static_cast<std::size_t>(std::llround(step * i)));
            BenchmarkSuite::Series series;
            for (auto s : sizes) {
                cfg.size = s;
                auto res = suite.run(cfg);
                // Print results for this size inline
                cout << "\n-- Size " << s << " --\n";
                for (const auto& r : res) {
                    cout << r.structure << ": insert=" << r.insert_ms_mean << ", search=" << r.search_ms_mean
                         << ", remove=" << r.remove_ms_mean << ", mem=" << r.memory_bytes << " bytes\n";
                }
                for (const auto& r : res)
                    series.push_back({s, r.structure, r.insert_ms_mean, r.search_ms_mean, r.remove_ms_mean});
            }
            cout << "\n=== Multi-Size Benchmark Series (runs per size=" << runs_per_size << ") ===\n";
            for (auto s : sizes)
                cout << " size=" << s;
            cout << "\n";
            // Summarize last size results (already captured) and mention series file
            if (cfg.csv_output) {
                std::string path = *cfg.csv_output;
                if (outfmt == BenchmarkConfig::OutputFormat::CSV)
                    suite.write_series_csv(path, series);
                else
                    suite.write_series_json(path, series, cfg);
                cout << "\nSaved series " << (outfmt == BenchmarkConfig::OutputFormat::CSV ? "CSV" : "JSON")
                     << " to: " << path << "\n";
                // Offer to generate plots
                if (outfmt == BenchmarkConfig::OutputFormat::CSV) {
                    bool gen = prompt_yesno("Generate series plots now?", true);
                    if (gen) {
                        // ensure results/plots exists
                        std::string cmd = "python3 scripts/plot_results.py --series-csv '" + path +
                                          "' --out-dir results/plots --yscale auto --note 'wizard series'";
                        int rc = std::system(cmd.c_str());
                        if (rc != 0)
                            std::cout << "[WARN] Plotting command failed; ensure Python/matplotlib are installed.\n";
                    }
                } else {
                    std::cout << "[INFO] Skipping plots: plotting supports CSV only.\n";
                }
            } else {
                // Print a brief table if no output file
                for (const auto& p : series) {
                    cout << p.size << ": " << p.structure << " ins=" << p.insert_ms << " sea=" << p.search_ms
                         << " rem=" << p.remove_ms << "\n";
                }
            }
            return series.empty() ? 1 : 0;
        }
    }

    // Crossover path
    string                maxsz_s   = prompt_line("Max size (sweep)", "100000");
    std::size_t           maxsz     = static_cast<std::size_t>(std::stoull(maxsz_s));
    string                srun_s    = prompt_line("Series runs per size", "1");
    int                   srun      = std::stoi(srun_s);
    string                tbudget_s = prompt_line("Time budget seconds (blank=no cap)", "");
    std::optional<double> tbudget;
    if (!tbudget_s.empty())
        tbudget = std::stod(tbudget_s);
    cfg.runs = srun;

    // Default crossover output name if none
    if (!cfg.csv_output) {
        std::string defcx = (outfmt == BenchmarkConfig::OutputFormat::CSV ? "results/csvs/crossover_results.csv"
                                                                          : "results/csvs/crossover_results.json");
        std::string cxout = prompt_line(std::string("Crossover output file (blank=default= ") + defcx + ")", defcx);
        if (!cxout.empty())
            cfg.csv_output = cxout; // reuse same member
    }

    std::vector<std::size_t> sizes;
    for (std::size_t s = 512; s <= maxsz; s *= 2)
        sizes.push_back(s);
    auto                   start = std::chrono::steady_clock::now();
    BenchmarkSuite::Series series;
    for (auto s : sizes) {
        cfg.size = s;
        auto res = suite.run(cfg);
        for (const auto& r : res)
            series.push_back({s, r.structure, r.insert_ms_mean, r.search_ms_mean, r.remove_ms_mean});
        if (tbudget) {
            double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
            if (elapsed >= *tbudget) {
                std::cout << "[INFO] Stopping early due to time budget" << std::endl;
                break;
            }
        }
    }
    auto cx = suite.compute_crossovers(series);
    cout << "\n=== Crossover Analysis (approximate sizes) ===\n";
    cout << "(runs per size: " << srun << ")\n";
    for (const auto& c : cx)
        cout << c.operation << ": " << c.a << " vs " << c.b << " -> ~" << c.size_at_crossover << " elements\n";
    if (cfg.csv_output) {
        if (outfmt == BenchmarkConfig::OutputFormat::CSV)
            suite.write_crossover_csv(*cfg.csv_output, cx);
        else
            suite.write_crossover_json(*cfg.csv_output, cx, cfg);
        cout << "\nSaved crossover " << (outfmt == BenchmarkConfig::OutputFormat::CSV ? "CSV" : "JSON")
             << " to: " << *cfg.csv_output << "\n";
    }
    return cx.empty() ? 1 : 0;
}

// ---------------------------------------------------------------------------
// run_op_tests
// ---------------------------------------------------------------------------

int run_op_tests(const std::vector<std::string>& names, std::size_t size) {
    using namespace std;
    cout << "\n=== Operation Tests (size=" << size << ") ===\n";
    for (const auto& name : names) {
        cout << name << ":\n";
        auto ds = hashbrowns::DataStructurePtr();
        // local make to avoid depending on benchmark_suite internals
        if (name == "array" || name == "dynamic-array") {
            ds = std::make_unique<hashbrowns::DynamicArray<std::pair<int, std::string>>>();
        } else if (name == "slist" || name == "list" || name == "singly-list") {
            ds = std::make_unique<hashbrowns::SinglyLinkedList<std::pair<int, std::string>>>();
        } else if (name == "dlist" || name == "doubly-list") {
            ds = std::make_unique<hashbrowns::DoublyLinkedList<std::pair<int, std::string>>>();
        } else if (name == "hashmap" || name == "hash-map") {
            ds = std::make_unique<hashbrowns::HashMap>(hashbrowns::HashStrategy::OPEN_ADDRESSING);
        }
        if (!ds) {
            cout << "  (unknown structure)\n";
            continue;
        }
        vector<int> keys(size);
        for (size_t i = 0; i < size; ++i)
            keys[i] = static_cast<int>(i);
        hashbrowns::Timer t;
        std::string       v;
        // Insert
        t.start();
        for (auto k : keys)
            ds->insert(k, to_string(k));
        auto ins_us = t.stop().count();
        // Search (verify)
        size_t found = 0;
        t.start();
        for (auto k : keys) {
            if (ds->search(k, v))
                ++found;
        }
        auto sea_us = t.stop().count();
        // Remove
        size_t removed = 0;
        t.start();
        for (auto k : keys) {
            if (ds->remove(k))
                ++removed;
        }
        auto rem_us = t.stop().count();
        cout << "  insert: " << (ins_us / 1000.0) << " ms, count=" << keys.size() << "\n";
        cout << "  search: " << (sea_us / 1000.0) << " ms, found=" << found << "/" << keys.size() << "\n";
        cout << "  remove: " << (rem_us / 1000.0) << " ms, removed=" << removed << "/" << keys.size() << "\n";
    }
    return 0;
}

} // namespace cli
} // namespace hashbrowns
