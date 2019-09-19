/*
 * Compact Float
 * =============
 *
 *
 * License
 * -------
 *
 * Copyright 2019 Karl Stenerud
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#ifndef KS_compact_float_H
#define KS_compact_float_H

#ifndef COMPACT_FLOAT_PUBLIC
    #if defined _WIN32 || defined __CYGWIN__
        #define COMPACT_FLOAT_PUBLIC __declspec(dllimport)
    #else
        #define COMPACT_FLOAT_PUBLIC
    #endif
#endif

#ifndef ANSI_EXTENSION
    #ifdef __GNUC__
        #define ANSI_EXTENSION __extension__
    #else
        #define ANSI_EXTENSION
    #endif
#endif


#ifdef __GNUC__
    #ifdef __cplusplus
        #include <decimal/decimal>
        // Hack: According to [ISO/IEC TR 24733], the compiler should supply this typedef
        // typedef std::decimal::decimal64 _Decimal64;
        #define _Decimal64 std::decimal::decimal64::__decfloat64
        #define SUPPORTS_DECFLOAT 1
    #else
        #define __STDC_WANT_DEC_FP__
        #include <float.h>
        #ifdef DEC_EVAL_METHOD
            #define SUPPORTS_DECFLOAT 1
        #endif
    #endif
#endif


#ifdef __cplusplus 
extern "C" {
#endif

#include <stdint.h>


// ---
// API
// ---

/**
 * Get the current library version as a semantic version (e.g. "1.5.2").
 *
 * @return The library version.
 */
COMPACT_FLOAT_PUBLIC const char* cfloat_version();

/**
 * Calculate the number of bytes that would be occupied by this float when
 * encoded.
 */
COMPACT_FLOAT_PUBLIC int cfloat_binary_encoded_size(double value);

/**
 * Encode a binary float to a destination buffer.
 *
 * Returns the number of bytes written to encode the value, or 0 if there wasn't
 * enough room.
 */
COMPACT_FLOAT_PUBLIC int cfloat_binary_encode(double value, uint8_t* dst, int dst_length);

/**
 * Decode a binary float from a source buffer.
 *
 * Returns the number of bytes read to decode the value, or 0 if there wasn't
 * enough data.
 */
COMPACT_FLOAT_PUBLIC int cfloat_binary_decode(const uint8_t* src, int src_length, double* value);


#ifdef SUPPORTS_DECFLOAT

/**
 * Calculate the number of bytes that would be occupied by this float when
 * encoded.
 */
ANSI_EXTENSION COMPACT_FLOAT_PUBLIC int cfloat_decimal_encoded_size(_Decimal64 value);

/**
 * Encode a decimal float to a destination buffer.
 *
 * Returns the number of bytes written to encode the date, or 0 if there wasn't
 * enough room.
 */
ANSI_EXTENSION COMPACT_FLOAT_PUBLIC int cfloat_decimal_encode(_Decimal64 value, uint8_t* dst, int dst_length);

/**
 * Decode a decimal float from a source buffer.
 *
 * Returns the number of bytes read to decode the date, or 0 if there wasn't
 * enough data.
 */
ANSI_EXTENSION COMPACT_FLOAT_PUBLIC int cfloat_decimal_decode(const uint8_t* src, int src_length, _Decimal64* value);

#endif // SUPPORTS_DECFLOAT


#ifdef __cplusplus 
}
#endif
#endif // KS_compact_float_H
