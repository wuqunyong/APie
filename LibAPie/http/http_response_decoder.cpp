#include "http_response_decoder.h"


#include "../network/connection.h"
#include "../network/command.h"
#include "../network/ctx.h"
#include "../network/logger.h"

#include "../common/string_utils.h"
#include "../network/client_connection.h"

namespace APie
{
    size_t HttpResponseDecoder::ms_maxHeaderSize = 80 * 1024;
    size_t HttpResponseDecoder::ms_maxBodySize = 10 * 1024 * 1024;

    HttpResponseDecoder::HttpResponseDecoder() :
		HttpParser(HTTP_RESPONSE)
    {
		m_response_ptr = new HttpResponse;
		m_session_ptr = NULL;
    }

    HttpResponseDecoder::~HttpResponseDecoder()
    {
    }
  
	void HttpResponseDecoder::setConnectSession(class ClientConnection *ptrSession)
	{
		this->m_session_ptr = ptrSession;
	}

    int HttpResponseDecoder::onMessageBegin()
    {
		m_response_ptr->clear();
        return 0;    
    }

    int HttpResponseDecoder::onHeader(const std::string& field, const std::string& value)
    {
        if(m_parser.nread >= ms_maxHeaderSize)
        {
            m_errorCode = 413;
            return -1;    
        }

		m_response_ptr->headers.insert(std::make_pair(field, value));
        return 0;
    }

    int HttpResponseDecoder::onHeadersComplete()
    {
		m_response_ptr->statusCode = m_parser.status_code;
        return 0;    
    }

    int HttpResponseDecoder::onBody(const char* at, size_t length)
    {
        if(m_response_ptr->body.size() + length >= ms_maxBodySize)
        {
            m_errorCode = 413;
            return  -1;
        }

		m_response_ptr->body.append(at, length);
        return 0;
    }

    int HttpResponseDecoder::onMessageComplete()
    {
		std::string traceIdField("X-Trace-Id");

		Headers_t::iterator ite = m_response_ptr->headers.find(traceIdField);
		if (ite != m_response_ptr->headers.end())
		{
			m_response_ptr->xLogRequestid = ite->second;
		}

		std::stringstream ss;
		ss << "iSerialNum:" << m_session_ptr->getSerialNum()
			<< " | " << "xLogRequestid:" << m_response_ptr->xLogRequestid
			<< " | " << "body:" << m_response_ptr->body;

		std::string tmp = ss.str();
		APie::ReplaceStrAll(tmp, "\r\n", "@r@n");
		ASYNC_PIE_LOG("http/recv_response", PIE_CYCLE_HOUR, PIE_NOTICE, "%s", tmp.c_str());

		Command cmd;
		cmd.type = Command::client_recv_packet;
		cmd.args.client_recv_packet.iSerialNum = m_session_ptr->getSerialNum();
		cmd.args.client_recv_packet.ptrData = m_response_ptr;
		CtxSingleton::get().getLogicThread()->push(cmd);

		m_response_ptr = new HttpResponse;

        return 0;    
    }

    int HttpResponseDecoder::onError(const HttpError& error)
    {
		m_session_ptr->close(error.message);
        return 0;    
    }

}
