// Unit tests for mstd::shared_ptr and mstd::weak_ptr.
//
// Build & run (from the repo root):
//   cmake -S . -B build && cmake --build build && ctest --test-dir build
//
// The sections follow a suggested implementation order, each one building on
// the previous. Until mstd::shared_ptr has a feature, its tests fail to
// compile; build only the span tests meanwhile with
//   cmake --build build --target span_test
//
// The weak_ptr tests are off by default. Turn them on once mstd::weak_ptr
// exists with
//   cmake -S . -B build -DCPPKIT_TEST_WEAK_PTR=ON

#include <mstd/shared_ptr.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

// Counts how many times it's destroyed, so tests can check that the managed
// object is deleted exactly once, and at the right moment.
struct Tracked {
  explicit Tracked(int* destroyed, int value = 0)
      : destroyed(destroyed), value(value) {}
  ~Tracked() { ++*destroyed; }

  int* destroyed;
  int value;
};

// ---------------------------------------------------------------------------
// 1. Basics: owning a single pointer
// ---------------------------------------------------------------------------

TEST(SharedPtrBasics, DefaultIsEmpty) {
  mstd::shared_ptr<int> p;
  EXPECT_EQ(p.get(), nullptr);
  EXPECT_EQ(p.use_count(), 0);
  EXPECT_FALSE(p);
}

TEST(SharedPtrBasics, FromNullptrIsEmpty) {
  mstd::shared_ptr<int> p(nullptr);
  EXPECT_EQ(p.get(), nullptr);
  EXPECT_EQ(p.use_count(), 0);
}

TEST(SharedPtrBasics, OwnsRawPointer) {
  int* raw = new int(42);
  mstd::shared_ptr<int> p(raw);
  EXPECT_EQ(p.get(), raw);
  EXPECT_EQ(p.use_count(), 1);
  EXPECT_TRUE(p);
}

TEST(SharedPtrBasics, DereferenceAndArrow) {
  mstd::shared_ptr<std::string> p(new std::string("hello"));
  EXPECT_EQ(*p, "hello");
  EXPECT_EQ(p->size(), 5u);
  *p = "bye";
  EXPECT_EQ(*p, "bye");
}

TEST(SharedPtrBasics, DestructorDeletesObject) {
  int destroyed = 0;
  {
    mstd::shared_ptr<Tracked> p(new Tracked(&destroyed));
    EXPECT_EQ(destroyed, 0);
  }
  EXPECT_EQ(destroyed, 1);
}

TEST(SharedPtrBasics, EmptyDestructorDoesNothing) {
  { mstd::shared_ptr<Tracked> p; }
  SUCCEED();
}

TEST(SharedPtrBasics, ConversionsAreExplicit) {
  // Implicit int* -> shared_ptr would let a function call silently take
  // ownership of (and later delete) a pointer it doesn't own.
  static_assert(!std::is_convertible_v<int*, mstd::shared_ptr<int>>);
  static_assert(std::is_constructible_v<mstd::shared_ptr<int>, int*>);
  // `if (p)` works, but `int n = p;` or `p + 1` must not compile.
  static_assert(!std::is_convertible_v<mstd::shared_ptr<int>, bool>);
  static_assert(std::is_constructible_v<bool, mstd::shared_ptr<int>>);
}

// ---------------------------------------------------------------------------
// 2. Copy and move: sharing ownership
// ---------------------------------------------------------------------------

TEST(SharedPtrCopy, CopySharesOwnership) {
  mstd::shared_ptr<int> a(new int(7));
  mstd::shared_ptr<int> b(a);
  EXPECT_EQ(a.get(), b.get());
  EXPECT_EQ(a.use_count(), 2);
  EXPECT_EQ(b.use_count(), 2);
}

