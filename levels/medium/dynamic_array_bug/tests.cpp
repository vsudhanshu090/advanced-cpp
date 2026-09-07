#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../../../framework/test_common.h"
#include "solution.h"

// NOTE on test design: proving a copy is genuinely independent doesn't
// require destroying both objects afterward. We deliberately allocate the
// objects used in the aliasing checks on the heap via `new` and never
// `delete` them - a tiny, intentional, one-time leak scoped to this short
// test binary - specifically so that an UNFIXED (shallow-copy) submission
// fails cleanly via a wrong-value assertion instead of crashing the whole
// process with a double-free when two aliased objects would otherwise both
// reach their destructor. Tests that only ever involve ONE independent
// object (no copying) use normal stack objects as usual.

TEST_CASE("construction and at() work correctly for a single object") {
    DynamicArray arr(5);
    arr.at(0) = 10;
    arr.at(4) = 50;
    CHECK(arr.at(0) == 10);
    CHECK(arr.at(4) == 50);
    CHECK(arr.size() == 5);
}

TEST_CASE("newly constructed elements are zero-initialized") {
    DynamicArray arr(3);
    CHECK(arr.at(0) == 0);
    CHECK(arr.at(1) == 0);
    CHECK(arr.at(2) == 0);
}

TEST_CASE("at() throws out_of_range for an index >= size()") {
    DynamicArray arr(3);
    CHECK_THROWS_AS(arr.at(3), std::out_of_range);
    CHECK_THROWS_AS(arr.at(100), std::out_of_range);
}

TEST_CASE("const at() also bounds-checks correctly") {
    const DynamicArray arr(3);
    CHECK_THROWS_AS(arr.at(3), std::out_of_range);
    CHECK_NOTHROW(arr.at(2));
}

TEST_CASE("a single object constructs and destructs cleanly with no copying involved") {
    // Exercises the normal (non-buggy-path) constructor/destructor pairing
    // safely - no aliasing possible since there's only ever one object.
    {
        DynamicArray arr(10);
        arr.at(0) = 42;
        CHECK(arr.at(0) == 42);
    }  // destructor runs here - must not crash
    CHECK(true);  // reaching this line means the above didn't crash
}

TEST_CASE("copy constructor produces an INDEPENDENT copy - modifying the copy must not affect the original") {
    auto* original = new DynamicArray(3);
    original->at(0) = 100;
    original->at(1) = 200;

    auto* copy = new DynamicArray(*original);
    copy->at(0) = 999;  // modify the copy only

    CHECK(copy->at(0) == 999);
    CHECK(original->at(0) == 100);   // must be UNCHANGED if copy is truly independent
    CHECK(original->at(1) == 200);

    // Deliberately not deleted - see note at top of file.
}

TEST_CASE("copy constructor produces an INDEPENDENT copy - modifying the original must not affect the copy") {
    auto* original = new DynamicArray(3);
    original->at(0) = 5;

    auto* copy = new DynamicArray(*original);
    original->at(0) = 12345;  // modify the original only

    CHECK(original->at(0) == 12345);
    CHECK(copy->at(0) == 5);   // must be UNCHANGED if copy is truly independent
}

TEST_CASE("operator= produces an INDEPENDENT copy - modifying the target must not affect the source") {
    auto* a = new DynamicArray(4);
    auto* b = new DynamicArray(4);
    a->at(0) = 7;
    b->at(0) = 999;

    *b = *a;  // b now should be an independent deep copy of a's contents
    CHECK(b->at(0) == 7);

    b->at(0) = 111;  // modify b only, after the assignment
    CHECK(a->at(0) == 7);   // a must be unaffected
    CHECK(b->at(0) == 111);
}

TEST_CASE("operator= produces an INDEPENDENT copy - modifying the source afterward must not affect the target") {
    auto* a = new DynamicArray(4);
    auto* b = new DynamicArray(4);
    a->at(0) = 55;

    *b = *a;
    a->at(0) = 777;  // modify a only, after the assignment

    CHECK(b->at(0) == 55);  // b must be unaffected by a's later change
}

TEST_CASE("copies preserve size and all element values correctly, not just index 0") {
    auto* original = new DynamicArray(5);
    for (size_t i = 0; i < 5; ++i) original->at(i) = static_cast<int>(i * 11);

    auto* copy = new DynamicArray(*original);
    CHECK(copy->size() == 5);
    for (size_t i = 0; i < 5; ++i) {
        CHECK(copy->at(i) == static_cast<int>(i * 11));
    }
}
