#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../../../framework/test_common.h"
#include "solution.h"
#include <string>

TEST_CASE("basic put then get returns the value") {
    LRUCache<int, int> cache(2);
    cache.put(1, 100);
    int out = 0;
    CHECK(cache.get(1, &out));
    CHECK(out == 100);
}

TEST_CASE("get on a missing key returns false and doesn't touch *out") {
    LRUCache<int, int> cache(2);
    int out = -999;
    CHECK_FALSE(cache.get(42, &out));
    CHECK(out == -999);  // must be left untouched
}

TEST_CASE("size() reflects number of distinct keys") {
    LRUCache<int, int> cache(5);
    CHECK(cache.size() == 0);
    cache.put(1, 10);
    cache.put(2, 20);
    CHECK(cache.size() == 2);
}

TEST_CASE("put on an existing key updates the value without growing size") {
    LRUCache<int, int> cache(3);
    cache.put(1, 10);
    cache.put(1, 999);
    CHECK(cache.size() == 1);
    int out = 0;
    CHECK(cache.get(1, &out));
    CHECK(out == 999);
}

TEST_CASE("exceeding capacity evicts the least recently used entry") {
    LRUCache<int, int> cache(2);
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);  // capacity 2 -> evicts key 1 (least recently used)

    int out = 0;
    CHECK_FALSE(cache.get(1, &out));  // evicted
    CHECK(cache.get(2, &out));
    CHECK(out == 20);
    CHECK(cache.get(3, &out));
    CHECK(out == 30);
    CHECK(cache.size() == 2);
}

TEST_CASE("get() counts as recently-used and protects a key from eviction") {
    LRUCache<int, int> cache(2);
    cache.put(1, 10);
    cache.put(2, 20);

    int out = 0;
    CHECK(cache.get(1, &out));  // touch key 1 -> now key 2 is least recently used

    cache.put(3, 30);  // should evict key 2, NOT key 1

    CHECK(cache.get(1, &out));
    CHECK(out == 10);
    CHECK_FALSE(cache.get(2, &out));  // evicted
    CHECK(cache.get(3, &out));
}

TEST_CASE("put() on an existing key also refreshes recency") {
    LRUCache<int, int> cache(2);
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(1, 15);   // touches key 1 via put -> key 2 now least recently used
    cache.put(3, 30);   // should evict key 2

    int out = 0;
    CHECK(cache.get(1, &out));
    CHECK(out == 15);
    CHECK_FALSE(cache.get(2, &out));
    CHECK(cache.get(3, &out));
}

TEST_CASE("classic trace: matches expected LRU behavior step by step") {
    LRUCache<int, int> cache(2);
    cache.put(1, 1);
    cache.put(2, 2);
    int out = 0;

    CHECK(cache.get(1, &out)); CHECK(out == 1);  // touches 1; order: 2,1 (1 most recent)
    cache.put(3, 3);                              // evicts 2 (LRU)
    CHECK_FALSE(cache.get(2, &out));

    cache.put(4, 4);                              // evicts 1 (LRU, since 3 was touched more recently)
    CHECK_FALSE(cache.get(1, &out));
    CHECK(cache.get(3, &out)); CHECK(out == 3);
    CHECK(cache.get(4, &out)); CHECK(out == 4);
}

TEST_CASE("size never exceeds capacity across many sequential unique inserts") {
    const size_t kCapacity = 10;
    LRUCache<int, int> cache(kCapacity);
    for (int i = 0; i < 500; ++i) {
        cache.put(i, i * 2);
        CHECK(cache.size() <= kCapacity);
    }
    CHECK(cache.size() == kCapacity);

    // only the most recent kCapacity keys should still be present
    for (int i = 500 - static_cast<int>(kCapacity); i < 500; ++i) {
        int out = 0;
        CHECK(cache.get(i, &out));
        CHECK(out == i * 2);
    }
    // anything older should be gone
    int out = 0;
    CHECK_FALSE(cache.get(0, &out));
}

TEST_CASE("works with non-integer key/value types") {
    LRUCache<std::string, std::string> cache(2);
    cache.put("a", "apple");
    cache.put("b", "banana");
    std::string out;
    CHECK(cache.get("a", &out));
    CHECK(out == "apple");

    cache.put("c", "cherry");  // evicts "b" (a was touched more recently)
    CHECK_FALSE(cache.get("b", &out));
    CHECK(cache.get("c", &out));
    CHECK(out == "cherry");
}

TEST_CASE("repeated overwrite of the same key never grows size past 1") {
    LRUCache<int, int> cache(4);
    for (int i = 0; i < 100; ++i) {
        cache.put(1, i);
    }
    CHECK(cache.size() == 1);
    int out = 0;
    CHECK(cache.get(1, &out));
    CHECK(out == 99);
}
