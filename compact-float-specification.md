Compact Float Format
====================

Compact float format is an encoding scheme to store a decimal floating point value in as few bytes as possible for data transmission.

Compact float can store all of the kinds of values that the IEEE754 decimal types can, without data loss:
* ±0
* ±infinity
* Signaling and quiet NaNs, including payload



Contents
--------

* [Encoded Structure](#encoded-structure)
  - [Exponent RVLQ](#exponent-rvlq)
  - [Significand RVLQ](#significand-rvlq)
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

A compact float value is encoded into one or two [RVLQ](https://github.com/kstenerud/vlq/blob/master/vlq-specification.md) structures, depending on the value being stored:

#### Normal Value Encoding:

    [Exponent RVLQ] [Significand RVLQ]

#### [Special Value](#special-values) Encoding:

    [Exponent RVLQ]



### Exponent RVLQ

The exponent RVLQ is a bit field containing the significand sign, exponent sign, and exponent magnitude:

| Field              | Bits | Notes                      |
| ------------------ | ---- | -------------------------- |
| Exponent Magnitude |   5+ | exponent high bits         |
| Exponent Sign      |    1 | 0 = positive, 1 = negative |
| Significand Sign   |    1 | 0 = positive, 1 = negative |

`exponent magnitude` is an absolute integer value representing (10 ^ exponent). The `exponent sign` determines whether the exponent is multiplied by `1` or `-1`.

#### Extended Exponent RVLQ

Some special values are signaled by an extended exponent RVLQ, which is an RVLQ that is encoded into one more group than is necessary to hold the exponent value.

Examples:

| Normal RVLQ | Extended RVLQ |
| ----------- | ------------- |
| `[00]`      | `[80 00]`     |
| `[7c]`      | `[80 7c]`     |
| `[9f 40]`   | `[80 9f 40]`  |


### Significand RVLQ

| Field                 | Bits |
| --------------------- | ---- |
| Significand Magnitude |   7+ |

The `significand magnitude` is an absolute integer value that will be multiplied by the `significand sign` and the `exponent`.



Special Values
--------------

Special values are encoded entirely into the exponent RVLQ, and have no significand RVLQ structure.


### Zero

Zero values (±0) are encoded using an exponent value of `-0`, which is an otherwise impossible value. The sign is determined by the significand sign field:

    00000010 = [02] = +0
    00000011 = [03] = -0


### Infinity

Infinity is encoded using an [extended exponent RVLQ](#extended-exponent-rvlq), encoding the exponent value `-0`. The sign is determined by the significand sign field.

    10000000 00000010 = [80 02] = +infinity
    10000000 00000011 = [80 03] = -infinity


### NaN

NaN (not-a-number) values are encoded using a modified [extended exponent RVLQ](#extended-exponent-rvlq) with the `exponent sign` fixed at `0`. The modified "exponent" field is encoded as follows:

| Field            | Size | Notes                                                     |
| ---------------- | ---- | --------------------------------------------------------- |
| NaN Payload      |   4+ | Right justified, leading zeroes and signaling bit omitted |
| Signaling Bit    |    1 | 0 = quiet, 1 = signaling                                  |
| 0                |    1 | Always 0                                                  |
| Significand Sign |    1 | Copied from IEEE754 float NaN value                       |

The `NaN payload` field contains the bit pattern of the original IEEE754 NaN value, signaling bit omitted, right-justified, with leading zero bits omitted.

**Note:** An implementation must properly convert the `signaling bit` to/from the underlying signaling bit encoding of the platform it's running on.



Rounding
--------

Compact float values must be rounded using IEEE754's recommended **round half to even** method, unless both sending and receiving parties have agreed to another method.



Smallest Possible Size
----------------------

As compact float values are not normalized, the same value can be represented in many ways. An encoder must store values in as few bytes as possible without losing data.

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

* With 6 significant digits, you'd have a margin of error of 1mm for a measure of 1km, for example.
* The [International Council for Science](https://physics.nist.gov/cuu/Constants/index.html) accepted physical constants are mostly between 9 and 12 significant digits.
* NASA's International Space Station Guidance Navigation and Control System (GNC) uses 15 significant digits.
* The Space Integrated Global Positioning System/Inertial Navigation System (SIGI) uses 16 significant digits.
* The fundamental constants of the universe are useful to 32 significant digits.
* 40 significant digits would allow measuring the entire universe with a margin of error less than the width of a hydrogen atom.

For most real-world measures, you'll rarely need more than 4 significant digits outside of financial and scientific data.

Storing more significant digits takes up more space. Here's a breakdown of the encoded compact float size in bytes for significant digits up to 16 (assuming 1 byte of exponent data, which gives a max/min exponent of ±31):

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

Binary floats are rarely directly representable in base 10. If we took the value 0.5083299875259399 as-is, we would be encoding many digits of false precision. For the purposes of this example, assume that after auditing our source precision and requirements, we settle upon 4 significant digits, representing the value as 0.5083.

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

    Value:       5083 (13db)
    Binary:      0001 0011 1101 1011
    Groups of 7: 00 0100111 1011011
    As RVLQ:     10100111 01011011
    Hex:         0xa7     0x5b

Result: `[12 a7 5b]`


### NaN

Given a 32-bit IEEE754 decimal float with a raw value of `0xfe008410`:

* Bits: `1111 1110 0000 0000 1000 0100 0001 0000`
* Bit 31 `1` indicates a negative sign bit.
* Bits 26-30 `11111` mark a NaN value.
* Bit 25 `1` indicates that this is a signaling NaN.
* Bits 0-24 form the NaN payload. After omitting leading zero bits, the payload is `0x8410`

Exponent field contents:

    Payload:   0x8410
    Bits:      1000 0100 0001 0000
    Signaling:                     1
    0:                               0
    Sign:                              1
    Total:     1000 0100 0001 0000 1 0 1

Build the exponent RVLQ:

    Value:       1000010000010000101
    Groups of 7: 10000 1000001 0000101
    As RVLQ:     10010000 11000001 00000101
    Hex:         0x90     0xc1     0x05

Result: `[90 c1 05]`



License
-------

Copyright 2019 Karl Stenerud

Specification released under Creative Commons Attribution 4.0 International Public License https://creativecommons.org/licenses/by/4.0/
