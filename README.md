Compact Float Format
====================

Compact float format (CFF) is an encoding scheme to store a floating point value in as few bytes as possible for data transmission. This format supports all values that can be stored in 128 bit ieee754 floating point values (decimal and binary).

CFF can store all of the kinds of values that ieee754 can without data loss:
* Normalized values
* Subnormal values
* Positive and negative 0
* Positive and negative infinity
* NaNs (not a number)



Specifications
--------------

This specification is part of the [Specification Project](https://github.com/kstenerud/specifications)

* [Compact Float Format Specification](compact-float-specification.md)



Implementations
---------------

* TODO



License
-------

Copyright 2019 Karl Stenerud

Specification released under Creative Commons Attribution 4.0 International Public License https://creativecommons.org/licenses/by/4.0/

Reference implementation released under MIT License https://opensource.org/licenses/MIT
