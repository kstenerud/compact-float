Compact Float Format
====================

Compact float format is an encoding scheme to store a decimal floating point value in as few bytes as possible for data transmission. It can store the same regular and special values that [IEEE754 decimal types](https://en.wikipedia.org/wiki/IEEE_754) can, with unlimite range.

**Features**:

 * Unlimited significand and exponent sizes
 * ±0
 * ±infinity
 * Signaling and quiet NaNs



Contents
--------

- [Compact Float Format](#compact-float-format)
  - [Contents](#contents)
  - [Special Value Encoding](#special-value-encoding)
    - [Zero](#zero)
    - [Infinity](#infinity)
    - [NaN](#nan)
  - [Normal Value Encoding](#normal-value-encoding)
    - [Exponent and Signs](#exponent-and-signs)
    - [Significand](#significand)
    - [Examples](#examples)
  - [Rounding](#rounding)
  - [Smallest Possible Size](#smallest-possible-size)
  - [How much precision do you need?](#how-much-precision-do-you-need)
  - [License](#license)



Special Value Encoding
----------------------

Special values are 1 or 2 byte sequences that **MUST** be checked against first before attempting to decode as a [normal value](#normal-value-encoding).


### Zero

Zero values (±0) are encoded as follows:

    00000010 = [02] = +0
    00000011 = [03] = -0

Interpreted as an [expont structure](#exponent-and-signs), this would normally correspond to an exponent value of `-0`, which is an impossible value.


### Infinity

Infinity is encoded as follows:

    10000010 00000000 = [82 00] = +infinity
    10000011 00000000 = [83 00] = -infinity

These [ULEB128](https://en.wikipedia.org/wiki/LEB128)-encoded values would normally decode to an exponent value of `-0` (like for [zero](#zero)), the only difference being that they are artificially extended by 1 group.


### NaN

NaN (not-a-number) is encoded as follows:

    10000000 00000000 = [80 00] = Quiet NaN
    10000001 00000000 = [81 00] = Signaling NaN

These [ULEB128](https://en.wikipedia.org/wiki/LEB128)-encoded values would normally decode to an exponent value of `0`, the only difference being that they are artificially extended by 1 group.



Normal Value Encoding
---------------------

The general conceptual form of a floating point number is:

    value = significand × baseᵉˣᵖᵒⁿᵉⁿᵗ

Where significand = (significand magnitude × significand sign), and exponent = (exponent magnitude × exponent sign).

Normal values are stored in two adjacent [ULEB128](https://en.wikipedia.org/wiki/LEB128)-encoded structures:

    [Exponent and signs] [Significand]


### Exponent and Signs

This structure is a [ULEB128](https://en.wikipedia.org/wiki/LEB128)-encoded bit field containing the exponent portions (sign and magnitude), as well as the sign of the significand:

| Field              | Bits | Notes                               |
| ------------------ | ---- | ----------------------------------- |
| Exponent Magnitude |   5+ |                                     |
| Exponent Sign      |    1 | 0 = positive (1), 1 = negative (-1) |
| Significand Sign   |    1 | 0 = positive (1), 1 = negative (-1) |

`exponent magnitude` is an absolute integer value that gets multiplied by the `exponent sign` to produce an exponent representing 10ᵉˣᵖᵒⁿᵉⁿᵗ. `significand sign` determines the sign of the [significand](#significand).


### Significand

The `significand magnitude` is an absolute integer value encoded as a [ULEB128](https://en.wikipedia.org/wiki/LEB128). It gets multiplied by the `significand sign` and 10ᵉˣᵖᵒⁿᵉⁿᵗ to produce the final value.

| Field                 | Bits |
| --------------------- | ---- |
| Significand Magnitude |   7+ |

Significand values larger than 127 follow standard ULEB128 encoding rules (for example, 128 is encoded as `[80 01]`).


### Examples

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



Rounding
--------

When rounding for storage in compact float format, values **MUST** default to rounding using IEEE754's **round half to even** method (also known as banker's rounding), unless all sending and receiving parties have agreed to another method.

**Example**: Binary float value ~ 0.5083299875259399, rounded to 4 significant digits

Binary floats are rarely directly representable in base 10, and usually the conversion results in many more, possibly even infinite significant digits. If we took the value 0.5083299875259399 as-is, we would be encoding many digits of [false precision](https://en.wikipedia.org/wiki/False_precision). For the purposes of this example we assume that, after auditing our measurement accuracy and precision requirements, we settle upon 4 significant digits (thus our effective value would be 0.5083, or 5083 × 10⁻⁴ for the purposes of storing in a compact float).

 * Significand: 5083
 * Exponent: -4

Exponent structure contents (in binary):

    Exponent magnitude: 100
    Exponent sign:          1
    Significand sign:         0
    ---------------------------
    Complete structure: 100 1 0

Binary 00010010 = hex 0x12, which is less than 0x80 (for the purposes of [ULEB128](https://en.wikipedia.org/wiki/LEB128) encoding), so we are done encoding the exponent structure.

Significand contents:

    Value:       5083
    Hex:         13db
    Binary:      0001 0011 1101 1011
    Groups of 7: 00 0100111 1011011
    As ULEB128:  11011011 00100111
    --------------------------------
    Hex:         0xdb     0x27

Result: `[12 db 27]`



Smallest Possible Size
----------------------

As compact float values are not normalized, the same value can be represented in many ways. An encoder **MUST** store values in as few bytes as possible without losing data beyond normal rounding.

For example, the value 4.09104981 rounded to 5 significant digits is 4.0910, which could be naively encoded as follows:

    Significand: 40910
    Exponent: -4
    Effective value: 4.091

Encoding 5 significant digits would require 4 bytes. However, since the last digit is 0, it can be removed from the encoding while still resulting in the same effective value by adjusting the exponent:

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

IEEE754 floating point format stores 7 significant digits for 32-bit values, 16 significant digits for 64-bit values, 34 significant digits for 128-bit values, and a whopping 71 significant digits for 256-bit values!

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

Copyright (C) 2019 Karl Stenerud

Specification released under Creative Commons Attribution 4.0 International Public License https://creativecommons.org/licenses/by/4.0/
