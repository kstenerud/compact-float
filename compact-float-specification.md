Compact Float Format
====================

Compact float format is an encoding scheme to store a decimal floating point value in as few bytes as possible for data transmission.

Compact float can store all of the kinds of values that the ieee754 decimal types can, without data loss:
* ±0
* ±infinity
* Signaling and quiet NaNs, including payload



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


### Significand RVLQ

| Field                 | Bits |
| --------------------- | ---- |
| Significand Magnitude |   7+ |

The `significand magnitude` is an absolute integer value that will be multiplied by the `exponent`. The `significand sign` determines whether the significand is multiplied by `1` or `-1`.



Special Values
--------------

Special values are encoded entirely into the exponent RVLQ, and have no significand RVLQ structure.


#### Extended Exponent RVLQ

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
| Significand Sign |    1 | 0 = positive, 1 = negative                                |

The `NaN payload` field contains the bit pattern of the original ieee754 NaN value, signaling bit omitted, right-justified, with leading zero bits omitted. An implementation must properly convert the `signaling bit` to/from the underlying encoding of the platform it's running on.



Examples
--------

### Float value 0.5083

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

Given a 32-bit ieee754 decimal float with raw value: `0xfe008410`

* Bits: `1111 1110 0000 0000 1000 0100 0001 0000`
* Bit 31 `1` indicates a negative sign bit.
* Bits 26-30 `11111` mark a NaN value.
* Bit 25 `1` indicates that this is a signaling NaN.
* Bits 0-24 are the NaN payload. With leading zero bits omitted: `0x8410`

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