TEST(SharedPtrCopy, LastOwnerDeletes) {
  int destroyed = 0;
  mstd::shared_ptr<Tracked> a(new Tracked(&destroyed));
  {
    mstd::shared_ptr<Tracked> b(a);
    mstd::shared_ptr<Tracked> c(b);
    EXPECT_EQ(a.use_count(), 3);
  }
  EXPECT_EQ(destroyed, 0);
  EXPECT_EQ(a.use_count(), 1);
  a = mstd::shared_ptr<Tracked>();
  EXPECT_EQ(destroyed, 1);
}

TEST(SharedPtrCopy, CopyOfEmptyIsEmpty) {
  mstd::shared_ptr<int> a;
  mstd::shared_ptr<int> b(a);
  EXPECT_EQ(b.get(), nullptr);
  EXPECT_EQ(b.use_count(), 0);
}

TEST(SharedPtrCopy, CopyAssignReleasesOldObject) {
  int destroyed_a = 0;
  int destroyed_b = 0;
  mstd::shared_ptr<Tracked> a(new Tracked(&destroyed_a));
  mstd::shared_ptr<Tracked> b(new Tracked(&destroyed_b));
  a = b;
  EXPECT_EQ(destroyed_a, 1);
  EXPECT_EQ(destroyed_b, 0);
  EXPECT_EQ(a.get(), b.get());
  EXPECT_EQ(b.use_count(), 2);
}

TEST(SharedPtrCopy, SelfCopyAssignIsSafe) {
  int destroyed = 0;
  mstd::shared_ptr<Tracked> a(new Tracked(&destroyed, 5));
  auto& alias = a;  // avoids -Wself-assign-overloaded
  a = alias;
  EXPECT_EQ(destroyed, 0);
  EXPECT_EQ(a.use_count(), 1);
  EXPECT_EQ(a->value, 5);
}

TEST(SharedPtrCopy, AssignSharedObjectToItself) {
  // Two owners of the same object: assigning one to the other must not drop
  // the count to zero in between.
  int destroyed = 0;
  mstd::shared_ptr<Tracked> a(new Tracked(&destroyed));
  mstd::shared_ptr<Tracked> b(a);
  a = b;
  EXPECT_EQ(destroyed, 0);
  EXPECT_EQ(a.use_count(), 2);
}

TEST(SharedPtrMove, MoveTransfersOwnership) {
  int* raw = new int(3);
  mstd::shared_ptr<int> a(raw);
  mstd::shared_ptr<int> b(std::move(a));
  EXPECT_EQ(b.get(), raw);
  EXPECT_EQ(b.use_count(), 1);  // moving doesn't add an owner
  EXPECT_EQ(a.get(), nullptr);  // NOLINT(bugprone-use-after-move)
  EXPECT_EQ(a.use_count(), 0);
}

TEST(SharedPtrMove, MoveAssignReleasesOldObject) {
  int destroyed_a = 0;
  int destroyed_b = 0;
  mstd::shared_ptr<Tracked> a(new Tracked(&destroyed_a));
  mstd::shared_ptr<Tracked> b(new Tracked(&destroyed_b));
  a = std::move(b);
  EXPECT_EQ(destroyed_a, 1);
  EXPECT_EQ(destroyed_b, 0);
  EXPECT_EQ(a.use_count(), 1);
  EXPECT_EQ(b.get(), nullptr);  // NOLINT(bugprone-use-after-move)
}

TEST(SharedPtrMove, SelfMoveAssignKeepsObject) {
  int destroyed = 0;
  mstd::shared_ptr<Tracked> a(new Tracked(&destroyed));
  auto& alias = a;  // avoids -Wself-move
  a = std::move(alias);
  EXPECT_EQ(destroyed, 0);
}

// ---------------------------------------------------------------------------
// 3. reset, swap and comparisons
// ---------------------------------------------------------------------------

TEST(SharedPtrModify, ResetReleasesOwnership) {
  int destroyed = 0;
  mstd::shared_ptr<Tracked> p(new Tracked(&destroyed));
  p.reset();
  EXPECT_EQ(destroyed, 1);
  EXPECT_EQ(p.get(), nullptr);
  EXPECT_EQ(p.use_count(), 0);
}

