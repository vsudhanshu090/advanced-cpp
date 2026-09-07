#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../../../framework/test_common.h"
#include "solution.h"

#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <algorithm>
#include <mutex>

using namespace std::chrono_literals;

TEST_CASE("single-threaded: push then pop preserves FIFO order") {
    BlockingQueue<int> q(10);
    q.push(1);
    q.push(2);
    q.push(3);
    CHECK(q.pop() == 1);
    CHECK(q.pop() == 2);
    CHECK(q.pop() == 3);
}

TEST_CASE("size() and empty() track state correctly, single-threaded") {
    BlockingQueue<int> q(10);
    CHECK(q.empty());
    CHECK(q.size() == 0);

    q.push(42);
    CHECK_FALSE(q.empty());
    CHECK(q.size() == 1);

    q.push(43);
    CHECK(q.size() == 2);

    q.pop();
    CHECK(q.size() == 1);

    q.pop();
    CHECK(q.empty());
    CHECK(q.size() == 0);
}

TEST_CASE("works with a capacity-1 queue in strict alternation") {
    BlockingQueue<int> q(1);
    for (int i = 0; i < 20; ++i) {
        q.push(i);
        CHECK(q.pop() == i);
    }
}

TEST_CASE("push blocks when the queue is full, unblocks after a pop frees a slot") {
    BlockingQueue<int> q(2);
    q.push(1);
    q.push(2);  // queue now full (capacity 2)

    std::atomic<bool> push_completed{false};
    std::thread producer([&] {
        q.push(3);  // must block here until a slot frees up
        push_completed.store(true);
    });

    std::this_thread::sleep_for(150ms);
    // If push() didn't actually block, this would already be true.
    CHECK_FALSE(push_completed.load());

    CHECK(q.pop() == 1);  // frees a slot -> should unblock the producer

    // Poll for completion instead of a plain join, so a genuinely broken
    // (permanently blocked) implementation fails the assertion instead of
    // hanging the whole test binary forever.
    bool completed_in_time = false;
    for (int i = 0; i < 20; ++i) {
        if (push_completed.load()) { completed_in_time = true; break; }
        std::this_thread::sleep_for(100ms);
    }
    CHECK(completed_in_time);

    producer.join();
    CHECK(q.pop() == 2);
    CHECK(q.pop() == 3);
}

TEST_CASE("pop blocks when the queue is empty, unblocks after a push arrives") {
    BlockingQueue<int> q(5);
    std::atomic<bool> pop_completed{false};
    int popped_value = -1;

    std::thread consumer([&] {
        popped_value = q.pop();  // must block here until something is pushed
        pop_completed.store(true);
    });

    std::this_thread::sleep_for(150ms);
    CHECK_FALSE(pop_completed.load());

    q.push(99);  // should unblock the consumer

    bool completed_in_time = false;
    for (int i = 0; i < 20; ++i) {
        if (pop_completed.load()) { completed_in_time = true; break; }
        std::this_thread::sleep_for(100ms);
    }
    CHECK(completed_in_time);

    consumer.join();
    CHECK(popped_value == 99);
}

TEST_CASE("stress: multiple producers and consumers exchange every item exactly once, in a bounded queue") {
    constexpr int kProducers = 4;
    constexpr int kItemsPerProducer = 500;
    constexpr int kConsumers = 4;
    constexpr int kTotalItems = kProducers * kItemsPerProducer;
    constexpr int kItemsPerConsumer = kTotalItems / kConsumers;

    BlockingQueue<int> q(50);  // small capacity forces real blocking under load

    std::vector<std::thread> producers;
    for (int p = 0; p < kProducers; ++p) {
        producers.emplace_back([&q, p] {
            for (int i = 0; i < kItemsPerProducer; ++i) {
                q.push(p * 10000 + i);  // unique per producer
            }
        });
    }

    std::mutex results_mutex;
    std::vector<int> results;
    results.reserve(kTotalItems);

    std::vector<std::thread> consumers;
    for (int c = 0; c < kConsumers; ++c) {
        consumers.emplace_back([&] {
            std::vector<int> local;
            local.reserve(kItemsPerConsumer);
            for (int i = 0; i < kItemsPerConsumer; ++i) {
                local.push_back(q.pop());
            }
            std::lock_guard<std::mutex> lock(results_mutex);
            results.insert(results.end(), local.begin(), local.end());
        });
    }

    for (auto& t : producers) t.join();
    for (auto& t : consumers) t.join();

    REQUIRE(results.size() == static_cast<size_t>(kTotalItems));

    std::vector<int> expected;
    expected.reserve(kTotalItems);
    for (int p = 0; p < kProducers; ++p)
        for (int i = 0; i < kItemsPerProducer; ++i)
            expected.push_back(p * 10000 + i);

    std::sort(results.begin(), results.end());
    std::sort(expected.begin(), expected.end());
    CHECK(results == expected);  // every item delivered exactly once, none lost/duplicated

    CHECK(q.empty());
    CHECK(q.size() == 0);
}
