#undef NDEBUG
#include <stdio.h>
#include <bstr/bstrlib.h>
#include <bstr/bstraux.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <ctype.h>
#include <dbg.h>

static int tWrite (const void * buf, size_t elsize, size_t nelem, void * parm) {
    bstring b = (bstring) parm;
    size_t i;

    if (NULL == b || NULL == buf || 0 == elsize || 0 == nelem)
        return -__LINE__;

    for (i=0; i < nelem; i++) {
        if (0 > bcatblk (b, buf, elsize)) break;
        buf = (const void *) (elsize + (const char *) buf);
    }
    return (int) i;	
}

int testaux0 (void) {
    struct bwriteStream * ws;
    bstring s;
    int ret = 0;

    debug ("TEST: struct bwriteStream functions.");

    ws = bwsOpen ((bNwrite) tWrite, (s = bfromcstr ("")));
    bwsBuffLength (ws, 8);
    ret += 8 != bwsBuffLength (ws, 0);
    bwsWriteBlk (ws, bsStaticBlkParms ("Hello "));
    ret += 0 == biseqcstr (s, "");
    bwsWriteBlk (ws, bsStaticBlkParms ("World"));
    ret += 0 == biseqcstr (s, "Hello Wo");
    ret += s != bwsClose (ws);
    ret += 0 == biseqcstr (s, "Hello World");

    bdestroy(s);

    debug ("\t# failures: %d", ret);

    return ret;
}

int testaux1 (void) {
    struct tagbstring t = bsStatic ("Hello world");
    bstring b, c, d;
    int ret = 0;

    debug ("TEST: bTail and bHead functions.");
    b = bTail (&t, 5);
    c = bHead (&t, 5);
    ret += 0 >= biseqcstr (b, "world");
    ret += 0 >= biseqcstr (c, "Hello");
    bdestroy (b);
    bdestroy (c);

    b = bTail (&t, 0);
    c = bHead (&t, 0);
    ret += 0 >= biseqcstr (b, "");
    ret += 0 >= biseqcstr (c, "");
    bdestroy (b);
    bdestroy (c);

    d = bstrcpy (&t);
    b = bTail (d, 5);
    c = bHead (d, 5);
    ret += 0 >= biseqcstr (b, "world");
    ret += 0 >= biseqcstr (c, "Hello");
    bdestroy (b);
    bdestroy (c);
    bdestroy (d);

    debug ("\t# failures: %d", ret);

    return ret;
}

int testaux2 (void) {
    struct tagbstring t = bsStatic ("Hello world");
    bstring b;
    int ret = 0, reto;

    debug ("TEST: bSetChar function.");
    ret += 0 <= bSetChar (&t, 4, ',');
    ret += 0 >  bSetChar (b = bstrcpy (&t), 4, ',');
    ret += 0 >= biseqcstr (b, "Hell, world");
    ret += 0 <= bSetChar (b, -1, 'x');
    b->slen = 2;
    ret += 0 >  bSetChar (b, 1, 'i');
    ret += 0 >= biseqcstr (b, "Hi");
    ret += 0 >  bSetChar (b, 2, 's');
    ret += 0 >= biseqcstr (b, "His");
    ret += 0 >  bSetChar (b, 1, '\0');
    ret += blength (b) != 3;
    ret += bchare (b, 0, '?') != 'H';
    ret += bchare (b, 1, '?') != '\0';
    ret += bchare (b, 2, '?') != 's';
    bdestroy (b);

    debug ("\t# failures: %d", ret);

    reto = ret;
    ret = 0;

    debug ("TEST: bSetCstrChar function.");
    ret += 0 <= bSetCstrChar (&t, 4, ',');
    ret += 0 >  bSetCstrChar (b = bstrcpy (&t), 4, ',');
    ret += 0 >= biseqcstr (b, "Hell, world");
    ret += 0 <= bSetCstrChar (b, -1, 'x');
    b->slen = 2;
    ret += 0 >  bSetCstrChar (b, 1, 'i');
    ret += 0 >= biseqcstr (b, "Hi");
    ret += 0 >  bSetCstrChar (b, 2, 's');
    ret += 0 >= biseqcstr (b, "His");
    ret += 0 >  bSetCstrChar (b, 1, '\0');
    ret += blength (b) != 1;
    ret += bchare (b, 0, '?') != 'H';
    bdestroy (b);

    debug ("\t# failures: %d", ret);

    return reto + ret;
}

int testaux3 (void) {
    struct tagbstring t = bsStatic ("Hello world");
    bstring b;
    int ret = 0;

    debug ("TEST: bFill function.");
    ret += 0 <= bFill (&t, 'x', 7);
    ret += 0 >  bFill (b = bstrcpy (&t), 'x', 7);
    ret += 0 >= biseqcstr (b, "xxxxxxx");
    ret += 0 <= bFill (b, 'x', -1);
    ret += 0 >  bFill (b, 'x', 0);
    ret += 0 >= biseqcstr (b, "");
    bdestroy (b);

    debug ("\t# failures: %d", ret);

    return ret;
}

int testaux4 (void) {
    struct tagbstring t = bsStatic ("foo");
    bstring b;
    int ret = 0;

    debug ("TEST: bReplicate function.");
    ret += 0 <= bReplicate (&t, 4);
    ret += 0 <= bReplicate (b = bstrcpy (&t), -1);
    ret += 0 >  bReplicate (b, 4);
    ret += 0 >= biseqcstr (b, "foofoofoofoo");
    ret += 0 >  bReplicate (b, 0);
    ret += 0 >= biseqcstr (b, "");
    bdestroy (b);

    debug ("\t# failures: %d", ret);

    return ret;
}

int testaux5 (void) {
    struct tagbstring t = bsStatic ("Hello world");
    bstring b;
    int ret = 0;

    debug ("TEST: bReverse function.");
    ret += 0 <= bReverse (&t);
    ret += 0 >  bReverse (b = bstrcpy (&t));
    ret += 0 >= biseqcstr (b, "dlrow olleH");
    b->slen = 0;
    ret += 0 >  bReverse (b);
    ret += 0 >= biseqcstr (b, "");
    bdestroy (b);

    debug ("\t# failures: %d", ret);

    return ret;
}

int testaux6 (void) {
    struct tagbstring t = bsStatic ("Hello world");
    bstring b;
    int ret = 0;

    debug ("TEST: bInsertChrs function.");
    ret += 0 <= bInsertChrs (&t, 6, 4, 'x', '?');
    ret += 0 >  bInsertChrs (b = bstrcpy (&t), 6, 4, 'x', '?');
    ret += 0 >= biseqcstr (b, "Hello xxxxworld");
    bdestroy (b);

    debug ("\t# failures: %d", ret);

    return ret;
}

int testaux7 (void) {
    struct tagbstring t = bsStatic ("  i am  ");
    bstring b;
    int ret = 0;

    debug ("TEST: bJustify functions.");
    ret += 0 <= bJustifyLeft (&t, ' ');
    ret += 0 <= bJustifyRight (&t, 8, ' ');
    ret += 0 <= bJustifyMargin (&t, 8, ' ');
    ret += 0 <= bJustifyCenter (&t, 8, ' ');
    ret += 0 >  bJustifyLeft (b = bstrcpy (&t), ' ');
    ret += 0 >= biseqcstr (b, "i am");
    ret += 0 >  bJustifyRight (b, 8, ' ');
    ret += 0 >= biseqcstr (b, "    i am");
    ret += 0 >  bJustifyMargin (b, 8, ' ');
    ret += 0 >= biseqcstr (b, "i     am");
    ret += 0 >  bJustifyCenter (b, 8, ' ');
    ret += 0 >= biseqcstr (b, "  i am");
    bdestroy (b);

    debug ("\t# failures: %d", ret);

    return ret;
}

int testaux8 (void) {
    struct tagbstring t = bsStatic ("Hello world");
    bstring b;
    char * c;
    int ret = 0;

    debug ("TEST: NetStr functions.");
    c = bStr2NetStr (&t);
    ret += 0 != strcmp (c, "11:Hello world,");
    b = bNetStr2Bstr (c);
    ret += 0 >= biseq (b, &t);
    bdestroy (b);
    bcstrfree (c);

    debug ("\t# failures: %d", ret);

    return ret;
}

int testaux9 (void) {
    struct tagbstring t = bsStatic ("Hello world");
    bstring b, c;
    int err, ret = 0;

    debug ("TEST: Base 64 codec.");

    b = bBase64Encode (&t);
    ret += 0 >= biseqcstr (b, "SGVsbG8gd29ybGQ=");
    c = bBase64DecodeEx (b, &err);
    ret += 0 != err;
    ret += 0 >= biseq (c, &t);
    bdestroy (b);
    bdestroy (c);

    debug ("\t# failures: %d", ret);

    return ret;
}

int testaux10 (void) {
    struct tagbstring t = bsStatic ("Hello world");
    bstring b, c;
    int err, ret = 0;

    debug ("TEST: UU codec.");

    b = bUuEncode (&t);
    ret += 0 >= biseqcstr (b, "+2&5L;&\\@=V]R;&0`\r\n");
    c = bUuDecodeEx (b, &err);
    ret += 0 != err;
    ret += 0 >= biseq (c, &t);
    bdestroy (b);
    bdestroy (c);

    debug ("\t# failures: %d", ret);

    return ret;
}

int testaux11 (void) {
    struct tagbstring t = bsStatic ("Hello world");
    unsigned char Ytstr[] = {0x72, 0x8f, 0x96, 0x96, 0x99, 0x4a, 0xa1, 0x99, 0x9c, 0x96, 0x8e};
    bstring b, c;
    int ret = 0;

    debug ("TEST: Y codec.");

    b = bYEncode (&t);
    ret += 11 != b->slen;
    ret += 0 >= bisstemeqblk (b, Ytstr, 11);
    c = bYDecode (b);
    ret += 0 >= biseq (c, &t);
    bdestroy (b);
    bdestroy (c);

    debug ("\t# failures: %d", ret);

    return ret;
}

int testaux12 (void) {
    struct tagbstring t = bsStatic ("Hello world");
    struct bStream * s;
    bstring b;
    int ret = 0;

    debug ("TEST: bsFromBstr.");

    ret = bsread (b = bfromcstr (""), s = bsFromBstr (&t), 6);
    ret += 1 != biseqcstr (b, "Hello ");
    if (b) b->slen = 0;
    ret = bsread (b, s, 6);
    ret += 1 != biseqcstr (b, "world");

    bdestroy (b);
    bsclose (s);

    debug ("\t# failures: %d", ret);

    return ret;
}

struct vfgetc {
    int ofs;
    bstring base;
};


/*
 * This source file is part of the bstring string library.  This code was
 * written by Paul Hsieh in 2002-2010, and is covered by either the 3-clause 
 * BSD open source license or GPL v2.0. Refer to the accompanying documentation 
 * for details on usage and license.
 */


static bstring dumpOut[16];
static int rot = 0;

static char * dumpBstring (const struct tagbstring * b) {
    rot = (rot + 1) % (unsigned)16;
    if (dumpOut[rot] == NULL) {
        dumpOut[rot] = bfromcstr ("");
        if (dumpOut[rot] == NULL) return "FATAL INTERNAL ERROR";
    }
    dumpOut[rot]->slen = 0;
    if (b == NULL) {
        bcatcstr (dumpOut[rot], "NULL");
    } else {
        static char msg[256];
        sprintf(msg, "%p", (void *)b);
        bcatcstr (dumpOut[rot], msg);

        if (b->slen < 0) {
            sprintf (msg, ":[err:slen=%d<0]", b->slen);
            bcatcstr (dumpOut[rot], msg);
        } else {
            if (b->mlen > 0 && b->mlen < b->slen) {
                sprintf (msg, ":[err:mlen=%d<slen=%d]", b->mlen, b->slen);
                bcatcstr (dumpOut[rot], msg);
            } else {
                if (b->mlen == -1) {
                    bcatcstr (dumpOut[rot], "[p]");
                } else if (b->mlen < 0) {
                    bcatcstr (dumpOut[rot], "[c]");
                }
                bcatcstr (dumpOut[rot], ":");
                if (b->data == NULL) {
                    bcatcstr (dumpOut[rot], "[err:data=NULL]");
                } else {
                    bcatcstr (dumpOut[rot], "\"");
                    bcatcstr (dumpOut[rot], (const char *) b->data);
                    bcatcstr (dumpOut[rot], "\"");
                }
            }
        }
    }
    return (char *) dumpOut[rot]->data;
}

static int test0_0 (const char * s, const char * res) {
    bstring b0 = bfromcstr (s);
    int ret = 0;

    if (s == NULL) {
        if (res != NULL) ret++;
        debug (".\tbfromcstr (NULL) = %s", dumpBstring (b0));
        return ret;
    }

    ret += (res == NULL) || ((int) strlen (res) != b0->slen)
        || (0 != memcmp (res, b0->data, b0->slen));
    ret += b0->data[b0->slen] != '\0';

    debug (".\tbfromcstr (\"%s\") = %s", s, dumpBstring (b0));
    bdestroy (b0);
    return ret;
}

static int test0_1 (const char * s, int len, const char * res) {
    bstring b0 = bfromcstralloc (len, s);
    int ret = 0;

    if (s == NULL) {
        if (res != NULL) ret++;
        debug (".\tbfromcstralloc (*, NULL) = %s", dumpBstring (b0));
        return ret;
    }

    ret += (res == NULL) || ((int) strlen (res) != b0->slen)
        || (0 != memcmp (res, b0->data, b0->slen));
    ret += b0->data[b0->slen] != '\0';
    ret += len > b0->mlen;

    debug (".\tbfromcstralloc (%d, \"%s\") = %s", len, s, dumpBstring (b0));
    bdestroy (b0);
    return ret;
}

#define EMPTY_STRING ""
#define SHORT_STRING "bogus"
#define LONG_STRING  "This is a bogus but reasonably long string.  Just long enough to cause some mallocing."

static int test0 (void) {
    int ret = 0;

    debug ("TEST: bstring bfromcstr (const char * str);");

    /* tests with NULL */
    ret += test0_0 (NULL, NULL);

    /* normal operation tests */
    ret += test0_0 (EMPTY_STRING, EMPTY_STRING);
    ret += test0_0 (SHORT_STRING, SHORT_STRING);
    ret += test0_0 (LONG_STRING, LONG_STRING);
    debug ("\t# failures: %d", ret);

    debug ("TEST: bstring bfromcstralloc (int len, const char * str);");

    /* tests with NULL */
    ret += test0_1 (NULL,  0, NULL);
    ret += test0_1 (NULL, 30, NULL);

    /* normal operation tests */
    ret += test0_1 (EMPTY_STRING,  0, EMPTY_STRING);
    ret += test0_1 (EMPTY_STRING, 30, EMPTY_STRING);
    ret += test0_1 (SHORT_STRING,  0, SHORT_STRING);
    ret += test0_1 (SHORT_STRING, 30, SHORT_STRING);
    ret += test0_1 ( LONG_STRING,  0,  LONG_STRING);
    ret += test0_1 ( LONG_STRING, 30,  LONG_STRING);
    debug ("\t# failures: %d", ret);

    return ret;
}

static int test1_0 (const void * blk, int len, const char * res) {
    bstring b0 = blk2bstr (blk, len);
    int ret = 0;
    if (b0 == NULL) {
        if (res != NULL) ret++;
        debug (".\tblk2bstr (NULL, len=%d) = %s", len, dumpBstring (b0));
    } else {
        ret += (res == NULL) || (len != b0->slen)
            || (0 != memcmp (res, b0->data, len));
        ret += b0->data[b0->slen] != '\0';
        debug (".\tblk2bstr (blk=%p, len=%d) = %s", blk, len, dumpBstring (b0));
    }

    if (ret) {
        debug ("\t\tfailure(%d) = %d (res = %p", __LINE__, ret, res);
        if (res) debug (" = \"%s\"", res);
        debug (")");
    }
    bdestroy (b0);
    return ret;
}

static int test1 (void) {
    int ret = 0;

    debug ("TEST: bstring blk2bstr (const void * blk, int len);");

    /* tests with NULL */
    ret += test1_0 (NULL, 10, NULL);
    ret += test1_0 (NULL, 0, NULL);
    ret += test1_0 (NULL, -1, NULL);

    /* normal operation tests */
    ret += test1_0 (SHORT_STRING, sizeof (SHORT_STRING)-1, SHORT_STRING);
    ret += test1_0 (LONG_STRING, sizeof (LONG_STRING)-1, LONG_STRING);
    ret += test1_0 (LONG_STRING, 5, "This ");
    ret += test1_0 (LONG_STRING, 0, "");
    ret += test1_0 (LONG_STRING, -1, NULL);
    debug ("\t# failures: %d", ret);
    return ret;
}

static int test2_0 (const_bstring b, char z, const unsigned char * res) {
    char * s = bstr2cstr (b, z);
    int ret = 0;
    if (s == NULL) {
        if (res != NULL) ret++;
        debug (".\tbstr2cstr (%s, %02X) = NULL", dumpBstring (b), z);
        bcstrfree(s);
        return ret;
    }

    if (res == NULL) ret++;
    else {
        if (z != '\0') if ((int) strlen (s) != b->slen) ret++;
        if (!ret) {
            ret += (0 != memcmp (res, b->data, b->slen));
        }
    }

    debug (".\tbstr2cstr (%s, %02X) = \"%s\"", dumpBstring (b), z, s);
    bcstrfree (s);
    return ret;
}

struct tagbstring emptyBstring = bsStatic ("");
struct tagbstring shortBstring = bsStatic ("bogus");
struct tagbstring longBstring  = bsStatic ("This is a bogus but reasonably long string.  Just long enough to cause some mallocing.");

struct tagbstring badBstring1 = {8,  4, NULL};
struct tagbstring badBstring2 = {2, -5, (unsigned char *) "bogus"};
struct tagbstring badBstring3 = {2,  5, (unsigned char *) "bogus"};

struct tagbstring xxxxxBstring = bsStatic ("xxxxx");

static int test2 (void) {
    int ret = 0;

    debug ("TEST: char * bstr2cstr (const_bstring s, char z);");

    /* tests with NULL */
    ret += test2_0 (NULL, (char) '?', NULL);

    /* normal operation tests */
    ret += test2_0 (&emptyBstring, (char) '?', emptyBstring.data);
    ret += test2_0 (&shortBstring, (char) '?', shortBstring.data);
    ret += test2_0 (&longBstring, (char) '?', longBstring.data);
    ret += test2_0 (&badBstring1, (char) '?', NULL);
    ret += test2_0 (&badBstring2, (char) '?', NULL);
    debug ("\t# failures: %d", ret);
    return ret;
}

static int test3_0 (const_bstring b) {
    bstring b0 = bstrcpy (b);
    int ret = 0;
    debug (".\tbstrcpy (%s) = %s", dumpBstring (b), dumpBstring (b0));
    if (b0 == NULL) {
        if (b != NULL && b->data != NULL && b->slen >= 0) ret++;
    } else {
        ret += (b == NULL) || (b->slen != b0->slen)
            || (0 != memcmp (b->data, b0->data, b->slen));
        ret += b0->data[b0->slen] != '\0';
    }
    bdestroy (b0);
    return ret;
}

static int test3 (void) {
    int ret = 0;

    debug ("TEST: bstring bstrcpy (const_bstring b1);");

    /* tests with NULL to make sure that there is NULL propogation */
    ret += test3_0 (NULL);
    ret += test3_0 (&badBstring1);
    ret += test3_0 (&badBstring2);

    /* normal operation tests */
    ret += test3_0 (&emptyBstring);
    ret += test3_0 (&shortBstring);
    ret += test3_0 (&longBstring);
    debug ("\t# failures: %d", ret);
    return ret;
}

