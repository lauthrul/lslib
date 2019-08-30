#include "stdafx.h"
#include "crypto.h"
#include "md5.h"
#include "sha1.h"
#include "sha256.h"
#include "base64.h"
#include "aes.h"
#include "url_encode.h"
#include <math.h>
#include "des.h"
#ifdef USE_LIBICONV
#include <iconv.h>
#endif

namespace lslib
{
    namespace crypto
    {
        LSLIB_API string hex_to_str(_lpbyte data, size_t len)
        {
            string strret;
            for (size_t i = 0; i < len; i++)
                strret += strtool::format("%02x", data[i]);
            return strret;
        }

        LSLIB_API string md5(_lpbyte data, size_t len)
        {
            MD5_CTX ctx;
            MD5Init(&ctx);
            MD5Update(&ctx, data, len);
            MD5Final(&ctx);
            return hex_to_str(ctx.digest, 16);
        }

        LSLIB_API string file_md5(_lpcstr pfile)
        {
            _ldword size = 0;
            _lpbyte data = os::get_file_buffer(pfile, &size);
            string strret = md5(data, size);
            os::release_file_buffer(data);
            return strret;
        }

        LSLIB_API string sha1(_lpbyte data, size_t len)
        {
            SHA1_CTX ctx;
            SHA1Init(&ctx);
            SHA1Update(&ctx, data, len);
            SHA1Final(&ctx);
            return hex_to_str(ctx.digest, 20);
        }

        LSLIB_API string file_sha1(_lpcstr pfile)
        {
            _ldword size = 0;
            _lpbyte data = os::get_file_buffer(pfile, &size);
            string strret = sha1(data, size);
            os::release_file_buffer(data);
            return strret;
        }

        LSLIB_API string sha224(_lpbyte data, size_t len)
        {
            return "";
        }

        LSLIB_API string file_sha224(_lpcstr pfile)
        {
            _ldword size = 0;
            _lpbyte data = os::get_file_buffer(pfile, &size);
            string strret = sha224(data, size);
            os::release_file_buffer(data);
            return strret;
        }

        LSLIB_API string sha256(_lpbyte data, size_t len)
        {
            uint8_t hash[SHA256_BYTES] = {0};
            ::sha256(data, len, hash);
            return hex_to_str(hash, SHA256_BYTES);
        }

        LSLIB_API string file_sha256(_lpcstr pfile)
        {
            _ldword size = 0;
            _lpbyte data = os::get_file_buffer(pfile, &size);
            string strret = sha256(data, size);
            os::release_file_buffer(data);
            return strret;
        }

        LSLIB_API string sha384(_lpbyte data, size_t len)
        {
            return "";
        }

        LSLIB_API string file_sha384(_lpcstr pfile)
        {
            _ldword size = 0;
            _lpbyte data = os::get_file_buffer(pfile, &size);
            string strret = sha384(data, size);
            os::release_file_buffer(data);
            return strret;
        }

        LSLIB_API string sha512(_lpbyte data, size_t len)
        {
            return "";
        }

        LSLIB_API string file_sha512(_lpcstr pfile)
        {
            _ldword size = 0;
            _lpbyte data = os::get_file_buffer(pfile, &size);
            string strret = sha512(data, size);
            os::release_file_buffer(data);
            return strret;
        }

        LSLIB_API string base64_encode(_lpbyte data, size_t len)
        {
            int outlen = estimate_base64_encode_len(len);
            _lpbyte pbuf = lsalloc(outlen + 1);
            outlen = ::base64_encode(data, len, (char*)pbuf);
            string strrect;
            if (outlen > 0) strrect.assign((_lpcstr)pbuf, outlen);
            lsfree(pbuf);
            return strrect;
        }

        LSLIB_API _lpbyte base64_decode(_lpcstr data, size_t len, __out__ int* out_len)
        {
            int estimated_out_len = estimate_base64_decode_len(len);
            _lpbyte pbuf = lsalloc(estimated_out_len + 1);
            estimated_out_len = ::base64_decode(data, len, pbuf);
            if (out_len != NULL) *out_len = estimated_out_len;
            return pbuf;
        }

