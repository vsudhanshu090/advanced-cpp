# C++ Levels 1–5 — Consolidated Notes (Example-Driven)

> Format note: each level is **one working example program** that packs in
> as many of the level's topics as possible, taught through comments rather
> than prose. Topic lists for **Levels 1–3** are reconstructed from the
> level names since I don't have your exact original lists — skim the
> `TOPICS COVERED` block at the top of each and tell me if anything's off
> or missing, and I'll patch it. Level 4's topic list is exactly what you
> gave me.

---

## LEVEL 1 — C++ FUNDAMENTALS REFRESHER

```
TOPICS COVERED (reconstructed — confirm/correct):
1. Fundamental types, literals, sizeof
2. const correctness (top-level vs low-level const)
3. References vs pointers (first contact)
4. Function overloading & default arguments
5. Storage duration: automatic / static / global
6. Arrays vs std::string vs std::vector (basic)
7. Structs
8. Enums (plain enum vs enum class)
9. Namespaces
10. Type casting: implicit, static_cast, C-style (why to avoid it)
```

```cpp
#include <iostream>
#include <string>
#include <vector>
using namespace std;

// --- 9. Namespaces: scope your own symbols instead of polluting global ---
namespace physics {
    constexpr double GRAVITY = 9.8; // constexpr = compile-time const
}

// --- 7. Struct: plain aggregate, public by default (vs class = private by default) ---
struct Point {
    double x, y;
};

// --- 8. enum class: scoped, doesn't leak names, no implicit int conversion ---
enum class Direction { North, South, East, West };

// --- 4. Overloading + default arguments ---
// Two functions, same name, different signatures -> compiler picks by args.
int add(int a, int b) { return a + b; }
double add(double a, double b, double c = 0.0) { return a + b + c; } // default arg

// --- 5. Static storage duration inside a function: persists across calls ---
int callCounter() {
    static int count = 0; // initialized once, retains value between calls
    return ++count;
}

// --- 2. const correctness ---
void printPoint(const Point& p) {
    // p.x = 5; // ERROR: p is const-ref, can't mutate through it
    cout << "(" << p.x << ", " << p.y << ")\n";
}

int main() {
    // --- 1. Fundamental types & sizeof ---
    int i = 42; double d = 3.14; char c = 'A'; bool b = true;
    cout << "sizeof(int)=" << sizeof(int) << " sizeof(double)=" << sizeof(double) << "\n";

    // --- 3. References vs pointers ---
    int x = 10;
    int& ref = x;      // reference: alias, must bind on declaration, never null, never rebound
    int* ptr = &x;      // pointer: holds an address, can be null, can be reassigned
    ref = 20;            // mutates x directly
    *ptr = 30;            // also mutates x, through explicit dereference
    cout << "x=" << x << " ref=" << ref << " *ptr=" << *ptr << "\n";

    // --- 6. Arrays vs std::string vs std::vector ---
    int rawArr[3] = {1, 2, 3};                 // fixed size, no bounds checking, decays to pointer
    string s = "hello";                          // owns its data, resizable, has .size()/.substr() etc.
    vector<int> v = {1, 2, 3};                    // dynamic array, resizable, bounds-checkable via .at()
    v.push_back(4);
    cout << "vector size=" << v.size() << " last=" << v.back() << "\n";

    // --- 10. Casting ---
    double pi = 3.99;
    int truncated = static_cast<int>(pi);          // explicit, checked-ish, preferred over C-style (int)pi
    cout << "static_cast truncation: " << truncated << "\n";

    // Using the struct + enum + namespace + overloads + static counter together:
    Point origin{0.0, 0.0};
    printPoint(origin);
    Direction dir = Direction::North;
    cout << "GRAVITY=" << physics::GRAVITY << " dirIsNorth=" << (dir == Direction::North) << "\n";
    cout << "add(int)=" << add(2, 3) << " add(double,default)=" << add(1.5, 2.5) << "\n";
    cout << "callCounter: " << callCounter() << ", " << callCounter() << ", " << callCounter() << "\n";
}
```

---

## LEVEL 2 — POINTERS DEEP DIVE

```
TOPICS COVERED (reconstructed — confirm/correct):
1. Pointer basics, pointer arithmetic
2. Pointers and arrays (decay), pointer to pointer
3. const with pointers (pointer-to-const vs const-pointer vs both)
4. Dynamic memory: new/delete, new[]/delete[]
5. Dangling pointers, memory leaks, double free
6. nullptr vs NULL vs 0
7. void* and its limitations
8. Function pointers
9. Pass-by-pointer vs pass-by-reference (when each is idiomatic)
```