static int test4_0 (const_bstring b, int left, int len, const char * res) {
    bstring b0 = bmidstr (b, left, len);
    int ret = 0;
    debug (".\tbmidstr (%s, %d, %d) = %s", dumpBstring (b), left, len, dumpBstring (b0));
    if (b0 == NULL) {
        if (b != NULL && b->data != NULL && b->slen >= 0 && len >= 0) ret++;
    } else {
        ret += (b == NULL) || (res == NULL) || (b0->slen > len && len >= 0)
            || (b0->slen != (int) strlen (res))
            || (b0->slen > 0 && 0 != memcmp (res, b0->data, b0->slen));
        ret += b0->data[b0->slen] != '\0';
    }
    if (ret) {
        debug ("(b == NULL)                  = %d", (b == NULL));
        debug ("(res == NULL)                = %d", (res == NULL));
        debug ("(b0->slen > len && len >= 0) = %d", (b0->slen > len && len >= 0));
        if (res) debug ("(b0->slen != strlen (res))   = %d", (b0->slen != (int) strlen (res)));
        debug ("(b0->slen > 0 && 0 != memcmp (res, b0->data, b0->slen) = %d", (b0->slen > 0 && 0 != memcmp (res, b0->data, b0->slen)));

        debug ("\t\tfailure(%d) = %d (res = %p", __LINE__, ret, res);
        if (res) debug (" = \"%s\"", res);
        debug (")");
    }
    bdestroy (b0);
    return ret;
}

static int test4 (void) {
    int ret = 0;

    debug ("TEST: bstring bmidstr (const_bstring b, int left, int len);");

    /* tests with NULL to make sure that there is NULL propogation */
    ret += test4_0 (NULL,  0,  0, NULL);
    ret += test4_0 (NULL,  0,  2, NULL);
    ret += test4_0 (NULL,  0, -2, NULL);
    ret += test4_0 (NULL, -5,  2, NULL);
    ret += test4_0 (NULL, -5, -2, NULL);
    ret += test4_0 (&badBstring1, 1, 3, NULL);
    ret += test4_0 (&badBstring2, 1, 3, NULL);

    /* normal operation tests on all sorts of subranges */
    ret += test4_0 (&emptyBstring,  0,  0, "");
    ret += test4_0 (&emptyBstring,  0, -1, "");
    ret += test4_0 (&emptyBstring,  1,  3, "");
    ret += test4_0 (&shortBstring,  0,  0, "");
    ret += test4_0 (&shortBstring,  0, -1, "");
    ret += test4_0 (&shortBstring,  1,  3, "ogu");
    ret += test4_0 (&shortBstring, -1,  3, "bo");
    ret += test4_0 (&shortBstring, -1,  9, "bogus");
    ret += test4_0 (&shortBstring,  3, -1, "");
    ret += test4_0 (&shortBstring,  9,  3, "");
    debug ("\t# failures: %d", ret);
    return ret;
}

static int test5_0 (bstring b0, const_bstring b1, const char * res) {
    bstring b2;
    int rv, ret = 0;

    if (b0 != NULL && b0->data != NULL && b0->slen >= 0 &&
            b1 != NULL && b1->data != NULL && b1->slen >= 0 ) {
        b2 = bstrcpy (b0);
        bwriteprotect (*b2);

        debug (".\tbconcat (%s, ", dumpBstring (b2));

        rv = bconcat (b2, b1);
        ret += (rv == 0);
        if (!biseq (b0, b2)) ret++;

        debug ("%s) = %s", dumpBstring (b1), dumpBstring (b2));

        bwriteallow (*b2);

        debug (".\tbconcat (%s, ", dumpBstring (b2));

        rv = bconcat (b2, b1);

        debug ("%s) = %s", dumpBstring (b1), dumpBstring (b2));

        if (b1) ret += (b2->slen != b0->slen + b1->slen);
        ret += ((0 != rv) && (b1 != NULL)) || ((0 == rv) && (b1 == NULL));
        ret += (res == NULL) || ((int) strlen (res) > b2->slen) 
            || (0 != memcmp (b2->data, res, b2->slen));
        ret += b2->data[b2->slen] != '\0';
        bdestroy (b2);
    } else {
        ret += (BSTR_ERR != (rv = bconcat (b0, b1)));
        debug (".\tbconcat (%s, %s) = %d", dumpBstring (b0), dumpBstring (b1), rv);
    }

    if (ret) {
        debug ("\t\tfailure(%d) = %d (res = %p", __LINE__, ret, res);
        if (res) debug (" = \"%s\"", res);
        debug (")");
    }
    return ret;
}

static int test5_1 (void) {
    bstring b, c;
    struct tagbstring t;
    int i, ret;

    debug ("TEST: bconcat aliasing");
    for (ret=i=0; i < longBstring.slen; i++) {
        b = bstrcpy (&longBstring);
        c = bstrcpy (&longBstring);
        bmid2tbstr (t, b, i, longBstring.slen);
        ret += 0 != bconcat (c, &t);
        ret += 0 != bconcat (b, &t);
        ret += !biseq (b, c);
        bdestroy (b);
        bdestroy (c);
    }

    b = bfromcstr ("abcd");
    c = bfromcstr ("abcd");

    for (ret=i=0; i < 100; i++) {
        bmid2tbstr (t, b, 0, 3);
        ret += 0 != bcatcstr (c, "abc");
        ret += 0 != bconcat (b, &t);
        ret += !biseq (b, c);
    }

    bdestroy (b);
    bdestroy (c);

    if (ret) {
        debug ("\t\talias failures(%d) = %d", __LINE__, ret);
    }

    return ret;
}

static int test5 (void) {
    int ret = 0;

    debug ("TEST: int bconcat (bstring b0, const_bstring b1);");

    /* tests with NULL */
    ret += test5_0 (NULL, NULL, NULL);
    ret += test5_0 (NULL, &emptyBstring, NULL);
    ret += test5_0 (&emptyBstring, NULL, "");
    ret += test5_0 (&emptyBstring, &badBstring1, NULL);
    ret += test5_0 (&emptyBstring, &badBstring2, NULL);
    ret += test5_0 (&badBstring1, &emptyBstring, NULL);
    ret += test5_0 (&badBstring2, &emptyBstring, NULL);

    /* normal operation tests on all sorts of subranges */
    ret += test5_0 (&emptyBstring, &emptyBstring, "");
    ret += test5_0 (&emptyBstring, &shortBstring, "bogus");
    ret += test5_0 (&shortBstring, &emptyBstring, "bogus");
    ret += test5_0 (&shortBstring, &shortBstring, "bogusbogus");

    ret += test5_1 ();

    debug ("\t# failures: %d", ret);
    return ret;
}

static int test6_0 (bstring b, char c, const char * res) {
    bstring b0;
    int rv, ret = 0;

    if (b != NULL && b->data != NULL && b->slen >= 0) {
        b0 = bstrcpy (b);
        bwriteprotect (*b0);
        rv = bconchar (b0, c);
        ret += (rv == 0);
        if (!biseq (b0, b)) ret++;

        debug (".\tbconchar (%s, %c) = %s", dumpBstring (b), c, dumpBstring (b0));

        bwriteallow (*b0);
        rv = bconchar (b0, c);
        ret += (0 != rv);
        ret += (b0->slen != b->slen + 1);
        ret += (res == NULL) || ((int) strlen (res) > b0->slen) 
            || (0 != memcmp (b0->data, res, b0->slen));
        ret += b0->data[b0->slen] != '\0';
        debug (".\tbconchar (%s, %c) = %s", dumpBstring (b), c, dumpBstring (b0));

        bdestroy (b0);
    } else {
        ret += (BSTR_ERR != (rv = bconchar (b, c)));
        debug (".\tbconchar (%s, %c) = %d", dumpBstring (b), c, rv);
    }

    if (ret) {
        debug ("\t\tfailure(%d) = %d (res = %p", __LINE__, ret, res);
        if (res) debug (" = \"%s\"", res);
        debug (")");
    }
    return ret;
}

static int test6 (void) {
    int ret = 0;

    debug ("TEST: int bconchar (bstring b, char c);");

    /* tests with NULL */
    ret += test6_0 (NULL, (char) 'x', NULL);
    ret += test6_0 (&badBstring1, (char) 'x', NULL);
    ret += test6_0 (&badBstring2, (char) 'x', NULL);

    /* normal operation tests on all sorts of subranges */
    ret += test6_0 (&emptyBstring, (char) 'x', "x");
    ret += test6_0 (&shortBstring, (char) 'x', "bogusx");
    debug ("\t# failures: %d", ret);
    return ret;
}

static int test7x8_0 (char * fnname, int (* fnptr) (const struct tagbstring *, const struct tagbstring *), const struct tagbstring * b0, const struct tagbstring * b1, int res) {
    int rv, ret = 0;

    ret += (res != (rv = fnptr (b0, b1)));
    debug (".\t%s (%s, %s) = %d", fnname, dumpBstring (b0), dumpBstring (b1), rv);
    if (ret) {
        debug ("\t\tfailure(%d) = %d (res = %d)", __LINE__, ret, res);
    }
    return ret;
}

static int test7x8 (char * fnname, int (* fnptr) (const struct tagbstring *, const struct tagbstring *),
        int retFail, int retLT, int retGT, int retEQ) {
    int ret = 0;

    debug ("TEST: int %s (const_bstring b0, const_bstring b1);", fnname);

    /* tests with NULL */
    ret += test7x8_0 (fnname, fnptr, NULL, NULL, retFail);
    ret += test7x8_0 (fnname, fnptr, &emptyBstring, NULL, retFail);
    ret += test7x8_0 (fnname, fnptr, NULL, &emptyBstring, retFail);
    ret += test7x8_0 (fnname, fnptr, &shortBstring, NULL, retFail);
    ret += test7x8_0 (fnname, fnptr, NULL, &shortBstring, retFail);
    ret += test7x8_0 (fnname, fnptr, &badBstring1, &badBstring1, retFail);
    ret += test7x8_0 (fnname, fnptr, &badBstring2, &badBstring2, retFail);
    ret += test7x8_0 (fnname, fnptr, &shortBstring, &badBstring2, retFail);
    ret += test7x8_0 (fnname, fnptr, &badBstring2, &shortBstring, retFail);

    /* normal operation tests on all sorts of subranges */
    ret += test7x8_0 (fnname, fnptr, &emptyBstring, &emptyBstring, retEQ);
    ret += test7x8_0 (fnname, fnptr, &shortBstring, &emptyBstring, retGT);
    ret += test7x8_0 (fnname, fnptr, &emptyBstring, &shortBstring, retLT);
    ret += test7x8_0 (fnname, fnptr, &shortBstring, &shortBstring, retEQ);

    {
        bstring b = bstrcpy (&shortBstring);
        b->data[1]++;
        ret += test7x8_0 (fnname, fnptr, b, &shortBstring, retGT);
        bdestroy (b);
    }

    if (fnptr == biseq) {
        ret += test7x8_0 (fnname, fnptr, &shortBstring, &longBstring, retGT);
        ret += test7x8_0 (fnname, fnptr, &longBstring, &shortBstring, retLT);
    } else {
        ret += test7x8_0 (fnname, fnptr, &shortBstring, &longBstring, 'b'-'T');
        ret += test7x8_0 (fnname, fnptr, &longBstring, &shortBstring, 'T'-'b');
    }

    debug ("\t# failures: %d", ret);
    return ret;
}

#define test7() test7x8 ("biseq", biseq, -1, 0, 0, 1)
#define test8() test7x8 ("bstrcmp", bstrcmp, SHRT_MIN, -1, 1, 0)

static int test9_0 (const_bstring b0, const_bstring b1, int n, int res) {
    int rv, ret = 0;

    ret += (res != (rv = bstrncmp (b0, b1, n)));
    debug (".\tbstrncmp (%s, %s, %d) = %d", dumpBstring (b0), dumpBstring (b1), n, rv);
    if (ret) {
        debug ("\t\tfailure(%d) = %d (res = %d)", __LINE__, ret, res);
    }
    return ret;
}

static int test9 (void) {
    int ret = 0;

    debug ("TEST: int bstrncmp (const_bstring b0, const_bstring b1, int n);");

    /* tests with NULL */
    ret += test9_0 (NULL, NULL, 0, SHRT_MIN);
    ret += test9_0 (NULL, NULL, -1, SHRT_MIN);
    ret += test9_0 (NULL, NULL, 1, SHRT_MIN);
    ret += test9_0 (&emptyBstring, NULL, 0, SHRT_MIN);
    ret += test9_0 (NULL, &emptyBstring, 0, SHRT_MIN);
    ret += test9_0 (&emptyBstring, NULL, 1, SHRT_MIN);
    ret += test9_0 (NULL, &emptyBstring, 1, SHRT_MIN);
    ret += test9_0 (&badBstring1, &badBstring1, 1, SHRT_MIN);
    ret += test9_0 (&badBstring2, &badBstring2, 1, SHRT_MIN);
    ret += test9_0 (&emptyBstring, &badBstring1, 1, SHRT_MIN);
    ret += test9_0 (&emptyBstring, &badBstring2, 1, SHRT_MIN);
    ret += test9_0 (&badBstring1, &emptyBstring, 1, SHRT_MIN);
    ret += test9_0 (&badBstring2, &emptyBstring, 1, SHRT_MIN);

    /* normal operation tests on all sorts of subranges */
    ret += test9_0 (&emptyBstring, &emptyBstring, -1, 0);
    ret += test9_0 (&emptyBstring, &emptyBstring, 0, 0);
    ret += test9_0 (&emptyBstring, &emptyBstring, 1, 0);
    ret += test9_0 (&shortBstring, &shortBstring, -1, 0);
    ret += test9_0 (&shortBstring, &shortBstring, 0, 0);
    ret += test9_0 (&shortBstring, &shortBstring, 1, 0);
    ret += test9_0 (&shortBstring, &shortBstring, 9, 0);
    debug ("\t# failures: %d", ret);
    return ret;
}

static int test10_0 (bstring b, int res, int nochange) {
    struct tagbstring sb = bsStatic ("<NULL>");
    int rv, x, ret = 0;

    if (b) sb = *b;
    debug (".\tbdestroy (%s) = ", dumpBstring (b));
    rv = bdestroy (b);
    debug ("%d", rv);

    if (b != NULL) {
        if (rv >= 0) 
            /* If the bdestroy was successful we have to assume
               the contents were "changed" */
            x = 1;
        else 
            x = memcmp (&sb, b, sizeof sb);
    } else x = !nochange;
    ret += (rv != res);
    ret += (!nochange) == (!x);
    if (ret) {
        debug ("\t\tfailure(%d) res = %d nochange = %d, x = %d, sb.slen = %d, sb.mlen = %d, sb.data = %p", __LINE__, res, nochange, x, sb.slen, sb.mlen, sb.data);
    }
    return ret;
}

static int test10 (void) {
    bstring c = bstrcpy (&shortBstring);
    bstring b = bstrcpy (&emptyBstring);
    int ret = 0;

    debug ("TEST: int bdestroy (const_bstring b);");
    /* tests with NULL */
    ret += test10_0 (NULL, BSTR_ERR, 1);

    /* protected, constant and regular instantiations on empty or not */
    bwriteprotect (*b);
    bwriteprotect (*c);
    ret += test10_0 (b, BSTR_ERR, 1);
    ret += test10_0 (c, BSTR_ERR, 1);
    bwriteallow (*b);
    bwriteallow (*c);
    ret += test10_0 (b, BSTR_OK, 0);
    ret += test10_0 (c, BSTR_OK, 0);
    ret += test10_0 (&emptyBstring, BSTR_ERR, 1);
    bwriteallow (emptyBstring);
    ret += test10_0 (&emptyBstring, BSTR_ERR, 1);
    ret += test10_0 (&shortBstring, BSTR_ERR, 1);
    bwriteallow (emptyBstring);
    ret += test10_0 (&shortBstring, BSTR_ERR, 1);
    ret += test10_0 (&badBstring1, BSTR_ERR, 1);
    ret += test10_0 (&badBstring2, BSTR_ERR, 1);

    debug ("\t# failures: %d", ret);
    return ret;
}

static int test11_0 (bstring s1, int pos, const_bstring s2, int res) {
    int rv, ret = 0;

    debug (".\tbinstr (%s, %d, %s) = ", dumpBstring (s1), pos, dumpBstring (s2));
    rv = binstr (s1, pos, s2);
    debug ("%d", rv);
    ret += (rv != res);
    if (ret) {
        debug ("\t\tfailure(%d) res = %d", __LINE__, res);
    }
    return ret;
}

static int test11_1 (bstring s1, int pos, const_bstring s2, int res) {
    int rv, ret = 0;

    debug (".\tbinstrcaseless (%s, %d, %s) = ", dumpBstring (s1), pos, dumpBstring (s2));
    rv = binstrcaseless (s1, pos, s2);
    debug ("%d", rv);
    ret += (rv != res);
    if (ret) {
        debug ("\t\tfailure(%d) res = %d", __LINE__, res);
    }
    return ret;
}

static int test11 (void) {
    bstring b, c;
    int ret = 0;

    debug ("TEST: int binstr (const_bstring s1, int pos, const_bstring s2);");
    ret += test11_0 (NULL, 0, NULL, BSTR_ERR);
    ret += test11_0 (&emptyBstring, 0, NULL, BSTR_ERR);
    ret += test11_0 (NULL, 0, &emptyBstring, BSTR_ERR);
    ret += test11_0 (&emptyBstring, 0, &badBstring1, BSTR_ERR);
    ret += test11_0 (&emptyBstring, 0, &badBstring2, BSTR_ERR);
    ret += test11_0 (&badBstring1, 0, &emptyBstring, BSTR_ERR);
    ret += test11_0 (&badBstring2, 0, &emptyBstring, BSTR_ERR);
    ret += test11_0 (&badBstring1, 0, &badBstring2, BSTR_ERR);
    ret += test11_0 (&badBstring2, 0, &badBstring1, BSTR_ERR);

    ret += test11_0 (&emptyBstring, 0, &emptyBstring, 0);
    ret += test11_0 (&emptyBstring, 1, &emptyBstring, BSTR_ERR);
    ret += test11_0 (&shortBstring, 1, &shortBstring, BSTR_ERR);
    ret += test11_0 (&shortBstring, 5, &emptyBstring, 5);
    ret += test11_0 (&shortBstring, -1, &shortBstring, BSTR_ERR);
    ret += test11_0 (&shortBstring, 0, &shortBstring, 0);
    ret += test11_0 (&shortBstring, 0, b = bstrcpy (&shortBstring), 0);
    bdestroy (b);
    ret += test11_0 (&shortBstring, 0, b = bfromcstr ("BOGUS"), BSTR_ERR);
    bdestroy (b);
    ret += test11_0 (&longBstring, 0, &shortBstring, 10);
    ret += test11_0 (&longBstring, 20, &shortBstring, BSTR_ERR);

    ret += test11_0 (c = bfromcstr ("sssssssssap"), 0, b = bfromcstr ("sap"), 8);
    bdestroy (c);
    bdestroy (b);
    ret += test11_0 (c = bfromcstr ("sssssssssap"), 3, b = bfromcstr ("sap"), 8);
    bdestroy (c);
    bdestroy (b);
    ret += test11_0 (c = bfromcstr ("ssssssssssap"), 3, b = bfromcstr ("sap"), 9);
    bdestroy (c);
    bdestroy (b);
    ret += test11_0 (c = bfromcstr ("sssssssssap"), 0, b = bfromcstr ("s"), 0);
    bdestroy (c);
    bdestroy (b);
    ret += test11_0 (c = bfromcstr ("sssssssssap"), 3, b = bfromcstr ("s"), 3);
    bdestroy (c);
    bdestroy (b);
    ret += test11_0 (c = bfromcstr ("sssssssssap"), 0, b = bfromcstr ("a"), 9);
    bdestroy (c);
    bdestroy (b);
    ret += test11_0 (c = bfromcstr ("sssssssssap"), 5, b = bfromcstr ("a"), 9);
    bdestroy (c);
    bdestroy (b);
    ret += test11_0 (c = bfromcstr ("sasasasasap"), 0, b = bfromcstr ("sap"), 8);
    bdestroy (c);
    bdestroy (b);
    ret += test11_0 (c = bfromcstr ("ssasasasasap"), 0, b = bfromcstr ("sap"), 9);
    bdestroy (c);
    bdestroy (b);

    debug ("TEST: int binstrcaseless (const_bstring s1, int pos, const_bstring s2);");
    ret += test11_1 (NULL, 0, NULL, BSTR_ERR);
    ret += test11_1 (&emptyBstring, 0, NULL, BSTR_ERR);
    ret += test11_1 (NULL, 0, &emptyBstring, BSTR_ERR);
    ret += test11_1 (&emptyBstring, 0, &badBstring1, BSTR_ERR);
    ret += test11_1 (&emptyBstring, 0, &badBstring2, BSTR_ERR);
    ret += test11_1 (&badBstring1, 0, &emptyBstring, BSTR_ERR);
    ret += test11_1 (&badBstring2, 0, &emptyBstring, BSTR_ERR);
    ret += test11_1 (&badBstring1, 0, &badBstring2, BSTR_ERR);
    ret += test11_1 (&badBstring2, 0, &badBstring1, BSTR_ERR);

    ret += test11_1 (&emptyBstring, 0, &emptyBstring, 0);
    ret += test11_1 (&emptyBstring, 1, &emptyBstring, BSTR_ERR);
    ret += test11_1 (&shortBstring, 1, &shortBstring, BSTR_ERR);
    ret += test11_1 (&shortBstring, 5, &emptyBstring, 5);
    ret += test11_1 (&shortBstring, -1, &shortBstring, BSTR_ERR);
    ret += test11_1 (&shortBstring, 0, &shortBstring, 0);
    ret += test11_1 (&shortBstring, 0, b = bstrcpy (&shortBstring), 0);
    bdestroy (b);
    ret += test11_1 (&shortBstring, 0, b = bfromcstr ("BOGUS"), 0);
    bdestroy (b);
    ret += test11_1 (&longBstring, 0, &shortBstring, 10);
    ret += test11_1 (&longBstring, 20, &shortBstring, BSTR_ERR);

    debug ("\t# failures: %d", ret);
    return ret;
}