TEST(SharedPtrModify, ResetWithNewPointer) {
  int destroyed_old = 0;
  int destroyed_new = 0;
  mstd::shared_ptr<Tracked> p(new Tracked(&destroyed_old));
  auto* fresh = new Tracked(&destroyed_new);
  p.reset(fresh);
  EXPECT_EQ(destroyed_old, 1);
  EXPECT_EQ(p.get(), fresh);
  EXPECT_EQ(p.use_count(), 1);
}

TEST(SharedPtrModify, ResetOneOwnerKeepsObjectForOthers) {
  int destroyed = 0;
  mstd::shared_ptr<Tracked> a(new Tracked(&destroyed));
  mstd::shared_ptr<Tracked> b(a);
  a.reset();
  EXPECT_EQ(destroyed, 0);
  EXPECT_EQ(b.use_count(), 1);
}

TEST(SharedPtrModify, Swap) {
  int* x = new int(1);
  int* y = new int(2);
  mstd::shared_ptr<int> a(x);
  mstd::shared_ptr<int> b(y);
  mstd::shared_ptr<int> b2(b);
  a.swap(b);
  EXPECT_EQ(a.get(), y);
  EXPECT_EQ(b.get(), x);
  EXPECT_EQ(a.use_count(), 2);
  EXPECT_EQ(b.use_count(), 1);
}

TEST(SharedPtrCompare, EqualityAndNullptr) {
  mstd::shared_ptr<int> a(new int(1));
  mstd::shared_ptr<int> b(a);
  mstd::shared_ptr<int> c(new int(1));
  mstd::shared_ptr<int> empty;
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a == c);  // same value, different objects
  EXPECT_TRUE(a != c);
  EXPECT_TRUE(empty == nullptr);
  EXPECT_TRUE(a != nullptr);
}

// ---------------------------------------------------------------------------
// 4. Custom deleters
// ---------------------------------------------------------------------------

TEST(SharedPtrDeleter, CustomDeleterIsCalledOnce) {
  int calls = 0;
  int value = 9;  // not heap-allocated, so `delete` would be wrong here
  {
    mstd::shared_ptr<int> a(&value, [&calls](int*) { ++calls; });
    mstd::shared_ptr<int> b(a);
  }
  EXPECT_EQ(calls, 1);
}

TEST(SharedPtrDeleter, DeleterReceivesTheOwnedPointer) {
  int* seen = nullptr;
  int value = 0;
  { mstd::shared_ptr<int> p(&value, [&seen](int* q) { seen = q; }); }
  EXPECT_EQ(seen, &value);
}

TEST(SharedPtrDeleter, DeleterDoesNotChangeType) {
  // Unlike unique_ptr, the deleter isn't part of shared_ptr's type: it lives
  // in the control block, so differently-deleted pointers mix freely.
  int calls = 0;
  int value = 0;
  mstd::shared_ptr<int> a(&value, [&calls](int*) { ++calls; });
  mstd::shared_ptr<int> b(new int(1));
  static_assert(std::is_same_v<decltype(a), decltype(b)>);
  b = a;
  EXPECT_EQ(calls, 0);
}

// ---------------------------------------------------------------------------
// 5. make_shared
// ---------------------------------------------------------------------------

struct Point {
  Point(int x, int y) : x(x), y(y) {}
  int x;
  int y;
};

TEST(MakeShared, ForwardsConstructorArguments) {
  auto p = mstd::make_shared<Point>(3, 4);
  static_assert(std::is_same_v<decltype(p), mstd::shared_ptr<Point>>);
  EXPECT_EQ(p->x, 3);
  EXPECT_EQ(p->y, 4);
  EXPECT_EQ(p.use_count(), 1);
}

