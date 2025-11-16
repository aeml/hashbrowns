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
#include <thread>
#include <sstream>
#include <cctype>

#if defined(__unix__) || defined(__APPLE__)
#include <sys/utsname.h>
#endif
#ifdef __APPLE__
#include <sys/sysctl.h>
#endif
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

namespace hashbrowns {

static std::string read_cpu_governor() {
#ifdef __linux__
    std::ifstream f("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
    std::string g; if (f) std::getline(f, g); return g;
#else
    return "unknown";
#endif
}

[[maybe_unused]] static bool set_cpu_affinity(int cpu_index) {
#ifdef __linux__
    cpu_set_t set; CPU_ZERO(&set); CPU_SET(cpu_index, &set);
    return sched_setaffinity(0, sizeof(set), &set) == 0;
#elif defined(_WIN32)
    DWORD_PTR mask = (cpu_index >= 0 && cpu_index < (int)(8 * sizeof(DWORD_PTR)))
        ? (DWORD_PTR(1) << cpu_index)
        : 1;
    HANDLE h = GetCurrentThread();
    return SetThreadAffinityMask(h, mask) != 0;
#else
    (void)cpu_index; return false;
#endif
}

#ifdef __linux__
[[maybe_unused]] static bool disable_turbo_linux() {
    std::ofstream turbo("/sys/devices/system/cpu/intel_pstate/no_turbo");
    if (turbo) { turbo << "1"; return true; }
    std::ofstream turbo2("/sys/devices/system/cpu/cpufreq/boost");
    if (turbo2) { turbo2 << "0"; return true; }
    return false;
}
#else
[[maybe_unused]] static bool disable_turbo_linux() { return false; }
#endif

static std::string git_commit_sha() {
#ifdef _WIN32
    // Simplest fallback on Windows to avoid _popen portability issues
    return "unknown";
#else
    FILE* pipe = popen("git rev-parse --short HEAD 2>/dev/null", "r");
    if (!pipe) return "unknown";
    char buf[64]; std::string out; if (fgets(buf, sizeof(buf), pipe)) out = buf; pclose(pipe);
    if (!out.empty() && out.back()=='\n') out.pop_back();
    return out.empty()?"unknown":out;
#endif
}

static std::string cpu_model() {
#ifdef __linux__
    std::ifstream in("/proc/cpuinfo");
    std::string line;
    while (std::getline(in, line)) {
        auto pos = line.find(":");
        if (pos == std::string::npos) continue;
        auto key = line.substr(0, pos);
        if (key.find("model name") != std::string::npos) {
            std::string val = line.substr(pos + 1);
            // trim leading spaces
            val.erase(0, val.find_first_not_of(" \t"));
            return val;
        }
    }
    return "unknown";
#elif defined(__APPLE__)
    char buf[256]; size_t sz = sizeof(buf);
    if (sysctlbyname("machdep.cpu.brand_string", &buf, &sz, nullptr, 0) == 0) {
        return std::string(buf, strnlen(buf, sizeof(buf)));
    }
    return "unknown";
#else
    return "unknown";
#endif
}

static unsigned long long total_ram_bytes() {
#ifdef __linux__
    std::ifstream in("/proc/meminfo");
    std::string key, unit; unsigned long long val = 0ULL;
    while (in >> key >> val >> unit) {
        if (key == "MemTotal:") {
            if (unit == "kB") return val * 1024ULL;
            return val; // bytes if unit not present
        }
    }
    return 0ULL;
#elif defined(__APPLE__)
    uint64_t mem = 0; size_t sz = sizeof(mem);
    if (sysctlbyname("hw.memsize", &mem, &sz, nullptr, 0) == 0) return static_cast<unsigned long long>(mem);
    return 0ULL;
#else
    return 0ULL;
#endif
}

static std::string kernel_version() {
#if defined(__unix__) || defined(__APPLE__)
    struct utsname u{};
    if (uname(&u) == 0) {
        std::ostringstream oss; oss << u.sysname << " " << u.release;
        return oss.str();
    }
    return "unknown";
#else
    return "unknown";
#endif
}

static const char* cpp_standard_str() {
#if __cplusplus >= 202302L
    return "C++23+";
#elif __cplusplus >= 202002L
    return "C++20";
#elif __cplusplus >= 201703L
    return "C++17";
#elif __cplusplus >= 201402L
    return "C++14";
#elif __cplusplus >= 201103L
    return "C++11";
#else
    return "pre-C++11";
#endif
}

static const char* build_type_str() {
#ifdef HASHBROWNS_BUILD_TYPE
    return HASHBROWNS_BUILD_TYPE;
#else
    return "Unknown";
#endif
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
                            [[maybe_unused]] const BenchmarkConfig& cfg,
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
    out << "    \"schema_version\": 1,\n";
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
    // Portable compiler version string
    auto compiler_version = []() -> std::string {
#if defined(__clang__)
    return std::string("Clang ") + __clang_version__;
#elif defined(_MSC_VER)
    // _MSC_FULL_VER is an integer, build a string like "MSVC 19.x"
    return std::string("MSVC ") + std::to_string(_MSC_FULL_VER);
#elif defined(__GNUC__)
    return std::string("GCC ") + __VERSION__;
#else
    return std::string("unknown");
#endif
    }();
    out << "    \"compiler\": \"" << compiler_version << "\",\n";
    out << "    \"cpp_standard\": \"" << cpp_standard_str() << "\",\n";
    out << "    \"build_type\": \"" << build_type_str() << "\",\n";
    out << "    \"cpu_model\": \"" << cpu_model() << "\",\n";
    out << "    \"cores\": " << std::thread::hardware_concurrency() << ",\n";
    out << "    \"total_ram_bytes\": " << total_ram_bytes() << ",\n";
    out << "    \"kernel\": \"" << kernel_version() << "\",\n";
    out << "    \"hash_strategy\": \"" << to_string(config.hash_strategy) << "\"";
    if (config.hash_initial_capacity) out << ",\n    \"hash_capacity\": " << *config.hash_initial_capacity;
    if (config.hash_max_load_factor) out << ",\n    \"hash_load\": " << *config.hash_max_load_factor;
    out << ",\n    \"pinned_cpu\": " << (config.pin_cpu ? config.pin_cpu_index : -1);
    out << ",\n    \"turbo_disabled\": " << (config.disable_turbo ? 1 : 0);
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
    out << "    \"schema_version\": 1,\n";
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
    out << "    \"schema_version\": 1,\n";
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

// --- Baseline loading and comparison helpers ---

// Very small, schema-aware JSON reader tailored to benchmark_results.json.
// It assumes the existing write_results_json layout and extracts only fields
// needed for baseline comparison.

static std::string trim(const std::string& s) {
    std::size_t start = 0, end = s.size();
    while (start < end && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
    while (end > start && std::isspace(static_cast<unsigned char>(s[end-1]))) --end;
    return s.substr(start, end - start);
}

std::vector<BenchmarkResult> load_benchmark_results_json(const std::string& path) {
    std::ifstream in(path);
    if (!in) return {};
    std::string json((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    std::vector<BenchmarkResult> out;

    std::size_t pos = json.find("\"results\"");
    if (pos == std::string::npos) return {};
    pos = json.find('[', pos);
    if (pos == std::string::npos) return {};
    ++pos;
    while (pos < json.size()) {
        pos = json.find('{', pos);
        if (pos == std::string::npos) break;
        auto end = json.find('}', pos);
        if (end == std::string::npos) break;
        std::string obj = json.substr(pos + 1, end - pos - 1);
        pos = end + 1;

        BenchmarkResult r;
        auto extract_double = [&obj](const char* key, double& target) {
            std::string pattern = std::string("\"") + key + "\"";
            auto kpos = obj.find(pattern);
            if (kpos == std::string::npos) return;
            kpos = obj.find(':', kpos);
            if (kpos == std::string::npos) return;
            ++kpos;
            std::size_t endv = obj.find_first_of(",}\n", kpos);
            std::string val = trim(obj.substr(kpos, endv - kpos));
            try { target = std::stod(val); } catch (...) {}
        };

        auto extract_size_t = [&obj](const char* key, std::size_t& target) {
            std::string pattern = std::string("\"") + key + "\"";
            auto kpos = obj.find(pattern);
            if (kpos == std::string::npos) return;
            kpos = obj.find(':', kpos);
            if (kpos == std::string::npos) return;
            ++kpos;
            std::size_t endv = obj.find_first_of(",}\n", kpos);
            std::string val = trim(obj.substr(kpos, endv - kpos));
            try { target = static_cast<std::size_t>(std::stoull(val)); } catch (...) {}
        };

        // structure name
        {
            std::string pattern = "\"structure\"";
            auto kpos = obj.find(pattern);
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

static double pct_delta(double baseline, double current) {
    if (baseline == 0.0) return 0.0;
    return (current - baseline) * 100.0 / baseline;
}

BaselineComparison compare_against_baseline(const std::vector<BenchmarkResult>& baseline,
                                            const std::vector<BenchmarkResult>& current,
                                            const BaselineConfig& cfg) {
    BaselineComparison out;
    if (baseline.empty() || current.empty()) return out;

    std::map<std::string, BenchmarkResult> base_map;
    for (const auto& b : baseline) base_map[b.structure] = b;

    for (const auto& cur : current) {
        auto it = base_map.find(cur.structure);
        if (it == base_map.end()) continue;
        const auto& b = it->second;
        BaselineComparison::Entry e;
        e.structure = cur.structure;

        // choose metrics based on scope
        auto pick = [&cfg](double mean, double p95, double ci_high) {
            switch (cfg.scope) {
                case BaselineConfig::MetricScope::MEAN: return mean;
                case BaselineConfig::MetricScope::P95: return p95;
                case BaselineConfig::MetricScope::CI_HIGH: return ci_high;
                case BaselineConfig::MetricScope::ANY: default: return mean;
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
            if (absd <= cfg.noise_floor_pct) return true;
            return delta <= cfg.threshold_pct;
        };

        e.insert_ok = within(e.insert_delta_pct);
        e.search_ok = within(e.search_delta_pct);
        e.remove_ok = within(e.remove_delta_pct);

        if (!e.insert_ok || !e.search_ok || !e.remove_ok) out.all_ok = false;
        out.entries.push_back(e);
    }
    return out;
}

void print_baseline_report(const BaselineComparison& report, double threshold_pct, double noise_floor_pct) {
    if (report.entries.empty()) {
        std::cout << "[baseline] No comparable structures between baseline and current results.\n";
        return;
    }
    std::cout << "[baseline] Threshold=" << threshold_pct << "% (noise floor=" << noise_floor_pct << "%)\n";
    for (const auto& e : report.entries) {
        auto status = (e.insert_ok && e.search_ok && e.remove_ok) ? "OK" : "FAIL";
        std::cout << "  " << status << "  " << e.structure
                  << "  insert=" << e.insert_delta_pct << "%"
                  << "  search=" << e.search_delta_pct << "%"
                  << "  remove=" << e.remove_delta_pct << "%" << "\n";
    }
    if (report.all_ok) {
        std::cout << "[baseline] All metrics within tolerance." << std::endl;
    } else {
        std::cout << "[baseline] Performance regression detected." << std::endl;
    }
}

} // namespace hashbrowns