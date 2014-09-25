#pragma once
#include <cstdint>

class Config
{
	uint16_t portNr;
	char rootPath[1024];
	
public:
	Config(const char* filePath);
	
	uint16_t getPortNr() const; 
	const char* getRootPath() const;
};
