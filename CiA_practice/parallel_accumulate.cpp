#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <future>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>

// parallel_accumulate over std::vector<int> which splits the range into blocks
// across threads
// 1. cap thread count at hardware_concurrency()
// 2. at min-workload threshold, put last block on calling thread and print
// hardware_concurrency()

int parallel_accumulate(const std::vector<int> &vec) {
  // min workload threshhold
  int min_per_thread = 25;
  int n = static_cast<int>(vec.size());
  size_t max_threads = static_cast<size_t>(
      std::ceil(static_cast<double>(n) / static_cast<double>(min_per_thread)));

  // cap thread count to hardware_concurrency
  size_t hardwareThreads = std::thread::hardware_concurrency() == 0
                               ? 1
                               : std::thread::hardware_concurrency();

  int threadCount = static_cast<int>(std::min({max_threads, hardwareThreads}));

  int const blockSize = n / threadCount;

  std::vector<std::future<int>> results;

  results.reserve(static_cast<size_t>(threadCount - 1));

  // spawn N-1 threads for blocks 0 ... N-2
  for (int i = 0; i < threadCount; ++i) {
    results.emplace_back(std::async(std::launch::async, [&, i] {
      int partialSum = 0;
      size_t start = size_t(i * blockSize);
      size_t end = size_t((i == threadCount - 1) ? n : (i + 1) * blockSize);
      for (size_t j = start; j < end; ++j) {
        partialSum += vec[j];
      }
      return partialSum;
    }));
  }

  int total = 0;
  for (auto &f : results) {
    total += f.get();
  }
  std::cout << "hardware_concurrency = " << hardwareThreads << ", using "
            << threadCount << " threads"
            << " (block size " << blockSize << ")\n";
  return total;
}

int main() {
  std::vector<int> vec{1, 2, 3, 4, 5};
  std::cout << parallel_accumulate(vec);
  return 0;
}