static int test12_0 (bstring s1, int pos, const_bstring s2, int res) {
    int rv, ret = 0;

    debug (".\tbinstrr (%s, %d, %s) = ", dumpBstring (s1), pos, dumpBstring (s2));
    rv = binstrr (s1, pos, s2);
    debug ("%d", rv);
    ret += (rv != res);
    if (ret) {
        debug ("\t\tfailure(%d) res = %d", __LINE__, res);
    }
    return ret;
}

static int test12_1 (bstring s1, int pos, const_bstring s2, int res) {
    int rv, ret = 0;

    debug (".\tbinstrrcaseless (%s, %d, %s) = ", dumpBstring (s1), pos, dumpBstring (s2));
    rv = binstrrcaseless (s1, pos, s2);
    debug ("%d", rv);
    ret += (rv != res);
    if (ret) {
        debug ("\t\tfailure(%d) res = %d", __LINE__, res);
    }
    return ret;
}

static int test12 (void) {
    bstring b;
    int ret = 0;

    debug ("TEST: int binstrr (const_bstring s1, int pos, const_bstring s2);");
    ret += test12_0 (NULL, 0, NULL, BSTR_ERR);
    ret += test12_0 (&emptyBstring, 0, NULL, BSTR_ERR);
    ret += test12_0 (NULL, 0, &emptyBstring, BSTR_ERR);
    ret += test12_0 (&emptyBstring, 0, &badBstring1, BSTR_ERR);
    ret += test12_0 (&emptyBstring, 0, &badBstring2, BSTR_ERR);
    ret += test12_0 (&badBstring1, 0, &emptyBstring, BSTR_ERR);
    ret += test12_0 (&badBstring2, 0, &emptyBstring, BSTR_ERR);
    ret += test12_0 (&badBstring1, 0, &badBstring2, BSTR_ERR);
    ret += test12_0 (&badBstring2, 0, &badBstring1, BSTR_ERR);

    ret += test12_0 (&emptyBstring, 0, &emptyBstring, 0);
    ret += test12_0 (&emptyBstring, 1, &emptyBstring, BSTR_ERR);
    ret += test12_0 (&shortBstring, 1, &shortBstring, 0);
    ret += test12_0 (&shortBstring, 5, &emptyBstring, 5);
    ret += test12_0 (&shortBstring, -1, &shortBstring, BSTR_ERR);
    ret += test12_0 (&shortBstring, 0, &shortBstring, 0);
    ret += test12_0 (&shortBstring, 0, b = bstrcpy (&shortBstring), 0);
    bdestroy (b);
    ret += test12_0 (&shortBstring, 0, b = bfromcstr ("BOGUS"), BSTR_ERR);
    bdestroy (b);
    ret += test12_0 (&longBstring, 0, &shortBstring, BSTR_ERR);
    ret += test12_0 (&longBstring, 20, &shortBstring, 10);

    debug ("TEST: int binstrrcaseless (const_bstring s1, int pos, const_bstring s2);");
    ret += test12_1 (NULL, 0, NULL, BSTR_ERR);
    ret += test12_1 (&emptyBstring, 0, NULL, BSTR_ERR);
    ret += test12_1 (NULL, 0, &emptyBstring, BSTR_ERR);
    ret += test12_1 (&emptyBstring, 0, &badBstring1, BSTR_ERR);
    ret += test12_1 (&emptyBstring, 0, &badBstring2, BSTR_ERR);
    ret += test12_1 (&badBstring1, 0, &emptyBstring, BSTR_ERR);
    ret += test12_1 (&badBstring2, 0, &emptyBstring, BSTR_ERR);
    ret += test12_1 (&badBstring1, 0, &badBstring2, BSTR_ERR);
    ret += test12_1 (&badBstring2, 0, &badBstring1, BSTR_ERR);

    ret += test12_1 (&emptyBstring, 0, &emptyBstring, 0);
    ret += test12_1 (&emptyBstring, 1, &emptyBstring, BSTR_ERR);
    ret += test12_1 (&shortBstring, 1, &shortBstring, 0);
    ret += test12_1 (&shortBstring, 5, &emptyBstring, 5);
    ret += test12_1 (&shortBstring, -1, &shortBstring, BSTR_ERR);
    ret += test12_1 (&shortBstring, 0, &shortBstring, 0);
    ret += test12_1 (&shortBstring, 0, b = bstrcpy (&shortBstring), 0);
    bdestroy (b);
    ret += test12_1 (&shortBstring, 0, b = bfromcstr ("BOGUS"), 0);
    bdestroy (b);
    ret += test12_1 (&longBstring, 0, &shortBstring, BSTR_ERR);
    ret += test12_1 (&longBstring, 20, &shortBstring, 10);

    debug ("\t# failures: %d", ret);
    return ret;
}

static int test13_0 (bstring s1, int pos, const_bstring s2, int res) {
    int rv, ret = 0;

    debug (".\tbinchr (%s, %d, %s) = ", dumpBstring (s1), pos, dumpBstring (s2));
    rv = binchr (s1, pos, s2);
    debug ("%d", rv);
    ret += (rv != res);
    if (ret) {
        debug ("\t\tfailure(%d) res = %d", __LINE__, res);
    }
    return ret;
}

static int test13 (void) {
    bstring b;
    int ret = 0;
    struct tagbstring multipleOs = bsStatic ("ooooo");

    debug ("TEST: int binchr (const_bstring s1, int pos, const_bstring s2);");
    ret += test13_0 (NULL, 0, NULL, BSTR_ERR);
    ret += test13_0 (&emptyBstring, 0, NULL, BSTR_ERR);
    ret += test13_0 (NULL, 0, &emptyBstring, BSTR_ERR);
    ret += test13_0 (&emptyBstring, 0, &badBstring1, BSTR_ERR);
    ret += test13_0 (&emptyBstring, 0, &badBstring2, BSTR_ERR);
    ret += test13_0 (&badBstring1, 0, &emptyBstring, BSTR_ERR);
    ret += test13_0 (&badBstring2, 0, &emptyBstring, BSTR_ERR);
    ret += test13_0 (&badBstring2, 0, &badBstring1, BSTR_ERR);
    ret += test13_0 (&badBstring1, 0, &badBstring2, BSTR_ERR);

    ret += test13_0 (&emptyBstring, 0, &emptyBstring, BSTR_ERR);
    ret += test13_0 (&shortBstring, 0, &emptyBstring, BSTR_ERR);
    ret += test13_0 (&shortBstring,  0, &shortBstring, 0);
    ret += test13_0 (&shortBstring,  0, &multipleOs, 1);
    ret += test13_0 (&shortBstring, 0, b = bstrcpy (&shortBstring), 0);
    bdestroy (b);
    ret += test13_0 (&shortBstring, -1, &shortBstring, BSTR_ERR);
    ret += test13_0 (&shortBstring, 10, &shortBstring, BSTR_ERR);
    ret += test13_0 (&shortBstring, 1, &shortBstring, 1);
    ret += test13_0 (&emptyBstring, 0, &shortBstring, BSTR_ERR);
    ret += test13_0 (&xxxxxBstring, 0, &shortBstring, BSTR_ERR);
    ret += test13_0 (&longBstring, 0, &shortBstring, 3);
    ret += test13_0 (&longBstring, 10, &shortBstring, 10);
    debug ("\t# failures: %d", ret);
    return ret;
}

static int test14_0 (bstring s1, int pos, const_bstring s2, int res) {
    int rv, ret = 0;

    debug (".\tbinchrr (%s, %d, %s) = ", dumpBstring (s1), pos, dumpBstring (s2));
    rv = binchrr (s1, pos, s2);
    debug ("%d", rv);
    ret += (rv != res);
    if (ret) {
        debug ("\t\tfailure(%d) res = %d", __LINE__, res);
    }
    return ret;
}

static int test14 (void) {
    bstring b;
    int ret = 0;

    debug ("TEST: int binchrr (const_bstring s1, int pos, const_bstring s2);");
    ret += test14_0 (NULL, 0, NULL, BSTR_ERR);
    ret += test14_0 (&emptyBstring, 0, NULL, BSTR_ERR);
    ret += test14_0 (NULL, 0, &emptyBstring, BSTR_ERR);
    ret += test14_0 (&emptyBstring, 0, &emptyBstring, BSTR_ERR);
    ret += test14_0 (&shortBstring, 0, &emptyBstring, BSTR_ERR);
    ret += test14_0 (&emptyBstring, 0, &badBstring1, BSTR_ERR);
    ret += test14_0 (&emptyBstring, 0, &badBstring2, BSTR_ERR);
    ret += test14_0 (&badBstring1, 0, &emptyBstring, BSTR_ERR);
    ret += test14_0 (&badBstring2, 0, &emptyBstring, BSTR_ERR);
    ret += test14_0 (&badBstring2, 0, &badBstring1, BSTR_ERR);
    ret += test14_0 (&badBstring1, 0, &badBstring2, BSTR_ERR);

    ret += test14_0 (&shortBstring,  0, &shortBstring, 0);
    ret += test14_0 (&shortBstring, 0, b = bstrcpy (&shortBstring), 0);
    bdestroy (b);
    ret += test14_0 (&shortBstring, -1, &shortBstring, BSTR_ERR);
    ret += test14_0 (&shortBstring, 5, &shortBstring, 4);
    ret += test14_0 (&shortBstring, 4, &shortBstring, 4);
    ret += test14_0 (&shortBstring, 1, &shortBstring, 1);
    ret += test14_0 (&emptyBstring, 0, &shortBstring, BSTR_ERR);
    ret += test14_0 (&xxxxxBstring, 4, &shortBstring, BSTR_ERR);
    ret += test14_0 (&longBstring, 0, &shortBstring, BSTR_ERR);
    ret += test14_0 (&longBstring, 10, &shortBstring, 10);
    debug ("\t# failures: %d", ret);
    return ret;
}

static int test15_0 (bstring b0, int pos, const_bstring b1, unsigned char fill, char * res) {
    bstring b2;
    int rv, ret = 0, linenum = 0;

    if (b0 != NULL && b0->data != NULL && b0->slen >= 0 &&
            b1 != NULL && b1->data != NULL && b1->slen >= 0) {
        b2 = bstrcpy (b0);
        bwriteprotect (*b2);

        debug (".\tbsetstr (%s, ", dumpBstring (b2));

        rv = bsetstr (b2, pos, b1, fill);
        ret += (rv == 0); if (ret && 0 == linenum) linenum = __LINE__;
        if (!biseq (b0, b2)) ret++; if (ret && 0 == linenum) linenum = __LINE__;

        debug ("%d, %s, %02X) = %s", pos, dumpBstring (b1), fill, dumpBstring (b2));

        bwriteallow (*b2);

        debug (".\tbsetstr (%s, ", dumpBstring (b2));

        rv = bsetstr (b2, pos, b1, fill);
        if (b1) {
            ret += (pos >= 0) && (b2->slen != b0->slen + b1->slen) && (b2->slen != pos + b1->slen); if (ret && 0 == linenum) linenum = __LINE__;
            ret += (pos <  0) && (b2->slen != b0->slen); if (ret && 0 == linenum) linenum = __LINE__;
        }

        ret += ((rv == 0) != (pos >= 0)); if (ret && 0 == linenum) linenum = __LINE__;
        ret += (res == NULL); if (ret && 0 == linenum) linenum = __LINE__;
        ret += ((int) strlen (res) > b2->slen); if (ret && 0 == linenum) linenum = __LINE__;
        ret += (0 != memcmp (b2->data, res, b2->slen)); if (ret && 0 == linenum) linenum = __LINE__;
        ret += b2->data[b2->slen] != '\0'; if (ret && 0 == linenum) linenum = __LINE__;

        debug ("%d, %s, %02X) = %s", pos, dumpBstring (b1), fill, dumpBstring (b2));

        bdestroy (b2);
    } else {
        ret += (BSTR_ERR != (rv = bsetstr (b0, pos, b1, fill))); if (ret && 0 == linenum) linenum = __LINE__;
        debug (".\tbsetstr (%s, %d, %s, %02X) = %d", dumpBstring (b0), pos, dumpBstring (b1), fill, rv);
    }

    if (ret) {
        debug ("\t\tfailure(%d) = %d (res = %p", linenum, ret, res);
        if (res) debug (" = \"%s\"", res);
        debug (")");
    }
    return ret;
}

static int test15 (void) {
    int ret = 0;
    debug ("TEST: int bsetstr (bstring b0, int pos, const_bstring b1, unsigned char fill);");
    /* tests with NULL */
    ret += test15_0 (NULL, 0, NULL, (unsigned char) '?', NULL);
    ret += test15_0 (NULL, 0, &emptyBstring, (unsigned char) '?', NULL);
    ret += test15_0 (&badBstring1, 0, NULL, (unsigned char) '?', NULL);
    ret += test15_0 (&badBstring1, 0, &badBstring1, (unsigned char) '?', NULL);
    ret += test15_0 (&emptyBstring, 0, &badBstring1, (unsigned char) '?', NULL);
    ret += test15_0 (&badBstring1, 0, &emptyBstring, (unsigned char) '?', NULL);
    ret += test15_0 (&badBstring2, 0, NULL, (unsigned char) '?', NULL);
    ret += test15_0 (&badBstring2, 0, &badBstring2, (unsigned char) '?', NULL);
    ret += test15_0 (&emptyBstring, 0, &badBstring2, (unsigned char) '?', NULL);
    ret += test15_0 (&badBstring2, 0, &emptyBstring, (unsigned char) '?', NULL);

    /* normal operation tests */
    ret += test15_0 (&emptyBstring,  0, &emptyBstring, (unsigned char) '?', "");
    ret += test15_0 (&emptyBstring,  5, &emptyBstring, (unsigned char) '?', "?????");
    ret += test15_0 (&emptyBstring,  5, &shortBstring, (unsigned char) '?', "?????bogus");
    ret += test15_0 (&shortBstring,  0, &emptyBstring, (unsigned char) '?', "bogus");
    ret += test15_0 (&emptyBstring,  0, &shortBstring, (unsigned char) '?', "bogus");
    ret += test15_0 (&shortBstring,  0, &shortBstring, (unsigned char) '?', "bogus");
    ret += test15_0 (&shortBstring, -1, &shortBstring, (unsigned char) '?', "bogus");
    ret += test15_0 (&shortBstring,  2, &shortBstring, (unsigned char) '?', "bobogus");
    ret += test15_0 (&shortBstring,  6, &shortBstring, (unsigned char) '?', "bogus?bogus");
    ret += test15_0 (&shortBstring,  6, NULL,          (unsigned char) '?', "bogus?");
    debug ("\t# failures: %d", ret);
    return ret;
}

static int test16_0 (bstring b0, int pos, const_bstring b1, unsigned char fill, char * res) {
    bstring b2;
    int rv, ret = 0;

    if (b0 != NULL && b0->data != NULL && b0->slen >= 0 &&
            b1 != NULL && b1->data != NULL && b1->slen >= 0) {
        b2 = bstrcpy (b0);
        bwriteprotect (*b2);

        debug (".\tbinsert (%s, ", dumpBstring (b2));

        rv = binsert (b2, pos, b1, fill);
        ret += (rv == 0);
        if (!biseq (b0, b2)) ret++;

        debug ("%d, %s, %02X) = %s", pos, dumpBstring (b1), fill, dumpBstring (b2));

        bwriteallow (*b2);

        debug (".\tbinsert (%s, ", dumpBstring (b2));

        rv = binsert (b2, pos, b1, fill);
        if (b1) {
            ret += (pos >= 0) && (b2->slen != b0->slen + b1->slen) && (b2->slen != pos + b1->slen);
            ret += (pos < 0) && (b2->slen != b0->slen);
            ret += ((rv == 0) != (pos >= 0 && pos <= b2->slen));
        }

        ret += (res == NULL) || ((int) strlen (res) > b2->slen)
            || (0 != memcmp (b2->data, res, b2->slen));
        ret += b2->data[b2->slen] != '\0';

        debug ("%d, %s, %02X) = %s", pos, dumpBstring (b1), fill, dumpBstring (b2));

        bdestroy (b2);
    } else {
        ret += (BSTR_ERR != (rv = binsert (b0, pos, b1, fill)));
        debug (".\tbinsert (%s, %d, %s, %02X) = %d", dumpBstring (b0), pos, dumpBstring (b1), fill, rv);
    }

    if (ret) {
        debug ("\t\tfailure(%d) = %d (res = %p", __LINE__, ret, res);
        if (res) debug (" = \"%s\"", res);
        debug (")");
    }
    return ret;
}

