#pragma once

#include <map>
#include <string>

extern "C"
{
#include "../../third_party/http-parser/http_parser.h" 
}

#include "http_parser_impl.h"
#include "http_request.h"

namespace APie
{
    class HttpRequest;

    class HttpRequestDecoder : public HttpParser
    {
    public:
		HttpRequestDecoder();
        ~HttpRequestDecoder();

		void setSession(class ServerConnection *ptrSession);

        static void setMaxHeaderSize(size_t size) { ms_maxHeaderSize = size; }
        static void setMaxBodySize(size_t size) { ms_maxBodySize = size; }
    
    private:
        int onMessageBegin();
        int onUrl(const char* at, size_t length);
        int onHeader(const std::string& field, const std::string& value);
        int onHeadersComplete();
        int onBody(const char* at, size_t length);
        int onMessageComplete();

        int onUpgrade(const char* at, size_t length);
        int onError(const HttpError& error);

    private:

        HttpRequest *m_request_ptr;    
		class ServerConnection *m_session_ptr;
  
        static size_t ms_maxHeaderSize;
        static size_t ms_maxBodySize;
    };    
}

