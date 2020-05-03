Compact Float Format
====================

Compact float format is an encoding scheme to store a decimal floating point value in as few bytes as possible for data transmission.

Compact float format can store the same special values that the IEEE754 decimal types can:

* ±0
* ±infinity
* Signaling and quiet NaNs



Contents
--------

* [Encoded Structure](#encoded-structure)
  - [Exponent Structure](#exponent-structure)
  - [Significand Structure](#significand-structure)
* [Special Values](#special-values)
  - [Zero](#zero)
  - [Infinity](#infinity)
  - [NaN](#nan)
* [Rounding](#rounding)
* [Smallest Possible Size](#smallest-possible-size)
* [How much precision do you need?](#how-much-precision-do-you-need)
* [Examples](#examples)
* [License](#license)



Encoded Structure
-----------------

The general conceptual form of a floating point number is:

    value = sign * significand * 10 ^ (exponent * exponent_sign)

A compact float value is encoded into one or two [unsigned LEB128](https://en.wikipedia.org/wiki/LEB128) structures, depending on the value being stored:

#### Normal Value Encoding:

    [Exponent structure (ULEB128)] [Significand structure (ULEB128)]

#### [Special Value](#special-values) Encoding:

    [Exponent structure (ULEB128)]



### Exponent Structure

The exponent structure is a bit field containing the significand sign, exponent sign, and exponent magnitude:

| Field              | Bits | Notes                      |
| ------------------ | ---- | -------------------------- |
| Exponent Magnitude |   5+ | exponent high bits         |
| Exponent Sign      |    1 | 0 = positive, 1 = negative |
| Significand Sign   |    1 | 0 = positive, 1 = negative |

`exponent magnitude` is an absolute integer value representing (10 ^ exponent). The `exponent sign` determines whether the exponent is multiplied by `1` or `-1`.

#### Extended Exponent

Some special values are signaled by an extended exponent, which is a ULEB128 that is encoded into one more group than is necessary to hold the value.

Examples:

| Normal RVLQ | Extended RVLQ |
| ----------- | ------------- |
| `[00]`      | `[80 00]`     |
| `[7c]`      | `[fc 00]`     |


### Significand Structure

| Field                 | Bits |
| --------------------- | ---- |
| Significand Magnitude |   7+ |

The `significand magnitude` is an absolute integer value that will be multiplied by the `significand sign` and the `exponent`.



Special Values
--------------

Special values are encoded entirely into the exponent structure, and have no significand structure.


### Zero

Zero values (±0) are encoded using an exponent value of `-0`, which is an otherwise impossible value. The significand sign field determines whether it's positive or negative 0.

    00000010 = [02] = +0
    00000011 = [03] = -0


### Infinity

Infinity is encoded using an [extended exponent](#extended-exponent) that encodes the exponent value `-0`. The significand sign field determines whether it's positive or negative infinity.

    10000010 00000000 = [82 00] = +infinity
    10000011 00000000 = [83 00] = -infinity


### NaN

NaN (not-a-number) is encoded using an [extended exponent](#extended-exponent) that encodes the exponent value `0`. The significand sign field determines whether it's a quiet or signaling NaN:

    10000000 00000000 = [80 00] = Quiet NaN
    10000001 00000000 = [81 00] = Signaling NaN



Rounding
--------

When rounding for storage in compact float format, values must be rounded using IEEE754's **round half to even** method (also known as banker's rounding), unless all sending and receiving parties have agreed to another method.



Smallest Possible Size
----------------------

As compact float values are not normalized, the same value can be represented in many ways. An encoder must store values in as few bytes as possible without losing data.

For example, the value 4.09104981 rounded to 5 significant digits is 4.0910, which could be naively encoded as follows:

    Significand: 40910
    Exponent: -4
    Effective value: 4.091

Encoding 5 significant digits would require 4 bytes. However, since the last digit is 0, it may be removed from the encoding while still resulting in the same effective value:

    Significand: 4091
    Exponent: -3
    Effective value: 4.091

Encoding 4 significant digits would require only 3 bytes.



How much precision do you need?
-------------------------------

When rounding float values, consider what level of precision you actually need; it's probably less than you'd think.

* With 6 significant digits, you'd have a margin of error of 1mm for a measure of 1km, for example. If you're having trouble visualizing this, it's roughly equivalent to worrying over a thousandth of an inch (or a tenth of a millimeter) when measuring an American football field.
* The [International Council for Science](https://physics.nist.gov/cuu/Constants/index.html) accepted physical constants are mostly between 9 and 12 significant digits.
* NASA's International Space Station Guidance Navigation and Control System (GNC) uses 15 significant digits.
* The Space Integrated Global Positioning System/Inertial Navigation System (SIGI) uses 16 significant digits.
* The fundamental constants of the universe are useful to 32 significant digits.
* 40 significant digits would allow measuring the entire universe with a margin of error less than the width of a hydrogen atom.

By comparison, ieee754 floating point format stores 7 significant digits for 32-bit values, 16 significant digits for 64-bit values, 34 significant digits for 128-bit values, and a whopping 71 significant digits for 256-bit values!

Storing values at a higher precision than your measuring system's accuracy leads to [false precision](https://en.wikipedia.org/wiki/False_precision).

For most real-world measures, you'll rarely need more than 4 significant digits outside of financial and high accuracy scientific data.

Storing more significant digits takes up more space. Here's a breakdown of the encoded compact float size in bytes for significant digits up to 16 (assuming 1 byte of exponent data, which gives a min/max exponent of ±31 for a range of almost ±0.000000000000000000000000000000001 to ±100,000,000,000,000,000,000,000,000,000,000):

| Digits | Bytes |
| ------ | ----- |
|   1-2  |   2   |
|   3-4  |   3   |
|   5-6  |   4   |
|   7-9  |   5   |
| 10-11  |   6   |
| 12-14  |   7   |
| 15-16  |   8   |



Examples
--------

### Binary float value ~ 0.5083299875259399, rounded to 4 significant digits

Binary floats are rarely directly representable in base 10. If we took the value 0.5083299875259399 as-is, we would be encoding many digits of false precision. For the purposes of this example we assume that, after auditing our measurement accuracy and precision requirements, we settle upon 4 significant digits, representing the value as 0.5083.

 * Significand: 5083
 * Exponent: -4

Exponent field contents:

    Exponent magnitude: 100
    Exponent sign:          1
    Significand sign:         0
    Total:              100 1 0

Build the exponent RVLQ:

    Value: 00010010
    Hex:   0x12

Significand field contents:

    Value:       5083 (0x13db)
    Binary:      0001 0011 1101 1011
    Groups of 7: 00 0100111 1011011
    As RVLQ:     11011011 00100111
    Hex:         0xdb     0x47

Result: `[12 db 47]`



License
-------

Copyright (C) Karl Stenerud

Specification released under Creative Commons Attribution 4.0 International Public License https://creativecommons.org/licenses/by/4.0/
