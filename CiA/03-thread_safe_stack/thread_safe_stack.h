#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <stack>
#include <utility>

struct empty_stack : std::exception {
  const char *what() const noexcept;
};

template <typename T> class threadsafe_stack {
private:
  std::stack<T> data{};
  mutable std::mutex m{};

public:
  threadsafe_stack() {};

  threadsafe_stack(const threadsafe_stack &other) {
    std::lock_guard<std::mutex> lock(other.m);
    data = other.data; // copy performed in constructor body
  }

  threadsafe_stack &operator=(const threadsafe_stack &) = delete;

  void push(T new_value) {
    std::lock_guard<std::mutex> lock(m);
    data.push(std::move(new_value));
  }

  std::shared_ptr<T> pop() {
    std::lock_guard<std::mutex> lock(m);
    if (data.empty()) {
      std::cout << "Empty\n";
      return nullptr;
    } // check for empty before trying to pop
    std::shared_ptr<T> const res(std::make_shared<T>(data.top())); // allocate return value before modifying stack
    data.pop();
    return res;
  }

  void pop(T &value) {
    std::lock_guard<std::mutex> lock(m);
    if (data.empty()) {
      std::cout << "Empty\n";
      return;
    }
    value = data.top();
    data.pop();
  }

  bool empty() const {
    std::lock_guard<std::mutex> lock(m);
    return data.empty();
  }
};
