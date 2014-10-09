#include "http.h"

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

int readRequestLine(const char* line, size_t len, RequestResult &result)
{
	char* sp1 = (char*)memchr(line, ' ', len);
	if (sp1 == nullptr)
	{
		return -1;
	}
	
	if ((sp1 - line) == 3 || memcmp(line, "GET", 3) == 0)
	{
		result.method = HttpMethod::GET;
	}
	else if ((sp1 - line) == 4 || memcmp(line, "HEAD", 4) == 0)
	{
		result.method = HttpMethod::HEAD;
	}
	else
	{
		return -1;
	}
	
	len -= (sp1 + 1) - line;
	line = sp1 + 1;
	
	char* sp2 = (char*)memchr(line, ' ', len);
	if (sp2 == nullptr)
	{
		return -1;
	}
	
	const char* requestURI = line;
	size_t requestURILen = sp2 - line;
	
	len -= (sp2 + 1) - line;
	line = sp2 + 1;
	
	char* crlf = (char*)memchr(line, '\r', len);
	if (crlf == nullptr)
	{
		return -1;
	}
	
	const char* version = line;
	size_t versionLen = crlf - line;
	
	if (versionLen != 8 || (memcmp(version, "HTTP/1.0", 8) != 0
		&& memcmp(version, "HTTP/1.1", 8) != 0))
	{
		return -1;
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
		return -1;
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
	
	return 0;
}
