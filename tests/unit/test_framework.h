#ifndef HASHBROWNS_TEST_FRAMEWORK_H
#define HASHBROWNS_TEST_FRAMEWORK_H

#include <iostream>
#include <string>
#include <functional>
#include <vector>
#include <stdexcept>
#include <chrono>
#include <sstream>

namespace hashbrowns {
namespace testing {

/**
 * @brief Simple but powerful testing framework for hashbrowns
 * 
 * Provides essential testing capabilities without external dependencies:
 * - Assertion macros with detailed error messages
 * - Test case organization and execution
 * - Performance measurement utilities
 * - Exception testing support
 * - Comprehensive test reporting
 */
class TestFramework {
public:
    struct TestResult {
        std::string name;
        bool passed;
        std::string error_message;
        double duration_ms;
    };

    struct TestSuite {
        std::string name;
        std::vector<TestResult> results;
        size_t total_tests = 0;
        size_t passed_tests = 0;
        double total_duration_ms = 0.0;
    };

private:
    static TestFramework* instance_;
    std::vector<TestSuite> suites_;
    TestSuite* current_suite_ = nullptr;
    std::chrono::high_resolution_clock::time_point test_start_time_;

public:
    static TestFramework& instance() {
        if (!instance_) {
            instance_ = new TestFramework();
        }
        return *instance_;
    }

    void begin_suite(const std::string& suite_name) {
        suites_.emplace_back();
        current_suite_ = &suites_.back();
        current_suite_->name = suite_name;
        std::cout << "\n=== " << suite_name << " ===\n";
    }

    void begin_test(const std::string& test_name) {
        if (!current_suite_) {
            throw std::runtime_error("No test suite active");
        }
        test_start_time_ = std::chrono::high_resolution_clock::now();
        std::cout << "  " << test_name << "... ";
    }

    void end_test(const std::string& test_name, bool passed, const std::string& error = "") {
        if (!current_suite_) return;
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(end_time - test_start_time_);
        
        TestResult result;
        result.name = test_name;
        result.passed = passed;
        result.error_message = error;
        result.duration_ms = duration.count();
        
        current_suite_->results.push_back(result);
        current_suite_->total_tests++;
        current_suite_->total_duration_ms += result.duration_ms;
        
        if (passed) {
            current_suite_->passed_tests++;
            std::cout << "PASS (" << result.duration_ms << "ms)\n";
        } else {
            std::cout << "FAIL (" << result.duration_ms << "ms)\n";
            if (!error.empty()) {
                std::cout << "    Error: " << error << "\n";
            }
        }
    }

    void print_summary() const {
        std::cout << "\n" << std::string(60, '=') << "\n";
        std::cout << "TEST SUMMARY\n";
        std::cout << std::string(60, '=') << "\n";
        
        size_t total_tests = 0;
        size_t total_passed = 0;
        double total_time = 0.0;
        
        for (const auto& suite : suites_) {
            std::cout << "\n" << suite.name << ":\n";
            std::cout << "  Tests: " << suite.passed_tests << "/" << suite.total_tests;
            std::cout << " (" << (suite.total_tests > 0 ? (suite.passed_tests * 100 / suite.total_tests) : 0) << "%)\n";
            std::cout << "  Time: " << suite.total_duration_ms << "ms\n";
            
            total_tests += suite.total_tests;
            total_passed += suite.passed_tests;
            total_time += suite.total_duration_ms;
        }
        
        std::cout << "\nOVERALL:\n";
        std::cout << "  Tests: " << total_passed << "/" << total_tests;
        std::cout << " (" << (total_tests > 0 ? (total_passed * 100 / total_tests) : 0) << "%)\n";
        std::cout << "  Time: " << total_time << "ms\n";
        
        if (total_passed == total_tests) {
            std::cout << "\n✓ ALL TESTS PASSED!\n";
        } else {
            std::cout << "\n✗ " << (total_tests - total_passed) << " TEST(S) FAILED!\n";
        }
        std::cout << std::string(60, '=') << "\n";
    }

    bool all_tests_passed() const {
        for (const auto& suite : suites_) {
            if (suite.passed_tests != suite.total_tests) {
                return false;
            }
        }
        return true;
    }
};

TestFramework* TestFramework::instance_ = nullptr;

// Helper class for RAII test management
class TestCase {
private:
    std::string name_;
    bool passed_ = true;
    std::string error_;

public:
    TestCase(const std::string& name) : name_(name) {
        TestFramework::instance().begin_test(name);
    }