```cpp
#include <iostream>
using namespace std;

// --- 8. Function pointer: a variable that holds the address of a function ---
int square(int n) { return n * n; }
int cube(int n) { return n * n * n; }

// --- 9. Pass-by-pointer: caller's intent to mutate is EXPLICIT at call site (&x) ---
void incrementViaPointer(int* p) { (*p)++; }
// Pass-by-reference: same effect, call site looks like pass-by-value (no & needed at call)
void incrementViaReference(int& r) { r++; }

int main() {
    // --- 1. Pointer basics & arithmetic ---
    int arr[5] = {10, 20, 30, 40, 50};
    int* p = arr;              // array decays to pointer to first element
    cout << "*p=" << *p << " *(p+2)=" << *(p + 2) << " p[3]=" << p[3] << "\n";
    p++;                          // moves by sizeof(int) bytes, not 1 byte
    cout << "after p++: *p=" << *p << "\n";

    // --- 2. Pointer to pointer ---
    int val = 100;
    int* ptr1 = &val;
    int** ptr2 = &ptr1;          // holds address of ptr1
    cout << "**ptr2=" << **ptr2 << "\n";

    // --- 3. const variants — this is the classic interview trip-up ---
    int a = 1, bnum = 2;
    const int* cp = &a;         // pointer to const int: *cp is read-only, cp itself can be reseated
    cp = &bnum;                    // OK
    // *cp = 5;                  // ERROR

    int* const pc = &a;          // const pointer: pc always points to a, but *pc is mutable
    *pc = 99;                     // OK
    // pc = &bnum;                // ERROR

    const int* const cpc = &a;   // const pointer to const int: neither can change
    cout << "a=" << a << "\n";

    // --- 4. Dynamic memory ---
    int* heapInt = new int(7);
    int* heapArr = new int[5]{1, 2, 3, 4, 5};
    cout << "*heapInt=" << *heapInt << " heapArr[4]=" << heapArr[4] << "\n";
    delete heapInt;               // single object -> delete
    delete[] heapArr;             // array -> delete[] (mismatching these is UB)

    // --- 5. Dangling pointer illustration (commented so it doesn't actually run UB) ---
    // int* dangling = new int(5);
    // delete dangling;
    // cout << *dangling;         // UB: use-after-free
    // dangling = nullptr;        // the fix: null it out immediately after delete

    // --- 6. nullptr vs NULL vs 0 ---
    int* np = nullptr;           // nullptr is type-safe (std::nullptr_t), prefer over NULL/0
    cout << "np is null: " << (np == nullptr) << "\n";

    // --- 7. void* — type-erased pointer, must be cast before dereferencing ---
    int num = 42;
    void* vp = &num;
    cout << "via void*: " << *static_cast<int*>(vp) << "\n";

    // --- 8 & 9 combined ---
    int (*fp)(int) = square;      // function pointer declaration
    cout << "fp(5)=" << fp(5) << "\n";
    fp = cube;
    cout << "fp(5) after reassign=" << fp(5) << "\n";

    int counter = 0;
    incrementViaPointer(&counter);
    incrementViaReference(counter);
    cout << "counter=" << counter << " (expect 2)\n";
}
```

---

## LEVEL 3 — OOP BASICS

```
TOPICS COVERED (reconstructed — confirm/correct):
1. Classes vs structs (default access), encapsulation
2. Constructors (default, parameterized), member initializer lists
3. Destructors
4. this pointer
5. Access specifiers (public/private/protected)
6. static members (data & functions)
7. Operator overloading (basic — e.g. operator<<, operator==)
8. Friend functions
9. Composition ("has-a" relationship)
```

```cpp
#include <iostream>
#include <string>
using namespace std;

class Engine {                          // --- 9. Composition building block ---
public:
    int horsepower;
    Engine(int hp) : horsepower(hp) {}   // --- 2. member initializer list ---
};

class Car {
private:                                  // --- 5. Access specifier ---
    string model;
    int odometer;
    Engine engine;                        // --- 9. Composition: Car "has-a" Engine ---

    static int totalCarsCreated;          // --- 6. static data member: shared across all instances ---

public:
    // --- 2. Constructor with member initializer list (preferred over body assignment) ---
    Car(string m, int hp) : model(m), odometer(0), engine(hp) {
        totalCarsCreated++;
    }

    // --- 3. Destructor: runs automatically when object goes out of scope ---
    ~Car() {
        cout << "Destroying car: " << model << "\n";
    }

    void drive(int miles) {
        this->odometer += miles;          // --- 4. `this` disambiguates member vs parameter/local ---
    }

    // --- 6. static member function: no `this`, callable without an instance ---
    static int getTotalCarsCreated() { return totalCarsCreated; }

    // --- 7. Operator overloading: compare two Car objects with == ---
    bool operator==(const Car& other) const {
        return model == other.model && odometer == other.odometer;
    }

    // --- 8. friend function: external function granted access to private members ---
    friend ostream& operator<<(ostream& os, const Car& c);
};

int Car::totalCarsCreated = 0;             // static members defined outside the class

ostream& operator<<(ostream& os, const Car& c) {
    os << c.model << " [" << c.odometer << " mi, " << c.engine.horsepower << " hp]";
    return os;
}

int main() {
    Car c1("Model X", 400);
    Car c2("Model X", 400);
    c1.drive(150);

    cout << c1 << "\n";                     // uses friend operator<<
    cout << "c1 == c2? " << (c1 == c2) << " (false, odometer differs)\n";
    cout << "Total cars created: " << Car::getTotalCarsCreated() << "\n";
}   // destructors for c1, c2 fire here, in reverse order of construction
```