static int test16 (void) {
    int ret = 0;
    debug ("TEST: int binsert (bstring b0, int pos, const_bstring b1, unsigned char fill);");
    /* tests with NULL */
    ret += test16_0 (NULL, 0, NULL, (unsigned char) '?', NULL);
    ret += test16_0 (NULL, 0, &emptyBstring, (unsigned char) '?', NULL);
    ret += test16_0 (&badBstring1, 0, NULL, (unsigned char) '?', NULL);
    ret += test16_0 (&badBstring1, 0, &badBstring1, (unsigned char) '?', NULL);
    ret += test16_0 (&emptyBstring, 0, &badBstring1, (unsigned char) '?', NULL);
    ret += test16_0 (&badBstring1, 0, &emptyBstring, (unsigned char) '?', NULL);
    ret += test16_0 (&badBstring2, 0, NULL, (unsigned char) '?', NULL);
    ret += test16_0 (&badBstring2, 0, &badBstring2, (unsigned char) '?', NULL);
    ret += test16_0 (&emptyBstring, 0, &badBstring2, (unsigned char) '?', NULL);
    ret += test16_0 (&badBstring2, 0, &emptyBstring, (unsigned char) '?', NULL);

    /* normal operation tests */
    ret += test16_0 (&emptyBstring,  0, &emptyBstring, (unsigned char) '?', "");
    ret += test16_0 (&emptyBstring,  5, &emptyBstring, (unsigned char) '?', "?????");
    ret += test16_0 (&emptyBstring,  5, &shortBstring, (unsigned char) '?', "?????bogus");
    ret += test16_0 (&shortBstring,  0, &emptyBstring, (unsigned char) '?', "bogus");
    ret += test16_0 (&emptyBstring,  0, &shortBstring, (unsigned char) '?', "bogus");
    ret += test16_0 (&shortBstring,  0, &shortBstring, (unsigned char) '?', "bogusbogus");
    ret += test16_0 (&shortBstring, -1, &shortBstring, (unsigned char) '?', "bogus");
    ret += test16_0 (&shortBstring,  2, &shortBstring, (unsigned char) '?', "bobogusgus");
    ret += test16_0 (&shortBstring,  6, &shortBstring, (unsigned char) '?', "bogus?bogus");
    ret += test16_0 (&shortBstring,  6, NULL,          (unsigned char) '?', "bogus");
    debug ("\t# failures: %d", ret);
    return ret;
}

static int test17_0 (bstring s1, int pos, int len, char * res) {
    bstring b2;
    int rv, ret = 0;

    if (s1 != NULL && s1->data != NULL && s1->slen >= 0) {
        b2 = bstrcpy (s1);
        bwriteprotect (*b2);

        debug (".\tbdelete (%s, ", dumpBstring (b2));

        rv = bdelete (b2, pos, len);
        ret += (rv == 0);
        if (!biseq (s1, b2)) ret++;

        debug ("%d, %d) = %s", pos, len, dumpBstring (b2));

        bwriteallow (*b2);

        debug (".\tbdelete (%s, ", dumpBstring (b2));

        rv = bdelete (b2, pos, len);
        ret += (len >= 0) != (rv == 0);
        ret += (b2->slen > s1->slen) || (b2->slen < pos && s1->slen >= pos);

        ret += (res == NULL) || ((int) strlen (res) > b2->slen)
            || (0 != memcmp (b2->data, res, b2->slen));
        ret += b2->data[b2->slen] != '\0';

        debug ("%d, %d) = %s", pos, len, dumpBstring (b2));

        bdestroy (b2);
    } else {
        ret += (BSTR_ERR != (rv = bdelete (s1, pos, len)));
        debug (".\tbdelete (%s, %d, %d) = %d", dumpBstring (s1), pos, len, rv);
    }

    if (ret) {
        debug ("\t\tfailure(%d) = %d (res = %p", __LINE__, ret, res);
        if (res) debug (" = \"%s\"", res);
        debug (")");
    }
    return ret;
}

static int test17 (void) {
    int ret = 0;
    debug ("TEST: int bdelete (bstring s1, int pos, int len);");
    /* tests with NULL */
    ret += test17_0 (NULL, 0, 0, NULL);
    ret += test17_0 (&badBstring1, 0, 0, NULL);
    ret += test17_0 (&badBstring2, 0, 0, NULL);

    /* normal operation tests */
    ret += test17_0 (&emptyBstring, 0, 0, "");
    ret += test17_0 (&shortBstring, 1, 3, "bs");
    ret += test17_0 (&shortBstring, -1, 3, "gus");
    ret += test17_0 (&shortBstring, 1, -3, "bogus");
    ret += test17_0 (&shortBstring, 3, 9, "bog");
    ret += test17_0 (&shortBstring, 3, 1, "bogs");
    ret += test17_0 (&longBstring, 4, 300, "This");

    debug ("\t# failures: %d", ret);
    return ret;
}

static int test18_0 (bstring b, int len, int res, int mlen) {
    int ret = 0;
    int rv;
    int ol = 0;

    debug (".\tballoc (%s, %d) = ", dumpBstring (b), len);
    if (b) ol = b->mlen;
    rv = balloc (b, len);
    debug ("%d", rv);

    if (b != NULL && b->data != NULL && b->slen >=0 && ol > b->mlen) {
        debug ("\t\tfailure(%d) oldmlen = %d, newmlen %d", __LINE__, ol, b->mlen);
        ret++;
    }

    if (rv != res) {
        debug ("\t\tfailure(%d) res = %d", __LINE__, res);
        ret++;
    }
    if (b != NULL && (mlen > b->mlen || b->mlen == 0)) {
        debug ("\t\tfailure(%d) b->mlen = %d mlen = %d", __LINE__, b->mlen, mlen);
        ret++;
    }
    return ret;
}

static int test18_1_int (bstring b, int len, int res, int mlen, int __line__) {
    int ret = 0;
    int rv;
    int ol = 0;

    debug (".\tballocmin (%s, %d) = ", dumpBstring (b), len);
    if (b) ol = b->mlen;

    rv = ballocmin (b, len);
    debug ("[%d] %d", __LINE__, rv);

    if (b != NULL && b->data != NULL && b->mlen != mlen) {
        debug ("\t\t[%d] failure(%d) oldmlen = %d, newmlen = %d, mlen = %d len = %d", __line__, __LINE__, ol, b->mlen, mlen, b->slen);
        ret++;
    }

    if (rv != res) {
        debug ("\t\t[%d] failure(%d) res = %d", __line__, __LINE__, res);
        ret++;
    }

    return ret;
}

#define test18_1(b, len, res, mlen) test18_1_int (b, len, res, mlen, __LINE__)

static int test18 (void) {
    int ret = 0, reto;
    bstring b = bfromcstr ("test");

    debug ("TEST: int balloc (bstring s, int len);");
    /* tests with NULL */
    ret += test18_0 (NULL, 2, BSTR_ERR, 0);
    ret += test18_0 (&badBstring1, 2, BSTR_ERR, 0);
    ret += test18_0 (&badBstring2, 2, BSTR_ERR, 0);

    /* normal operation tests */
    ret += test18_0 (b, 2, 0, b->mlen);
    ret += test18_0 (b, -1, BSTR_ERR, b->mlen);
    ret += test18_0 (b, 9, 0, 9);
    ret += test18_0 (b, 2, 0, 9);
    bwriteprotect (*b);
    ret += test18_0 (b, 4, BSTR_ERR, b->mlen);
    bwriteallow (*b);
    ret += test18_0 (b, 2, 0, b->mlen);
    ret += test18_0 (&emptyBstring, 9, BSTR_ERR, emptyBstring.mlen);

    bdestroy (b);
    debug ("\t# failures: %d", ret);

    reto = ret;
    ret = 0;

    b = bfromcstr ("test");

    debug ("TEST: int ballocmin (bstring s, int len);");
    /* tests with NULL */
    ret += test18_1 (NULL, 2, BSTR_ERR, 0);
    ret += test18_1 (&badBstring1, 2, BSTR_ERR, 0);
    ret += test18_1 (&badBstring2, 2, BSTR_ERR, 2);

    /* normal operation tests */
    ret += test18_1 (b, 2, 0, b->slen + 1);
    ret += test18_1 (b, -1, BSTR_ERR, b->mlen);
    ret += test18_1 (b, 9, 0, 9);
    ret += test18_1 (b, 2, 0, b->slen + 1);
    ret += test18_1 (b, 9, 0, 9);
    bwriteprotect (*b);
    ret += test18_1 (b, 4, BSTR_ERR, -1);
    bwriteallow (*b);
    ret += test18_1 (b, 2, 0, b->slen + 1);
    ret += test18_1 (&emptyBstring, 9, BSTR_ERR, emptyBstring.mlen);

    bdestroy (b);
    debug ("\t# failures: %d", ret);

    return reto + ret;
}

static int test19_0 (bstring b, int len, const char * res, int erv) {
    int rv, ret = 0;
    bstring b1;

    if (b != NULL && b->data != NULL && b->slen >= 0) {
        b1 = bstrcpy (b);
        bwriteprotect (*b1);
        ret += bpattern (b1, len) != BSTR_ERR;
        ret += !biseq (b1, b);
        bwriteallow (*b1);

        debug (".\tbpattern (%s, %d) = ", dumpBstring (b1), len);

        rv = bpattern (b1, len);

        debug ("%s", dumpBstring (b1));

        ret += (rv != erv);
        ret += (res == NULL) || ((int) strlen (res) > b1->slen)
            || (0 != memcmp (b1->data, res, b1->slen));
        ret += b1->data[b1->slen] != '\0';

        bdestroy(b1);
    } else {
        ret += BSTR_ERR != (rv = bpattern (b, len));
        debug (".\tbpattern (%s, %d) = %d", dumpBstring (b), len, rv);
    }

    if (ret) {
        debug ("\t\tfailure(%d) rv = %d erv = %d (res = %p", __LINE__, rv, erv, res);
        if (res) debug (" = \"%s\"", res);
        debug (")");
    }

    return ret;
}

static int test19 (void) {
    int ret = 0;

    debug ("TEST: int bpattern (bstring b, int len);");
    /* tests with NULL */
    ret += test19_0 (NULL, 0, NULL, BSTR_ERR);
    ret += test19_0 (NULL, 5, NULL, BSTR_ERR);
    ret += test19_0 (NULL, -5, NULL, BSTR_ERR);
    ret += test19_0 (&badBstring1, 5, NULL, BSTR_ERR);
    ret += test19_0 (&badBstring2, 5, NULL, BSTR_ERR);

    /* normal operation tests */
    ret += test19_0 (&emptyBstring, 0, "", BSTR_ERR);
    ret += test19_0 (&emptyBstring, 10, "", BSTR_ERR);
    ret += test19_0 (&emptyBstring, -1, "", BSTR_ERR);
    ret += test19_0 (&shortBstring, 0, "", 0);
    ret += test19_0 (&shortBstring, 12, "bogusbogusbo", 0);
    ret += test19_0 (&shortBstring, -1, "bogus", BSTR_ERR);

    debug ("\t# failures: %d", ret);
    return ret;
}

static int test20 (void) {
    int ret = 0;

#if !defined (BSTRLIB_NOVSNP)
    int rv;
    bstring b, c;

    debug ("TEST: bstring bformat (const char * fmt, ...);");
    /* tests with NULL */
    debug (".\tbformat (NULL, 1, 2) = ");
    b = bformat (NULL, 1, 2);
    debug ("%s", dumpBstring (b));
    ret += b != NULL;

    /* normal operation tests */
    debug (".\tbformat (\"%%d %%s\", 1, \"xy\") = ");
    b = bformat ("%d %s", 1, "xy");
    debug ("%s", dumpBstring (b));
    ret += !biseq (c = bfromcstr ("1 xy"), b);
    bdestroy (b);

    debug (".\tbformat (\"%%d %%s(%%s)\", 6, %s, %s) = ", dumpBstring (c), dumpBstring (&shortBstring));
    b = bformat ("%d %s(%s)", 6, c->data, shortBstring.data);
    debug ("%s", dumpBstring (b));
    bdestroy (c);
    ret += !biseq (c = bfromcstr ("6 1 xy(bogus)"), b);
    bdestroy (c);
    bdestroy (b);

    debug (".\tbformat (\"%%s%%s%%s%%s%%s%%s%%s%%s\", ...) ...");
    b = bformat ("%s%s%s%s%s%s%s%s", longBstring.data, longBstring.data
            , longBstring.data, longBstring.data
            , longBstring.data, longBstring.data
            , longBstring.data, longBstring.data);
    c = bstrcpy (&longBstring);
    bconcat (c, c);
    bconcat (c, c);
    bconcat (c, c);
    ret += !biseq (c, b);
    bdestroy (c);
    bdestroy (b);

    debug ("\t# failures: %d", ret);

    b = bfromcstr ("");
    debug ("TEST: int bformata (bstring b, const char * fmt, ...);");
    /* tests with NULL */
    debug (".\tbformata (%s, NULL, 1, 2) = ", dumpBstring (b));
    rv = bformata (b, NULL, 1, 2);
    debug ("%d", rv);
    ret += rv != BSTR_ERR;
    debug (".\tbformata (%s, \"%%d %%d\", 1, 2) = ", dumpBstring (&badBstring1));
    rv = bformata (&badBstring1, "%d %d", 1, 2);
    debug ("%d", rv);
    ret += rv != BSTR_ERR;
    debug (".\tbformata (%s, \"%%d %%d\", 1, 2) = ", dumpBstring (b));
    rv = bformata (b, "%d %d", 1, 2);
    debug ("%s", dumpBstring (b));
    ret += !biseq (c = bfromcstr ("1 2"), b);
    bdestroy (c);
    bdestroy (b);

    debug (".\tbformata (\"x\", \"%%s%%s%%s%%s%%s%%s%%s%%s\", ...) ...");
    rv = bformata (b = bfromcstr ("x"), "%s%s%s%s%s%s%s%s", 
            longBstring.data, longBstring.data,
            longBstring.data, longBstring.data,
            longBstring.data, longBstring.data,
            longBstring.data, longBstring.data);
    ret += rv == BSTR_ERR;
    c = bstrcpy (&longBstring);
    bconcat (c, c);
    bconcat (c, c);
    bconcat (c, c);
    binsertch (c, 0, 1, (char) 'x');
    ret += !biseq (c, b);
    bdestroy (c);
    bdestroy (b);

    debug ("\t# failures: %d", ret);

    b = bfromcstr ("Initial");
    debug ("TEST: int bassignformat (bstring b, const char * fmt, ...);");
    /* tests with NULL */
    debug (".\tbassignformat (%s, NULL, 1, 2) = ", dumpBstring (b));
    rv = bassignformat (b, NULL, 1, 2);
    debug ("%d", rv);
    ret += rv != BSTR_ERR;
    debug (".\tbassignformat (%s, \"%%d %%d\", 1, 2) = ", dumpBstring (&badBstring1));
    rv = bassignformat (&badBstring1, "%d %d", 1, 2);
    debug ("%d", rv);
    ret += rv != BSTR_ERR;
    debug (".\tbassignformat (%s, \"%%d %%d\", 1, 2) = ", dumpBstring (b));
    rv = bassignformat (b, "%d %d", 1, 2);
    debug ("%s", dumpBstring (b));
    ret += !biseq (c = bfromcstr ("1 2"), b);
    bdestroy (c);
    bdestroy (b);

    debug (".\tbassignformat (\"x\", \"%%s%%s%%s%%s%%s%%s%%s%%s\", ...) ...");
    rv = bassignformat (b = bfromcstr ("x"), "%s%s%s%s%s%s%s%s", 
            longBstring.data, longBstring.data,
            longBstring.data, longBstring.data,
            longBstring.data, longBstring.data,
            longBstring.data, longBstring.data);
    ret += rv == BSTR_ERR;
    c = bstrcpy (&longBstring);
    bconcat (c, c);
    bconcat (c, c);
    bconcat (c, c);
    ret += !biseq (c, b);
    bdestroy (c);
    bdestroy (b);

    debug ("\t# failures: %d", ret);
#endif

    return ret;
}

static int test21_0 (bstring b, char sc, int ns) {
    struct bstrList * l;
    int ret = 0;

    debug (".\tbsplit (%s, '%c') = ", dumpBstring (b), sc);

    if (b != NULL && b->data != NULL && b->slen >= 0) {
        bstring c;
        struct tagbstring t;

        blk2tbstr(t,&sc,1);

        debug ("{");

        l = bsplit (b, sc);

        if (l) {
            int i;
            for (i=0; i < l->qty; i++) {
                if (i != 0) debug (", ");
                debug ("%s", dumpBstring (l->entry[i]));
            }
            debug (":<%d>", l->qty);
            if (ns != l->qty) ret++;
        } else {
            debug ("NULL");
            ret ++;
        }

        debug ("}");

        c = bjoin (l, &t);
        ret += !biseq (c, b);
        bdestroy (c);
        ret += 0 != bstrListDestroy (l);
    } else {
        l = bsplit (b, sc);
        ret += (l != NULL);
        debug ("%p", (void *) l);
    }

    if (ret) {
        debug ("\t\tfailure(%d) ns = %d", __LINE__, ns);
    }

    return ret;
}

static int test21_1 (bstring b, const_bstring sc, int ns) {
    struct bstrList * l;
    int ret = 0;

    debug (".\tbsplitstr (%s, %s) = ", dumpBstring (b), dumpBstring (sc));

    if (b != NULL && b->data != NULL && b->slen >= 0) {
        bstring c;

        debug ("{");

        l = bsplitstr (b, sc);

        if (l) {
            int i;
            for (i=0; i < l->qty; i++) {
                if (i != 0) debug (", ");
                debug ("%s", dumpBstring (l->entry[i]));
            }
            debug (":<%d>", l->qty);
            if (ns != l->qty) ret++;
        } else {
            debug ("NULL");
            ret ++;
        }

        debug ("}");

        c = bjoin (l, sc);
        ret += !biseq (c, b);
        bdestroy (c);
        ret += 0 != bstrListDestroy (l);
    } else {
        l = bsplitstr (b, sc);
        ret += (l != NULL);
        debug ("%p", (void *) l);
    }

    if (ret) {
        debug ("\t\tfailure(%d) ns = %d", __LINE__, ns);
    }

    return ret;
}

static int test21 (void) {
    struct tagbstring is = bsStatic ("is");
    struct tagbstring ng = bsStatic ("ng");
    int ret = 0;

    debug ("TEST: struct bstrList * bsplit (const_bstring str, unsigned char splitChar);");
    /* tests with NULL */
    ret += test21_0 (NULL, (char) '?', 0);
    ret += test21_0 (&badBstring1, (char) '?', 0);
    ret += test21_0 (&badBstring2, (char) '?', 0);

    /* normal operation tests */
    ret += test21_0 (&emptyBstring, (char) '?', 1);
    ret += test21_0 (&shortBstring, (char) 'o', 2);
    ret += test21_0 (&shortBstring, (char) 's', 2);
    ret += test21_0 (&shortBstring, (char) 'b', 2);
    ret += test21_0 (&longBstring, (char) 'o', 9);

    debug ("TEST: struct bstrList * bsplitstr (bstring str, const_bstring splitStr);");

    ret += test21_1 (NULL, NULL, 0);
    ret += test21_1 (&badBstring1, &emptyBstring, 0);
    ret += test21_1 (&badBstring2, &emptyBstring, 0);

    /* normal operation tests */
    ret += test21_1 (&shortBstring, &emptyBstring, 5);
    ret += test21_1 (&longBstring, &is, 3);
    ret += test21_1 (&longBstring, &ng, 5);

    if (0 == ret) {
        struct bstrList * l;
        unsigned char c;
        struct tagbstring t;
        bstring b;
        bstring list[3] = { &emptyBstring, &shortBstring, &longBstring };
        int i;

        blk2tbstr (t, &c, 1);

        for (i=0; i < 3; i++) {
            c = (unsigned char) '\0';
            for (;;) {
                b = bjoin (l = bsplit (list[i], c), &t);
                if (!biseq (b, list[i])) {
                    debug ("\t\tfailure(%d) ", __LINE__);
                    debug ("join (bsplit (%s, x%02X), {x%02X}) = %s", dumpBstring (list[i]), c, c, dumpBstring (b));
                    ret++;
                }
                bdestroy (b);
                bstrListDestroy (l);
                if (ret) break;

                b = bjoin (l = bsplitstr (list[i], &t), &t);
                if (!biseq (b, list[i])) {
                    debug ("\t\tfailure(%d) ", __LINE__);
                    debug ("join (bsplitstr (%s, {x%02X}), {x%02X}) = %s", dumpBstring (list[i]), c, c, dumpBstring (b));
                    ret++;
                }
                bdestroy (b);
                bstrListDestroy (l);
                if (ret) break;

                if (UCHAR_MAX == c) break;
                c++;
            }
            if (ret) break;
        }
    }

    debug ("\t# failures: %d", ret);
    return ret;
}

