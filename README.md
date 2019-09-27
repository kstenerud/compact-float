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

Copyright 2019 Karl Stenerud

Specification released under Creative Commons Attribution 4.0 International Public License https://creativecommons.org/licenses/by/4.0/

Reference implementation released under MIT License https://opensource.org/licenses/MIT
