#ifndef HASHBROWNS_TIMER_H
#define HASHBROWNS_TIMER_H

#include <chrono>
#include <vector>
#include <string>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <functional>
#include <thread>

namespace hashbrowns {

/**
 * @brief High-precision timer for benchmarking operations
 * 
 * This class provides microsecond-precision timing with statistical analysis
 * capabilities. It's designed to handle the challenges of accurate performance
 * measurement including warm-up periods, outlier detection, and statistical
 * aggregation across multiple runs.
 */
class Timer {
public:
    using clock_type = std::chrono::high_resolution_clock;
    using time_point = clock_type::time_point;
    using duration = std::chrono::nanoseconds;

    /**
     * @brief Statistical summary of timing measurements
     */
    struct Statistics {
        double mean_ns;         ///< Mean time in nanoseconds
        double median_ns;       ///< Median time in nanoseconds
        double std_dev_ns;      ///< Standard deviation in nanoseconds
        double min_ns;          ///< Minimum time in nanoseconds
        double max_ns;          ///< Maximum time in nanoseconds
        size_t sample_count;    ///< Number of samples
        double outlier_ratio;   ///< Ratio of outliers removed

        // Convenience accessors for different time units
        double mean_us() const { return mean_ns / 1000.0; }
        double mean_ms() const { return mean_ns / 1000000.0; }
        double median_us() const { return median_ns / 1000.0; }
        double median_ms() const { return median_ns / 1000000.0; }
        double std_dev_us() const { return std_dev_ns / 1000.0; }
        double std_dev_ms() const { return std_dev_ns / 1000000.0; }
    };

    /**
     * @brief Construct timer with optional outlier detection
     * @param remove_outliers If true, automatically remove statistical outliers
     * @param outlier_threshold Z-score threshold for outlier detection (default 2.0)
     */
    explicit Timer(bool remove_outliers = true, double outlier_threshold = 2.0);

    /**
     * @brief Start timing an operation
     */
    void start();

    /**
     * @brief Stop timing and record the duration
     * @return Duration of the timed operation in nanoseconds
     */
    duration stop();

    /**
     * @brief Add a pre-measured duration to the sample set
     * @param d Duration to add
     */
    void add_sample(duration d);

    /**
     * @brief Get the duration of the last measured operation
     * @return Last measured duration in nanoseconds
     */
    duration last_duration() const { return last_duration_; }

    /**
     * @brief Clear all recorded samples
     */
    void reset();

    /**
     * @brief Get statistical summary of all recorded samples
     * @return Statistics object with aggregated timing data
     */
    Statistics get_statistics() const;

    /**
     * @brief Get all raw sample durations
     * @return Vector of all recorded durations
     */
    const std::vector<duration>& get_samples() const { return samples_; }

    /**
     * @brief Get number of recorded samples
     */
    size_t sample_count() const { return samples_.size(); }

    /**
     * @brief Check if timer is currently running
     */
    bool is_running() const { return is_running_; }

