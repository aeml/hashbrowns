// benchmark_baseline.cpp
// Baseline loading, comparison, and reporting helpers for BenchmarkSuite.
// Extracted from benchmark_suite.cpp to isolate schema-aware JSON parsing
// and regression-detection logic from the timing run loop.

#include "benchmark_suite.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace hashbrowns {

namespace {

static std::string trim_ws(const std::string& s) {
    std::size_t start = 0;
    std::size_t end   = s.size();
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

static std::optional<std::size_t> find_key_value_start(const std::string& text, const std::string& key) {
    const std::string pattern = std::string("\"") + key + "\"";
    auto              pos     = text.find(pattern);
    if (pos == std::string::npos)
        return std::nullopt;
    pos = text.find(':', pos + pattern.size());
    if (pos == std::string::npos)
        return std::nullopt;
    ++pos;
    while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos])))
        ++pos;
    return pos;
}

static std::optional<std::size_t> find_matching_delim(const std::string& text, std::size_t start, char open, char close) {
    if (start >= text.size() || text[start] != open)
        return std::nullopt;
    int  depth     = 0;
    bool in_string = false;
    bool escaped   = false;
    for (std::size_t i = start; i < text.size(); ++i) {
        char ch = text[i];
        if (in_string) {
            if (escaped) {
                escaped = false;
            } else if (ch == '\\') {
                escaped = true;
            } else if (ch == '"') {
                in_string = false;
            }
            continue;
        }
        if (ch == '"') {
            in_string = true;
            continue;
        }
        if (ch == open) {
            ++depth;
        } else if (ch == close) {
            --depth;
            if (depth == 0)
                return i;
        }
    }
    return std::nullopt;
}

static std::optional<std::string> extract_object_by_key(const std::string& text, const std::string& key) {
    auto start = find_key_value_start(text, key);
    if (!start || *start >= text.size() || text[*start] != '{')
        return std::nullopt;
    auto end = find_matching_delim(text, *start, '{', '}');
    if (!end)
        return std::nullopt;
    return text.substr(*start + 1, *end - *start - 1);
}

static std::optional<std::string> extract_array_by_key(const std::string& text, const std::string& key) {
    auto start = find_key_value_start(text, key);
    if (!start || *start >= text.size() || text[*start] != '[')
        return std::nullopt;
    auto end = find_matching_delim(text, *start, '[', ']');
    if (!end)
        return std::nullopt;
    return text.substr(*start + 1, *end - *start - 1);
}

static std::optional<std::string> extract_string_field(const std::string& obj, const std::string& key) {
    auto start = find_key_value_start(obj, key);
    if (!start || *start >= obj.size() || obj[*start] != '"')
        return std::nullopt;
    std::string out;
    bool        escaped = false;
    for (std::size_t i = *start + 1; i < obj.size(); ++i) {
        char ch = obj[i];
        if (escaped) {
            out.push_back(ch);
            escaped = false;
            continue;
        }
        if (ch == '\\') {
            escaped = true;
            continue;
        }
        if (ch == '"')
            return out;
        out.push_back(ch);
    }
    return std::nullopt;
}

static std::optional<std::string> extract_scalar_field(const std::string& obj, const std::string& key) {
    auto start = find_key_value_start(obj, key);
    if (!start)
        return std::nullopt;
    std::size_t end = *start;
    while (end < obj.size() && obj[end] != ',' && obj[end] != '}' && obj[end] != ']' && obj[end] != '\n')
        ++end;
    auto value = trim_ws(obj.substr(*start, end - *start));
    if (value.empty())
        return std::nullopt;
    return value;
}

static std::optional<int> extract_int_field(const std::string& obj, const std::string& key) {
    auto value = extract_scalar_field(obj, key);
    if (!value)
        return std::nullopt;
    try {
        return std::stoi(*value);
    } catch (...) {
        return std::nullopt;
    }
}

static std::optional<unsigned int> extract_uint_field(const std::string& obj, const std::string& key) {
    auto value = extract_scalar_field(obj, key);
    if (!value)
        return std::nullopt;
    try {
        return static_cast<unsigned int>(std::stoul(*value));
    } catch (...) {
        return std::nullopt;
    }
}

