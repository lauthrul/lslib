#include "stdafx.h"
#include "cmdline.h"

//////////////////////////////////////////////////////////////////////////
namespace lslib
{
    namespace utils
    {
        int ICmdRuntine::ParserCmdLine(lpcstr cmd, __out__ string_array& outarr)
        {
            if (strtool::is_empty(cmd)) return 0;

            lchar buf[MAX_PATH + 1] = {0};
            lpcstr p = cmd;
            lpcstr q = p;
            do
            {
                while (isspace(*q)) // skip blank
                    q++;

                p = q;
                if (*p == '\"' || *p == '\'')
                {
                    q = strstr(p + 1, string(1, *p).c_str());
                    p++;
                }
                else
                {
                    q = strstr(p, " ");
                }

                if (q == NULL)
                    break;

                memset(buf, 0, MAX_PATH + 1);
                strncpy(buf, p, MIN(q - p, MAX_PATH));
                if (find(outarr.begin(), outarr.end(), buf) == outarr.end())
                    outarr.push_back(buf);

                if (*q == '\"' || *q == '\'') // skip " and '
                    q++;
            }
            while (q != NULL);

            if (p != NULL && *p != 0)
                if (find(outarr.begin(), outarr.end(), p) == outarr.end())
                    outarr.push_back(p);

            return outarr.size();
        }

    } // utils

} // lslib
