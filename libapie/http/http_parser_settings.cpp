#include "http_parser_settings.h"
#include "http_parser_impl.h"

namespace APie
{

	HttpParserSettings::HttpParserSettings()
	{
	}

	void HttpParserSettings::initSettings(struct http_parser_settings& settings)
	{
		settings.on_message_begin = &HttpParserSettings::onMessageBegin;
		settings.on_url = &HttpParserSettings::onUrl;
		settings.on_status = &HttpParserSettings::onStatus;
		settings.on_header_field = &HttpParserSettings::onHeaderField;
		settings.on_header_value = &HttpParserSettings::onHeaderValue;
		settings.on_headers_complete = &HttpParserSettings::onHeadersComplete;
		settings.on_body = &HttpParserSettings::onBody;
		settings.on_message_complete = &HttpParserSettings::onMessageComplete;
		settings.on_chunk_header = NULL;
		settings.on_chunk_complete = NULL;
	}

	int HttpParserSettings::onMessageBegin(struct http_parser* parser)
	{
		HttpParser* p = (HttpParser*)parser->data;
		return p->onParser(HttpParser::Parser_MessageBegin, 0, 0);
	}

	int HttpParserSettings::onUrl(struct http_parser* parser, const char* at, size_t length)
	{
		HttpParser* p = (HttpParser*)parser->data;
		return p->onParser(HttpParser::Parser_Url, at, length);
	}

	int HttpParserSettings::onStatus(struct http_parser* parser, const char* at, size_t length)
	{
		HttpParser* p = (HttpParser*)parser->data;
		return p->onParser(HttpParser::Parser_Status, at, length);
	}

	int HttpParserSettings::onHeaderField(struct http_parser* parser, const char* at, size_t length)
	{
		HttpParser* p = (HttpParser*)parser->data;
		return p->onParser(HttpParser::Parser_HeaderField, at, length);
	}

	int HttpParserSettings::onHeaderValue(struct http_parser* parser, const char* at, size_t length)
	{
		HttpParser* p = (HttpParser*)parser->data;
		return p->onParser(HttpParser::Parser_HeaderValue, at, length);
	}

	int HttpParserSettings::onHeadersComplete(struct http_parser* parser)
	{
		HttpParser* p = (HttpParser*)parser->data;
		return p->onParser(HttpParser::Parser_HeadersComplete, 0, 0);
	}

	int HttpParserSettings::onBody(struct http_parser* parser, const char* at, size_t length)
	{
		HttpParser* p = (HttpParser*)parser->data;
		return p->onParser(HttpParser::Parser_Body, at, length);
	}

	int HttpParserSettings::onMessageComplete(struct http_parser* parser)
	{
		HttpParser* p = (HttpParser*)parser->data;
		return p->onParser(HttpParser::Parser_MessageComplete, 0, 0);
	}
}
