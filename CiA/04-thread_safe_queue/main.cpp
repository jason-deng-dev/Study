#include "thread_safe_queue.h"
#include <iostream>

int main() {
  threadsafe_queue<int> q {};
  std::cout << "queue is empty:" << q.empty() <<'\n';
  q.push(25);
  std::cout << "queue is empty:" << q.empty() <<'\n';
  int removeVal;
  q.try_pop(removeVal);
  std::cout << "Removed val:" << removeVal << '\n';
  std::cout << "queue is empty:" << q.empty() <<'\n';
  
}
