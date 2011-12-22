/**
 * \file cipher.h
 * 
 * \brief Generic cipher wrapper.
 *
 * \author Adriaan de Jong <dejong@fox-it.com>
 *
 *  Copyright (C) 2006-2010, Brainspark B.V.
 *
 *  This file is part of PolarSSL (http://www.polarssl.org)
 *  Lead Maintainer: Paul Bakker <polarssl_maintainer at polarssl.org>
 *
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef POLARSSL_CIPHER_H
#define POLARSSL_CIPHER_H

#include <string.h>

#ifdef _MSC_VER
#define inline _inline
#endif

typedef enum {
    POLARSSL_CIPHER_ID_NONE = 0,
    POLARSSL_CIPHER_ID_AES,
    POLARSSL_CIPHER_ID_DES,
    POLARSSL_CIPHER_ID_3DES,
    POLARSSL_CIPHER_ID_CAMELLIA,
} cipher_id_t;

typedef enum {
    POLARSSL_CIPHER_NONE = 0,
    POLARSSL_CIPHER_CAMELLIA_128_CBC,
    POLARSSL_CIPHER_CAMELLIA_192_CBC,
    POLARSSL_CIPHER_CAMELLIA_256_CBC,
    POLARSSL_CIPHER_AES_128_CBC,
    POLARSSL_CIPHER_AES_192_CBC,
    POLARSSL_CIPHER_AES_256_CBC,
    POLARSSL_CIPHER_DES_CBC,
    POLARSSL_CIPHER_DES_EDE_CBC,
    POLARSSL_CIPHER_DES_EDE3_CBC
} cipher_type_t;

typedef enum {
    POLARSSL_MODE_NONE = 0,
    POLARSSL_MODE_CBC,
    POLARSSL_MODE_CFB,
    POLARSSL_MODE_OFB,
} cipher_mode_t;

typedef enum {
    POLARSSL_DECRYPT = 0,
    POLARSSL_ENCRYPT,
} operation_t;

enum {
    /** Undefined key length */
    POLARSSL_KEY_LENGTH_NONE = 0,
    /** Key length, in bits, for DES keys */
    POLARSSL_KEY_LENGTH_DES  = 56,
    /** Key length, in bits, for DES in two key EDE */
    POLARSSL_KEY_LENGTH_DES_EDE = 112,
    /** Key length, in bits, for DES in three-key EDE */
    POLARSSL_KEY_LENGTH_DES_EDE3 = 168,
    /** Maximum length of any IV, in bytes */
    POLARSSL_MAX_IV_LENGTH = 16,
};

/**
 * Cipher information. Allows cipher functions to be called in a generic way.
 */
typedef struct {
    /** Full cipher identifier (e.g. POLARSSL_CIPHER_AES_256_CBC) */
    cipher_type_t type;

    /** Base Cipher type (e.g. POLARSSL_CIPHER_ID_AES) */
    cipher_id_t cipher;

    /** Cipher mode (e.g. POLARSSL_CIPHER_MODE_CBC) */
    cipher_mode_t mode;

    /** Cipher key length, in bits (default length for variable sized ciphers) */
    unsigned int key_length;

    /** Name of the cipher */
    const char * name;

    /** IV size, in bytes */
    unsigned int iv_size;

    /** block size, in bytes */
    unsigned int block_size;

    /** Encrypt using CBC */
    int (*cbc_func)( void *ctx, operation_t mode, size_t length, unsigned char *iv,
            const unsigned char *input, unsigned char *output );

    /** Set key for encryption purposes */
    int (*setkey_enc_func)( void *ctx, const unsigned char *key, unsigned int key_length);

    /** Set key for decryption purposes */
    int (*setkey_dec_func)( void *ctx, const unsigned char *key, unsigned int key_length);

    /** Allocate a new context */
    void * (*ctx_alloc_func)( void );

    /** Free the given context */
    void (*ctx_free_func)( void *ctx );

} cipher_info_t;

/**
 * Generic message digest context.
 */
typedef struct {
    /** Information about the associated cipher */
    const cipher_info_t *cipher_info;

    /** Key length to use */
    int key_length;

    /** Operation that the context's key has been initialised for */
    operation_t operation;

    /** Buffer for data that hasn't been encrypted yet */
    unsigned char unprocessed_data[POLARSSL_MAX_IV_LENGTH];

    /** Number of bytes that still need processing */
    size_t unprocessed_len;

    /** Current IV */
    unsigned char iv[POLARSSL_MAX_IV_LENGTH];

    /** Cipher-specific context */
    void *cipher_ctx;
} cipher_context_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Returns the list of ciphers supported by the generic cipher module.
 *
 * \return              a statically allocated array of ciphers, the last entry
 *                      is 0.
 */
const int *cipher_list( void );

/**
 * \brief               Returns the cipher information structure associated
 *                      with the given cipher name.
 *
 * \param cipher_name   Name of the cipher to search for.
 *
 * \return              the cipher information structure associated with the
 *                      given cipher_name, or NULL if not found.
 */
const cipher_info_t *cipher_info_from_string( const char *cipher_name );

/**
 * \brief               Returns the cipher information structure associated
 *                      with the given cipher type.
 *
 * \param cipher_type   Type of the cipher to search for.
 *
 * \return              the cipher information structure associated with the
 *                      given cipher_type, or NULL if not found.
 */
const cipher_info_t *cipher_info_from_type( const cipher_type_t cipher_type );

