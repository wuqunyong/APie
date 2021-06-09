#include <string>
#include <sstream>

#include "http_request_decoder.h"

#include "http_parser_impl.h"
#include "http_request.h"

#include "../network/server_connection.h"
#include "../network/command.h"
#include "../network/ctx.h"
#include "../network/logger.h"

#include "../common/string_utils.h"

namespace APie
{
    size_t HttpRequestDecoder::ms_maxHeaderSize = 80 * 1024;
    size_t HttpRequestDecoder::ms_maxBodySize = 10 * 1024 * 1024;

	HttpRequestDecoder::HttpRequestDecoder():
		HttpParser(HTTP_REQUEST)
        , m_request_ptr(NULL)
		, m_session_ptr(NULL)
    {
		m_request_ptr = new HttpRequest();
    }

	HttpRequestDecoder::~HttpRequestDecoder()
    {
		if (NULL != m_request_ptr)
		{
			delete m_request_ptr;
			m_request_ptr = NULL;
		}
    }

	void HttpRequestDecoder::setSession(class ServerConnection *ptrSession)
	{
		m_session_ptr = ptrSession;
	}

    int HttpRequestDecoder::onMessageBegin()
    {
		m_request_ptr->clear();
        return 0;    
    }

    int HttpRequestDecoder::onUrl(const char* at, size_t length)
    {
		m_request_ptr->url.append(at, length);
        return 0;
    }

    int HttpRequestDecoder::onHeader(const std::string& field, const std::string& value)
    {        
        if(m_parser.nread >= ms_maxHeaderSize)
        {
            m_errorCode = 413;
            return -1;         
        }
    
 
		m_request_ptr->headers.insert(make_pair(field, value));
        return 0;
    }

    int HttpRequestDecoder::onBody(const char* at, size_t length)
    {
        if(m_request_ptr->body.size() + length > ms_maxBodySize)
        {
            m_errorCode = 413;
            return -1;    
        }

		m_request_ptr->body.append(at, length);
        return 0;
    }

    int HttpRequestDecoder::onHeadersComplete()
    {
		m_request_ptr->majorVersion = m_parser.http_major;
		m_request_ptr->minorVersion = m_parser.http_minor;
        
		m_request_ptr->method = (http_method)m_parser.method;

		m_request_ptr->parseUrl();

        return 0;    
    }

    int HttpRequestDecoder::onMessageComplete()
    {
		static uint32_t logId = 0;
		logId++;

		std::string traceIdField("X-Trace-Id");

		m_request_ptr->parseQuery();

		Command cmd;
		cmd.type = Command::recv_http_request;
		cmd.args.recv_http_request.iSerialNum = m_session_ptr->getSerialNum();
		cmd.args.recv_http_request.ptrData = m_request_ptr;

		std::stringstream ss;
		ss << "logid_" << CtxSingleton::get().launchTime() << "_" << logId;
		cmd.args.recv_http_request.ptrData->xLogRequestid = ss.str();

		Headers_t::iterator ite = m_request_ptr->headers.find(traceIdField);
		if (ite != m_request_ptr->headers.end())
		{
			cmd.args.recv_http_request.ptrData->xLogRequestid = ite->second;
		}

		ss.str("");

		ss << "iSerialNum:" << cmd.args.recv_http_request.iSerialNum
			<< " | " << "xLogRequestid:" << m_request_ptr->xLogRequestid
			<< " | " << "url:" << m_request_ptr->url 
			<< " | " << "method:" << m_request_ptr->method
			<< " | " << "headers:" << m_request_ptr->formatHeaders()
			<< " | " << "body:" << m_request_ptr->body;

		std::string tmp = ss.str();
		APie::ReplaceStrAll(tmp, "\r\n", "@r@n");
		ASYNC_PIE_LOG("http/request", PIE_CYCLE_HOUR, PIE_NOTICE, "%s", tmp.c_str());

		char* ptrDecode = APie::URLDecode(m_request_ptr->query.c_str());
		if (ptrDecode != NULL)
		{
			m_request_ptr->query = ptrDecode;
			free(ptrDecode);
		}
		
		ASYNC_PIE_LOG("http/request", PIE_CYCLE_HOUR, PIE_NOTICE, "query decode:%s", m_request_ptr->query.c_str());

		CtxSingleton::get().getLogicThread()->push(cmd);

		m_request_ptr = new HttpRequest();
        return 0;     
    }

    int HttpRequestDecoder::onUpgrade(const char* at, size_t length)
    {
		m_session_ptr->close("onUpgrade");
        return 0;    
    }

    int HttpRequestDecoder::onError(const HttpError& error)
    {
		Headers_t httpHead;
		httpHead.insert(Headers_t::value_type("Connection", "close"));

		std::stringstream ss;
		ss << "parse error|" << time(NULL) << std::endl;
		HttpResponse response(error.statusCode, httpHead, ss.str());
		std::string responseContent = response.dump();
		m_session_ptr->handleSend(responseContent.data(), responseContent.size());

		m_session_ptr->close(error.message);
        return 0;    
    }

}
