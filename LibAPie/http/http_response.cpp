#include "http_response.h"

#include <string.h>
#include <stdio.h>
#include <time.h>

#include "http_util.h"

using namespace std;

namespace APie
{

    HttpResponse::HttpResponse()
        : statusCode(200)
    {
    }

    HttpResponse::HttpResponse(int code, const Headers_t& headers, const string& body)
        : statusCode(code)
        , body(body)
        , headers(headers)
    {
        
    }

    HttpResponse::~HttpResponse()
    {
        
    }

    string HttpResponse::dump()
    {
        string str;
        
        char buf[1024];
        int n = snprintf(buf, sizeof(buf), "HTTP/1.1 %d %s\r\n", statusCode, HttpUtil::codeReason(statusCode).c_str());    

        str.append(buf, n);
    
        n = snprintf(buf, sizeof(buf), "%d", int(body.size()));

		//static const string ContentLength = "Content-Length";
        string ContentLength = "Content-Length";
		Headers_t::iterator ite = headers.find(ContentLength);
		if (ite == headers.end())
		{
			headers.insert(make_pair(ContentLength, string(buf, n)));
		}

		Headers_t::iterator it = headers.begin();
        while(it != headers.end())
        {
            n = snprintf(buf, sizeof(buf), "%s: %s\r\n", it->first.c_str(), it->second.c_str());
            str.append(buf, n);
            ++it;    
        }

        str.append("\r\n");
        str.append(body);

        return str;
    }     

    void HttpResponse::setContentType(const std::string& contentType)
    {
        static const string ContentTypeKey = "Content-Type";
        headers.insert(make_pair(ContentTypeKey, contentType));    
    }

    void HttpResponse::setKeepAlive(bool on)
    {
        static const string ConnectionKey = "Connection";
        if(on)
        {
            static const string KeepAliveValue = "Keep-Alive";
            headers.insert(make_pair(ConnectionKey, KeepAliveValue));    
        }    
        else
        {
            static const string CloseValue = "close";
            headers.insert(make_pair(ConnectionKey, CloseValue));    
        }
    }
    
    void HttpResponse::enableDate()
    {
        char buf[128];
		time_t now = time(NULL);
		size_t n = strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));

        static const string DateKey = "Date";
        headers.insert(make_pair(DateKey, string(buf, n)));
    }
}
