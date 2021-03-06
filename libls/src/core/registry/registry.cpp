#include "stdafx.h"
#include "registry.h"

//////////////////////////////////////////////////////////////////////////
namespace lslib
{
    namespace registry
    {
        struct reg_ctx
        {
            DWORD  type;
            LPBYTE data;
            DWORD  size;
        };

        static LPBYTE malloc_registry_buffer(DWORD size)
        {
            LPBYTE buf = new BYTE[size];
            memset(buf, 0, size);
            return buf;
        }

        static void release_registry_buffer(LPBYTE buf)
        {
            delete[] buf;
        }

        static long registry_get(HKEY hKeyRoot, lpcstr lpSubKey, lpcstr lpValueName, __out__ reg_ctx* ctx)
        {
            LSTATUS ret = ERROR_SUCCESS;
            HKEY hKey;
            ret = RegOpenKey(hKeyRoot, _T(lpSubKey), &hKey);
            if (ret == ERROR_SUCCESS)
            {
                DWORD dwType = REG_SZ;
                DWORD dwSize = 0;
                ret = RegQueryValueEx(hKey, _T(lpValueName), NULL, &dwType, NULL, &dwSize);
                if (ret == ERROR_SUCCESS)
                {

                    LPBYTE buf = malloc_registry_buffer(dwSize); // should call release_registry_buffer() outside
                    ret = RegQueryValueEx(hKey, _T(lpValueName), NULL, &dwType, buf, &dwSize);
                    if (ret == ERROR_SUCCESS)
                    {
                        if (ctx != NULL)
                        {
                            ctx->type = dwType;
                            ctx->data = buf;
                            ctx->size = dwSize;
                        }
                    }
                }
            }
            RegCloseKey(hKey);
            return ret;
        }

        static long registry_set(HKEY hKeyRoot, lpcstr lpSubKey, lpcstr lpValueName, const reg_ctx& ctx, bool bCreate = true)
        {
            LSTATUS ret = ERROR_SUCCESS;
            HKEY hKey;
            ret = RegOpenKey(hKeyRoot, _T(lpSubKey), &hKey);
            if (ret != ERROR_SUCCESS)
            {
                if (!bCreate) return ret;

                ret = RegCreateKey(hKeyRoot, _T(lpSubKey), &hKey);
                if (ret != ERROR_SUCCESS) return ret;
            }
            ret = RegSetValueEx(hKey, _T(lpValueName), NULL, ctx.type, (LPBYTE)ctx.data, ctx.size);
            RegCloseKey(hKey);
            return ret;
        }

        LSLIB_API string registry_get_str(HKEY hKeyRoot, lpcstr lpSubKey, lpcstr lpValueName)
        {
            string strret;
            reg_ctx ctx;
            if (registry_get(hKeyRoot, lpSubKey, lpValueName, &ctx) == ERROR_SUCCESS)
            {
                if (ctx.type == REG_SZ || ctx.type == REG_EXPAND_SZ || ctx.type == REG_MULTI_SZ)
                {
                    strret.assign((lpcstr)ctx.data, ctx.size);
                }
                release_registry_buffer(ctx.data);
            }
            return strret;
        }

        LSLIB_API DWORD registry_get_dword(HKEY hKeyRoot, lpcstr lpSubKey, lpcstr lpValueName)
        {
            DWORD dwRet = 0;
            reg_ctx ctx;
            if (registry_get(hKeyRoot, lpSubKey, lpValueName, &ctx) == ERROR_SUCCESS)
            {
                if (ctx.size == 4)
                {
                    if (ctx.type == REG_DWORD || ctx.type == REG_DWORD_LITTLE_ENDIAN)
                    {
                        dwRet = (DWORD)ctx.data[0];
                        dwRet |= ((DWORD)ctx.data[1] << 8);
                        dwRet |= ((DWORD)ctx.data[2] << 16);
                        dwRet |= ((DWORD)ctx.data[3] << 24);
                    }
                    else if (ctx.type == REG_DWORD_BIG_ENDIAN)
                    {
                        dwRet = (DWORD)ctx.data[3];
                        dwRet |= ((DWORD)ctx.data[2] << 8);
                        dwRet |= ((DWORD)ctx.data[1] << 16);
                        dwRet |= ((DWORD)ctx.data[0] << 24);
                    }
                }
                release_registry_buffer(ctx.data);
            }
            return dwRet;
        }

        LSLIB_API long registry_set_str(HKEY hKeyRoot, lpcstr lpSubKey, lpcstr lpValueName, lpcstr lpValue, bool bCreate /*= true*/)
        {
            reg_ctx ctx;
            ctx.type = REG_SZ;
            ctx.data = (LPBYTE)lpValue;
            ctx.size = strlen(lpValue);
            if (strstr(lpValue, "\r") != NULL || strstr(lpValue, "\n") != NULL)
                ctx.type = REG_MULTI_SZ;
            else if (strstr(lpValue, "%%") != NULL)
                ctx.type = REG_EXPAND_SZ;
            return registry_set(hKeyRoot, lpSubKey, lpValueName, ctx, bCreate);
        }

        LSLIB_API long registry_set_dword(HKEY hKeyRoot, lpcstr lpSubKey, lpcstr lpValueName, DWORD dwValue, bool bCreate /*= true*/)
        {
            reg_ctx ctx;
            ctx.size = sizeof(DWORD);
            ctx.data = malloc_registry_buffer(ctx.size);
            int i = 1;
            if (((char*)&i)[0] == 1) // little endian
            {
                ctx.type = REG_DWORD;
                ctx.data[0] = dwValue & 0xff;
                ctx.data[1] = (dwValue >> 8) & 0xff;
                ctx.data[2] = (dwValue >> 16) & 0xff;
                ctx.data[3] = (dwValue >> 24) & 0xff;
            }
            else
            {
                ctx.type = REG_DWORD_BIG_ENDIAN;
                ctx.data[3] = dwValue & 0xff;
                ctx.data[2] = (dwValue >> 8) & 0xff;
                ctx.data[1] = (dwValue >> 16) & 0xff;
                ctx.data[0] = (dwValue >> 24) & 0xff;
            }
            long ret = registry_set(hKeyRoot, lpSubKey, lpValueName, ctx, bCreate);
            release_registry_buffer(ctx.data);
            return ret;
        }

        LSLIB_API long registry_delete_value(HKEY hKeyRoot, lpcstr lpSubKey, lpcstr lpValueName)
        {
            LSTATUS ret = ERROR_SUCCESS;
            HKEY hKey;
            ret = RegOpenKey(hKeyRoot, _T(lpSubKey), &hKey);
            if (ret != ERROR_SUCCESS) return ret;

            ret = RegDeleteValue(hKey, lpValueName);
            RegCloseKey(hKey);
            return ret;
        }

        LSLIB_API long registry_delete_key(HKEY hKeyRoot, lpcstr lpSubKey)
        {
            return SHDeleteKey(hKeyRoot, lpSubKey);
        }

    } // registry

} // lslib
