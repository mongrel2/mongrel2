/* /////////////////////////////////////////////////////////////////////////
 * File:        b64/b64.h
 *
 * Purpose:     Header file for the b64 library
 *
 * Created:     18th October 2004
 * Updated:     18th November 2009
 *
 * Thanks:      To Adam McLaurin, for ideas regarding the b64_decode2() and
 *              b64_encode2().
 *
 * Home:        http://synesis.com.au/software/
 *
 * Copyright (c) 2004-2009, Matthew Wilson and Synesis Software
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name(s) of Matthew Wilson and Synesis Software nor the names of
 *   any contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * ////////////////////////////////////////////////////////////////////// */


/** \file b64/b64.h
 *
 * \brief [C/C++] Header file for the b64 library.
 */

#ifndef B64_INCL_B64_H_B64
#define B64_INCL_B64_H_B64

/* /////////////////////////////////////////////////////////////////////////
 * Version information
 */

#ifndef B64_DOCUMENTATION_SKIP_SECTION
# define B64_VER_B64_H_B64_MAJOR    1
# define B64_VER_B64_H_B64_MINOR    6
# define B64_VER_B64_H_B64_REVISION 1
# define B64_VER_B64_H_B64_EDIT     31
#endif /* !B64_DOCUMENTATION_SKIP_SECTION */

/** \def B64_VER_MAJOR
 * The major version number of b64
 */

/** \def B64_VER_MINOR
 * The minor version number of b64
 */

/** \def B64_VER_REVISION
 * The revision version number of b64
 */

/** \def B64_VER
 * The current composite version number of b64
 */

#ifndef B64_DOCUMENTATION_SKIP_SECTION
# define B64_VER_1_0_1          0x01000100
# define B64_VER_1_0_2          0x01000200
# define B64_VER_1_0_3          0x01000300
# define B64_VER_1_1_1          0x01010100
# define B64_VER_1_1_2          0x01010200
# define B64_VER_1_1_3          0x01010300
# define B64_VER_1_2_1          0x01020100
# define B64_VER_1_2_2          0x01020200
# define B64_VER_1_2_3          0x01020300
# define B64_VER_1_2_4          0x01020400
# define B64_VER_1_2_5          0x01020500
# define B64_VER_1_2_6          0x01020600
# define B64_VER_1_2_7          0x01020700
# define B64_VER_1_3_1          0x010301ff
# define B64_VER_1_3_2          0x010302ff
# define B64_VER_1_3_3          0x010303ff
# define B64_VER_1_3_4          0x010304ff
# define B64_VER_1_4_1_ALPHA_1  0x01040101
#endif /* !B64_DOCUMENTATION_SKIP_SECTION */

#define B64_VER_MAJOR       1
#define B64_VER_MINOR       4
#define B64_VER_REVISION    1
#define B64_VER             0x01040101

/* /////////////////////////////////////////////////////////////////////////
 * Includes
 */

#include <stddef.h>

/* /////////////////////////////////////////////////////////////////////////
 * Features
 */

/* If the Synesis pre-processor symbol SYNESIS_VARIANT_TEST is defined, then we
 * automatically define B64_VARIANT_TEST, which requests a test build: i.e.
 * debug information off, optimisations on, contract enforcements on
 */
#if defined(SYNESIS_VARIANT_TEST)
# if !defined(B64_VARIANT_TEST)
#  define B64_VARIANT_TEST
# endif /* !B64_VARIANT_TEST */
#endif /* SYNESIS_VARIANT_TEST */

/* /////////////////////////////////////////////////////////////////////////
 * Namespace
 */

#if !defined(B64_NO_NAMESPACE) && \
    !defined(__cplusplus)
# define B64_NO_NAMESPACE
#endif /* !B64_NO_NAMESPACE && !__cplusplus */

#ifdef B64_NAMESPACE
# undef B64_NAMESPACE
#endif /* B64_NAMESPACE */

#ifdef B64_NAMESPACE_QUALIFIER
# undef B64_NAMESPACE_QUALIFIER
#endif /* B64_NAMESPACE_QUALIFIER */


#ifndef B64_NO_NAMESPACE

# ifdef B64_CUSTOM_NAMESPACE
#  define B64_NAMESPACE     B64_CUSTOM_NAMESPACE
# else /* ? B64_CUSTOM_NAMESPACE */
#  define B64_NAMESPACE     b64
# endif /* B64_CUSTOM_NAMESPACE */

