#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../../../framework/test_common.h"
#include "solution.h"
#include <utility>

TEST_CASE("basic get/set work for a single object") {
    ResourceHandle h(42);
    CHECK(h.get() == 42);
    h.set(100);
    CHECK(h.get() == 100);
}

TEST_CASE("copy constructor produces an independent copy") {
    // Heap-allocated and intentionally not deleted, so that testing
    // copy-independence never risks a double-free crash regardless of
    // whether the submission has fixed the copy semantics yet.
    auto* original = new ResourceHandle(5);
    auto* copy = new ResourceHandle(*original);

    copy->set(999);
    CHECK(copy->get() == 999);
    CHECK(original->get() == 5);  // must be unaffected
}

TEST_CASE("copy assignment produces an independent copy") {
    auto* a = new ResourceHandle(1);
    auto* b = new ResourceHandle(2);

    *b = *a;
    CHECK(b->get() == 1);

    a->set(777);
    CHECK(b->get() == 1);  // b must be unaffected by a's later change
}

TEST_CASE("self-assignment does not corrupt the object") {
    ResourceHandle h(55);
    h = h;
    CHECK(h.get() == 55);
}

TEST_CASE("move constructor transfers the value") {
    auto* original = new ResourceHandle(321);
    auto* moved_to = new ResourceHandle(std::move(*original));
    CHECK(moved_to->get() == 321);
}

TEST_CASE("move assignment transfers the value") {
    auto* a = new ResourceHandle(11);
    auto* b = new ResourceHandle(22);
    *b = std::move(*a);
    CHECK(b->get() == 11);
}

TEST_CASE("a moved-from object is left in a safely destructible state") {
    // This is the test that genuinely exercises full construction AND
    // destruction of both the moved-from and moved-to object on the
    // stack. On a correctly-refactored solution this is completely safe.
    // On the still-broken starting code (where "move" silently falls
    // back to a shallow copy), this is the one test that can actually
    // crash the process - which is expected and informative for this
    // problem, not a judge malfunction (see the judge's crash reporting).
    ResourceHandle original(7);
    ResourceHandle moved_to(std::move(original));
    CHECK(moved_to.get() == 7);
    // Both destruct here at scope exit - must not double-free.
}