        _lpbyte crypto_padding(__inout__ _lpbyte& data_buf, __inout__ int& data_len, int bock_size, crypto_padding_mode mode)
        {
            if (data_buf != NULL && data_len > 0)
            {
                int padding_len = bock_size - data_len % bock_size;
                data_buf = (_lpbyte)realloc(data_buf, data_len + padding_len);
                switch (mode)
                {
                    case crypto_zeropadding:
                        memset(data_buf + data_len, 0, padding_len);
                        break;
                    case crypto_pkcs5padding:
                    case crypto_pkcs7padding:
                        memset(data_buf + data_len, padding_len, padding_len);
                        break;
                }
                data_len += padding_len;
            }
            return data_buf;
        }

        _lpbyte crypto_unpadding(__inout__ _lpbyte& data_buf, __inout__ int& data_len, crypto_padding_mode mode)
        {
            if (data_buf != NULL && data_len > 0)
            {
                switch (mode)
                {
                    case crypto_zeropadding:
                        while (data_buf[--data_len] == 0 && data_len >= 0);
                        ++data_len;
                        break;
                    case crypto_pkcs5padding:
                    case crypto_pkcs7padding:
                        data_len -= data_buf[data_len - 1];
                        break;
                }
                data_buf = (_lpbyte)realloc(data_buf, data_len);
            }
            return data_buf;
        }

        LSLIB_API string des_encrypt(_lpcstr data,                 // data(string or byte array) to be encrypt
                                     int data_len,                 // data length in bytes
                                     _lpcstr key,                  // the key must be length of 64 bits (8 bytes)
                                     crypto_padding_mode mode,     // padding mode. only data_len is multiple of 16, crypto_nopadding can be set, otherwise the result will be uncertain.
                                     __out__ int* out_len)           // result length in bytes
        {
            uint8_t key_schedule[16][6] = {0};
            des_key_setup((uint8_t*)key, key_schedule, DES_ENCRYPT);

            _lpbyte data_buf = (_lpbyte)malloc(data_len);
            memcpy(data_buf, data, data_len);
            crypto_padding(data_buf, data_len, DES_BLOCK_SIZE, mode);

            for (int i = 0; i < data_len; i += DES_BLOCK_SIZE)
                ::des_crypt(data_buf + i, data_buf + i, key_schedule);

            string strret;
            strret.assign((_lpcstr)data_buf, data_len);
            free(data_buf);
            if (out_len) *out_len = data_len;
            return strret;
        }

        LSLIB_API string des_decrypt(_lpcstr data,                 // data(string or byte array) to be decrypt, data length must be multiple of 8.
                                     int data_len,                 // data length in bytes
                                     _lpcstr key,                  // the key must be length of 64 bits (8 bytes)
                                     crypto_padding_mode mode,     // padding mode. only data_len is multiple of 16, crypto_nopadding can be set, otherwise the result will be uncertain.
                                     __out__ int* out_len)           // result length in bytes
        {
            if (data_len % DES_BLOCK_SIZE != 0) return ""; // decrypt data size must be times of AES_BLOCK_SIZE

            uint8_t key_schedule[16][6] = {0};
            des_key_setup((uint8_t*)key, key_schedule, DES_DECRYPT);

            _lpbyte data_buf = (_lpbyte)malloc(data_len);
            memcpy(data_buf, data, data_len);

            for (int i = 0; i < data_len; i += DES_BLOCK_SIZE)
                ::des_crypt(data_buf + i, data_buf + i, key_schedule);

            crypto_unpadding(data_buf, data_len, mode);

            string strret;
            strret.assign((_lpcstr)data_buf, data_len);
            free(data_buf);
            if (out_len) *out_len = data_len;
            return strret;
        }

