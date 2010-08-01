#include <ctype.h>

int atoin(char *s, int len)
{
    int i, r = 0;
    for(i = 0; i < len; i++)
        r = (r * 10) + (s[i] - '0');
    return r;
}

int strnicmp(char *s1, char *s2, int len)
{
    int i; 
    for(i = 0; i < len && s1[i] && s2[i]; i++)
        if(toupper(s1[i]) != toupper(s2[i]))
            return s1[i] - s2[i];
    return 0;
}