    ~TestCase() {
        TestFramework::instance().end_test(name_, passed_, error_);
    }

    void fail(const std::string& message) {
        passed_ = false;
        error_ = message;
    }

    bool is_passing() const { return passed_; }
};

// Exception testing helper
template<typename ExceptionType, typename Func>
bool expect_exception(Func&& func) {
    try {
        func();
        return false; // No exception thrown
    } catch (const ExceptionType&) {
        return true; // Expected exception caught
    } catch (...) {
        return false; // Wrong exception type
    }
}

} // namespace testing
} // namespace hashbrowns

// Test macros for convenient testing
#define TEST_SUITE(name) hashbrowns::testing::TestFramework::instance().begin_suite(name)

#define TEST_CASE(name) \
    if (hashbrowns::testing::TestCase test_case(name); true)

#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            std::ostringstream oss; \
            oss << "Assertion failed: " #condition " at " << __FILE__ << ":" << __LINE__; \
            test_case.fail(oss.str()); \
            return; \
        } \
    } while(0)

#define ASSERT_FALSE(condition) \
    do { \
        if (condition) { \
            std::ostringstream oss; \
            oss << "Assertion failed: !(" #condition ") at " << __FILE__ << ":" << __LINE__; \
            test_case.fail(oss.str()); \
            return; \
        } \
    } while(0)

#define ASSERT_EQ(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            std::ostringstream oss; \
            oss << "Assertion failed: " #expected " == " #actual \
                << " (expected: " << (expected) << ", actual: " << (actual) \
                << ") at " << __FILE__ << ":" << __LINE__; \
            test_case.fail(oss.str()); \
            return; \
        } \
    } while(0)

#define ASSERT_NE(not_expected, actual) \
    do { \
        if ((not_expected) == (actual)) { \
            std::ostringstream oss; \
            oss << "Assertion failed: " #not_expected " != " #actual \
                << " (both equal: " << (actual) << ") at " << __FILE__ << ":" << __LINE__; \
            test_case.fail(oss.str()); \
            return; \
        } \
    } while(0)

#define ASSERT_LT(left, right) \
    do { \
        if (!((left) < (right))) { \
            std::ostringstream oss; \
            oss << "Assertion failed: " #left " < " #right \
                << " (" << (left) << " not less than " << (right) \
                << ") at " << __FILE__ << ":" << __LINE__; \
            test_case.fail(oss.str()); \
            return; \
        } \
    } while(0)

#define ASSERT_LE(left, right) \
    do { \
        if (!((left) <= (right))) { \
            std::ostringstream oss; \
            oss << "Assertion failed: " #left " <= " #right \
                << " (" << (left) << " not less than or equal to " << (right) \
                << ") at " << __FILE__ << ":" << __LINE__; \
            test_case.fail(oss.str()); \
            return; \
        } \
    } while(0)

#define ASSERT_GT(left, right) \
    do { \
        if (!((left) > (right))) { \
            std::ostringstream oss; \
            oss << "Assertion failed: " #left " > " #right \
                << " (" << (left) << " not greater than " << (right) \
                << ") at " << __FILE__ << ":" << __LINE__; \
            test_case.fail(oss.str()); \
            return; \
        } \
    } while(0)

#define ASSERT_GE(left, right) \
    do { \
        if (!((left) >= (right))) { \
            std::ostringstream oss; \
            oss << "Assertion failed: " #left " >= " #right \
                << " (" << (left) << " not greater than or equal to " << (right) \
                << ") at " << __FILE__ << ":" << __LINE__; \
            test_case.fail(oss.str()); \
            return; \
        } \
    } while(0)

#define ASSERT_THROWS(exception_type, expression) \
    do { \
        if (!hashbrowns::testing::expect_exception<exception_type>([&]() { expression; })) { \
            std::ostringstream oss; \
            oss << "Expected exception " #exception_type " not thrown by: " #expression \
                << " at " << __FILE__ << ":" << __LINE__; \
            test_case.fail(oss.str()); \
            return; \
        } \
    } while(0)

#define ASSERT_NO_THROW(expression) \
    do { \
        try { \
            expression; \
        } catch (...) { \
            std::ostringstream oss; \
            oss << "Unexpected exception thrown by: " #expression \
                << " at " << __FILE__ << ":" << __LINE__; \
            test_case.fail(oss.str()); \
            return; \
        } \
    } while(0)

#endif // HASHBROWNS_TEST_FRAMEWORK_H