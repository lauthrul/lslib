#pragma once

#ifdef USE_LIBCURL

#include "curl/curl.h"

namespace lslib
{
    namespace net
    {
        //////////////////////////////////////////////////////////////////////////
#define TRYCOUNT_HTTPGET                2
#define TRYCOUNT_HTTPPOST               2
#define TRYCOUNT_DOWNLOAD               2
#define TRYCOUNT_UPLOAD                 2

#define TIMEOUT_CONNECT                 3       //sec
#define TIMEOUT_HTTPGET                 5       //sec
#define TIMEOUT_HTTPPOST                15      //sec
#define TIMEOUT_HTTPDOWNLOAD            30      //sec
#define TIMEOUT_HTTPUPLOAD              15      //sec

#define GZIP_THRESHOLD_SIZE             (100*1024)

        //////////////////////////////////////////////////////////////////////////
        enum EHttpMethod
        {
            HTTP_GET,
            HTTP_POST,
            HTTP_DOWNLOAD,
            HTTP_UPLOAD,
        };
        inline string GetHttpMethodStr(EHttpMethod eMethod)
        {
            switch (eMethod)
            {
                case HTTP_GET: return "HTTP_GET";               break;
                case HTTP_POST: return "HTTP_POST";             break;
                case HTTP_DOWNLOAD: return "HTTP_DOWNLOAD";     break;
                case HTTP_UPLOAD: return "HTTP_UPLOAD";         break;
            }
            return "";
        }

        //////////////////////////////////////////////////////////////////////////
        struct SHttpParam
        {
            EHttpMethod             eMethod;
            string                 strUrl;
            map<string, string>   mapHeaders;
            int                     nConnetTimeout;
            int                     nPerformTimeout;
            int                     nTryCount;

            SHttpParam()
            {
                Reset();
            }
            SHttpParam(_lpcstr lpstrUrl)
            {
                Reset();
                strUrl = lpstrUrl;
            }
            void Reset()
            {
                eMethod = HTTP_GET;
                strUrl.clear();
                mapHeaders.clear();
                nConnetTimeout = TIMEOUT_CONNECT;
                nPerformTimeout = -1; // evaluate by program
                nTryCount = -1; // evaluate by program
            }
        };

        struct SHttpGetParam : public SHttpParam
        {
            SHttpGetParam()
            {
                Reset();
            }
            SHttpGetParam(_lpcstr lpstrUrl)
            {
                Reset();
                strUrl = lpstrUrl;
            }
            void Reset()
            {
                SHttpParam::Reset();
                eMethod = HTTP_GET;
                nPerformTimeout = TIMEOUT_HTTPGET;
                nTryCount = TRYCOUNT_HTTPGET;
            }
        };

        struct SHttpPostParam : public SHttpParam
        {
            string                 strPost;
            bool                    bgZip;
            int                     nThresholdSize;     // above this value, data will be gzip

            SHttpPostParam()
            {
                Reset();
            }
            SHttpPostParam(_lpcstr lpstrUrl, _lpcstr lpstrPost)
            {
                Reset();
                strUrl = lpstrUrl;
                strPost = lpstrPost;
            }
            void Reset()
            {
                SHttpParam::Reset();
                eMethod = HTTP_POST;
                nPerformTimeout = TIMEOUT_HTTPPOST;
                nTryCount = TRYCOUNT_HTTPPOST;
                strPost.clear();
                bgZip = false;
                nThresholdSize = GZIP_THRESHOLD_SIZE;
            }
        };

        typedef int (*DownloadFileCallback)(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow);

        struct SHttpDowloadParam : public SHttpParam
        {
            string                 strFile;
            string                 strToken;
            bool                    bBreakPointSupport;
            DownloadFileCallback    cb;
            void*                   clientp;

            SHttpDowloadParam()
            {
                Reset();
            }
            SHttpDowloadParam(_lpcstr lpstrUrl, _lpcstr lpstrFile)
            {
                Reset();
                strUrl = lpstrUrl;
                strFile = lpstrFile;
            }
            void Reset()
            {
                SHttpParam::Reset();
                eMethod = HTTP_DOWNLOAD;
                nPerformTimeout = TIMEOUT_HTTPDOWNLOAD;
                nTryCount = TRYCOUNT_DOWNLOAD;
                strFile.clear();
                strToken.clear();
                bBreakPointSupport = false;
                cb = NULL;
                clientp = NULL;
            }
        };

        struct SHttpUploadParam : public SHttpParam
        {
            string                 strFile;
            string                 strRemoteFile;

            SHttpUploadParam()
            {
                Reset();
            }
            SHttpUploadParam(_lpcstr lpstrUrl, _lpcstr lpstrFile)
            {
                Reset();
                strUrl = lpstrUrl;
                strFile = lpstrFile;
            }
            void Reset()
            {
                SHttpParam::Reset();
                eMethod = HTTP_UPLOAD;
                nPerformTimeout = TIMEOUT_HTTPUPLOAD;
                nTryCount = TRYCOUNT_UPLOAD;
                strFile.clear();
                strRemoteFile.clear();
            }
        };

        struct SHttpResult
        {
            int                     nCode;
            map<string, string>    mapHeaders;
            string                 strData;
            int                     nDataLen;
            int                     nTimeSpend; // million second

            SHttpResult()
            {
                Reset();
            }
            void Reset()
            {
                nCode = 0;
                mapHeaders.clear();
                strData.clear();
                nDataLen = 0;
            }
        };

        struct SHttpUrl
        {
            string strScheme;
            string strHostName;
            int nPort;
            string strPath;
            map<string, string> mapQuerys;
            string strTag;
        };
        LSLIB_API SHttpUrl          CrackUrl(_lpcstr lpstrUrl);
        LSLIB_API string           ReverseUrl(const SHttpUrl& vUrl);

        //////////////////////////////////////////////////////////////////////////
        class LSLIB_API CHttpClient
        {
        public:
            static void             Init(_lpcstr lpstrDefaultCookieFile = NULL, _lpcstr lpstrDefaultAgent = NULL);
            static bool             IsInit();
            static void             SetDefaultCookieFile(_lpcstr lpstrDefaultCookieFile);
            static void             SetDefaultAgent(_lpcstr lpstrDefaultAgent);

            static SHttpResult      HttpGet(const SHttpGetParam& vParam);
            static SHttpResult      HttpPost(const SHttpPostParam& vParam);
            static SHttpResult      DownloadFile(const SHttpDowloadParam& vParam);
            static SHttpResult      UploadFile(const SHttpUploadParam& vParam);

            // for debug print
        protected:
            static int              Perform(CURL* pCurl, const SHttpParam& vParam, __inout__ SHttpResult& vResult);
            static string          DumpParamText(SHttpParam* pParam);
            static string          DumpResultText(SHttpResult* pResult);

        private:
            static string          m_strDefaultAgent;
            static string          m_strDefaultCookieFile;
        };

        // for multi-thread calling CHttpClient, must call following two functions in main thread to avoid https multi-thread unsafety.
        extern LSLIB_API int httpclient_thread_setup(void);
        extern LSLIB_API int httpclient_thread_cleanup(void);

    } // namespace net

} // namespace lslib

#endif // endof USE_LIBCURL