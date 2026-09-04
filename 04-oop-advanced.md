# 🧱 Level 4 — OOP Advanced: Inheritance & Polymorphism

> **Prereq mindset:** a derived object physically *contains* its base sub-object inside it. Everything below follows from that one fact.

## 📑 Topics
1. [Inheritance & Access Levels](#1-inheritance--access-levels)
2. [Constructor / Destructor Order](#2-constructor--destructor-order)
3. [Virtual Functions & Dynamic Dispatch](#3-virtual-functions--dynamic-dispatch)
4. [Vtable & Vptr — How Dispatch Actually Works](#4-vtable--vptr--how-dispatch-actually-works)
5. [Pure Virtual Functions & Abstract Classes](#5-pure-virtual-functions--abstract-classes)
6. [Virtual Destructors](#6-virtual-destructors)
7. [`override` and `final`](#7-override-and-final)
8. [Overriding vs Hiding](#8-overriding-vs-hiding)
9. [Multiple Inheritance & the Diamond Problem](#9-multiple-inheritance--the-diamond-problem)
10. [Object Slicing](#10-object-slicing)
11. [Upcasting / Downcasting, `static_cast` vs `dynamic_cast`](#11-upcasting--downcasting-static_cast-vs-dynamic_cast)
12. [Interface Pattern (ABCs)](#12-interface-pattern-abcs)

---

## 1. Inheritance & Access Levels

Inheritance exists to avoid duplicating code across related classes. The keyword you inherit *with* controls how the base class's members are seen inside the derived class (**not** whether outsiders can see them — that's a separate question).

| Base member | `public` inheritance | `protected` inheritance | `private` inheritance |
|---|---|---|---|
| `public`    | stays `public`    | becomes `protected` | becomes `private` |
| `protected` | stays `protected` | becomes `protected` | becomes `private` |
| `private`   | inaccessible       | inaccessible         | inaccessible |

```cpp
class Derived : public Base    {};  // "IS-A"                — most common
class Derived : protected Base {};  // rare, framework-internal use
class Derived : private Base   {};  // "implemented-in-terms-of"
```

> 💡 **Rule of thumb:** default to `public` inheritance unless you specifically want to hide the "IS-A" relationship from the outside world (see §11 for the IS-A vs implemented-in-terms-of distinction with a real example).

---

## 2. Constructor / Destructor Order

**Base constructs first, derived constructs last.** Destruction is the exact mirror image.

```cpp
class A {};
class B : public A {};
class C : public B {};
```

```
Construction order:  A → B → C
Destruction order:   C → B → A
```

```cpp
class Animal {
public:
    Animal()  { cout << "Animal\n"; }
    ~Animal() { cout << "~Animal\n"; }
};
class Dog : public Animal {
public:
    Dog()  { cout << "Dog\n"; }
    ~Dog() { cout << "~Dog\n"; }
};

Dog d;
// construction: Animal → Dog
// (d goes out of scope) destruction: ~Dog → ~Animal
```

### Passing arguments to a base constructor
If `Base` has no default constructor (or you don't want the default), you must call it explicitly in the derived class's **member initializer list**:

```cpp
class Animal {
public:
    Animal(int age) : age_(age) {}
private:
    int age_;
};

class Dog : public Animal {
public:
    Dog(int age, string name) : Animal(age), name_(name) {}
private:
    string name_;
};
```

---

## 3. Virtual Functions & Dynamic Dispatch

### The problem virtual functions solve

```cpp
class Base {
public:
    void speak() { cout << "Base speaks\n"; }
};
class Derived : public Base {
public:
    void speak() { cout << "Derived speaks\n"; }
};

Base* p = new Derived();
p->speak();   // prints "Base speaks"  <-- surprising!
```

Without `virtual`, the compiler decides **at compile time** which `speak()` to call, purely by looking at the **declared (static) type of the pointer** (`Base*`). This is called **static dispatch**. It has no idea `p` actually points at a `Derived` at runtime.

### The fix

```cpp
class Base {
public:
    virtual void speak() { cout << "Base speaks\n"; }
};
class Derived : public Base {
public:
    void speak() override { cout << "Derived speaks\n"; }
};

Base* p = new Derived();
p->speak();   // now prints "Derived speaks"
```

Marking `speak()` `virtual` in the base switches the call to **dynamic dispatch**: the *actual* type of the object `p` points to (checked at **runtime**) decides which override runs. This is also called **runtime polymorphism**.

```cpp
Animal* p = condition ? (Animal*)new Dog() : (Animal*)new Cat();
p->speak();   // only known at runtime which one this calls
```

> ⚠️ **Key mental model — write this on a sticky note:**
> - **Non-virtual function** → resolved by the **static/declared type of the pointer or reference** you're calling through, decided at **compile time**.
> - **Virtual function** → resolved by the **actual/dynamic type of the object** the pointer/reference points to, decided at **runtime**. This is true *no matter what pointer type you're holding it through* — even if you `static_cast` the pointer back to `Base*`, calling a virtual function through it still reaches the most-derived override. **The pointer's type only controls what's visible/callable — never which override of a *virtual* function actually runs.**

---

## 4. Vtable & Vptr — How Dispatch Actually Works

- **vtable** (virtual table): a per-**class** (not per-object) static array of function pointers, one slot per virtual function, built once by the compiler. Each entry points to whichever override is correct *for that class*.
- **vptr** (virtual pointer): a hidden pointer stored inside **every object instance** of a class that has at least one virtual function. It points to that class's vtable.

This works identically for heap objects, stack objects, and objects nested inside other objects — the vptr lives *in the object itself*, not in the pointer you're using to access it. That's exactly why `b3->printVirtual()` in Q7 still reaches the derived override even after a `static_cast<Base*>`: the cast only changes what the *compiler* thinks the pointer's type is; the object in memory, and its vptr, never changed.

```
Circle obj
┌─────────────┐
│ vptr ───────┼──> Circle's vtable ──> [ &Circle::area, &Circle::~Circle, ... ]
│ radius      │
└─────────────┘
```

Call sequence for `shapePtr->area()`:
1. Follow `shapePtr` to the object.
2. Read the object's **vptr**.
3. Look up the `area` slot in the vtable it points to.
4. Call whatever function pointer is sitting in that slot.

That extra indirection (step 2–4 vs. a direct call) is the "cost" of virtual functions — one pointer dereference and one indirect call, instead of a direct call resolved at compile time. Negligible for almost everything you'll write; it starts to matter in hot inner loops (revisit in the Level 17 performance discussion).

---

## 5. Pure Virtual Functions & Abstract Classes

A **pure virtual function** has no implementation *requirement* in the base — it's declared with `= 0`:

```cpp
class Animal {
public:
    virtual void speak() = 0;   // pure virtual — no body required
};
```

Any class with **at least one** pure virtual function is an **abstract class**:
- ✅ Can have constructors, destructors, data members, and regular (non-pure) functions.
- ❌ **Cannot be instantiated** — `Animal a;` is a compile error.
- A derived class only becomes concrete (instantiable) once it overrides **every** pure virtual function it inherited.

```cpp
Animal a;              // ❌ compile error: cannot instantiate abstract class
Animal* p = new Dog();  // ✅ fine — Dog overrides speak()
```

> 🧠 **Gotcha (this is exactly what tripped up Q3):** you cannot write a function parameter `void printArea(Shape s)` **by value** if `Shape` is abstract, because passing by value requires constructing a `Shape` object — which is illegal. This is precisely why the working version of the exercise had to take `Shape*` (or would need `const Shape&`) instead. Pass-by-pointer/reference never constructs a new base object, so it's legal even for abstract types. See the Slicing section (§10) for how to actually demonstrate slicing (it needs a *non-abstract* class).

### Pure virtual functions *can* still have a body
Uncommon, but legal — useful when a derived override wants to explicitly reuse the base's behavior as a starting point:

```cpp
class Base {
public:
    virtual void foo() = 0;   // still pure virtual — Base still abstract
};
void Base::foo() { cout << "Base default behavior\n"; }

class Derived : public Base {
public:
    void foo() override {
        Base::foo();          // explicitly call the base body
        cout << "Derived extra behavior\n";
    }
};
```

---

## 6. Virtual Destructors

**The rule:** if you will ever `delete` an object **through a base class pointer**, the base class destructor **must** be `virtual`.

```cpp
class Base {
public:
    ~Base() { cout << "Base\n"; }          // NOT virtual — bug waiting to happen
};
class Derived : public Base {
public:
    ~Derived() { cout << "Derived\n"; }
};

Base* p = new Derived();
delete p;
// prints only "Base"
// ~Derived() NEVER RUNS -> any resources Derived owns (memory, file handles,
// sockets...) leak, and this is undefined behavior territory.
```

Fix:

```cpp
class Base {
public:
    virtual ~Base() { cout << "Base\n"; }
};
// now delete p; correctly prints:
// Derived
// Base
```

> 💡 **On the syntax question — "why does a destructor have `()` if it can't take parameters?"**
> Parentheses in C++ are simply the *function-call syntax marker*, independent of whether the function accepts arguments (plenty of zero-argument functions use `()`, e.g. `foo()`). A destructor is a member function like any other under the hood (compiler-called automatically, or callable manually as `obj.~ClassName()`), so it keeps the same call syntax for consistency — it just happens to never accept parameters and never returns anything, not even `void`.

> 💡 **`virtual ~Shape() = default;` vs `virtual ~Shape() = 0;`** — both make the destructor virtual, but they mean different things:
> - `= default` → a normal virtual destructor, compiler-generated body. Use this in the overwhelming majority of cases (this is what you correctly used for `Shape`).
> - `= 0` → a **pure virtual destructor**. This *also* forces the class to be abstract (on top of any other pure virtuals it may have) — but unusually, a pure virtual destructor still **needs a definition** somewhere, because every derived class's destructor implicitly calls the base destructor at the end of the destruction chain, and that call needs something to link against:
>   ```cpp
>   class Shape {
>   public:
>       virtual ~Shape() = 0;   // pure, but...
>   };
>   Shape::~Shape() {}          // ...still needs a body!
>   ```
>   Since `Shape` was already abstract because of `area() = 0`, there was no reason to also make the destructor pure — `= default` was the right (simpler) choice.

> 🧠 **Rule of thumb:** if a class has *any* virtual function, or is ever meant to be used polymorphically (deleted through a base pointer), give it a virtual destructor. Cost is one extra vtable entry — always worth it.

---

## 7. `override` and `final`

### `override`
Put this on every function you intend to be an override. It doesn't change runtime behavior — it's purely a compile-time safety net: the compiler verifies the signature actually matches a virtual function in the base. Without it, a typo like a mismatched `const` or parameter type silently creates a brand-new, unrelated function instead of an override — and you get no error, just mysteriously wrong behavior at runtime.

```cpp
class Animal { public: virtual void speak() const {} };
class Dog : public Animal {
public:
    void speak() override {}   // ❌ compile error!
    // Animal::speak() is `const`, this one isn't -> different signature,
    // NOT an override -> override keyword catches it immediately.
};
```

Very very good example of override usage with const
```cpp
class Shape {
public:
    virtual double area() const = 0;
    virtual ~Shape() = default;
};

class Circle : public Shape {
public:
    int radius;
    double area() override {   // ❌ compile error, without override this could compile (causing error later)
        return radius * radius;
    }
};
```

### `final`
Two uses:

```cpp
// 1. Prevent a specific virtual function from being overridden further
class Dog : public Animal {
public:
    void speak() final { ... }
};
class Labrador : public Dog {
public:
    void speak() override { ... }  // ❌ compile error: speak() is final in Dog
};

// 2. Prevent the whole class from being inherited from
class Dog final : public Animal {};
class GermanShepherd : public Dog {};  // ❌ compile error: base 'Dog' is marked 'final'
```

You can combine both: `void speak() override final {}`.

---

## 8. Overriding vs Hiding

This is one of the sharpest edges in C++ inheritance — and the source of the confusion in Q8.

**Overriding** = same name, same full signature, base function is `virtual` → dynamic dispatch applies.

**Name hiding** = derived class declares *any* function with the same name as one in the base (regardless of parameters) → it hides **all** base overloads of that name, virtual or not.

```cpp
class Base {
public:
    void foo();
    void foo(int);
};
class Derived : public Base {
public:
    void foo(double);   // hides BOTH Base::foo() and Base::foo(int)!
};

Derived d;
d.foo(5);      // ❌ compile error — foo() and foo(int) are hidden, only foo(double) is visible
d.foo(2.5);    // ✅ ok
```

**Unhide with a `using` declaration:**

```cpp
class Derived : public Base {
public:
    using Base::foo;      // brings ALL Base::foo overloads back into scope
    void foo(double) { cout << "double\n"; }
};

Derived d;
d.foo();     // ✅ Base::foo()
d.foo(5);    // ✅ Base::foo(int)
d.foo(2.5);  // ✅ Derived::foo(double)
```

### 🎯 Why Q8's actual output happened (and it is *not* slicing)

```cpp
class BaseQ8 {
public:
    void notVirtualPrint()          { cout << "BASE: not virtual\n"; }
    virtual void virtualPrint()     { cout << "BASE: virtual\n"; }
};
class DerivedQ8 : public BaseQ8 {
public:
    void notVirtualPrint()          { cout << "DERIVED: not virtual\n"; }
    void virtualPrint() override    { cout << "DERIVED: virtual\n"; }
};

BaseQ8* b = new DerivedQ8();
b->notVirtualPrint();  // "BASE: not virtual"
b->virtualPrint();     // "DERIVED: virtual"
```

No copying happens anywhere in this code — `b` is a pointer to a heap-allocated `DerivedQ8`, so **slicing cannot be the explanation** (slicing only happens when you copy an object *by value* into a base-typed variable — see §10). What's actually happening is exactly the static-vs-dynamic-binding rule from §3:

- `notVirtualPrint()` is **not virtual** → resolved at **compile time** using `b`'s *static* type, `BaseQ8*` → calls `BaseQ8::notVirtualPrint`.
- `virtualPrint()` **is virtual** → resolved at **runtime** via the object's vptr, which points at `DerivedQ8`'s vtable → calls `DerivedQ8::virtualPrint`.

Same pointer, two different resolution rules, because one function is virtual and the other isn't. This is the whole point of §3's mental model box.

---

## 9. Multiple Inheritance & the Diamond Problem

A class can inherit from more than one base — fine on its own:

```cpp
class Flyable    { public: void fly()  {} };
class Swimmable  { public: void swim() {} };
class Duck : public Flyable, public Swimmable {};  // d.fly(), d.swim() both fine
```

The **diamond problem** appears specifically when two bases you're inheriting from **share a common ancestor**:

```
      Animal
      /    \
  Mammal   Bird
      \    /
       Bat
```

```cpp
class Animal  { public: int age; };
class Mammal  : public Animal {};
class Bird    : public Animal {};
class Bat     : public Mammal, public Bird {};

Bat b;
b.age = 10;   // ❌ ambiguous! compiler doesn't know Mammal::Animal::age or Bird::Animal::age
```

`Bat` ends up containing **two separate `Animal` sub-objects** — one via `Mammal`, one via `Bird`. You *can* disambiguate manually:

```cpp
b.Mammal::age = 5;
b.Bird::age = 10;   // two independent ages — almost certainly not what you want
```

But usually you want **one shared `Animal`**, which is what **virtual inheritance** gives you:

```cpp
class Animal  { public: int age; };
class Mammal  : virtual public Animal {};
class Bird    : virtual public Animal {};
class Bat     : public Mammal, public Bird {};

Bat b;
b.age = 10;   // ✅ fine — only one Animal now
```

### Construction order changes with virtual inheritance
Normally each intermediate class constructs its own base. With a **virtual** base, the rule changes: **the most-derived class is responsible for constructing the virtual base directly**, and any `Base(...)` calls written in the intermediate classes' initializer lists are **ignored** if the most-derived class also specifies one.

```cpp
class Animal {
public:
    Animal(int x) { cout << "Animal(" << x << ")\n"; }
};
class Mammal : virtual public Animal {
public:
    Mammal() : Animal(1) {}
};
class Bird : virtual public Animal {
public:
    Bird() : Animal(2) {}
};
class Bat : public Mammal, public Bird {
public:
    Bat() : Animal(100) {}   // this wins
};

Bat b;   // prints: Animal(100)   -- Animal(1) and Animal(2) are both skipped
```

> ⚠️ **Note on your Q6 code specifically:** `class A { int x; };` makes `x` **private by default** (class members default to `private`). That means even *without* the diamond issue, `B` and `C` can't touch `x` directly — you'd need `public:` (or `protected:`) on `x` first before the diamond ambiguity is even the thing stopping you. Small thing, but worth fixing if you rerun this experiment.

---

## 10. Object Slicing

**Slicing** happens when you copy a `Derived` object **by value** into a `Base`-typed variable — the derived-only parts get silently chopped off, keeping only the `Base` portion.

```cpp
class Animal { public: int age = 5; };
class Dog : public Animal { public: int weight = 20; };

Dog d;
Animal a = d;   // ⚠️ SLICED — copy constructor for Animal only copies the Animal part

// a.age    -> 5   (fine, this is the Animal part)
// a.weight -> ❌ doesn't exist on 'a' at all — Animal has no 'weight' member
```

It also silently breaks virtual dispatch, because `a` is now a genuinely separate, full `Animal` object — there's no vptr trickery that can recover the `Dog` behavior once the copy has happened:

```cpp
class Animal { public: virtual void speak() { cout << "Animal\n"; } };
class Dog : public Animal { public: void speak() override { cout << "Dog\n"; } };

void printSpeak(Animal a) {   // <-- by VALUE, this is the trap
    a.speak();
}

Dog d;
printSpeak(d);   // prints "Animal" — d was sliced into a plain Animal on the way in
```

**Slicing never happens through pointers or references**, because nothing is copied — you're still looking at the original object:

```cpp
Animal& a = d;   // no slicing — reference, no copy
Animal* p = &d;  // no slicing — pointer, no copy
p->speak();      // still prints "Dog" — full virtual dispatch intact
```

> 🧠 **Why Q3's exercise as literally written couldn't work:** `void printArea(Shape s)` needs `Shape` to be copyable-by-value, but `Shape` is abstract (pure virtual `area()`), so you can't even construct a plain `Shape` to slice into — the compiler rejects it outright, before slicing is even a question. To actually *demonstrate* slicing, you need a **non-abstract** base with a **virtual function that has a real implementation**, like the `Animal`/`Dog` example just above. Your instinct to switch to `Shape*` was the right engineering call for *this* specific case — just note that it sidesteps slicing entirely rather than fixing it, since pointers were never at risk of slicing to begin with.

### 4 ways to avoid slicing
1. **Pass by reference:** `void foo(const Animal& a);`
2. **Pass/store by pointer:** `Animal* p;`
3. **Store pointers in containers:** `vector<unique_ptr<Animal>>` instead of `vector<Animal>`
4. **Delete the copy constructor**, forcing a compile error at the slicing site:
   ```cpp
   class Animal {
   public:
       Animal(const Animal&) = delete;
   };
   ```

---

## 11. Upcasting / Downcasting, `static_cast` vs `dynamic_cast`

### Upcasting (Derived → Base) — implicit, always safe
A derived object physically contains its base sub-object, so this needs no cast at all:

```cpp
Dog d;
Animal* p  = &d;   // implicit upcast
Animal& ref = d;   // implicit upcast
```

### Downcasting (Base → Derived) — needs an explicit cast
Not every `Animal` is a `Dog`, so the compiler won't do this for you automatically.

**`dynamic_cast`** — safe, checked at runtime, but requires the base class to be **polymorphic** (have at least one virtual function — a virtual destructor is enough):

```cpp
class Animal { public: virtual ~Animal() = default; };  // must have >=1 virtual fn
class Dog : public Animal {};
class Cat : public Animal {};

Animal* p = new Dog();
Dog* d = dynamic_cast<Dog*>(p);   // ✅ works, d points at the Dog

Animal* p2 = new Cat();
Dog* d2 = dynamic_cast<Dog*>(p2); // no compile error — d2 is simply nullptr at runtime
if (d2 == nullptr) { /* cast failed safely */ }
```

How does it know? **RTTI (Run-Time Type Information)**: when a class has at least one virtual function, the compiler stores metadata describing the object's *real* type alongside the vtable. `dynamic_cast` walks that metadata at runtime to check whether the cast is actually valid.

- On **pointers**, a failed `dynamic_cast` yields `nullptr`.
- On **references**, there's no "null reference" to fall back on, so a failed `dynamic_cast` **throws `std::bad_cast`** instead:
  ```cpp
  Animal& a = cat;
  Dog& d = dynamic_cast<Dog&>(a);  // throws std::bad_cast
  ```
- No virtual functions at all on the base → `dynamic_cast` is a **compile error** (no RTTI to check against).

**`static_cast`** — no runtime check, no safety net, but faster (compiles to a plain pointer adjustment):

```cpp
Animal* p1 = new Dog();
Animal* p2 = new Cat();

Dog* d1 = static_cast<Dog*>(p1);   // ✅ actually correct, works fine
Dog* d2 = static_cast<Dog*>(p2);   // 💥 compiles fine, but UNDEFINED BEHAVIOR at runtime
                                    //    — static_cast just trusts you and does no checking
```

> 🎯 **Why Q7's `b3->printVirtual()` printed the derived version even after `static_cast<Base*>`:** this is the exact same principle from §3/§4 — casting a pointer only changes what the **compiler** believes the *static* type is (i.e., what member names are visible to write in your code). It does **nothing** to the object in memory or its vptr. `printVirtual()` is virtual, so the call still walks through the real object's vptr at runtime and lands on `Derived1::printVirtual()`, completely regardless of what pointer type you're calling it through. Casting the pointer type and changing the object's actual type are two totally different things — only the latter would affect virtual dispatch, and casts never do that.

### Rule of thumb
| Situation | Use |
|---|---|
| Derived → Base | nothing — implicit upcast |
| Base → Derived, not 100% sure it's safe | `dynamic_cast` (checked, slower) |
| Base → Derived, you are certain | `static_cast` (unchecked, faster) |

---

## 12. Interface Pattern (ABCs)

C++ has no `interface` keyword like Java. Instead, the convention is an **abstract base class where every function is pure virtual and there's no data**:

```cpp
class Drawable {
public:
    virtual void draw() = 0;
    virtual ~Drawable() = default;   // always give interfaces a virtual destructor
};

class Circle : public Drawable    { public: void draw() override { /* ... */ } };
class Rectangle : public Drawable { public: void draw() override { /* ... */ } };
class Triangle : public Drawable  { public: void draw() override { /* ... */ } };
```

The payoff: one function handles every implementer, instead of one overload per concrete type.

```cpp
void render(Drawable& obj) { obj.draw(); }   // works for Circle, Rectangle, Triangle, ...

// without the interface, you'd be stuck writing:
// void renderCircle(Circle&);
// void renderRectangle(Rectangle&);
// void renderTriangle(Triangle&);
```

A class can implement **multiple interfaces** with no diamond-problem risk (since interfaces carry no data, there's nothing to duplicate) — this is a large part of why "many small interfaces, multiply inherited" is considered good C++ design, unlike multiple inheritance of classes-with-data.

---

## 🎓 Quick-Reference Cheat Sheet

| Concept | One-line rule |
|---|---|
| Non-virtual call | Resolved by **static (pointer/reference) type**, compile time |
| Virtual call | Resolved by **dynamic (actual object) type**, runtime, via vptr |
| Abstract class | Has ≥1 pure virtual (`= 0`) function; cannot instantiate |
| Virtual destructor | Mandatory if deleting through a base pointer |
| `override` | Compiler-checked "I mean to override this" — always use it |
| `final` | Blocks further overriding (function) or inheriting (class) |
| Name hiding | Same name in derived hides **all** base overloads of that name |
| Diamond problem | Two bases sharing an ancestor → duplicate sub-objects |
| Virtual inheritance | Fixes the diamond by sharing one common base sub-object |
| Slicing | Copying Derived **by value** into Base var → derived part lost |
| Slicing fix | Pass/store by pointer or reference, or `= delete` the copy ctor |
| `dynamic_cast` | Safe, checked, needs polymorphic base, `nullptr`/`bad_cast` on failure |
| `static_cast` | Fast, unchecked, UB if you're wrong |