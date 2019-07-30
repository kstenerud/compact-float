Reference Implementation for Compact Float
==========================================

A C implementation to demonstrate compact float.



Usage
-----

```cpp
```

Output:



```cpp
```

Output:




Requirements
------------

  * Meson 0.49 or newer
  * Ninja 1.8.2 or newer
  * A C compiler
  * A C++ compiler (for the tests)



Building
--------

    meson build
    ninja -C build



Running Tests
-------------

    ninja -C build test

For the full report:

    ./build/run_tests



Installing
----------

    ninja -C build install
