Compact Float Format
====================

Compact float format (CFF) is an encoding scheme to store a floating point value in as few bytes as possible for data transmission. CFF supports all values that can be stored in any sized ieee754 decimal and binary floating point encoding.

CFF can store all of the kinds of values that ieee754 can, without data loss:
* Binary and decimal floating point values
* Normal values
* Subnormal values
* ±0
* ±infinity
* Signaling and quiet NaNs, including payload


This specification is part of the [Specification Project](https://github.com/kstenerud/specifications)



Specifications
--------------

* [Compact Float Format Specification](compact-float-specification.md)



Implementations
---------------

* TODO



License
-------

Copyright 2019 Karl Stenerud

Specification released under Creative Commons Attribution 4.0 International Public License https://creativecommons.org/licenses/by/4.0/

Reference implementation released under MIT License https://opensource.org/licenses/MIT
