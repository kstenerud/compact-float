Compact Float Format
====================

Compact float format is an encoding scheme to store a decimal floating point value in as few bytes as possible for data transmission.

Compact float can store all of the kinds of values that [IEEE754 decimal types](https://en.wikipedia.org/wiki/IEEE_754) can to unlimited range, and without data loss:
* ±0
* ±infinity
* Signaling and quiet NaNs



Specifications
--------------

* [Compact Float Format Specification](compact-float-specification.md)



Implementations
---------------

* [C implementation](https://github.com/kstenerud/c-compact-float)
* [Go implementation](https://github.com/kstenerud/go-compact-float)



License
-------

Copyright 2019 Karl Stenerud. All rights reserved.

Specifications released under [Creative Commons Attribution 4.0 International Public License](LICENSE.md).
