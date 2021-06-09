#pragma once

#include <string>
#include <map>
#include <string.h>

extern "C"
{
struct http_parser;    
}

namespace APie
{

    class HttpRequest;
    class HttpResponse;

    class HttpParser;
    class HttpSession;

    //we use 599 for our tnet error


    class HttpError
    {
    public:
        HttpError(int code = 200, const std::string& m = std::string())
            : statusCode(code)
            , message(m)
        {}    

        //200 for no error
        int statusCode;
        std::string message;
    };
    
    enum WsEvent
    { 
        Ws_OpenEvent,
        Ws_CloseEvent, 
        Ws_MessageEvent,
        Ws_PongEvent,    
        Ws_ErrorEvent,
    };

    enum RequestEvent
    {
        Request_Upgrade, 
        Request_Complete,
        Request_Error,   
    };
    
    //Request_Upgrade: context is &StackBuffer
    //Request_Complete: context is 0
    //Request_Error: context is &HttpError

    enum ResponseEvent
    {
        Response_Complete,
        Response_Error,    
    };

    struct CaseKeyCmp
    {
        bool operator() (const std::string& p1, const std::string& p2) const
        {
#ifdef WIN32
            return _stricmp(p1.c_str(), p2.c_str()) < 0;
#else
			return strcasecmp(p1.c_str(), p2.c_str()) < 0;
#endif
        }    
    };

    typedef std::multimap<std::string, std::string, CaseKeyCmp> Headers_t;
    typedef std::multimap<std::string, std::string> Params_t;
}