TEST(MakeShared, ForwardsRvalues) {
  std::string s(100, 'x');
  auto p = mstd::make_shared<std::string>(std::move(s));
  EXPECT_EQ(p->size(), 100u);
}

TEST(MakeShared, DestroysObjectWhenLastOwnerGoes) {
  int destroyed = 0;
  {
    auto a = mstd::make_shared<Tracked>(&destroyed);
    auto b = a;
  }
  EXPECT_EQ(destroyed, 1);
}

// ---------------------------------------------------------------------------
// 6. Derived-to-base conversion
// ---------------------------------------------------------------------------

struct Base {
  int base_value = 1;
  // Deliberately NOT virtual: see DeletesThroughOriginalType.
  ~Base() = default;
};

struct Derived : Base {
  explicit Derived(int* destroyed) : destroyed(destroyed) {}
  ~Derived() { ++*destroyed; }
  int* destroyed;
};

TEST(SharedPtrConvert, DerivedToBase) {
  int destroyed = 0;
  mstd::shared_ptr<Derived> d(new Derived(&destroyed));
  mstd::shared_ptr<Base> b(d);
  EXPECT_EQ(b.get(), d.get());
  EXPECT_EQ(d.use_count(), 2);
  EXPECT_EQ(b->base_value, 1);
}

TEST(SharedPtrConvert, ImplicitDerivedToBaseAndRejectsOthers) {
  static_assert(std::is_convertible_v<mstd::shared_ptr<Derived>,
                                      mstd::shared_ptr<Base>>);
  static_assert(std::is_convertible_v<mstd::shared_ptr<int>,
                                      mstd::shared_ptr<const int>>);
  static_assert(!std::is_convertible_v<mstd::shared_ptr<Base>,
                                       mstd::shared_ptr<Derived>>);
  static_assert(!std::is_constructible_v<mstd::shared_ptr<int>,
                                         mstd::shared_ptr<long>>);
}

TEST(SharedPtrConvert, DeletesThroughOriginalType) {
  // ~Base isn't virtual, so `delete base_ptr` would skip ~Derived. shared_ptr
  // must remember the type it was created with (in the control block) and
  // delete through a Derived*.
  int destroyed = 0;
  { mstd::shared_ptr<Base> b(new Derived(&destroyed)); }
  EXPECT_EQ(destroyed, 1);

  destroyed = 0;
  {
    mstd::shared_ptr<Base> b;
    b = mstd::make_shared<Derived>(&destroyed);
  }
  EXPECT_EQ(destroyed, 1);
}

// ---------------------------------------------------------------------------
// 7. weak_ptr: observing without owning
//
// Enable with -DCPPKIT_TEST_WEAK_PTR=ON.
// ---------------------------------------------------------------------------

#ifdef MSTD_TEST_WEAK_PTR

TEST(WeakPtr, DefaultIsExpired) {
  mstd::weak_ptr<int> w;
  EXPECT_TRUE(w.expired());
  EXPECT_EQ(w.use_count(), 0);
  EXPECT_EQ(w.lock().get(), nullptr);
}

TEST(WeakPtr, DoesNotAddAnOwner) {
  auto p = mstd::make_shared<int>(5);
  mstd::weak_ptr<int> w(p);
  EXPECT_EQ(p.use_count(), 1);
  EXPECT_EQ(w.use_count(), 1);
  EXPECT_FALSE(w.expired());
}

TEST(WeakPtr, LockReturnsOwnerWhileAlive) {
  auto p = mstd::make_shared<int>(5);
  mstd::weak_ptr<int> w(p);
  mstd::shared_ptr<int> locked = w.lock();
  EXPECT_EQ(locked.get(), p.get());
  EXPECT_EQ(p.use_count(), 2);
}

