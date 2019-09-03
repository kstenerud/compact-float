Compact Float Format
====================

Compact float format (CFF) is an encoding scheme to store a decimal floating point value in as few bytes as possible for data transmission.

CFF can store all of the kinds of values that the ieee754 decimal types can, without data loss:
* ±0
* ±infinity
* Signaling and quiet NaNs, including payload



Encoded Structure
-----------------

The general conceptual form of a floating point number is:

    value = sign * significand * 10 ^ (exponent * exponent_sign)

A compact float value is encoded into one or two [RVLQ](https://github.com/kstenerud/vlq/blob/master/vlq-specification.md) structures, depending on the value being stored:

Normal Values:

    [Exponent RVLQ] [Significand RVLQ]

[Special Values](#special-values):

    [Exponent RVLQ]



### Exponent RVLQ

The exponent RVLQ is a bit field containing the significand sign, exponent sign, and exponent magnitude:

| Field              | Bits | Notes                      |
| ------------------ | ---- | -------------------------- |
| Exponent Magnitude |   5+ | exponent high bits         |
| Exponent Sign      |    1 | 0 = positive, 1 = negative |
| Significand Sign   |    1 | 0 = positive, 1 = negative |

`exponent magnitude` is an absolute integer value representing (10 ^ exponent). The `exponent sign` determines whether the exponent is multiplied by `1` or `-1`.


### Significand RVLQ

| Field                 | Bits |
| --------------------- | ---- |
| Significand Magnitude |   7+ |

The `significand magnitude` is an absolute integer value that will be multiplied by the `exponent`. The `significand sign` determines whether the significand is multiplied by `1` or `-1`.



Special Values
--------------

Special values are encoded entirely into the exponent RVLQ, and have no significand RVLQ structure.


#### Extended Exponent

Some special values are signaled by an extended exponent RVLQ, which is an RVLQ that is encoded into one more group than is necessary to hold the exponent value.

Examples:

| Normal RVLQ | Extended RVLQ |
| ----------- | ------------- |
| `[00]`      | `[80 00]`     |
| `[7c]`      | `[80 7c]`     |
| `[9f 40]`   | `[80 9f 40]`  |


### Zero

Zero values (±0) are encoded using an exponent value of `-0`, which is an otherwise impossible value. The sign is determined by the significand sign field:

    00000010 = [02] = +0
    00000011 = [03] = -0


### Infinity

Infinity is encoded using an [extended exponent RVLQ](#extended-exponent), encoding the exponent value `-0`. The sign is determined by the significand sign field.

    10000000 00000010 = [80 02] = +infinity
    10000000 00000011 = [80 03] = -infinity


### NaN

NaN values are encoded using a modified [extended exponent RVLQ](#extended-exponent) with the `exponent sign` fixed at `0`. The modified "exponent" field is encoded as follows:

| Field            | Size | Notes                                                     |
| ---------------- | ---- | --------------------------------------------------------- |
| NaN Payload      |   4+ | Right justified, leading zeroes and signaling bit omitted |
| Signaling Bit    |    1 | 0 = quiet, 1 = signaling                                  |
| 0                |    1 | Always 0                                                  |
| Significand Sign |    1 | 0 = positive, 1 = negative                                |

The `NaN payload` field contains the bit pattern of the original ieee754 NaN value, signaling bit omitted, right-justified, with leading zero bits omitted. An implementation must properly convert the `signaling bit` to/from the underlying encoding of the platform it's running on.



Examples
--------

TODO: Rebuild these. They're in the old format!

#### Example NaN

    10000000 00100000 10001010 00010100 = [80 20 c9 22]
    = signaling NaN with a (right-justified) payload of 10010100010


### Binary float value 0.50830078125

    32-bit ieee754 binary: 0x3f022000 (0 01111110 00000100010000000000000)
    Sign bit: 0
    Exponent: 0x7e (126)

Convert the exponent to a signed int and subtract the bias:

    exponent = signed(126) - 127 = -1

Build exponent RVLQ:

    Exponent magnitude: 1 (min length = 5, so 00001)
    Exponent sign: 1
    Significand sign: 0
    Result: 00001 1 0
    As a RVLQ: 0000110 = [06]

Strip trailing zero bits from the significand:

    Original: 00000100010000000000000
    Stripped: 0000010001

Build significand LVLQ:

    Significand: 0000010001
    Split into 7 bit groups (left justified): 0000010 001(0000)
    As a LVLQ (little endian): 10010000 00000010 = [90 02]

Result: `[06 90 02]`


### Binary float value -2.03703597633448608627e+90

    64 bit ieee754 binary: 0xd2b0000000000000 (1 10100101011 00000000...)
    Sign bit: 1
    Exponent: 0x52b (1323)

Convert the exponent to a signed int and subtract the bias:

    exponent = signed(1323) - 1023 = 300 (0x12c)

Build exponent RVLQ:

    Exponent magnitude: 300 (100101100)
    Exponent sign: 0
    Significand sign: 1
    Result: 100101100 0 1
    Split into 7 bit groups (left justified): 1001 0110001
    As a RVLQ: 10001001 00110001 = [81 31]

Strip trailing zero bytes from the significand:

    Original: 0000 00000000 00000000 00000000 00000000 00000000 00000000
    Stripped: [empty]

Build significand LVLQ:

    Significand: [empty]
    As a little endian VLQ: 00000000 [00]

Result: `[88 4b 00]`


### Decimal float value 1.43

    32 bit ieee754 BID: 0x3180008f (0 01 100011 00000000000000010001111)
    Sign bit: 0
    Exponent : 01 100011 (099) - 101 bias = -2
    Significand: 00000000000000010001111 (143)

Build exponent RVLQ:

    Significand sign: 0
    Exponent sign: 1
    Exponent magnitude: 2
    Result: 00010 1 0
    Split into 7 bit groups (right justified): 0001010
    As a RVLQ: 00001010 = [0a]

Build significand RVLQ:

    Significand: 10001111
    Split into 7 bit groups (right justified): 1 0001111
    As a RVLQ: 10000001 00001111 [81 0f]

Result: `[0a 81 0f]`



License
-------

Copyright 2019 Karl Stenerud

Specification released under Creative Commons Attribution 4.0 International Public License https://creativecommons.org/licenses/by/4.0/