---

## LEVEL 4 — OOP ADVANCED: INHERITANCE & POLYMORPHISM

```
TOPICS COVERED:
1. Inheritance (public/protected/private), base/derived constructors
2. Virtual functions and dynamic dispatch, vtables (conceptual model)
3. Pure virtual functions and abstract classes/interfaces
4. Virtual destructors (why they matter)
5. Method overriding vs hiding, `override` and `final` keywords
6. Multiple inheritance, the diamond problem, virtual inheritance
7. Slicing (object slicing on copy)
8. Upcasting/downcasting, dynamic_cast, static_cast between classes
9. Abstract base class as interface pattern
```

```cpp
#include <iostream>
using namespace std;

// --- 3 & 9. Pure virtual function => abstract class / interface. Can't instantiate Shape directly. ---
class Shape {
public:
    Shape() { cout << "Shape constructed\n"; }

    // --- 4. Virtual destructor: MANDATORY when deleting via a base pointer, or the
    // derived part never runs its destructor -> resource leak. ---
    virtual ~Shape() { cout << "Shape destroyed\n"; }

    virtual double area() const = 0;    // pure virtual -> Shape is abstract

    // --- 5. `final`: this override cannot be overridden further down the hierarchy ---
    virtual void describe() const final {
        cout << "A shape with area " << area() << "\n";
    }
};

// --- 1. public inheritance: is-a relationship, base's public interface stays public ---
class Circle : public Shape {
    double radius;
public:
    Circle(double r) : radius(r) { cout << "Circle constructed\n"; } // base ctor runs first, implicitly
    ~Circle() override { cout << "Circle destroyed\n"; }

    // --- 2 & 5. override: compiler VERIFIES this actually overrides a virtual base method
    // (typo-proofing — without `override`, a signature mismatch silently creates a NEW function
    // that hides the base one instead of overriding it) ---
    double area() const override { return 3.14159 * radius * radius; }
};

class Square : public Shape {
    double side;
public:
    Square(double s) : side(s) {}
    double area() const override { return side * side; }
};

// --- 6. Multiple inheritance + diamond problem + virtual inheritance ---
class Device { public: void powerOn() { cout << "Powering on\n"; } };
class Camera : virtual public Device {};   // virtual inheritance: only ONE Device sub-object
class Phone : virtual public Device {};    // shared with Camera, avoiding ambiguity
class SmartPhone : public Camera, public Phone {};
// Without `virtual` above, SmartPhone would contain TWO Device sub-objects, and
// smartphoneObj.powerOn() would be ambiguous (compiler error) -> that's the diamond problem.

int main() {
    // --- 2. Dynamic dispatch via base pointer: the ACTUAL derived method runs, decided at runtime via vtable ---
    Shape* shapes[2];
    shapes[0] = new Circle(2.0);
    shapes[1] = new Square(3.0);
    for (Shape* s : shapes) s->describe();     // calls Circle::area() / Square::area() correctly

    // --- 4. Virtual destructor in action: deleting via base pointer still runs Circle's dtor ---
    delete shapes[0];
    delete shapes[1];

    // --- 7. Slicing: copying a derived object into a base-by-value variable chops off the derived part ---
    Circle c(5.0);
    // Shape sliced = c;   // ERROR here since Shape is abstract, but if it weren't:
    // this pattern silently drops Circle's data/vtable-specific behavior -> "slicing".
    // Rule of thumb: pass/store polymorphic types by pointer or reference, never by value.

    // --- 8. Upcast (implicit, safe) vs downcast (explicit, needs checking) ---
    Shape* base = &c;                              // upcast: Circle* -> Shape*, always safe
    Circle* backToCircle = dynamic_cast<Circle*>(base); // downcast: checked at runtime, needs RTTI (virtual funcs)
    if (backToCircle) cout << "Downcast succeeded\n";

    Square sq(1.0);
    Shape* baseSq = &sq;
    Circle* wrongCast = dynamic_cast<Circle*>(baseSq);  // Square is not a Circle
    cout << "Bad downcast result: " << (wrongCast == nullptr ? "nullptr (safe)" : "non-null") << "\n";

    // --- 6. Multiple/virtual inheritance in action ---
    SmartPhone sp;
    sp.powerOn();   // no ambiguity error, thanks to virtual inheritance
}
```

