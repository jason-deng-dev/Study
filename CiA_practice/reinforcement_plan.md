# Concurrency in Action — Reinforcement Plan (Chapters 1–6)

Goal: **active recall + hands-on grading**. Not re-reading — answering, building, and getting feedback.

## Sources used
- Book: *C++ Concurrency in Action* (Bilbo), ch 1–6
- Your Anki bank: `cia_cards.tsv` (86 cards, ch 1–5) — used as the concept-question pool
- Your Obsidian notes: `02 - Learning/QD/Concurrency/Concurrency in Action/{1..6}.md`

## Progress

> Last updated: 2025-07-16 — Phase 2 recall graded. Below.

### Completed

- **Phase 0 — Baseline (graded):**
  - Q1 Concurrency vs parallelism: ✅ solid
  - Q2 Destroy without join: ✅ solid (`std::terminate`)
  - Q3 seq_cst vs relaxed: ⚠️ right instincts; needs "single global total order across all seq_cst atomics" phrasing
  - Q4 Check-then-act: ✅ solid (understood interface-level race)
- **Phase 1 — Ch 1 recall (graded, all solid):**
  - Q1 Concurrency vs parallelism: ✅ solid
  - Q2 Task switching vs hardware concurrency: ✅ solid (caught cold-cache/TLB refill — the part people forget)
  - Q3 Task vs data parallelism: ✅ solid (nudged on task-parallelism definition; "embarrassingly parallel" scales because pieces are independent)
  - Q4 Reasons to/not to use concurrency: ✅ solid (complexity vs overhead; nudged on 2nd reason = performance, and oversubscription cost)
- **Phase 1 — Build (`parallel_accumulate`):** ✅ PASSED — clean compile with `-std=c++23 -Wall -Wextra -Wconversion -Wsign-conversion -Wshadow -Werror`; correct across n=1..1000 (matches `std::accumulate`); fixed: data race (per-thread partial sums), `ceil` integer-division bug (`double` cast), `hardware_concurrency()==0` guard, consistent `size_t` (no conversion warnings).

