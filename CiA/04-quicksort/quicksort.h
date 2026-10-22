#include <algorithm>
#include <future>
#include <list>

template <typename T>
std::list<T> sequential_quick_sort(std::list<T> input) {
  if (input.empty()) return input;

  std::list<T> result;
  // take first element as pivot by slicing it offf the front of the list using splice()
  // we splice directly into result
  result.splice(result.begin(), input, input.begin());

  // to use for fcomparison, take reference to it to avoid copying
  T const &pivot = *result.begin();

  /*
  std::partiiton reorders element in range into 2 groups based on condition, doing so in-place and very efficiently
  it rearranges the elements in input so that
  Group 1 (Front) all elemnts where lambda returns true are moved to beginnning of range
  Group 2 (Back) all elements where lambda returns false are moved to end of range
  divide point is iterator to first element of Group 2

  */

  // use parition to divide the sequence into those values less than pivot and those no less than pivot
  auto divide_point = std::partition(input.begin(), input.end(), [&](T const &t) { return t < pivot; });

  // to use recursion to sort the two "halves", we create 2 lists using splice()
  // to move vlaues form input up to divide point into lower_part
  // this leaves remaining value alone in input
  std::list<T> lower_part;
  lower_part.splice(lower_part.end(), input, input.begin(), divide_point);

  // we sort the 2 lists using recursive calls
  // by using std::move() to pass lsit in, can avoid copying
  auto new_lower(sequential_quick_sort(std::move(lower_part)));
  auto new_higher(sequential_quick_sort(std::move(input)));

  // use splice to piece result together in the right order
  result.splice(result.end(), new_higher);
  result.splice(result.begin(), new_lower);
  return result;
}

template <typename T>
std::list<T> parallel_quick_sort(std::list<T> input) {
  if (input.empty()) {
    return input;
  }
  std::list<T> result;
  result.splice(result.begin(), input, input.begin());
  T const &pivot = *result.begin();
  auto divide_point = std::partition(input.begin(), input.end(), [&](T const &t) { return t < pivot; });

  std::list<T> lower_part;
  lower_part.splice(lower_part.end(), input, input.begin(), divide_point);

  // rather than sorting lower portion on current thread, sort it on another thread using std::async
  std::future <std::list<T>> new_lower (std::async(&parallel_quick_sort<T>, std::move(lower_part)));
  // upper portion of the list is sorted with direct recursion as before
  auto new_higher(parallel_quick_sort(std::move(input)));
  result.splice(result.end(),new_higher); 
  result.splice(result.begin(),new_lower.get()); 
  return result;
}
