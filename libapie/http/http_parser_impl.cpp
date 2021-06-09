#include "http_parser_impl.h"

#include "http_parser_settings.h"
#include "http_util.h"

#include "../network/logger.h"
#include "../network/ctx.h"

namespace APie
{

	HttpParser::HttpParser(enum http_parser_type type)
	{
		http_parser_init(&m_parser, type);

		m_parser.data = this;
		HttpParserSettings::initSettings(m_settings);

		m_lastWasValue = true;
	}

	HttpParser::~HttpParser()
	{

	}

    int HttpParser::onParser(Event event, const char* at, size_t length)
    {
        switch(event)
        {
            case Parser_MessageBegin:
                return handleMessageBegin();
            case Parser_Url:
                return onUrl(at, length);
            case Parser_Status:
                return onStatus(at, length);
            case Parser_HeaderField:
                return handleHeaderField(at, length);
            case Parser_HeaderValue:
                return handleHeaderValue(at, length);
            case Parser_HeadersComplete:
                return handleHeadersComplete();
            case Parser_Body:
                return onBody(at, length);
            case Parser_MessageComplete:
                return onMessageComplete();
            default:
                break;
        }

        return 0;
    }

    int HttpParser::handleMessageBegin()
    {
        m_curField.clear();
        m_curValue.clear();
        m_lastWasValue = true;
        
        m_errorCode = 0;

        return onMessageBegin();
    }        
        
    int HttpParser::handleHeaderField(const char* at, size_t length)
    {
        if(m_lastWasValue)
        {
            if(!m_curField.empty())
            {  
                onHeader(HttpUtil::normalizeHeader(m_curField), m_curValue);
            }
            
            m_curField.clear();    
            m_curValue.clear();
        }

        m_curField.append(at, length);

        m_lastWasValue = 0;

        return 0;
    }
        
    int HttpParser::handleHeaderValue(const char* at, size_t length)
    {
        m_curValue.append(at, length);
        m_lastWasValue = 1;

        return 0;
    }
        
    int HttpParser::handleHeadersComplete()
    {
        if(!m_curField.empty())
        {
            std::string field = HttpUtil::normalizeHeader(m_curField); 
            onHeader(field, m_curValue);    
        }    

        return onHeadersComplete();
    }

    int HttpParser::execute(const char* buffer, size_t count)
    {
		size_t n = http_parser_execute(&m_parser, &m_settings, buffer, count);
        if(m_parser.upgrade)
        {
            onUpgrade(buffer + n, count - n); 
            return 400;
        }
        else if(n != count)
        {
            int code = (m_errorCode != 0 ? m_errorCode : 400);
            
            HttpError error(code, http_errno_description((http_errno)m_parser.http_errno));

			asyncPieLog("Http/parser", PIE_CYCLE_DAY, PIE_ERROR, "parser error %s", error.message.c_str());
            
            onError(error);

            return code;
        }     

        return 0;
    }
}

