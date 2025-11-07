#ifndef HASHBROWNS_STATS_ANALYZER_H
#define HASHBROWNS_STATS_ANALYZER_H

#include <vector>
#include <numeric>
#include <cmath>
#include <algorithm>
#include <random>

namespace hashbrowns {

struct StatsSummary {
    double mean{0.0};
    double stddev{0.0};
    double median{0.0};
    double p95{0.0};
    double ci_low{0.0};  // 95% CI lower bound (mean, bootstrap)
    double ci_high{0.0}; // 95% CI upper bound (mean, bootstrap)
    int samples{0};
};

inline static double percentile(std::vector<double> v, double p) {
    if (v.empty()) return 0.0;
    std::sort(v.begin(), v.end());
    double idx = p * (v.size() - 1);
    size_t i0 = static_cast<size_t>(std::floor(idx));
    size_t i1 = static_cast<size_t>(std::ceil(idx));
    if (i0 == i1) return v[i0];
    double w = idx - i0;
    return v[i0] * (1.0 - w) + v[i1] * w;
}

inline StatsSummary summarize(const std::vector<double>& samples, int bootstrap_iters = 0) {
    StatsSummary s;
    s.samples = static_cast<int>(samples.size());
    if (samples.empty()) return s;
    double sum = std::accumulate(samples.begin(), samples.end(), 0.0);
    s.mean = sum / samples.size();
    double var = 0.0;
    for (double v : samples) var += (v - s.mean) * (v - s.mean);
    s.stddev = std::sqrt(var / samples.size());
    // median & p95
    std::vector<double> tmp = samples;
    std::sort(tmp.begin(), tmp.end());
    s.median = tmp[tmp.size()/2];
    s.p95 = percentile(tmp, 0.95);
    if (bootstrap_iters > 0 && samples.size() > 1) {
        std::vector<double> means; means.reserve(bootstrap_iters);
        std::mt19937_64 rng(123456789ULL); // deterministic bootstrap seed
        std::uniform_int_distribution<size_t> dist(0, samples.size()-1);
        for (int i = 0; i < bootstrap_iters; ++i) {
            double bsum = 0.0;
            for (size_t k = 0; k < samples.size(); ++k) bsum += samples[dist(rng)];
            means.push_back(bsum / samples.size());
        }
        std::sort(means.begin(), means.end());
        auto idx_lo = static_cast<size_t>(0.025 * (means.size()-1));
        auto idx_hi = static_cast<size_t>(0.975 * (means.size()-1));
        s.ci_low = means[idx_lo];
        s.ci_high = means[idx_hi];
    } else {
        s.ci_low = s.mean; s.ci_high = s.mean; // degenerate CI
    }
    return s;
}

} // namespace hashbrowns

#endif // HASHBROWNS_STATS_ANALYZER_H