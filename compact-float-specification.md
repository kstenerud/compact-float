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

Note: CFF does not store meta data about what kind of floating point value is contained within (decimal or binary).



Encoded Structure
-----------------

The general conceptual form of a floating point number is:

    value = sign * significand * base ^ exponent

Where `base` is `2` for binary floating point numbers and `10` for decimal floating point numbers.

A CFF value is encoded into one or two [VLQ](https://github.com/kstenerud/vlq/blob/master/vlq-specification.md) structures, depending on the value being stored:

    [Exponent RVLQ]

or:

    [Exponent RVLQ] [Significand RVLQ or LVLQ]


### Exponent RVLQ

The exponent is a bit field containing the significand sign, exponent sign, and exponent magnitude:

| Field              | Bits | Notes                      |
| ------------------ | ---- | -------------------------- |
| Exponent Magnitude |   5+ | Stored as unsigned integer |
| Exponent Sign      |    1 | 0 = positive, 1 = negative |
| Significand Sign   |    1 | 0 = positive, 1 = negative |

`exponent magnitude` is an unsigned integer value (no bias). The `exponent sign` determines the sign of the exponent.

The normally invalid exponent value `-0` is used to encode a [zero value](#zero-value).

The exponent is stored as a [RVLQ](https://github.com/kstenerud/vlq/blob/master/vlq-specification.md).

#### Extended Exponent VLQ

An extended VLQ is a VLQ that is encoded into one more group than is necessary to hold the value. So for example (assuming both sign bits are 0):

| Exponent Magnitude | Normal    | Extended     |
| ------------------ | --------- | ------------ |
|                  0 | `[00]`    | `[80 00]`    |
|                 37 | `[25]`    | `[80 25]`    |
|               1000 | `[87 68]` | `[80 87 68]` |

When an extended exponent RVLQ is encountered, it signifies a [special value](#special-values).


### Significand VLQ

| Field                 | Bits |
| --------------------- | ---- |
| Significand Magnitude |   7+ |

#### Significand VLQ Type

The significand VLQ type depends on whether a binary or a decimal floating point value is being encoded.

Binary floating point significands are stored as a fraction with an implied leading `1` bit in the same manner as binary ieee754 significands. The value "grows" rightward as more significand digits are needed.

Decima floating point significands are stored as a plain integer in the same manner as ieee754 binary integer decimal significands. The value "grows" leftward as more significant digits are needed.

Since they grow in opposite directions, they are stored as different VLQ types:

| Float Type | Example (4.25) | Normalized | Bit Orientation | VLQ Type |
| ---------- | -------------- | ---------- | --------------- | -------- |
| Binary     | 1.0001 x 2^2   |     Yes    | `abcdefgh----`  | LVLQ     |
| Decimal    | 425 x 10^-2    |     No     | `----abcdefgh`  | RVLQ     |



Normal Numbers
--------------

Normal numbers operate on the same principle as in ieee754: A signed exponent, and a signed significand.

As in ieee754, binary floating point values are left-justified and have an implied leading `1` bit. Decimal floating point values are right-justified and have no implied bits. Binary floating point values must always be normalized, even if they are being encoded from subnormal values. Decimal floating point values are never normalized.

### Zero Value

As a special case, an exponent value of `-0` denotes a zero (±0) significand. In this case, there is no significand VLQ, and the final value's sign is determined by the significand sign field:

    00100000 = [02] = +0
    01100000 = [03] = -0



Special Values
--------------

Special values are signaled by an [extended exponent VLQ](#extended-exponent-vlq). The following special values are possible:


### Infinity

Infinity is encoded using an extended exponent VLQ, encoding the exponent value `0`. There is no significand VLQ, and sign is determined by the significand sign field.

    10000000 00000000 = [80 00] = +infinity
    10000000 01000000 = [80 01] = -infinity


### NaN

NaN values are encoded using an extended exponent VLQ, encoding the exponent value `-0`. The significand VLQ encodes a `signaling bit` and a `NaN payload`, in big endian order.

| Field         | Size | Notes           |
| ------------- | ---- | --------------- |
| Signaling Bit |    1 | `1` = signaling |
| NaN Payload   |   6+ | Right justified |

#### Signaling Bit

The ieee754 spec does not actually require a specific encoding of the first bit of the NaN payload to differentiate signaling from non-signaling binary float NaN values (it says **should** rather than **must**), and the recommended values are the opposite of the signaling bit for decimal floats. CFF harmonizes this for all NaN values:

| Value | Meaning       |
| ----- | ------------- |
|   0   | Quiet NaN     |
|   1   | Signaling NaN |

An implementation must properly convert to/from the underlying encoding of the platform it's running on.

#### NaN Payload

The `NaN payload` field contains the bit pattern after the signaling bit from the original ieee754 value, right-justified, with leading zero bits omitted.

#### Example

    10000000 00100000 10001010 00010100 = [80 20 c9 22]
    = signaling NaN with a (right-justified) payload of 10010100010

Any encoded value starting with `[80 20]` is a NaN value.


### Subnormal Numbers

Binary floating point values require special encoding for subnormal numbers. Decimal floating point values support unnormalized numbers directly, and don't require special encoding.

When encoding a binary float subnormal number, it must first be normalized, and then stored in the same manner as a normal number, except that it must be encoded using an [extended exponent VLQ](#extended-exponent-vlq). When decoding, the original ieee754 float size of the subnormal can be inferred from the exponent value.



Examples
--------

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