static std::optional<std::size_t> extract_size_t_field(const std::string& obj, const std::string& key) {
    auto value = extract_scalar_field(obj, key);
    if (!value)
        return std::nullopt;
    try {
        return static_cast<std::size_t>(std::stoull(*value));
    } catch (...) {
        return std::nullopt;
    }
}

static std::optional<unsigned long long> extract_ull_field(const std::string& obj, const std::string& key) {
    auto value = extract_scalar_field(obj, key);
    if (!value)
        return std::nullopt;
    try {
        return static_cast<unsigned long long>(std::stoull(*value));
    } catch (...) {
        return std::nullopt;
    }
}

static std::optional<double> extract_double_field(const std::string& obj, const std::string& key) {
    auto value = extract_scalar_field(obj, key);
    if (!value)
        return std::nullopt;
    try {
        return std::stod(*value);
    } catch (...) {
        return std::nullopt;
    }
}

static std::vector<std::string> extract_string_array_field(const std::string& obj, const std::string& key) {
    std::vector<std::string> out;
    auto                     start = find_key_value_start(obj, key);
    if (!start || *start >= obj.size() || obj[*start] != '[')
        return out;
    auto end = find_matching_delim(obj, *start, '[', ']');
    if (!end)
        return out;
    std::string array_body = obj.substr(*start + 1, *end - *start - 1);
    std::size_t pos        = 0;
    while (pos < array_body.size()) {
        auto quote = array_body.find('"', pos);
        if (quote == std::string::npos)
            break;
        auto close = array_body.find('"', quote + 1);
        if (close == std::string::npos)
            break;
        out.push_back(array_body.substr(quote + 1, close - quote - 1));
        pos = close + 1;
    }
    return out;
}

static bool optional_string_equal(const std::optional<std::string>& a, const std::optional<std::string>& b) {
    if (a.has_value() != b.has_value())
        return false;
    if (!a.has_value())
        return true;
    return *a == *b;
}

static std::vector<std::string> compare_string_vectors(const std::vector<std::string>& a, const std::vector<std::string>& b) {
    if (a == b)
        return {};
    std::vector<std::string> rendered;
    auto                     render = [](const std::vector<std::string>& values) {
        std::string out = "[";
        for (std::size_t i = 0; i < values.size(); ++i) {
            out += values[i];
            if (i + 1 < values.size())
                out += ",";
        }
        out += "]";
        return out;
    };
    rendered.push_back(render(a));
    rendered.push_back(render(b));
    return rendered;
}

static bool optional_size_t_equal(const std::optional<std::size_t>& a, const std::optional<std::size_t>& b) {
    if (a.has_value() != b.has_value())
        return false;
    if (!a.has_value())
        return true;
    return *a == *b;
}

static bool optional_ull_equal(const std::optional<unsigned long long>& a, const std::optional<unsigned long long>& b) {
    if (a.has_value() != b.has_value())
        return false;
    if (!a.has_value())
        return true;
    return *a == *b;
}

static bool optional_double_equal(const std::optional<double>& a, const std::optional<double>& b) {
    if (a.has_value() != b.has_value())
        return false;
    if (!a.has_value())
        return true;
    return std::fabs(*a - *b) < 1e-9;
}

} // namespace

