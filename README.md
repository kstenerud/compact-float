Compact Float Format
====================

Compact float format is an encoding scheme to store a decimal floating point value in as few bytes as possible for data transmission.

Compact float can store all of the kinds of values that the IEEE754 decimal types can, without data loss:
* ±0
* ±infinity
* Signaling and quiet NaNs


This specification is part of the [Specifications Project](https://github.com/kstenerud/specifications)



Specifications
--------------

* [Compact Float Format Specification](compact-float-specification.md)



Implementations
---------------

* [C implementation](https://github.com/kstenerud/c-compact-float)
* [Go implementation](https://github.com/kstenerud/go-compact-float)



License
-------

Copyright Karl Stenerud. All rights reserved.

Specifications released under [Creative Commons Attribution 4.0 International Public License](LICENSE.md).
