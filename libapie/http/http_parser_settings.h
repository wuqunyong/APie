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
	class HttpParserSettings
	{
	public:
		HttpParserSettings();

		static void initSettings(struct http_parser_settings& settings);

		static int onMessageBegin(struct http_parser*);
		static int onUrl(struct http_parser*, const char*, size_t);
		static int onStatus(struct http_parser*, const char*, size_t);
		static int onHeaderField(struct http_parser*, const char*, size_t);
		static int onHeaderValue(struct http_parser*, const char*, size_t);
		static int onHeadersComplete(struct http_parser*);
		static int onBody(struct http_parser*, const char*, size_t);
		static int onMessageComplete(struct http_parser*);
	};
}