static int test22_0 (const_bstring b, const_bstring sep, int ns, ...) {
    va_list arglist;
    struct bstrList * l;
    int ret = 0;

    debug (".\tbsplits (%s, %s)", dumpBstring (b), dumpBstring (sep));
    if (  b != NULL &&   b->data != NULL &&   b->slen >= 0 &&
            sep != NULL && sep->data != NULL && sep->slen >= 0) {
        debug (" {");

        l = bsplits (b, sep);

        if (l) {
            int i;
            va_start (arglist, ns);

            for (i=0; i < l->qty; i++) {
                char * res;

                res = va_arg (arglist, char *);

                if (i != 0) debug (", ");
                debug ("%s", dumpBstring (l->entry[i]));

                ret += (res == NULL) || ((int) strlen (res) > l->entry[i]->slen)
                    || (0 != memcmp (l->entry[i]->data, res, l->entry[i]->slen));
                ret += l->entry[i]->data[l->entry[i]->slen] != '\0';
            }

            va_end (arglist);

            debug (":<%d>", l->qty);
            if (ns != l->qty) ret++;
        } else {
            debug ("NULL");
            ret += (ns != 0);
        }

        debug ("}");

        ret += (0 != bstrListDestroy (l) && l != NULL);
    } else {
        l = bsplits (b, sep);
        ret += (l != NULL);
        debug (" = %p", (void *) l);
    }

    if (ret) {
        debug ("\t\tfailure(%d) ns = %d", __LINE__, ns);
    }

    return ret;
}

static int test22 (void) {
    int ret = 0;
    struct tagbstring o=bsStatic("o");
    struct tagbstring s=bsStatic("s");
    struct tagbstring b=bsStatic("b");
    struct tagbstring bs=bsStatic("bs");
    struct tagbstring uo=bsStatic("uo");

    debug ("TEST: extern struct bstrList * bsplits (const_bstring str, const_bstring splitStr);");
    /* tests with NULL */
    ret += test22_0 (NULL, &o, 0);
    ret += test22_0 (&o, NULL, 0);

    /* normal operation tests */
    ret += test22_0 (&emptyBstring, &o, 1, "");
    ret += test22_0 (&emptyBstring, &uo, 1, "");
    ret += test22_0 (&shortBstring, &emptyBstring, 1, "bogus");
    ret += test22_0 (&shortBstring, &o, 2, "b", "gus");
    ret += test22_0 (&shortBstring, &s, 2, "bogu", "");
    ret += test22_0 (&shortBstring, &b, 2, "" , "ogus");
    ret += test22_0 (&shortBstring, &bs, 3, "" , "ogu", "");
    ret += test22_0 (&longBstring, &o, 9, "This is a b", "gus but reas", "nably l", "ng string.  Just l", "ng en", "ugh t", " cause s", "me mall", "cing.");
    ret += test22_0 (&shortBstring, &uo, 3, "b", "g", "s");

    if (0 == ret) {
        struct bstrList * l;
        unsigned char c;
        struct tagbstring t;
        bstring bb;
        bstring list[3] = { &emptyBstring, &shortBstring, &longBstring };
        int i;

        blk2tbstr (t, &c, 1);

        for (i=0; i < 3; i++) {
            c = (unsigned char) '\0';
            for (;;) {
                bb = bjoin (l = bsplits (list[i], &t), &t);
                if (!biseq (bb, list[i])) {
                    debug ("\t\tfailure(%d) ", __LINE__);
                    debug ("join (bsplits (%s, {x%02X}), {x%02X}) = %s", dumpBstring (list[i]), c, c, dumpBstring (bb));
                    ret++;
                }
                bdestroy (bb);
                bstrListDestroy (l);
                if (ret) break;
                if (UCHAR_MAX == c) break;
                c++;
            }
            if (ret) break;
        }
    }

    debug ("\t# failures: %d", ret);
    return ret;
}

struct sbstr {
    int ofs;
    bstring b;
};

static size_t test23_aux_read (void *buff, size_t elsize, size_t nelem, void *parm) {
    struct sbstr * sb = (struct sbstr *)parm;
    int els, len;

    if (parm == NULL || elsize == 0 || nelem == 0) return 0;
    len = (int) (nelem * elsize); if (len <= 0) return 0;
    if (len + sb->ofs > sb->b->slen) len = sb->b->slen - sb->ofs;
    els = (int) (len / elsize);
    len = (int) (els * elsize);
    if (len > 0) {
        memcpy (buff, sb->b->data + sb->ofs, len);
        sb->ofs += len;
    }
    return els;
}

static int test23_aux_open (struct sbstr * sb, bstring b) {
    if (!sb || b == NULL || b->data == NULL) return -__LINE__;
    sb->ofs = 0;
    sb->b = b;
    return 0;
}

static int test23_aux_splitcb (void * parm, int ofs, const struct tagbstring * entry) {
    bstring b = (bstring) parm;

    (void)(ofs);
    if (b->slen > 0) bconchar (b, (char) '|');
    bconcat (b, entry);
    return 0;
}

struct tagBss {
    int first;
    unsigned char sc;
    bstring b;
};

static int test23_aux_splitcbx (void * parm, int ofs, const struct tagbstring * entry) {
    struct tagBss * p = (struct tagBss *) parm;

    (void)(ofs);
    if (!p->first) {
        bconchar (p->b, (char) p->sc);
    } else p->first = 0;

    bconcat (p->b, entry);
    return 0;
}

static int test23 (void) {
    struct tagbstring space = bsStatic (" ");
    struct sbstr sb;
    struct bStream * bs;
    bstring b;
    int l, ret = 0;

    debug ("TEST: bstream integrated test");
    test23_aux_open (&sb, &longBstring);
    ret += NULL != (bs = bsopen ((bNread) NULL, &sb));
    ret += NULL == (bs = bsopen ((bNread) test23_aux_read, &sb));
    ret += (bseof (bs) != 0);
    ret += BSTR_ERR != bsbufflength (NULL, -1);
    ret += BSTR_ERR != bsbufflength (NULL, 1);
    ret += BSTR_ERR != bsbufflength (bs, -1);
    debug (".\tbsbufflength (bs, 0) -> %d", bsbufflength (bs, 0));
    ret += BSTR_ERR == bsbufflength (bs, 1);
    ret += BSTR_ERR != bspeek (NULL, bs);
    ret += BSTR_ERR != bsreadln (NULL, bs, (char) '?');
    ret += BSTR_ERR != bsreadln (&emptyBstring, bs, (char) '?');
    ret += BSTR_ERR != bspeek (&emptyBstring, bs);

    ret += BSTR_ERR == bspeek (b = bfromcstr (""), bs);

    debug (".\tbspeek () -> %s", dumpBstring (b));
    ret += BSTR_ERR != bsreadln (b, NULL, (char) '?');
    b->slen = 0;
    ret += BSTR_ERR == bsreadln (b, bs, (char) '?');
    ret += (bseof (bs) <= 0);
    ret += biseq (b, &longBstring) < 0;
    debug (".\tbsreadln ('?') -> %s", dumpBstring (b));
    ret += BSTR_ERR == bsunread (bs, b);
    ret += (bseof (bs) != 0);
    debug (".\tbsunread (%s)", dumpBstring (b));
    b->slen = 0;
    ret += BSTR_ERR == bspeek (b, bs);
    ret += biseq (b, &longBstring) < 0;
    debug (".\tbspeek () -> %s", dumpBstring (b));
    b->slen = 0;
    ret += BSTR_ERR == bsreadln (b, bs, (char) '?');
    ret += (bseof (bs) <= 0);
    ret += biseq (b, &longBstring) < 0;
    debug (".\tbsreadln ('?') -> %s", dumpBstring (b));
    ret += NULL == bsclose (bs);
    sb.ofs = 0;

    ret += NULL == (bs = bsopen ((bNread) test23_aux_read, &sb));
    b->slen = 0;
    ret += BSTR_ERR == bsreadln (b, bs, (char) '.');
    l = b->slen;
    ret += (0 != bstrncmp (b, &longBstring, l)) || (longBstring.data[l-1] != '.');
    debug (".\tbsreadln ('.') -> %s", dumpBstring (b));
    ret += BSTR_ERR == bsunread (bs, b);

    debug (".\tbsunread (%s)", dumpBstring (b));
    b->slen = 0;
    ret += BSTR_ERR == bspeek (b, bs);
    ret += biseq (b, &longBstring) < 0;
    debug (".\tbspeek () -> %s", dumpBstring (b));
    b->slen = 0;
    ret += BSTR_ERR == bsreadln (b, bs, (char) '.');

    ret += b->slen != l || (0 != bstrncmp (b, &longBstring, l)) || (longBstring.data[l-1] != '.');
    debug (".\tbsreadln ('.') -> %s", dumpBstring (b));
    ret += NULL == bsclose (bs);

    test23_aux_open (&sb, &longBstring);
    ret += NULL == (bs = bsopen ((bNread) test23_aux_read, &sb));
    ret += (bseof (bs) != 0);
    b->slen = 0;
    l = bssplitscb (bs, &space, test23_aux_splitcb, b);
    ret += (bseof (bs) <= 0);
    ret += NULL == bsclose (bs);
    debug (".\tbssplitscb (' ') -> %s", dumpBstring (b));

    for (l=1; l < 4; l++) {
        char * str;
        for (str = (char *) longBstring.data; *str; str++) {
            test23_aux_open (&sb, &longBstring);
            ret += NULL == (bs = bsopen ((bNread) test23_aux_read, &sb));
            ret += bseof (bs) != 0;
            ret += 0 > bsbufflength (bs, l);
            b->slen = 0;
            while (0 == bsreadlna (b, bs, *str)) ;
            ret += 0 == biseq (b, &longBstring);
            ret += bseof (bs) <= 0;
            ret += NULL == bsclose (bs);
            if (ret) break;
        }
        if (ret) break;
    }

    bdestroy (b);

    if (0 == ret) {
        unsigned char c;
        struct tagbstring t;
        bstring list[3] = { &emptyBstring, &shortBstring, &longBstring };
        int i;

        blk2tbstr (t, &c, 1);

        for (i=0; i < 3; i++) {
            c = (unsigned char) '\0';
            for (;;) {
                struct tagBss bss;

                bss.sc = c;
                bss.b = bfromcstr ("");
                bss.first = 1;

                test23_aux_open (&sb, list[i]);
                bs = bsopen ((bNread) test23_aux_read, &sb);
                bssplitscb (bs, &t, test23_aux_splitcbx, &bss);
                bsclose (bs);

                if (!biseq (bss.b, list[i])) {
                    debug ("\t\tfailure(%d) ", __LINE__);
                    debug ("join (bssplitscb (%s, {x%02X}), {x%02X}) = %s", dumpBstring (list[i]), c, c, dumpBstring (bss.b));
                    ret++;
                }
                bdestroy (bss.b);
                if (ret) break;
                if (UCHAR_MAX == c) break;
                c++;
            }
            if (ret) break;

            for (;;) {
                struct tagBss bss;

                bss.sc = c;
                bss.b = bfromcstr ("");
                bss.first = 1;

                test23_aux_open (&sb, list[i]);
                bs = bsopen ((bNread) test23_aux_read, &sb);
                bssplitstrcb (bs, &t, test23_aux_splitcbx, &bss);
                bsclose (bs);

                if (!biseq (bss.b, list[i])) {
                    debug ("\t\tfailure(%d) ", __LINE__);
                    debug ("join (bssplitstrcb (%s, {x%02X}), {x%02X}) = %s", dumpBstring (list[i]), c, c, dumpBstring (bss.b));
                    ret++;
                }
                bdestroy (bss.b);
                if (ret) break;
                if (UCHAR_MAX == c) break;
                c++;
            }
            if (ret) break;
        }
    }

    debug ("\t# failures: %d", ret);
    return ret;
}

static int test24_0 (bstring s1, int pos, const_bstring s2, int res) {
    int rv, ret = 0;

    debug (".\tbninchr (%s, %d, %s) = ", dumpBstring (s1), pos, dumpBstring (s2));
    rv = bninchr (s1, pos, s2);
    debug ("%d", rv);
    ret += (rv != res);
    if (ret) {
        debug ("\t\tfailure(%d) res = %d", __LINE__, res);
    }
    return ret;
}

static int test24 (void) {
    bstring b;
    int ret = 0;

    debug ("TEST: int bninchr (const_bstring s1, int pos, const_bstring s2);");
    ret += test24_0 (NULL, 0, NULL, BSTR_ERR);
    ret += test24_0 (&emptyBstring, 0, NULL, BSTR_ERR);
    ret += test24_0 (NULL, 0, &emptyBstring, BSTR_ERR);
    ret += test24_0 (&shortBstring, 3, &badBstring1, BSTR_ERR);
    ret += test24_0 (&badBstring1, 3, &shortBstring, BSTR_ERR);

    ret += test24_0 (&emptyBstring, 0, &emptyBstring, BSTR_ERR);
    ret += test24_0 (&shortBstring, 0, &emptyBstring, BSTR_ERR);
    ret += test24_0 (&shortBstring,  0, &shortBstring, BSTR_ERR);
    ret += test24_0 (&shortBstring,  1, &shortBstring, BSTR_ERR);
    ret += test24_0 (&longBstring, 3, &shortBstring, 4);
    ret += test24_0 (&longBstring, 3, b = bstrcpy (&shortBstring), 4);
    bdestroy (b);
    ret += test24_0 (&longBstring, -1, &shortBstring, BSTR_ERR);
    ret += test24_0 (&longBstring, 1000, &shortBstring, BSTR_ERR);
    ret += test24_0 (&xxxxxBstring, 0, &shortBstring, 0);
    ret += test24_0 (&xxxxxBstring, 1, &shortBstring, 1);
    ret += test24_0 (&emptyBstring, 0, &shortBstring, BSTR_ERR);

    ret += test24_0 (&longBstring, 0, &shortBstring, 0);
    ret += test24_0 (&longBstring, 10, &shortBstring, 15);
    debug ("\t# failures: %d", ret);
    return ret;
}

static int test25_0 (bstring s1, int pos, const_bstring s2, int res) {
    int rv, ret = 0;

    debug (".\tbninchrr (%s, %d, %s) = ", dumpBstring (s1), pos, dumpBstring (s2));
    rv = bninchrr (s1, pos, s2);
    debug ("%d", rv);
    ret += (rv != res);
    if (ret) {
        debug ("\t\tfailure(%d) res = %d", __LINE__, res);
    }
    return ret;
}

static int test25 (void) {
    bstring b;
    int ret = 0;

    debug ("TEST: int bninchrr (const_bstring s1, int pos, const_bstring s2);");
    ret += test25_0 (NULL, 0, NULL, BSTR_ERR);
    ret += test25_0 (&emptyBstring, 0, NULL, BSTR_ERR);
    ret += test25_0 (NULL, 0, &emptyBstring, BSTR_ERR);
    ret += test25_0 (&emptyBstring, 0, &emptyBstring, BSTR_ERR);
    ret += test25_0 (&shortBstring, 0, &emptyBstring, BSTR_ERR);
    ret += test25_0 (&shortBstring, 0, &badBstring1, BSTR_ERR);
    ret += test25_0 (&badBstring1, 0, &shortBstring, BSTR_ERR);

    ret += test25_0 (&shortBstring,  0, &shortBstring, BSTR_ERR);
    ret += test25_0 (&shortBstring,  4, &shortBstring, BSTR_ERR);
    ret += test25_0 (&longBstring, 10, &shortBstring, 9);
    ret += test25_0 (&longBstring, 10, b = bstrcpy (&shortBstring), 9);
    bdestroy (b);
    ret += test25_0 (&xxxxxBstring, 4, &shortBstring, 4);
    ret += test25_0 (&emptyBstring, 0, &shortBstring, BSTR_ERR);

    debug ("\t# failures: %d", ret);
    return ret;
}

static int test26_0 (bstring b0, int pos, int len, const_bstring b1, unsigned char fill, char * res) {
    bstring b2;
    int rv, ret = 0;

    if (b0 != NULL && b0->data != NULL && b0->slen >= 0 &&
            b1 != NULL && b1->data != NULL && b1->slen >= 0) {
        b2 = bstrcpy (b0);
        bwriteprotect (*b2);

        debug (".\tbreplace (%s, ", dumpBstring (b2));

        rv = breplace (b2, pos, len, b1, fill);
        ret += (rv == 0);
        if (!biseq (b0, b2)) ret++;

        debug ("%d, %d, %s, %02X) = %s", pos, len, dumpBstring (b1), fill, dumpBstring (b2));

        bwriteallow (*b2);

        debug (".\tbreplace (%s, ", dumpBstring (b2));

        rv = breplace (b2, pos, len, b1, fill);
        if (b1) {
            ret += (pos < 0) && (b2->slen != b0->slen);
            ret += ((rv == 0) != (pos >= 0 && pos <= b2->slen));
        }

        ret += (res == NULL) || ((int) strlen (res) > b2->slen)
            || (0 != memcmp (b2->data, res, b2->slen));
        ret += b2->data[b2->slen] != '\0';

        debug ("%d, %d, %s, %02X) = %s", pos, len, dumpBstring (b1), fill, dumpBstring (b2));

        bdestroy (b2);
    } else {
        ret += (BSTR_ERR != (rv = breplace (b0, pos, len, b1, fill)));
        debug (".\tbreplace (%s, %d, %d, %s, %02X) = %d", dumpBstring (b0), pos, len, dumpBstring (b1), fill, rv);
    }

    if (ret) {
        debug ("\t\tfailure(%d) = %d (res = %p", __LINE__, ret, res);
        if (res) debug (" = \"%s\"", res);
        debug (")");
    }
    return ret;
}

static int test26 (void) {
    int ret = 0;
    debug ("TEST: int breplace (bstring b0, int pos, int len, const_bstring b1, unsigned char fill);");
    /* tests with NULL */
    ret += test26_0 (NULL, 0, 0, NULL, (unsigned char) '?', NULL);
    ret += test26_0 (NULL, 0, 0, &emptyBstring, (unsigned char) '?', NULL);
    ret += test26_0 (&badBstring1,   1, 3, &shortBstring, (unsigned char) '?', NULL);
    ret += test26_0 (&shortBstring,  1, 3,  &badBstring1, (unsigned char) '?', NULL);

    /* normal operation tests */
    ret += test26_0 (&emptyBstring,  0, 0, &emptyBstring, (unsigned char) '?', "");
    ret += test26_0 (&emptyBstring,  5, 0, &emptyBstring, (unsigned char) '?', "?????");
    ret += test26_0 (&emptyBstring,  5, 0, &shortBstring, (unsigned char) '?', "?????bogus");
    ret += test26_0 (&shortBstring,  0, 0, &emptyBstring, (unsigned char) '?', "bogus");
    ret += test26_0 (&emptyBstring,  0, 0, &shortBstring, (unsigned char) '?', "bogus");
    ret += test26_0 (&shortBstring,  0, 0, &shortBstring, (unsigned char) '?', "bogusbogus");
    ret += test26_0 (&shortBstring,  1, 3, &shortBstring, (unsigned char) '?', "bboguss");
    ret += test26_0 (&shortBstring,  3, 8, &shortBstring, (unsigned char) '?', "bogbogus");
    ret += test26_0 (&shortBstring, -1, 0, &shortBstring, (unsigned char) '?', "bogus");
    ret += test26_0 (&shortBstring,  2, 0, &shortBstring, (unsigned char) '?', "bobogusgus");
    ret += test26_0 (&shortBstring,  6, 0, &shortBstring, (unsigned char) '?', "bogus?bogus");
    ret += test26_0 (&shortBstring,  6, 0, NULL,          (unsigned char) '?', "bogus");
    debug ("\t# failures: %d", ret);
    return ret;
}

