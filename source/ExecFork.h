#pragma once

#include "ExecErr.h"
#include "network.h"

class ExecFork
{
private:
	Network* network;

public:
	ExecErr init(Network* network);
	ExecErr shutdown();
	
	ExecErr run();
};

typedef ExecFork Exec;