        LSLIB_API string des_encrypt_cbc(_lpcstr data,             // data(string or byte array) to be encrypt
                                         int data_len,             // data length in bytes
                                         _lpcstr key,              // the key must be length of 64 bits (8 bytes)
                                         crypto_padding_mode mode, // padding mode. only data_len is multiple of 8, crypto_nopadding can be set, otherwise the result will be uncertain.
                                         _lchar iv[8],             // init vector. must be length of 8 bytes
                                         __out__ int* out_len)       // result length in bytes
        {
            uint8_t key_schedule[16][6] = {0};
            des_key_setup((uint8_t*)key, key_schedule, DES_ENCRYPT);

            _lpbyte data_buf = (_lpbyte)malloc(data_len);
            memcpy(data_buf, data, data_len);
            crypto_padding(data_buf, data_len, DES_BLOCK_SIZE, mode);

            ::des_encrypt_cbc(data_buf, data_len, data_buf, key_schedule, (uint8_t*)iv);

            string strret;
            strret.assign((_lpcstr)data_buf, data_len);
            free(data_buf);
            if (out_len) *out_len = data_len;
            return strret;
        }

        // return data is a string or byte array (stored in string), check the length (out_len) before use.
        LSLIB_API string des_decrypt_cbc(_lpcstr data,             // data(string or byte array) to be decrypt, data length must be multiple of 8.
                                         int data_len,             // data length in bytes, must be multiple of 8.
                                         _lpcstr key,              // the key must be length of 64 bits (8 bytes)
                                         crypto_padding_mode mode, // padding mode. only data_len is multiple of 8, crypto_nopadding can be set, otherwise the result will be uncertain.
                                         _lchar iv[8],             // init vector. must be length of 8 bytes
                                         __out__ int* out_len)       // result length in bytes
        {
            if (data_len % DES_BLOCK_SIZE != 0) return ""; // decrypt data size must be times of AES_BLOCK_SIZE

            uint8_t key_schedule[16][6] = {0};
            des_key_setup((uint8_t*)key, key_schedule, DES_DECRYPT);

            _lpbyte data_buf = (_lpbyte)malloc(data_len);
            memcpy(data_buf, data, data_len);

            ::des_decrypt_cbc(data_buf, data_len, data_buf, key_schedule, (uint8_t*)iv);

            crypto_unpadding(data_buf, data_len, mode);

            string strret;
            strret.assign((_lpcstr)data_buf, data_len);
            free(data_buf);
            if (out_len) *out_len = data_len;
            return strret;
        }

        LSLIB_API string three_des_encrypt(_lpcstr data,                 // data(string or byte array) to be encrypt
                                           int data_len,                 // data length in bytes
                                           _lpcstr key,                  // the key must be length of 192 bits (24 bytes)
                                           crypto_padding_mode mode,     // padding mode. only data_len is multiple of 16, crypto_nopadding can be set, otherwise the result will be uncertain.
                                           __out__ int* out_len)           // result length in bytes
        {
            uint8_t key_schedule[3][16][6] = {0};
            three_des_key_setup((uint8_t*)key, key_schedule, DES_ENCRYPT);

            _lpbyte data_buf = (_lpbyte)malloc(data_len);
            memcpy(data_buf, data, data_len);
            crypto_padding(data_buf, data_len, DES_BLOCK_SIZE, mode);

            for (int i = 0; i < data_len; i += DES_BLOCK_SIZE)
                ::three_des_crypt(data_buf + i, data_buf + i, key_schedule);

            string strret;
            strret.assign((_lpcstr)data_buf, data_len);
            free(data_buf);
            if (out_len) *out_len = data_len;
            return strret;
        }

        LSLIB_API string three_des_decrypt(_lpcstr data,                 // data(string or byte array) to be decrypt, data length must be multiple of 8.
                                           int data_len,                 // data length in bytes
                                           _lpcstr key,                  // the key must be length of 192 bits (24 bytes)
                                           crypto_padding_mode mode,     // padding mode. only data_len is multiple of 16, crypto_nopadding can be set, otherwise the result will be uncertain.
                                           __out__ int* out_len)           // result length in bytes
        {
            if (data_len % DES_BLOCK_SIZE != 0) return ""; // decrypt data size must be times of AES_BLOCK_SIZE

            uint8_t key_schedule[3][16][6] = {0};
            three_des_key_setup((uint8_t*)key, key_schedule, DES_DECRYPT);

            _lpbyte data_buf = (_lpbyte)malloc(data_len);
            memcpy(data_buf, data, data_len);

            for (int i = 0; i < data_len; i += DES_BLOCK_SIZE)
                ::three_des_crypt(data_buf + i, data_buf + i, key_schedule);

            crypto_unpadding(data_buf, data_len, mode);

            string strret;
            strret.assign((_lpcstr)data_buf, data_len);
            free(data_buf);
            if (out_len) *out_len = data_len;
            return strret;
        }