static int test27_0 (bstring b0, const_bstring b1, const char * res) {
    bstring b2;
    int rv, ret = 0;

    if (b0 != NULL && b0->data != NULL && b0->slen >= 0 &&
            b1 != NULL && b1->data != NULL && b1->slen >= 0) {
        b2 = bstrcpy (b0);
        bwriteprotect (*b2);

        debug (".\tbassign (%s, ", dumpBstring (b2));

        rv = bassign (b2, b1);
        ret += (rv == 0);
        if (!biseq (b0, b2)) ret++;

        debug ("%s) = %s", dumpBstring (b1), dumpBstring (b2));

        bwriteallow (*b2);

        debug (".\tbassign (%s, ", dumpBstring (b2));

        rv = bassign (b2, b1);

        debug ("%s) = %s", dumpBstring (b1), dumpBstring (b2));

        if (b1) ret += (b2->slen != b1->slen);
        ret += ((0 != rv) && (b1 != NULL)) || ((0 == rv) && (b1 == NULL));
        ret += (res == NULL) || ((int) strlen (res) != b2->slen) 
            || (0 != memcmp (b2->data, res, b2->slen));
        ret += b2->data[b2->slen] != '\0';
        bdestroy (b2);
    } else {
        ret += (BSTR_ERR != (rv = bassign (b0, b1)));
        debug (".\tbassign (%s, %s) = %d", dumpBstring (b0), dumpBstring (b1), rv);
    }

    if (ret) {
        debug ("\t\tfailure(%d) = %d (res = %p", __LINE__, ret, res);
        if (res) debug (" = \"%s\"", res);
        debug (")");
    }
    return ret;
}

static int test27 (void) {
    int ret = 0;

    debug ("TEST: int bassign (bstring b0, const_bstring b1);");

    /* tests with NULL */
    ret += test27_0 (NULL, NULL, NULL);
    ret += test27_0 (NULL, &emptyBstring, NULL);
    ret += test27_0 (&emptyBstring, NULL, "");
    ret += test27_0 (&badBstring1, &emptyBstring, NULL);
    ret += test27_0 (&badBstring2, &emptyBstring, NULL);
    ret += test27_0 (&emptyBstring, &badBstring1, NULL);
    ret += test27_0 (&emptyBstring, &badBstring2, NULL);

    /* normal operation tests on all sorts of subranges */
    ret += test27_0 (&emptyBstring, &emptyBstring, "");
    ret += test27_0 (&emptyBstring, &shortBstring, "bogus");
    ret += test27_0 (&shortBstring, &emptyBstring, "");
    ret += test27_0 (&shortBstring, &shortBstring, "bogus");
    debug ("\t# failures: %d", ret);
    return ret;
}

static int test28_0 (bstring s1, int c, int res) {
    int rv, ret = 0;

    debug (".\tbstrchr (%s, %d) = ", dumpBstring (s1), c);
    rv = bstrchr (s1, c);
    debug ("%d", rv);
    ret += (rv != res);
    if (ret) {
        debug ("\t\tfailure(%d) res = %d", __LINE__, res);
    }
    return ret;
}

static int test28_1 (bstring s1, int c, int res) {
    int rv, ret = 0;

    debug (".\tbstrrchr (%s, %d) = ", dumpBstring (s1), c);
    rv = bstrrchr (s1, c);
    debug ("%d", rv);
    ret += (rv != res);
    if (ret) {
        debug ("\t\tfailure(%d) res = %d rv = %d", __LINE__, res, rv);
    }
    return ret;
}

static int test28_2 (bstring s1, int c, int pos, int res) {
    int rv, ret = 0;

    debug (".\tbstrchrp (%s, %d, %d) = ", dumpBstring (s1), c, pos);
    rv = bstrchrp (s1, c, pos);
    debug ("%d", rv);
    ret += (rv != res);
    if (ret) {
        debug ("\t\tfailure(%d) res = %d", __LINE__, res);
    }
    return ret;
}

static int test28_3 (bstring s1, int c, int pos, int res) {
    int rv, ret = 0;

    debug (".\tbstrrchrp (%s, %d, %d) = ", dumpBstring (s1), c, pos);
    rv = bstrrchrp (s1, c, pos);
    debug ("%d", rv);
    ret += (rv != res);
    if (ret) {
        debug ("\t\tfailure(%d) res = %d rv = %d", __LINE__, res, rv);
    }
    return ret;
}

static int test28 (void) {
    int ret = 0;

    debug ("TEST: int bstrchr (const_bstring s1, int c);");
    ret += test28_0 (NULL, 0, BSTR_ERR);
    ret += test28_0 (&badBstring1, 'b', BSTR_ERR);
    ret += test28_0 (&badBstring2, 's', BSTR_ERR);

    ret += test28_0 (&emptyBstring, 0, BSTR_ERR);
    ret += test28_0 (&shortBstring, 0, BSTR_ERR);
    ret += test28_0 (&shortBstring, 'b', 0);
    ret += test28_0 (&shortBstring, 's', 4);
    ret += test28_0 (&shortBstring, 'q', BSTR_ERR);
    ret += test28_0 (&xxxxxBstring, 0, BSTR_ERR);
    ret += test28_0 (&xxxxxBstring, 'b', BSTR_ERR);
    ret += test28_0 (&longBstring, 'i', 2);

    debug ("TEST: int bstrrchr (const_bstring s1, int c);");
    ret += test28_1 (NULL, 0, BSTR_ERR);
    ret += test28_1 (&badBstring1, 'b', BSTR_ERR);
    ret += test28_1 (&badBstring2, 's', BSTR_ERR);

    ret += test28_1 (&emptyBstring, 0, BSTR_ERR);
    ret += test28_1 (&shortBstring, 0, BSTR_ERR);
    ret += test28_1 (&shortBstring, 'b', 0);
    ret += test28_1 (&shortBstring, 's', 4);
    ret += test28_1 (&shortBstring, 'q', BSTR_ERR);
    ret += test28_1 (&xxxxxBstring, 0, BSTR_ERR);
    ret += test28_1 (&xxxxxBstring, 'b', BSTR_ERR);
    ret += test28_1 (&longBstring, 'i', 82);

    debug ("TEST: int bstrchrp (const_bstring s1, int c, int pos);");
    ret += test28_2 (NULL, 0, 0, BSTR_ERR);
    ret += test28_2 (&badBstring1, 'b', 0, BSTR_ERR);
    ret += test28_2 (&badBstring2, 's', 0, BSTR_ERR);
    ret += test28_2 (&shortBstring, 'b', -1, BSTR_ERR);
    ret += test28_2 (&shortBstring, 'b', shortBstring.slen, BSTR_ERR);

    ret += test28_2 (&emptyBstring, 0, 0, BSTR_ERR);
    ret += test28_2 (&shortBstring, 0, 0, BSTR_ERR);
    ret += test28_2 (&shortBstring, 'b', 0, 0);
    ret += test28_2 (&shortBstring, 'b', 1, BSTR_ERR);
    ret += test28_2 (&shortBstring, 's', 0, 4);
    ret += test28_2 (&shortBstring, 'q', 0, BSTR_ERR);

    debug ("TEST: int bstrrchrp (const_bstring s1, int c, int pos);");
    ret += test28_3 (NULL, 0, 0, BSTR_ERR);
    ret += test28_3 (&badBstring1, 'b', 0, BSTR_ERR);
    ret += test28_3 (&badBstring2, 's', 0, BSTR_ERR);
    ret += test28_3 (&shortBstring, 'b', -1, BSTR_ERR);
    ret += test28_3 (&shortBstring, 'b', shortBstring.slen, BSTR_ERR);

    ret += test28_3 (&emptyBstring, 0, 0, BSTR_ERR);
    ret += test28_3 (&shortBstring, 0, 0, BSTR_ERR);
    ret += test28_3 (&shortBstring, 'b', 0, 0);
    ret += test28_3 (&shortBstring, 'b', shortBstring.slen - 1, 0);
    ret += test28_3 (&shortBstring, 's', shortBstring.slen - 1, 4);
    ret += test28_3 (&shortBstring, 's', 0, BSTR_ERR);

    debug ("\t# failures: %d", ret);
    return ret;
}

static int test29_0 (bstring b0, char * s, const char * res) {
    bstring b2;
    int rv, ret = 0;

    if (b0 != NULL && b0->data != NULL && b0->slen >= 0) {
        b2 = bstrcpy (b0);
        bwriteprotect (*b2);

        debug (".\tbcatcstr (%s, ", dumpBstring (b2));

        rv = bcatcstr (b2, s);
        ret += (rv == 0);
        if (!biseq (b0, b2)) ret++;

        debug ("%p) = %s", s, dumpBstring (b2));

        bwriteallow (*b2);

        debug (".\tbcatcstr (%s, ", dumpBstring (b2));

        rv = bcatcstr (b2, s);

        debug ("%p) = %s", s, dumpBstring (b2));

        if (s) ret += (b2->slen != b0->slen + (int) strlen (s));
        ret += ((0 != rv) && (s != NULL)) || ((0 == rv) && (s == NULL));
        ret += (res == NULL) || ((int) strlen (res) != b2->slen) 
            || (0 != memcmp (b2->data, res, b2->slen));
        ret += b2->data[b2->slen] != '\0';
        bdestroy (b2);
    } else {
        ret += (BSTR_ERR != (rv = bcatcstr (b0, s)));
        debug (".\tbcatcstr (%s, %p) = %d", dumpBstring (b0), s, rv);
    }

    if (ret) {
        debug ("\t\tfailure(%d) = %d (res = %p", __LINE__, ret, res);
        if (res) debug (" = \"%s\"", res);
        debug (")");
    }
    return ret;
}

static int test29 (void) {
    int ret = 0;

    debug ("TEST: int bcatcstr (bstring b0, const char * s);");

    /* tests with NULL */
    ret += test29_0 (NULL, NULL, NULL);
    ret += test29_0 (NULL, "", NULL);
    ret += test29_0 (&emptyBstring, NULL, "");
    ret += test29_0 (&badBstring1, "bogus", NULL);
    ret += test29_0 (&badBstring2, "bogus", NULL);

    /* normal operation tests on all sorts of subranges */
    ret += test29_0 (&emptyBstring, "", "");
    ret += test29_0 (&emptyBstring, "bogus", "bogus");
    ret += test29_0 (&shortBstring, "", "bogus");
    ret += test29_0 (&shortBstring, "bogus", "bogusbogus");
    debug ("\t# failures: %d", ret);
    return ret;
}

static int test30_0 (bstring b0, const unsigned char * s, int len, const char * res) {
    bstring b2;
    int rv, ret = 0;

    if (b0 != NULL && b0->data != NULL && b0->slen >= 0) {
        b2 = bstrcpy (b0);
        bwriteprotect (*b2);

        debug (".\tbcatblk (%s, ", dumpBstring (b2));

        rv = bcatblk (b2, s, len);
        ret += (rv == 0);
        if (!biseq (b0, b2)) ret++;

        debug ("%p) = %s", s, dumpBstring (b2));

        bwriteallow (*b2);

        debug (".\tbcatblk (%s, ", dumpBstring (b2));

        rv = bcatblk (b2, s, len);

        debug ("%p) = %s", s, dumpBstring (b2));

        if (s) {
            if (len >= 0) ret += (b2->slen != b0->slen + len);
            else ret += (b2->slen != b0->slen);
        }
        ret += ((0 != rv) && (s != NULL && len >= 0)) || ((0 == rv) && (s == NULL || len < 0));
        ret += (res == NULL) || ((int) strlen (res) != b2->slen) 
            || (0 != memcmp (b2->data, res, b2->slen));
        ret += b2->data[b2->slen] != '\0';
        bdestroy (b2);
    } else {
        ret += (BSTR_ERR != (rv = bcatblk (b0, s, len)));
        debug (".\tbcatblk (%s, %p, %d) = %d", dumpBstring (b0), s, len, rv);
    }

    if (ret) {
        debug ("\t\tfailure(%d) = %d (res = %p", __LINE__, ret, res);
        if (res) debug (" = \"%s\"", res);
        debug (")");
    }
    return ret;
}

static int test30 (void) {
    int ret = 0;

    debug ("TEST: int bcatblk (bstring b0, const char * s);");

    /* tests with NULL */
    ret += test30_0 (NULL, NULL, 0, NULL);
    ret += test30_0 (NULL, (unsigned char *) "", 0, NULL);
    ret += test30_0 (&emptyBstring, NULL, 0, "");
    ret += test30_0 (&emptyBstring, NULL, -1, "");
    ret += test30_0 (&badBstring1, NULL, 0, NULL);
    ret += test30_0 (&badBstring2, NULL, 0, NULL);

    /* normal operation tests on all sorts of subranges */
    ret += test30_0 (&emptyBstring, (unsigned char *) "", -1, "");
    ret += test30_0 (&emptyBstring, (unsigned char *) "", 0, "");
    ret += test30_0 (&emptyBstring, (unsigned char *) "bogus", 5, "bogus");
    ret += test30_0 (&shortBstring, (unsigned char *) "", 0, "bogus");
    ret += test30_0 (&shortBstring, (unsigned char *) "bogus", 5, "bogusbogus");
    ret += test30_0 (&shortBstring, (unsigned char *) "bogus", -1, "bogus");
    debug ("\t# failures: %d", ret);
    return ret;
}

static int test31_0 (bstring b0, const_bstring find, const_bstring replace, int pos, char * res) {
    bstring b2;
    int rv, ret = 0;

    if (b0 != NULL && b0->data != NULL && b0->slen >= 0 &&
            find != NULL && find->data != NULL && find->slen >= 0 &&
            replace != NULL && replace->data != NULL && replace->slen >= 0) {
        b2 = bstrcpy (b0);
        bwriteprotect (*b2);

        debug (".\tbfindreplace (%s, %s, %s, %d) = ", dumpBstring (b2), dumpBstring (find), dumpBstring (replace), pos);

        rv = bfindreplace (b2, find, replace, pos);
        ret += (rv == 0);
        if (!biseq (b0, b2)) ret++;

        debug ("%d", rv);

        bwriteallow (*b2);

        debug (".\tbfindreplace (%s, %s, %s, %d)", dumpBstring (b2), dumpBstring (find), dumpBstring (replace), pos);

        rv = bfindreplace (b2, find, replace, pos);

        ret += (res == NULL) || ((int) strlen (res) > b2->slen)
            || (0 != memcmp (b2->data, res, b2->slen));
        ret += b2->data[b2->slen] != '\0';

        debug (" -> %s", dumpBstring (b2));

        bdestroy (b2);
    } else {
        ret += (BSTR_ERR != (rv = bfindreplace (b0, find, replace, pos)));
        debug (".\tbfindreplace (%s, %s, %s, %d) = %d", dumpBstring (b0), dumpBstring (find), dumpBstring (replace), pos, rv);
    }

    if (ret) {
        debug ("\t\tfailure(%d) = %d (res = %p", __LINE__, ret, res);
        if (res) debug (" = \"%s\"", res);
        debug (")");
    }
    return ret;
}

static int test31_1 (bstring b0, const_bstring find, const_bstring replace, int pos, char * res) {
    bstring b2;
    int rv, ret = 0;

    if (b0 != NULL && b0->data != NULL && b0->slen >= 0 &&
            find != NULL && find->data != NULL && find->slen >= 0 &&
            replace != NULL && replace->data != NULL && replace->slen >= 0) {
        b2 = bstrcpy (b0);
        bwriteprotect (*b2);

        debug (".\tbfindreplacecaseless (%s, %s, %s, %d) = ", dumpBstring (b2), dumpBstring (find), dumpBstring (replace), pos);

        rv = bfindreplacecaseless (b2, find, replace, pos);
        ret += (rv == 0);
        if (!biseq (b0, b2)) ret++;

        debug ("%d", rv);

        bwriteallow (*b2);

        debug (".\tbfindreplacecaseless (%s, %s, %s, %d)", dumpBstring (b2), dumpBstring (find), dumpBstring (replace), pos);

        rv = bfindreplacecaseless (b2, find, replace, pos);

        ret += (res == NULL) || ((int) strlen (res) > b2->slen)
            || (0 != memcmp (b2->data, res, b2->slen));
        ret += b2->data[b2->slen] != '\0';

        debug (" -> %s", dumpBstring (b2));

        bdestroy (b2);
    } else {
        ret += (BSTR_ERR != (rv = bfindreplacecaseless (b0, find, replace, pos)));
        debug (".\tbfindreplacecaseless (%s, %s, %s, %d) = %d", dumpBstring (b0), dumpBstring (find), dumpBstring (replace), pos, rv);
    }

    if (ret) {
        debug ("\t\tfailure(%d) = %d (res = %p", __LINE__, ret, res);
        if (res) debug (" = \"%s\"", res);
        debug (")");
    }
    return ret;
}

#define LOTS_OF_S "ssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss"

static int test31 (void) {
    int ret = 0;
    struct tagbstring t0 = bsStatic ("funny");
    struct tagbstring t1 = bsStatic ("weird");
    struct tagbstring t2 = bsStatic ("s");
    struct tagbstring t3 = bsStatic ("long");
    struct tagbstring t4 = bsStatic ("big");
    struct tagbstring t5 = bsStatic ("ss");
    struct tagbstring t6 = bsStatic ("sstsst");
    struct tagbstring t7 = bsStatic ("xx" LOTS_OF_S "xx");
    struct tagbstring t8 = bsStatic ("S");
    struct tagbstring t9 = bsStatic ("LONG");

    debug ("TEST: int bfindreplace (bstring b, const_bstring f, const_bstring r, int pos);");
    /* tests with NULL */
    ret += test31_0 (NULL, NULL, NULL, 0, NULL);
    ret += test31_0 (&shortBstring, NULL, &t1, 0, (char *) shortBstring.data);
    ret += test31_0 (&shortBstring, &t2, NULL, 0, (char *) shortBstring.data);
    ret += test31_0 (&badBstring1, &t2, &t1, 0, NULL);
    ret += test31_0 (&badBstring2, &t2, &t1, 0, NULL);

    /* normal operation tests */
    ret += test31_0 (&longBstring, &shortBstring, &t0, 0, "This is a funny but reasonably long string.  Just long enough to cause some mallocing.");
    ret += test31_0 (&longBstring, &t2, &t1, 0, "Thiweird iweird a boguweird but reaweirdonably long weirdtring.  Juweirdt long enough to cauweirde weirdome mallocing.");
    ret += test31_0 (&shortBstring, &t2, &t1, 0, "boguweird");
    ret += test31_0 (&shortBstring, &t8, &t1, 0, "bogus");
    ret += test31_0 (&longBstring, &t2, &t1, 27, "This is a bogus but reasonably long weirdtring.  Juweirdt long enough to cauweirde weirdome mallocing.");
    ret += test31_0 (&longBstring, &t3, &t4, 0, "This is a bogus but reasonably big string.  Just big enough to cause some mallocing.");
    ret += test31_0 (&longBstring, &t9, &t4, 0, "This is a bogus but reasonably long string.  Just long enough to cause some mallocing.");
    ret += test31_0 (&t6, &t2, &t5, 0, "sssstsssst");
    ret += test31_0 (&t7, &t2, &t5, 0, "xx" LOTS_OF_S LOTS_OF_S "xx");

    debug ("TEST: int bfindreplacecaseless (bstring b, const_bstring f, const_bstring r, int pos);");
    /* tests with NULL */
    ret += test31_1 (NULL, NULL, NULL, 0, NULL);
    ret += test31_1 (&shortBstring, NULL, &t1, 0, (char *) shortBstring.data);
    ret += test31_1 (&shortBstring, &t2, NULL, 0, (char *) shortBstring.data);
    ret += test31_1 (&badBstring1, &t2, &t1, 0, NULL);
    ret += test31_1 (&badBstring2, &t2, &t1, 0, NULL);

    /* normal operation tests */
    ret += test31_1 (&longBstring, &shortBstring, &t0, 0, "This is a funny but reasonably long string.  Just long enough to cause some mallocing.");
    ret += test31_1 (&longBstring, &t2, &t1, 0, "Thiweird iweird a boguweird but reaweirdonably long weirdtring.  Juweirdt long enough to cauweirde weirdome mallocing.");
    ret += test31_1 (&shortBstring, &t2, &t1, 0, "boguweird");
    ret += test31_1 (&shortBstring, &t8, &t1, 0, "boguweird");
    ret += test31_1 (&longBstring, &t2, &t1, 27, "This is a bogus but reasonably long weirdtring.  Juweirdt long enough to cauweirde weirdome mallocing.");
    ret += test31_1 (&longBstring, &t3, &t4, 0, "This is a bogus but reasonably big string.  Just big enough to cause some mallocing.");
    ret += test31_1 (&longBstring, &t9, &t4, 0, "This is a bogus but reasonably big string.  Just big enough to cause some mallocing.");
    ret += test31_1 (&t6, &t2, &t5, 0, "sssstsssst");
    ret += test31_1 (&t6, &t8, &t5, 0, "sssstsssst");
    ret += test31_1 (&t7, &t2, &t5, 0, "xx" LOTS_OF_S LOTS_OF_S "xx");

    debug ("\t# failures: %d", ret);
    return ret;
}

