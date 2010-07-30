/*
 * Based on pattern matching code from Lua removing captures.
 */

#include <dbg.h>
#include <stdlib.h>
#include <pattern.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>



typedef struct MatchState {
  const char *src_init;  /* init of source string */
  const char *src_end;  /* end (`\0') of source string */
} MatchState;


#define CAP_UNFINISHED    (-1)
#define CAP_POSITION    (-2)

#define uchar(S) (S)


#define L_ESC        '\\'


const char *classend (MatchState *ms, const char *p) {
  switch (*p++) {
    case L_ESC: {
      if (*p == '\0')
        log_err("malformed pattern (ends with '\\0')");
      return p+1;
    }
    case '[': {
      if (*p == '^') p++;
      do {  /* look for a `]' */
        if (*p == '\0')
          log_err("malformed pattern (missing ])");
        if (*(p++) == L_ESC && *p != '\0')
          p++;  /* skip escapes (e.g. `\\]') */
      } while (*p != ']');
      return p+1;
    }
    default: {
      return p;
    }
  }
}


int match_class (int c, int cl) {
  int res;
  switch (tolower(cl)) {
    case 'a' : res = isalpha(c); break;
    case 'c' : res = iscntrl(c); break;
    case 'd' : res = isdigit(c); break;
    case 'l' : res = islower(c); break;
    case 'p' : res = ispunct(c); break;
    case 's' : res = isspace(c); break;
    case 'u' : res = isupper(c); break;
    case 'w' : res = isalnum(c); break;
    case 'x' : res = isxdigit(c); break;
    case 'z' : res = (c == 0); break;
    default: return (cl == c);
  }
  return (islower(cl) ? res : !res);
}


int matchbracketclass (int c, const char *p, const char *ec) {
  int sig = 1;
  if (*(p+1) == '^') {
    sig = 0;
    p++;  /* skip the `^' */
  }
  while (++p < ec) {
    if (*p == L_ESC) {
      p++;
      if (match_class(c, uchar(*p)))
        return sig;
    }
    else if ((*(p+1) == '-') && (p+2 < ec)) {
      p+=2;
      if (uchar(*(p-2)) <= c && c <= uchar(*p))
        return sig;
    }
    else if (uchar(*p) == c) return sig;
  }
  return !sig;
}


int singlematch (int c, const char *p, const char *ep) {
  switch (*p) {
    case '.': return 1;  /* matches any char */
    case L_ESC: return match_class(c, uchar(*(p+1)));
    case '[': return matchbracketclass(c, p, ep-1);
    default:  return (uchar(*p) == c);
  }
}


const char *match (MatchState *ms, const char *s, const char *p);


const char *matchbalance (MatchState *ms, const char *s,
                                   const char *p) {
  if (*p == 0 || *(p+1) == 0)
    log_err("unbalanced pattern");
  if (*s != *p) return NULL;
  else {
    int b = *p;
    int e = *(p+1);
    int cont = 1;
    while (++s < ms->src_end) {
      if (*s == e) {
        if (--cont == 0) return s+1;
      }
      else if (*s == b) cont++;
    }
  }
  return NULL;  /* string ends out of balance */
}


const char *max_expand (MatchState *ms, const char *s,
                                 const char *p, const char *ep) {
  int i = 0;  /* counts maximum expand for item */
  while ((s+i) < ms->src_end && singlematch(uchar(*(s+i)), p, ep))
    i++;
  /* keeps trying to match with the maximum repetitions */
  while (i>=0) {
    const char *res = match(ms, (s+i), ep+1);
    if (res) return res;
    i--;  /* else didn't match; reduce 1 repetition to try again */
  }
  return NULL;
}


const char *min_expand (MatchState *ms, const char *s,
                                 const char *p, const char *ep) {
  for (;;) {
    const char *res = match(ms, s, ep+1);
    if (res != NULL)
      return res;
    else if (s<ms->src_end && singlematch(uchar(*s), p, ep))
      s++;  /* try with one more repetition */
    else return NULL;
  }
}


const char *pattern_match(const char *s, size_t len, const char *p)
{
    MatchState ms = {.src_init = s, .src_end = s + len};
    return match(&ms, s, p);
}


const char *bstring_match(bstring s, bstring pattern)
{
    MatchState ms = {.src_init = bdata(s), .src_end = bdata(s) + blength(s)};
    return match(&ms, bdata(s), bdata(pattern));
}

const char *match(MatchState *ms, const char *s, const char *p) {
    //TODO: get it to understand \ to skip pattern chars
    
  init: /* using goto's to optimize tail recursion */
  switch (*p) {
    case '(':
        p++;
        goto init;
    case ')':
        p++;
        goto init;
    case L_ESC: {
      switch (*(p+1)) {
        case 'b': {  /* balanced string? */
          s = matchbalance(ms, s, p+2);
          if (s == NULL) return NULL;
          p+=4; goto init;  /* else return match(ms, s, p+4); */
        }
        case 'f': {  /* frontier? */
          const char *ep; char previous;
          p += 2;
          if (*p != '[')
            log_err("missing '[' after \\\\f in pattern");
          ep = classend(ms, p);  /* points to what is next */
          previous = (s == ms->src_init) ? '\0' : *(s-1);
          if (matchbracketclass(uchar(previous), p, ep-1) ||
             !matchbracketclass(uchar(*s), p, ep-1)) return NULL;
          p=ep; goto init;  /* else return match(ms, s, ep); */
        }
        default: {
          goto dflt;  /* case default */
        }
      }
    }
    case '\0': {  /* end of pattern */
      return s;  /* match succeeded */
    }
    case '$': {
      if (*(p+1) == '\0')  /* is the `$' the last char in pattern? */
        return (s == ms->src_end) ? s : NULL;  /* check end of string */
      else goto dflt;
    }
    default: dflt: {  /* it is a pattern item */
      const char *ep = classend(ms, p);  /* points to what is next */
      int m = s<ms->src_end && singlematch(uchar(*s), p, ep);
      switch (*ep) {
        case '?': {  /* optional */
          const char *res;
          if (m && ((res=match(ms, s+1, ep+1)) != NULL))
            return res;
          p=ep+1; goto init;  /* else return match(ms, s, ep+1); */
        }
        case '*': {  /* 0 or more repetitions */
          return max_expand(ms, s, p, ep);
        }
        case '+': {  /* 1 or more repetitions */
          return (m ? max_expand(ms, s+1, p, ep) : NULL);
        }
        case '-': {  /* 0 or more repetitions (minimum) */
          return min_expand(ms, s, p, ep);
        }
        default: {
          if (!m) return NULL;
          s++; p=ep; goto init;  /* else return match(ms, s+1, ep); */
        }
      }
    }
  }
}

