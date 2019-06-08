#ifndef BASE64_H
#define BASE64_H

#define estimate_base64_encode_len(s) ((unsigned int)((((s) + 2) / 3) * 4 + 1))
#define estimate_base64_decode_len(s) ((unsigned int)(((s) / 4) * 3))

#ifdef __cplusplus
extern "C" {
#endif
/*
 * out is null-terminated encode string.
 * return values is out length, exclusive terminating `\0'
 */
unsigned int base64_encode(const unsigned char *in, unsigned int inlen, char *out);

/*
 * return values is out length
 */
unsigned int base64_decode(const char *in, unsigned int inlen, unsigned char *out);

#ifdef __cplusplus
}
#endif

#endif /* BASE64_H */