# if defined(B64_CUSTOM_NAMESPACE) && \
     defined(B64_CUSTOM_NAMESPACE_QUALIFIER)
#  define B64_NAMESPACE_QUALIFIER       B64_CUSTOM_NAMESPACE_QUALIFIER
# else /* B64_CUSTOM_NAMESPACE && B64_CUSTOM_NAMESPACE_QUALIFIER */
#  define B64_NAMESPACE_QUALIFIER       ::B64_NAMESPACE
# endif /* B64_CUSTOM_NAMESPACE && B64_CUSTOM_NAMESPACE_QUALIFIER */


/** [C/C++] The b64 namespace, within which the core library types and functions
 * reside in C++ compilation. In C compilation, they all reside in the global
 * namespace.
 *
 * \htmlonly
 * <hr>
 * \endhtmlonly
 */
namespace B64_NAMESPACE
{
#endif /* !B64_NO_NAMESPACE */

/* /////////////////////////////////////////////////////////////////////////
 * Typedefs
 */

/** The ambient character type of the library
 *
 * \note Currently, this is always \c char, supporting only multibyte strings
 */
typedef char    b64_char_t;

/** Return codes (from b64_encode2() / b64_decode2())
 */
enum B64_RC
{
        B64_RC_OK                   =   0   /*!< Operation was successful. */
    ,   B64_RC_INSUFFICIENT_BUFFER  =   1   /*!< The given translation buffer was not of sufficient size. */
    ,   B64_RC_TRUNCATED_INPUT      =   2   /*!< The input did not represent a fully formed stream of octet couplings. */
    ,   B64_RC_DATA_ERROR           =   3   /*!< Invalid data. */
#ifndef B64_DOCUMENTATION_SKIP_SECTION
    ,   B64_max_RC_value
#endif /* !B64_DOCUMENTATION_SKIP_SECTION */
};

#ifndef __cplusplus
typedef enum B64_RC B64_RC;
#endif /* !__cplusplus */

/** Coding behaviour modification flags (for b64_encode2() / b64_decode2())
 */
enum B64_FLAGS
{
        B64_F_LINE_LEN_USE_PARAM    =   0x0000  /*!< Uses the lineLen parameter to b64_encode2(). Ignored by b64_decode2(). */
    ,   B64_F_LINE_LEN_INFINITE     =   0x0001  /*!< Ignores the lineLen parameter to b64_encode2(). Line length is infinite. Ignored by b64_decode2(). */
    ,   B64_F_LINE_LEN_64           =   0x0002  /*!< Ignores the lineLen parameter to b64_encode2(). Line length is 64. Ignored by b64_decode2(). */
    ,   B64_F_LINE_LEN_76           =   0x0003  /*!< Ignores the lineLen parameter to b64_encode2(). Line length is 76. Ignored by b64_decode2(). */
    ,   B64_F_LINE_LEN_MASK         =   0x000f  /*!< Mask for testing line length flags to b64_encode2(). Ignored by b64_encode2(). */
    ,   B64_F_STOP_ON_NOTHING       =   0x0000  /*!< Decoding ignores all invalid characters in the input data. Ignored by b64_encode2(). */
    ,   B64_F_STOP_ON_UNKNOWN_CHAR  =   0x0100  /*!< Causes decoding to break if any non-Base-64 [a-zA-Z0-9=+/], non-whitespace character is encountered. Ignored by b64_encode2(). */
    ,   B64_F_STOP_ON_UNEXPECTED_WS =   0x0200  /*!< Causes decoding to break if any unexpected whitespace is encountered. Ignored by b64_encode2(). */
    ,   B64_F_STOP_ON_BAD_CHAR      =   0x0300  /*!< Causes decoding to break if any non-Base-64 [a-zA-Z0-9=+/] character is encountered. Ignored by b64_encode2(). */
};

#ifndef __cplusplus
typedef enum B64_FLAGS  B64_FLAGS;
#endif /* !__cplusplus */

/* /////////////////////////////////////////////////////////////////////////
 * Functions
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** Encodes a block of binary data into Base-64
 *
 * \param src Pointer to the block to be encoded. May not be NULL, except when
 *   \c dest is NULL, in which case it is ignored.
 * \param srcSize Length of block to be encoded
 * \param dest Pointer to the buffer into which the result is to be written. May
 *   be NULL, in which case the function returns the required length
 * \param destLen Length of the buffer into which the result is to be written. Must
 *   be at least as large as that indicated by the return value from
 *   \link b64::b64_encode b64_encode(NULL, srcSize, NULL, 0)\endlink.
 *
 * \return 0 if the size of the buffer was insufficient, or the length of the
 * converted buffer was longer than \c destLen
 *
 * \note The function returns the required length if \c dest is NULL
 *
 * \note The function returns the required length if \c dest is NULL. The returned size
 *   might be larger than the actual required size, but will never be smaller.
 *
 * \note Threading: The function is fully re-entrant.
 *
 * \see b64::encode()
 */
size_t b64_encode(
    void const* src
,   size_t      srcSize
,   b64_char_t* dest
,   size_t      destLen
);

/** Encodes a block of binary data into Base-64
 *
 * \param src Pointer to the block to be encoded. May not be NULL, except when
 *   \c dest is NULL, in which case it is ignored.
 * \param srcSize Length of block to be encoded
 * \param dest Pointer to the buffer into which the result is to be written. May
 *   be NULL, in which case the function returns the required length
 * \param destLen Length of the buffer into which the result is to be written. Must
 *   be at least as large as that indicated by the return value from
 *   \link b64::b64_encode2 b64_encode2(NULL, srcSize, NULL, 0, flags, lineLen, rc)\endlink.
 * \param flags A combination of the B64_FLAGS enumeration, that moderate the
 *   behaviour of the function
 * \param lineLen If the flags parameter contains B64_F_LINE_LEN_USE_PARAM, then
 *   this parameter represents the length of the lines into which the encoded form is split,
 *   with a hard line break ('\\r\\n'). If this value is 0, then the line is not
 *   split. If it is <0, then the RFC-1113 recommended line length of 64 is used
 * \param rc The return code representing the status of the operation. May be NULL.
 *
 * \return 0 if the size of the buffer was insufficient, or the length of the
 *   converted buffer was longer than \c destLen
 *
 * \note The function returns the required length if \c dest is NULL. The returned size
 *   might be larger than the actual required size, but will never be smaller.
 *
 * \note Threading: The function is fully re-entrant.
 *
 * \see b64::encode()
 */
size_t b64_encode2(
    void const* src
,   size_t      srcSize
,   b64_char_t* dest
,   size_t      destLen
,   unsigned    flags
,   int         lineLen /* = 0 */
,   B64_RC*     rc     /* = NULL */
);

/** Decodes a sequence of Base-64 into a block of binary data
 *
 * \param src Pointer to the Base-64 block to be decoded. May not be NULL, except when
 *   \c dest is NULL, in which case it is ignored. If \c dest is NULL, and \c src is
 *   <b>not</b> NULL, then the returned value is calculated exactly, otherwise a value
 *   is returned that is guaranteed to be large enough to hold the decoded block.
 *
 * \param srcLen Length of block to be encoded. Must be an integral of 4, the Base-64
 *   encoding quantum, otherwise the Base-64 block is assumed to be invalid
 * \param dest Pointer to the buffer into which the result is to be written. May
 *   be NULL, in which case the function returns the required length
 * \param destSize Length of the buffer into which the result is to be written. Must
 *   be at least as large as that indicated by the return value from
 *   \c b64_decode(src, srcSize, NULL, 0), even in the case where the encoded form
 *   contains a number of characters that will be ignored, resulting in a lower total
 *   length of converted form.
 *
 * \return 0 if the size of the buffer was insufficient, or the length of the
 *   converted buffer was longer than \c destSize
 *
 * \note The function returns the required length if \c dest is NULL. The returned size
 *   might be larger than the actual required size, but will never be smaller.
 *
 * \note \anchor anchor__4_characters The behaviour of both
 * \link b64::b64_encode2 b64_encode2()\endlink
 * and
 * \link b64::b64_decode2 b64_decode2()\endlink
 * are undefined if the line length is not a multiple of 4.
 *
 * \note Threading: The function is fully re-entrant.
 *
 * \see b64::decode()
 */
size_t b64_decode(
    b64_char_t const*   src
,   size_t              srcLen
,   void*               dest
,   size_t              destSize
);

/** Decodes a sequence of Base-64 into a block of binary data
 *
 * \param src Pointer to the Base-64 block to be decoded. May not be NULL, except when
 * \c dest is NULL, in which case it is ignored. If \c dest is NULL, and \c src is
 * <b>not</b> NULL, then the returned value is calculated exactly, otherwise a value
 * is returned that is guaranteed to be large enough to hold the decoded block.
 *
 * \param srcLen Length of block to be encoded. Must be an integral of 4, the Base-64
 *   encoding quantum, otherwise the Base-64 block is assumed to be invalid
 * \param dest Pointer to the buffer into which the result is to be written. May
 *   be NULL, in which case the function returns the required length
 * \param destSize Length of the buffer into which the result is to be written. Must
 *   be at least as large as that indicated by the return value from
 *   \c b64_decode(src, srcSize, NULL, 0), even in the case where the encoded form
 *   contains a number of characters that will be ignored, resulting in a lower total
 *   length of converted form.
 * \param flags A combination of the B64_FLAGS enumeration, that moderate the
 *   behaviour of the function.
 * \param rc The return code representing the status of the operation. May be NULL.
 * \param badChar If the flags parameter does not contain B64_F_STOP_ON_NOTHING, this
 *   parameter specifies the address of a pointer that will be set to point to any
 *   character in the sequence that stops the parsing, as dictated by the flags
 *   parameter. May be NULL.
 *
 * \return 0 if the size of the buffer was insufficient, or the length of the
 * converted buffer was longer than \c destSize, or a bad character stopped parsing.
 *
 * \note The function returns the required length if \c dest is NULL. The returned size
 *   might be larger than the actual required size, but will never be smaller.
 *
 * \note The behaviour of both
 * \link b64::b64_encode2 b64_encode2()\endlink
 * and
 * \link b64::b64_decode2 b64_decode2()\endlink
 * are undefined if the line length is not a multiple of 4.
 *
 * \note Threading: The function is fully re-entrant.
 *
 * \see b64::decode()
 */
size_t b64_decode2(
    b64_char_t const*   src
,   size_t              srcLen
,   void*               dest
,   size_t              destSize
,   unsigned            flags
,   b64_char_t const**  badChar /* = NULL */
,   B64_RC*             rc      /* = NULL */
);

/** Returns the textual description of the error
 *
 * \param code The \link b64::B64_RC error code\endlink
 */
char const* b64_getErrorString(B64_RC code);

/** Returns the length of the textual description of the error
 *
 * \see b64_getErrorString()
 *
 * \param code The \link b64::B64_RC error code\endlink
 */
size_t b64_getErrorStringLength(B64_RC code);


#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

/* /////////////////////////////////////////////////////////////////////////
 * Namespace
 */

#ifndef B64_NO_NAMESPACE
} /* namespace B64_NAMESPACE */

