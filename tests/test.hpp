// Adapter Fabric — deterministic test harness.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <functional>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>

namespace af_test {

struct TestCase { std::string name; std::function<void()> fn; };
inline std::vector<TestCase>& registry() { static std::vector<TestCase> r; return r; }

inline int& failures() { static int f = 0; return f; }
inline int& checks() { static int c = 0; return c; }
inline std::string current() { return static std::string(); }
inline std::string& current_name() { static std::string s; return s; }

inline bool register_test(const std::string& name, std::function<void()> fn) {
  registry().push_back({name, std::move(fn)});
  return true;
}

inline void fail(const std::string& msg) {
  ++failures();
  std::cerr << "  [FAIL] " << msg << "\n";
}

inline void check(bool ok, const char* expr, const char* file, int line) {
  ++checks();
  if (!ok) { std::ostringstream os; os << file << ":" << line << ": expected " << expr; fail(os.str()); }
}

inline void check_eq(long long a, long long b, const char* ea, const char* eb, const char* file, int line) {
  ++checks();
  if (a != b) { std::ostringstream os; os << file << ":" << line << ": " << a << " != " << b << " (" << ea << " != " << eb << ")"; fail(os.str()); }
}

inline void check_eq(const std::string& a, const std::string& b, const char* ea, const char* eb, const char* file, int line) {
  ++checks();
  if (a != b) { std::ostringstream os; os << file << ":" << line << ": " << a << " != " << b << " (" << ea << " != " << eb << ")"; fail(os.str()); }
}

inline int run_all() {
  int passed = 0;
  for (auto& t : registry()) {
    current_name() = t.name;
    int before_fail = failures();
    int before_chk = checks();
    try { t.fn(); }
    catch (const std::exception& e) { ++failures(); std::cerr << "  [UNCAUGHT] " << t.name << ": " << e.what() << "\n"; }
    catch (...) { ++failures(); std::cerr << "  [UNCAUGHT] " << t.name << ": unknown exception\n"; }
    if (failures() == before_fail) { ++passed; std::cout << "[PASS] " << t.name << "  (" << (checks() - before_chk) << " checks)\n"; }
    else { std::cout << "[FAIL] " << t.name << "\n"; }
  }
  std::cout << "\nTOTAL: " << checks() << " checks, " << failures() << " failure(s), " << passed << "/" << registry().size() << " test cases passed.\n";
  return failures() == 0 ? 0 : 1;
}

}  // namespace af_test

#define AF_TEST_CASE(name) \
  static const bool af_test_##name = af_test::register_test(#name, [](){}); \
  static void af_test_##name##_body()

#undef AF_TEST_CASE
#define AF_TEST_CASE(name) \
  static void af_test_##name##_impl(); \
  static const bool af_test_##name##_reg = af_test::register_test(#name, af_test_##name##_impl); \
  static void af_test_##name##_impl()

#define AF_CHECK(expr) af_test::check((expr), #expr, __FILE__, __LINE__)
#define AF_CHECK_EQ(a, b) af_test::check_eq((a), (b), #a, #b, __FILE__, __LINE__)
#define AF_CHECK_MSG(expr, msg) do { ++af_test::checks(); if (!(expr)) { std::ostringstream os; os << (msg); os << " ("; os << #expr << ")"; af_test::fail(os.str()); } } while (0)

#define AF_CHECK_THROWS(expr) do { bool did = false; try { expr; } catch (...) { did = true; } ++af_test::checks(); if (!did) { std::ostringstream os; os << __FILE__ << ":" << __LINE__ << ": expected throw from " << #expr; af_test::fail(os.str()); } } while (0)
#define AF_CHECK_NOTHROW(expr) do { bool did = false; try { expr; } catch (const std::exception& e) { did = true; std::ostringstream os; os << __FILE__ << ":" << __LINE__ << ": unexpected throw: " << e.what(); af_test::fail(os.str()); } } while (0)

#define AF_TEST_MAIN int main() { return af_test::run_all(); }