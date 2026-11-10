// Unit tests for mstd::span.
//
// Build & run (from the repo root):
//   cmake -S . -B build && cmake --build build && ctest --test-dir build
//
// The default build tests the API mstd::span has today. Configure with
// -DCPPKIT_SPAN_CONFORMANCE=ON to also compile the checks for std::span
// behaviour that mstd::span doesn't have yet (they fail to compile or fail
// at runtime until it does).

#include <mstd/span.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <numeric>
#include <string>
#include <type_traits>
#include <vector>

namespace {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TEST(SpanConstruct, FromPointerAndSize) {
  int arr[] = {1, 2, 3, 4};
  mstd::span<int> s(arr, 4);
  EXPECT_EQ(s.data(), arr);
  EXPECT_EQ(s.size(), 4u);
  EXPECT_FALSE(s.empty());
}

TEST(SpanConstruct, FromPointerAndSizeSubrange) {
  int arr[] = {1, 2, 3, 4, 5};
  mstd::span<int> s(arr + 1, 3);
  ASSERT_EQ(s.size(), 3u);
  EXPECT_EQ(s[0], 2);
  EXPECT_EQ(s[2], 4);
}

TEST(SpanConstruct, FromNullptrAndZeroIsEmpty) {
  mstd::span<int> s(nullptr, 0);
  EXPECT_TRUE(s.empty());
  EXPECT_EQ(s.size(), 0u);
  EXPECT_EQ(s.data(), nullptr);
  EXPECT_EQ(s.begin(), s.end());
}

TEST(SpanConstruct, FromVector) {
  std::vector<int> v = {10, 20, 30};
  mstd::span<int> s(v);
  EXPECT_EQ(s.data(), v.data());
  EXPECT_EQ(s.size(), v.size());
}

TEST(SpanConstruct, FromStdArray) {
  std::array<int, 3> a = {7, 8, 9};
  mstd::span<int> s(a);
  EXPECT_EQ(s.data(), a.data());
  EXPECT_EQ(s.size(), 3u);
}

TEST(SpanConstruct, FromString) {
  std::string str = "hello";
  mstd::span<char> s(str);
  ASSERT_EQ(s.size(), 5u);
  EXPECT_EQ(s[0], 'h');
  EXPECT_EQ(s[4], 'o');
}

TEST(SpanConstruct, ConstSpanFromConstVector) {
  const std::vector<int> v = {1, 2, 3};
  mstd::span<const int> s(v);
  EXPECT_EQ(s.data(), v.data());
  EXPECT_EQ(s.size(), 3u);
}

TEST(SpanConstruct, ConstSpanFromMutableVector) {
  std::vector<int> v = {1, 2, 3};
  mstd::span<const int> s(v);
  EXPECT_EQ(s.data(), v.data());
  EXPECT_EQ(s.size(), 3u);
}

TEST(SpanConstruct, FromEmptyVector) {
  std::vector<int> v;
  mstd::span<int> s(v);
  EXPECT_TRUE(s.empty());
  EXPECT_EQ(s.begin(), s.end());
}

TEST(SpanConstruct, ImplicitConversionAtCallSite) {
  auto sum = [](mstd::span<const int> s) {
    int total = 0;
    for (int x : s) total += x;
    return total;
  };
  std::vector<int> v = {1, 2, 3, 4};
  std::array<int, 2> a = {5, 6};
  EXPECT_EQ(sum(v), 10);
  EXPECT_EQ(sum(a), 11);
}

// ---------------------------------------------------------------------------
// Copy semantics: span is a non-owning view, so copies alias the same data.
// ---------------------------------------------------------------------------

TEST(SpanCopy, IsShallow) {
  std::vector<int> v = {1, 2, 3};
  mstd::span<int> a(v);
  mstd::span<int> b = a;
  EXPECT_EQ(b.data(), a.data());
  EXPECT_EQ(b.size(), a.size());
  b[0] = 42;
  EXPECT_EQ(a[0], 42);
  EXPECT_EQ(v[0], 42);
}

TEST(SpanCopy, FromConstSpan) {
  std::vector<int> v = {1, 2, 3};
  const mstd::span<int> a(v);
  mstd::span<int> b = a;
  EXPECT_EQ(b.data(), v.data());
  EXPECT_EQ(b.size(), 3u);
}

TEST(SpanCopy, AssignmentRebindsView) {
  std::vector<int> v1 = {1, 2, 3};
  std::vector<int> v2 = {4, 5};
  mstd::span<int> s(v1);
  s = mstd::span<int>(v2);
  EXPECT_EQ(s.data(), v2.data());
  EXPECT_EQ(s.size(), 2u);
  EXPECT_EQ(v1[0], 1);  // assignment must not touch the old elements
}

TEST(SpanCopy, MutableSpanToConstSpan) {
  std::vector<int> v = {1, 2, 3};
  mstd::span<int> s(v);
  mstd::span<const int> cs(s);
  EXPECT_EQ(cs.data(), v.data());
  EXPECT_EQ(cs.size(), 3u);
}

// ---------------------------------------------------------------------------
// Element access and mutation
// ---------------------------------------------------------------------------

TEST(SpanAccess, SubscriptReadsElements) {
  int arr[] = {5, 6, 7};
  mstd::span<int> s(arr, 3);
  EXPECT_EQ(s[0], 5);
  EXPECT_EQ(s[1], 6);
  EXPECT_EQ(s[2], 7);
}

TEST(SpanAccess, SubscriptWritesThroughToUnderlyingData) {
  std::vector<int> v = {0, 0, 0};
  mstd::span<int> s(v);
  s[1] = 99;
  EXPECT_EQ(v[1], 99);
}

TEST(SpanAccess, ConstSpanObjectStillAllowsMutation) {
  // Like std::span, constness of the span object is shallow: a const
  // span<int> still hands out int&. Use span<const int> for read-only access.
  std::vector<int> v = {1, 2, 3};
  const mstd::span<int> s(v);
  s[0] = 100;
  *s.begin() += 1;
  EXPECT_EQ(v[0], 101);
}

TEST(SpanAccess, ViewObservesLaterChangesToUnderlyingData) {
  std::vector<int> v = {1, 2, 3};
  mstd::span<const int> s(v);
  v[2] = 30;
  EXPECT_EQ(s[2], 30);
}

// ---------------------------------------------------------------------------
// Iteration
// ---------------------------------------------------------------------------

TEST(SpanIterate, BeginEndBoundTheElements) {
  int arr[] = {1, 2, 3};
  mstd::span<int> s(arr, 3);
  EXPECT_EQ(s.begin(), arr);
  EXPECT_EQ(s.end(), arr + 3);
  EXPECT_EQ(s.end() - s.begin(), 3);
}

TEST(SpanIterate, RangeForVisitsEveryElementInOrder) {
  std::vector<int> v = {1, 2, 3, 4};
  mstd::span<int> s(v);
  std::vector<int> seen;
  for (int x : s) seen.push_back(x);
  EXPECT_EQ(seen, v);
}

TEST(SpanIterate, RangeForByReferenceMutates) {
  std::vector<int> v = {1, 2, 3};
  mstd::span<int> s(v);
  for (int& x : s) x *= 10;
  EXPECT_EQ(v, (std::vector<int>{10, 20, 30}));
}

TEST(SpanIterate, WorksWithStandardAlgorithms) {
  std::vector<int> v = {1, 2, 3, 4, 5};
  mstd::span<const int> s(v);
  EXPECT_EQ(std::accumulate(s.begin(), s.end(), 0), 15);
}

// ---------------------------------------------------------------------------
// Member types and compile-time properties
// ---------------------------------------------------------------------------

TEST(SpanTraits, MemberTypes) {
  static_assert(std::is_same_v<mstd::span<int>::iterator, int*>);
  static_assert(std::is_same_v<mstd::span<int>::reference, int&>);
  static_assert(std::is_same_v<mstd::span<const int>::reference, const int&>);
}

TEST(SpanTraits, IsTriviallyCopyable) {
  // A span is just a pointer and a size; it should be cheap to pass by value.
  static_assert(std::is_trivially_copyable_v<mstd::span<int>>);
  static_assert(sizeof(mstd::span<int>) == sizeof(int*) + sizeof(std::size_t));
}

TEST(SpanTraits, ConstSpanRejectsWrites) {
  static_assert(!std::is_assignable_v<mstd::span<const int>::reference, int>);
}

// ---------------------------------------------------------------------------
// std::span conformance gaps (see the review). Enable with
// -DCPPKIT_SPAN_CONFORMANCE=ON.
// ---------------------------------------------------------------------------

#ifdef MSTD_SPAN_CONFORMANCE

struct Base {
  int x;
};
struct Derived : Base {
  int y;
};

TEST(SpanConformance, DefaultConstructor) {
  mstd::span<int> s;
  EXPECT_TRUE(s.empty());
  EXPECT_EQ(s.data(), nullptr);
}

TEST(SpanConformance, ConstructFromCArray) {
  int arr[] = {1, 2, 3};
  mstd::span<int> s(arr);
  EXPECT_EQ(s.size(), 3u);
}

TEST(SpanConformance, CtadFromContainer) {
  std::vector<int> v = {1, 2, 3};
  mstd::span s(v);
  static_assert(std::is_same_v<decltype(s), mstd::span<int>>);
}

TEST(SpanConformance, RejectsNonContainers) {
  static_assert(!std::is_convertible_v<int&, mstd::span<int>>);
  static_assert(!std::is_constructible_v<mstd::span<int>, int&>);
}

TEST(SpanConformance, RejectsDerivedToBase) {
  // A span<Base> over Derived objects would step by sizeof(Base) and read
  // garbage; std::span only allows qualification conversions (T -> const T).
  static_assert(
      !std::is_constructible_v<mstd::span<Base>, std::vector<Derived>&>);
}

TEST(SpanConformance, Constexpr) {
  static constexpr int arr[] = {1, 2, 3};
  constexpr mstd::span<const int> s(arr, 3);
  static_assert(s.size() == 3);
  static_assert(s[1] == 2);
}

TEST(SpanConformance, StandardMemberTypes) {
  using S = mstd::span<const int>;
  static_assert(std::is_same_v<S::element_type, const int>);
  static_assert(std::is_same_v<S::value_type, int>);
  static_assert(std::is_same_v<S::size_type, std::size_t>);
  static_assert(std::is_same_v<S::difference_type, std::ptrdiff_t>);
  static_assert(std::is_same_v<S::pointer, const int*>);
}

TEST(SpanConformance, FrontBack) {
  std::vector<int> v = {1, 2, 3};
  mstd::span<int> s(v);
  EXPECT_EQ(s.front(), 1);
  EXPECT_EQ(s.back(), 3);
}

TEST(SpanConformance, FirstLastSubspan) {
  std::vector<int> v = {1, 2, 3, 4, 5};
  mstd::span<int> s(v);
  EXPECT_EQ(s.first(2).size(), 2u);
  EXPECT_EQ(s.first(2)[1], 2);
  EXPECT_EQ(s.last(2)[0], 4);
  EXPECT_EQ(s.subspan(1, 3)[0], 2);
  EXPECT_EQ(s.subspan(1, 3).size(), 3u);
  EXPECT_EQ(s.subspan(3).size(), 2u);
}

TEST(SpanConformance, SizeBytes) {
  std::vector<int> v = {1, 2, 3};
  mstd::span<int> s(v);
  EXPECT_EQ(s.size_bytes(), 3 * sizeof(int));
}

TEST(SpanConformance, ReverseIteration) {
  std::vector<int> v = {1, 2, 3};
  mstd::span<int> s(v);
  std::vector<int> seen(s.rbegin(), s.rend());
  EXPECT_EQ(seen, (std::vector<int>{3, 2, 1}));
}

#endif  // MSTD_SPAN_CONFORMANCE

}  // namespace