TEST(WeakPtr, ExpiresWhenLastOwnerGoes) {
  int destroyed = 0;
  auto p = mstd::make_shared<Tracked>(&destroyed);
  mstd::weak_ptr<Tracked> w(p);
  p.reset();
  // The object is gone even though a weak_ptr still exists...
  EXPECT_EQ(destroyed, 1);
  EXPECT_TRUE(w.expired());
  // ...and lock() must not resurrect it.
  EXPECT_EQ(w.lock().get(), nullptr);
}

TEST(WeakPtr, OutlivingWeakPtrIsSafe) {
  // The control block must outlive the object while weak_ptrs still point at
  // it; destroying the weak_ptr last must not touch freed memory (run under
  // ASan to catch it).
  mstd::weak_ptr<int> w;
  {
    auto p = mstd::make_shared<int>(1);
    w = p;
  }
  EXPECT_TRUE(w.expired());
}

TEST(WeakPtr, CopyAndReset) {
  auto p = mstd::make_shared<int>(1);
  mstd::weak_ptr<int> a(p);
  mstd::weak_ptr<int> b(a);
  EXPECT_FALSE(b.expired());
  a.reset();
  EXPECT_TRUE(a.expired());
  EXPECT_FALSE(b.expired());
}

TEST(WeakPtr, BreaksReferenceCycle) {
  // Two nodes owning each other with shared_ptr would never be freed; making
  // the back link a weak_ptr lets both go.
  struct Node {
    explicit Node(int* destroyed) : destroyed(destroyed) {}
    ~Node() { ++*destroyed; }
    int* destroyed;
    mstd::shared_ptr<Node> next;
    mstd::weak_ptr<Node> prev;
  };
  int destroyed = 0;
  {
    auto a = mstd::make_shared<Node>(&destroyed);
    auto b = mstd::make_shared<Node>(&destroyed);
    a->next = b;
    b->prev = a;
  }
  EXPECT_EQ(destroyed, 2);
}

#endif  // MSTD_TEST_WEAK_PTR

// ---------------------------------------------------------------------------
// 8. Thread safety of the reference count
// ---------------------------------------------------------------------------

TEST(SharedPtrThreads, ConcurrentCopiesKeepCountExact) {
  // Each thread copies and destroys its own shared_ptr to one shared object.
  // With a non-atomic count, increments get lost and the object is freed too
  // early or never. Flaky failures here mean a data race; run under TSan to
  // see it reliably.
  int destroyed = 0;
  auto p = mstd::make_shared<Tracked>(&destroyed);
  std::atomic<bool> go{false};
  std::vector<std::thread> threads;
  for (int t = 0; t < 8; ++t) {
    threads.emplace_back([&p, &go] {
      mstd::shared_ptr<Tracked> local(p);
      while (!go.load()) {
      }
      for (int i = 0; i < 20000; ++i) {
        mstd::shared_ptr<Tracked> copy(local);
      }
    });
  }
  go = true;
  for (auto& t : threads) t.join();
  EXPECT_EQ(p.use_count(), 1);
  EXPECT_EQ(destroyed, 0);
  p.reset();
  EXPECT_EQ(destroyed, 1);
}

#ifdef MSTD_TEST_WEAK_PTR

TEST(SharedPtrThreads, ConcurrentLockAndReset) {
  // Threads keep trying weak_ptr::lock() while the main thread drops the last
  // owner: every lock either gets the live object or an empty pointer, and
  // the object is destroyed exactly once.
  for (int round = 0; round < 50; ++round) {
    int destroyed = 0;
    auto p = mstd::make_shared<Tracked>(&destroyed, 7);
    mstd::weak_ptr<Tracked> w(p);
    std::atomic<bool> bad{false};
    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
      threads.emplace_back([w, &bad] {
        for (int i = 0; i < 1000; ++i) {
          if (auto s = w.lock(); s && s->value != 7) bad = true;
        }
      });
    }
    p.reset();
    for (auto& t : threads) t.join();
    EXPECT_FALSE(bad.load());
    EXPECT_EQ(destroyed, 1);
  }
}

#endif  // MSTD_TEST_WEAK_PTR

}  // namespace