static int test32_0 (const_bstring b, const char * s, int res) {
    int rv, ret = 0;

    ret += (res != (rv = biseqcstr (b, s)));
    debug (".\tbiseqcstr (%s, %p:<%s>) = %d", dumpBstring (b), s, (s ? s : NULL), rv);
    if (ret) {
        debug ("\t\tfailure(%d) = %d (res = %d)", __LINE__, ret, res);
    }
    return ret;
}

static int test32_1 (const_bstring b, const char * s, int res) {
    int rv, ret = 0;

    ret += (res != (rv = biseqcstrcaseless (b, s)));
    debug (".\tbiseqcstrcaseless (%s, %p:<%s>) = %d", dumpBstring (b), s, (s ? s : NULL), rv);
    if (ret) {
        debug ("\t\tfailure(%d) = %d (res = %d)", __LINE__, ret, res);
    }
    return ret;
}


static int test32 (void) {
    int ret = 0;

    debug ("TEST: int biseqcstr (const_bstring b, const char * s);");

    /* tests with NULL */
    ret += test32_0 (NULL, NULL, BSTR_ERR);
    ret += test32_0 (&emptyBstring, NULL, BSTR_ERR);
    ret += test32_0 (NULL, "", BSTR_ERR);
    ret += test32_0 (&badBstring1, "", BSTR_ERR);
    ret += test32_0 (&badBstring2, "bogus", BSTR_ERR);

    /* normal operation tests on all sorts of subranges */
    ret += test32_0 (&emptyBstring, "", 1);
    ret += test32_0 (&shortBstring, "bogus", 1);
    ret += test32_0 (&emptyBstring, "bogus", 0);
    ret += test32_0 (&shortBstring, "", 0);

    {
        bstring b = bstrcpy (&shortBstring);
        b->data[1]++;
        ret += test32_0 (b, (char *) shortBstring.data, 0);
        bdestroy (b);
    }

    debug ("TEST: int biseqcstrcaseless (const_bstring b, const char * s);");

    /* tests with NULL */
    ret += test32_1 (NULL, NULL, BSTR_ERR);
    ret += test32_1 (&emptyBstring, NULL, BSTR_ERR);
    ret += test32_1 (NULL, "", BSTR_ERR);
    ret += test32_1 (&badBstring1, "", BSTR_ERR);
    ret += test32_1 (&badBstring2, "bogus", BSTR_ERR);

    /* normal operation tests on all sorts of subranges */
    ret += test32_1 (&emptyBstring, "", 1);
    ret += test32_1 (&shortBstring, "bogus", 1);
    ret += test32_1 (&shortBstring, "BOGUS", 1);
    ret += test32_1 (&emptyBstring, "bogus", 0);
    ret += test32_1 (&shortBstring, "", 0);

    {
        bstring b = bstrcpy (&shortBstring);
        b->data[1]++;
        ret += test32_1 (b, (char *) shortBstring.data, 0);
        bdestroy (b);
    }

    if (ret) debug ("\t# failures: %d", ret);
    return ret;
}

static int test33_0 (bstring b0, const char * res) {
    bstring b2;
    int rv, ret = 0;

    if (b0 != NULL && b0->data != NULL && b0->slen >= 0) {
        b2 = bstrcpy (b0);
        bwriteprotect (*b2);

        debug (".\tbtoupper (%s)", dumpBstring (b2));

        rv = btoupper (b2);
        ret += (rv == 0);
        if (!biseq (b0, b2)) ret++;

        debug (" = %s", dumpBstring (b2));

        bwriteallow (*b2);

        debug (".\tbtoupper (%s)", dumpBstring (b2));

        rv = btoupper (b2);

        debug (" = %s", dumpBstring (b2));

        ret += (b2->slen != b0->slen);
        ret += (0 != rv);
        ret += (res == NULL) || ((int) strlen (res) != b2->slen) 
            || (0 != memcmp (b2->data, res, b2->slen));
        ret += b2->data[b2->slen] != '\0';
        bdestroy (b2);
    } else {
        ret += (BSTR_ERR != (rv = btoupper (b0)));
        debug (".\tbtoupper (%s) = %d", dumpBstring (b0), rv);
    }

    if (ret) {
        debug ("\t\tfailure(%d) = %d (res = %p", __LINE__, ret, res);
        if (res) debug (" = \"%s\"", res);
        debug (")");
    }
    return ret;
}

static int test33 (void) {
    int ret = 0;

    debug ("TEST: int btoupper (bstring b);");

    /* tests with NULL */
    ret += test33_0 (NULL, NULL);
    ret += test33_0 (&badBstring1, NULL);
    ret += test33_0 (&badBstring2, NULL);

    /* normal operation tests on all sorts of subranges */
    ret += test33_0 (&emptyBstring, "");
    ret += test33_0 (&shortBstring, "BOGUS");
    ret += test33_0 (&longBstring, "THIS IS A BOGUS BUT REASONABLY LONG STRING.  JUST LONG ENOUGH TO CAUSE SOME MALLOCING.");

    if (ret) debug ("\t# failures: %d", ret);
    return ret;
}

static int test34_0 (bstring b0, const char * res) {
    bstring b2;
    int rv, ret = 0;

    if (b0 != NULL && b0->data != NULL && b0->slen >= 0) {
        b2 = bstrcpy (b0);
        bwriteprotect (*b2);

        debug (".\tbtolower (%s)", dumpBstring (b2));

        rv = btolower (b2);
        ret += (rv == 0);
        if (!biseq (b0, b2)) ret++;

        debug (" = %s", dumpBstring (b2));

        bwriteallow (*b2);

        debug (".\tbtolower (%s)", dumpBstring (b2));

        rv = btolower (b2);

        debug (" = %s", dumpBstring (b2));

        ret += (b2->slen != b0->slen);
        ret += (0 != rv);
        ret += (res == NULL) || ((int) strlen (res) != b2->slen) 
            || (0 != memcmp (b2->data, res, b2->slen));
        ret += b2->data[b2->slen] != '\0';
        bdestroy (b2);
    } else {
        ret += (BSTR_ERR != (rv = btolower (b0)));
        debug (".\tbtolower (%s) = %d", dumpBstring (b0), rv);
    }

    if (ret) {
        debug ("\t\tfailure(%d) = %d (res = %p", __LINE__, ret, res);
        if (res) debug (" = \"%s\"", res);
        debug (")");
    }
    return ret;
}

static int test34 (void) {
    int ret = 0;

    debug ("TEST: int btolower (bstring b);");

    /* tests with NULL */
    ret += test34_0 (NULL, NULL);
    ret += test34_0 (&badBstring1, NULL);
    ret += test34_0 (&badBstring2, NULL);

    /* normal operation tests on all sorts of subranges */
    ret += test34_0 (&emptyBstring, "");
    ret += test34_0 (&shortBstring, "bogus");
    ret += test34_0 (&longBstring, "this is a bogus but reasonably long string.  just long enough to cause some mallocing.");

    if (ret) debug ("\t# failures: %d", ret);
    return ret;
}

static int test35_0 (const_bstring b0, const_bstring b1, int res) {
    int rv, ret = 0;

    ret += (res != (rv = bstricmp (b0, b1)));
    debug (".\tbstricmp (%s, %s) = %d", dumpBstring (b0), dumpBstring (b1), rv);
    if (ret) {
        debug ("\t\tfailure(%d) = %d (res = %d)", __LINE__, ret, res);
    }
    return ret;
}

static int test35 (void) {
    int ret = 0;
    struct tagbstring t0 = bsStatic ("bOgUs");
    struct tagbstring t1 = bsStatic ("bOgUR");
    struct tagbstring t2 = bsStatic ("bOgUt");

    debug ("TEST: int bstricmp (const_bstring b0, const_bstring b1);");

    /* tests with NULL */
    ret += test35_0 (NULL, NULL, SHRT_MIN);
    ret += test35_0 (&emptyBstring, NULL, SHRT_MIN);
    ret += test35_0 (NULL, &emptyBstring, SHRT_MIN);
    ret += test35_0 (&emptyBstring, &badBstring1, SHRT_MIN);
    ret += test35_0 (&badBstring1, &emptyBstring, SHRT_MIN);
    ret += test35_0 (&shortBstring, &badBstring2, SHRT_MIN);
    ret += test35_0 (&badBstring2, &shortBstring, SHRT_MIN);

    /* normal operation tests on all sorts of subranges */
    ret += test35_0 (&emptyBstring, &emptyBstring, 0);
    ret += test35_0 (&shortBstring, &t0, 0);
    ret += test35_0 (&shortBstring, &t1, tolower (shortBstring.data[4]) - tolower (t1.data[4]));
    ret += test35_0 (&shortBstring, &t2, tolower (shortBstring.data[4]) - tolower (t2.data[4]));

    t0.slen++;
    ret += test35_0 (&shortBstring, &t0, -(UCHAR_MAX+1));
    ret += test35_0 (&t0, &shortBstring,  (UCHAR_MAX+1));

    if (ret) debug ("\t# failures: %d", ret);
    return ret;
}

static int test36_0 (const_bstring b0, const_bstring b1, int n, int res) {
    int rv, ret = 0;

    ret += (res != (rv = bstrnicmp (b0, b1, n)));
    debug (".\tbstrnicmp (%s, %s, %d) = %d", dumpBstring (b0), dumpBstring (b1), n, rv);
    if (ret) {
        debug ("\t\tfailure(%d) = %d (res = %d)", __LINE__, ret, res);
    }
    return ret;
}

static int test36 (void) {
    int ret = 0;
    struct tagbstring t0 = bsStatic ("bOgUs");
    struct tagbstring t1 = bsStatic ("bOgUR");
    struct tagbstring t2 = bsStatic ("bOgUt");

    debug ("TEST: int bstrnicmp (const_bstring b0, const_bstring b1);");

    /* tests with NULL */
    ret += test36_0 (NULL, NULL, 0, SHRT_MIN);
    ret += test36_0 (&emptyBstring, NULL, 0, SHRT_MIN);
    ret += test36_0 (NULL, &emptyBstring, 0, SHRT_MIN);
    ret += test36_0 (&emptyBstring, &badBstring1, 0, SHRT_MIN);
    ret += test36_0 (&badBstring1, &emptyBstring, 0, SHRT_MIN);
    ret += test36_0 (&shortBstring, &badBstring2, 5, SHRT_MIN);
    ret += test36_0 (&badBstring2, &shortBstring, 5, SHRT_MIN);

    /* normal operation tests on all sorts of subranges */
    ret += test36_0 (&emptyBstring, &emptyBstring, 0, 0);
    ret += test36_0 (&shortBstring, &t0, 0, 0);
    ret += test36_0 (&shortBstring, &t0, 5, 0);
    ret += test36_0 (&shortBstring, &t0, 4, 0);
    ret += test36_0 (&shortBstring, &t0, 6, 0);
    ret += test36_0 (&shortBstring, &t1, 5, shortBstring.data[4] - t1.data[4]);
    ret += test36_0 (&shortBstring, &t1, 4, 0);
    ret += test36_0 (&shortBstring, &t1, 6, shortBstring.data[4] - t1.data[4]);
    ret += test36_0 (&shortBstring, &t2, 5, shortBstring.data[4] - t2.data[4]);
    ret += test36_0 (&shortBstring, &t2, 4, 0);
    ret += test36_0 (&shortBstring, &t2, 6, shortBstring.data[4] - t2.data[4]);

    t0.slen++;
    ret += test36_0 (&shortBstring, &t0, 5, 0);
    ret += test36_0 (&shortBstring, &t0, 6, -(UCHAR_MAX+1));
    ret += test36_0 (&t0, &shortBstring, 6,  (UCHAR_MAX+1));

    if (ret) debug ("\t# failures: %d", ret);
    return ret;
}

static int test37_0 (const_bstring b0, const_bstring b1, int res) {
    int rv, ret = 0;

    ret += (res != (rv = biseqcaseless (b0, b1)));
    debug (".\tbiseqcaseless (%s, %s) = %d", dumpBstring (b0), dumpBstring (b1), rv);
    if (ret) {
        debug ("\t\tfailure(%d) = %d (res = %d)", __LINE__, ret, res);
    }
    return ret;
}

static int test37 (void) {
    int ret = 0;
    struct tagbstring t0 = bsStatic ("bOgUs");
    struct tagbstring t1 = bsStatic ("bOgUR");
    struct tagbstring t2 = bsStatic ("bOgUt");

    debug ("TEST: int biseqcaseless (const_bstring b0, const_bstring b1);");

    /* tests with NULL */
    ret += test37_0 (NULL, NULL, BSTR_ERR);
    ret += test37_0 (&emptyBstring, NULL, BSTR_ERR);
    ret += test37_0 (NULL, &emptyBstring, BSTR_ERR);
    ret += test37_0 (&emptyBstring, &badBstring1, BSTR_ERR);
    ret += test37_0 (&badBstring1, &emptyBstring, BSTR_ERR);
    ret += test37_0 (&shortBstring, &badBstring2, BSTR_ERR);
    ret += test37_0 (&badBstring2, &shortBstring, BSTR_ERR);

    /* normal operation tests on all sorts of subranges */
    ret += test37_0 (&emptyBstring, &emptyBstring, 1);
    ret += test37_0 (&shortBstring, &t0, 1);
    ret += test37_0 (&shortBstring, &t1, 0);
    ret += test37_0 (&shortBstring, &t2, 0);

    if (ret) debug ("\t# failures: %d", ret);
    return ret;
}

struct emuFile {
    int ofs;
    bstring contents;
};

static int test38_aux_bNgetc (struct emuFile * f) {
    int v = EOF;
    if (NULL != f && EOF != (v = bchare (f->contents, f->ofs, EOF))) f->ofs++;
    return v;
}

static size_t test38_aux_bNread (void *buff, size_t elsize, size_t nelem, struct emuFile * f) {
    char * b = (char *) buff;
    int v;
    size_t i, j, c = 0;

    if (NULL == f || NULL == b) return c;
    for (i=0; i < nelem; i++) for (j=0; j < elsize; j++) {
        v = test38_aux_bNgetc (f);
        if (EOF == v) {
            *b = (char) '\0';
            return c;
        } else {
            *b = (char) v;
            b++;
            c++;
        }
    }

    return c;
}

static int test38_aux_bNopen (struct emuFile * f, bstring b) {
    if (NULL == f || NULL == b) return -__LINE__;
    f->ofs = 0;
    f->contents = b;
    return 0;
}

static int test38 (void) {
    struct emuFile f;
    bstring b0, b1, b2, b3;
    int ret = 0;

    debug ("TEST: bgets/breads test");

    test38_aux_bNopen (&f, &shortBstring);

    /* Creation/reads */

    b0 = bgets ((bNgetc) test38_aux_bNgetc, &f, (char) 'b');
    b1 = bread ((bNread) test38_aux_bNread, &f);
    b2 = bgets ((bNgetc) test38_aux_bNgetc, &f, (char) '\0');
    b3 = bread ((bNread) test38_aux_bNread, &f);

    ret += 1 != biseqcstr (b0, "b");
    ret += 1 != biseqcstr (b1, "ogus");
    ret += NULL != b2;
    ret += 1 != biseqcstr (b3, "");

    /* Bogus accumulations */

    f.ofs = 0;

    ret += 0 <= bgetsa (NULL, (bNgetc) test38_aux_bNgetc, &f, (char) 'o');
    ret += 0 <= breada (NULL, (bNread) test38_aux_bNread, &f);
    ret += 0 <= bgetsa (&shortBstring, (bNgetc) test38_aux_bNgetc, &f, (char) 'o');
    ret += 0 <= breada (&shortBstring, (bNread) test38_aux_bNread, &f);

    /* Normal accumulations */

    ret += 0 > bgetsa (b0, (bNgetc) test38_aux_bNgetc, &f, (char) 'o');
    ret += 0 > breada (b1, (bNread) test38_aux_bNread, &f);

    ret += 1 != biseqcstr (b0, "bbo");
    ret += 1 != biseqcstr (b1, "ogusgus");

    /* Attempt to append past end should do nothing */

    ret += 0 > bgetsa (b0, (bNgetc) test38_aux_bNgetc, &f, (char) 'o');
    ret += 0 > breada (b1, (bNread) test38_aux_bNread, &f);

    ret += 1 != biseqcstr (b0, "bbo");
    ret += 1 != biseqcstr (b1, "ogusgus");

    bdestroy (b0);
    bdestroy (b1);
    bdestroy (b2);
    bdestroy (b3);

    if (ret) debug ("\t# failures: %d", ret);
    return ret;
}

static int test39_0 (const_bstring b, const_bstring lt, const_bstring rt, const_bstring t) {
    bstring r;
    int ret = 0;

    ret += 0 <= bltrimws (NULL);
    ret += 0 <= brtrimws (NULL);
    ret += 0 <= btrimws (NULL);

    r = bstrcpy (b);
    bwriteprotect (*r);
    ret += 0 <= bltrimws (r);
    ret += 0 <= brtrimws (r);
    ret += 0 <= btrimws (r);
    bwriteallow(*r);
    ret += 0 != bltrimws (r);
    debug (".\tbltrim (%s) = %s", dumpBstring (b), dumpBstring (r));
    ret += !biseq (r, lt);
    bdestroy (r);

    r = bstrcpy (b);
    ret += 0 != brtrimws (r);
    debug (".\tbrtrim (%s) = %s", dumpBstring (b), dumpBstring (r));
    ret += !biseq (r, rt);
    bdestroy (r);

    r = bstrcpy (b);
    ret += 0 != btrimws (r);
    debug (".\tbtrim  (%s) = %s", dumpBstring (b), dumpBstring (r));
    ret += !biseq (r, t);
    bdestroy (r);

    return ret;
}

