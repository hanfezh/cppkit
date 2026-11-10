#pragma once

#include <cstddef>

namespace mstd {

template <class T>
class span {
 public:
  using iterator = T*;
  using reference = T&;

  template <class Container>
  span(Container& c) : data_(c.data()), size_(c.size()) {}

  span(T* data, size_t size) : data_(data), size_(size) {}

  T* data() const { return data_; }

  size_t size() const { return size_; }

  bool empty() const { return size_ == 0; }

  iterator begin() const { return data_; }

  iterator end() const { return data_ + size_; }

  reference operator[](size_t i) const { return data_[i]; }

 private:
  T* data_;
  size_t size_;
};

}  // namespace mstd