    /**
     * @brief Perform warm-up runs to stabilize CPU caches and frequency
     * @param warmup_count Number of warm-up iterations
     * @param operation Function to execute during warm-up
     */
    template<typename Func>
    void warmup(size_t warmup_count, Func&& operation) {
        for (size_t i = 0; i < warmup_count; ++i) {
            operation();
        }
        // Give CPU time to settle after warm-up
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    /**
     * @brief Time a function multiple times and collect statistics
     * @param operation Function to time
     * @param iterations Number of iterations to run
     * @param warmup_runs Number of warm-up runs before timing
     * @return Statistics for the timed operations
     */
    template<typename Func>
    Statistics time_operation(Func&& operation, size_t iterations, size_t warmup_runs = 3) {
        reset();
        
        // Warm-up phase
        if (warmup_runs > 0) {
            warmup(warmup_runs, operation);
        }
        
        // Actual timing
        for (size_t i = 0; i < iterations; ++i) {
            start();
            operation();
            stop();
        }
        
        return get_statistics();
    }

private:
    time_point start_time_;
    duration last_duration_;
    std::vector<duration> samples_;
    bool is_running_;
    bool remove_outliers_;
    double outlier_threshold_;

    /**
     * @brief Remove statistical outliers from samples
     * @param data Vector of durations to filter
     * @return Filtered vector with outliers removed
     */
    std::vector<duration> remove_outliers(const std::vector<duration>& data) const;

    /**
     * @brief Calculate Z-score for a value in a dataset
     */
    double calculate_zscore(double value, double mean, double std_dev) const;
};

/**
 * @brief RAII scope timer for automatic timing of code blocks
 * 
 * This class automatically starts timing on construction and stops on destruction,
 * making it perfect for timing scoped operations.
 * 
 * Example usage:
 * @code
 * {
 *     ScopeTimer timer("Operation Name");
 *     // ... code to time ...
 * } // timer automatically stops and optionally prints result
 * @endcode
 */
class ScopeTimer {
public:
    /**
     * @brief Construct scope timer with optional name and auto-print
     * @param name Optional name for the operation being timed
     * @param auto_print If true, print timing result on destruction
     */
    explicit ScopeTimer(const std::string& name = "", bool auto_print = true);

    /**
     * @brief Destructor - automatically stops timing
     */
    ~ScopeTimer();

    /**
     * @brief Get the elapsed time so far (without stopping)
     * @return Current elapsed time
     */
    Timer::duration elapsed() const;

    /**
     * @brief Manually stop the timer and get final duration
     * @return Total elapsed time
     */
    Timer::duration stop();

private:
    Timer::time_point start_time_;
    std::string operation_name_;
    bool auto_print_;
    bool stopped_;
};

/**
 * @brief Benchmark runner for comparative performance analysis
 * 
 * This class facilitates running multiple benchmark tests and comparing
 * their performance characteristics.
 */
class BenchmarkRunner {
public:
    struct BenchmarkResult {
        std::string name;
        Timer::Statistics stats;
        double operations_per_second;
        size_t data_size;
    };

    /**
     * @brief Add a benchmark test
     * @param name Name identifier for the benchmark
     * @param operation Function to benchmark
     * @param iterations Number of times to run the operation
     * @param data_size Size of data being processed (for throughput calculation)
     */
    template<typename Func>
    void add_benchmark(const std::string& name, Func&& operation, 
                      size_t iterations, size_t data_size = 0) {
        Timer timer(true, 2.0); // Remove outliers with 2-sigma threshold
        auto stats = timer.time_operation(std::forward<Func>(operation), iterations, 5);
        
        double ops_per_sec = 0.0;
        if (stats.mean_ns > 0) {
            ops_per_sec = 1e9 / stats.mean_ns; // Convert to operations per second
        }
        
        results_.push_back({name, stats, ops_per_sec, data_size});
    }

    /**
     * @brief Get all benchmark results
     */
    const std::vector<BenchmarkResult>& get_results() const { return results_; }

    /**
     * @brief Clear all results
     */
    void clear() { results_.clear(); }

    /**
     * @brief Print formatted benchmark comparison
     */
    void print_comparison() const;

    /**
     * @brief Export results to CSV format
     * @param filename Output CSV file path
     */
    void export_csv(const std::string& filename) const;

private:
    std::vector<BenchmarkResult> results_;
};

/**
 * @brief Convenience macro for scope timing
 */
#define SCOPE_TIMER(name) hashbrowns::ScopeTimer _scope_timer(name)

/**
 * @brief Convenience macro for scope timing with custom auto-print setting
 */
#define SCOPE_TIMER_PRINT(name, print) hashbrowns::ScopeTimer _scope_timer(name, print)

} // namespace hashbrowns

#endif // HASHBROWNS_TIMER_H