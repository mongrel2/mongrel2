#ifndef __unused_h__
#define __unused_h__

#if defined(__GNUC__) || defined(__clang__)
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif

#endif
