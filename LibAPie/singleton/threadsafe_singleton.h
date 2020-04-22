#pragma once

#include <memory>
#include <mutex>

namespace Envoy {


template <class T> class ThreadSafeSingleton {
public:
  static T& get() 
  {
	  std::call_once(ThreadSafeSingleton<T>::create_once_, &ThreadSafeSingleton<T>::Create);
	  return *ThreadSafeSingleton<T>::instance_;
  }

protected:
  template <typename A> friend class TestThreadsafeSingletonInjector;

  static void Create() { instance_ = new T(); }

  static std::once_flag create_once_;
  static T* instance_;
};

template <class T> std::once_flag ThreadSafeSingleton<T>::create_once_;

template <class T> T* ThreadSafeSingleton<T>::instance_ = nullptr;

} // namespace Envoy
