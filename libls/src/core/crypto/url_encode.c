#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "url_encode.h"

static unsigned char hexchars[] = "0123456789ABCDEF";

static int ls_htoi(char* s)
{
    int value;
    int c;

    c = ((unsigned char*)s)[0];
    if (isupper(c))
        c = tolower(c);
    value = (c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10) * 16;

    c = ((unsigned char*)s)[1];
    if (isupper(c))
        c = tolower(c);
    value += c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10;

    return (value);
}


int url_encode(const char* s, int len, char* buff, int buff_size)
{
    register unsigned char c;
    unsigned char* to, *start;
    const unsigned char* from, *end;
    int new_length;

    from = (unsigned char*)s;
    end  = (unsigned char*)s + len;
    start = to = (unsigned char*) calloc(1, 3 * len + 1);

    while (from < end)
    {
        c = *from++;

        if (c == ' ')
        {
            *to++ = '+';
        }
        else if ((c < '0' && c != '-' && c != '.') ||
                 (c < 'A' && c > '9') ||
                 (c > 'Z' && c < 'a' && c != '_') ||
                 (c > 'z'))
        {
            to[0] = '%';
            to[1] = hexchars[c >> 4];
            to[2] = hexchars[c & 15];
            to += 3;
        }
        else
        {
            *to++ = c;
        }
    }
    *to = 0;

    new_length = to - start;
    if (buff != NULL)
        memcpy(buff, start, buff_size < new_length ? buff_size : new_length);
    free(start);
    return new_length;
}


int url_decode( char* str, int len)
{
    char* dest = str;
    char* data = str;

    while (len--)
    {
        if (*data == '+')
        {
            *dest = ' ';
        }
        else if (*data == '%' && len >= 2 && isxdigit((int) * (data + 1)) && isxdigit((int) * (data + 2)))
        {
            *dest = (char) ls_htoi(data + 1);
            data += 2;
            len -= 2;
        }
        else
        {
            *dest = *data;
        }
        data++;
        dest++;
    }
    *dest = 0;
    return dest - str;
}
