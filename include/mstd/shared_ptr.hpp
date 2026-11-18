#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>

namespace mstd {

class ctrl_base {
 public:
  virtual ~ctrl_base() = default;

  void increase() { count_++; }

  void release() {
    if (--count_ == 0) {
      destroy();
    }
  }

  long use_count() const { return count_; }

 protected:
  virtual void destroy() = 0;

 protected:
  long count_ = 1;
};

template <class T>
struct default_delete {
  void operator()(T* p) const { delete p; }
};

template <class T, class D = default_delete<T>>
class ctrl_ptr : public ctrl_base {
 public:
  ctrl_ptr(T* p) : ptr_(p) {}

  ctrl_ptr(T* p, D d) : ptr_(p), deleter_(d) {}

  void destroy() override {
    deleter_(ptr_);
    delete this;
  }

 private:
  T* ptr_ = nullptr;
  D deleter_;
};

template <class T>
class ctrl_inplace : public ctrl_base {
 public:
  template <class... Args>
  ctrl_inplace(Args... args) {
    new (storage_) T(std::forward<Args>(args)...);
  }

  void destroy() override {
    get()->~T();
    delete this;
  }

  T* get() { return reinterpret_cast<T*>(storage_); }

 private:
  alignas(T) unsigned char storage_[sizeof(T)];
};

template <class T>
class shared_ptr {
 private:
  template <class U>
  friend class shared_ptr;

 public:
  shared_ptr() = default;

  shared_ptr(std::nullptr_t) {}

  explicit shared_ptr(T* ptr) : ptr_(ptr) { cb_ = new ctrl_ptr<T>(ptr); }

  template <class Deleter>
  shared_ptr(T* ptr, Deleter d) : ptr_(ptr) {
    cb_ = new ctrl_ptr<T, Deleter>(ptr, d);
  }

  // Copy constructor
  shared_ptr(const shared_ptr& r) : ptr_(r.ptr_), cb_(r.cb_) {
    if (cb_ != nullptr) {
      cb_->increase();
    }
  }

  template <class U>
    requires std::is_convertible_v<U*, T*>
  shared_ptr(const shared_ptr<U>& r) : ptr_(r.ptr_), cb_(r.cb_) {
    if (cb_ != nullptr) {
      cb_->increase();
    }
  }

  shared_ptr(ctrl_base* cb, T* ptr) : ptr_(ptr), cb_(cb) {}

  ~shared_ptr() {
    if (cb_ != nullptr) {
      cb_->release();
    }
  }

  long use_count() const { return cb_ != nullptr ? cb_->use_count() : 0; }

  T* get() const { return ptr_; }

  void reset() {
    if (cb_ != nullptr) {
      cb_->release();
      cb_ = nullptr;
      ptr_ = nullptr;
    }
  }

  void reset(T* ptr) {
    reset();
    ptr_ = ptr;
    cb_ = new ctrl_ptr(ptr);
  }

  void swap(shared_ptr& r) {
    std::swap(ptr_, r.ptr_);
    std::swap(cb_, r.cb_);
  }

  T* operator->() const { return ptr_; }

  T& operator*() const { return *ptr_; }

  explicit operator bool() const { return ptr_ != nullptr; }

  // Copy assignment
  shared_ptr& operator=(const shared_ptr& r) {
    shared_ptr copy(r);
    swap(copy);
    return *this;
  }

  friend bool operator==(const shared_ptr& a, const shared_ptr& b) {
    return a.get() == b.get();
  }

 private:
  T* ptr_ = nullptr;
  ctrl_base* cb_ = nullptr;
};

template <class T, class... Args>
shared_ptr<T> make_shared(Args... args) {
  auto* cb = new ctrl_inplace<T>(std::forward<Args>(args)...);
  return shared_ptr<T>(cb, cb->get());
}

}  // namespace mstd
