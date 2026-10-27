#include "thread_safe_stack.h"
#include <iostream>

int main() {
  threadsafe_stack<int> stk {};
  std::cout << "stack is empty:" << stk.empty() <<'\n';
  stk.push(25);
  std::cout << "stack is empty:" << stk.empty() <<'\n';
  int removeVal;
  stk.pop(removeVal);
  std::cout << "Removed val:" << removeVal << '\n';
  std::cout << "stack is empty:" << stk.empty() <<'\n';
  
}