        LSLIB_API string three_des_encrypt_cbc(_lpcstr data,             // data(string or byte array) to be encrypt
                                               int data_len,             // data length in bytes
                                               _lpcstr key,              // the key must be length of 64 bits (8 bytes)
                                               crypto_padding_mode mode, // padding mode. only data_len is multiple of 8, crypto_nopadding can be set, otherwise the result will be uncertain.
                                               _lchar iv[8],             // init vector. must be length of 8 bytes
                                               __out__ int* out_len)       // result length in bytes
        {
            uint8_t key_schedule[3][16][6] = {0};
            three_des_key_setup((uint8_t*)key, key_schedule, DES_ENCRYPT);

            _lpbyte data_buf = (_lpbyte)malloc(data_len);
            memcpy(data_buf, data, data_len);
            crypto_padding(data_buf, data_len, DES_BLOCK_SIZE, mode);

            ::three_des_encrypt_cbc(data_buf, data_len, data_buf, key_schedule, (uint8_t*)iv);

            string strret;
            strret.assign((_lpcstr)data_buf, data_len);
            free(data_buf);
            if (out_len) *out_len = data_len;
            return strret;
        }

        LSLIB_API string three_des_decrypt_cbc(_lpcstr data,             // data(string or byte array) to be decrypt, data length must be multiple of 8.
                                               int data_len,             // data length in bytes, must be multiple of 8.
                                               _lpcstr key,              // the key must be length of 64 bits (8 bytes)
                                               crypto_padding_mode mode, // padding mode. only data_len is multiple of 8, crypto_nopadding can be set, otherwise the result will be uncertain.
                                               _lchar iv[8],             // init vector. must be length of 8 bytes
                                               __out__ int* out_len)       // result length in bytes
        {
            if (data_len % DES_BLOCK_SIZE != 0) return ""; // decrypt data size must be times of AES_BLOCK_SIZE

            uint8_t key_schedule[3][16][6] = {0};
            three_des_key_setup((uint8_t*)key, key_schedule, DES_DECRYPT);

            _lpbyte data_buf = (_lpbyte)malloc(data_len);
            memcpy(data_buf, data, data_len);

            ::three_des_decrypt_cbc(data_buf, data_len, data_buf, key_schedule, (uint8_t*)iv);

            crypto_unpadding(data_buf, data_len, mode);

            string strret;
            strret.assign((_lpcstr)data_buf, data_len);
            free(data_buf);
            if (out_len) *out_len = data_len;
            return strret;
        }


        LSLIB_API string aes_encrypt(_lpcstr data, int data_len, _lpcstr key, crypto_key_bits key_bits, crypto_padding_mode mode, __out__ int* out_len)
        {
            uint32_t key_schedule[60] = {0};
            aes_key_setup((uint8_t*)key, key_schedule, key_bits);

            _lpbyte data_buf = (_lpbyte)malloc(data_len);
            memcpy(data_buf, data, data_len);
            crypto_padding(data_buf, data_len, AES_BLOCK_SIZE, mode);

            for (int i = 0; i < data_len; i += AES_BLOCK_SIZE)
                ::aes_encrypt(data_buf + i, data_buf + i, key_schedule, key_bits);

            string strret;
            strret.assign((_lpcstr)data_buf, data_len);
            free(data_buf);
            if (out_len) *out_len = data_len;
            return strret;
        }