/**
 * \brief               Initialises and fills the cipher context structure with
 *                      the appropriate values.
 *
 * \param ctx           context to initialise. May not be NULL.
 * \param cipher_info   cipher to use.
 *
 * \return              \c 0 on success, \c 1 on parameter failure, \c 2 if
 *                      allocation of the cipher-specific context failed.
 */
int cipher_init_ctx( cipher_context_t *ctx, const cipher_info_t *cipher_info );

/**
 * \brief               Free the cipher-specific context of ctx. Freeing ctx
 *                      itself remains the responsibility of the caller.
 *
 * \param ctx           Free the cipher-specific context
 *
 * \returns             0 on success, 1 if parameter verification fails.
 */
int cipher_free_ctx( cipher_context_t *ctx );

/**
 * \brief               Returns the block size of the given cipher.
 *
 * \param ctx           cipher's context. Must have been initialised.
 *
 * \return              size of the cipher's blocks, or 0 if ctx has not been
 *                      initialised.
 */
static inline unsigned int cipher_get_block_size( const cipher_context_t *ctx )
{
    if( NULL == ctx || NULL == ctx->cipher_info )
        return 0;

    return ctx->cipher_info->block_size;
}

/**
 * \brief               Returns the size of the cipher's IV.
 *
 * \param ctx           cipher's context. Must have been initialised.
 *
 * \return              size of the cipher's IV, or 0 if ctx has not been
 *                      initialised.
 */
static inline int cipher_get_iv_size( const cipher_context_t *ctx )
{
    if( NULL == ctx || NULL == ctx->cipher_info )
        return 0;

    return ctx->cipher_info->iv_size;
}

/**
 * \brief               Returns the type of the given cipher.
 *
 * \param ctx           cipher's context. Must have been initialised.
 *
 * \return              type of the cipher, or POLARSSL_CIPHER_NONE if ctx has
 *                      not been initialised.
 */
static inline cipher_type_t cipher_get_type( const cipher_context_t *ctx )
{
    if( NULL == ctx || NULL == ctx->cipher_info )
        return 0;

    return ctx->cipher_info->type;
}

/**
 * \brief               Returns the name of the given cipher, as a string.
 *
 * \param ctx           cipher's context. Must have been initialised.
 *
 * \return              name of the cipher, or NULL if ctx was not initialised.
 */
static inline const char *cipher_get_name( const cipher_context_t *ctx )
{
    if( NULL == ctx || NULL == ctx->cipher_info )
        return 0;

    return ctx->cipher_info->name;
}

/**
 * \brief               Returns the key length of the cipher.
 *
 * \param ctx           cipher's context. Must have been initialised.
 *
 * \return              cipher's key length, in bits, or
 *                      POLARSSL_KEY_LENGTH_NONE if ctx has not been
 *                      initialised.
 */
static inline int cipher_get_key_size ( const cipher_context_t *ctx )
{
    if( NULL == ctx )
        return POLARSSL_KEY_LENGTH_NONE;

    return ctx->key_length;
}

/**
 * \brief               Set the key to use with the given context.
 *
 * \param ctx           generic cipher context. May not be NULL. Must have been
 *                      initialised using cipher_context_from_type or
 *                      cipher_context_from_string.
 * \param key           The key to use.
 * \param key_length    key length to use, in bits.
 * \param operation     Operation that the key will be used for, either
 *                      POLARSSL_ENCRYPT or POLARSSL_DECRYPT.
 *
 * \returns             0 on success, 1 if parameter verification fails.
 */
int cipher_setkey( cipher_context_t *ctx, const unsigned char *key, int key_length,
        const operation_t operation );

/**
 * \brief               Reset the given context, setting the IV to iv
 *
 * \param ctx           generic cipher context
 * \param iv            IV to use
 *
 * \returns             0 on success, 1 if parameter verification fails.
 */
int cipher_reset( cipher_context_t *ctx, const unsigned char *iv );

/**
 * \brief               Generic cipher update function. Encrypts/decrypts
 *                      using the given cipher context. Writes as many block
 *                      size'd blocks of data as possible to output. Any data
 *                      that cannot be written immediately will either be added
 *                      to the next block, or flushed when cipher_final is
 *                      called.
 *
 * \param ctx           generic cipher context
 * \param input         buffer holding the input data
 * \param ilen          length of the input data
 * \param output        buffer for the output data. Should be able to hold at
 *                      least ilen + block_size. Cannot be the same buffer as
 *                      input!
 * \param olen          length of the output data, will be filled with the
 *                      actual number of bytes written.
 *
 * \returns             0 on success, 1 if parameter verification fails.
 */
int cipher_update( cipher_context_t *ctx, const unsigned char *input, size_t ilen,
        unsigned char *output, size_t *olen );

/**
 * \brief               Generic cipher finalisation function. If data still
 *                      needs to be flushed from an incomplete block, data
 *                      contained within it will be padded with the size of
 *                      the last block, and written to the output buffer.
 *
 * \param ctx           Generic message digest context
 * \param output        buffer to write data to. Needs block_size data available.
 * \param olen          length of the data written to the output buffer.
 *
 * \returns             0 on success, 1 if parameter verification fails.
 */
int cipher_finish( cipher_context_t *ctx, unsigned char *output, size_t *olen);


/**
 * \brief          Checkup routine
 *
 * \return         0 if successful, or 1 if the test failed
 */
int cipher_self_test( int verbose );

#ifdef __cplusplus
}
#endif

#endif /* POLARSSL_MD_H */
