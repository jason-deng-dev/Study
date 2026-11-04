#include <atomic>
#include <memory>
#include <stdexcept>
#include <thread>

template <typename T> class lock_free_stack {
private:
  struct node {
    std::shared_ptr<T> data; // data held by pointer
    node *next;
    node(T const &data_)
        : data(std::make_shared<T>(data_)) {
    } // create shared_ptr for newly allocated T
  };

  std::atomic<node *> head;

public:
  void push(T const &data) {
    node *const new_node = new node(data);
    new_node->next = head.load();
    while (!head.compare_exchange_weak(new_node->next, new_node))
      ;
  }

private:
  std::atomic<unsigned> threads_in_pop;
  std::atomic<node *> to_be_deleted;

  static void delete_nodes(node *nodes) {
    while (nodes) {
      node *next = nodes->next;
      delete nodes;
      nodes = next;
    }
  }

  void try_reclaim(node *old_head) {
    if (threads_in_pop == 1) { // 1
      node *nodes_to_delete = to_be_deleted.exchange(
          nullptr);                    // 2 Claim list of to-be-deleted nodes
      if (!--threads_in_pop) {         // 3 Are you the only thread in pop()?
        delete_nodes(nodes_to_delete); // 4
      } else if (nodes_to_delete) {    // 5
        chain_pending_nodes(nodes_to_delete); // 6
      }
      delete old_head; // 7

    } else {
      chain_pending_nodes(old_head); // 8
      --threads_in_pop;
    }
  }

  void chain_pending_nodes(node *nodes) {
    node *last = nodes;
    while (node *const next =
               last->next) { // 9 Follow the next pointer chain to the end
      last = next;
    }
    chain_pending_nodes(nodes, last);
  }

  void chain_pending_nodes(node *first, node *last) {
    last->next = to_be_deleted; // 10
    while (!to_be_deleted.compare_exchange_weak(
        last->next, first)) // 11 Loop to guarantee that last->next is correct
      ;
  }
  void chain_pending_node(node *n) { chain_pending_node(n, n); } // 12

public:
  std::shared_ptr<T> &get_hazard_pointer_for_current_thread();
  bool outstanding_hazard_pointers_for();
  void delete_nodes_with_no_hazards();

  std::shared_ptr<T> pop(T &result) {
    std::atomic<void *> &hp = get_hazard_pointer_for_current_thread();
    node *old_head = head.load();
    do {
      node *temp;
      do { // 1 loop until set the hazard pointer to head
        temp = old_head;
        hp.store(old_head);
        old_head = head.load();
      } while (old_head != temp);
    } while (old_head &&
             !head.compare_exchange_strong(old_head, old_head->next));
    hp.store(nullptr); // 2 clear hazard pointer once you're finished
    std::shared_ptr<T> res;
    if (old_head) {
      res.swal(old_head->data);
      if (outstanding_hazard_pointers_for(
              old_head)) {       // 3 check for hazard pointers referencing node
                                 // before deleting
        reclaim_later(old_head); // 4
      } else {
        delete old_head; // 5
      }
      delete_nodes_with_no_hazards();
    }
    return res;
  }
};
