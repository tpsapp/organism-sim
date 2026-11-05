#include "TestHarness.hpp"
#include <iostream>

int main() {
    std::cout << "=======================================\n";
    std::cout << " Running Headless Simulation Test Suite\n";
    std::cout << "=======================================\n";

    int tests_passed = 0;
    int tests_failed = 0;

    for (const auto& tc : test::GetRegistry()) {
        int checks_before = test::g_failed_checks;
        std::cout << "[ RUN      ] " << tc.name << "\n";
        try {
            tc.func();
        } catch (const std::exception& e) {
            std::cerr << "  EXCEPTION: " << e.what() << "\n";
            ++test::g_failed_checks;
        } catch (...) {
            std::cerr << "  UNKNOWN EXCEPTION\n";
            ++test::g_failed_checks;
        }

        if (test::g_failed_checks == checks_before) {
            std::cout << "[       OK ] " << tc.name << "\n";
            ++tests_passed;
        } else {
            std::cout << "[  FAILED  ] " << tc.name << "\n";
            ++tests_failed;
        }
    }

    std::cout << "=======================================\n";
    std::cout << " Test Summary: " << tests_passed << " passed, " << tests_failed << " failed ("
              << test::g_total_checks << " total assertions)\n";
    std::cout << "=======================================\n";

    return (tests_failed == 0) ? 0 : 1;
}

