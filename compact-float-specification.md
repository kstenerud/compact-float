Compact Float Format
====================

Compact float format (CFF) is an encoding scheme to store a floating point value in as few bytes as possible for data transmission. CFF supports all values that can be stored in any sized ieee754 decimal and binary floating point encoding.

The general conceptual form of a floating point number is:

    value = sign * significand * base ^ exponent

Where `base` is `2` for binary floating point numbers and `10` for decimal floating point numbers.

Note: CFF does not store meta data about what kind of floating point value is contained within (decimal or binary).



Encoded Structure
-----------------

A floating point value is encoded into one or two [VLQ](https://github.com/kstenerud/vlq/blob/master/vlq-specification.md) structures, depending on the value being stored:

    [Exponent VLQ]

or:

    [Exponent VLQ] [Significand VLQ]


### Exponent VLQ

The exponent VLQ is a bit field containing the significand sign, exponent sign, and exponent magnitude:

| Field              | Bits | Notes                      |
| ------------------ | ---- | -------------------------- |
| Significand Sign   |    1 | 0 = positive, 1 = negative |
| Exponent Sign      |    1 | 0 = positive, 1 = negative |
| Exponent Magnitude |   5+ | Stored as unsigned integer |

`exponent magnitude` is an unsigned integer value (no bias). The `exponent sign` determines the sign of the exponent.

The normally invalid exponent value `-0` is used to encode a [zero value](#zero-value).

The exponent VLQ is encoded in big endian order.

#### Extended Exponent VLQ

An extended VLQ is a VLQ that is encoded into one more group than is necessary to hold the value. So for example (assuming both sign bits are 0):

| Exponent Magnitude | Normal    | Extended     |
| ------------------ | --------- | ------------ |
|                  0 | `[00]`    | `[80 00]`    |
|                 37 | `[25]`    | `[80 25]`    |
|               1000 | `[87 68]` | `[80 87 68]` |

When an extended exponent VLQ is encountered, it signifies a [special value](#special-values).


### Significand VLQ

| Field                 | Bits |
| --------------------- | ---- |
| Significand Magnitude |   7+ |

The significand encoding and endianness depends on whether a binary or a decimal floating point value is being encoded:

#### Binary Floating Point Significand

The significand is encoded as a fraction with an implied leading `1` bit in the same manner as binary ieee754 significands. Since the value is left-justified, it is encoded in little endian order with trailing zero bits omitted.

#### Decimal Floating Point Significand

The significand is encoded as a plain integer in the same manner as ieee754 binary integer decimal significands. Since the value is right-justified, it is encoded in big endian order with leading zero bits omitted.

#### Significand VLQ Endianness

Because of how binary and decimal significands are encoded, they have different optimal endianness.

**Optimal endianness** means the endianness that allows progressive building of the decoded value without requiring additional state (i.e. each new decoded group can be applied to the accumulator using a shift by 7 followed by a logical OR, regardless of the accumulator's value at the previous step).

For values that "grow" leftward (e.g. 123456):

    accumulator = (accumulator << 7) | next_7_bits

For values that "grow" rightward (e.g. normalized 1.23456):

    accumulator = (accumulator >> 7) | (next_7_bits << (sizeof(accumulator)*8 - 7))

Binary floats are represented using a normalized fractional significand, which "grows" rightward. Decimal floats are represented using a whole number significand, which "grows" leftward. Since they grow in opposite directions, they have different optimal endianness:

| Float Type | Example (4.25) | Normalized | Bit Positioning | Grows     | Optimal Endianness |
| ---------- | -------------- | ---------- | --------------- | --------- | ------------------ |
| Binary     | 1.0001 x 2^2   |     Yes    | `abcdefgh----`  | Rightward | Little Endian      |
| Decimal    | 425 x 10^-2    |     No     | `----abcdefgh`  | Leftward  | Big Endian         |

Fields that grow rightward are best encoded as little endian, and fields that grow leftward are best encoded as big endian.



Normal Numbers
--------------

Normal numbers operate on the same principle as in ieee754: A signed exponent, and a signed significand.

As in ieee754, binary floating point values are left-justified and have an implied leading `1` bit. Decimal floating point values are right-justified and have no implied bits. Binary floating point values must always be normalized, even if they are being encoded from subnormal values. Decimal floating point values are never normalized.

### Zero Value

As a special case, an exponent value of `-0` denotes a zero (±0) significand. In this case, there is no significand VLQ, and the final value's sign is determined by the significand sign field:

    00100000 = [20] = +0
    01100000 = [60] = -0



Special Values
--------------

Special values are signaled by an [extended exponent VLQ](#extended-exponent-vlq). The following special values are possible:


### Infinity

Infinity is encoded using an extended exponent VLQ, encoding the exponent value `0`. There is no significand VLQ, and sign is determined by the significand sign field.

    10000000 00000000 = [80 00] = +infinity
    10000000 01000000 = [80 40] = -infinity


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

### Binary float value 0.2539978

    32-bit ieee754 binary: 0x3e820c00 (0 01111101 00000100000110000000000)
    Sign bit: 0
    Exponent: 0x7d (125)

Convert the exponent to a signed int and subtract the bias:

    exponent = signed(125) - 127 = -2

Strip trailing zero bits from the significand:

    Original: 00000100000110000000000
    Stripped: 0000010000011

Build exponent VLQ:

    Significand sign: 0
    Exponent sign: 1
    Exponent magnitude: 2 (min length = 5, so 00010)
    Result: 0 1 00010
    As a big endian VLQ: 00100010 = [22]

Build significand VLQ:

    Significand: 0000010000011
    Split into 7 bit groups (left justified): 0000010 000011 (0)
    As a little endian VLQ: 10000110 00000010 = [86 02]

Result: `[22 86 02]`


### Binary float value -2.03703597633448608627e+90

    64 bit ieee754 binary: 0xd2b0000000000000 (1 10100101011 00000000...)
    Sign bit: 1
    Exponent: 0x52d (1325)

Convert the exponent to a signed int and subtract the bias:

    exponent = signed(1325) - 1023 = 302 (0x12e)

Strip trailing zero bytes from the significand:

    Original: 0000 00000000 00000000 00000000 00000000 00000000 00000000
    Stripped: [empty]

Build exponent VLQ:

    Significand sign: 1
    Exponent sign: 0
    Exponent magnitude: 302
    Result: 1 0 100101110
    Split into 7 bit groups (left justified): 1010010 1110 (000)
    As a big endian VLQ: 11010010 01110000 = [d2 70]

Build significand VLQ:

    Significand: [empty]
    As a little endian VLQ: 00000000 [00]

Result: `[d2 70 00]`


### Decimal float value 1.43

    32 bit ieee754 BID: 0x3180008f (0 01 100011 00000000000000010001111)
    Sign bit: 0
    Exponent : 01 100011 (099) - 101 bias = -2
    Significand: 00000000000000010001111 (143)

Build exponent VLQ:

    Significand sign: 0
    Exponent sign: 1
    Exponent magnitude: 2
    Result: 0 1 00010
    Split into 7 bit groups (right justified): 0100010
    As a big endian VLQ: 00100010 = [22]

Build significand VLQ:

    Significand: 10001111
    As a big endian VLQ: 10000001 00001111 [81 0f]

Result: `[22 81 0f]`



License
-------

Copyright 2019 Karl Stenerud

Specification released under Creative Commons Attribution 4.0 International Public License https://creativecommons.org/licenses/by/4.0/