# ifndef B64_DOCUMENTATION_SKIP_SECTION

namespace stlsoft
{

    inline char const *c_str_data_a( B64_NAMESPACE_QUALIFIER::B64_RC code)
    {
        return B64_NAMESPACE_QUALIFIER::b64_getErrorString(code);
    }
    inline char const *c_str_data( B64_NAMESPACE_QUALIFIER::B64_RC code)
    {
        return B64_NAMESPACE_QUALIFIER::b64_getErrorString(code);
    }

    inline size_t c_str_len_a( B64_NAMESPACE_QUALIFIER::B64_RC code)
    {
        return B64_NAMESPACE_QUALIFIER::b64_getErrorStringLength(code);
    }
    inline size_t c_str_len( B64_NAMESPACE_QUALIFIER::B64_RC code)
    {
        return B64_NAMESPACE_QUALIFIER::b64_getErrorStringLength(code);
    }

    inline char const *c_str_ptr_a( B64_NAMESPACE_QUALIFIER::B64_RC code)
    {
        return B64_NAMESPACE_QUALIFIER::b64_getErrorString(code);
    }
    inline char const *c_str_ptr( B64_NAMESPACE_QUALIFIER::B64_RC code)
    {
        return B64_NAMESPACE_QUALIFIER::b64_getErrorString(code);
    }

} /* namespace stlsoft */

# endif /* !B64_DOCUMENTATION_SKIP_SECTION */

#endif /* !B64_NO_NAMESPACE */

/* ////////////////////////////////////////////////////////////////////// */

#endif /* B64_INCL_B64_H_B64 */

/* ///////////////////////////// end of file //////////////////////////// */
