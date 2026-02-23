// benchmark_io.cpp
// Serialization helpers (CSV/JSON write) and environment metadata capture for
// BenchmarkSuite.  Extracted from benchmark_suite.cpp to keep the run-loop
// translation unit focused on timing logic.

#include "benchmark_suite.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <sstream>
#include <thread>

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

// ---------------------------------------------------------------------------
// Environment metadata helpers (file-local)
// ---------------------------------------------------------------------------

static std::string read_cpu_governor() {
#ifdef __linux__
    std::ifstream f("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
    std::string   g;
    if (f)
        std::getline(f, g);
    return g;
#else
    return "unknown";
#endif
}

static std::string git_commit_sha() {
#ifdef _WIN32
    return "unknown";
#else
    FILE* pipe = popen("git rev-parse --short HEAD 2>/dev/null", "r");
    if (!pipe)
        return "unknown";
    char        buf[64];
    std::string out;
    if (fgets(buf, sizeof(buf), pipe))
        out = buf;
    pclose(pipe);
    if (!out.empty() && out.back() == '\n')
        out.pop_back();
    return out.empty() ? "unknown" : out;
#endif
}

static std::string cpu_model() {
#ifdef __linux__
    std::ifstream in("/proc/cpuinfo");
    std::string   line;
    while (std::getline(in, line)) {
        auto pos = line.find(":");
        if (pos == std::string::npos)
            continue;
        auto key = line.substr(0, pos);
        if (key.find("model name") != std::string::npos) {
            std::string val = line.substr(pos + 1);
            val.erase(0, val.find_first_not_of(" \t"));
            return val;
        }
    }
    return "unknown";
#elif defined(__APPLE__)
    char   buf[256];
    size_t sz = sizeof(buf);
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
    std::ifstream      in("/proc/meminfo");
    std::string        key, unit;
    unsigned long long val = 0ULL;
    while (in >> key >> val >> unit) {
        if (key == "MemTotal:") {
            if (unit == "kB")
                return val * 1024ULL;
            return val;
        }
    }
    return 0ULL;
#elif defined(__APPLE__)
    uint64_t mem = 0;
    size_t   sz  = sizeof(mem);
    if (sysctlbyname("hw.memsize", &mem, &sz, nullptr, 0) == 0)
        return static_cast<unsigned long long>(mem);
    return 0ULL;
#else
    return 0ULL;
#endif
}

static std::string kernel_version() {
#if defined(__unix__) || defined(__APPLE__)
    struct utsname u{};
    if (uname(&u) == 0) {
        std::ostringstream oss;
        oss << u.sysname << " " << u.release;
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

// ---------------------------------------------------------------------------
// Pattern / HashStrategy -> string helpers (file-local)
// ---------------------------------------------------------------------------

static const char* to_string(BenchmarkConfig::Pattern p) {
    using P = BenchmarkConfig::Pattern;
    switch (p) {
    case P::SEQUENTIAL:
        return "sequential";
    case P::RANDOM:
        return "random";
    case P::MIXED:
        return "mixed";
    }
    return "sequential";
}

static const char* to_string(HashStrategy s) {
    switch (s) {
    case HashStrategy::OPEN_ADDRESSING:
        return "open";
    case HashStrategy::SEPARATE_CHAINING:
        return "chain";
    }
    return "open";
}

// ---------------------------------------------------------------------------
// write_results_csv  (file-local helper called from BenchmarkSuite::run)
// ---------------------------------------------------------------------------

void write_results_csv_impl(const std::string& path, const std::vector<BenchmarkResult>& results,
                            [[maybe_unused]] const BenchmarkConfig& cfg, unsigned long long actual_seed) {
    std::ofstream out(path);
    if (!out)
        return;
    out << "structure,seed,insert_ms_mean,insert_ms_stddev,insert_ms_median,insert_ms_p95,insert_ci_low,insert_ci_"
           "high,";
    out << "search_ms_mean,search_ms_stddev,search_ms_median,search_ms_p95,search_ci_low,search_ci_high,";
    out << "remove_ms_mean,remove_ms_stddev,remove_ms_median,remove_ms_p95,remove_ci_low,remove_ci_high,";
    out << "memory_bytes,memory_insert_mean,memory_insert_stddev,memory_search_mean,memory_search_stddev,memory_remove_"
           "mean,memory_remove_stddev,";
    out << "insert_probes_mean,insert_probes_stddev,search_probes_mean,search_probes_stddev,remove_probes_mean,remove_"
           "probes_stddev\n";
    for (const auto& r : results) {
        out << r.structure << "," << actual_seed << "," << r.insert_ms_mean << "," << r.insert_ms_stddev << ","
            << r.insert_ms_median << "," << r.insert_ms_p95 << "," << r.insert_ci_low << "," << r.insert_ci_high << ","
            << r.search_ms_mean << "," << r.search_ms_stddev << "," << r.search_ms_median << "," << r.search_ms_p95
            << "," << r.search_ci_low << "," << r.search_ci_high << "," << r.remove_ms_mean << "," << r.remove_ms_stddev
            << "," << r.remove_ms_median << "," << r.remove_ms_p95 << "," << r.remove_ci_low << "," << r.remove_ci_high
            << "," << r.memory_bytes << "," << r.memory_insert_bytes_mean << "," << r.memory_insert_bytes_stddev << ","
            << r.memory_search_bytes_mean << "," << r.memory_search_bytes_stddev << "," << r.memory_remove_bytes_mean
            << "," << r.memory_remove_bytes_stddev << "," << r.insert_probes_mean << "," << r.insert_probes_stddev
            << "," << r.search_probes_mean << "," << r.search_probes_stddev << "," << r.remove_probes_mean << ","
            << r.remove_probes_stddev << "\n";
    }
}

// ---------------------------------------------------------------------------
// write_results_json  (file-local helper called from BenchmarkSuite::run)
// ---------------------------------------------------------------------------

void write_results_json_impl(const std::string& path, const std::vector<BenchmarkResult>& results,
                             const BenchmarkConfig& config, unsigned long long actual_seed) {
    std::ofstream out(path);
    if (!out)
        return;
    out << "{\n";
    out << "  \"meta\": {\n";
    out << "    \"schema_version\": 1,\n";
    out << "    \"size\": " << config.size << ",\n";
    out << "    \"runs\": " << config.runs << ",\n";
    out << "    \"warmup_runs\": " << config.warmup_runs << ",\n";
    out << "    \"bootstrap_iters\": " << config.bootstrap_iters << ",\n";
    out << "    \"structures\": [";
    for (std::size_t i = 0; i < config.structures.size(); ++i) {
        out << "\"" << config.structures[i] << "\"";
        if (i + 1 < config.structures.size())
            out << ",";
    }
    out << "],\n";
    out << "    \"pattern\": \"" << to_string(config.pattern) << "\",\n";
    out << "    \"seed\": " << actual_seed << ",\n";
    out << "    \"timestamp\": \"";
    {
        std::time_t t = std::time(nullptr);
        char        buf[64];
        std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&t));
        out << buf;
    }
    out << "\",\n";
    out << "    \"cpu_governor\": \"" << read_cpu_governor() << "\",\n";
    out << "    \"git_commit\": \"" << git_commit_sha() << "\",\n";
    auto compiler_version = []() -> std::string {
#if defined(__clang__)
        return std::string("Clang ") + __clang_version__;
#elif defined(_MSC_VER)
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
    if (config.hash_initial_capacity)
        out << ",\n    \"hash_capacity\": " << *config.hash_initial_capacity;
    if (config.hash_max_load_factor)
        out << ",\n    \"hash_load\": " << *config.hash_max_load_factor;
    out << ",\n    \"pinned_cpu\": " << (config.pin_cpu ? config.pin_cpu_index : -1);
    out << ",\n    \"turbo_disabled\": " << (config.disable_turbo ? 1 : 0);
    out << "\n  },\n";
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
            << "\"remove_probes_stddev\": " << r.remove_probes_stddev << "}" << (i + 1 < results.size() ? "," : "")
            << "\n";
    }
    out << "  ]\n}\n";
}

// ---------------------------------------------------------------------------
// BenchmarkSuite public serialization methods
// ---------------------------------------------------------------------------

void BenchmarkSuite::write_crossover_csv(const std::string& path, const std::vector<CrossoverInfo>& info) {
    std::ofstream out(path);
    if (!out)
        return;
    out << "operation,a,b,size_at_crossover\n";
    for (const auto& c : info) {
        out << c.operation << "," << c.a << "," << c.b << "," << c.size_at_crossover << "\n";
    }
}

void BenchmarkSuite::write_crossover_json(const std::string& path, const std::vector<CrossoverInfo>& info,
                                          const BenchmarkConfig& config) {
    std::ofstream out(path);
    if (!out)
        return;
    out << "{\n";
    out << "  \"meta\": {\n";
    out << "    \"schema_version\": 1,\n";
    out << "    \"runs\": " << config.runs << ",\n";
    out << "    \"structures\": [";
    for (std::size_t i = 0; i < config.structures.size(); ++i) {
        out << "\"" << config.structures[i] << "\"";
        if (i + 1 < config.structures.size())
            out << ",";
    }
    out << "],\n";
    out << "    \"pattern\": \"" << to_string(config.pattern) << "\"";
    if (config.seed.has_value())
        out << ",\n    \"seed\": " << *config.seed;
    out << "\n  },\n";
    out << "  \"crossovers\": [\n";
    for (std::size_t i = 0; i < info.size(); ++i) {
        const auto& c = info[i];
        out << "    {"
            << "\"operation\": \"" << c.operation << "\","
            << "\"a\": \"" << c.a << "\","
            << "\"b\": \"" << c.b << "\","
            << "\"size_at_crossover\": " << c.size_at_crossover << "}" << (i + 1 < info.size() ? "," : "") << "\n";
    }
    out << "  ]\n}\n";
}

void BenchmarkSuite::write_series_csv(const std::string& path, const Series& series) {
    std::ofstream out(path);
    if (!out)
        return;
    out << "size,structure,insert_ms,search_ms,remove_ms\n";
    for (const auto& p : series) {
        out << p.size << "," << p.structure << "," << p.insert_ms << "," << p.search_ms << "," << p.remove_ms << "\n";
    }
}

void BenchmarkSuite::write_series_json(const std::string& path, const Series& series, const BenchmarkConfig& config) {
    std::ofstream out(path);
    if (!out)
        return;
    out << "{\n";
    out << "  \"meta\": {\n";
    out << "    \"schema_version\": 1,\n";
    out << "    \"runs_per_size\": " << config.runs << ",\n";
    out << "    \"structures\": [";
    for (std::size_t i = 0; i < config.structures.size(); ++i) {
        out << "\"" << config.structures[i] << "\"";
        if (i + 1 < config.structures.size())
            out << ",";
    }
    out << "],\n";
    out << "    \"pattern\": \"" << to_string(config.pattern) << "\"";
    if (config.seed.has_value())
        out << ",\n    \"seed\": " << *config.seed;
    out << "\n  },\n";
    out << "  \"series\": [\n";
    for (std::size_t i = 0; i < series.size(); ++i) {
        const auto& p = series[i];
        out << "    {\"size\": " << p.size << ", \"structure\": \"" << p.structure << "\""
            << ", \"insert_ms\": " << p.insert_ms << ", \"search_ms\": " << p.search_ms
            << ", \"remove_ms\": " << p.remove_ms << "}" << (i + 1 < series.size() ? "," : "") << "\n";
    }
    out << "  ]\n}\n";
}

} // namespace hashbrowns
