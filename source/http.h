#pragma once

#include <cstddef>

enum class URIType{
	RELATIVE,
	ABSOLUTE,
	NOT_IMPL,
	ERROR = -1
};

int identifyFragment(const char* string, size_t stringlen);
URIType identifyURI(const char* string, size_t stringlen);
int findQuery(const char* string, size_t stringlen);
int findParam(const char* string, size_t stringlen);
int unescape(const char* string, char* output, size_t stringlen);

enum class HttpMethod
{
	GET,
	HEAD,
};

enum class HttpVersion
{
	V0_9 = 0x0009,
	V1_0 = 0x0109,
};

struct RequestResult
{
	HttpMethod method;
	HttpVersion version;
	const char* path;
	size_t pathLen;
};

enum class RequestErr
{
	OK = 200,
	BAD_REQUEST = 400,
	FORBIDDEN = 403,
	NOT_FOUND = 404,
	INTERNAL = 500,
	NOT_IMPLEMENTED = 501,
};

RequestErr readRequestLine(const char* line, size_t len, RequestResult &result);
