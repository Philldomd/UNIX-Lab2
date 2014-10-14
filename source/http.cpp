#include "http.h"

#include <cstdio>
#include <cstring>

int identifyFragment(const char* string, size_t stringlen)
{
	const void *value;
	if((value = memchr(string, '#', stringlen)) == nullptr)
	{	
		return -1;
	}
	
	return (const char*)value - string;
} 

URIType identifyURI(const char* string, size_t stringlen)
{
	if(stringlen >= 2 && string[0] == '/' && string[1] == '/')
	{
		return URIType::NOT_IMPL;
	}
	if(stringlen >= 1 && string[0] == '/')
	{
		return URIType::RELATIVE;
	}
	return URIType::NOT_IMPL;
}

int findQuery(const char* string, size_t stringlen)
{
	const void *value;
	if((value = memchr(string, '?', stringlen)) == nullptr)
	{	
		return -1;
	}
	return (const char*)value - string;
}

int findParam(const char* string, size_t stringlen)
{
	const void *value;
	if((value = memchr(string, ';', stringlen)) == nullptr)
	{	
		return -1;
	}
	return (const char*)value - string;
}

bool isIn(char c, char low, char high)
{
	return (c >= low && c <= high);
}

bool isNum(char c)
{
	return isIn(c, '0', '9');
}

char numToInt(char c)
{
	return c - '0';
}

char lowerHexToInt(char c)
{
	return c - ('a' - 10);
}

char upperHexToInt(char c)
{
	return c - ('A' - 10);
}

int hexNumToInt(char c)
{
	if (isNum(c))
	{
		return numToInt(c);
	}
	else if (isIn(c, 'a', 'f'))
	{
		return lowerHexToInt(c);
	}
	else if (isIn(c, 'A', 'F'))
	{
		return upperHexToInt(c);
	}
	else
	{
		return -1;
	}
}

int unescape(const char* string, char* output, size_t stringlen)
{
	char* out = output;
	const char* prevPos = string;
	const char* curPos = string;
	const char* endPos = string + stringlen;
	while((curPos = (const char*)memchr(curPos, '%', endPos - curPos)) != nullptr)
	{
		size_t len = curPos - prevPos;
		memcpy(out, prevPos, len);
		out += len;
		
		if (endPos - curPos < 3)
		{
			return -1;
		}
		
		int high = hexNumToInt(curPos[1]);
		int low = hexNumToInt(curPos[2]);
		if (high == -1 || low == -1)
		{
			return -1;
		}
		
		char val = (high << 4) + low;
		*out = val;
		++out;
		
		curPos += 3;
		prevPos = curPos;
	}
	
	memcpy(out, prevPos, endPos - prevPos);
	out += endPos - prevPos;
	
	return out - output;
}

char* memlws(const char* s, size_t n)
{
	char* space = (char*)memchr(s, ' ', n);
	char* tab = (char*)memchr(s, '\t', n);
	
	if (space == nullptr)
	{
		return tab;
	}
	else if (tab == nullptr)
	{
		return space;
	}
	
	return tab < space ? tab : space;
}

char* memnonlws(const char* s, size_t n)
{
	const char* endPos = s + n;
	while (s < endPos)
	{
		if (*s != ' ' && *s != '\t')
		{
			return (char*)s;
		}
		
		++s;
	}
	
	return nullptr;
}

RequestErr readRequestLine(const char* line, size_t len, RequestResult &result)
{
	const char* endPos = line + len;
	
	char* ws1 = memlws(line, len);
	
	if (ws1 == nullptr)
	{
		return RequestErr::BAD_REQUEST;
	}
	
	size_t methodLen = ws1 - line;
	
	if (methodLen == 3 && memcmp(line, "GET", methodLen) == 0)
	{
		result.method = HttpMethod::GET;
	}
	else if (methodLen == 4 && memcmp(line, "HEAD", methodLen) == 0)
	{
		result.method = HttpMethod::HEAD;
	}
	else
	{
		return RequestErr::NOT_IMPLEMENTED;
	}
	
	char* nonWs = memnonlws(ws1, endPos - ws1);
	if (nonWs == nullptr)
	{
		return RequestErr::BAD_REQUEST;
	}
	
	char* ws2 = memlws(nonWs, endPos - nonWs);
	if (ws2 == nullptr)
	{
		// HTTP/0.9
		return RequestErr::NOT_IMPLEMENTED;
	}
	
	const char* requestURI = nonWs;
	size_t requestURILen = ws2 - nonWs;
	
	nonWs = memnonlws(ws2, endPos - ws2);
	if (nonWs == nullptr)
	{
		return RequestErr::BAD_REQUEST;
	}
	
	const char* version = nonWs;
	size_t versionLen = endPos - nonWs;
	
	if (versionLen != 8 || (memcmp(version, "HTTP/1.0", versionLen) != 0
		&& memcmp(version, "HTTP/1.1", versionLen) != 0))
	{
		printf("Strange version: %.8s\n", version);
		return RequestErr::NOT_IMPLEMENTED;
	}
	
	result.version = HttpVersion::V1_0;
	
	int fragPos = identifyFragment(requestURI, requestURILen);
	if (fragPos != -1)
	{
		requestURILen = fragPos;
	}
	
	URIType res = identifyURI(requestURI, requestURILen);
	if (res != URIType::RELATIVE)
	{
		return RequestErr::NOT_IMPLEMENTED;
	}
	
	int queryPos = findQuery(requestURI, requestURILen);
	if (queryPos != -1)
	{
		requestURILen = queryPos;
	}
	
	int paramPos = findParam(requestURI, requestURILen);
	if (paramPos != -1)
	{
		requestURILen = paramPos;
	}
	
	result.path = requestURI;
	result.pathLen = requestURILen;
	
	return RequestErr::OK;
}
