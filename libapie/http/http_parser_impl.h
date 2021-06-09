#pragma once

#include <string>

extern "C"
{
#include "../../third_party/http-parser/http_parser.h"    
}

#include "http_common.h"
#include "../common/noncopyable.h"

namespace APie
{
    class HttpParser : public Noncopyable
    {
    public:
        friend class HttpParserSettings;

        HttpParser(enum http_parser_type type);
        virtual ~HttpParser();

        enum http_parser_type getType() { return (http_parser_type)m_parser.type; }

        enum Event
        {
            Parser_MessageBegin,    
            Parser_Url,
			Parser_Status,
            Parser_HeaderField,
            Parser_HeaderValue,
            Parser_HeadersComplete,
            Parser_Body,
            Parser_MessageComplete,
        };

        int execute(const char* buf, size_t count);

    protected:
        virtual int onMessageBegin() { return 0; }
        virtual int onUrl(const char*, size_t) { return 0; }
		virtual int onStatus(const char*, size_t) { return 0; }
        virtual int onHeader(const std::string& field, const std::string& value) { return 0; }
        virtual int onHeadersComplete() { return 0; }
        virtual int onBody(const char*, size_t) { return 0; }
        virtual int onMessageComplete() { return 0; }

        virtual int onUpgrade(const char*, size_t) { return 0; }
        virtual int onError(const HttpError& error) { return 0; }

    private:
        int onParser(Event, const char*, size_t);
    
        int handleMessageBegin();
        int handleHeaderField(const char*, size_t);
        int handleHeaderValue(const char*, size_t);
        int handleHeadersComplete();

    protected:
        struct http_parser m_parser;
		struct http_parser_settings m_settings;

        std::string m_curField;
        std::string m_curValue;
        bool m_lastWasValue; 
        
        int m_errorCode;    
    };   
}