BenchmarkData load_benchmark_data_json(const std::string& path) {
    std::ifstream in(path);
    if (!in)
        return {};

    std::string json((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    BenchmarkData data;

    if (auto meta_obj = extract_object_by_key(json, "meta")) {
        auto& meta = data.meta;
        if (auto v = extract_int_field(*meta_obj, "schema_version"))
            meta.schema_version = *v;
        if (auto v = extract_size_t_field(*meta_obj, "size"))
            meta.size = *v;
        if (auto v = extract_int_field(*meta_obj, "runs"))
            meta.runs = *v;
        if (auto v = extract_int_field(*meta_obj, "warmup_runs"))
            meta.warmup_runs = *v;
        if (auto v = extract_int_field(*meta_obj, "bootstrap_iters"))
            meta.bootstrap_iters = *v;
        meta.structures = extract_string_array_field(*meta_obj, "structures");
        if (auto v = extract_string_field(*meta_obj, "pattern"))
            meta.pattern = *v;
        meta.seed = extract_ull_field(*meta_obj, "seed");
        if (auto v = extract_string_field(*meta_obj, "timestamp"))
            meta.timestamp = *v;
        if (auto v = extract_string_field(*meta_obj, "cpu_governor"))
            meta.cpu_governor = *v;
        if (auto v = extract_string_field(*meta_obj, "git_commit"))
            meta.git_commit = *v;
        if (auto v = extract_string_field(*meta_obj, "compiler"))
            meta.compiler = *v;
        if (auto v = extract_string_field(*meta_obj, "cpp_standard"))
            meta.cpp_standard = *v;
        if (auto v = extract_string_field(*meta_obj, "build_type"))
            meta.build_type = *v;
        if (auto v = extract_string_field(*meta_obj, "cpu_model"))
            meta.cpu_model = *v;
        if (auto v = extract_string_field(*meta_obj, "profile"))
            meta.profile = *v;
        if (auto profile_manifest = extract_object_by_key(*meta_obj, "profile_manifest")) {
            if (auto v = extract_string_field(*profile_manifest, "selected_profile"))
                meta.profile_selected = *v;
            meta.profile_applied_defaults   = extract_string_array_field(*profile_manifest, "applied_defaults");
            meta.profile_explicit_overrides = extract_string_array_field(*profile_manifest, "explicit_overrides");
        }
        if (auto v = extract_uint_field(*meta_obj, "cores"))
            meta.cores = *v;
        if (auto v = extract_ull_field(*meta_obj, "total_ram_bytes"))
            meta.total_ram_bytes = *v;
        if (auto v = extract_string_field(*meta_obj, "kernel"))
            meta.kernel = *v;
        if (auto v = extract_string_field(*meta_obj, "hash_strategy"))
            meta.hash_strategy = *v;
        meta.hash_capacity = extract_size_t_field(*meta_obj, "hash_capacity");
        meta.hash_load     = extract_double_field(*meta_obj, "hash_load");
        if (auto v = extract_int_field(*meta_obj, "pinned_cpu"))
            meta.pinned_cpu = *v;
        if (auto v = extract_int_field(*meta_obj, "turbo_disabled"))
            meta.turbo_disabled = (*v != 0);
    }

    auto results_array = extract_array_by_key(json, "results");
    if (!results_array)
        return data;

    std::size_t pos = 0;
    while (pos < results_array->size()) {
        auto obj_start = results_array->find('{', pos);
        if (obj_start == std::string::npos)
            break;
        auto obj_end = find_matching_delim(*results_array, obj_start, '{', '}');
        if (!obj_end)
            break;
        std::string obj = results_array->substr(obj_start + 1, *obj_end - obj_start - 1);
        pos             = *obj_end + 1;

        BenchmarkResult r;
        if (auto v = extract_string_field(obj, "structure"))
            r.structure = *v;
        if (auto v = extract_double_field(obj, "insert_ms_mean"))
            r.insert_ms_mean = *v;
        if (auto v = extract_double_field(obj, "search_ms_mean"))
            r.search_ms_mean = *v;
        if (auto v = extract_double_field(obj, "remove_ms_mean"))
            r.remove_ms_mean = *v;
        if (auto v = extract_double_field(obj, "insert_ms_p95"))
            r.insert_ms_p95 = *v;
        if (auto v = extract_double_field(obj, "search_ms_p95"))
            r.search_ms_p95 = *v;
        if (auto v = extract_double_field(obj, "remove_ms_p95"))
            r.remove_ms_p95 = *v;
        if (auto v = extract_double_field(obj, "insert_ci_high"))
            r.insert_ci_high = *v;
        if (auto v = extract_double_field(obj, "search_ci_high"))
            r.search_ci_high = *v;
        if (auto v = extract_double_field(obj, "remove_ci_high"))
            r.remove_ci_high = *v;
        if (auto v = extract_size_t_field(obj, "memory_bytes"))
            r.memory_bytes = *v;

        if (!r.structure.empty())
            data.results.push_back(r);
    }

    return data;
}

std::vector<BenchmarkResult> load_benchmark_results_json(const std::string& path) {
    return load_benchmark_data_json(path).results;
}

BaselineMetadataReport compare_benchmark_metadata(const BenchmarkMeta& baseline, const BenchmarkMeta& current,
                                                  const BaselineConfig& cfg) {
    BaselineMetadataReport report;

    auto require_string_equal = [&report](const std::string& field, const std::string& a, const std::string& b) {
        if (a != b)
            report.errors.push_back(field + " mismatch: baseline='" + a + "' current='" + b + "'");
    };
    auto require_int_equal = [&report](const std::string& field, auto a, auto b) {
        if (a != b)
            report.errors.push_back(field + " mismatch: baseline='" + std::to_string(a) + "' current='" +
                                    std::to_string(b) + "'");
    };
    auto require_optional_ull_equal = [&report](const std::string& field, const std::optional<unsigned long long>& a,
                                                const std::optional<unsigned long long>& b) {
        if (!optional_ull_equal(a, b)) {
            report.errors.push_back(field + " mismatch: baseline='" + (a ? std::to_string(*a) : std::string("unset")) +
                                    "' current='" + (b ? std::to_string(*b) : std::string("unset")) + "'");
        }
    };
    auto require_optional_size_t_equal = [&report](const std::string& field, const std::optional<std::size_t>& a,
                                                   const std::optional<std::size_t>& b) {
        if (!optional_size_t_equal(a, b)) {
            report.errors.push_back(field + " mismatch: baseline='" + (a ? std::to_string(*a) : std::string("unset")) +
                                    "' current='" + (b ? std::to_string(*b) : std::string("unset")) + "'");
        }
    };
    auto require_optional_double_equal = [&report](const std::string& field, const std::optional<double>& a,
                                                   const std::optional<double>& b) {
        if (!optional_double_equal(a, b)) {
            report.errors.push_back(field + " mismatch: baseline='" + (a ? std::to_string(*a) : std::string("unset")) +
                                    "' current='" + (b ? std::to_string(*b) : std::string("unset")) + "'");
        }
    };
    auto warn_string_equal = [&report](const std::string& field, const std::string& a, const std::string& b) {
        if (a != b)
            report.warnings.push_back(field + " changed: baseline='" + a + "' current='" + b + "'");
    };
    auto warn_int_equal = [&report](const std::string& field, auto a, auto b) {
        if (a != b)
            report.warnings.push_back(field + " changed: baseline='" + std::to_string(a) + "' current='" +
                                      std::to_string(b) + "'");
    };

    require_int_equal("schema_version", baseline.schema_version, current.schema_version);
    require_int_equal("size", baseline.size, current.size);
    require_int_equal("runs", baseline.runs, current.runs);
    require_int_equal("warmup_runs", baseline.warmup_runs, current.warmup_runs);
    require_int_equal("bootstrap_iters", baseline.bootstrap_iters, current.bootstrap_iters);
    require_string_equal("profile", baseline.profile, current.profile);
    if (baseline.structures != current.structures) {
        auto rendered = compare_string_vectors(baseline.structures, current.structures);
        report.errors.push_back("structures mismatch: baseline='" + rendered[0] + "' current='" + rendered[1] + "'");
    }
    require_string_equal("pattern", baseline.pattern, current.pattern);
    require_optional_ull_equal("seed", baseline.seed, current.seed);
    require_string_equal("build_type", baseline.build_type, current.build_type);
    require_string_equal("hash_strategy", baseline.hash_strategy, current.hash_strategy);
    require_optional_size_t_equal("hash_capacity", baseline.hash_capacity, current.hash_capacity);
    require_optional_double_equal("hash_load", baseline.hash_load, current.hash_load);
    require_int_equal("pinned_cpu", baseline.pinned_cpu, current.pinned_cpu);
    require_int_equal("turbo_disabled", baseline.turbo_disabled ? 1 : 0, current.turbo_disabled ? 1 : 0);

    if (cfg.strict_profile_intent) {
        auto manifest_present = [](const BenchmarkMeta& meta) {
            return !meta.profile_applied_defaults.empty() || !meta.profile_explicit_overrides.empty() || meta.profile_selected != "custom";
        };
        const bool baseline_has_manifest = manifest_present(baseline);
        const bool current_has_manifest  = manifest_present(current);
        if (baseline_has_manifest != current_has_manifest) {
            report.errors.push_back("profile_manifest mismatch: baseline='" + std::string(baseline_has_manifest ? "present" : "missing") +
                                    "' current='" + std::string(current_has_manifest ? "present" : "missing") + "'");
        } else if (baseline_has_manifest && current_has_manifest) {
            require_string_equal("profile_selected", baseline.profile_selected, current.profile_selected);
            if (baseline.profile_applied_defaults != current.profile_applied_defaults) {
                auto rendered = compare_string_vectors(baseline.profile_applied_defaults, current.profile_applied_defaults);
                report.errors.push_back("profile_applied_defaults mismatch: baseline='" + rendered[0] + "' current='" + rendered[1] + "'");
            }
            if (baseline.profile_explicit_overrides != current.profile_explicit_overrides) {
                auto rendered = compare_string_vectors(baseline.profile_explicit_overrides, current.profile_explicit_overrides);
                report.errors.push_back("profile_explicit_overrides mismatch: baseline='" + rendered[0] + "' current='" + rendered[1] + "'");
            }
        }
    }

    warn_string_equal("cpu_model", baseline.cpu_model, current.cpu_model);
    warn_string_equal("compiler", baseline.compiler, current.compiler);
    warn_string_equal("cpp_standard", baseline.cpp_standard, current.cpp_standard);
    warn_string_equal("cpu_governor", baseline.cpu_governor, current.cpu_governor);
    warn_int_equal("cores", baseline.cores, current.cores);
    warn_int_equal("total_ram_bytes", baseline.total_ram_bytes, current.total_ram_bytes);
    warn_string_equal("kernel", baseline.kernel, current.kernel);

    report.ok = report.errors.empty();
    return report;
}

BaselineComparison compare_against_baseline(const std::vector<BenchmarkResult>& baseline,
                                            const std::vector<BenchmarkResult>& current, const BaselineConfig& cfg) {
    BaselineComparison out;

    auto scope_name = [&cfg]() {
        switch (cfg.scope) {
        case BaselineConfig::MetricScope::MEAN:
            return std::string("mean");
        case BaselineConfig::MetricScope::P95:
            return std::string("p95");
        case BaselineConfig::MetricScope::CI_HIGH:
            return std::string("ci_high");
        case BaselineConfig::MetricScope::ANY:
        default:
            return std::string("any");
        }
    };
    out.scope = scope_name();

    if (baseline.empty() || current.empty())
        return out;

    std::map<std::string, BenchmarkResult> base_map;
    std::set<std::string> duplicate_baseline_names;
    for (const auto& b : baseline) {
        if (base_map.find(b.structure) != base_map.end())
            duplicate_baseline_names.insert(b.structure);
        base_map[b.structure] = b;
    }

    std::set<std::string> current_names;
    std::set<std::string> duplicate_current_names;
    for (const auto& cur : current) {
        if (current_names.find(cur.structure) != current_names.end())
            duplicate_current_names.insert(cur.structure);
        current_names.insert(cur.structure);
    }

    out.coverage.baseline_structure_count = base_map.size();
    out.coverage.current_structure_count  = current_names.size();
    out.coverage.duplicate_baseline_structures.assign(duplicate_baseline_names.begin(), duplicate_baseline_names.end());
    out.coverage.duplicate_current_structures.assign(duplicate_current_names.begin(), duplicate_current_names.end());
    for (const auto& [name, _] : base_map) {
        if (current_names.find(name) == current_names.end())
            out.coverage.baseline_only_structures.push_back(name);
    }

    auto within = [&cfg](double delta) {
        double absd = std::fabs(delta);
        if (absd <= cfg.noise_floor_pct)
            return true;
        return delta <= cfg.threshold_pct;
    };

    auto select_metric = [&cfg, &within](double mean_delta, double p95_delta, double ci_high_delta) {
        struct MetricDecision {
            double delta_pct{0.0};
            bool ok{true};
            std::string basis{"mean"};
        };

        MetricDecision decision;
        switch (cfg.scope) {
        case BaselineConfig::MetricScope::MEAN:
            decision.delta_pct = mean_delta;
            decision.ok        = within(mean_delta);
            decision.basis     = "mean";
            return decision;
        case BaselineConfig::MetricScope::P95:
            decision.delta_pct = p95_delta;
            decision.ok        = within(p95_delta);
            decision.basis     = "p95";
            return decision;
        case BaselineConfig::MetricScope::CI_HIGH:
            decision.delta_pct = ci_high_delta;
            decision.ok        = within(ci_high_delta);
            decision.basis     = "ci_high";
            return decision;
        case BaselineConfig::MetricScope::ANY:
        default: {
            const bool mean_ok    = within(mean_delta);
            const bool p95_ok     = within(p95_delta);
            const bool ci_high_ok = within(ci_high_delta);
            decision.delta_pct = mean_delta;
            decision.ok        = mean_ok || p95_ok || ci_high_ok;
            decision.basis     = "any(mean=" + std::string(mean_ok ? "ok" : "fail") + ",p95=" +
                                 std::string(p95_ok ? "ok" : "fail") + ",ci_high=" +
                                 std::string(ci_high_ok ? "ok" : "fail") + ")";
            return decision;
        }
        }
    };

    for (const auto& cur : current) {
        auto it = base_map.find(cur.structure);
        if (it == base_map.end()) {
            if (std::find(out.coverage.current_only_structures.begin(), out.coverage.current_only_structures.end(), cur.structure) ==
                out.coverage.current_only_structures.end()) {
                out.coverage.current_only_structures.push_back(cur.structure);
            }
            continue;
        }
        ++out.coverage.comparable_structure_count;
        const auto&               b = it->second;
        BaselineComparison::Entry e;
        e.structure = cur.structure;

        const double insert_mean_delta    = pct_delta(b.insert_ms_mean, cur.insert_ms_mean);
        const double insert_p95_delta     = pct_delta(b.insert_ms_p95, cur.insert_ms_p95);
        const double insert_ci_high_delta = pct_delta(b.insert_ci_high, cur.insert_ci_high);
        e.insert_mean_delta_pct           = insert_mean_delta;
        e.insert_p95_delta_pct            = insert_p95_delta;
        e.insert_ci_high_delta_pct        = insert_ci_high_delta;
        e.insert_mean_ok                  = within(insert_mean_delta);
        e.insert_p95_ok                   = within(insert_p95_delta);
        e.insert_ci_high_ok               = within(insert_ci_high_delta);
        const auto insert_decision        = select_metric(insert_mean_delta, insert_p95_delta, insert_ci_high_delta);
        e.insert_delta_pct                = insert_decision.delta_pct;
        e.insert_ok                       = insert_decision.ok;
        e.insert_basis                    = insert_decision.basis;

        const double search_mean_delta    = pct_delta(b.search_ms_mean, cur.search_ms_mean);
        const double search_p95_delta     = pct_delta(b.search_ms_p95, cur.search_ms_p95);
        const double search_ci_high_delta = pct_delta(b.search_ci_high, cur.search_ci_high);
        e.search_mean_delta_pct           = search_mean_delta;
        e.search_p95_delta_pct            = search_p95_delta;
        e.search_ci_high_delta_pct        = search_ci_high_delta;
        e.search_mean_ok                  = within(search_mean_delta);
        e.search_p95_ok                   = within(search_p95_delta);
        e.search_ci_high_ok               = within(search_ci_high_delta);
        const auto search_decision        = select_metric(search_mean_delta, search_p95_delta, search_ci_high_delta);
        e.search_delta_pct                = search_decision.delta_pct;
        e.search_ok                       = search_decision.ok;
        e.search_basis                    = search_decision.basis;

        const double remove_mean_delta    = pct_delta(b.remove_ms_mean, cur.remove_ms_mean);
        const double remove_p95_delta     = pct_delta(b.remove_ms_p95, cur.remove_ms_p95);
        const double remove_ci_high_delta = pct_delta(b.remove_ci_high, cur.remove_ci_high);
        e.remove_mean_delta_pct           = remove_mean_delta;
        e.remove_p95_delta_pct            = remove_p95_delta;
        e.remove_ci_high_delta_pct        = remove_ci_high_delta;
        e.remove_mean_ok                  = within(remove_mean_delta);
        e.remove_p95_ok                   = within(remove_p95_delta);
        e.remove_ci_high_ok               = within(remove_ci_high_delta);
        const auto remove_decision        = select_metric(remove_mean_delta, remove_p95_delta, remove_ci_high_delta);
        e.remove_delta_pct                = remove_decision.delta_pct;
        e.remove_ok                       = remove_decision.ok;
        e.remove_basis                    = remove_decision.basis;

        auto append_failure = [&out, &cfg, &e](const std::string& operation, bool op_ok, const std::string& basis,
                                               double chosen_delta, bool mean_ok, bool p95_ok, bool ci_high_ok) {
            if (op_ok)
                return;
            BaselineComparison::Failure failure;
            failure.structure        = e.structure;
            failure.operation        = operation;
            failure.chosen_basis     = basis;
            failure.chosen_delta_pct = chosen_delta;
            failure.threshold_pct    = cfg.threshold_pct;
            if (!mean_ok)
                failure.failed_metric_families.push_back("mean");
            if (!p95_ok)
                failure.failed_metric_families.push_back("p95");
            if (!ci_high_ok)
                failure.failed_metric_families.push_back("ci_high");
            out.failures.push_back(failure);
        };

        append_failure("insert", e.insert_ok, e.insert_basis, e.insert_delta_pct,
                       e.insert_mean_ok, e.insert_p95_ok, e.insert_ci_high_ok);
        append_failure("search", e.search_ok, e.search_basis, e.search_delta_pct,
                       e.search_mean_ok, e.search_p95_ok, e.search_ci_high_ok);
        append_failure("remove", e.remove_ok, e.remove_basis, e.remove_delta_pct,
                       e.remove_mean_ok, e.remove_p95_ok, e.remove_ci_high_ok);

        if (!e.insert_ok || !e.search_ok || !e.remove_ok)
            out.all_ok = false;
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

void print_baseline_metadata_report(const BaselineMetadataReport& report) {
    if (report.errors.empty() && report.warnings.empty()) {
        std::cout << "[baseline-meta] Benchmark metadata is compatible." << std::endl;
        return;
    }
    for (const auto& warning : report.warnings)
        std::cout << "[baseline-meta] WARN  " << warning << std::endl;
    for (const auto& error : report.errors)
        std::cout << "[baseline-meta] ERROR " << error << std::endl;
    if (report.ok)
        std::cout << "[baseline-meta] Metadata is comparable with warnings." << std::endl;
    else
        std::cout << "[baseline-meta] Metadata mismatch invalidates comparison." << std::endl;
}

void write_baseline_report_json(const std::string& path, const BaselineReport& report) {
    std::ofstream out(path);
    if (!out)
        return;

    auto write_string_array = [&out](const std::vector<std::string>& values) {
        out << "[";
        for (std::size_t i = 0; i < values.size(); ++i) {
            out << "\"" << values[i] << "\"";
            if (i + 1 < values.size())
                out << ",";
        }
        out << "]";
    };

    out << "{\n";
    out << "  \"baseline_path\": \"" << report.baseline_path << "\",\n";
    out << "  \"scope\": \"" << report.scope << "\",\n";
    out << "  \"threshold_pct\": " << report.threshold_pct << ",\n";
    out << "  \"noise_floor_pct\": " << report.noise_floor_pct << ",\n";
    out << "  \"strict_profile_intent\": " << (report.strict_profile_intent ? "true" : "false") << ",\n";
    out << "  \"exit_code\": " << report.exit_code << ",\n";
    out << "  \"metadata\": {\n";
    out << "    \"ok\": " << (report.metadata.ok ? "true" : "false") << ",\n";
    out << "    \"errors\": ";
    write_string_array(report.metadata.errors);
    out << ",\n";
    out << "    \"warnings\": ";
    write_string_array(report.metadata.warnings);
    out << "\n  },\n";
    out << "  \"comparison\": {\n";
    out << "    \"all_ok\": " << (report.comparison.all_ok ? "true" : "false") << ",\n";
    out << "    \"decision_basis\": \"" << report.comparison.scope << "\",\n";
    out << "    \"coverage\": {\n";
    out << "      \"baseline_structure_count\": " << report.comparison.coverage.baseline_structure_count << ",\n";
    out << "      \"current_structure_count\": " << report.comparison.coverage.current_structure_count << ",\n";
    out << "      \"comparable_structure_count\": " << report.comparison.coverage.comparable_structure_count << ",\n";
    out << "      \"baseline_only_structures\": ";
    write_string_array(report.comparison.coverage.baseline_only_structures);
    out << ",\n";
    out << "      \"current_only_structures\": ";
    write_string_array(report.comparison.coverage.current_only_structures);
    out << ",\n";
    out << "      \"duplicate_baseline_structures\": ";
    write_string_array(report.comparison.coverage.duplicate_baseline_structures);
    out << ",\n";
    out << "      \"duplicate_current_structures\": ";
    write_string_array(report.comparison.coverage.duplicate_current_structures);
    out << "\n    },\n";
    out << "    \"failures\": [\n";
    for (std::size_t i = 0; i < report.comparison.failures.size(); ++i) {
        const auto& f = report.comparison.failures[i];
        out << "      {\"structure\": \"" << f.structure << "\", "
            << "\"operation\": \"" << f.operation << "\", "
            << "\"chosen_basis\": \"" << f.chosen_basis << "\", "
            << "\"chosen_delta_pct\": " << f.chosen_delta_pct << ", "
            << "\"threshold_pct\": " << f.threshold_pct << ", "
            << "\"failed_metric_families\": ";
        write_string_array(f.failed_metric_families);
        out << "}" << (i + 1 < report.comparison.failures.size() ? "," : "") << "\n";
    }
    out << "    ],\n";
    out << "    \"entries\": [\n";
    for (std::size_t i = 0; i < report.comparison.entries.size(); ++i) {
        const auto& e = report.comparison.entries[i];
        out << "      {\"structure\": \"" << e.structure << "\", "
            << "\"insert_delta_pct\": " << e.insert_delta_pct << ", "
            << "\"search_delta_pct\": " << e.search_delta_pct << ", "
            << "\"remove_delta_pct\": " << e.remove_delta_pct << ", "
            << "\"insert_ok\": " << (e.insert_ok ? "true" : "false") << ", "
            << "\"search_ok\": " << (e.search_ok ? "true" : "false") << ", "
            << "\"remove_ok\": " << (e.remove_ok ? "true" : "false") << ", "
            << "\"insert_basis\": \"" << e.insert_basis << "\", "
            << "\"search_basis\": \"" << e.search_basis << "\", "
            << "\"remove_basis\": \"" << e.remove_basis << "\", "
            << "\"insert_mean_delta_pct\": " << e.insert_mean_delta_pct << ", "
            << "\"insert_p95_delta_pct\": " << e.insert_p95_delta_pct << ", "
            << "\"insert_ci_high_delta_pct\": " << e.insert_ci_high_delta_pct << ", "
            << "\"insert_mean_ok\": " << (e.insert_mean_ok ? "true" : "false") << ", "
            << "\"insert_p95_ok\": " << (e.insert_p95_ok ? "true" : "false") << ", "
            << "\"insert_ci_high_ok\": " << (e.insert_ci_high_ok ? "true" : "false") << ", "
            << "\"search_mean_delta_pct\": " << e.search_mean_delta_pct << ", "
            << "\"search_p95_delta_pct\": " << e.search_p95_delta_pct << ", "
            << "\"search_ci_high_delta_pct\": " << e.search_ci_high_delta_pct << ", "
            << "\"search_mean_ok\": " << (e.search_mean_ok ? "true" : "false") << ", "
            << "\"search_p95_ok\": " << (e.search_p95_ok ? "true" : "false") << ", "
            << "\"search_ci_high_ok\": " << (e.search_ci_high_ok ? "true" : "false") << ", "
            << "\"remove_mean_delta_pct\": " << e.remove_mean_delta_pct << ", "
            << "\"remove_p95_delta_pct\": " << e.remove_p95_delta_pct << ", "
            << "\"remove_ci_high_delta_pct\": " << e.remove_ci_high_delta_pct << ", "
            << "\"remove_mean_ok\": " << (e.remove_mean_ok ? "true" : "false") << ", "
            << "\"remove_p95_ok\": " << (e.remove_p95_ok ? "true" : "false") << ", "
            << "\"remove_ci_high_ok\": " << (e.remove_ci_high_ok ? "true" : "false") << "}"
            << (i + 1 < report.comparison.entries.size() ? "," : "") << "\n";
    }
    out << "    ]\n  }\n}\n";
}

} // namespace hashbrowns
