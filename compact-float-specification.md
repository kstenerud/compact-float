Compact Float Format
====================

Compact float format (CFF) is an encoding scheme to store a floating point value in as few bytes as possible for data transmission. This format supports all values that can be stored in ieee754 decimal and binary floating point values.



Encoded Structure
-----------------

The encoded floating point structure contains an 8-bit header, followed by a possible exponent, and a possible significand:

| Field       | Bytes |
| ----------- | ----- |
| Header      |     1 |
| Exponent    |   0-3 |
| Significand |  0-30 |

The general conceptual form of a floating point number is:

    value = sign * significand * base ^ (signed exponent)

Where the base is `2` for binary floating point numbers and `10` for decimal floating point numbers.

Note: The encoded floating point structure does not contain information about whether it's a binary or decimal floating point value.


### Header Fields

| Field               | Bits  | Order | Notes                                |
| ------------------- | ----- | ----- | ------------------------------------ |
| Sign                |     1 | High  | 0 = positive, 1 = negative           |
| Special Value       |     1 |       | 0 = normal number, 1 = special value |
| Exponent Size       |     2 |       |                                      |
| Significand Size    |     4 | Low   | Value 15 = significand size 30       |

#### Sign Field

Determines whether the number is positive or negative: 0 = positive, 1 = negative.

#### Special Value Field

Determines whether this is a normal number or a special value: 0 = normal, 1 = special.

See [Normal Numbers](#normal-numbers) and [Special Values](#special-values)

#### Exponent Size Field

Determines the size of the exponent payload in bytes.

#### Significand Size Field

Determines the size of the significand payload in bytes.

As a special case, if `significand size` is 15, the actual significand payload size is 30 (to support 256 bit float).


### Exponent Payload

Contains the exponent value, encoded as a little endian signed 2's complement integer (without bias).

### Significand Payload

Contains the significand value, encoding the same bit patterns as the original ieee754 value, stored in little endian byte order. The value is left-justified (meaning that all low order zero bytes have been omitted).


### Normal Numbers

Normal numbers operate on the same principle as in ieee754: A sign, an exponent, and a significand with an implied leading `1` bit. The only difference is that compact float encodes the exponent as a 2's complement integer with no bias applied.


### Special Values

When the `normal number` field is 0, the other fields encode different meanings:

| Significand Size | Exponent Size | Sign | Data Type | Value     | Notes                              |
| ---------------- | ------------- | ---- | --------- | --------- | ---------------------------------- |
|                0 |             0 |    0 | normal    |        +0 | No exponent or significand payload |
|                0 |             0 |    1 | normal    |        -0 | No exponent or significand payload |
|                0 |             1 |    0 | normal    | +infinity | No exponent or significand payload |
|                0 |             1 |    1 | normal    | -infinity | No exponent or significand payload |
|              > 0 |             0 |    * | NaN       |         * | No exponent payload                |
|              > 0 |           > 0 |    * | subnormal |         * |                                    |

#### +- 0

There is no exponent or significand payload. Sign is determined by the sign field.

#### +- Infinity

There is no exponent or significand payload. Sign is determined by the sign field.

#### NaN Values

There is no exponent payload. The significand payload is the same as in the original ieee754 float value.

#### Subnormal Values

The significand payload is the same as in the original ieee754 float value. As in ieee754, there is no implied leading `1` bit for subnormals.

The exponent field is set to the minimum exponent value of the original type and size (the value that would encode to 0 after the bias is applied):

| Type    | Size | Subnormal Exponent |
| ------- | ---- | ------------------ |
| Binary  |   16 |                -14 |
| Binary  |   32 |               -126 |
| Binary  |   64 |              -1022 |
| Binary  |  128 |             -16382 |
| Binary  |  256 |            -262142 |
| Decimal |   32 |                -95 |
| Decimal |   64 |               -383 |
| Decimal |  128 |              -6143 |



Decimal Floating Point Encoding
-------------------------------

IEEE754 defines two kinds of decimal floating point encodings (Densely Packed Decimal and Binary Decimal) without a means to automatically differentiate between them. For the purposes of this spec, all decimal floating point significands must be encoded in densely packed decimal format.



Examples
--------

### Value 1.3769248e-20

    32-bit ieee754 binary: 0x1e820c00 (0 00111101 00000100000110000000000)
    Sign bit: 0
    Exponent: 0x3d (61)

Convert the exponent to a signed int and subtract the bias:

    exponent = signed(61) - 127 = -66 (0xbe)

Strip trailing zero bytes from the significand:

    Original: 00000100000110000000000
    Stripped: 000001000001100 = 0x4c

Build header:

    Sign: 0
    Special: 0
    Exponent bytes: 1
    Significand bytes: 1
    Result: 00010001 = 0x11

Result: `[11 be 4c]`


### Value -2.03703597633448608627e+90

    64 bit ieee754 binary: 0xd2b0000000000000 (1 10100101011 00000000...)
    Sign bit: 1
    Exponent: 0x52d (1325)

Convert the exponent to a signed int and subtract the bias:

    exponent = signed(1325) - 1023 = 302 (0x12e)

Strip trailing zero bytes from the significand:

    Original: 0000 00000000 00000000 00000000 00000000 00000000 00000000
    Stripped: [empty]

Build header:

    Sign: 1
    Special: 0
    Exponent bytes: 2
    Significand bytes: 0
    Result: 10100000 = 0xb0

Result: `[b0 2e 01]`



License
-------

Copyright 2019 Karl Stenerud

Specification released under Creative Commons Attribution 4.0 International Public License https://creativecommons.org/licenses/by/4.0/
