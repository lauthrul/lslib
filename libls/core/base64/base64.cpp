#include"stdafx.h"
#include"base64.h"

//////////////////////////////////////////////////////////////////////////
namespace lslib
{
    namespace base64
    {
        lstring encode(_lpcstr data, int len)
        {
            if (is_empty(data) || len <= 0) return "";

            //�����
            const char EncodeTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

            lstring strEncode;
            unsigned char Tmp[4] = { 0 };
            int LineLength = 0;
            for (int i = 0; i < (int)(len / 3); i++)
            {
                Tmp[1] = *data++;
                Tmp[2] = *data++;
                Tmp[3] = *data++;
                strEncode += EncodeTable[Tmp[1] >> 2];
                strEncode += EncodeTable[((Tmp[1] << 4) | (Tmp[2] >> 4)) & 0x3F];
                strEncode += EncodeTable[((Tmp[2] << 2) | (Tmp[3] >> 6)) & 0x3F];
                strEncode += EncodeTable[Tmp[3] & 0x3F];
                if (LineLength += 4, LineLength == 76) { strEncode += "\r\n"; LineLength = 0; }
            }

            //��ʣ�����ݽ��б���
            int Mod = len % 3;
            if (Mod == 1)
            {
                Tmp[1] = *data++;
                strEncode += EncodeTable[(Tmp[1] & 0xFC) >> 2];
                strEncode += EncodeTable[((Tmp[1] & 0x03) << 4)];
                strEncode += "==";
            }
            else if (Mod == 2)
            {
                Tmp[1] = *data++;
                Tmp[2] = *data++;
                strEncode += EncodeTable[(Tmp[1] & 0xFC) >> 2];
                strEncode += EncodeTable[((Tmp[1] & 0x03) << 4) | ((Tmp[2] & 0xF0) >> 4)];
                strEncode += EncodeTable[((Tmp[2] & 0x0F) << 2)];
                strEncode += "=";
            }

            return strEncode;
        }

        lstring decode(_lpcstr data, int len, __out__ int* outlen)
        {
            if (is_empty(data) || len <= 0) return "";

            //�����
            const char DecodeTable[] =
            {
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                62, // '+'
                0, 0, 0,
                63, // '/'
                52, 53, 54, 55, 56, 57, 58, 59, 60, 61, // '0'-'9'
                0, 0, 0, 0, 0, 0, 0,
                0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
                13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, // 'A'-'Z'
                0, 0, 0, 0, 0, 0,
                26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
                39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, // 'a'-'z'
            };

            string strDecode;
            int nValue;
            int i = 0;
            int nByte = 0;
            while (i < len)
            {
                if (*data != '\r' && *data != '\n')
                {
                    nValue = DecodeTable[*data++] << 18;
                    nValue += DecodeTable[*data++] << 12;
                    strDecode += (nValue & 0x00FF0000) >> 16;
                    nByte++;
                    if (*data != '=')
                    {
                        nValue += DecodeTable[*data++] << 6;
                        strDecode += (nValue & 0x0000FF00) >> 8;
                        nByte++;
                        if (*data != '=')
                        {
                            nValue += DecodeTable[*data++];
                            strDecode += nValue & 0x000000FF;
                            nByte++;
                        }
                    }
                    i += 4;
                }
                else// �س�����,����
                {
                    data++;
                    i++;
                }
            }
            if (outlen != NULL) *outlen = nByte;
            return strDecode;
        }
    }
}