        LSLIB_API string aes_decrypt(_lpcstr data, int data_len, _lpcstr key, crypto_key_bits key_bits, crypto_padding_mode mode, __out__ int* out_len)
        {
            if (data_len % AES_BLOCK_SIZE != 0) return ""; // decrypt data size must be times of AES_BLOCK_SIZE

            uint32_t key_schedule[60] = {0};
            aes_key_setup((uint8_t*)key, key_schedule, key_bits);

            _lpbyte data_buf = (_lpbyte)malloc(data_len);
            memcpy(data_buf, data, data_len);

            for (int i = 0; i < data_len; i += AES_BLOCK_SIZE)
                ::aes_decrypt(data_buf + i, data_buf + i, key_schedule, key_bits);

            crypto_unpadding(data_buf, data_len, mode);

            string strret;
            strret.assign((_lpcstr)data_buf, data_len);
            free(data_buf);
            if (out_len) *out_len = data_len;
            return strret;
        }

        LSLIB_API string aes_encrypt_cbc(_lpcstr data, int data_len, _lpcstr key, crypto_key_bits key_bits, crypto_padding_mode mode, _lchar iv[16], __out__ int* out_len)
        {
            uint32_t key_schedule[60] = {0};
            aes_key_setup((uint8_t*)key, key_schedule, key_bits);

            _lpbyte data_buf = (_lpbyte)malloc(data_len);
            memcpy(data_buf, data, data_len);
            crypto_padding(data_buf, data_len, AES_BLOCK_SIZE, mode);

            ::aes_encrypt_cbc(data_buf, data_len, data_buf, key_schedule, key_bits, (uint8_t*)iv);

            string strret;
            strret.assign((_lpcstr)data_buf, data_len);
            free(data_buf);
            if (out_len) *out_len = data_len;
            return strret;
        }

        LSLIB_API string aes_decrypt_cbc(_lpcstr data, int data_len, _lpcstr key, crypto_key_bits key_bits, crypto_padding_mode mode, _lchar iv[16], __out__ int* out_len)
        {
            if (data_len % AES_BLOCK_SIZE != 0) return ""; // decrypt data size must be times of AES_BLOCK_SIZE

            uint32_t key_schedule[60] = {0};
            aes_key_setup((uint8_t*)key, key_schedule, key_bits);

            _lpbyte data_buf = (_lpbyte)malloc(data_len);
            memcpy(data_buf, data, data_len);

            ::aes_decrypt_cbc(data_buf, data_len, data_buf, key_schedule, key_bits, (uint8_t*)iv);

            crypto_unpadding(data_buf, data_len, mode);

            string strret;
            strret.assign((_lpcstr)data_buf, data_len);
            free(data_buf);
            if (out_len) *out_len = data_len;
            return strret;
        }

        LSLIB_API string url_encode(_lpcstr data, int len)
        {
            int out_len = ::url_encode(data, len, NULL, 0);
            char* pbuf = new char[out_len + 1];
            memset(pbuf, 0, out_len + 1);
            ::url_encode(data, len, pbuf, out_len);
            string str; str.assign(pbuf, out_len);
            delete[] pbuf;
            return str;
        }

        LSLIB_API string url_decode(_lpcstr data, int len)
        {
            char* pbuf = new char[len + 1];
            memset(pbuf, 0, len + 1);
            memcpy(pbuf, data, len);
            int out_len = ::url_decode(pbuf, len);
            string str; str.assign(pbuf, out_len);
            delete[] pbuf;
            return str;
        }

#ifdef USE_LIBICONV
        LSLIB_API int encoding_convert(_lpcstr from_charset, _lpcstr to_charset, _lpcstr inbuf, size_t inlen, __inout__ _lpstr outbuf, __inout__ size_t outlen)
        {
            iconv_t cd;
            const char** pin = &inbuf;
            char** pout = &outbuf;
            cd = iconv_open(to_charset, from_charset);
            if (cd <= 0) return -1;
            memset(outbuf, 0, outlen);
            int ret = iconv(cd, pin, &inlen, pout, &outlen);
            iconv_close(cd);
            return ret;
        }

        LSLIB_API string encoding_convert(_lpcstr data, _lpcstr from_charset, _lpcstr to_charset)
        {
            size_t buff_size = 4 * strlen(data);
            _lpstr buf = (_lpstr)malloc(buff_size);
            memset(buf, 0, buff_size);
            encoding_convert(from_charset, to_charset, data, strlen(data), buf, buff_size);
            string strret; strret.assign(buf, buff_size);
            free(buf);
            return strret;
        }
#endif

    } // crypto

} // lslib