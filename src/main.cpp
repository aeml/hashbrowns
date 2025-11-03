#include <iostream>
#include <string>
#include <memory>
#include <thread>
#include <chrono>

#include "core/data_structure.h"
#include "core/memory_manager.h"
#include "core/timer.h"

using namespace hashbrowns;

void print_banner() {
    std::cout << R"(
🥔 hashbrowns - C++ Data Structure Benchmarking Suite
======================================================

A crispy performance comparison of:
- Dynamic Arrays (contiguous memory)
- Linked Lists (pointer-based nodes)  
- Hash Maps (manual hashing implementation)

Core architecture initialized successfully!
)" << std::endl;
}

void demonstrate_core_features() {
    std::cout << "=== Core Features Demonstration ===\n\n";
    
    // Memory tracking demonstration
    std::cout << "1. Memory Tracking System:\n";
    auto& tracker = MemoryTracker::instance();
    tracker.set_detailed_tracking(true);
    tracker.reset();
    
    // Allocate some memory to show tracking
    {
        auto array = make_unique_array<int>(100);
        for (size_t i = 0; i < 100; ++i) {
            array[i] = static_cast<int>(i);
        }
        
        auto stats = tracker.get_stats();
        std::cout << "   - Allocated: " << stats.total_allocated << " bytes\n";
        std::cout << "   - Current usage: " << stats.current_usage << " bytes\n";
        std::cout << "   - Allocation count: " << stats.allocation_count << "\n";
    } // array goes out of scope here
    
    // Check final stats
    auto final_stats = tracker.get_stats();
    std::cout << "   - After cleanup: " << final_stats.current_usage << " bytes\n";
    
    // Timer demonstration
    std::cout << "\n2. High-Resolution Timer:\n";
    Timer timer;
    
    // Time a simple operation
    timer.start();
    volatile int sum = 0;
    for (int i = 0; i < 1000000; ++i) {
        sum += i;
    }
    auto duration = timer.stop();
    
    std::cout << "   - Million integer additions: " 
              << duration.count() / 1000.0 << " microseconds\n";
    
    // Scope timer demonstration
    {
        ScopeTimer scope_timer("Sleep operation", true);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    std::cout << "\n3. Memory Leak Check:\n";
    bool clean = tracker.check_leaks();
    if (clean) {
        std::cout << "   ✓ No memory leaks detected!\n";
    }
}

void show_usage() {
    std::cout << R"(
Usage: hashbrowns [OPTIONS]

OPTIONS:
  --size N              Set benchmark data size (default: 10000)
  --runs N              Number of benchmark runs (default: 10)
  --structures LIST     Comma-separated list: array,list,hashmap
  --output FILE         Export results to CSV file
  --pattern TYPE        Data pattern: sequential, random, mixed
  --memory-tracking     Enable detailed memory analysis
  --crossover-analysis  Find performance crossover points
  --verbose             Detailed output
  --help               Show this help message

EXAMPLES:
  hashbrowns --size 50000 --runs 20
  hashbrowns --structures array,hashmap --output results.csv
  hashbrowns --crossover-analysis --max-size 100000

NOTE: Data structure implementations are coming in the next phase!
      This build demonstrates the core architecture and benchmarking framework.
)" << std::endl;
}

int main(int argc, char* argv[]) {
    print_banner();
    
    // Parse basic command line arguments
    bool show_help = false;
    bool demo_mode = true;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            show_help = true;
            demo_mode = false;
        } else if (arg == "--size" || arg == "--runs" || arg == "--structures") {
            // Skip the next argument (parameter value)
            if (i + 1 < argc) i++;
            demo_mode = false;
        } else if (arg.find("--") == 0) {
            demo_mode = false;
        }
    }
    
    if (show_help) {
        show_usage();
        return 0;
    }
    
    if (demo_mode) {
        std::cout << "Running in demonstration mode...\n";
        demonstrate_core_features();
        std::cout << "\nRun with --help to see available options.\n";
        std::cout << "Full benchmarking capabilities will be available once data structures are implemented!\n";
    } else {
        std::cout << "Command-line parsing detected, but data structure implementations are not ready yet.\n";
        std::cout << "Please run without arguments for a demonstration of the core architecture.\n";
        return 1;
    }
    
    return 0;
}