- **Phase 2 — Ch 2 recall (graded, concept-solid / precision-gaps):**
  - Q1 join / jthread / exception-safe: ⚠️ correct fixes (RAII-joiner + jthread), but missed the standard's *terminate-on-exception-during-unwind* guarantee
  - Q2 arg passing + two traps: ⚠️ caught dangling-trap + `std::ref`; missed the copy-vs-move distinction (trap #2)
  - Q3 movable-only / most-vexing-parse: ✅ move-only correct; answered copy-trap instead of the parsing trap (`Foo()` → function declaration, fix = brace-init / lambda)
  - Q4 `std::thread::id`: didn't know at all

### Next

- Phase 2 **build challenge** still open (vector + move-only accumulator + vexing-parse demo/fix); else proceed to **Phase 3** (Ch 3: sharing data).

## How we work (the loop)
1. **Phase chapter**: I hand you the active-recall questions for that chapter. You answer from memory (chat or scratch file). No looking up until you've tried.
2. **Grade round 1**: I score each answer on the rubric and flag the gaps.
3. **Implementation challenge**: I give you one build task for that chapter. You write the code (single file in `/tmp` or a scratch dir).
4. **Grade round 2**: I review the code — correctness, thread-safety, idiom, edge cases — and return a score + specific fixes.
5. **Recap**: I tell you which concepts are solid and which to redo, then move on.

## Grading rubric (1–5 each)
- **Concept answers**: (C1) Correctness, (C2) Precision/terminology, (C3) Did you recall it yourself vs. look it up.
- **Code**: (K1) Compiles & runs, (K2) Correct results under concurrency (race/deadlock-free), (K3) Idiomatic C++ for the book (`unique_lock`, `scoped_lock`, `std::async`, `memory_order`, etc.), (K4) Edge cases + exception safety.

Scale: **1** = wrong/broken · **3** = works for the main case · **5** = correct, idiomatic, handles edge cases.

## Suggested pace
One chapter per "session". Each session ≈ 45–90 min (5 recall Qs + one build). Full run = 7 phases (6 chapters + capstone).

---

## Phase 0 — Baseline (10 min, no grading)
Answer 4 questions from memory. Just tell me where you stand so I calibrate difficulty:
1. Concurrency vs. parallelism — what is each *for*?
2. What actually happens when a `std::thread` is destroyed without `join()`/`detach()`?
3. What does `memory_order_seq_cst` guarantee that `memory_order_relaxed` does not?
4. Why is a mutex-protected stack still racy at the interface level (check-then-act)?

---

## Phase 1 — Ch 1: Hello, world of concurrency in C++
**Recall questions** (answer from memory first):
1. Concurrency vs. parallelism — what is each actually for? (decides whether threads are the right tool)
2. Task switching vs. hardware concurrency — same program can behave subtly differently why? What does a context switch actually cost?
3. Task parallelism vs. data parallelism — definitions + why "embarrassingly parallel" scales well.
4. The two legitimate reasons to use concurrency; and the concrete reasons *not* to.

**Build challenge:** Write a `parallel_accumulate` over a `std::vector<int>` that splits the range into blocks across threads. Requirements: cap thread count at `hardware_concurrency()` AND at a min-workload threshold, put the last block on the calling thread, and print `hardware_concurrency()`.
**Grade on:** thread count logic, last-block-on-caller, no data race on the partial sums, join exception-safety.

## Phase 2 — Ch 2: Managing threads
**Recall questions:**
1. What happens on destroy-without-join; two ways to make `join()` exception-safe; what `std::jthread` adds.
2. How are thread arguments passed (the copy+rvalue rule), and the two traps (dangling pointer/buffer, missing `std::ref`/`std::move`).
3. Why is `std::thread` movable but not copyable; what does that enforce? The "most vexing parse" with threads and its fix.
4. `std::thread::id` — what can you do with it; what's implementation-defined?

**Build challenge:** Pass a `std::vector<int>` and a move-only accumulator into a thread correctly (use `std::ref`/`std::move` as needed). Then demonstrate the most-vexing-parse trap with a function-object thread and fix it with brace init + lambda.
**Grade on:** correct arg handling, no copies where you meant moves, correct fix for the parse trap.

## Phase 3 — Ch 3: Sharing data between threads
**Recall questions:**
1. What makes a race condition a *bug* (invariant broken)? The three ways to avoid races.
2. The cardinal rule for mutex-protected data (don't leak refs/pointers out of the lock).
3. Why check-then-act is still racy even with locked member functions; the four fixes for `pop()`.
4. `std::lock` + `std::scoped_lock`; the four deadlock-avoidance guidelines; how `hierarchical_mutex` enforces order.
5. `unique_lock` vs `lock_guard`; `call_once`/`once_flag`; `shared_mutex`/`shared_lock`.

**Build challenge:** Write a small `list`-like structure protected by two mutexes, accessing both at once — do it deadlock-free with `std::scoped_lock`. Then write a `pop()` that never loses an element if its copy throws.
**Grade on:** deadlock-free multi-lock, exception-safe pop, no escaped references.

## Phase 4 — Ch 4: Synchronizing concurrent operations
**Recall questions:**
1. Why flag+sleep is bad; what replaces it; why `wait` takes a predicate.
2. Why `notify` after releasing the mutex; `condition_variable` vs `any`; why the queue's mutex must be `mutable`.
3. `wait_and_pop` vs `try_pop`; `future` vs `shared_future`; `std::async` launch policies; where exceptions from async tasks go; `packaged_task` vs `promise`.
4. Timeout families (`wait_for`/`wait_until` + `wait_for_pred`/`wait_until_pred`); what `wait_for` returns; clocks/durations/time points.

**Build challenge:** A bounded thread-safe queue with a `condition_variable`: `wait_and_pop()` blocks until an item or shutdown, `try_pop()` returns an enum, and a producer/consumer driver that tests shutdown + multiple waiters. Then use `std::async` to return a value from a task that throws, and catch it via the future.
**Grade on:** no lost/wakeups, no spurious wakeup bug, clean shutdown, exception propagates through the future.

## Phase 5 — Ch 5: The C++ memory model & atomics
**Recall questions:**
1. Two aspects of the memory model; what atomics are for. Object vs. memory location — why the definition decides where data races live.
2. Modification order guarantee (and what it doesn't give). What an atomic op guarantees + its cost.
3. `std::atomic_flag` (the one guaranteed lock-free type); `atomic<bool>`, `atomic<T*>`, integral types; `compare_exchange_weak` vs `_strong` and why two orderings.
4. The six memory orderings + three models. Seq_cst guarantee/price; relaxed; acquire-release (and why "pairwise"); transitive synchronization. `consume`/data-dependency. Release sequences; fences. Ordering non-atomic ops with atomics.

**Build challenge:** Implement thread 1 → thread 2 message passing **twice** — once with `seq_cst` atomics, once with acquire-release on a flag-and-payload — and explain (in comments) why both are correct but acquire-release is cheaper. Optionally add a third thread to show transitive synchronization via release sequences.
**Grade on:** correct memory-order choice, correct flag/payload ordering, a correct explanation of the synchronizes-with/happens-before, no real data race.

## Phase 6 — Ch 6: Designing lock-based concurrent data structures
*(not yet in your Anki set — this is the newest coverage)*
**Recall questions:**
1. Guidelines for designing data structures for concurrency.
2. Thread-safe stack using locks; thread-safe queue using locks + CVs; the fine-grained-lock + CV queue and how separating data enables concurrency.
3. Thread-safe lookup table (fine-grained map); thread-safe list using locks.

**Build challenge:** A thread-safe `map`-style lookup table with fine-grained locking (lock per bucket) and a thread-safe queue using fine-grained locks + condition variables. Write a multithreaded stress driver (N producers/consumers) and verify with ThreadSanitizer (`-fsanitize=thread`).
**Grade on:** correctness under load, no per-bucket deadlock, no data race (clean TSan), reasonable lock granularity.

## Phase 7 — Capstone (cumulative, graded hardest)
Build a small **producer/consumer work system**:
- A bounded thread-safe queue (Ch 4/6).
- A pool of worker threads pulling tasks (Ch 2).
- Tasks submitted via `std::async`/`packaged_task`, returning values, propagating exceptions (Ch 4/5).
- Clean shutdown that drains and joins without lost work.

Deliver one file + a short write-up: which memory orderings/guards you chose and why. I give a full rubric score across all chapters and a "what to revisit" list.

---

