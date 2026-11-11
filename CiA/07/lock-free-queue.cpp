#include <atomic>
#include <memory>

template <typename T>
class lock_free_queue_spsc {
private:
  struct node {
    std::shared_ptr<T> data;
    node *next;
    node() : next(nullptr) {}
  };
  std::atomic<node *> head;
  std::atomic<node *> tail;

  node *pop_head() {
    node *const old_head = head.load();
    if (old_head == tail.load()) { // 1
      return nullptr;
    }
    head.store(old_head->next);
    return old_head;
  }

public:
  lock_free_queue_spsc() : head(new node), tail(head.load()) {}

  lock_free_queue_spsc(const lock_free_queue_spsc &other) = delete;

  lock_free_queue_spsc &operator=(const lock_free_queue_spsc &other) = delete;

  ~lock_free_queue_spsc() {
    while (node *const old_head = head.load()) {
      head.store(old_head->next);
      delete old_head;
    }
  }
  std::shared_ptr<T> pop() {
    node *old_head = pop_head();
    if (!old_head) {
      return std::shared_ptr<T>();
    }
    std::shared_ptr<T> const res(old_head->data); // 2
    delete old_head;
    return res;
  }
  void push(T new_value) {
    std::shared_ptr<T> new_data(std::make_shared<T>(new_value));
    node *p = new node;                 // 3
    node *const old_tail = tail.load(); // 4
    old_tail->data.swap(new_data);      // 5
    old_tail->next = p;                 // 6
    tail.store(p);                      // 7
  }
};

template <typename T>
class lock_free_queue_reference_counted_tail {
private:
  struct node;
  struct counted_node_ptr {
    int external_count;
    node *ptr;
  };
  std::atomic<counted_node_ptr> head;
  std::atomic<counted_node_ptr> tail;
  struct node_counter {
    unsigned internal_count : 30;
    unsigned external_counters : 2;
  };
  struct node {
    std::atomic<T *> data;
    std::atomic<node_counter> count;
    std::atomic<counted_node_ptr> next;
    node() {
      node_counter new_count;
      new_count.internal_count = 0;
      new_count.external_counters = 2;
      count.store(new_count);
      next.ptr = nullptr;
      next.external_count = 0;
    }
    void release_ref() {
      node_counter old_counter = count.load(std::memory_order_relaxed);
      node_counter new_counter;
      do {
        new_counter = old_counter;
        --new_counter.internal_count;                                   // 1
      } while (!count.compare_exchange_strong(old_counter, new_counter, // 2
                                              std::memory_order_acquire,
                                              std::memory_order_relaxed));
      if (!new_counter.internal_count && !new_counter.external_counters) {
        delete this; // 3
      }
    }
  };

  void set_new_tail(counted_node_ptr &old_tail,
                    counted_node_ptr &new_tail) { // 1
    node *const current_tail_ptr = old_tail.ptr;
    while (!tail.compare_exchange_weak(old_tail, new_tail) && // 2
           old_tail.ptr == current_tail_ptr)
      ;
    if (old_tail.ptr == current_tail_ptr) // 3
      free_external_counter(old_tail);    // 4
    else
      current_tail_ptr->release_ref(); // 5
  }

public:
  void push(T new_value) {
    std::unique_ptr<T> new_data(new T(new_value));
    counted_node_ptr new_next;
    new_next.ptr = new node;
    new_next.external_count = 1;
    counted_node_ptr old_tail = tail.load();
    for (;;) {
      increase_external_count(tail, old_tail);
      T *old_data = nullptr;
      if (old_tail.ptr->data.compare_exchange_strong(old_data, // 6
                                                     new_data.get())) {
        counted_node_ptr old_next = {0};
        if (!old_tail.ptr->next.compare_exchange_strong(old_next,
                                                        new_next)) { // 7
          delete new_next.ptr;                                       // 8
          new_next = old_next;                                       // 9
        }
        set_new_tail(old_tail, new_next);
        new_data.release();
        break;
      } else { // 10
        counted_node_ptr old_next = {0};
        if (old_tail.ptr->next.compare_exchange_strong(old_next,
                                                       new_next)) { // 11
          old_next = new_next;                                      // 12
          new_next.ptr = new node;                                  // 13
        }
        set_new_tail(old_tail, old_next); // 14
      }
    }
  }

  std::unique_ptr<T> pop() {
    counted_node_ptr old_head = head.load(std::memory_order_relaxed); // 1
    for (;;) {
      increase_external_count(head, old_head); // 2
      node *const ptr = old_head.ptr;
      if (ptr == tail.load().ptr) {
        return std::unique_ptr<T>();
      }
      counted_node_ptr next = ptr->next.load();
      if (head.compare_exchange_strong(old_head, ptr->next)) { // 4
        T *const res = ptr->data.exchange(nullptr);
        free_external_counter(old_head); // 5
        return std::unique_ptr<T>(res);
      }
      ptr->release_ref(); // 6
    }
  }

private:
  static void increase_external_count(std::atomic<counted_node_ptr> &counter,
                                      counted_node_ptr &old_counter) {
    counted_node_ptr new_counter;
    do {
      new_counter = old_counter;
      ++new_counter.external_count;
    } while (!counter.compare_exchange_strong(old_counter, new_counter,
                                              std::memory_order_acquire,
                                              std::memory_order_relaxed));
    old_counter.external_count = new_counter.external_count;
  }

  static void free_external_counter(counted_node_ptr &old_node_ptr) {
    node *const ptr = old_node_ptr.ptr;
    int const count_increase = old_node_ptr.external_count - 2;
    node_counter old_counter = ptr->count.load(std::memory_order_relaxed);
    node_counter new_counter;
    do {
      new_counter = old_counter;
      --new_counter.external_counters;
      new_counter.internal_count += count_increase;
    } while (!ptr->count.compare_exchange_strong(old_counter, new_counter,
                                                 std::memory_order_acquire,
                                                 std::memory_order_relaxed));
    if (!new_counter.internal_count && !new_counter.external_counters) {
      delete ptr;
    }
  }
};