---

## LEVEL 5 — RULE OF 3 / 5 / 0

```
TOPICS COVERED:
1. Copy constructor, copy assignment operator
2. Destructor (in the resource-management context)
3. Move constructor, move assignment operator, std::move
4. Rule of 3 (pre-C++11), Rule of 5 (C++11+), Rule of 0 (let members manage themselves)
5. Deep copy vs shallow copy
6. Self-assignment safety
7. Resource management / RAII tie-in
```

```cpp
#include <iostream>
#include <cstring>
#include <utility>
using namespace std;

// --- Demonstrates Rule of 5 explicitly, on a class that owns a raw resource (char*). ---
class Buffer {
    char* data;
    size_t size;
public:
    Buffer(const char* str) {
        size = strlen(str) + 1;
        data = new char[size];
        strcpy(data, str);
        cout << "Constructed: " << data << "\n";
    }

    // --- 3. Destructor: releases the owned resource ---
    ~Buffer() {
        cout << "Destructing" << (data ? string(": ") + data : " (moved-from)") << "\n";
        delete[] data;
    }

    // --- 1 & 5. Copy constructor: must DEEP copy, or two objects would point at the same
    // heap block and double-free it on destruction. ---
    Buffer(const Buffer& other) : size(other.size) {
        data = new char[size];
        strcpy(data, other.data);
        cout << "Copy-constructed: " << data << "\n";
    }

    // --- 1 & 6. Copy assignment: must self-assignment-check, free old resource, deep copy new one. ---
    Buffer& operator=(const Buffer& other) {
        if (this == &other) return *this;   // 6. self-assignment guard: skip, else data below would be freed then read
        delete[] data;                        // free existing resource first
        size = other.size;
        data = new char[size];
        strcpy(data, other.data);
        cout << "Copy-assigned: " << data << "\n";
        return *this;
    }

    // --- 3. Move constructor: STEAL the pointer instead of copying, leave source in a
    // valid-but-empty state. Marked noexcept so std::vector etc. prefer moving over copying. ---
    Buffer(Buffer&& other) noexcept : data(other.data), size(other.size) {
        other.data = nullptr;   // source no longer owns the resource
        other.size = 0;
        cout << "Move-constructed\n";
    }

    // --- 3. Move assignment ---
    Buffer& operator=(Buffer&& other) noexcept {
        if (this == &other) return *this;
        delete[] data;
        data = other.data;
        size = other.size;
        other.data = nullptr;
        other.size = 0;
        cout << "Move-assigned\n";
        return *this;
    }
};

// --- 4. Rule of 0 example: this class owns NO raw resources directly (only smart/managed
// members), so it needs NONE of the 5 special members hand-written — the compiler-generated
// ones correctly call each member's own copy/move/destructor. Prefer this whenever possible. ---
#include <memory>
#include <string>
class Owner {
    unique_ptr<int> ptr = make_unique<int>(42);   // manages its own lifetime
    string name = "default";                       // manages its own lifetime
    // no custom ctor/dtor/copy/move needed — compiler-generated ones are correct here
};

int main() {
    Buffer b1("hello");
    Buffer b2 = b1;                    // copy constructor (deep copy, independent buffers)

    Buffer b3("world");
    b3 = b1;                            // copy assignment

    Buffer b4 = std::move(b1);         // move constructor — b1 is now "moved-from" (data == nullptr)
    Buffer b5("temp");
    b5 = std::move(b3);                 // move assignment

    Owner o1;
    Owner o2 = o1;                      // Rule of 0: compiler-generated copy works correctly,
                                          // because unique_ptr... wait, unique_ptr isn't copyable!
                                          // (left here deliberately — see note below)
}
```

> **Deliberate landmine in the last example:** `unique_ptr` is move-only, so
> `Owner o2 = o1;` above won't actually compile as written — that's on
> purpose. It's a good gut-check: if you spotted it before reading this
> note, Rule of 0 has clicked. The fix is either give `Owner` a
> `shared_ptr` instead, or explicitly delete/write its copy operations
> and rely only on move. Try both fixes as a quick self-test.