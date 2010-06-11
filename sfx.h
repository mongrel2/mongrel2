/*
 * SideChk---A utility which tries to determine whether a given C expression
 * is free of side effects. This can be used for verifying that macros which
 * expand their arguments more than once are not being accidentally misused.
 *
 * Copyright (C) 1999 Kaz Kylheku <kaz@ashi.footprints.net>
 *
 * Free Software License:
 *
 * All rights are reserved by the author, with the following exceptions:
 * Permission is granted to freely reproduce and distribute this software,
 * possibly in exchange for a fee, provided that this copyright notice appears
 * intact. Permission is also granted to adapt this software to produce
 * derivative works, as long as the modified versions carry this copyright
 * notice and additional notices stating that the work has been modified.
 * This source code may be translated into executable form and incorporated
 * into proprietary software; there is no requirement for such software to
 * contain a copyright notice related to this source.
 *
 * $Id: sfx.h,v 1.7 1999/11/07 17:01:53 kaz Exp $
 * $Name: kazlib_1_20 $
 */

#ifndef SFX_H
#define SFX_H

#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    sfx_none, sfx_potential, sfx_certain
} sfx_rating_t;

int sfx_determine(const char *, sfx_rating_t *);
int sfx_declare(const char *, sfx_rating_t);
void sfx_check(const char *, const char *, unsigned long);

#ifdef __cplusplus
}
#endif

#define SFX_CHECK(E) (sfx_check(#E, __FILE__, __LINE__), (E))
#define SFX_STRING(E) #E

#endif
