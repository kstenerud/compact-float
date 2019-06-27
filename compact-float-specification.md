Compact Float Format
====================

Compact float format (CFF) is an encoding scheme to store a floating point value in as few bytes as possible for data transmission. This format supports all values that can be stored in 128 bit ieee754 floating point values (decimal and binary).

CFF can store all of the kinds of values that ieee754 can without data loss:
* Normalized values
* Subnormal values
* Positive and negative 0
* Positive and negative infinity
* NaNs (not a number)

Smaller size priority is generally given to the more common values.



Encoded Structure
-----------------

A compact floating point number is stored as a series of bitfields, the whole of which are then encoded as a [VLQ](https://github.com/kstenerud/vlq/blob/master/vlq-specification.md).

The general conceptual form of a floating point number is:

    value = significand * base ^ exponent

Where the base is 2 for binary floating point numbers and 10 for decimal floating point numbers.

Note: The encoded structure does not contain overall type information (i.e. whether this is a binary or decimal floating point value).


### Fields

The following fields are always present:

| Field            | Bits | Order | Notes                                     |
| ---------------- | ---- | ----- | ----------------------------------------- |
| Significand Sign |    1 | First | 0 = positive, 1 = negative                |
| Exponent Sign    |    1 |       | 0 = positive, 1 = negative                |
| Exponent Size    |    4 |       | Valid range 1-14. 0, 15 = special meaning |
| Payload          |    * | Last  | Contents determined by value type         |

#### Exponent Sign

Determines the sign of the exponent field: 0 = positive, 1 = negative.

#### Significand sign

Determines the sign of the significand field: 0 = positive, 1 = negative.

#### Exponent Size

Determines how many bits of exponent data will exist between this field and the significand field. The valid range is 1-14 bits, with values 0 and 15 representing special cases:

##### Exponent Size 0

The payload field is empty, and the significand and exponent sign fields determine the final value:

|Exponent Sign | Significand Sign | Value     |
|------------- | ---------------- | --------- |
|      0       |         0        | +0        |
|      0       |         1        | -0        |
|      1       |         0        | +infinity |
|      1       |         1        | -infinity |

##### Exponent Size 15

The exponent sign field determines the kind of value:

|Exponent Sign | Notes              |
|------------- | ------------------ |
|      1       | Subnormal Value    |
|      0       | Not a Number (NaN) |

#### Payload

The payload contents depend on what kind of value this is.



### Normalized Value

Normalized values have the following payload contents:

| Field            | Bits | Order | Notes                                     |
| ---------------- | ---- | ----- | ----------------------------------------- |
| Exponent         |    * | First | Bit width determined by exponent size     |
| Significand      |    * | Last  |                                           |

#### Exponent

Determines the amount that the base of the number (either 2 or 10) will be exponentially raised to.

#### Significand

The significand is interpreted as a normalized value, meaning a single, whole, nonzero digit, with the remainder of the value representing a fractional component:

    1.[fractional component] (binary)
    (1-9).[fractional component] (decimal)

As well, since the signifiand is always normalized, the initial `1` bit is implied (not actually stored in the significand field).

##### Example: Significand field = 01101001

Decimal FP:

    Field: 01101001
    Add implied 1: 101101001 (361 in decimal)
    Result = 3.61 (3 + 6/10 + 1/100)

Binary FP:

    Field: 01101001
    Add implied 1: 101101001
    Result = 1.01101001 (1 + 1/4 + 1/8 + 1/32 + 1/256 = 1.41015625 in decimal)



### Subnormal Value

A subnormal value indicates that the original value before encoding was a subnormal value. The value must be normalized for storage in the compact float, and then encoded in the same manner as a normalized value, with exponent size 15 and a negative exponent sign.



### Not a Number

NaNs have the following payload contents:

| Field   | Bits | Order | Notes                    |
| ------- | ---- | ----- | ------------------------ |
| Quiet   |  1   | First | 0 = signaling, 1 = quiet |
| Payload |  *   | Last  | Typically 0              |



License
-------

Copyright 2019 Karl Stenerud

Specification released under Creative Commons Attribution 4.0 International Public License https://creativecommons.org/licenses/by/4.0/
