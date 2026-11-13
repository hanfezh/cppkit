#pragma once

#include <cstddef>
#include <iterator>
#include <ranges>
#include <type_traits>

namespace mstd {

inline constexpr std::size_t dynamic_extent = static_cast<std::size_t>(-1);

template <class T>
class span {
 public:
  using element_type = T;
  using value_type = std::remove_cv_t<T>;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;
  using iterator = T*;
  using reverse_iterator = std::reverse_iterator<iterator>;

  // default constructor
  constexpr span() noexcept : data_(nullptr), size_(0) {}

  // Only qualification conversions (U -> const U) are allowed: U(*)[] doesn't
  // convert to T(*)[] for derived-to-base or numeric conversions, which would
  // make the span step through memory with the wrong element size.
  template <class U>
    requires std::is_convertible_v<U (*)[], T (*)[]>
  constexpr span(U* data, size_type size) noexcept : data_(data), size_(size) {}

  constexpr span(std::nullptr_t, size_type size) noexcept
      : data_(nullptr), size_(size) {}

  // A temporary range is only viewable through span<const T>, unless the range
  // is borrowed (its elements outlive it), so a mutable view can't dangle.
  template <class R>
    requires std::ranges::contiguous_range<R> && std::ranges::sized_range<R> &&
             (std::ranges::borrowed_range<R> || std::is_const_v<T>) &&
             std::is_convertible_v<
                 std::remove_reference_t<std::ranges::range_reference_t<R>> (*)[],
                 T (*)[]>
  constexpr span(R&& r) noexcept(noexcept(std::ranges::data(r)) &&
                                 noexcept(std::ranges::size(r)))
      : data_(std::ranges::data(r)), size_(std::ranges::size(r)) {}

  constexpr pointer data() const noexcept { return data_; }

  constexpr size_type size() const noexcept { return size_; }

  constexpr size_type size_bytes() const noexcept { return size_ * sizeof(T); }

  constexpr bool empty() const noexcept { return size_ == 0; }

  constexpr iterator begin() const noexcept { return data_; }

  constexpr iterator end() const noexcept { return data_ + size_; }

  constexpr reverse_iterator rbegin() const noexcept {
    return reverse_iterator(end());
  }

  constexpr reverse_iterator rend() const noexcept {
    return reverse_iterator(begin());
  }

  constexpr reference operator[](size_type i) const { return data_[i]; }

  constexpr reference front() const { return data_[0]; }

  constexpr reference back() const { return data_[size_ - 1]; }

  constexpr span first(size_type count) const { return {data_, count}; }

  constexpr span last(size_type count) const {
    return {data_ + (size_ - count), count};
  }

  constexpr span subspan(size_type offset,
                         size_type count = dynamic_extent) const {
    return {data_ + offset, count == dynamic_extent ? size_ - offset : count};
  }

 private:
  T* data_;
  size_type size_;
};

// Deduction guide
template <class R>
span(R&&) -> span<std::remove_reference_t<std::ranges::range_reference_t<R>>>;

}  // namespace mstd

// mstd::span doesn't own its elements, so a temporary span can be iterated
// safely; this also lets span<int> be built from a span<int> rvalue.
template <class T>
inline constexpr bool std::ranges::enable_borrowed_range<mstd::span<T>> = true;
