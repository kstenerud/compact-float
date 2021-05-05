Compact Float Format
====================

Compact float format is an encoding scheme to store a decimal floating point value in as few bytes as possible for data transmission. It can store the same regular and special values that the IEEE754 types can, with unlimite range.

**Features**:

 * Unlimited significand and exponent sizes
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
* [License](#license)



Encoded Structure
-----------------

The general conceptual form of a floating point number is:

    value = sign * significand * 10 ^ (exponent * exponent_sign)

A compact float value is encoded into one or two structures, depending on the value being stored. These structures are then encoded as [unsigned LEB128](https://en.wikipedia.org/wiki/LEB128).

#### Normal Value Encoding:

Normal values are stored in two adjacent ULEB128-encoded structures: one for the exponent, and one for the significand.

    [Exponent structure] [Significand structure]

#### Special Value Encoding:

[Special values](#special-values) such as NaNs and infinities are represented by single, special values that are 1-2 bytes long.

    [special value]


### Exponent Structure

The exponent structure is a ULEB128-encoded bit field containing the significand sign, exponent sign, and exponent magnitude:

| Field              | Bits | Notes                      |
| ------------------ | ---- | -------------------------- |
| Exponent Magnitude |   5+ |                            |
| Exponent Sign      |    1 | 0 = positive, 1 = negative |
| Significand Sign   |    1 | 0 = positive, 1 = negative |

`exponent magnitude` is an absolute integer value representing (10 ^ exponent). The `exponent sign` determines whether the exponent is multiplied by `1` or `-1`. Similarly, `significand sign` determines the sign of the [significand](#significand-structure).


### Significand Structure

The `significand magnitude` gets multiplied by the `significand sign` and finished exponent to produce the final value.

| Field                 | Bits |
| --------------------- | ---- |
| Significand Magnitude |   7+ |

Significand values larger than 127 follow standard ULEB128 encoding rules (for example, 128 is encoded as `[80 01]`).


**Example**: Value 0.1

 * Exponent: `1` (exponent = 1)
 * Exp sign: `1` (exponent sign = negative)
 * Sig sign: `0` (significand sign = positive)
 * Significand: `1` (significand = 1)
 * Exponent struct: `0000110`
 * Significand struct: `0000001`
 * All bits, with ULEB128 encoding: `0 0000110`, `0 0000001` = `[06 01]`

**Example**: Value 1.0e+10000

 * Exponent: `10011100010000` (exponent = 10000)
 * Exp sign: `0` (exponent sign = positive)
 * Sig sign: `0` (significand sign = positive)
 * Significand: `1` (significand = 1)
 * Exponent struct: `1001110001000000`
 * Significand struct: `0000001`
 * All bits, with ULEB128 encoding: `1 1000000 1 0111000 0 0000010`, `0 0000001` = `[c0 b8 02 01]`

**Example**: Value -1.94618882e-200

 * Exponent: `11010000` (exponent = 208 due to the extra 8 digits required to shift significand from 194618882 to 1.94618882)
 * Exp sign: `1` (exponent sign = negative)
 * Sig sign: `1` (significand sign = negative)
 * Significand: `1011100110011010011000000010` (significand = 194618882)
 * Exponent struct: `00001101000011`
 * Significand struct: `1011100110011010011000000010`
 * All bits, with ULEB128 encoding: `1 1000011 0 0000110`, `1 0000010 1 1001100 1 1100110 0 1011100` = `[c3 06 82 cc e6 5c]`



Special Values
--------------

Special values are special 1 or 2 byte sequences that **MUST** be checked against before attempting to decode as a general case [exponent structure](#exponent-structure).


### Zero

Zero values (±0) are encoded as follows:

    00000010 = [02] = +0
    00000011 = [03] = -0

(Interpreted as an expont structure, this would correspond to an exponent value of `-0`, which is an otherwise impossible value)


### Infinity

Infinity is encoded as follows:

    10000010 00000000 = [82 00] = +infinity
    10000011 00000000 = [83 00] = -infinity

These values would normally decode as ULEB128 to values `2` and `3`, the only difference being that they are artificially extended by 1 group. Decoders **MUST** therefore check for the sequences `82 00` and `83 00` before moving on to the general encoding.


### NaN

NaN (not-a-number) is encoded as follows:

    10000000 00000000 = [80 00] = Quiet NaN
    10000001 00000000 = [81 00] = Signaling NaN

These values would normally decode as ULEB128 to values `0` and `1`, the only difference being that they are artificially extended by 1 group. Decoders **MUST** therefore check for the sequences `80 00` and `81 00` before moving on to the general encoding.



Rounding
--------

When rounding for storage in compact float format, values **MUST** default to rounding using IEEE754's **round half to even** method (also known as banker's rounding), unless all sending and receiving parties have agreed to another method.


**Example**: Binary float value ~ 0.5083299875259399, rounded to 4 significant digits

Binary floats are rarely directly representable in base 10. If we took the value 0.5083299875259399 as-is, we would be encoding many digits of false precision. For the purposes of this example we assume that, after auditing our measurement accuracy and precision requirements, we settle upon 4 significant digits, representing the value as 0.5083.

 * Significand: 5083
 * Exponent: -4

Exponent field contents:

    Exponent magnitude: 100
    Exponent sign:          1
    Significand sign:         0
    Total:              100 1 0

Build the exponent ULEB128:

    Value: 00010010
    Hex:   0x12

Significand field contents:

    Value:       5083 (0x13db)
    Binary:      0001 0011 1101 1011
    Groups of 7: 00 0100111 1011011
    As ULEB128:  11011011 00100111
    Hex:         0xdb     0x27

Result: `[12 db 27]`



Smallest Possible Size
----------------------

As compact float values are not normalized, the same value can be represented in many ways. An encoder **MUST** store values in as few bytes as possible without losing data beyond normal rounding.

For example, the value 4.09104981 rounded to 5 significant digits is 4.0910, which could be naively encoded as follows:

    Significand: 40910
    Exponent: -4
    Effective value: 4.091

Encoding 5 significant digits would require 4 bytes. However, since the last digit is 0, it can be removed from the encoding while still resulting in the same effective value:

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



License
-------

Copyright (C) Karl Stenerud

Specification released under Creative Commons Attribution 4.0 International Public License https://creativecommons.org/licenses/by/4.0/
