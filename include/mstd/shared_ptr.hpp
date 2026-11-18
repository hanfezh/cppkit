#pragma once

#include <cstddef>
#include <utility>

namespace mstd {

class ctrl_base {
 public:
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
class ctrl_ptr : public ctrl_base {
 public:
  ctrl_ptr(T* p) : ptr_(p) {}

  void destroy() override {
    delete ptr_;
    delete this;
  }

 private:
  T* ptr_ = nullptr;
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
 public:
  shared_ptr() = default;

  shared_ptr(std::nullptr_t) {}

  shared_ptr(T* p) : ptr_(p) { cb_ = new ctrl_ptr(p); }

  shared_ptr(const shared_ptr<T>& p) : cb_(p.cb_), ptr_(p.ptr_) {
    cb_->increase();
  }

  shared_ptr(ctrl_base* cb, T* p) : cb_(cb), ptr_(p) {}

  ~shared_ptr() {
    if (cb_ != nullptr) {
      cb_->release();
    }
  }

  long use_count() const { return cb_ != nullptr ? cb_->use_count() : 0; }

  T* get() const { return ptr_; }

  T* operator->() const { return ptr_; }

  T& operator*() const { return *ptr_; }

 private:
  ctrl_base* cb_ = nullptr;
  T* ptr_ = nullptr;
};

template <class T, class... Args>
shared_ptr<T> make_shared(Args... args) {
  auto* cb = new ctrl_inplace<T>(std::forward<Args>(args)...);
  return shared_ptr<T>(cb, cb->get());
}

}  // namespace mstd
