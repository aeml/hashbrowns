#include "timer.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <thread>

namespace hashbrowns {

Timer::Timer(bool remove_outliers, double outlier_threshold)
    : last_duration_(0), is_running_(false), 
      remove_outliers_(remove_outliers), outlier_threshold_(outlier_threshold) {
}

void Timer::start() {
    if (is_running_) {
        throw std::runtime_error("Timer is already running");
    }
    start_time_ = clock_type::now();
    is_running_ = true;
}

Timer::duration Timer::stop() {
    if (!is_running_) {
        throw std::runtime_error("Timer is not running");
    }
    
    auto end_time = clock_type::now();
    last_duration_ = std::chrono::duration_cast<duration>(end_time - start_time_);
    samples_.push_back(last_duration_);
    is_running_ = false;
    
    return last_duration_;
}

void Timer::add_sample(duration d) {
    samples_.push_back(d);
}

void Timer::reset() {
    samples_.clear();
    last_duration_ = duration(0);
    is_running_ = false;
}

Timer::Statistics Timer::get_statistics() const {
    if (samples_.empty()) {
        return {0.0, 0.0, 0.0, 0.0, 0.0, 0, 0.0};
    }
    
    // Work with a copy so we can sort without modifying original
    std::vector<duration> data = samples_;
    
    // Remove outliers if requested
    size_t original_count = data.size();
    if (remove_outliers_ && data.size() > 3) {
        data = remove_outliers(data);
    }
    
    if (data.empty()) {
        return {0.0, 0.0, 0.0, 0.0, 0.0, 0, 1.0}; // All samples were outliers
    }
    
    // Sort for median calculation
    std::sort(data.begin(), data.end());
    
    // Calculate basic statistics
    double sum = 0.0;
    for (const auto& d : data) {
        sum += d.count();
    }
    
    double mean = sum / data.size();
    
    // Median
    double median;
    size_t n = data.size();
    if (n % 2 == 0) {
        median = (data[n/2 - 1].count() + data[n/2].count()) / 2.0;
    } else {
        median = data[n/2].count();
    }
    
    // Standard deviation
    double variance_sum = 0.0;
    for (const auto& d : data) {
        double diff = d.count() - mean;
        variance_sum += diff * diff;
    }
    double std_dev = std::sqrt(variance_sum / data.size());
    
    // Min/max
    double min_val = data.front().count();
    double max_val = data.back().count();
    
    // Outlier ratio
    double outlier_ratio = static_cast<double>(original_count - data.size()) / original_count;
    
    return {mean, median, std_dev, min_val, max_val, data.size(), outlier_ratio};
}

std::vector<Timer::duration> Timer::remove_outliers(const std::vector<duration>& data) const {
    if (data.size() <= 3) {
        return data; // Too few samples for outlier detection
    }
    
    // Calculate mean and standard deviation
    double sum = 0.0;
    for (const auto& d : data) {
        sum += d.count();
    }
    double mean = sum / data.size();
    
    double variance_sum = 0.0;
    for (const auto& d : data) {
        double diff = d.count() - mean;
        variance_sum += diff * diff;
    }
    double std_dev = std::sqrt(variance_sum / data.size());
    
    // Filter out outliers
    std::vector<duration> filtered;
    for (const auto& d : data) {
        double zscore = calculate_zscore(d.count(), mean, std_dev);
        if (std::abs(zscore) <= outlier_threshold_) {
            filtered.push_back(d);
        }
    }
    
    return filtered.empty() ? data : filtered; // Return original if all were outliers
}

double Timer::calculate_zscore(double value, double mean, double std_dev) const {
    if (std_dev == 0.0) return 0.0;
    return (value - mean) / std_dev;
}

// ScopeTimer implementation
ScopeTimer::ScopeTimer(const std::string& name, bool auto_print)
    : operation_name_(name), auto_print_(auto_print), stopped_(false) {
    start_time_ = Timer::clock_type::now();
}

ScopeTimer::~ScopeTimer() {
    if (!stopped_ && auto_print_) {
        auto duration = stop();
        std::cout << "[TIMER] ";
        if (!operation_name_.empty()) {
            std::cout << operation_name_ << ": ";
        }
        std::cout << std::fixed << std::setprecision(3) 
                  << duration.count() / 1000000.0 << " ms\n";
    }
}

Timer::duration ScopeTimer::elapsed() const {
    auto now = Timer::clock_type::now();
    return std::chrono::duration_cast<Timer::duration>(now - start_time_);
}

Timer::duration ScopeTimer::stop() {
    if (!stopped_) {
        stopped_ = true;
        return elapsed();
    }
    return Timer::duration(0);
}

// BenchmarkRunner implementation
void BenchmarkRunner::print_comparison() const {
    if (results_.empty()) {
        std::cout << "No benchmark results to display.\n";
        return;
    }
    
    std::cout << "\n=== Benchmark Results ===\n";
    std::cout << std::left << std::setw(20) << "Benchmark"
              << std::right << std::setw(12) << "Mean (ms)"
              << std::setw(12) << "Median (ms)"
              << std::setw(12) << "Std Dev (ms)"
              << std::setw(15) << "Ops/Sec"
              << std::setw(10) << "Samples\n";
    std::cout << std::string(81, '-') << "\n";
    
    for (const auto& result : results_) {
        std::cout << std::left << std::setw(20) << result.name
                  << std::right << std::fixed << std::setprecision(3)
                  << std::setw(12) << result.stats.mean_ms()
                  << std::setw(12) << result.stats.median_ms()
                  << std::setw(12) << result.stats.std_dev_ms()
                  << std::setw(15) << std::scientific << std::setprecision(2) << result.operations_per_second
                  << std::setw(10) << result.stats.sample_count << "\n";
    }
    
    // Find fastest and slowest
    if (results_.size() > 1) {
        auto fastest = std::min_element(results_.begin(), results_.end(),
            [](const auto& a, const auto& b) { return a.stats.mean_ns < b.stats.mean_ns; });
        auto slowest = std::max_element(results_.begin(), results_.end(),
            [](const auto& a, const auto& b) { return a.stats.mean_ns < b.stats.mean_ns; });
        
        double speedup = slowest->stats.mean_ns / fastest->stats.mean_ns;
        
        std::cout << "\n--- Performance Analysis ---\n";
        std::cout << "Fastest: " << fastest->name << "\n";
        std::cout << "Slowest: " << slowest->name << "\n";
        std::cout << "Speedup: " << std::fixed << std::setprecision(2) << speedup << "x\n";
    }
    
    std::cout << "========================\n\n";
}

void BenchmarkRunner::export_csv(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file) {
        throw std::runtime_error("Cannot open file for writing: " + filename);
    }
    
    // CSV header
    file << "Name,Mean_ns,Median_ns,StdDev_ns,Min_ns,Max_ns,Samples,Ops_per_sec,Data_size\n";
    
    // Data rows
    for (const auto& result : results_) {
        file << result.name << ","
             << result.stats.mean_ns << ","
             << result.stats.median_ns << ","
             << result.stats.std_dev_ns << ","
             << result.stats.min_ns << ","
             << result.stats.max_ns << ","
             << result.stats.sample_count << ","
             << result.operations_per_second << ","
             << result.data_size << "\n";
    }
    
    file.close();
    std::cout << "Benchmark results exported to: " << filename << "\n";
}

} // namespace hashbrowns