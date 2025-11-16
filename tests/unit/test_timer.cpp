#include "core/timer.h"
#include <iostream>
#include <thread>
#include <stdexcept>
#include <fstream>

using namespace hashbrowns;

int run_timer_tests() {
    int failures = 0;
    std::cout << "Timer Test Suite\n";
    std::cout << "================\n\n";

    // Basic start/stop with sleep (two valid cycles)
    {
        Timer timer;

        // First measurement
        timer.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        auto d1 = timer.stop();
        if (d1.count() <= 0) {
            std::cout << "❌ Timer duration not positive on first stop\n";
            ++failures;
        }

        // Second measurement (fresh start/stop)
        timer.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        auto d2 = timer.stop();
        if (d2.count() <= 0) {
            std::cout << "❌ Timer duration not positive on second stop\n";
            ++failures;
        }
    }

    // Test add_sample and get_statistics
    {
        Timer timer;
        timer.add_sample(Timer::duration(1000));
        timer.add_sample(Timer::duration(2000));
        timer.add_sample(Timer::duration(3000));
        timer.add_sample(Timer::duration(4000));
        timer.add_sample(Timer::duration(5000));

        auto stats = timer.get_statistics();
        if (stats.sample_count != 5) {
            std::cout << "❌ Expected 5 samples, got " << stats.sample_count << "\n";
            ++failures;
        }
        if (stats.mean_ns != 3000.0) {
            std::cout << "❌ Expected mean 3000ns, got " << stats.mean_ns << "\n";
            ++failures;
        }
        if (stats.median_ns != 3000.0) {
            std::cout << "❌ Expected median 3000ns, got " << stats.median_ns << "\n";
            ++failures;
        }
        if (stats.min_ns != 1000.0) {
            std::cout << "❌ Expected min 1000ns, got " << stats.min_ns << "\n";
            ++failures;
        }
        if (stats.max_ns != 5000.0) {
            std::cout << "❌ Expected max 5000ns, got " << stats.max_ns << "\n";
            ++failures;
        }
    }

    // Test reset
    {
        Timer timer;
        timer.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        timer.stop();
        
        if (timer.sample_count() != 1) {
            std::cout << "❌ Expected 1 sample before reset\n";
            ++failures;
        }

        timer.reset();
        if (timer.sample_count() != 0) {
            std::cout << "❌ Expected 0 samples after reset, got " << timer.sample_count() << "\n";
            ++failures;
        }
        if (timer.is_running()) {
            std::cout << "❌ Timer should not be running after reset\n";
            ++failures;
        }
        if (timer.last_duration().count() != 0) {
            std::cout << "❌ Last duration should be 0 after reset\n";
            ++failures;
        }
    }

    // Test get_statistics with outliers
    {
        Timer timer(true, 2.0);  // Enable outlier removal
        timer.add_sample(Timer::duration(100));
        timer.add_sample(Timer::duration(110));
        timer.add_sample(Timer::duration(105));
        timer.add_sample(Timer::duration(108));
        timer.add_sample(Timer::duration(1000));  // Outlier
        
        auto stats = timer.get_statistics();
        if (stats.outlier_ratio <= 0) {
            std::cout << "❌ Expected outliers to be detected and removed\n";
            ++failures;
        }
        // Mean should be close to 105-110 range after outlier removal
        if (stats.mean_ns > 200) {
            std::cout << "❌ Outlier not removed, mean is " << stats.mean_ns << "\n";
            ++failures;
        }
    }

    // Test empty statistics
    {
        Timer timer;
        auto stats = timer.get_statistics();
        if (stats.mean_ns != 0.0 || stats.sample_count != 0) {
            std::cout << "❌ Empty timer should return zero statistics\n";
            ++failures;
        }
    }

    // Test error handling - start when already running
    {
        Timer timer;
        timer.start();
        bool caught_exception = false;
        try {
            timer.start();  // Should throw
        } catch (const std::runtime_error&) {
            caught_exception = true;
        }
        if (!caught_exception) {
            std::cout << "❌ Should throw exception when starting already running timer\n";
            ++failures;
        }
        timer.stop();  // Clean up
    }

    // Test error handling - stop when not running
    {
        Timer timer;
        bool caught_exception = false;
        try {
            timer.stop();  // Should throw
        } catch (const std::runtime_error&) {
            caught_exception = true;
        }
        if (!caught_exception) {
            std::cout << "❌ Should throw exception when stopping non-running timer\n";
            ++failures;
        }
    }

    // Test ScopeTimer
    {
        // Test auto-print disabled
        ScopeTimer st("test_operation", false);
        auto elapsed = st.elapsed();
        if (elapsed.count() < 0) {
            std::cout << "❌ ScopeTimer elapsed should be non-negative\n";
            ++failures;
        }
        auto stopped = st.stop();
        if (stopped.count() < 0) {
            std::cout << "❌ ScopeTimer stop should return non-negative duration\n";
            ++failures;
        }
        
        // Test stop again - should return 0
        auto stopped_again = st.stop();
        if (stopped_again.count() != 0) {
            std::cout << "❌ ScopeTimer second stop should return 0\n";
            ++failures;
        }
    }

    // Test ScopeTimer with auto-print (will print to stdout)
    {
        std::cout << "Testing ScopeTimer auto-print (should see output below):\n";
        {
            ScopeTimer st("auto_print_test", true);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }  // Destructor should print timing
    }

    // Test BenchmarkRunner
    {
        BenchmarkRunner runner;
        
        // Add some benchmark operations
        int counter1 = 0;
        runner.add_benchmark("test_op1", [&counter1]() {
            for (int i = 0; i < 1000; ++i) counter1++;
        }, 10, 1000);
        
        int counter2 = 0;
        runner.add_benchmark("test_op2", [&counter2]() {
            for (int i = 0; i < 2000; ++i) counter2++;
        }, 10, 2000);
        
        // Test print_comparison
        std::cout << "\nTesting BenchmarkRunner::print_comparison:\n";
        runner.print_comparison();
        
        // Test export_csv
        std::string csv_file = "test_benchmark_export.csv";
        runner.export_csv(csv_file);
        
        // Verify CSV file was created (basic check)
        std::ifstream check(csv_file);
        if (!check.good()) {
            std::cout << "❌ Failed to export CSV file\n";
            ++failures;
        } else {
            std::cout << "✅ CSV export successful\n";
        }
        check.close();
    }

    if (failures == 0) {
        std::cout << "\n✅ Timer tests passed!\n";
    } else {
        std::cout << "\n❌ " << failures << " Timer test(s) failed.\n";
    }

    return failures == 0 ? 0 : 1;
}
