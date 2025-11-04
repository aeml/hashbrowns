#ifndef HASHBROWNS_STATS_ANALYZER_H
#define HASHBROWNS_STATS_ANALYZER_H

#include <vector>
#include <numeric>
#include <cmath>

namespace hashbrowns {

struct StatsSummary {
    double mean{0.0};
    double stddev{0.0};
};

inline StatsSummary summarize(const std::vector<double>& samples) {
    StatsSummary s;
    if (samples.empty()) return s;
    double sum = std::accumulate(samples.begin(), samples.end(), 0.0);
    s.mean = sum / samples.size();
    double var = 0.0;
    for (double v : samples) var += (v - s.mean) * (v - s.mean);
    s.stddev = std::sqrt(var / samples.size());
    return s;
}

} // namespace hashbrowns

#endif // HASHBROWNS_STATS_ANALYZER_H