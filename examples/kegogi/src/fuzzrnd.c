/**
 *
 * Copyright (c) 2010, Zed A. Shaw and Mongrel2 Project Contributors.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 * 
 *     * Neither the name of the Mongrel2 Project, Zed A. Shaw, nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

/**
 * We use one source of ArcFour data for now.  This means that things aren't
 * thread safe yet, but since the ArcFour is just for the current weaker implementation
 * I'm not investing any more time making it thread safe.
 */
static struct
{
  unsigned char  i,j;                        /* ArcFour variables */
  unsigned char  sbox[256];                  /* ArcFour s-box */
} ArcFour;


/** 
 * Returns a buffer of random bytes of length that you can use 
 * for generating randomness.  It uses the ArcFour cipher to 
 * make the randomness, so the same seeds produce the same 
 * random bits, and the randomness is reasonably high quality.
 *
 * Don't use this for secure random generation.  It probably would
 * work if you seeded from a /dev/random that worked, but don't
 * blame me if you get hacked.
 *
 * The main motiviation for using ArcFour without automated reseed
 * is to produce lots of random bytes quickly, make them high enough
 * quality for good random tests, and to make sure that we can replay
 * possible sequences if there's a sequence that we want to test.
 */
char *FuzzRnd_data(size_t len)
{

  unsigned int n;
  unsigned char a,b;
  char *p = NULL;

  p = malloc(len);

  for (n=0;n<len;n++)             /* run the ArcFour algorithm as long as it needs */
  {
    ArcFour.i++;
    a     =         ArcFour.sbox[ArcFour.i];
    ArcFour.j = (unsigned char) (ArcFour.j + a);     /* avoid MSVC picky compiler warning */
    b     =         ArcFour.sbox[ArcFour.j];
    ArcFour.sbox[ArcFour.i] = b;
    ArcFour.sbox[ArcFour.j] = a;
    p[n]  = ArcFour.sbox[(a+b) & 0xFF];
  }

  return p;
}


/** 
 * Seeds the global ArcFour random generator with the given seed.  The same seeds
 * should produce the exact same stream of random data so that you can get 
 * large amounts of randomness but replay possible interactions using just 
 * an initial key.
 *
 * Taken from http://www.mozilla.org/projects/security/pki/nss/draft-kaukonen-cipher-arcfour-03.txt
 * sample code, but compared with the output of the ArcFour implementation in
 * the Phelix test code to make sure it is the same initialization.  The main
 * difference is that this init takes an arbitrary keysize while the original
 * Phelix ArcFour only took a 32bit key.
 */
void FuzzRnd_seed(char *key, size_t key_len) 
{

  unsigned int t, u;
  unsigned int keyindex;
  unsigned int stateindex;
  unsigned char *state;
  unsigned int counter;

  state = ArcFour.sbox;
  ArcFour.i = 0;
  ArcFour.j = 0;

  for (counter = 0; counter < 256; counter++)
    state[counter] = counter;

  keyindex = 0;
  stateindex = 0;
  for (counter = 0; counter < 256; counter++)
  {
    t = state[counter];
    stateindex = (stateindex + key[keyindex] + t) & 0xff;
    u = state[stateindex];
    state[stateindex] = t;
    state[counter] = u;
    if (++keyindex >= key_len)
      keyindex = 0;
  }
}



