#pragma once

#include "http_common.h"
#include "http_parser_impl.h"
#include "http_response.h"

namespace APie
{

    class HttpResponseDecoder : public HttpParser
    {
    public:
        HttpResponseDecoder();
        ~HttpResponseDecoder();

		void setConnectSession(class ClientConnection *ptrSession);

        static void setMaxHeaderSize(size_t size) { ms_maxHeaderSize = size; }
        static void setMaxBodySize(size_t size) { ms_maxBodySize = size; }
        
    private:
        int onMessageBegin();
        int onHeader(const std::string& field, const std::string& value);
        int onHeadersComplete();
        int onBody(const char* at, size_t length);
        int onMessageComplete();

        int onError(const HttpError& error);

    private:
		HttpResponse *m_response_ptr;
		class ClientConnection *m_session_ptr;

        static size_t ms_maxHeaderSize;
        static size_t ms_maxBodySize;
    };
    
}
