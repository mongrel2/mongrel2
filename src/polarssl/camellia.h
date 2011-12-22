/**
 * \file camellia.h
 *
 * \brief Camellia block cipher
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
#ifndef POLARSSL_CAMELLIA_H
#define POLARSSL_CAMELLIA_H

#include <string.h>

#ifdef _MSC_VER
#include <basetsd.h>
typedef UINT32 uint32_t;
#else
#include <inttypes.h>
#endif

#define CAMELLIA_ENCRYPT     1
#define CAMELLIA_DECRYPT     0

#define POLARSSL_ERR_CAMELLIA_INVALID_KEY_LENGTH           -0x0024  /**< Invalid key length. */
#define POLARSSL_ERR_CAMELLIA_INVALID_INPUT_LENGTH         -0x0026  /**< Invalid data input length. */

/**
 * \brief          CAMELLIA context structure
 */
typedef struct
{
    int nr;                     /*!<  number of rounds  */
    uint32_t rk[68];            /*!<  CAMELLIA round keys    */
}
camellia_context;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief          CAMELLIA key schedule (encryption)
 *
 * \param ctx      CAMELLIA context to be initialized
 * \param key      encryption key
 * \param keysize  must be 128, 192 or 256
 * 
 * \return         0 if successful, or POLARSSL_ERR_CAMELLIA_INVALID_KEY_LENGTH
 */
int camellia_setkey_enc( camellia_context *ctx, const unsigned char *key, unsigned int keysize );

/**
 * \brief          CAMELLIA key schedule (decryption)
 *
 * \param ctx      CAMELLIA context to be initialized
 * \param key      decryption key
 * \param keysize  must be 128, 192 or 256
 * 
 * \return         0 if successful, or POLARSSL_ERR_CAMELLIA_INVALID_KEY_LENGTH
 */
int camellia_setkey_dec( camellia_context *ctx, const unsigned char *key, unsigned int keysize );

/**
 * \brief          CAMELLIA-ECB block encryption/decryption
 *
 * \param ctx      CAMELLIA context
 * \param mode     CAMELLIA_ENCRYPT or CAMELLIA_DECRYPT
 * \param input    16-byte input block
 * \param output   16-byte output block
 * 
 * \return         0 if successful
 */
int camellia_crypt_ecb( camellia_context *ctx,
                    int mode,
                    const unsigned char input[16],
                    unsigned char output[16] );

/**
 * \brief          CAMELLIA-CBC buffer encryption/decryption
 *                 Length should be a multiple of the block
 *                 size (16 bytes)
 *
 * \param ctx      CAMELLIA context
 * \param mode     CAMELLIA_ENCRYPT or CAMELLIA_DECRYPT
 * \param length   length of the input data
 * \param iv       initialization vector (updated after use)
 * \param input    buffer holding the input data
 * \param output   buffer holding the output data
 * 
 * \return         0 if successful, or POLARSSL_ERR_CAMELLIA_INVALID_INPUT_LENGTH
 */
int camellia_crypt_cbc( camellia_context *ctx,
                    int mode,
                    size_t length,
                    unsigned char iv[16],
                    const unsigned char *input,
                    unsigned char *output );

/**
 * \brief          CAMELLIA-CFB128 buffer encryption/decryption
 *
 * \param ctx      CAMELLIA context
 * \param mode     CAMELLIA_ENCRYPT or CAMELLIA_DECRYPT
 * \param length   length of the input data
 * \param iv_off   offset in IV (updated after use)
 * \param iv       initialization vector (updated after use)
 * \param input    buffer holding the input data
 * \param output   buffer holding the output data
 * 
 * \return         0 if successful, or POLARSSL_ERR_CAMELLIA_INVALID_INPUT_LENGTH
 */
int camellia_crypt_cfb128( camellia_context *ctx,
                       int mode,
                       size_t length,
                       int *iv_off,
                       unsigned char iv[16],
                       const unsigned char *input,
                       unsigned char *output );

/**
 * \brief          Checkup routine
 *
 * \return         0 if successful, or 1 if the test failed
 */
int camellia_self_test( int verbose );

#ifdef __cplusplus
}
#endif

#endif /* camellia.h */