static int test39 (void) {
    int ret = 0;
    struct tagbstring t0 = bsStatic ("   bogus string   ");
    struct tagbstring t1 = bsStatic ("bogus string   ");
    struct tagbstring t2 = bsStatic ("   bogus string");
    struct tagbstring t3 = bsStatic ("bogus string");
    struct tagbstring t4 = bsStatic ("     ");
    struct tagbstring t5 = bsStatic ("");

    debug ("TEST: trim functions");

    ret += test39_0 (&t0, &t1, &t2, &t3);
    ret += test39_0 (&t1, &t1, &t3, &t3);
    ret += test39_0 (&t2, &t3, &t2, &t3);
    ret += test39_0 (&t3, &t3, &t3, &t3);
    ret += test39_0 (&t4, &t5, &t5, &t5);
    ret += test39_0 (&t5, &t5, &t5, &t5);

    if (ret) debug ("\t# failures: %d", ret);
    return ret;
}

static int test40_0 (bstring b0, const_bstring b1, int left, int len, const char * res) {
    bstring b2;
    int rv, ret = 0;

    if (b0 != NULL && b0->data != NULL && b0->slen >= 0 &&
            b1 != NULL && b1->data != NULL && b1->slen >= 0) {
        b2 = bstrcpy (b0);
        bwriteprotect (*b2);

        debug (".\tbassignmidstr (%s, ", dumpBstring (b2));

        rv = bassignmidstr (b2, b1, left, len);
        ret += (rv == 0);
        if (!biseq (b0, b2)) ret++;

        debug ("%s, %d, %d) = %s", dumpBstring (b1), left, len, dumpBstring (b2));

        bwriteallow (*b2);

        debug (".\tbassignmidstr (%s, ", dumpBstring (b2));

        rv = bassignmidstr (b2, b1, left, len);

        debug ("%s, %d, %d) = %s", dumpBstring (b1), left, len, dumpBstring (b2));

        if (b1) ret += (b2->slen > len) | (b2->slen < 0);
        ret += ((0 != rv) && (b1 != NULL)) || ((0 == rv) && (b1 == NULL));
        ret += (res == NULL) || ((int) strlen (res) != b2->slen) 
            || (0 != memcmp (b2->data, res, b2->slen));
        ret += b2->data[b2->slen] != '\0';
        bdestroy (b2);
    } else {
        ret += (BSTR_ERR != (rv = bassignmidstr (b0, b1, left, len)));
        debug (".\tbassignmidstr (%s, %s, %d, %d) = %d", dumpBstring (b0), dumpBstring (b1), left, len, rv);
    }

    if (ret) {
        debug ("\t\tfailure(%d) = %d (res = %p", __LINE__, ret, res);
        if (res) debug (" = \"%s\"", res);
        debug (")");
    }
    return ret;
}

static int test40 (void) {
    int ret = 0;

    debug ("TEST: int bassignmidstr (bstring b0, const_bstring b1, int left, int len);");

    /* tests with NULL */
    ret += test40_0 (NULL, NULL, 0, 1, NULL);
    ret += test40_0 (NULL, &emptyBstring, 0, 1, NULL);
    ret += test40_0 (&emptyBstring, NULL, 0, 1, "");
    ret += test40_0 (&badBstring1, &emptyBstring, 0, 1, NULL);
    ret += test40_0 (&badBstring2, &emptyBstring, 0, 1, NULL);
    ret += test40_0 (&emptyBstring, &badBstring1, 0, 1, NULL);
    ret += test40_0 (&emptyBstring, &badBstring2, 0, 1, NULL);

    /* normal operation tests on all sorts of subranges */
    ret += test40_0 (&emptyBstring, &emptyBstring, 0, 1, "");
    ret += test40_0 (&emptyBstring, &shortBstring, 1, 3, "ogu");
    ret += test40_0 (&shortBstring, &emptyBstring, 0, 1, "");
    ret += test40_0 (&shortBstring, &shortBstring, 1, 3, "ogu");
    ret += test40_0 (&shortBstring, &shortBstring, -1, 4, "bog");
    ret += test40_0 (&shortBstring, &shortBstring, 1, 9, "ogus");
    ret += test40_0 (&shortBstring, &shortBstring, 9, 1, "");

    debug ("\t# failures: %d", ret);
    return ret;
}

static int test41_0 (bstring b1, int left, int len, const char * res) {
    struct tagbstring t;
    bstring b2, b3;
    int ret = 0;

    if (b1 != NULL && b1->data != NULL && b1->slen >= 0) {
        b2 = bfromcstr ("");

        bassignmidstr (b2, b1, left, len);

        bmid2tbstr (t, b1, left, len);
        b3 = bstrcpy (&t);

        debug (".\tbmid2tbstr (%s, %d, %d) = %s", dumpBstring (b1), left, len, dumpBstring (b3));

        ret += !biseq (&t, b2);

        bdestroy (b2);
        bdestroy (b3);
    } else {
        bmid2tbstr (t, b1, left, len);
        b3 = bstrcpy (&t);
        ret += t.slen != 0;

        debug (".\tbmid2tbstr (%s, %d, %d) = %s", dumpBstring (b1), left, len, dumpBstring (b3));
        bdestroy (b3);
    }

    if (ret) {
        debug ("\t\tfailure(%d) = %d (res = %p", __LINE__, ret, res);
        if (res) debug (" = \"%s\"", res);
        debug (")");
    }
    return ret;
}

static int test41 (void) {
    int ret = 0;

    debug ("TEST: int bmid2tbstr (struct tagbstring &t, const_bstring b1, int left, int len);");

    /* tests with NULL */
    ret += test41_0 (NULL, 0, 1, NULL);
    ret += test41_0 (&emptyBstring, 0, 1, NULL);
    ret += test41_0 (NULL, 0, 1, "");
    ret += test41_0 (&emptyBstring, 0, 1, NULL);
    ret += test41_0 (&emptyBstring, 0, 1, NULL);
    ret += test41_0 (&badBstring1, 0, 1, NULL);
    ret += test41_0 (&badBstring2, 0, 1, NULL);

    /* normal operation tests on all sorts of subranges */
    ret += test41_0 (&emptyBstring, 0, 1, "");
    ret += test41_0 (&shortBstring, 1, 3, "ogu");
    ret += test41_0 (&emptyBstring, 0, 1, "");
    ret += test41_0 (&shortBstring, 1, 3, "ogu");
    ret += test41_0 (&shortBstring, -1, 4, "bog");
    ret += test41_0 (&shortBstring, 1, 9, "ogus");
    ret += test41_0 (&shortBstring, 9, 1, "");

    debug ("\t# failures: %d", ret);
    return ret;
}

static int test42_0 (const_bstring bi, int len, const char * res) {
    bstring b;
    int rv, ret = 0;

    rv = btrunc (b = bstrcpy (bi), len);
    ret += (len >= 0) ? (rv < 0) : (rv >= 0);
    if (res) ret += (0 == biseqcstr (b, res));
    debug (".\tbtrunc (%s, %d) = %s", dumpBstring (bi), len, dumpBstring (b));
    bdestroy (b);
    return ret;
}

static int test42 (void) {
    int ret = 0;

    debug ("TEST: int btrunc (bstring b, int n);");

    /* tests with NULL */
    ret += 0 <= btrunc (NULL, 2);
    ret += 0 <= btrunc (NULL, 0);
    ret += 0 <= btrunc (NULL, -1);

    /* write protected */
    ret += 0 <= btrunc (&shortBstring,  2);
    ret += 0 <= btrunc (&shortBstring,  0);
    ret += 0 <= btrunc (&shortBstring, -1);

    ret += test42_0 (&emptyBstring, 10, "");
    ret += test42_0 (&emptyBstring,  0, "");
    ret += test42_0 (&emptyBstring, -1, NULL);

    ret += test42_0 (&shortBstring, 10, "bogus");
    ret += test42_0 (&shortBstring,  3, "bog");
    ret += test42_0 (&shortBstring,  0, "");
    ret += test42_0 (&shortBstring, -1, NULL);

    debug ("\t# failures: %d", ret);
    return ret;
}

static int test43 (void) {
    static struct tagbstring ts0 = bsStatic ("");
    static struct tagbstring ts1 = bsStatic ("    ");
    static struct tagbstring ts2 = bsStatic (" abc");
    static struct tagbstring ts3 = bsStatic ("abc ");
    static struct tagbstring ts4 = bsStatic (" abc ");
    static struct tagbstring ts5 = bsStatic ("abc");
    bstring tstrs[6] = { &ts0, &ts1, &ts2, &ts3, &ts4, &ts5 };
    int ret = 0;
    int i;

    debug ("TEST: int btfromblk*trim (struct tagbstring t, void * s, int l);");

    for (i=0; i < 6; i++) {
        struct tagbstring t;
        bstring b;

        btfromblkltrimws (t, tstrs[i]->data, tstrs[i]->slen);
        bltrimws (b = bstrcpy (tstrs[i]));
        if (!biseq (b, &t)) {
            ret++;
            bassign (b, &t);
            debug ("btfromblkltrimws failure: <%s> -> <%s>", tstrs[i]->data, b->data);
        }
        debug (".\tbtfromblkltrimws (\"%s\", \"%s\", %d)", (char *) bdatae (b, NULL), tstrs[i]->data, tstrs[i]->slen);
        bdestroy (b);

        btfromblkrtrimws (t, tstrs[i]->data, tstrs[i]->slen);
        brtrimws (b = bstrcpy (tstrs[i]));
        if (!biseq (b, &t)) {
            ret++;
            bassign (b, &t);
            debug ("btfromblkrtrimws failure: <%s> -> <%s>", tstrs[i]->data, b->data);
        }
        debug (".\tbtfromblkrtrimws (\"%s\", \"%s\", %d)", (char *) bdatae (b, NULL), tstrs[i]->data, tstrs[i]->slen);
        bdestroy (b);

        btfromblktrimws (t, tstrs[i]->data, tstrs[i]->slen);
        btrimws (b = bstrcpy (tstrs[i]));
        if (!biseq (b, &t)) {
            ret++;
            bassign (b, &t);
            debug ("btfromblktrimws failure: <%s> -> <%s>", tstrs[i]->data, b->data);
        }
        debug (".\tbtfromblktrimws (\"%s\", \"%s\", %d)", (char *) bdatae (b, NULL), tstrs[i]->data, tstrs[i]->slen);
        bdestroy (b);
    }

    debug ("\t# failures: %d", ret);
    return ret;
}

static int test44_0 (const char * str) {
    int ret = 0, v;
    bstring b = bfromcstr("");

    if (NULL == str) {
        ret += 0 <= bassigncstr (NULL, "test");
        debug (".\tbassigncstr (b = %s, NULL)", dumpBstring (b = bfromcstr ("")));
        ret += 0 <= (v = bassigncstr (b, NULL));
        debug (" = %d; b -> %s", v, dumpBstring (b));
        ret += 0 <= bassigncstr (&shortBstring, NULL);
        bdestroy (b);
        return ret;
    }

    ret += 0 <= bassigncstr (NULL, str);
    debug (".\tbassigncstr (b = %s, \"%s\")", dumpBstring (b = bfromcstr ("")), str);
    ret += 0 > (v = bassigncstr (b, str));
    debug (" = %d; b -> %s", v, dumpBstring (b));
    ret += 0 != strcmp (bdatae (b, ""), str);
    ret += ((size_t) b->slen) != strlen (str);
    ret += 0 > bassigncstr (b, "xxxxx");
    bwriteprotect(*b)
        log_info(".\tbassigncstr (b = %s, \"%s\")", dumpBstring (b), str);
    ret += 0 <= (v = bassigncstr (b, str));
    debug (" = %d; b -> %s", v, dumpBstring (b));
    ret += 0 != strcmp (bdatae (b, ""), "xxxxx");
    ret += ((size_t) b->slen) != strlen ("xxxxx");
    bwriteallow(*b)
        ret += 0 <= bassigncstr (&shortBstring, str);
    bdestroy (b);
    debug (".\tbassigncstr (a = %s, \"%s\")", dumpBstring (&shortBstring), str);
    ret += 0 <= (v = bassigncstr (&shortBstring, str));
    debug (" = %d; a -> %s", v, dumpBstring (&shortBstring));
    return ret;
}

static int test44 (void) {
    int ret = 0;

    debug ("TEST: int bassigncstr (bstring a, char * str);");

    /* tests with NULL */
    ret += test44_0 (NULL);

    ret += test44_0 (EMPTY_STRING);
    ret += test44_0 (SHORT_STRING);
    ret += test44_0 (LONG_STRING);

    debug ("\t# failures: %d", ret);
    return ret;
}

static int test45_0 (const char * str) {
    int ret = 0, v, len;
    bstring b = bfromcstr("");
    if (NULL == str) {
        ret += 0 <= bassignblk (NULL, "test", 4);
        debug (".\tbassignblk (b = %s, NULL, 1)", dumpBstring (b = bfromcstr ("")));
        ret += 0 <= (v = bassignblk (b, NULL, 1));
        debug (" = %d; b -> %s", v, dumpBstring (b));
        ret += 0 <= bassignblk (&shortBstring, NULL, 1);
        bdestroy (b);
        return ret;
    }

    len = (int) strlen (str);
    ret += 0 <= bassignblk (NULL, str, len);
    debug (".\tbassignblk (b = %s, \"%s\", %d)", dumpBstring (b = bfromcstr ("")), str, len);
    ret += 0 > (v = bassignblk (b, str, len));
    debug (" = %d; b -> %s", v, dumpBstring (b));
    ret += 0 != strcmp (bdatae (b, ""), str);
    ret += b->slen != len;
    ret += 0 > bassigncstr (b, "xxxxx");
    bwriteprotect(*b)
        debug (".\tbassignblk (b = %s, \"%s\", %d)", dumpBstring (b), str, len);
    ret += 0 <= (v = bassignblk (b, str, len));
    debug (" = %d; b -> %s", v, dumpBstring (b));
    ret += 0 != strcmp (bdatae (b, ""), "xxxxx");
    ret += ((size_t) b->slen) != strlen ("xxxxx");
    bwriteallow(*b)
        ret += 0 <= bassignblk (&shortBstring, str, len);
    bdestroy (b);
    debug (".\tbassignblk (a = %s, \"%s\", %d)", dumpBstring (&shortBstring), str, len);
    ret += 0 <= (v = bassignblk (&shortBstring, str, len));
    debug (" = %d; a -> %s", v, dumpBstring (&shortBstring));
    return ret;
}

static int test45 (void) {
    int ret = 0;

    debug ("TEST: int bassignblk (bstring a, const void * s, int len);");

    /* tests with NULL */
    ret += test45_0 (NULL);

    ret += test45_0 (EMPTY_STRING);
    ret += test45_0 (SHORT_STRING);
    ret += test45_0 (LONG_STRING);

    debug ("\t# failures: %d", ret);
    return ret;
}

static int test46_0 (const_bstring r, bstring b, int count, const char * fmt, ...) {
    int ret;
    va_list arglist;

    debug (".\tbvcformata (%s, %d, \"%s\", ...) -> ", dumpBstring (b), count, fmt);
    va_start (arglist, fmt);
    ret = bvcformata (b, count, fmt, arglist);
    va_end (arglist);
    debug ("%d, %s (%s)", ret, dumpBstring (b), dumpBstring (r));
    if (ret < 0) return (NULL != r);
    ret += 1 != biseq (r, b);
    if (0 != ret) debug ("\t->failed");
    return ret;
}

static int test46_1 (bstring b, const char * fmt, const_bstring r, ...) {
    int ret;

    debug (".\tbvformata (&, %s, \"%s\", ...) -> ", dumpBstring (b), fmt);
    bvformata (ret, b, fmt, r);
    debug ("%d, %s (%s)", ret, dumpBstring (b), dumpBstring (r));
    if (ret < 0) return (NULL != r);
    ret += 1 != biseq (r, b);
    if (0 != ret) debug ("\t->failed");
    return ret;
}

static int test46 (void) {
    bstring b;
    int ret = 0;

    debug ("TEST: int bvcformata (bstring b, int count, const char * fmt, va_list arg);");

    ret += test46_0 (NULL, NULL, 8, "[%d]", 15);
    ret += test46_0 (NULL, &shortBstring, 8, "[%d]", 15);
    ret += test46_0 (NULL, &badBstring1, 8, "[%d]", 15);
    ret += test46_0 (NULL, &badBstring2, 8, "[%d]", 15);
    ret += test46_0 (NULL, &badBstring3, 8, "[%d]", 15);

    b = bfromcstr ("");
    ret += test46_0 (&shortBstring, b, shortBstring.slen, "%s", (char *) shortBstring.data);
    b->slen = 0;
    ret += test46_0 (&shortBstring, b, shortBstring.slen + 1, "%s", (char *) shortBstring.data);
    b->slen = 0;
    ret += test46_0 (NULL, b, shortBstring.slen-1, "%s", (char *) shortBstring.data);

    debug ("TEST: bvformata (int &ret, bstring b, const char * fmt, <type> lastarg);");

    ret += test46_1 (NULL, "[%d]", NULL, 15);
    ret += test46_1 (&shortBstring, "[%d]", NULL, 15);
    ret += test46_1 (&badBstring1, "[%d]", NULL, 15);
    ret += test46_1 (&badBstring2, "[%d]", NULL, 15);
    ret += test46_1 (&badBstring3, "[%d]", NULL, 15);

    b->slen = 0;
    ret += test46_1 (b, "%s", &shortBstring, (char *) shortBstring.data);

    b->slen = 0;
    ret += test46_1 (b, "%s", &longBstring, (char *) longBstring.data);

    bdestroy (b);
    debug ("\t# failures: %d", ret);
    return ret;
}

int taskmain (int argc, char * argv[]) 
{
    FILE *log_file = fopen("tests/tests.log", "a+");
    setbuf(log_file, NULL);
    dbg_set_log(log_file);

    int ret = 0;

    (void)(argc);
    (void)(argv);

    debug ("Direct case testing of bstring core functions");

    ret += test0 ();
    ret += test1 ();
    ret += test2 ();
    ret += test3 ();
    ret += test4 ();
    ret += test5 ();
    ret += test6 ();
    ret += test7 ();
    ret += test8 ();
    ret += test9 ();
    ret += test10 ();
    ret += test11 ();
    ret += test12 ();
    ret += test13 ();
    ret += test14 ();
    ret += test15 ();
    ret += test16 ();
    ret += test17 ();
    ret += test18 ();
    ret += test19 ();
    ret += test20 ();
    ret += test21 ();
    ret += test22 ();
    ret += test23 ();
    ret += test24 ();
    ret += test25 ();
    ret += test26 ();
    ret += test27 ();
    ret += test28 ();
    ret += test29 ();
    ret += test30 ();
    ret += test31 ();
    ret += test32 ();
    ret += test33 ();
    ret += test34 ();
    ret += test35 ();
    ret += test36 ();
    ret += test37 ();
    ret += test38 ();
    ret += test39 ();
    ret += test40 ();
    ret += test41 ();
    ret += test42 ();
    ret += test43 ();
    ret += test44 ();
    ret += test45 ();
    ret += test46 ();

    debug ("# test bstr failures: %d", ret);

    debug ("Direct case testing of bstraux functions");

    ret += testaux0 ();
    ret += testaux1 ();
    ret += testaux2 ();
    ret += testaux3 ();
    ret += testaux4 ();
    ret += testaux5 ();
    ret += testaux6 ();
    ret += testaux7 ();
    ret += testaux8 ();
    ret += testaux9 ();
    ret += testaux10 ();
    ret += testaux11 ();
    ret += testaux12 ();

    if(ret > 0) {
        printf("FAILED: %d failures in bstrlib", ret);
    }

    debug ("# test aux failures: %d", ret);

    return ret;
}
