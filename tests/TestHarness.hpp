#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <cmath>

namespace test {

struct TestCase {
    std::string name;
    std::function<void()> func;
};

inline std::vector<TestCase>& GetRegistry() {
    static std::vector<TestCase> registry;
    return registry;
}

struct Registerer {
    Registerer(const std::string& name, std::function<void()> func) {
        GetRegistry().push_back({name, std::move(func)});
    }
};

inline int g_failed_checks = 0;
inline int g_total_checks = 0;

inline void CheckTrue(bool cond, const char* expr, const char* file, int line) {
    ++g_total_checks;
    if (!cond) {
        ++g_failed_checks;
        std::cerr << "  FAILED: " << expr << " at " << file << ":" << line << "\n";
    }
}

template <typename T, typename U>
inline void CheckEq(const T& a, const U& b, const char* expr_a, const char* expr_b, const char* file, int line) {
    ++g_total_checks;
    if (!(a == b)) {
        ++g_failed_checks;
        std::cerr << "  FAILED: (" << expr_a << " == " << expr_b << ") [" << a << " != " << b << "] at " << file << ":" << line << "\n";
    }
}

template <typename T, typename U>
inline void CheckNear(const T& a, const U& b, double eps, const char* expr_a, const char* expr_b, const char* file, int line) {
    ++g_total_checks;
    if (std::abs(static_cast<double>(a) - static_cast<double>(b)) > eps) {
        ++g_failed_checks;
        std::cerr << "  FAILED: (" << expr_a << " ~= " << expr_b << ") [" << a << " vs " << b << ", diff > " << eps << "] at " << file << ":" << line << "\n";
    }
}

} // namespace test

#define TEST_CASE(name) \
    static void test_func_##name(); \
    static ::test::Registerer reg_##name(#name, test_func_##name); \
    static void test_func_##name()

#define CHECK_TRUE(cond) ::test::CheckTrue(static_cast<bool>(cond), #cond, __FILE__, __LINE__)
#define CHECK_FALSE(cond) ::test::CheckTrue(!static_cast<bool>(cond), "!(" #cond ")", __FILE__, __LINE__)
#define CHECK_EQ(a, b) ::test::CheckEq((a), (b), #a, #b, __FILE__, __LINE__)
#define CHECK_NEAR(a, b, eps) ::test::CheckNear((a), (b), (eps), #a, #b, __FILE__, __LINE__)

